void mainRecieve() {                       //メインボードのデータ受信
  byte length = SERIAL_DATA_LENGTH;                         //一回読み取り分のデータの長さ
  int head1 = SERIAL_HEAD_NORMAL;                         //通常のモーター走行
  int head2 = SERIAL_HEAD_COAST_BRAKE;                         //開放型ブレーキ
  int head3 = SERIAL_HEAD_BRAKE;                         //非解放ブレーキ
  int head4 = SERIAL_HEAD_FORCE_RUN;                         //ライン脱出などの強制的なモーター走行
  byte recievedData[length - 1];                //受信データを一文字づつ格納するための配列を作成
  while (Serial1.available() >= length) {  //Serial1のバッファに一回読み取り分のデータの長さ以上のデータが蓄積されている時に実行
    char readData = Serial1.read();
    if (readData == head1) {  //１文字読み取り、それが上で定義したヘッダー文字と一致、つまりデータの先頭
      if (motorFlag != 1) { // ライン検出によるブレーキ中は受信しない
        motorFlag = 0;                       //フラグを通常走行に設定
        byte i = 0;                          //繰返し回数管理用の変数
        while (i <= length - 2) {            //配列は0番目から始まることを考慮して「length - 2」となっている。0を処理するのが1回目の処理だからね。(ここがわからなかったら情報Ⅰの教科書を見直そう)
          recievedData[i] = Serial1.read();  //配列recievedDataのi番目にデータを一桁分代入
          i++;
        }
        //受信データを変数に代入
        powerFL = recievedData[0] - 100;
        powerFR = recievedData[1] - 100;
        powerBL = recievedData[2] - 100;
        powerBR = recievedData[3] - 100;
      }

    } else if (readData == head2) {
      if (motorFlag != 1) {
        motorFlag = 2;  //フラグを開放型ブレーキに設定
        byte i = 0;
        while (i <= length - 2) {
          recievedData[i] = Serial1.read();
          i++;
        }
      }
    } else if (readData == head3) {
      motorFlag = 3;  //フラグを非開放ブレーキに設定
      byte i = 0;
      while (i <= length - 2) {
        recievedData[i] = Serial1.read();
        i++;
      }
    } else if (readData == head4) {
      motorFlag = 4;  //フラグを強制走行に設定
      byte i = 0;
      while (i <= length - 2) {
        recievedData[i] = Serial1.read();
        i++;
      }
      //受信データを変数に代入
      powerFL = recievedData[0] - 100;
      powerFR = recievedData[1] - 100;
      powerBL = recievedData[2] - 100;
      powerBR = recievedData[3] - 100;
    }
  }
}
