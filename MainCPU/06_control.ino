// 目標方向への回転制御
const int kickerCompassRotateThreshold = 45;
const int kickerCameraRotateThreshold = 45;
const int kickerCameraGoalClamp = 45;

void rotateToTarget(int targetTheta, unsigned long timeoutMs, float gain, int threshold, bool brakeAfter) {
    // 現在の絶対角度を取得
    int currentAbs = currentDirection();
    // 目標の絶対角度を計算（現在の角度を基準に相対角度から計算）
    int targetAbs = In360(currentAbs + targetTheta);

    unsigned long startTime = millis();
    int diffTheta = In180(targetAbs - currentAbs);
    while (true) {
        allCamProcess();  // ブロッキング中もカメラバッファを継続消費
        // 現在の絶対角度を取得
        currentAbs = currentDirection();
        // 現在の絶対角度と目標絶対角度の差を計算（-180～180度の範囲で）
        diffTheta = In180(targetAbs - currentAbs);

        // 目標角度に到達、またはタイムアウトした場合はループを抜ける
        if (abs(diffTheta) < threshold ||
            (timeoutMs != 0 && millis() - startTime >= timeoutMs)) {
            break;
        }

        // 回転制御
        turnTheta(diffTheta, gain);
    }

    /*
    turnTheta(diffTheta, gain);
    delay(50); // 回転開始後に少し待つ（モーターの反応を待つため）
    */
    if (brakeAfter) {
        brake(true);
    }
}

void turnToTargetIF(int targetTheta) {
  // 現在の絶対角度を取得
  int currentAbs = currentDirection();
  // 目標の絶対角度を計算（現在の角度を基準に相対角度から計算）
  int targetAbs = In180(currentAbs + targetTheta);
  int diffThreshold = 20;
  int diffTheta = In180(targetAbs - currentAbs);
  if (abs(In180(diffTheta)) < diffThreshold) {
    delay(0);
  } else {
    turnTheta(diffTheta);
  }
}


// ゴール方向の取得
int getValidatedGoalDirection() {
    if (goalColorTheta != -1 && (abs(In180(goalColorTheta)) < 30)) {
        return In180(goalColorTheta);
    } else if (goalColorTheta != -1) {
        if (In180(goalColorTheta) > 0) {
            return 30;
        } else {
            return -30;
        }
    }
    return In180(goalDirection - currentDirection());
}

// ボール追跡時の方向制御
int getBallChasingDirection() {
    if ((ballTheta < 90 || ballTheta > 270) && goalColorTheta != -1) {
        // ヒステリシス制御: コンパス→カメラは80cm未満、カメラ→コンパスは90cm以上
        if (kickerUseCompassForGoal) {
            if (goalColorDistance != -1 && goalColorDistance < 80 && !(abs(In180(convertToAbsoluteAngle(goalColorTheta))) > 70)) {
                kickerUseCompassForGoal = false; // カメラ補正に切り替え
            } else {
                return In180(goalDirection - currentDirection()); // コンパス補正
            }
        } else {
            if (goalColorDistance == -1 || goalColorDistance >= 90 || abs(In180(convertToAbsoluteAngle(goalColorTheta))) > 70) {
                kickerUseCompassForGoal = true; // コンパス補正に戻る
                return In180(goalDirection - currentDirection());
            }
        }
        // カメラ補正ロジック
        // ロボット正面(0°)がゴールの角度範囲内にある場合は補正不要
        if (goalColorLeftEdge != -1 && goalColorRightEdge != -1) {
            // 中心と端の中間点で範囲を狭める
            int le = In180((goalColorLeftEdge + goalColorTheta) / 2);
            int re = In180((goalColorRightEdge + goalColorTheta) / 2);
            if (le <= 0 && re >= 0) return 0;
        } else {
            // エッジ情報なし時のフォールバック: 中心が±20°以内なら補正しない
            if (abs(In180(goalColorTheta)) < 20) return 0;
        }
        int goalTheta = In180(goalColorTheta);
        if (abs(goalTheta) < kickerCameraGoalClamp) {
            return goalTheta;
        }
        return (goalTheta > 0) ? kickerCameraGoalClamp : -kickerCameraGoalClamp;
    }
    return In180(goalDirection - currentDirection());
}

bool isCameraGoalUsedForBallChasing() {
    return (ballTheta < 90 || ballTheta > 270) && goalColorTheta != -1 && !kickerUseCompassForGoal;
}

// ボールが見えない時の共通処理
void handleNoBallVisible() {
    int targetTheta = In180(goalDirection - currentDirection());
    if (abs(In180(targetTheta)) >= 30) {
        // 短時間でタイムアウトし、次ループでボール検出に戻れるようにする
        rotateToTarget(targetTheta, 50UL);
    } else {
        echoPos2(50);
    }
}

// 共通の初期処理
void commonPlaySetup() {
    digitalWrite(led, LOW);  // 試合中はLED消灯
    setGoalDirection();
    // clearSerialBuffers は毎ループ呼ぶと allCamProcess 直後にバッファが空になり
    // 次フレームが届く前に camRecieve が何も読まずに返るため、200ms 間隔に制限する
    static unsigned long lastClearTime = 0;
    if (millis() - lastClearTime >= 200) {
        clearSerialBuffers();
        lastClearTime = millis();
    }
    allCamProcess();
    updateDisplay();
}

