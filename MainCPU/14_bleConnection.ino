
/*
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial1.begin(9600);
}

void loop() {
  serialReceivedFromBle();
  bleSendFromMain(1,1,155);
  delay(100);
}
*/

void serialReceivedFromBle() {
  // 定数定義
  static const int LENGTH = 6;  // ヘッダー込み
  static const byte HEAD_CONNECT = 255;
  static const byte HEAD_DISCONNECT = 254;
  static const unsigned long TIMEOUT = 100; // タイムアウト100ms

  // デバッグ情報の強化
  static unsigned long lastSuccessTime = 0;
  static uint32_t totalReceived = 0;
  static uint32_t errorCount = 0;

  // バッファオーバーフロー防止
  if (Serial1.available() > 64) { // バッファサイズの3/4以上
    Serial.println("警告: バッファオーバーフロー防止のためクリア");
    while(Serial1.available()) {
      Serial1.read();
    }
    return;
  }

  // データ長チェック
  if (Serial1.available() < LENGTH) {
    return;
  }

  // ヘッダーとデータの読み取り処理
  byte receivedData[LENGTH] = {0};
  unsigned long startTime = millis();
  
  // ヘッダー読み取り
  receivedData[0] = Serial1.read();
  
  // ヘッダー検証前のデバッグ出力
  Serial.println("lengthClear");

  // ヘッダー検証とBluetooth接続状態の更新
  if (receivedData[0] == HEAD_DISCONNECT) {
    isBleConnected = false;
    Serial.println("=== Bluetooth切断検知 ===");
    Serial.println("接続状態: 切断");
  } else if (receivedData[0] == HEAD_CONNECT) {
    isBleConnected = true;
    Serial.println("=== Bluetooth接続検知 ===");
    Serial.println("接続状態: 接続中");
  } else {
    Serial.println("エラー: 無効なヘッダー");
    errorCount++;
    return;
  }

  // ヘッダー検証後のデバッグ出力
  Serial.println("headClear");

  // 残りのデータ読み取り
  int dataIndex = 1;
  while (dataIndex < LENGTH) {
    if (Serial1.available()) {
      receivedData[dataIndex] = Serial1.read();
      dataIndex++;
    }
    
    if (millis() - startTime > TIMEOUT) {
      Serial.println("エラー: 受信タイムアウト");
      errorCount++;
      return;
    }
  }

  // データ処理成功
  totalReceived++;
  lastSuccessTime = millis();

  //データ復元
  bool InGame = receivedData[1];
  bool InDribble = receivedData[2];
  int ballScore = receivedData[3] * 100 + receivedData[4] * 10 + receivedData[5];

  
  // 詳細なデバッグ情報
  Serial.println("=== 受信成功 ===");
  Serial.print("ヘッダー: 0x");
  Serial.println(receivedData[0], HEX);
  Serial.print("データ: ");
  for (int i = 1; i < LENGTH; i++) {
    Serial.print(receivedData[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("InGame:");
  Serial.println(InGame);
  Serial.print("InDribble:");
  Serial.println(InDribble);
  Serial.print("ballScore:");
  Serial.println(ballScore);
  Serial.println("================");
  Serial.print("累計受信回数: ");
  Serial.println(totalReceived);
  Serial.print("エラー回数: ");
  Serial.println(errorCount);
  Serial.print("最終成功時刻: ");
  Serial.println(lastSuccessTime);
  Serial.println("============");
}

void bleSendFromMain(bool InGame, bool InDribble, int ballScore){
  byte sendData[5];
  sendData[0] = InGame;
  sendData[1] = InDribble;
  sendData[2] = ballScore / 100;
  sendData[3] = (ballScore / 10) % 10;
  sendData[4] = ballScore % 10;

  Serial1.write(255);
  Serial1.write(sendData, 5);
  //デバッグ情報
  Serial.print("Send: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(sendData[i]);
    Serial.print(" ");
  }
  Serial.println();
}
