// 方向変更の遅延時間（ミリ秒）
#define DIRECTION_DELAY 1000  // 1秒の遅延

// カメラ用UARTバッファ（デフォルト64バイト→256バイトに拡張）
// addMemoryForRead はbegin()より前に呼ぶ必要があるため、バッファはstaticで確保
static uint8_t cam1_rx_buf[256];
static uint8_t cam2_rx_buf[256];
static uint8_t cam3_rx_buf[256];
static uint8_t cam4_rx_buf[256];
static uint8_t line_rx_buf[256];

//セットアップ処理
void setup() {
  /* 初期化処理の流れ
    1. シリアル通信の初期化（PC + 4台のカメラ + モーター）
    2. ピンモードの設定（LED + スイッチ類）
    3. SDカードの初期化（ロギング有効時のみ）
    4. I2C通信の開始
    5. BNO055の初期化
    6. OLEDディスプレイの初期化
    7. 設定値の読み込み
    8. デバッグモードの開始
    
    エラー処理：
    - 各デバイスの初期化失敗時は無限ループで停止
    - エラー内容はシリアル出力で確認可能
    */
  Serial.begin(9600);     //PC
  Serial1.addMemoryForRead(cam1_rx_buf, sizeof(cam1_rx_buf));
  Serial1.begin(38400);  //cam1
  Serial2.begin(115200);  //motor
  Serial3.addMemoryForRead(cam3_rx_buf, sizeof(cam3_rx_buf));
  Serial3.begin(38400);  //cam3
  Serial4.addMemoryForRead(cam4_rx_buf, sizeof(cam4_rx_buf));
  Serial4.begin(38400);  //cam4
  Serial5.addMemoryForRead(cam2_rx_buf, sizeof(cam2_rx_buf));
  Serial5.begin(38400);  //cam2
  Serial6.addMemoryForRead(line_rx_buf, sizeof(line_rx_buf));
  Serial6.begin(115200);  //line Sensor
  Serial.println("Info: Setup started");
  botNam = 1;          //ロボットの機体番号を指定
  loggingMode = true;  //ログを残すか否か
  debugMode = false;    //デバッグモードか否か

  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  pinMode(sw1_pin, INPUT_PULLUP);
  pinMode(sw2_pin, INPUT_PULLUP);
  pinMode(sw3_pin, INPUT_PULLUP);
  pinMode(sw4_pin, INPUT_PULLUP);
  pinMode(tg1_pin, INPUT_PULLUP);
  pinMode(tg2_pin, INPUT_PULLUP);
  pinMode(tg3_pin, INPUT_PULLUP);
  pinMode(ballSensor_pin, INPUT);
  pinMode(kicker, OUTPUT);
  pinMode(dribbler_pin, OUTPUT);
  pinMode(buzzer, OUTPUT);
  initNeoPixelRing();

  if (loggingMode) {
    // SDカードの初期化を複数回試行
    bool sdInitialized = false;
    for (int retry = 0; retry < 3; retry++) {
      if (SD.begin(SdioConfig(FIFO_SDIO))) {
        sdInitialized = true;
        
        // クラッシュレポートがある場合は保存する
        if (CrashReport) {
          FsFile crashFile = SD.open("crash_report.txt", O_RDWR | O_CREAT | O_APPEND);
          if (crashFile) {
            crashFile.println("\n========================================");
            crashFile.println("CRASH REPORT");
            crashFile.println("========================================");
            crashFile.print(CrashReport);
            crashFile.println("========================================");
            crashFile.close();
            Serial.println("Info: Crash report saved to crash_report.txt");
          } else {
            Serial.println("Error: Failed to save crash report");
          }
        }

        logging("Info: SD card initialized successfully");
           Serial.println("Info: SD card initialized successfully");
        break;
      }
      delay(100);
    }

    if (!sdInitialized) {
         Serial.println("Error: SD card initialization failed");
      loggingMode = false;
    }
  }

  Wire.begin();
  Wire.setClock(400000);
     Serial.println("Info: I2C initialization successful");
  // BNO055を初期化
  if (!bno.begin(OPERATION_MODE_IMUPLUS)) {
       Serial.println("Error: BNO055 initialization failed");
    while (1)
      ;
  }
     Serial.println("Info: BNO055 initialized in IMUPLUS mode");
  // 保存済みキャリブレーションがあれば起動時に流し込む
  if (loadBnoCalibrationOffsets()) {
       Serial.println("Info: BNO055 calibration offsets loaded from SD");
  } else {
       Serial.println("Info: BNO055 calibration offsets not found; waiting for live calibration");
  }

  //SSD1306_SWITCHCAPVCC = 内部で3.3Vからディスプレイ電圧を生成する
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
       Serial.println("Error: SSD1306 allocation failed");
    for (;;)
      ;  //先に進まないで、永遠にループする
  }
     Serial.println("Info: OLED initialization successful");

  // 重みの読み込み
     Serial.println("Info: Starting to load weights...");
  loadWeights();
     Serial.println("Info: Weights loading process completed");

  display.clearDisplay();  //バッファをクリアする
  display.display();       //ディスプレイに表示
  getSetting();

  // サブマイコン起動待ち後、閾値を送信
  delay(500);
  sendLineThresholds();
  Serial.println("Info: Line thresholds sent to sub-MCU");

  //Serial.print(botSpeed);
  if (switch1 == HIGH) {
    debugSerial();
  }
  //debugOLED();
  //debugSerial();
}

