//デバッグ系

// スイッチによる増減値の定義
const int DECREMENT_LARGE = -10;
const int DECREMENT_SMALL = -1;
const int INCREMENT_SMALL = 1;
const int INCREMENT_LARGE = 10;

void debugSerial() {  //PCを使ったデバッグ
  debugType = 0;
  Serial.begin(115200);
  Serial.println("This is debug mode.");
  
  for (;;) {
    Serial.println("What do you want to do?");
    while (!Serial.available()) {}  //何もPCから受信がない場合は待ち続ける
    byte debugNum = Serial.read();
    titleFlag = false;  //項目のタイトルはまだ表示されていないのでfalse
    
    if (debugNum == 's') { // 's'が入力されたら設定モードに入る
      Serial.println("Entering setting mode...");
      for(;;){
        Serial.println("Select setting item:");
        Serial.println("1: Goal direction");
        Serial.println("5: Kicker threshold");
        Serial.println("6: Robot speed");
        Serial.println("8: Line sensor threshold");
        Serial.println("e: Exit setting mode");
        while (!Serial.available()) {}
        byte settingNum = Serial.read();
        if (settingNum == 'e') {
          Serial.println("Exiting setting mode...");
          break;
        }
        setThreshold(settingNum);
      }
    } else {
      for (;;) {
        debug(debugNum);  //デバッグ実行
        
        // シリアルからの新しい入力をチェック
        if (Serial.available()) {
          byte input = Serial.read();
          if (input == 'e') {
            Serial.println("\nReturning to main menu...");
            break;  // 内側のループを抜けてメインメニューに戻る
          }
          debugNum = input;
          titleFlag = false;  // 新しいデバッグモードのためタイトルフラグをリセット
          Serial.println("\nSwitching debug mode...");
        }
      }
    }
  }
}

void debugOLED() {  //OLEDを使ったデバッグ
  debugType = 1;
  byte debugNum = 1;
  while(!toggle3) {
    while ((switch1 == LOW && switch2 == LOW) || !(toggle1)) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      // debugNum 1-9 → '1'-'9'、10以降 → 'a','b',...
      byte caseKey = (debugNum <= 9) ? (debugNum + '0') : ('a' + (debugNum - 10));
      if (toggle1) {
        debug(caseKey);  //デバッグ実行
      } else {
        setThreshold(caseKey);  //閾値設定
      }
      display.display();
    }
    if (switch1 == HIGH) {
      if (debugNum != 1) {
        debugNum--;
      }
    } else {
      if (debugNum < 10) debugNum++;  // 上限: 10='a'

    }
    delay(50);
    while (switch1 || switch2) {
      delay(0);
      digitalWrite(led, HIGH);
      analogWrite(buzzer, 128); 
    }
    digitalWrite(led, LOW);
    analogWrite(buzzer, 0);
  }
  brake(true);
  display.clearDisplay();
  display.display();
}

void debugPrint(String thing) {  //シリアルまたはOLEDに文字を表示
  if (debugType == 0) {
    Serial.print(thing);  //シリアル通信で表示
  } else {
    display.print(thing);  //OLEDに表示
  }
}

void debugPrintln(String thing) {  //シリアルまたはOLEDに文字を表示(改行付き)
  if (debugType == 0) {
    Serial.println(thing);  //シリアル通信で表示
  } else {
    display.println(thing);  //OLEDに表示
  }
}

void debugPrintTitle(String thing) {  //シリアルまたはOLEDにタイトル表示(シリアルの場合は初回だけ)
  if (debugType == 0) {
    if (!titleFlag) {
      Serial.println(thing);
      titleFlag = true;
    }
  } else {
    display.println(thing);  // OLEDの場合は毎回表示
  }
}

