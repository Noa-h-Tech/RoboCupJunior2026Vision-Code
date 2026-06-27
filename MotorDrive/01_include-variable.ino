#include <Noa.h-DRV8870.h>  //モータードライバー用ライブラリ

//モーター関連
DRV8870 motorFL(D10, D9);     //前左モーターのピン
DRV8870 motorFR(D1, D0);     //前右モーターのピン
DRV8870 motorBL(D3, D2);    //後左モーターのピン
DRV8870 motorBR(D4, D5);   //後右モーターのピン

//#define brakePin D8  //ライン処理ボードとの接続ピン

int powerFL;  //左前モーター出力
int powerFR;  //右前モーター出力
int powerBL;  //左後モーター出力
int powerBR;  //右後モーター出力

int botSpeed = 100;  //全体のモーターパワー出力調整。基本はここではなくメインボードで調整。

volatile byte motorFlag;  //モーターの動作命令フラグ。0:通常走行 1:ライン検出によるブレーキ 2:開放型ブレーキ 3:非開放ブレーキ 4:強制走行

// シリアル通信関連の定数
const byte SERIAL_DATA_LENGTH = 5; // 受信するデータの長さ
const int SERIAL_HEAD_NORMAL = 255; // 通常走行のヘッダー
const int SERIAL_HEAD_COAST_BRAKE = 254; // 開放型ブレーキのヘッダー
const int SERIAL_HEAD_BRAKE = 253; // 非開放ブレーキのヘッダー
const int SERIAL_HEAD_FORCE_RUN = 252; // 強制走行のヘッダー
