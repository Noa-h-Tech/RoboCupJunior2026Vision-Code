// ライブラリの宣言
#include <Wire.h>              //I2C通信用ライブラリ
#include <SPI.h>               //SPI通信用ライブラリ
#include <Adafruit_SSD1306.h>  //OLED用ライブラリ
#define SCREEN_WIDTH 128       // OLEDディスプレイの幅（ピクセル単位）
#define SCREEN_HEIGHT 32       // OLEDディスプレイの高さ（ピクセル単位）
#include <Adafruit_Sensor.h>   //Adafruitセンサー用ライブラリ
#include <Adafruit_BNO055.h>   //BNO055用ライブラリ
#include <utility/imumaths.h>  //コンパス計算用ライブラリ
#include <SdFat.h>             //SDカード用ライブラリ
#include <Adafruit_NeoPixel.h> //NeoPixel用ライブラリ
#include "src/distance_predict/teensy_polynomial_model.h" //距離予測用ライブラリ

// OLED処理
#define OLED_RESET 4         // リセット ピン番号 (Arduino リセット ピンを共有する場合は -1)
#define SCREEN_ADDRESS 0x3C  // アドレスについてはデータシートを参照。128x64 の場合は 0x3D、128x32 の場合は 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//BNO055処理
Adafruit_BNO055 bno = Adafruit_BNO055(55);  //BNO055としてセットアップ
const char* bnoCalibrationFile = "bno055_cal.txt";

//SD処理
SdFs SD;  //SDカード処理

bool blueGoalEnabled;   // 青ゴール認識の有効/無効
bool yellowGoalEnabled; // 黄ゴール認識の有効/無効

// 変数の宣言
// 設定系
byte botNam;       // 機体番号
bool loggingMode;  // ログをつけるかどうか
bool debugMode;    // true:ログを記録する, false:記録しない
byte debugType;    // デバッグ方法(0:USBによるシリアル通信 1:OLED)
bool titleFlag;    // USBによるデバッグの時に項目ののタイトルを表示済みかどうか
int botSpeed;      // 基準移動速度（100が標準、個体差調整用）
int minPower = 10;      // 最低動作パワー(P制御)
int minRotatePowerLeft = 10;   // 左回転の最低パワー
int minRotatePowerRight = 10;  // 右回転の最低パワー

// コンパス系（ジャイロ方式に変更済み。SDファイル互換のため変数は残す）
int blueDirection;    // 青ゴール方向の基準角度（度）※setForwardDirection()で設定
int yellowDirection;  // 黄ゴール方向の基準角度（度）※setForwardDirection()で設定
int goalDirection;    // ゴール方向の基準角度（度）

// ジャイロ方式による方向管理
int gyroOffset = 0;              // sw4ボタンで「前方」をセットした時のrawYaw
bool forwardDirectionSet = false; // 前方方向がセット済みかどうか

//カメラ系
//座標や角度など
int ballX[4];
int ballY[4];
int ballH[4];
int blueX[4];
int blueY[4];
int blueH[4];
int yellowX[4];
int yellowY[4];
int yellowH[4];
int fps[4];

int ballP[4];    // ボールのbbox面積 (w*h) ※pixel countから変更
int blueP[4];    // 青ゴールのbbox面積
int yellowP[4];  // 黄ゴールのbbox面積
int lastSeenBallP[4];
int lastSeenBlueP[4];
int lastSeenYellowP[4];

int blueW[4];    // 青ゴールのblob幅（ピクセル）
int yellowW[4];  // 黄ゴールのblob幅（ピクセル）
int lastSeenBlueW[4];
int lastSeenYellowW[4];

// 最後に見えた座標を保持
int lastSeenBallX[4];
int lastSeenBallY[4];
int lastSeenBallH[4];
int lastSeenBlueX[4];
int lastSeenBlueY[4];
int lastSeenBlueH[4];
int lastSeenYellowX[4];
int lastSeenYellowY[4];
int lastSeenYellowH[4];

int ballTheta = -1;  //ボールの角度
int blueTheta = -1;  //青の角度
int yellowTheta = -1;  //黄の角度
int ballDistance = -1;  //ボールの距離
int blueDistance = -1;  //青の距離
int yellowDistance = -1;  //黄の距離
int goalColorTheta = -1;  //目標ゴールの色の角度
int goalColorDistance = -1; //目標ゴールの色の距離
int opponentColorTheta = -1; //相手の目標ゴールの色の角度
int opponentColorDistance = -1; //相手の目標ゴールの色の距離

