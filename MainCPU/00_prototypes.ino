// 速度計算に使う過去フレーム数
const int SNAP_HISTORY = 5;

// 極座標スナップショット構造体
struct PolarSnapshot {
  int   theta;          // 最後の実測絶対方位（deg）
  int   dist;           // 最後の実測距離（cm）
  float vTheta;         // 平均速度 [deg/ms]
  float vDist;          // 平均距離速度 [cm/ms]
  int   prevTheta;      // 1フレーム前の角度
  int   prevDist;       // 1フレーム前の距離
  unsigned long lastT;  // 最後にデータが変化したmillis()
  bool  valid;          // スナップショットが有効かどうか
  // 速度計算用リングバッファ（直近 SNAP_HISTORY 回分）
  int          thetaHist[SNAP_HISTORY];
  int          distHist[SNAP_HISTORY];
  unsigned long timeHist[SNAP_HISTORY];
  int   histIdx;        // 次の書き込み位置
  int   histCount;      // 有効エントリ数 (0〜SNAP_HISTORY)
};

// 関数のプロトタイプ宣言

// センサー関連の関数
int currentDirection();
int In360(int theta);
int In180(int theta);
bool isCalibrated();
bool waitForCalibration(unsigned long timeout);
long getEcho(int trig, int echo);
long microsecondsToCentimeters(long microseconds);
bool loadBnoCalibrationOffsets();
bool saveBnoCalibrationOffsets();

// カメラ関連の関数
void resetHistory(PolarSnapshot& snap, int theta, int dist, unsigned long t);
void updatePolarSnapshot(PolarSnapshot& snap, int theta, int dist);
bool predictPolar(const PolarSnapshot& snap, long msElapsed, int* outTheta, int* outDist);
void camRecieve(int camNumber, int currentYaw);
int SerialNumRead(byte num);
int SerialNumAvailable(byte Num);
void allCamProcess();
void getLocation(int cam, int object, int *theta, int *distance);
float edgeXToAngle(int cam, float x, float y);
int getGoalDistanceFromCameras(int object, int cam1, int cam2 = 0);
void getGoalThetaWithEdges(int object, int *theta, int *distance, int *leftEdge, int *rightEdge);
byte find(int object);
void clearSerialBuffers();

// 制御関連の関数
void rotateToTarget(int targetTheta, unsigned long timeoutMs = 50, float gain = -0.8, int threshold = 25, bool brakeAfter = true);
void turnToTargetIF(int targetTheta);
int getValidatedGoalDirection();
int getBallChasingDirection();
void handleNoBallVisible();
void commonPlaySetup();
void handleLongDistanceBall();
void handleShortDistanceBall();
void handleLongDistanceBallWithDribbler();
void handleShortDistanceBallWithDribbler();
void handleBallPossession();
void handleShooting();
void handleBallChasing();
void updateDisplay();

// モーター関連の関数
void move(int speed1, int speed2, int speed3, int speed4, float maximize);
void forced_move(int speed1, int speed2, int speed3, int speed4, float maximize);
void rotate(int speed);
void brake(bool type);
void moveTheta(int speed, int targetTheta, bool forced, float maximize = 1.0f);
void rotateAndMoveTheta(int speed, int targetMoveTheta, int targetRotateTheta, bool forced, float maximize);
void turnTheta(int targetTheta, float gain = -0.4);
void rawRotate(int speed);
int findMinRotatePower(bool leftRotation);
void minPowerRotate(bool leftRotation);
void turnOnDribbler();
void turnOffDribbler();

// ライン関連の関数
void lineCheck(bool avoid);
void processLineValues(bool avoid);
void processLineResponse(bool avoid);
void processDetectedLines();
void processLineAvoidance();
void processDebugData();
int adjustEscapeThetaByUltrasonic(int escapeTheta);
void sendLineThresholds();
void setLineDebugMode(bool enabled);
bool line1Check();
bool line2Check();
bool line3Check();
bool line4Check();
void lineleaveWall(int speed);

// チェイス関連の関数
void chase(int speed, int targetTheta, bool maximize);
int adjustBallAngle2(int cam, int x, float theta);
int adjustBallAngle3(int cam, int x, int y, float theta);
int getChaseAngle(int cam, int optimizedBallTheta);
int getApproachAngle(int memo);
int convertToAbsoluteAngle(int relativeTheta);

// ホームポジション関連の関数
void echoPos(int speed);
void echoPos2(int speed);

// 座標予測関連の関数
void loadWeights();
float calculateDistance(float theta, float y, int height);
int getFileIndex(int x);
int getRelativeX(int x);
float getDistanceFromCoordinates(int x, int y, int height);
float fieldBearingFromLocal(int localTheta);
void predictCoordinatesGlobal(int* cx, int* cy);
void predictCoordinatesLocal();
int parallelTheta(int theta);

// カメラ角度計算の共通ヘルパー
float pixelToRelativeAngle(float x, float y);

// デバッグ関連の関数
void debug(byte num);
void setThreshold(byte num);
bool shouldExit();
void exitRoutine();

// 設定関連の関数
void writeSetting();
void readSetting();

// NeoPixel関連の関数
void initNeoPixelRing();
void clearNeoPixelRing();
void updateDebugModeNeoPixelIndicator();
void updateDirectionNeoPixelDebug();
void updateChaseThetaNeoPixel(int theta);
void setArcMarker(uint8_t *rBuf, uint8_t *gBuf, uint8_t *bBuf,
                  int fromTheta, int toTheta, uint8_t r, uint8_t g, uint8_t b);
