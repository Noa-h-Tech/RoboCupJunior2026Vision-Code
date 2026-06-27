inline bool isInCameraFrame(int x, int y) {
  return (x >= 0 && x < camX_max && y >= 0 && y < camY_max);
}
const int CAMERA_INVALID_COORD = -100;
const unsigned long CAMERA_HOLD_MS = 500;

inline bool hasValidDetection(int x, int y) {
  if ((x == 0 && y == 0) || (x == CAMERA_INVALID_COORD && y == CAMERA_INVALID_COORD)) {
    return false;
  }
  return isInCameraFrame(x, y);
}

inline bool hasValidBoundingBox(int x1, int y1, int x2, int y2) {
  if (x1 == CAMERA_INVALID_COORD && y1 == CAMERA_INVALID_COORD &&
      x2 == CAMERA_INVALID_COORD && y2 == CAMERA_INVALID_COORD) {
    return false;
  }
  return isInCameraFrame(x1, y1) &&
         x2 > x1 && x2 <= camX_max &&
         y2 > y1 && y2 <= camY_max;
}

// ---- 極座標ベース位置予測 ----
// PolarSnapshot構造体は00_prototypes.inoで定義

PolarSnapshot ballSnap          = {0, -1, 0.0f, 0.0f, 0, -1, 0, false};
PolarSnapshot goalColorSnap     = {0, -1, 0.0f, 0.0f, 0, -1, 0, false};
PolarSnapshot opponentColorSnap = {0, -1, 0.0f, 0.0f, 0, -1, 0, false};