// ゴールの端角度（極座標、-1=不明）
int blueLeftEdge   = -1;  // 青ゴール左端の角度
int blueRightEdge  = -1;  // 青ゴール右端の角度
int yellowLeftEdge  = -1; // 黄ゴール左端の角度
int yellowRightEdge = -1; // 黄ゴール右端の角度
int goalColorLeftEdge     = -1;  // 目標ゴール左端の角度
int goalColorRightEdge    = -1;  // 目標ゴール右端の角度
int opponentColorLeftEdge  = -1; // 相手ゴール左端の角度
int opponentColorRightEdge = -1; // 相手ゴール右端の角度
#define camX_max 160  //カメラのX座標の最大値
#define camY_max 120  //カメラのY座標の最大値(QQVGA)

// find() は 0(無効) か 1-4(カメラ番号)を返すため、配列添字(0-3)に安全に変換する
inline int autoCamIndex(int object) {
	int cam = find(object);
	if (cam <= 0) {
		return 0;  // 無効時は安全なデフォルト(カメラ1)
	}
	if (cam > 4) {
		return 3;
	}
	return cam - 1;
}

#define auto_ballX ballX[autoCamIndex(0)]   //自動で最適なカメラから取得したボールのX座標
#define auto_ballY ballY[autoCamIndex(0)]   //自動で最適なカメラから取得したボールのY座標
#define auto_ballH ballH[autoCamIndex(0)]   //自動で最適なカメラから取得したボールの高さ
#define auto_blueX blueX[autoCamIndex(1)]   //自動で最適なカメラから取得した青のX座標
#define auto_blueY blueY[autoCamIndex(1)]   //自動で最適なカメラから取得した青のY座標
#define auto_yellowX yellowX[autoCamIndex(2)] //自動で最適なカメラから取得した黄のX座標
#define auto_yellowY yellowY[autoCamIndex(2)] //自動で最適なカメラから取得した黄のY座標

#define cam1_ballX ballX[0]      //カメラ1のボールのX座標
#define cam1_ballY ballY[0]      //カメラ1のボールのY座標
#define cam1_blueX blueX[0]      //カメラ1の青のX座標
#define cam1_blueY blueY[0]      //カメラ1の青のY座標
#define cam1_yellowX yellowX[0]  //カメラ1の黄のX座標
#define cam1_yellowY yellowY[0]  //カメラ1の黄のY座標
#define cam1_fps fps[0]          //カメラ1のfps

#define cam2_ballX ballX[1]      //カメラ2のボールのX座標
#define cam2_ballY ballY[1]      //カメラ2のボールのY座標
#define cam2_blueX blueX[1]      //カメラ2の青のX座標
#define cam2_blueY blueY[1]      //カメラ2の青のY座標
#define cam2_yellowX yellowX[1]  //カメラ2の黄のX座標
#define cam2_yellowY yellowY[1]  //カメラ2の黄のY座標
#define cam2_fps fps[1]          //カメラ2のfps

#define cam3_ballX ballX[2]      //カメラ3のボールのX座標
#define cam3_ballY ballY[2]      //カメラ3のボールのY座標
#define cam3_blueX blueX[2]      //カメラ3の青のX座標
#define cam3_blueY blueY[2]      //カメラ3の青のY座標
#define cam3_yellowX yellowX[2]  //カメラ3の黄のX座標
#define cam3_yellowY yellowY[2]  //カメラ3の黄のY座標
#define cam3_fps fps[2]          //カメラ3のfps

#define cam4_ballX ballX[3]      //カメラ4のボールのX座標
#define cam4_ballY ballY[3]      //カメラ4のボールのY座標
#define cam4_blueX blueX[3]      //カメラ4の青のX座標
#define cam4_blueY blueY[3]      //カメラ4の青のY座標
#define cam4_yellowX yellowX[3]  //カメラ4の黄のX座標
#define cam4_yellowY yellowY[3]  //カメラ4の黄のY座標
#define cam4_fps fps[3]          //カメラ4のfps

//超音波センサ系
const long trigPins[] = { 5, 4, 6, 37};     // 超音波センサのトリガーピンを宣言, 後ろは37, 無効なら-1
const long echoPins[] = { 5, 4, 6, 37};     // 超音波センサのエコーピンを宣言, 後ろは37, 無効なら-1
#define echoF getEcho(trigPins[0],echoPins[0])  // 超音波センサ1の距離
#define echoL getEcho(trigPins[1],echoPins[1])  // 超音波センサ2の距離
#define echoR getEcho(trigPins[2],echoPins[2])  // 超音波センサ3の距離
#define echoB getEcho(trigPins[3],echoPins[3])  // 超音波センサ4の距離