// OLED表示の更新（試合中は常に消灯、500ms間隔でクリア）
void updateDisplay() {
    static unsigned long lastClearTime = 0;
    if (millis() - lastClearTime < 500) return;
    lastClearTime = millis();
    display.clearDisplay();
    display.display();
}

// 通常プレイ用の遠距離ボール処理
void handleLongDistanceBall() {
    bool usingCameraGoal = isCameraGoalUsedForBallChasing();
    int targetTheta = getBallChasingDirection();
    int rotateThreshold = usingCameraGoal ? kickerCameraRotateThreshold : kickerCompassRotateThreshold;
    if (abs(In180(targetTheta)) >= rotateThreshold) {
        rotateToTarget(targetTheta, 50UL);
    } else {
        rotateAndMoveTheta(80, getApproachAngle(ballTheta), In180(targetTheta), true, true);//なんか加えてみた:D
        lineCheck(true);
    }
}

// 通常プレイ用の近距離ボール処理
void handleShortDistanceBall() {
    bool usingCameraGoal = isCameraGoalUsedForBallChasing();
    int targetTheta = getBallChasingDirection();
    // targetTheta = In180(ballTheta);
    // targetTheta = In180(goalDirection - currentDirection());
    int rotateThreshold = usingCameraGoal ? kickerCameraRotateThreshold : kickerCompassRotateThreshold;
    if (abs(In180(targetTheta)) >= rotateThreshold) {
        rotateToTarget(targetTheta, 50UL);
    } else {
        if((abs(In180(ballTheta) - In180(goalColorTheta)) < 30) && goalColorTheta != -1) {
            // rotateAndMoveTheta(80, ballTheta, In180(targetTheta), true, true);
            if(abs(In180(ballTheta)) < 20) {
                chase(80, targetTheta, true);
            } else {
                chase(80, targetTheta, true);
            }
        } else {
            if(abs(In180(ballTheta)) < 20) {
                chase(80, targetTheta, true);
            } else {
                chase(80, targetTheta, true);
            }
        }
        lineCheck(true);
    }
}

// ドリブラー用の遠距離ボール処理
void handleLongDistanceBallWithDribbler() {
    bool usingCameraGoal = isCameraGoalUsedForBallChasing();
    int targetTheta = getBallChasingDirection();
    int rotateThreshold = usingCameraGoal ? kickerCameraRotateThreshold : kickerCompassRotateThreshold;
    if (abs(In180(targetTheta)) >= rotateThreshold) {
        rotateToTarget(targetTheta, 50UL);
    } else {
        rotateAndMoveTheta(80, ballTheta, In180(targetTheta), true, true);
        lineCheck(true);
    }
}

// ドリブラー用の近距離ボール処理
void handleShortDistanceBallWithDribbler() {
    turnOnDribbler();

    if (!isHoldingBall()) {
        // ボール未保持: コンパスで前に向きつつゆっくりボールに近づく
        int targetTheta = In180(goalDirection - currentDirection());
        chase(50, targetTheta, false);
        return;
    }

    // --- ボール保持後のシーケンス ---

    // ステップ1: 500msで前進速度を40→0に落として保持を安定化
    unsigned long startTime = millis();
    const unsigned long decelDuration = 500UL;
    while (true) {
        unsigned long elapsed = millis() - startTime;
        if (elapsed >= decelDuration) break;
        int forwardSpeed = 40 - (int)((40UL * elapsed) / decelDuration);
        if (forwardSpeed < 0) forwardSpeed = 0;
        turnOnDribbler();
        moveTheta(forwardSpeed, 0, true, false);
        lineCheck(true);
        if (!isHoldingBall()) return;
        if (!toggle1) { brake(true); return; }
    }
    brake(true);

    // ステップ2: 最低回転パワーで旋回してゴールに背中を向ける
    unsigned long rotateStartTime = millis();
    while (true) {
        allCamProcess();  // ブロッキング中もカメラバッファを継続消費
        int backTarget = In180((goalDirection + 180) - currentDirection());
        if (abs(backTarget) < 20 || (millis() - rotateStartTime >= 2000)) {
            break;
        }
        // diffが負なら左回転、正なら右回転
        minPowerRotate(backTarget < 0);
    }
    brake(true);
    if (!isHoldingBall()) return;

    // ステップ3: カメラでゴールを見ながらゴール方向に移動して近づく
    allCamProcess();
    while (goalColorDistance > 20 || goalColorDistance == -1) {
        turnOnDribbler();
        allCamProcess();
        if (goalColorTheta != -1) {
            // カメラでゴールが見える: ゴール方向に直接移動
            moveTheta(40, goalColorTheta, true, false);
        } else {
            // カメラでゴールが見えない: コンパスで推定したゴール方向に移動
            int goalRelative = In180(goalDirection - currentDirection());
            moveTheta(40, In360(goalRelative), true, false);
        }
        lineCheck(true);
        if (!isHoldingBall()) return;
        if (!toggle1) { brake(true); return; }
    }
    brake(true);

    // ステップ4: ゴール方向に向く
    int goalTarget = In180(goalDirection - currentDirection());
    rotateToTarget(goalTarget, 1500, -0.4, 15, true);

    // ステップ5: キック
    turnOffDribbler();
    digitalWrite(kicker, HIGH);
    delay(30);
    digitalWrite(kicker, LOW);
    delay(150);
}