void readyForPlay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  // キャリブレーション待機
  if (!waitForCalibration(300000)) {  // 300秒のタイムアウト
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Calibrating...");
    display.println("timeout!");
    display.display();
    logging("Error: BNO055 calibration timeout");
    delay(2000);
  } else {
    if (bno.isFullyCalibrated() && saveBnoCalibrationOffsets()) {
      logging("Info: BNO055 calibration saved");
      Serial.println("Info: BNO055 calibration saved");
    }
  }

  // スイッチ1が押されるまでループ
  while (switch1 == LOW) {
    // スイッチ2でモード切り替え
    if (switch2) {
      isDribblerMode = !isDribblerMode;
      analogWrite(buzzer, 128);
      delay(200);  // チャタリング防止
      analogWrite(buzzer, 0);
    }
    // スイッチ3でデバッグモード切り替え
    if (switch3) {
      debugMode = !debugMode;
      analogWrite(buzzer, 128);
      delay(200);  // チャタリング防止
      analogWrite(buzzer, 0);
    }
    // スイッチ4でゴール方向をセット（toggle2で青/黄を選択してから押す）
    if (switch4) {
      setForwardDirection();
    }

    updateDebugModeNeoPixelIndicator();

    // OLED表示の更新
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Ready...");
    display.print("Mode: ");
    display.println(isDribblerMode ? "Dribbler" : "Kicker");
    display.print("Debug: ");
    display.println(debugMode ? "ON" : "OFF");
    display.print("Dir: ");
    display.println(forwardDirectionSet ? "SET" : "NOT SET");
    display.display();

    // LED点滅
    digitalWrite(led, HIGH);
    delay(100);
    digitalWrite(led, LOW);
    delay(100);
  }

  clearNeoPixelRing();
  display.clearDisplay();
  display.display();
  setGoalDirection();
}

// sw4ボタンで呼び出す: 現在の向きをゴール方向として登録する
// toggle2がON(青攻め)なら blueDirection=0, yellowDirection=180
// toggle2がOFF(黄攻め)なら yellowDirection=0, blueDirection=180
void setForwardDirection() {
  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  gyroOffset = (int)euler.x();

  if (toggle2) {
    blueDirection = 0;     // 今向いている方向 = 青ゴール
    yellowDirection = 180; // 反対方向 = 黄ゴール
  } else {
    yellowDirection = 0;   // 今向いている方向 = 黄ゴール
    blueDirection = 180;   // 反対方向 = 青ゴール
  }

  forwardDirectionSet = true;
  analogWrite(buzzer, 128);
  delay(300);
  analogWrite(buzzer, 0);
}

void setGoalDirection() {
  if (toggle2) {
    goalDirection = blueDirection;  //青の方向をゴール方向に設定
  } else {
    goalDirection = yellowDirection;  //黄の方向をゴール方向に設定
  }
}