// ピンの定義
#define sw1_pin 29  //スイッチ1のピン番号
#define sw2_pin 28  //スイッチ2のピン番号
#define sw3_pin 35  //スイッチ3のピン番号
#define sw4_pin 36  //スイッチ4のピン番号
#define tg1_pin 30  //トグル1のピン番号
#define tg2_pin 33  //トグル2のピン番号
#define tg3_pin 34  //トグル3のピン番号
#define dribbler_pin 22 //ドリブラーのピン番号
#define switch1 (!(digitalRead(sw1_pin)))  //スイッチ1の状態
#define switch2 (!(digitalRead(sw2_pin)))  //スイッチ2の状態
#define switch3 (!(digitalRead(sw3_pin)))  //スイッチ3の状態
#define switch4 (!(digitalRead(sw4_pin)))  //スイッチ4の状態
#define toggle1 !((digitalRead(tg1_pin)))  //トグル1の状態
#define toggle2 !((digitalRead(tg2_pin)))  //トグル2の状態
#define toggle3 !(digitalRead(tg3_pin))  //トグル3の状態
#define led 32  //LEDのピン番号
#define ballSensor_pin A9  //ボールセンサーのピン番号
#define ballSensor analogRead(ballSensor_pin)  //ボールセンサーの値
#define kicker 41  //キッカーのピン番号
#define buzzer 12  //ブザーのピン番号
#define neopixel_pin 37 // NeoPixelリングの信号ピン（必要に応じて変更）, 無効なら-1
#define neopixel_count 24 // NeoPixelリングのLED数

Adafruit_NeoPixel neoRing(neopixel_count, neopixel_pin, NEO_GRB + NEO_KHZ800);

//モーター関連
int lastMotorTheta;  //前回のモーターの角度

int kickerThreshold;  // キッカーの閾値

// ライン通信関連の定義
#define LINE_START_MARKER 254
#define LINE_END_MARKER 253
#define LINE_ESCAPE_MARKER 252

// コマンドID（メイン → サブ）
#define LINE_CMD_SET_ALL_THRESHOLDS 0xF1
#define LINE_CMD_REQUEST_DEBUG 0xF2
#define LINE_CMD_REQUEST_NORMAL 0xF3

// レスポンスID（サブ → メイン）
#define LINE_RESP_DETECTED 0xA0
#define LINE_RESP_DEBUG 0xA1
#define LINE_RESP_ACK 0xA2

// ライン通信状態
bool lineDebugMode = false;
byte lineRespBuffer[64];
int lineRespIndex = 0;
bool lineRespReceiving = false;
bool lineRespEscapeNext = false;

// 検出されたライン情報
int detectedLineCount = 0;
int detectedLineSensors[16];
int detectedLineValues[16];

int lineValues[16];  // ラインセンサーの値を保存
int line1Threshold;  // ラインセンサー1の閾値（左）
int line2Threshold;  // ラインセンサー2の閾値（右）
int line3Threshold;  // ラインセンサー3の閾値（後ろ）
int line4Threshold;  // ラインセンサー4の閾値（内側）
// 1番を60に、3番を240に変更（あくまで推測値です）
int lineAngles[16] = {100, 80, 280, 260, 314, 350, 170, 190, 80, 278, 242, 278, 26, 62, 98, 0};

// 距離推定モデルのパラメータ
float* coefficients = nullptr;  // 多項式回帰の係数
int num_features = 15;         // 特徴量の数（weights.csvの係数の数に合わせる）

// 標準化パラメータ
float y_mean = 0.0;
float y_std = 0.0;
float theta_mean = 0.0;
float theta_std = 0.0;
float distance_mean = 0.0;
float distance_std = 0.0;

// 関数プロトタイプ
bool waitForCalibration(unsigned long timeout);

int lineFlag = 0; // ラインの状態

// ドリブラー関連
int dribblerSpeed = 80;  // ドリブラーの回転速度（0-255）
bool isDribblerMode = false;  // true: ドリブラーモード, false: キッカーモード
bool isDribblerOn = false;    // true: ドリブラー回転中（超音波センサー使用不可）
// Bluetooth接続状態を管理するグローバル変数
bool isBleConnected = true;

int lastChaseTheta; // 最後に追跡した角度
int debugAdjustBallAngle3 = -1; // デバッグ表示用: adjustBallAngle3の出力角度

bool lastChaseStrategy; // 最後に使用した追跡処理

bool started = false; // スタートフラグ
unsigned long playResumedAt = 0; // プレイ開始/一時停止解除時のタイムスタンプ
unsigned long noiseDetectedAt = 0; // ノイズ検出タイムスタンプ（LED点灯制御用）
bool kickerUseCompassForGoal = true; // キッカーモードのゴール補正方法（ヒステリシス用）