void camRecieve(int camNumber, int currentYaw) {
  /* カメラからのデータフォーマット
  1バイト目: ヘッダー（255固定）
  2-4バイト目: ボールX座標
  5-7バイト目: ボールY座標
  8-10バイト目: 青ゴールX座標
  ... 以下同様
  
  座標値は実際の値+100してカメラから送信（データの長さを確実に3桁にするため）
  例：実際の座標が50の場合、150として送信
  */

  byte length = 40;                                  // 1回のデータ長（ヘッダー+座標データ+高さデータ+ピクセルデータ）
  int head1 = 255;                                   //データのヘッダー文字1。ここでは'255'で1つの文字として扱っている(詳しい話はAIに聞いてね)これを受信したらカメラからの座標データを読み込む。
  int recievedData[length - 1];                          //受信データを一文字づつ格納するための配列を作成
  while (SerialNumAvailable(camNumber) >= length) {  //シリアルのバッファに一回読み取り分のデータの長さ以上のデータが蓄積されている時に実行
    int readData = SerialNumRead(camNumber);
    if (readData == head1) {                         //１文字読み取り、それが上で定義したヘッダー文字と一致、つまりデータの先頭
      byte i = 0;                                    //繰返し回数管理用の変数
      bool packetTimeout = false;
      while (i <= length - 2) {                       //配列は0番目から始まることを考慮して「length - 1」となっている。0を処理するのが1回目の処理だからね。(ここがわからなかったら情報Ⅰの教科書を見直そう)
        // バイトが届くまで待機（1バイト≒260us@38400baud）、2msでタイムアウトしてパケット破棄
        unsigned long waitStart = micros();
        while (SerialNumAvailable(camNumber) == 0) {
          if (micros() - waitStart > 2000) { packetTimeout = true; break; }
          delayMicroseconds(50);
        }
        if (packetTimeout) break;
        recievedData[i] = SerialNumRead(camNumber);  //配列recievedDataのi番目にデータを一桁分代入
        i++;
      }
      if (packetTimeout) continue;
      // 受信データを変数に代入（対角2点フォーマット: x1,y1,x2,y2 各3桁）
      // Teensy側でcx=(x1+x2)/2, cy=(y1+y2)/2, h=y2-y1, w=x2-x1, area=w*h を算出
      // ---- Ball ----
      int newBallX1 = ((recievedData[0] * 100) + (recievedData[1] * 10) + recievedData[2]) - 100;
      int newBallY1 = ((recievedData[3] * 100) + (recievedData[4] * 10) + recievedData[5]) - 100;
      int newBallX2 = ((recievedData[6] * 100) + (recievedData[7] * 10) + recievedData[8]) - 100;
      int newBallY2 = ((recievedData[9] * 100) + (recievedData[10] * 10) + recievedData[11]) - 100;
      bool ballVisible = hasValidBoundingBox(newBallX1, newBallY1, newBallX2, newBallY2);
      int newBallX = ballVisible ? (newBallX1 + newBallX2) / 2 : -100;
      int newBallY = ballVisible ? (newBallY1 + newBallY2) / 2 : -100;
      int newBallH = ballVisible ? (newBallY2 - newBallY1) : -100;
      int newBallP = ballVisible ? (newBallX2 - newBallX1) * (newBallY2 - newBallY1) : -100;

      // ---- Blue goal ----
      int newBlueX1 = ((recievedData[12] * 100) + (recievedData[13] * 10) + recievedData[14]) - 100;
      int newBlueY1 = ((recievedData[15] * 100) + (recievedData[16] * 10) + recievedData[17]) - 100;
      int newBlueX2 = ((recievedData[18] * 100) + (recievedData[19] * 10) + recievedData[20]) - 100;
      int newBlueY2 = ((recievedData[21] * 100) + (recievedData[22] * 10) + recievedData[23]) - 100;
      bool blueVisible = hasValidBoundingBox(newBlueX1, newBlueY1, newBlueX2, newBlueY2);
      int newBlueX = blueVisible ? (newBlueX1 + newBlueX2) / 2 : -100;
      int newBlueY = blueVisible ? (newBlueY1 + newBlueY2) / 2 : -100;
      int newBlueH = blueVisible ? (newBlueY2 - newBlueY1) : -100;
      int newBlueP = blueVisible ? (newBlueX2 - newBlueX1) * (newBlueY2 - newBlueY1) : -100;
      int newBlueW = blueVisible ? (newBlueX2 - newBlueX1) : 0;
      // ---- Yellow goal ----
      int newYellowX1 = ((recievedData[24] * 100) + (recievedData[25] * 10) + recievedData[26]) - 100;
      int newYellowY1 = ((recievedData[27] * 100) + (recievedData[28] * 10) + recievedData[29]) - 100;
      int newYellowX2 = ((recievedData[30] * 100) + (recievedData[31] * 10) + recievedData[32]) - 100;
      int newYellowY2 = ((recievedData[33] * 100) + (recievedData[34] * 10) + recievedData[35]) - 100;
      bool yellowVisible = hasValidBoundingBox(newYellowX1, newYellowY1, newYellowX2, newYellowY2);
      int newYellowX = yellowVisible ? (newYellowX1 + newYellowX2) / 2 : -100;
      int newYellowY = yellowVisible ? (newYellowY1 + newYellowY2) / 2 : -100;
      int newYellowH = yellowVisible ? (newYellowY2 - newYellowY1) : -100;
      int newYellowP = yellowVisible ? (newYellowX2 - newYellowX1) * (newYellowY2 - newYellowY1) : -100;
      int newYellowW = yellowVisible ? (newYellowX2 - newYellowX1) : 0;
      fps[camNumber - 1] = constrain(((recievedData[36] * 100) + (recievedData[37] * 10) + recievedData[38]) - 100, 0, 999);
      if (newBallX != -100 && !isInCameraFrame(newBallX, newBallY)) {
        newBallX = -100;
        newBallY = -100;
        newBallH = -100;
        newBallP = -100;
        noiseDetectedAt = millis();
      }
      if (newBlueX != -100 && !isInCameraFrame(newBlueX, newBlueY)) {
        newBlueX = -100;
        newBlueY = -100;
        newBlueH = -100;
        newBlueP = -100;
        noiseDetectedAt = millis();
      }
      if (newYellowX != -100 && !isInCameraFrame(newYellowX, newYellowY)) {
        newYellowX = -100;
        newYellowY = -100;
        newYellowH = -100;
        newYellowP = -100;
        noiseDetectedAt = millis();
      }

      // 物体が検出された場合は座標を更新し、最後に見えた座標も更新
      if (newBallX != -100) {
        ballX[camNumber - 1] = newBallX;
        ballY[camNumber - 1] = newBallY;
        ballH[camNumber - 1] = newBallH;
        ballP[camNumber - 1] = newBallP;
        lastSeenBallX[camNumber - 1] = newBallX;
        lastSeenBallY[camNumber - 1] = newBallY;
        lastSeenBallH[camNumber - 1] = newBallH;
        lastSeenBallP[camNumber - 1] = newBallP;
      } else {
        ballX[camNumber - 1] = -100;
        ballY[camNumber - 1] = -100;
        ballH[camNumber - 1] = -100;
        ballP[camNumber - 1] = -100;
      }

      if (newBlueX != -100 && blueGoalEnabled) {
        // カメラの向きとゴール方向の整合性チェック
        int goalRelative = In180(blueDirection - currentYaw);
        int cameraCenter = 90 * (camNumber - 1);
        int angleDiff = abs(In180(goalRelative - cameraCenter));

        if (angleDiff > 120) {
          // このカメラはゴール方向を向いていない → 無視
          blueX[camNumber - 1] = -100;
          blueY[camNumber - 1] = -100;
          blueH[camNumber - 1] = -100;
          blueP[camNumber - 1] = -100;
          blueW[camNumber - 1] = 0;
          noiseDetectedAt = millis();
        } else {
          // 正常なゴール検出として処理
          blueX[camNumber - 1] = newBlueX;
          blueY[camNumber - 1] = newBlueY;
          blueH[camNumber - 1] = newBlueH;
          blueP[camNumber - 1] = newBlueP;
          blueW[camNumber - 1] = newBlueW;
          lastSeenBlueX[camNumber - 1] = newBlueX;
          lastSeenBlueY[camNumber - 1] = newBlueY;
          lastSeenBlueH[camNumber - 1] = newBlueH;
          lastSeenBlueP[camNumber - 1] = newBlueP;
          lastSeenBlueW[camNumber - 1] = newBlueW;
        }
      } else {
        blueX[camNumber - 1] = -100;
        blueY[camNumber - 1] = -100;
        blueH[camNumber - 1] = -100;
        blueP[camNumber - 1] = -100;
        blueW[camNumber - 1] = 0;
      }

      if (newYellowX != -100 && yellowGoalEnabled) {
        // カメラの向きとゴール方向の整合性チェック
        int goalRelative = In180(yellowDirection - currentYaw);
        int cameraCenter = 90 * (camNumber - 1);
        int angleDiff = abs(In180(goalRelative - cameraCenter));

        if (angleDiff > 120) {
          // このカメラはゴール方向を向いていない → 無視
          yellowX[camNumber - 1] = -100;
          yellowY[camNumber - 1] = -100;
          yellowH[camNumber - 1] = -100;
          yellowP[camNumber - 1] = -100;
          yellowW[camNumber - 1] = 0;
          noiseDetectedAt = millis();
        } else {
          // 正常なゴール検出として処理
          yellowX[camNumber - 1] = newYellowX;
          yellowY[camNumber - 1] = newYellowY;
          yellowH[camNumber - 1] = newYellowH;
          yellowP[camNumber - 1] = newYellowP;
          yellowW[camNumber - 1] = newYellowW;
          lastSeenYellowX[camNumber - 1] = newYellowX;
          lastSeenYellowY[camNumber - 1] = newYellowY;
          lastSeenYellowH[camNumber - 1] = newYellowH;
          lastSeenYellowP[camNumber - 1] = newYellowP;
          lastSeenYellowW[camNumber - 1] = newYellowW;
        }
      } else {
        yellowX[camNumber - 1] = -100;
        yellowY[camNumber - 1] = -100;
        yellowH[camNumber - 1] = -100;
        yellowP[camNumber - 1] = -100;
        yellowW[camNumber - 1] = 0;
      }
    }
  }
}