void debug(byte num) {  //デバッグの実行
  switch (num) {
    case '1':  //BNO055 コンパスチェック
      debugPrintTitle("directionCheck");
      debugPrint("direction : ");
      debugPrintln(currentDirection());
      break;
    case '2':  //カメラチェック
      debugPrintTitle("Camera Check");
      if (!titleFlag) {  // 初回のみ表示
        debugPrintln("Press SW3/send 's' to start");
        titleFlag = true;
      }
      
      if (switch3 || (debugType == 0 && Serial.available() && Serial.read() == 's')) {
        while (switch3) {
          digitalWrite(led, HIGH);
          analogWrite(buzzer, 128);
        }
        digitalWrite(led, LOW);
        analogWrite(buzzer, 0);
        
        const byte NUM_CAM_MODES = 5;
        const char* camModeNames[] = {
          "Object Check",
          "Cam Position",
          "Pixel Check",
          "FPS Check",
          "Position Map"
        };
        byte checkMode = 1;
        byte readCam = 1;
        byte objectType = 0;
        bool modeMenuShown = false;  // モードメニュー表示フラグ
        
        while (!shouldExit()) {
          if (debugType == 1) {  // OLED表示の初期化
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 0);
            modeMenuShown = false;  // OLED使用時は毎回表示更新
          }
          
          if (!modeMenuShown) {  // メニューが未表示の場合のみ表示
            debugPrintTitle("Select Mode:");
            // スライドウィンドウ: 3モードを表示（選択モードが中央に来るよう調整）
            byte windowStart = (byte)constrain((int)checkMode - 1, 1, (int)NUM_CAM_MODES - 2);
            for (byte wi = windowStart; wi <= windowStart + 2; wi++) {
              debugPrint(checkMode == wi ? ">" : " ");
              debugPrintln(camModeNames[wi - 1]);
            }
            modeMenuShown = true;
          }

          if (switch1 || switch2) {  // モード切り替え（SW1:前へ / SW2:次へ）
            if (switch1) {
              checkMode = (checkMode <= 1) ? NUM_CAM_MODES : checkMode - 1;
            } else {
              checkMode = (checkMode >= NUM_CAM_MODES) ? 1 : checkMode + 1;
            }
            analogWrite(buzzer, 128);
            while (switch1 || switch2) {
              digitalWrite(led, HIGH);
            }
            digitalWrite(led, LOW);
            analogWrite(buzzer, 0);
          }
          
          if (debugType == 1) {
            display.display();
          }
          
          // SW3で選択確定
          if (switch3 || (Serial.available() && Serial.read() == 's')) {
            analogWrite(buzzer, 128);
            while (switch3) {
              digitalWrite(led, HIGH);
            }
            digitalWrite(led, LOW);
            analogWrite(buzzer, 0);
            
            // 選択したモードの処理
            while (!shouldExit()) {
              allCamProcess();
              updateDirectionNeoPixelDebug();
              if (debugType == 1) {  // OLED表示の初期化
                display.clearDisplay();
                display.setTextSize(1);
                display.setTextColor(SSD1306_WHITE);
                display.setCursor(0, 0);
              }

              if (checkMode == 1) {  // オブジェクト検出チェック
                String objectName;
                switch(objectType) {
                  case 0: objectName = "Ball"; break;
                  case 1: objectName = "Blue"; break;
                  case 2: objectName = "Yellow"; break;
                }
                
                debugPrintTitle(objectName);
                int globalX = 255;
                int globalY = 255;
                predictCoordinatesGlobal(&globalX, &globalY);
                int objTheta = (objectType == 0 ? ballTheta :
                               objectType == 1 ? blueTheta : yellowTheta);
                int objDist = (objectType == 0 ? ballDistance :
                              objectType == 1 ? blueDistance : yellowDistance);
                int bestCam = find(objectType);
                int objX = 0;
                int objY = 0;
                if (bestCam > 0) {
                  if (objectType == 0) {
                    objX = ballX[bestCam - 1];
                    objY = ballY[bestCam - 1];
                  } else if (objectType == 1) {
                    objX = blueX[bestCam - 1];
                    objY = blueY[bestCam - 1];
                  } else {
                    objX = yellowX[bestCam - 1];
                    objY = yellowY[bestCam - 1];
                  }
                }
                if (debugType == 1) {
                  // OLEDは3行まで
                  debugPrint("A:");
                  debugPrint(String(objTheta));
                  debugPrint(" D:");
                  debugPrintln(String(objDist));
                  debugPrint("GX:");
                  debugPrint(String(globalX));
                  debugPrint(" GY:");
                  debugPrintln(String(globalY));
                  if (bestCam > 0) {
                    debugPrint("C:");
                    debugPrint(String(bestCam));
                    debugPrint(" X:");
                    debugPrint(String(objX));
                    debugPrint(" Y:");
                    debugPrintln(String(objY));
                  } else {
                    debugPrintln("C:None");
                  }
                } else {
                  debugPrint("Ang:");
                  debugPrintln(String(objTheta));
                  debugPrint("Dis:");
                  debugPrintln(String(objDist));
                  debugPrint("GX:");
                  debugPrintln(String(globalX));
                  debugPrint("GY:");
                  debugPrintln(String(globalY));
                  if (bestCam > 0) {
                    debugPrint("Cam:");
                    debugPrintln(String(bestCam));
                    debugPrint("X:");
                    debugPrintln(String(objX));
                    debugPrint("Y:");
                    debugPrintln(String(objY));
                  } else {
                    debugPrintln("Cam:None");
                  }
                }
                /*debugPrint("Cam:");
                debugPrintln(String(find(objectType)));*/

                if (objectType == 0) { // ボールの場合のみ下限Y座標を表示
                  int bestCamForBall = find(0);
                  if (bestCamForBall > 0) { // 有効なカメラがある場合
                    int ballLowerY = ballY[bestCamForBall - 1] + (ballH[bestCamForBall - 1] / 2);
                    (void)ballLowerY;
                    debugPrint("Cam : ");
                    debugPrintln(String(bestCamForBall));
                  }
                }
                
                // SW1/SW2でオブジェクト切り替え
                if (switch1 || switch2) {
                  if (switch1) {
                    objectType = (objectType == 0) ? 2 : objectType - 1;
                  } else {
                    objectType = (objectType == 2) ? 0 : objectType + 1;
                  }
                  while (switch1 || switch2) {
                    digitalWrite(led, HIGH);
                    analogWrite(buzzer, 128);
                  }
                  digitalWrite(led, LOW);
                  analogWrite(buzzer, 0);
                }
                
              } else if(checkMode == 2) {  // カメラ座標チェック
                debugPrintTitle("Cam" + String(readCam));
                debugPrint("X:");
                debugPrintln(String(ballX[readCam - 1]));
                debugPrint("Y:");
                debugPrintln(String(ballY[readCam - 1]));
                int ballLowerY = ballY[readCam - 1] + (ballH[readCam - 1] / 2);
                debugPrint("Ball Lower Y");
                debugPrintln(String(ballLowerY));
                
                // SW1/SW2でカメラ切り替え
                if (switch1 || switch2) {
                  if (switch1) {
                    if (readCam != 1) readCam--;
                  } else {
                    if (readCam != 4) readCam++;
                  }
                  while (switch1 || switch2) {
                    digitalWrite(led, HIGH);
                    analogWrite(buzzer, 128);
                  }
                  digitalWrite(led, LOW);
                  analogWrite(buzzer, 0);
                }
              } else if (checkMode == 3) {  // PixelCheckモード
                debugPrintTitle("PixelCheck");
                debugPrint("Cam: ");
                debugPrintln(String(readCam));
                String objectName;
                int pixelValue = 0;
                switch (objectType) {
                  case 0:
                    objectName = "Ball";
                    pixelValue = ballP[readCam - 1];
                    break;
                  case 1:
                    objectName = "Blue";
                    pixelValue = blueP[readCam - 1];
                    break;
                  case 2:
                    objectName = "Yellow";
                    pixelValue = yellowP[readCam - 1];
                    break;
                }
                debugPrint("Obj: ");
                debugPrintln(objectName);
                debugPrint("Area(W*H): ");  // P配列はbbox面積(w*h)に変更
                debugPrintln(String(pixelValue));

                // SW1/SW2でカメラ切り替え
                if (switch1 || switch2) {
                  if (switch1) {
                    if (readCam > 1) readCam--;
                  } else {
                    if (readCam < 4) readCam++;
                  }
                  while (switch1 || switch2) {
                    digitalWrite(led, HIGH);
                    analogWrite(buzzer, 128);
                  }
                  digitalWrite(led, LOW);
                  analogWrite(buzzer, 0);
                }

                // SW3でオブジェクト切り替え
                if (switch3) {
                  objectType = (objectType + 1) % 3;  // 0→1→2→0の循環
                  while (switch3) {
                    digitalWrite(led, HIGH);
                    analogWrite(buzzer, 128);
                  }
                  digitalWrite(led, LOW);
                  analogWrite(buzzer, 0);
                }
              } else if (checkMode == 4) {  // FPS確認モード
                debugPrintTitle("FPS Check");
                for (int i = 0; i < 4; i++) {
                  debugPrint("Cam");
                  debugPrint(String(i + 1));
                  debugPrint(":");
                  debugPrint(String(fps[i]));
                  if (i < 3) debugPrint(" ");
                }
                debugPrintln("");
              } else if (checkMode == 5) {  // Position Map: コート上の自己位置を可視化
                int globalX = 255, globalY = 255;
                predictCoordinatesGlobal(&globalX, &globalY);

                if (debugType == 1) {
                  // コートエリア (x=0, y=1, w=40, h=30)
                  // フィールド Y(-121.5..+121.5) → screen X(0..39): 左=味方ゴール, 右=敵ゴール
                  // フィールド X(-91..+91)       → screen Y(1..30): 上=左端, 下=右端
                  display.drawRect(0, 1, 40, 30, SSD1306_WHITE);
                  // センターライン (フィールドY=0)
                  display.drawFastVLine(20, 1, 30, SSD1306_WHITE);

                  if (globalX != 255 && globalY != 255) {
                    int rx = constrain((int)((globalY + 121.5f) / 243.0f * 40.0f), 0, 39);
                    int ry = constrain(1 + (int)((globalX + 91.0f) / 182.0f * 30.0f), 1, 30);
                    display.fillCircle(rx, ry, 1, SSD1306_WHITE);
                  }

                  // テキストエリア (x=42〜)
                  display.setCursor(42, 0);
                  display.print("PosMap");
                  display.setCursor(42, 8);
                  if (globalX != 255) {
                    display.print("X:");
                    display.print(globalX);
                  } else {
                    display.print("X:--");
                  }
                  display.setCursor(42, 16);
                  if (globalY != 255) {
                    display.print("Y:");
                    display.print(globalY);
                  } else {
                    display.print("Y:--");
                  }
                  display.setCursor(42, 24);
                  display.print("G:");
                  display.print(goalColorDistance > 0 ? "MY" : "--");
                  if (opponentColorDistance > 0) display.print("+OP");
                } else {
                  debugPrintTitle("Position Map");
                  debugPrint("X:");
                  debugPrintln(globalX == 255 ? "--" : String(globalX));
                  debugPrint("Y:");
                  debugPrintln(globalY == 255 ? "--" : String(globalY));
                  debugPrint("Goal:");
                  debugPrintln(String(goalColorDistance));
                  debugPrint("Opp:");
                  debugPrintln(String(opponentColorDistance));
                }
              }

              if (debugType == 1) {
                display.display();
              }
            }
          }
        }
        exitRoutine();  // 終了処理を共通化
      }
      break;
    case '3':  // echo超音波距離センサーチェック
      debugPrintTitle("echoCheck");
      if (!titleFlag) {  // 初回のみ表示
        debugPrintln("Press SW3/send 's' to start");
        titleFlag = true;
      }
      debugPrint("PressSW3 or send 's' to start");
      if (switch3 || (Serial.available() && Serial.read() == 's')) {
          if (switch3 || (Serial.available() && Serial.read() == 's')) {
            analogWrite(buzzer, 128);
            while (switch3) {
              digitalWrite(led, HIGH);
            }
            digitalWrite(led, LOW);
            analogWrite(buzzer, 0);
        }
        byte readEcho = 1;
        while (!switch4 && !toggle3 && !(Serial.available() && Serial.read() == 'e')) {
          if (debugType == 1) {
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 0);
          }
          debugPrintTitle("echo" + (String)readEcho + "Check");
          debugPrint("echo" + (String)readEcho + " : ");
          debugPrintln(getEcho(trigPins[readEcho - 1], echoPins[readEcho - 1]));
          if (switch1 || switch2) {  // センサー切り替えはSW1/SW2のまま
            if (switch1) {
              if (readEcho != 1) {
                readEcho--;
              }
            } else {
              if (readEcho != 4) {
                readEcho++;
              }
            }
            analogWrite(buzzer, 128);
            while (switch1 || switch2) {
              digitalWrite(led, HIGH);
            }
            digitalWrite(led, LOW);
            analogWrite(buzzer, 0);
          }

          if (debugType == 1) {
            display.display();
          }
        }
        while (switch4) {
          digitalWrite(led, HIGH);
          analogWrite(buzzer, 128); 
        }
        digitalWrite(led, LOW);
        analogWrite(buzzer, 0);
      }
      break;
    case '4':  // ボールファインド。4つのカメラのうち、ボールがカメラの中心付近に見えているカメラを一つ選ぶ。
      allCamProcess();
      debugPrintTitle("ballFind");
      debugPrint("camFlag_ball : ");
      debugPrintln(find(0));  // 0を渡してボールを探す場合
      break;
    case '5':  //デジタルチェック。ボタンやトグルの状態を確認。
      debugPrintTitle("digitalCheck");
      debugPrint("sw1 : ");
      debugPrint(switch1);
      debugPrint(" sw4 : ");
      debugPrintln(switch4);
      debugPrint("sw2 : ");
      debugPrint(switch2);
      debugPrint(" tg1 : ");
      debugPrintln(toggle1);
      debugPrint("sw3 : ");
      debugPrint(switch3);
      debugPrint(" tg2 : ");
      debugPrintln(toggle2);
      break;
    case '6':  //モーターチェック
      debugPrintTitle("motorCheck");
      debugPrint("PressSW3 or send 's' to start");
      if (switch3 || (Serial.available() && Serial.read() == 's')) {
        while (switch3) {
          digitalWrite(led, HIGH);
          analogWrite(buzzer, 128);
        }
        digitalWrite(led, LOW);
        analogWrite(buzzer, 0);
        byte checkThing = 1;
        while (!switch4 && !toggle3 && !(Serial.available() && Serial.read() == 'e')) {
          if (debugType == 1) {
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 0);
          }
          switch (checkThing) {
            case 1:  //個別モーターチェック
              debugPrintTitle("motorSingleCheck");
              debugPrint("PressSW3 or send 's' to start");
              if (switch3 || (Serial.available() && Serial.read() == 's')) {
                analogWrite(buzzer, 128);
                while (switch3) {
                  digitalWrite(led, HIGH);
                }
                digitalWrite(led, LOW);
                analogWrite(buzzer, 0);
                
                byte currentMotor = 1; // 1:FL, 2:FR, 3:BL, 4:BR
                bool isForward = true; // true:正転, false:逆転
                
                while (!switch4 && !toggle3 && !(Serial.available() && Serial.read() == 'e')) {
                  if (Serial.available() && Serial.read() == 's') break;
                  
                  if (debugType == 1) {
                    display.clearDisplay();
                    display.setTextSize(1);
                    display.setTextColor(SSD1306_WHITE);
                    display.setCursor(0, 0);
                  }
                  
                  debugPrintTitle("motorSingleCheck");
                  
                  // モーター名を表示
                  String motorName;
                  switch(currentMotor) {
                    case 1: motorName = "FL"; break;
                    case 2: motorName = "FR"; break;
                    case 3: motorName = "BL"; break;
                    case 4: motorName = "BR"; break;
                  }
                  debugPrint("Motor: ");
                  debugPrintln(motorName);
                  debugPrint("Direction: ");
                  debugPrintln(isForward ? "Forward" : "Reverse");
                  debugPrintln("running...");
                  
                  // SW1/SW2でモーター切り替え
                  if (switch1) {
                    if (currentMotor > 1) {
                      currentMotor--;
                      analogWrite(buzzer, 128);
                      while (switch1) {
                        digitalWrite(led, HIGH);
                      }
                      digitalWrite(led, LOW);
                      analogWrite(buzzer, 0);
                    }
                  }
                  if (switch2) {
                    if (currentMotor < 4) {
                      currentMotor++;
                      analogWrite(buzzer, 128);
                      while (switch2) {
                        digitalWrite(led, HIGH);
                      }
                      digitalWrite(led, LOW);
                      analogWrite(buzzer, 0);
                    }
                  }
                  
                  // SW3で回転方向切り替え
                  if (switch3) {
                    isForward = !isForward;
                    analogWrite(buzzer, 128);
                    while (switch3) {
                      digitalWrite(led, HIGH);
                    }
                    digitalWrite(led, LOW);
                    analogWrite(buzzer, 0);
                  }
                  
                  // モーター制御
                  int speed = isForward ? 50 : -50;
                  switch(currentMotor) {
                    case 1: forced_move(speed, 0, 0, 0, false); break;  // FL
                    case 2: forced_move(0, speed, 0, 0, false); break;  // FR
                    case 3: forced_move(0, 0, speed, 0, false); break;  // BL
                    case 4: forced_move(0, 0, 0, speed, false); break;  // BR
                  }
                  
                  if (debugType == 1) {
                    display.display();
                  }
                }
                
                brake(true);  // テスト終了時にモーターを停止
                while (switch4) {
                  digitalWrite(led, HIGH);
                  analogWrite(buzzer, 128);
                }
                digitalWrite(led, LOW);
                analogWrite(buzzer, 0);
              }
              break;
            case 2:  //その場回転チェック
              debugPrintTitle("rotateCheck");
              debugPrint("PressSW3 or send 's' to start");
              if (switch3 || (Serial.available() && Serial.read() == 's')) {
                analogWrite(buzzer, 128);
                while (switch3) {
                  digitalWrite(led, HIGH);
                }
                digitalWrite(led, LOW);
                analogWrite(buzzer, 0);
                
                bool isLeftRotation = true;  // 回転方向フラグ（true: 左回転, false: 右回転）
                
                while (!switch4 && !toggle3 && !(Serial.available() && Serial.read() == 'e')) {
                  if (Serial.available() && Serial.read() == 's') break;
                  
                  if (debugType == 1) {
                    display.clearDisplay();
                    display.setTextSize(1);
                    display.setTextColor(SSD1306_WHITE);
                    display.setCursor(0, 0);
                  }
                  
                  debugPrintTitle("rotateCheck");
                  debugPrint("Direction: ");
                  debugPrintln(isLeftRotation ? "Left" : "Right");
                  debugPrint("Speed: ");
                  debugPrintln(String(botSpeed));
                  debugPrintln("running...");
                  
                  // SW3で回転方向切り替え
                  if (switch3) {
                    isLeftRotation = !isLeftRotation;
                    analogWrite(buzzer, 128);
                    while (switch3) {
                      digitalWrite(led, HIGH);
                    }
                    digitalWrite(led, LOW);
                    analogWrite(buzzer, 0);
                  }
                  
                  rotate(isLeftRotation ? 50 : -50);
                  
                  if (debugType == 1) {
                    display.display();
                  }
                }
                
                brake(true);  // モーターチェック終了時に停止
                while (switch4) {
                  digitalWrite(led, HIGH);
                  analogWrite(buzzer, 128);
                }
                digitalWrite(led, LOW);
                analogWrite(buzzer, 0);
              }
              break;
            case 3:  //円状に回り続けるモーターチェック
              debugPrintTitle("thetaCheck");
              debugPrint("PressSW3 or send 's' to start");
              if (switch3 || (Serial.available() && Serial.read() == 's')) {
                analogWrite(buzzer, 128);
                while (switch3) {
                  digitalWrite(led, HIGH);
                }
                digitalWrite(led, LOW);
                analogWrite(buzzer, 0);
                int debugTheta = 0;
                while (!switch4 && !toggle3 && !(Serial.available() && Serial.read() == 'e')) {
                  if (Serial.available() && Serial.read() == 's') break;
                  
                  if (debugType == 1) {
                    display.clearDisplay();
                    display.setTextSize(1);
                    display.setTextColor(SSD1306_WHITE);
                    display.setCursor(0, 0);
                  }
                  debugPrintTitle("thetaCheck");
                  debugTheta = In360(debugTheta + 5);
                  debugPrint("Theta = ");
                  debugPrintln(debugTheta);
                  debugPrintln("running...");
                  moveTheta(100, debugTheta, true);
                  if (debugType == 1) {
                    display.display();
                  }
                }
                brake(true);  // モーターチェック終了時に停止
                while (switch4) {
                  digitalWrite(led, HIGH);
                  analogWrite(buzzer, 128); 
                }
                digitalWrite(led, LOW);
                analogWrite(buzzer, 0);
              }
              break;
            case 4:  //4方向チェック
              debugPrintTitle("4directionCheck");
              debugPrint("PressSW3 or send 's' to start");
              if (switch3 || (Serial.available() && Serial.read() == 's')) {
                analogWrite(buzzer, 128);
                while (switch3) {
                  digitalWrite(led, HIGH);
                }
                digitalWrite(led, LOW);
                analogWrite(buzzer, 0);

                int currentDirection = 0;
                unsigned long lastChangeTime = millis();
                bool isMoving = false;

                while (!switch4 && !toggle3 && !(Serial.available() && Serial.read() == 'e')) {
                  if (Serial.available() && Serial.read() == 's') break;

                  if (debugType == 1) {
                    display.clearDisplay();
                    display.setTextSize(1);
                    display.setTextColor(SSD1306_WHITE);
                    display.setCursor(0, 0);
                  }

                  debugPrintTitle("4directionCheck");
                  debugPrint("Direction: ");
                  switch(currentDirection) {
                    case 0: debugPrintln("Forward"); break;
                    case 1: debugPrintln("Right"); break;
                    case 2: debugPrintln("Back"); break;
                    case 3: debugPrintln("Left"); break;
                  }
                  debugPrintln("running...");

                  // 方向切り替えの判定
                  if (millis() - lastChangeTime >= DIRECTION_DELAY) {
                    if (isMoving) {
                      brake(true);  // 現在の移動を停止
                      delay(500);   // ブレーキ後の待機
                      isMoving = false;
                      currentDirection = (currentDirection + 1) % 4;
                      lastChangeTime = millis();
                    }
                  } else if (!isMoving) {
                    // 新しい方向への移動開始
                    int moveAngle = In360(currentDirection * 90);
                    moveTheta(100, moveAngle, true);
                    isMoving = true;
                  }

                  if (debugType == 1) {
                    display.display();
                  }
                }
                brake(true);  // 終了時にはモーターを停止
                while (switch4) {
                  digitalWrite(led, HIGH);
                  analogWrite(buzzer, 128);
                }
                digitalWrite(led, LOW);
                analogWrite(buzzer, 0);
              }
              break;
            case 5:  //最低回転パワー測定
              debugPrintTitle("minRotatePower");
              debugPrint("L:");
              debugPrint(String(minRotatePowerLeft));
              debugPrint(" R:");
              debugPrintln(String(minRotatePowerRight));
              debugPrint("PressSW3 or send 's' to start");
              if (switch3 || (Serial.available() && Serial.read() == 's')) {
                analogWrite(buzzer, 128);
                while (switch3) {
                  digitalWrite(led, HIGH);
                }
                digitalWrite(led, LOW);
                analogWrite(buzzer, 0);

                // 左回転の測定
                int resultLeft = -1;
                int resultRight = -1;
                bool cancelled = false;

                for (int power = 1; power <= 100 && !cancelled; power++) {
                  if (debugType == 1) {
                    display.clearDisplay();
                    display.setTextSize(1);
                    display.setTextColor(SSD1306_WHITE);
                    display.setCursor(0, 0);
                  }
                  debugPrintTitle("Measuring Left...");
                  debugPrint("Power: ");
                  debugPrintln(String(power));
                  if (debugType == 1) display.display();

                  rawRotate(power);
                  unsigned long startTime = millis();
                  int initialDir = currentDirection();
                  bool found = false;

                  while (millis() - startTime < 500) {
                    if (abs(In180(currentDirection() - initialDir)) >= 3) {
                      resultLeft = power;
                      found = true;
                      break;
                    }
                    if (shouldExit()) { cancelled = true; break; }
                  }
                  brake(true);
                  if (found || cancelled) break;
                  delay(200);
                }
                brake(true);
                delay(500);
                titleFlag = false;  // 右測定のタイトル表示のためリセット

                // 右回転の測定
                if (!cancelled) {
                  for (int power = 1; power <= 100 && !cancelled; power++) {
                    if (debugType == 1) {
                      display.clearDisplay();
                      display.setTextSize(1);
                      display.setTextColor(SSD1306_WHITE);
                      display.setCursor(0, 0);
                    }
                    debugPrintTitle("Measuring Right...");
                    debugPrint("Power: ");
                    debugPrintln(String(power));
                    if (debugType == 1) display.display();

                    rawRotate(-power);
                    unsigned long startTime = millis();
                    int initialDir = currentDirection();
                    bool found = false;

                    while (millis() - startTime < 500) {
                      if (abs(In180(currentDirection() - initialDir)) >= 3) {
                        resultRight = power;
                        found = true;
                        break;
                      }
                      if (shouldExit()) { cancelled = true; break; }
                    }
                    brake(true);
                    if (found || cancelled) break;
                    delay(200);
                  }
                }
                brake(true);

                // 結果保存・表示
                titleFlag = false;  // 結果タイトル表示のためリセット
                if (resultLeft > 0) minRotatePowerLeft = resultLeft;
                if (resultRight > 0) minRotatePowerRight = resultRight;
                if (resultLeft > 0 || resultRight > 0) writeSetting();

                // 結果表示ループ
                while (!shouldExit()) {
                  if (debugType == 1) {
                    display.clearDisplay();
                    display.setTextSize(1);
                    display.setTextColor(SSD1306_WHITE);
                    display.setCursor(0, 0);
                  }
                  debugPrintTitle("Result:");
                  debugPrint("L:");
                  debugPrint(resultLeft > 0 ? String(resultLeft) : "FAIL");
                  debugPrint(" R:");
                  debugPrintln(resultRight > 0 ? String(resultRight) : "FAIL");
                  debugPrintln("Saved to SD");
                  if (debugType == 1) display.display();
                }
                exitRoutine();
              }
              break;
            case 6:  //保存パワーで回転テスト
              debugPrintTitle("minPwrRotate");
              debugPrint("L:");
              debugPrint(String(minRotatePowerLeft));
              debugPrint(" R:");
              debugPrintln(String(minRotatePowerRight));
              debugPrint("PressSW3 or send 's' to start");
              if (switch3 || (Serial.available() && Serial.read() == 's')) {
                analogWrite(buzzer, 128);
                while (switch3) {
                  digitalWrite(led, HIGH);
                }
                digitalWrite(led, LOW);
                analogWrite(buzzer, 0);

                bool isLeftRotation = true;

                while (!shouldExit()) {
                  if (debugType == 1) {
                    display.clearDisplay();
                    display.setTextSize(1);
                    display.setTextColor(SSD1306_WHITE);
                    display.setCursor(0, 0);
                  }

                  int power = isLeftRotation ? minRotatePowerLeft : minRotatePowerRight;
                  debugPrintTitle("minPwrRotate");
                  debugPrint(isLeftRotation ? "Left:" : "Right:");
                  debugPrintln(String(power));
                  debugPrintln("SW3:dir running...");

                  minPowerRotate(isLeftRotation);

                  // SW3で回転方向切り替え
                  if (switch3) {
                    isLeftRotation = !isLeftRotation;
                    brake(true);
                    analogWrite(buzzer, 128);
                    while (switch3) {
                      digitalWrite(led, HIGH);
                    }
                    digitalWrite(led, LOW);
                    analogWrite(buzzer, 0);
                  }

                  if (debugType == 1) display.display();
                }
                exitRoutine();
              }
              break;
          }
          if (switch1 || switch2) {
            if (switch1) {
              if (checkThing != 1) {
                checkThing--;
              }
            } else {
              if (checkThing != 7) {
                checkThing++;
              }
            }
            analogWrite(buzzer, 128);
            while (switch1 || switch2) {
              digitalWrite(led, HIGH);
            }
            digitalWrite(led, LOW);
            analogWrite(buzzer, 0);
          }

          if (debugType == 1) {
            display.display();
          }
        }
        brake(true);
        while (switch4) {
          digitalWrite(led, HIGH);
          analogWrite(buzzer, 128); 
        }
        digitalWrite(led, LOW);
        analogWrite(buzzer, 0);
      }
      break;
    case '7':  // ballSensorの値チェック
      debugPrintTitle("ballSensorCheck");
      debugPrint("ballSensor : ");
      debugPrintln(String(ballSensor));
      if(switch3 == HIGH) {
        digitalWrite(kicker, HIGH);
        analogWrite(buzzer, 128); 
      } else {
        digitalWrite(kicker, LOW);
        analogWrite(buzzer, 0);
      }
      break;
    case '8': // ラインチェック
      debugPrintTitle("lineCheck");
      debugPrint("PressSW3 or send 's' to start");
      if (switch3 || (Serial.available() && Serial.read() == 's')) {
        analogWrite(buzzer, 128);
        while (switch3) {
          digitalWrite(led, HIGH);
        }
        digitalWrite(led, LOW);
        analogWrite(buzzer, 0);

        // デバッグモード開始（全センサー値取得）
        setLineDebugMode(true);

        byte page = 0; // OLED: 0-3（4ページ）, PCシリアル: 0-1（2ページ）
        while (!switch4 && !toggle3 && !(Serial.available() && Serial.read() == 'e')) {
          if (debugType == 1) {
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 0);
          }

          // センサー値を取得（avoid=falseで回避動作なし）
          lineCheck(false);

          // OLEDは4ページ構成（1ページ4センサー）で4行固定表示
          if (debugType == 1) {
            int baseIndex = page * 4;
            for (int row = 0; row < 4; row++) {
              int sensorIndex = baseIndex + row;
              String line = "";
              if (row == 0) {
                line = "P" + String(page + 1) + " ";
              }
              line += String(sensorIndex + 1) + ":" + String(lineValues[sensorIndex]);
              debugPrintln(line);
            }
          } else {
            debugPrintTitle("lineCheck - Page " + String(page + 1));
            // PCシリアル表示は従来どおり
            if (page == 0) {
              for (int i = 0; i < 8; i++) {
                debugPrint(String(i + 1) + ":");
                debugPrint(String(lineValues[i]));
                if (i % 2 == 1) {
                  debugPrintln("");
                } else {
                  debugPrint(" ");
                }
              }
            } else {
              for (int i = 8; i < 16; i++) {
                debugPrint(String(i + 1) + ":");
                debugPrint(String(lineValues[i]));
                if (i % 2 == 1) {
                  debugPrintln("");
                } else {
                  debugPrint(" ");
                }
              }
            }
          }

          // SW1/SW2でページ切り替え
          if (switch1 || switch2) {
            byte pageCount = (debugType == 1) ? 4 : 2;
            page = (page + 1) % pageCount;
            analogWrite(buzzer, 128);
            while (switch1 || switch2) {
              digitalWrite(led, HIGH);
            }
            digitalWrite(led, LOW);
            analogWrite(buzzer, 0);
          }

          if (debugType == 1) {
            display.display();
          }
        }

        // デバッグモード終了（通常モードに戻す）
        setLineDebugMode(false);

        while (switch4) {
          digitalWrite(led, HIGH);
          analogWrite(buzzer, 128);
        }
        digitalWrite(led, LOW);
        analogWrite(buzzer, 0);
      }
      break;
    case '9':  // ドリブラーチェック
      debugPrintTitle("dribblerCheck");
      debugPrint("dribblerSpeed: ");
      debugPrintln(String(dribblerSpeed));
      debugPrint("PressSW3 or send 's' to start");
      if (switch3 || (Serial.available() && Serial.read() == 's')) {
        analogWrite(buzzer, 128);
        while (switch3) {
          digitalWrite(led, HIGH);
        }
        digitalWrite(led, LOW);
        analogWrite(buzzer, 0);
        
        analogWrite(dribbler_pin, dribblerSpeed);
        while (!switch4 && !toggle3 && !(Serial.available() && Serial.read() == 'e')) {
          if (debugType == 1) {
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 0);
          }
          debugPrintTitle("dribblerCheck");
          debugPrint("Speed: ");
          debugPrintln(String(dribblerSpeed));
          debugPrintln("Running...");
          
          if (debugType == 1) {
            display.display();
          }
        }
        analogWrite(dribbler_pin, 0);  // ドリブラー停止
        while (switch4) {
          digitalWrite(led, HIGH);
          analogWrite(buzzer, 128);
        }
        digitalWrite(led, LOW);
        analogWrite(buzzer, 0);
      }
      break;
      
    case 'a':  // ライン脱出テスト
      debugPrintTitle("lineEscapeTest");
      debugPrint("PressSW3 or send 's' to start");
      if (switch3 || (Serial.available() && Serial.read() == 's')) {
        analogWrite(buzzer, 128);
        while (switch3) {
          digitalWrite(led, HIGH);
        }
        digitalWrite(led, LOW);
        analogWrite(buzzer, 0);

        setLineDebugMode(false);  // 通常モード（検出のみ）

        while (!switch4 && !toggle3 && !(Serial.available() && Serial.read() == 'e')) {
          if (debugType == 1) {
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 0);
          }

          debugPrintTitle("lineEscapeTest");

          // ライン検出 + 回避動作を実行
          lineCheck(true);

          // 検出センサー数とフラグ表示
          debugPrint("Det:");
          debugPrint(String(detectedLineCount));
          debugPrint(" Flg:");
          debugPrintln(String(lineFlag));

          // 検出センサーIDと値を最大4件表示
          int dispCount = min(detectedLineCount, 4);
          for (int i = 0; i < dispCount; i++) {
            debugPrint("S");
            debugPrint(String(detectedLineSensors[i]));
            debugPrint(":");
            debugPrint(String(detectedLineValues[i]));
            if (i < dispCount - 1) debugPrint(" ");
          }
          if (dispCount > 0) debugPrintln("");

          if (debugType == 1) {
            display.display();
          }
        }

        brake(true);
        while (switch4) {
          digitalWrite(led, HIGH);
          analogWrite(buzzer, 128);
        }
        digitalWrite(led, LOW);
        analogWrite(buzzer, 0);
      }
      break;

    default:  //いずれにも当てはまらない時に実行
      debugPrintTitle("Kyomu check");
      break;
  }
}

