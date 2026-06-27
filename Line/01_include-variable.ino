#include <CD74HC4067.h>
CD74HC4067 my_mux(D1, D2, D3, D4);  // S0, S1, S2, S3
const int g_common_pin = A0;  // Common pin
int lineValues[16]; // Line values (16 sensors)

// 受信閾値（メインマイコンから設定される）
int receivedThresholds[4] = {300, 300, 300, 300}; // block1(左), block2(右), block3(後ろ), block4(内側)

// 動作モード
enum LineMode { MODE_NORMAL, MODE_DEBUG };
LineMode currentMode = MODE_NORMAL;

// コマンド受信用バッファ
#define CMD_BUFFER_SIZE 16
byte cmdBuffer[CMD_BUFFER_SIZE];
int cmdIndex = 0;
bool cmdReceiving = false;
bool cmdEscapeNext = false;

// センサー → ブロックマッピング (1:左, 2:右, 3:後ろ, 4:内側)
// センサー 0-1:右(2), 2-3:左(1), 4-5:内側(4), 6-7:後ろ(3), 8-15:内側(4)
const int sensorBlockMap[16] = {2, 2, 1, 1, 4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4};

// UART通信用のマーカー
#define START_MARKER 254
#define END_MARKER 253
#define ESCAPE_MARKER 252

// コマンドID（メイン → サブ）
#define CMD_SET_ALL_THRESHOLDS 0xF1
#define CMD_REQUEST_DEBUG 0xF2
#define CMD_REQUEST_NORMAL 0xF3

// レスポンスID（サブ → メイン）
#define RESP_LINE_DETECTED 0xA0
#define RESP_DEBUG_DATA 0xA1
#define RESP_ACK 0xA2

byte lastSentDetectedCount = 0;
unsigned long lastZeroReportTime = 0;
const unsigned long ZERO_REPORT_INTERVAL_MS = 100;

unsigned long lastUsbMonitorPrintTime = 0;
const unsigned long USB_MONITOR_INTERVAL_MS = 200;