int SerialNumRead(byte num) {
  switch (num) {
    case 1:
      return Serial1.read();
      break;
    case 2:
      return Serial5.read();
      break;
    case 3:
      return Serial4.read();
      break;
    case 4:
      return Serial3.read();
      break;
    default:
      return 0;
  }
}

int SerialNumAvailable(byte Num) {
  switch (Num) {
    case 1:
      return Serial1.available();
      break;
    case 2:
      return Serial5.available();
      break;
    case 3:
      return Serial4.available();
      break;
    case 4:
      return Serial3.available();
      break;
    default:
      return 0;
  }
}

// 速度計算用リングバッファをリセットして最初のエントリを書き込む
void resetHistory(PolarSnapshot& snap, int theta, int dist, unsigned long t) {
  snap.vTheta      = 0.0f;
  snap.vDist       = 0.0f;
  snap.thetaHist[0] = theta;
  snap.distHist[0]  = dist;
  snap.timeHist[0]  = t;
  snap.histIdx     = 1;  // 次の書き込み位置
  snap.histCount   = 1;
}

// 実測値からスナップショットの速度を更新（直近 SNAP_HISTORY 回の平均変化量）
void updatePolarSnapshot(PolarSnapshot& snap, int theta, int dist) {
  unsigned long now = millis();
  bool dataChanged = (theta != snap.theta || dist != snap.dist);

  if (!snap.valid) {
    // 初回検出: 履歴リセット
    resetHistory(snap, theta, dist, now);
    snap.lastT = now;
  } else if (dataChanged) {
    // 直近の履歴エントリから経過時間をチェック
    int prevNewest = (snap.histIdx + SNAP_HISTORY - 1) % SNAP_HISTORY;
    bool stale = (snap.histCount == 0 || (now - snap.timeHist[prevNewest] > 500));

    if (stale) {
      // ロスト後の再検出 or 長時間静止後の再移動: 履歴リセット
      resetHistory(snap, theta, dist, now);
    } else {
      // リングバッファに新しい観測を追加
      snap.thetaHist[snap.histIdx] = theta;
      snap.distHist[snap.histIdx]  = dist;
      snap.timeHist[snap.histIdx]  = now;
      snap.histIdx = (snap.histIdx + 1) % SNAP_HISTORY;
      if (snap.histCount < SNAP_HISTORY) snap.histCount++;

      // 最古〜最新の観測から平均速度を計算
      int newestIdx = (snap.histIdx + SNAP_HISTORY - 1) % SNAP_HISTORY;
      int oldestIdx = (snap.histIdx + SNAP_HISTORY - snap.histCount) % SNAP_HISTORY;
      float dtTotal = (float)(snap.timeHist[newestIdx] - snap.timeHist[oldestIdx]);
      if (dtTotal > 5.0f && dtTotal < 2000.0f) {
        snap.vTheta = (float)In180(snap.thetaHist[newestIdx] - snap.thetaHist[oldestIdx]) / dtTotal;
        snap.vDist  = (float)(snap.distHist[newestIdx] - snap.distHist[oldestIdx]) / dtTotal;
      }
    }
    snap.lastT = now;
  }
  // データ未変化: snap.lastT は更新しない

  snap.prevTheta = snap.theta;
  snap.prevDist  = snap.dist;
  snap.theta = theta;
  snap.dist  = dist;
  snap.valid = true;
}