void setThreshold(byte num) {  //閾値設定
  /* 各閾値やパラメータの設定
    '2': ゴール認識設定
        - toggle2: 青ゴール/黄ゴール切り替え
        - 状態をトグルで有効/無効化
    
    設定可能項目：
    '1': ゴール方向設定
        - SW1: -10度
        - SW2: -1度
        - SW3: +1度
        - SW4: +10度
    '5': キッカー閾値設定
        - SW1: -10
        - SW2: -1
        - SW3: +1
        - SW4: +10
    '6': ロボット速度設定
        - SW1: -10
        - SW2: -1
        - SW3: +1
        - SW4: +10
    '8': ラインセンサー閾値設定
        - SW1: センサー番号-1
        - SW2: センサー番号+1
        - SW3: 閾値-1
        - SW4: 閾値+1
    
    注意：
    - 設定値はSDカードに自動保存
    - Toggle2でブルー/イエローの切り替え
    */
  byte currentBlock = 1; // 1:左, 2:右, 3:後ろ, 4:内側
  switch (num) {
    case '1':  //青、黄ゴールの方角を設定
      debugPrintTitle("directionSet");
      if (toggle2) {  //toggle2で青、黄のいずれを調整するか選択する
        debugPrint("blue : ");
        
        if (switch1) {
          blueDirection += DECREMENT_LARGE;
          analogWrite(buzzer, 128);
          while (switch1) {}
          analogWrite(buzzer, 0);
        }
        if (switch2) {
          blueDirection += DECREMENT_SMALL;
          analogWrite(buzzer, 128);
          while (switch2) {}
          analogWrite(buzzer, 0);
        }
        if (switch3) {
          blueDirection += INCREMENT_SMALL;
          analogWrite(buzzer, 128);
          while (switch3) {}
          analogWrite(buzzer, 0);
        }
        if (switch4) {
          blueDirection += INCREMENT_LARGE;
          analogWrite(buzzer, 128);
          while (switch4) {}
          analogWrite(buzzer, 0);
        }
        blueDirection = In360(blueDirection); // 0-359の範囲に制限
        debugPrintln(blueDirection);
      } else {
        debugPrint("yellow : ");
        if (switch1) {
          yellowDirection += DECREMENT_LARGE;
          analogWrite(buzzer, 128);
          while (switch1) {}
          analogWrite(buzzer, 0);
        }
        if (switch2) {
          yellowDirection += DECREMENT_SMALL;
          analogWrite(buzzer, 128);
          while (switch2) {}
          analogWrite(buzzer, 0);
        }
        if (switch3) {
          yellowDirection += INCREMENT_SMALL;
          analogWrite(buzzer, 128);
          while (switch3) {}
          analogWrite(buzzer, 0);
        }
        if (switch4) {
          yellowDirection += INCREMENT_LARGE;
          analogWrite(buzzer, 128);
          while (switch4) {}
          analogWrite(buzzer, 0);
        }
        yellowDirection = In360(yellowDirection); // 0-359の範囲に制限
        debugPrintln(yellowDirection);
      }
      writeSetting();  //設定を書き込む
      break;
    case '7':  //キッカーの閾値設定
      debugPrintTitle("kickerThresholdSet");
      debugPrint("kickerThreshold : ");
      if (switch1) {
        kickerThreshold += DECREMENT_LARGE;
        analogWrite(buzzer, 128);
        while (switch1) {}
        analogWrite(buzzer, 0);
      }
      if (switch2) {
        kickerThreshold += DECREMENT_SMALL;
        analogWrite(buzzer, 128);
        while (switch2) {}
        analogWrite(buzzer, 0);
      }
      if (switch3) {
        kickerThreshold += INCREMENT_SMALL;
        analogWrite(buzzer, 128);
        while (switch3) {}
        analogWrite(buzzer, 0);
      }
      if (switch4) {
        kickerThreshold += INCREMENT_LARGE;
        analogWrite(buzzer, 128);
        while (switch4) {}
        analogWrite(buzzer, 0);
      }
      kickerThreshold = constrain(kickerThreshold, 0, 1023); // 0-1023の範囲に制限
      debugPrintln(kickerThreshold);
      writeSetting();  //設定を書き込む
      break;
    case '6':
      debugPrintTitle("SetSpeed");  //モーターのスピードを設定する
      debugPrint("botSpeed : ");
      if (switch1) {
        botSpeed += DECREMENT_LARGE;
        analogWrite(buzzer, 128);
        while (switch1) {}
        analogWrite(buzzer, 0);
      }
      if (switch2) {
        botSpeed += DECREMENT_SMALL;
        analogWrite(buzzer, 128);
        while (switch2) {}
        analogWrite(buzzer, 0);
      }
      if (switch3) {
        botSpeed += INCREMENT_SMALL;
        analogWrite(buzzer, 128);
        while (switch3) {}
        analogWrite(buzzer, 0);
      }
      if (switch4) {
        botSpeed += INCREMENT_LARGE;
        analogWrite(buzzer, 128);
        while (switch4) {}
        analogWrite(buzzer, 0);
      }
      botSpeed = constrain(botSpeed, 0, 255); // 0-255の範囲に制限
      debugPrintln(botSpeed);
      writeSetting();
      break;
    case '8': // ラインチェック
      debugPrintTitle("lineThresholdSet");
      // デバッグモード開始（全センサー値取得）
      setLineDebugMode(true);
      while (!toggle1) {
        if (debugType == 1) {
          display.clearDisplay();
          display.setTextSize(1);
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(0, 0);
        }

        debugPrintTitle("lineThresholdSet");

        // ブロック名を表示
        String blockName;
        switch (currentBlock) {
          case 1: blockName = "Left"; break;
          case 2: blockName = "Right"; break;
          case 3: blockName = "Back"; break;
          case 4: blockName = "Inner"; break;
        }
        debugPrint("Block: ");
        debugPrintln(blockName);

        // 最小値と最大値を計算
        int minVal = 1023, maxVal = 0;
        lineCheck(false);
        switch (currentBlock) {
          case 1: // 左ブロック
            for (int i = 2; i < 4; i++) {
              if (lineValues[i] > 0) { // 0より大きい場合のみ比較
                minVal = min(minVal, lineValues[i]);
              }
              maxVal = max(maxVal, lineValues[i]);
            }
            debugPrint("MIN: ");
            debugPrint(String(minVal));
            debugPrint(" MAX: ");
            debugPrintln(String(maxVal));
            debugPrint("Threshold: ");
            debugPrintln(String(line1Threshold));
            break;
          case 2: // 右ブロック
            for (int i = 0; i < 2; i++) {
              if (lineValues[i] > 0) { // 0より大きい場合のみ比較
                minVal = min(minVal, lineValues[i]);
              }
              maxVal = max(maxVal, lineValues[i]);
            }
            debugPrint("MIN: ");
            debugPrint(String(minVal));
            debugPrint( "MAX: ");
            debugPrintln(String(maxVal));
            debugPrint("Threshold: ");
            debugPrintln(String(line2Threshold));
            break;
          case 3: // 後ろブロック
            for (int i = 6; i < 8; i++) {
              if (lineValues[i] > 0) { // 0より大きい場合のみ比較
                minVal = min(minVal, lineValues[i]);
              }
              maxVal = max(maxVal, lineValues[i]);
            }
            debugPrint("MIN: ");
            debugPrint(String(minVal));
            debugPrint(" MAX: ");
            debugPrintln(String(maxVal));
            debugPrint("Threshold: ");
            debugPrintln(String(line3Threshold));
            break;
          case 4: // 内側ブロック
            for (int i = 4; i < 6; i++) {
              if (lineValues[i] > 0) { // 0より大きい場合のみ比較
                minVal = min(minVal, lineValues[i]);
              }
              maxVal = max(maxVal, lineValues[i]);
            }
            for (int i = 8; i < 16; i++) {
              if (lineValues[i] > 0) { // 0より大きい場合のみ比較
                minVal = min(minVal, lineValues[i]);
              }
              maxVal = max(maxVal, lineValues[i]);
            }
            debugPrint("MIN: ");
            debugPrint(String(minVal));
            debugPrint(" MAX: ");
            debugPrintln(String(maxVal));
            debugPrint("Threshold: ");
            debugPrintln(String(line4Threshold));
            break;
        }

        // SW1/SW2でブロック切り替え
        if (switch1) {
          if (currentBlock > 1) {
            currentBlock--;
          }
          waitForSwitchRelease();
        }
        if (switch2) {
          if (currentBlock < 4) {
            currentBlock++;
          }
          waitForSwitchRelease();
        }

        // SW3/SW4で閾値調整
        if (switch3) {
          switch (currentBlock) {
            case 1: line1Threshold -= 5; line1Threshold = constrain(line1Threshold, 0, 1023); break;
            case 2: line2Threshold -= 5; line2Threshold = constrain(line2Threshold, 0, 1023); break;
            case 3: line3Threshold -= 5; line3Threshold = constrain(line3Threshold, 0, 1023); break;
            case 4: line4Threshold -= 5; line4Threshold = constrain(line4Threshold, 0, 1023); break;
          }
          analogWrite(buzzer, 128);
          writeSetting();
          sendLineThresholds(); // サブマイコンに閾値送信
          waitForSwitchRelease();
          analogWrite(buzzer, 0);
        }
        if (switch4) {
          switch (currentBlock) {
            case 1: line1Threshold += 5; line1Threshold = constrain(line1Threshold, 0, 1023); break;
            case 2: line2Threshold += 5; line2Threshold = constrain(line2Threshold, 0, 1023); break;
            case 3: line3Threshold += 5; line3Threshold = constrain(line3Threshold, 0, 1023); break;
            case 4: line4Threshold += 5; line4Threshold = constrain(line4Threshold, 0, 1023); break;
          }
          analogWrite(buzzer, 128);
          writeSetting();
          sendLineThresholds(); // サブマイコンに閾値送信
          waitForSwitchRelease();
          analogWrite(buzzer, 0);
        }

        if (debugType == 1) {
          display.display();
        }
      }
      // デバッグモード終了（通常モードに戻す）
      setLineDebugMode(false);
      exitRoutine(); // 終了処理
      break;
    case '9': // ドリブラースピード
      debugPrintTitle("SetDribbleSpeed");
      debugPrint("dribblerSpeed : ");
      if (switch1) {
        dribblerSpeed += DECREMENT_SMALL;
        analogWrite(buzzer, 128);
        while (switch1) {}
        analogWrite(buzzer, 0);
      }
      if (switch2) {
        dribblerSpeed += DECREMENT_LARGE;
        analogWrite(buzzer, 128);
        while (switch2) {}
        analogWrite(buzzer, 0);
      }
      if (switch3) {
        dribblerSpeed += INCREMENT_SMALL;
        analogWrite(buzzer, 128);
        while (switch3) {}
        analogWrite(buzzer, 0);
      }
      if (switch4) {
        dribblerSpeed += INCREMENT_LARGE;
        analogWrite(buzzer, 128);
        while (switch4) {}
        analogWrite(buzzer, 0);
      }
      dribblerSpeed = constrain(dribblerSpeed, 0, 255); // 0-128の範囲に制限
      debugPrintln(dribblerSpeed);
      writeSetting();
      break;
    case '2':  // ゴール認識設定
      debugPrintTitle("goalDetectionSet");
      if (toggle2) {  // 青ゴール設定
        debugPrint("Blue goal detection: ");
        debugPrintln(blueGoalEnabled ? "Enabled" : "Disabled");
        if (switch1) {
          blueGoalEnabled = !blueGoalEnabled;  // トグル
          waitForSwitchRelease();  // スイッチ解放待ち
          writeSetting();  // 設定を保存
        }
      } else {  // 黄ゴール設定
        debugPrint("Yellow goal detection: ");
        debugPrintln(yellowGoalEnabled ? "Enabled" : "Disabled");
        if (switch1) {
          yellowGoalEnabled = !yellowGoalEnabled;  // トグル
          waitForSwitchRelease();  // スイッチ解放待ち
          writeSetting();  // 設定を保存
        }
      }
      writeSetting();  // 設定を保存
      break;

    default:  //いずれにも当てはまらない時に実行
      debug(num);
  }
}