//SDカードから設定データ取得
void getSetting() {
  if (!loggingMode) {
    // デフォルト値を設定
    blueDirection = 0;
    yellowDirection = 180;
    botSpeed = 100;
    blueGoalEnabled = true;
    yellowGoalEnabled = true;
       Serial.println("Warning: Using default settings (SD card unavailable)");
    return;
  }

  // 設定ファイルを開く前にSDカードの状態を確認
  if (!SD.exists("setting.txt")) {
       Serial.println("Info: Creating new settings file");
    writeSetting();  // デフォルト値で新しい設定ファイルを作成
    return;
  }

  FsFile settingFile = SD.open("setting.txt", FILE_READ);
  if (!settingFile) {
    // 設定ファイルが存在しない場合、新規作成
    blueDirection = 0;
    yellowDirection = 180;
    botSpeed = 100;
    kickerThreshold = 80;  // デフォルト値を設定
    line1Threshold = 100;
    line2Threshold = 100;
    line3Threshold = 100;
    line4Threshold = 100;
    dribblerSpeed = 250;
    blueGoalEnabled = true;
    yellowGoalEnabled = true;
    writeSetting();        // 新しい設定を保存
    logging("Info: Created new settings file");
       Serial.println("Info: Created new settings file");
    return;
  }

  // バッファサイズを確保
  char buffer[32];
  int settingCount = 0;

  while (settingFile.available() && settingCount < 13) {
    if (settingFile.fgets(buffer, sizeof(buffer))) {
      // 改行文字を削除
      char* newline = strchr(buffer, '\n');
      if (newline) *newline = 0;

      int value = atoi(buffer);

      switch (settingCount) {
        case 0:
          blueDirection = constrain(value, 0, 359);
          break;
        case 1:
          yellowDirection = constrain(value, 0, 359);
          break;
        case 2:
          botSpeed = constrain(value, 0, 200);
          break;
        case 3: // kickerThresholdの読み込み
          kickerThreshold = constrain(value, 0, 1023);
          break;
        case 4: // line1Thresholdの読み込み
          line1Threshold = constrain(value, 0, 1023); // 0-1023の範囲に制限
          break;
        case 5: // line2Thresholdの読み込み
          line2Threshold = constrain(value, 0, 1023); // 0-1023の範囲に制限
          break;
        case 6: // line3Thresholdの読み込み
          line3Threshold = constrain(value, 0, 1023); // 0-1023の範囲に制限
          break;
        case 7: // line4Thresholdの読み込み
          line4Threshold = constrain(value, 0, 1023); // 0-1023の範囲に制限
          break;
        case 8: // dribblerSpeedの読み込み
          dribblerSpeed = constrain(value, 0, 255); // 0-255の範囲に制限
          break;
        case 9: // 青ゴール認識の読み込み
          blueGoalEnabled = (value == 1);
          break;
        case 10: // 黄ゴール認識の読み込み
          yellowGoalEnabled = (value == 1);
          break;
        case 11: // 左回転の最低パワー
          minRotatePowerLeft = constrain(value, 1, 100);
          break;
        case 12: // 右回転の最低パワー
          minRotatePowerRight = constrain(value, 1, 100);
          break;
        default:
          // settingCountが11以上の場合、または不明なcaseの場合
          break;
      }
      settingCount++;
    }
  }

  settingFile.close();
  logging("Info: Settings loaded successfully");
}

void writeSetting() {
  if (!loggingMode) {
       Serial.println("Warning: Cannot write settings (SD card unavailable)");
    return;
  }

  // O_TRUNCを追加して、ファイルを新規作成モードで開く
  FsFile settingFile = SD.open("setting.txt", O_RDWR | O_CREAT | O_TRUNC);
  if (!settingFile) {
    logging("Error: Failed to open settings file for writing");
       Serial.println("Error: Failed to open settings file for writing");
    return;
  }

  // 設定値を書き込む
  settingFile.println(blueDirection);
  settingFile.println(yellowDirection);
  settingFile.println(botSpeed);
  settingFile.println(kickerThreshold);
  settingFile.println(line1Threshold);
  settingFile.println(line2Threshold);
  settingFile.println(line3Threshold);
  settingFile.println(line4Threshold);
  settingFile.println(dribblerSpeed);
  settingFile.println(blueGoalEnabled ? 1 : 0);  // 青ゴール認識
  settingFile.println(yellowGoalEnabled ? 1 : 0);  // 黄ゴール認識
  settingFile.println(minRotatePowerLeft);   // 左回転の最低パワー
  settingFile.println(minRotatePowerRight);  // 右回転の最低パワー
  settingFile.close();

  logging("Info: Settings saved successfully");
}