// 最後の検出からmsElapsedミリ秒後の予測位置を返す
// 戻り値: 予測が有効ならtrue、スナップショット未初期化ならfalse
bool predictPolar(const PolarSnapshot& snap, long msElapsed, int* outTheta, int* outDist) {
  if (!snap.valid) return false;
  *outTheta = In360((int)(snap.theta + snap.vTheta * (float)msElapsed));
  *outDist = (snap.dist < 0)
      ? -1
      : constrain((int)(snap.dist + snap.vDist * (float)msElapsed), 0, 300);
  return true;
}

void allCamProcess() {
  /* 全カメラの処理を実行
  
  処理の流れ：
  1. 4台のカメラからデータを順次取得
  2. 最適なカメラを選択（findBall関数）
  3. 選択したカメラの座標を基に位置を計算
  
  注意点：
  - カメラの取り付け位置による座標系の違いを考慮
  - 死角が発生しないようカメラ配置が重要
  */
  int currentYaw = currentDirection();
  for (int i = 1; i <= 4; i++) {
    // 開始後または一時停止解除後1sはカメラ4を無効化
    if (i == 4 && playResumedAt != 0 && millis() - playResumedAt < 1000) continue;
    camRecieve(i, currentYaw);
  }

  getLocation(find(0), 0, &ballTheta, &ballDistance);
  getGoalThetaWithEdges(1, &blueTheta, &blueDistance, &blueLeftEdge, &blueRightEdge);
  getGoalThetaWithEdges(2, &yellowTheta, &yellowDistance, &yellowLeftEdge, &yellowRightEdge);

  // 最終検出時刻（速度計算用の snap.lastT とは独立して管理）
  static unsigned long ballLastSeenT      = 0;
  static unsigned long goalColorLastSeenT = 0;
  static unsigned long oppColorLastSeenT  = 0;
  // 最後に見たゴールのエッジ角度を保持
  static int frozenGoalColorLeftEdgeAbs  = -1, frozenGoalColorRightEdgeAbs  = -1;
  static int frozenOppColorLeftEdgeAbs   = -1, frozenOppColorRightEdgeAbs   = -1;

  // 観測角を絶対方位で履歴化し、機体回転の影響を速度推定から除く。
  if (ballTheta != -1 && ballDistance != -1) {
    updatePolarSnapshot(ballSnap, In360(ballTheta + currentYaw), ballDistance);
    ballLastSeenT = millis();
  } else if (ballTheta != -1) {
    ballSnap.theta = In360(ballTheta + currentYaw);
    ballSnap.valid = true;
    ballLastSeenT  = millis();
  } else if (ballSnap.valid) {
    unsigned long elapsed = millis() - ballLastSeenT;
    if (elapsed <= CAMERA_HOLD_MS) {
      int predictedAbs;
      if (predictPolar(ballSnap, elapsed, &predictedAbs, &ballDistance)) {
        ballTheta = In360(predictedAbs - currentYaw);
      }
    }
  }

  if (toggle2) {
    goalColorTheta = blueTheta;
    goalColorDistance = blueDistance;
    opponentColorTheta = yellowTheta;
    opponentColorDistance = yellowDistance;
    goalColorLeftEdge      = blueLeftEdge;
    goalColorRightEdge     = blueRightEdge;
    opponentColorLeftEdge  = yellowLeftEdge;
    opponentColorRightEdge = yellowRightEdge;
  } else {
    goalColorTheta = yellowTheta;
    goalColorDistance = yellowDistance;
    opponentColorTheta = blueTheta;
    opponentColorDistance = blueDistance;
    goalColorLeftEdge      = yellowLeftEdge;
    goalColorRightEdge     = yellowRightEdge;
    opponentColorLeftEdge  = blueLeftEdge;
    opponentColorRightEdge = blueRightEdge;
  }

  if (goalColorTheta != -1 && goalColorDistance != -1) {
    updatePolarSnapshot(goalColorSnap, In360(goalColorTheta + currentYaw), goalColorDistance);
    goalColorLastSeenT       = millis();
    frozenGoalColorLeftEdgeAbs = (goalColorLeftEdge == -1) ? -1 : In360(goalColorLeftEdge + currentYaw);
    frozenGoalColorRightEdgeAbs = (goalColorRightEdge == -1) ? -1 : In360(goalColorRightEdge + currentYaw);
  } else if (goalColorTheta != -1) {
    goalColorSnap.theta      = In360(goalColorTheta + currentYaw);
    goalColorSnap.valid      = true;
    goalColorLastSeenT       = millis();
    frozenGoalColorLeftEdgeAbs = (goalColorLeftEdge == -1) ? -1 : In360(goalColorLeftEdge + currentYaw);
    frozenGoalColorRightEdgeAbs = (goalColorRightEdge == -1) ? -1 : In360(goalColorRightEdge + currentYaw);
  } else if (goalColorSnap.valid) {
    unsigned long elapsed = millis() - goalColorLastSeenT;
    if (elapsed <= CAMERA_HOLD_MS) {
      int predictedAbs;
      if (predictPolar(goalColorSnap, elapsed, &predictedAbs, &goalColorDistance)) {
        goalColorTheta = In360(predictedAbs - currentYaw);
      }
      goalColorLeftEdge = (frozenGoalColorLeftEdgeAbs == -1)
          ? -1 : In360(frozenGoalColorLeftEdgeAbs - currentYaw);
      goalColorRightEdge = (frozenGoalColorRightEdgeAbs == -1)
          ? -1 : In360(frozenGoalColorRightEdgeAbs - currentYaw);
    }
  }

  if (opponentColorTheta != -1 && opponentColorDistance != -1) {
    updatePolarSnapshot(opponentColorSnap, In360(opponentColorTheta + currentYaw), opponentColorDistance);
    oppColorLastSeenT       = millis();
    frozenOppColorLeftEdgeAbs = (opponentColorLeftEdge == -1) ? -1 : In360(opponentColorLeftEdge + currentYaw);
    frozenOppColorRightEdgeAbs = (opponentColorRightEdge == -1) ? -1 : In360(opponentColorRightEdge + currentYaw);
  } else if (opponentColorTheta != -1) {
    opponentColorSnap.theta = In360(opponentColorTheta + currentYaw);
    opponentColorSnap.valid = true;
    oppColorLastSeenT       = millis();
    frozenOppColorLeftEdgeAbs = (opponentColorLeftEdge == -1) ? -1 : In360(opponentColorLeftEdge + currentYaw);
    frozenOppColorRightEdgeAbs = (opponentColorRightEdge == -1) ? -1 : In360(opponentColorRightEdge + currentYaw);
  } else if (opponentColorSnap.valid) {
    unsigned long elapsed = millis() - oppColorLastSeenT;
    if (elapsed <= CAMERA_HOLD_MS) {
      int predictedAbs;
      if (predictPolar(opponentColorSnap, elapsed, &predictedAbs, &opponentColorDistance)) {
        opponentColorTheta = In360(predictedAbs - currentYaw);
      }
      opponentColorLeftEdge = (frozenOppColorLeftEdgeAbs == -1)
          ? -1 : In360(frozenOppColorLeftEdgeAbs - currentYaw);
      opponentColorRightEdge = (frozenOppColorRightEdgeAbs == -1)
          ? -1 : In360(frozenOppColorRightEdgeAbs - currentYaw);
    }
  }
}