//ログ関連
void logging(String dataString) {
    if (!loggingMode) return;
    
    // タイムスタンプとログレベルを加
    String logEntry = String(millis()) + ",";
    
    // ログレベルに応じてプレフィックスを追加
    if (dataString.startsWith("Error:")) {
        logEntry += "ERROR,";
    } else if (dataString.startsWith("Warning:")) {
        logEntry += "WARN,";
    } else {
        logEntry += "INFO,";
    }
    
    logEntry += dataString;
    
    FsFile dataFile = SD.open("datalog.txt");
    if (dataFile) {
        dataFile.println(logEntry);
        dataFile.close();
    } else {
        // エラー時はOLEDにも表示
        if (debugType) {
            debugPrintln("Error: Failed to write log");
        }
        
        // SDカードの再初期化を試みる
        if (!SD.begin(SdioConfig(FIFO_SDIO))) {
            if (debugType) {
                debugPrintln("Error: SD reinit failed");
            }
        }
    }
}

// 終了条件をチェックする共通関数
bool shouldExit() {
  if (debugType == 0) {  // シリアル通信の場合
    return (Serial.available() && Serial.read() == 'e');
  } else {  // OLEDの場合
    return (switch4 || toggle3);
  }
}

// 終了時の共通処理
void exitRoutine() {
  brake(true);  // 安全のため、モーターを停止
  clearNeoPixelRing();
  
  if (debugType == 1) {  // OLEDの場合
    while (switch4) {
      analogWrite(buzzer, 128);
      digitalWrite(led, HIGH);
    }
    digitalWrite(led, LOW);
    analogWrite(buzzer, 0);
    display.clearDisplay();
    display.display();
  }
}

// デバッグ入力待ち共通関数
bool waitForInput() {
  if (debugType == 0) {  // シリアル通信の場合
    return (Serial.available() && Serial.read() == 's');
  } else {  // OLEDの場合
    return switch3;
  }
}

// スイッチ入力待ち共通関数
void waitForSwitchRelease() {
  while (switch1 || switch2 || switch3 || switch4) {
    digitalWrite(led, HIGH);
    analogWrite(buzzer, 128); // tone(buzzer, 2794, 20);
  }
  digitalWrite(led, LOW);
  analogWrite(buzzer, 0); // noTone(buzzer);
}
