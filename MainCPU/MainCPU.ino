/*Noa.h 2025
  Main CPU
  Ver.1.00
  This code is for Teensy4.1

  【プログラム全体の説明】
  このプログラムは4輪オムニホイールロボット用の制御プログラムです。
  主な機能：
  - 4台のカメラによるボール/ゴール検出
  - BNO055による方位検出
  - 全方向移動制御
  - SDカードによる設定保存とログ記録
  - OLEDディスプレイによるデバッグ表示

  【ファイル構成】
  MainCPU.ino: 全体説明（このファイル）
  00_prototypes.ino: 関数プロトタイプ宣言
  01_include-variable.ino: ライブラリとグローバル変数の定義
  02_setup.ino: 初期化処理
  03_optimization.ino: ロボット個体差の補正
  04_debug.ino: デバッグ機能
  05_loop.ino: メインループ
  06_control.ino: 制御（回転/移動など）
  07_sensor.ino: センサー関連（BNO055/超音波など）
  08_camera.ino: カメラ制御
  09_motor.ino: モーター制御
  10_chase.ino: ボール追跡
  11_homepos.ino: ホームポジション/復帰処理
  12_line.ino: ラインセンサー処理
  13_predict-location.ino: 距離推定/位置予測
  14_bleConnection.ino: BLE通信
*/

// 高速ループを優先するため標準yieldを無効化する。
// serialEventなどyield経由の暗黙処理は実行されないため、必要な通信処理は明示的に呼ぶ。
void yield() {
}