// カメラX座標(pixel)をカメラ中心からの相対角度(度)に変換する共通関数
// x: 0~159, y: 0~119 (camX_max=160, camY_max=120)
// センターオフセット +5.0 はキャリブレーション済み補正値
// edgeXToAngle() と getLocation() で同一キャリブレーション系を使うため共通化
float pixelToRelativeAngle(float x, float y) {
  float thetamax = 59.0f + (-1.13f * y + 0.0207f * y * y) + (-9.87e-5f * y * y * y);
  thetamax = constrain(thetamax, -40.0f, 40.0f);
  float calAngle = ((x + 5.0f) - (camX_max / 2.0f)) / (camX_max / 2.0f);
  calAngle = constrain(calAngle, -1.0f, 1.0f);
  return calAngle * thetamax;
}

// カメラ内のX座標（端点など）を絶対角度に変換するヘルパー
// getLocation()内の角度計算と同一ロジック
float edgeXToAngle(int cam, float x, float y) {
  return 90.0f * (cam - 1) + pixelToRelativeAngle(x, y);
}

int getGoalDistanceFromCameras(int object, int cam1, int cam2) {
  int theta1 = -1;
  int distance1 = -1;
  getLocation(cam1, object, &theta1, &distance1);

  if (cam2 == 0) {
    return distance1;
  }

  int theta2 = -1;
  int distance2 = -1;
  getLocation(cam2, object, &theta2, &distance2);

  if (distance1 != -1 && distance2 != -1) {
    return (distance1 + distance2) / 2;
  }
  if (distance1 != -1) {
    return distance1;
  }
  return distance2;
}

// ゴールの角度と端点角度を取得（隣接カメラをまたぐ場合に対応）
// object: 1=青ゴール, 2=黄ゴール
void getGoalThetaWithEdges(int object, int *theta, int *distance, int *leftEdge, int *rightEdge) {
  int *gX = (object == 1) ? blueX  : yellowX;
  int *gY = (object == 1) ? blueY  : yellowY;
  int *gW = (object == 1) ? blueW  : yellowW;

  *leftEdge  = -1;
  *rightEdge = -1;

  // 隣接カメラペアを全て確認してクロスカメラ検出を試みる
  for (int n = 0; n < 4; n++) {
    int n2 = (n + 1) % 4;

    bool cam1Valid = hasValidDetection(gX[n], gY[n]) && gW[n] > 0;
    bool cam2Valid = hasValidDetection(gX[n2], gY[n2]) && gW[n2] > 0;

    if (!cam1Valid || !cam2Valid) continue;

    // cam n: ゴールが右寄り（左端が見えている）
    // cam n+1: ゴールが左寄り（右端が見えている）
    float leftEdgeX     = gX[n]  - gW[n]  / 2.0f;
    float rightEdgeX    = gX[n2] + gW[n2] / 2.0f;
    float cam1RightEdge = gX[n]  + gW[n]  / 2.0f;
    float cam2LeftEdge  = gX[n2] - gW[n2] / 2.0f;

    if (cam1RightEdge >= camX_max / 2.0f && cam2LeftEdge <= camX_max / 2.0f) {

      float lAngle = edgeXToAngle(n + 1, leftEdgeX,  (float)gY[n]);
      float rAngle = edgeXToAngle(n2 + 1, rightEdgeX, (float)gY[n2]);

      // 円形平均で中心角を計算（ラップアラウンド対応）
      float lRad = lAngle * (float)M_PI / 180.0f;
      float rRad = rAngle * (float)M_PI / 180.0f;
      float centerDeg = atan2f(sinf(lRad) + sinf(rRad), cosf(lRad) + cosf(rRad)) * 180.0f / (float)M_PI;

      *theta = In360((int)centerDeg);
      *leftEdge  = In360((int)lAngle);
      *rightEdge = In360((int)rAngle);

      // 距離は単カメラフォールバックで計算
      *distance = getGoalDistanceFromCameras(object, n + 1, n2 + 1);
      return;
    }
  }

  // クロスカメラでない場合: 通常の単カメラ処理
  int bestCam = find(object);
  if (bestCam == 0) {
    *theta    = -1;
    *distance = -1;
    return;
  }
  getLocation(bestCam, object, theta, distance);

  // 単カメラでも幅からエッジ角度を推定
  if (*theta != -1 && gW[bestCam - 1] > 0) {
    float halfW = gW[bestCam - 1] / 2.0f;
    float cx = (float)gX[bestCam - 1];
    float cy = (float)gY[bestCam - 1];
    *leftEdge  = In360((int)edgeXToAngle(bestCam, cx - halfW, cy));
    *rightEdge = In360((int)edgeXToAngle(bestCam, cx + halfW, cy));
  }
}

void getLocation(int cam, int object, int *theta, int *distance) {  //物体の位置を極座標系で取得
  // カメラ番号が0の場合（すべてのカメラが無効）は-1を返す
  if (cam == 0) {
    *theta = -1;
    *distance = -1;
    return;
    
  }

  int x = 0;
  int y = 0;
  int h = 0;  // 高さ情報を追加
  bool isVisible = true;  // 物体が見えているかどうかのフラグ

  switch (object) {
    case 0:
      x = ballX[cam - 1];
      y = ballY[cam - 1];
      h = ballH[cam - 1];  // ボールの高さ
      isVisible = (x != 0 || y != 0);  // ボールが見えているか確認
      break;
    case 1:
      x = blueX[cam - 1];
      y = blueY[cam - 1];
      h = blueH[cam - 1];  // 青ゴールの高さ
      isVisible = (x != 0 || y != 0);  // 青ゴールが見えているか確認
      break;
    case 2:
      x = yellowX[cam - 1];
      y = yellowY[cam - 1];
      h = yellowH[cam - 1];  // 黄ゴールの高さ
      isVisible = (x != 0 || y != 0);  // 黄ゴールが見えているか確認
      break;
  }

  // ボールセンサーでボールが検出された場合は真正面・距離0として処理
  isVisible = hasValidDetection(x, y);

  if (object == 0 && ballSensor < kickerThreshold) {
    *theta = 0;    // 0度方向
    *distance = 0; // 距離0cm
    return;
  }

  // 通常の可視性チェック
  if (!isVisible) {
    *theta = -1;  // 物体が見えていない場合は-1を返す
    *distance = -1;
    return;
  }

  // カメラの取り付け角度（基準角）を計算
  int baseAngle = 90 * (cam - 1);

  // pixelToRelativeAngle で edgeXToAngle と同一キャリブレーション系を使用
  float relativeAngle = pixelToRelativeAngle((float)x, (float)y);

  // 最終的な角度を計算（基準角 + 相対角度）
  float calTheta = baseAngle + relativeAngle;
  *theta = In360(int(calTheta));
  

  // フラグに基づいて距離推定方法を選択
  if (usePolynomialDistance) {
    *distance = calculateDistance(relativeAngle, float(y), h);  // 高さ情報を追加
  } else {
    *distance = getDistanceFromCoordinates(x, y, h);
  }
}

byte find(int object) {  // objectの型をintに変更
  /* 選択した物体を最も見やすい位置で捉えているカメラを選択
    スコア計算方法：
    - 各カメラのピクセル数を比較
    - ピクセル数が大きいほど、そのカメラが適している
    - 座標が(0,0)または(-100,-100)のカメラは無効
    戻り値：最適なカメラの番号（1-4）、全カメラ無効の場合は0
  */

  // 各カメラのピクセル数と有効性を格納
  struct CameraScore {
    int pixels;
    bool isValid;
  };
  CameraScore scores[4];
  static byte lastBestCam[3] = {0, 0, 0};

  if (object < 0 || object > 2) {
    return 0;
  }
  
  // 物体タイプに応じてピクセル数と有効性を計算
  for (int i = 0; i < 4; i++) {
    int x = 0, y = 0, p = 0;
    switch (object) {
      case 0:
        x = ballX[i];
        y = ballY[i];
        p = ballP[i];
        break;
      case 1:
        x = blueX[i];
        y = blueY[i];
        p = blueP[i];
        break;
      case 2:
        x = yellowX[i];
        y = yellowY[i];
        p = yellowP[i];
        break;
    }
    
    // 座標が(0,0)または(-100,-100)の場合、またはピクセル数が無効な場合は無効とする
    if (!hasValidDetection(x, y) || p == -100) {
      scores[i].pixels = -1;
      scores[i].isValid = false;
    } else {
      scores[i].pixels = p;
      scores[i].isValid = true;
    }
  }
  
  // 有効なカメラの中で最もピクセル数が大きいものを選択
  byte bestCam = 1;
  int bestPixels = -1;
  bool anyValidCamera = false;
  
  for (int i = 0; i < 4; i++) {
    if (scores[i].isValid && scores[i].pixels > bestPixels) {
      bestPixels = scores[i].pixels;
      bestCam = i + 1;
      anyValidCamera = true;
    }
  }
  
  // すべてのカメラが無効な場合は0を返す
  if (!anyValidCamera) {
    lastBestCam[object] = 0;
    return 0;
  }

  byte previousCam = lastBestCam[object];
  if (previousCam >= 1 && previousCam <= 4) {
    int previousPixels = scores[previousCam - 1].pixels;
    if (scores[previousCam - 1].isValid && previousPixels * 5 >= bestPixels * 4) {
      bestCam = previousCam;
    }
  }

  lastBestCam[object] = bestCam;
  return bestCam;
}

// 定期的にバッファをクリア
void clearSerialBuffers() {
  while(Serial1.available() > 40) Serial1.read();
  while(Serial3.available() > 40) Serial3.read();
  while(Serial4.available() > 40) Serial4.read();
  while(Serial5.available() > 40) Serial5.read();
}
