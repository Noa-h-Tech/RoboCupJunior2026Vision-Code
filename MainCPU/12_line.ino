void writeLineEscapedByte(byte data) {
    if (data == LINE_START_MARKER || data == LINE_END_MARKER || data == LINE_ESCAPE_MARKER) {
        Serial6.write(LINE_ESCAPE_MARKER);
        Serial6.write(data ^ 0x20);
    } else {
        Serial6.write(data);
    }
}

bool verifyLineResponseFrame() {
    if (lineRespIndex < 3) return false;

    byte length = lineRespBuffer[1];
    int expectedSize = 2 + length + 1; // ID + LENGTH + PAYLOAD + CHECKSUM
    if (lineRespIndex != expectedSize) return false;

    byte checksum = 0;
    for (int i = 0; i < 2 + length; i++) {
        checksum ^= lineRespBuffer[i];
    }
    return checksum == lineRespBuffer[2 + length];
}

// 閾値送信関数
void sendLineThresholds() {
    int thresholds[] = {line1Threshold, line2Threshold, line3Threshold, line4Threshold};
    byte checksum = LINE_CMD_SET_ALL_THRESHOLDS ^ 8; // ID ^ LENGTH

    Serial6.write(LINE_START_MARKER);
    writeLineEscapedByte(LINE_CMD_SET_ALL_THRESHOLDS);
    writeLineEscapedByte((byte)8); // length: 4閾値 × 2バイト

    for (int i = 0; i < 4; i++) {
        byte upper = (thresholds[i] >> 8) & 0xFF;
        byte lower = thresholds[i] & 0xFF;
        writeLineEscapedByte(upper);
        writeLineEscapedByte(lower);
        checksum ^= upper;
        checksum ^= lower;
    }

    writeLineEscapedByte(checksum);
    Serial6.write(LINE_END_MARKER);
}

// デバッグモード切替
void setLineDebugMode(bool enabled) {
    lineDebugMode = enabled;
    byte cmdId = enabled ? LINE_CMD_REQUEST_DEBUG : LINE_CMD_REQUEST_NORMAL;
    byte checksum = cmdId ^ 0; // ID ^ LENGTH(0)

    Serial6.write(LINE_START_MARKER);
    writeLineEscapedByte(cmdId);
    writeLineEscapedByte((byte)0); // length
    writeLineEscapedByte(checksum);
    Serial6.write(LINE_END_MARKER);
}

// レスポンス受信処理
void lineCheck(bool avoid) {
    while (Serial6.available() > 0) {
        byte data = Serial6.read();

        if (!lineRespReceiving) {
            if (data == LINE_START_MARKER) {
                lineRespReceiving = true;
                lineRespEscapeNext = false;
                lineRespIndex = 0;
            }
            continue;
        }

        if (lineRespEscapeNext) {
            data ^= 0x20;
            lineRespEscapeNext = false;
        } else if (data == LINE_ESCAPE_MARKER) {
            lineRespEscapeNext = true;
            continue;
        } else if (data == LINE_START_MARKER) {
            // 受信途中に開始マーカーが来た場合は同期し直す
            lineRespEscapeNext = false;
            lineRespIndex = 0;
            continue;
        } else if (data == LINE_END_MARKER) {
            processLineResponse(avoid);
            lineRespReceiving = false;
            lineRespEscapeNext = false;
            lineRespIndex = 0;
            continue;
        }

        if (lineRespIndex < 64) {
            lineRespBuffer[lineRespIndex++] = data;
        } else {
            // バッファオーバー時はフレーム破棄
            lineRespReceiving = false;
            lineRespEscapeNext = false;
            lineRespIndex = 0;
        }
    }
}

// レスポンス処理
void processLineResponse(bool avoid) {
    if (!verifyLineResponseFrame()) return;

    byte respId = lineRespBuffer[0];
    byte length = lineRespBuffer[1];

    switch (respId) {
        case LINE_RESP_DETECTED:
            processDetectedLines();
            if (avoid && detectedLineCount > 0) {
                processLineAvoidance();
            }
            break;
        case LINE_RESP_DEBUG:
            if (length == 32) {
                processDebugData();
            }
            break;
        case LINE_RESP_ACK:
            // ACK受信確認（必要に応じてログ）
            break;
    }
}

// 検出ライン処理
void processDetectedLines() {
    if (lineRespIndex < 4) return;

    byte length = lineRespBuffer[1];
    if (length < 1) return;

    int countFromPayload = lineRespBuffer[2];
    int maxCountByLength = (length - 1) / 3;
    detectedLineCount = min(16, min(countFromPayload, maxCountByLength));

    // lineValues配列をリセット
    for (int i = 0; i < 16; i++) {
        lineValues[i] = 0;
    }

    for (int i = 0; i < detectedLineCount && i < 16; i++) {
        int offset = 3 + i * 3;
        if (offset + 2 >= (2 + length)) break;

        int sensorId = lineRespBuffer[offset];
        int value = (lineRespBuffer[offset + 1] << 8) | lineRespBuffer[offset + 2];

        if (sensorId < 16) {
            detectedLineSensors[i] = sensorId;
            detectedLineValues[i] = value;
            lineValues[sensorId] = value;
        }
    }
}

// デバッグデータ処理（全センサー値）
void processDebugData() {
    int payloadEnd = 2 + lineRespBuffer[1]; // checksumは含めない
    for (int i = 0; i < 16; i++) {
        int offset = 2 + i * 2;
        if (offset + 1 >= payloadEnd) break;
        lineValues[i] = (lineRespBuffer[offset] << 8) | lineRespBuffer[offset + 1];
    }
}

// ライン脱出方向の20cm以内に障害物があれば、最も開けたセンサー方向に補正
// ドリブラーOFF時のみ呼び出すこと（getEchoがdribblerを止めるため）
int adjustEscapeThetaByUltrasonic(int escapeTheta) {
    const int sensorAngles[4] = {0, 90, 180, 270}; // 前, 右, 後, 左
    long sensorDist[4];
    sensorDist[0] = echoF;
    sensorDist[1] = echoR;
    sensorDist[2] = echoB;
    sensorDist[3] = echoL;

    // 3cm未満の値はノイズとして無視（タイムアウト=障害物なし扱い）
    for (int i = 0; i < 4; i++) {
        if (sensorDist[i] > 0 && sensorDist[i] < 3) {
            sensorDist[i] = 0;
        }
    }

    // 脱出方向に最も近いセンサーを探す
    int closestSensor = 0;
    int minDiff = 361;
    for (int i = 0; i < 4; i++) {
        if (sensorDist[i] < 0) continue; // 無効センサーはスキップ
        int diff = abs(In180(escapeTheta - sensorAngles[i]));
        if (diff < minDiff) {
            minDiff = diff;
            closestSensor = i;
        }
    }

    // 脱出方向のセンサーが20cm以内に障害物を検出している場合（0=タイムアウト=障害物なし）
    long escapeDist = sensorDist[closestSensor];
    if (escapeDist > 0 && escapeDist <= 20) {
        // 最も距離が大きい（障害物が少ない）センサー方向を選択
        int bestSensor = -1;
        long maxDist = -1;
        for (int i = 0; i < 4; i++) {
            long d = sensorDist[i];
            if (d < 0) continue; // 無効センサーはスキップ
            if (d == 0) d = 9999; // タイムアウト=障害物なし=非常に遠い
            if (d > maxDist) {
                maxDist = d;
                bestSensor = i;
            }
        }
        if (bestSensor >= 0) {
            return sensorAngles[bestSensor];
        }
    }

    return escapeTheta;
}

// ライン脱出ループ中にいずれかのセンサーが20cm以内を検出したら、最も開けた方向へ上書き
// ドリブラーOFF時のみ呼び出すこと（getEchoがdribblerを止めるため）
int overrideEscapeThetaIfNearWall(int escapeTheta) {
    const int sensorAngles[4] = {0, 90, 180, 270}; // 前, 右, 後, 左
    long sensorDist[4];
    sensorDist[0] = echoF;
    sensorDist[1] = echoR;
    sensorDist[2] = echoB;
    sensorDist[3] = echoL;

    // 3cm未満の値はノイズとして無視
    for (int i = 0; i < 4; i++) {
        if (sensorDist[i] > 0 && sensorDist[i] < 3) {
            sensorDist[i] = 0;
        }
    }

    // いずれかのセンサーが20cm以内に壁を検出しているか確認
    bool wallDetected = false;
    for (int i = 0; i < 4; i++) {
        if (sensorDist[i] > 0 && sensorDist[i] <= 20) {
            wallDetected = true;
            break;
        }
    }

    if (!wallDetected) return escapeTheta;

    // 最も距離が大きい（最も開けた）センサー方向を選択
    int bestSensor = -1;
    long maxDist = -1;
    for (int i = 0; i < 4; i++) {
        long d = sensorDist[i];
        if (d < 0) continue; // 無効センサーはスキップ
        if (d == 0) d = 9999; // タイムアウト=障害物なし=非常に遠い
        if (d > maxDist) {
            maxDist = d;
            bestSensor = i;
        }
    }
    if (bestSensor >= 0) {
        return sensorAngles[bestSensor];
    }

    return escapeTheta;
}

// ライン回避処理
void processLineAvoidance() {
    // センサーの範囲と対応するブロック番号を定義
    struct SensorConfig {
        int start;
        int end;
        int block;
        int threshold;
    };

    const SensorConfig configs[] = {
        {0, 2, 2, line2Threshold},  // 右ブロック
        {2, 4, 1, line1Threshold},  // 左ブロック
        {4, 6, 4, line4Threshold},  // 内側ブロック
        {6, 8, 3, line3Threshold},  // 後ろブロック
        {8, 16, 4, line4Threshold}  // 内側ブロック
    };

    float sumX = 0;
    float sumY = 0;
    int detectedSensorsCount = 0;
    int detectedBlock = 0;

    // 検出されたセンサーの角度をベクトル合算
    for (int i = 0; i < detectedLineCount && i < 16; i++) {
        int sensorId = detectedLineSensors[i];
        if (sensorId < 16 && enableLineSensors[sensorId]) {
            float rad = lineAngles[sensorId] * PI / 180.0;
            sumX += cos(rad);
            sumY += sin(rad);
            detectedSensorsCount++;

            if (detectedBlock == 0) {
                // ブロック決定
                for (const auto& config : configs) {
                    if (sensorId >= config.start && sensorId < config.end) {
                        detectedBlock = config.block;
                        break;
                    }
                }
            }
        }
    }

    if (detectedSensorsCount > 0) {
        brake(true); // 脱出方向が決まるまでブレーキ
        float averageRad = atan2(sumY, sumX);
        int averageAngle = (int)(averageRad * 180.0 / PI);
        int escapeTheta = In360(averageAngle + 180);
        bool goalNear = false;
        int goalEscapeTheta = 0;

        if (goalColorDistance != -1 && goalColorDistance <= 30.0
            && abs(In180(convertToAbsoluteAngle(goalColorTheta))) <= 40) {
            goalNear = true;
            goalEscapeTheta = In360(goalColorTheta + 180);
        }
        if (opponentColorDistance != -1 && opponentColorDistance <= 30.0
            && abs(In180(convertToAbsoluteAngle(opponentColorTheta))) >= 140) {
            if (!goalNear || opponentColorDistance < goalColorDistance || goalColorDistance == -1) {
                goalNear = true;
                goalEscapeTheta = In360(opponentColorTheta + 180);
            }
        }

        if (goalNear) {
            escapeTheta = goalEscapeTheta;
        } else if (ballTheta != -1) {
            // ライン脱出方向(70%) + ボール方向(30%) をベクトルブレンド
            float bx = 0.7f * cosf(escapeTheta * PI / 180.0f) + 0.3f * cosf(ballTheta * PI / 180.0f);
            float by = 0.7f * sinf(escapeTheta * PI / 180.0f) + 0.3f * sinf(ballTheta * PI / 180.0f);
            escapeTheta = In360((int)(atan2f(by, bx) * 180.0f / PI));
        }
        // ドリブラーOFF時のみ: 脱出方向30cm以内に障害物があれば最も開けた方向へ補正
        if (!isDribblerOn) {
            escapeTheta = adjustEscapeThetaByUltrasonic(escapeTheta);
        }
        digitalWrite(led, HIGH);
        moveTheta(100, escapeTheta, true);

        // ラインに触れなくなるまで脱出を継続
        unsigned long escapeStartTime = millis();
        const unsigned long maxEscapeTime = 150;
        //digitalWrite(led, HIGH);
        tone(buzzer, 2794, 100);
        while (millis() - escapeStartTime < maxEscapeTime) {
            lineCheck(false); // センサー値を更新（avoid=falseで回避動作は起動しない）

            // ライン検出がなくなったかチェック
            if (detectedLineCount == 0) break;

            // 脱出角度を再計算
            float loopSumX = 0;
            float loopSumY = 0;
            int loopDetectedCount = 0;
            for (int i = 0; i < detectedLineCount && i < 16; i++) {
                int sensorId = detectedLineSensors[i];
                if (sensorId < 16 && enableLineSensors[sensorId]) {
                    float rad = lineAngles[sensorId] * PI / 180.0;
                    loopSumX += cos(rad);
                    loopSumY += sin(rad);
                    loopDetectedCount++;
                }
            }

            if (loopDetectedCount > 0) {
                float loopAverageRad = atan2(loopSumY, loopSumX);
                int loopAverageAngle = (int)(loopAverageRad * 180.0 / PI);
                int loopEscapeTheta = In360(loopAverageAngle + 180);
                bool loopGoalNear = false;
                int loopGoalEscapeTheta = 0;

                if (goalColorDistance != -1 && goalColorDistance <= 40.0
                    && abs(In180(convertToAbsoluteAngle(goalColorTheta))) <= 40) {
                    loopGoalNear = true;
                    loopGoalEscapeTheta = In360(goalColorTheta + 180);
                }
                if (opponentColorDistance != -1 && opponentColorDistance <= 40.0
                    && abs(In180(convertToAbsoluteAngle(opponentColorTheta))) >= 140) {
                    if (!loopGoalNear || opponentColorDistance < goalColorDistance || goalColorDistance == -1) {
                        loopGoalNear = true;
                        loopGoalEscapeTheta = In360(opponentColorTheta + 180);
                    }
                }

                if (loopGoalNear) {
                    loopEscapeTheta = loopGoalEscapeTheta;
                } else if (ballTheta != -1) {
                    // ライン脱出方向(70%) + ボール方向(30%) をベクトルブレンド
                    float bx = 0.7f * cosf(loopEscapeTheta * PI / 180.0f) + 0.3f * cosf(ballTheta * PI / 180.0f);
                    float by = 0.7f * sinf(loopEscapeTheta * PI / 180.0f) + 0.3f * sinf(ballTheta * PI / 180.0f);
                    loopEscapeTheta = In360((int)(atan2f(by, bx) * 180.0 / PI));
                }
                // ドリブラーOFF時のみ: いずれかのセンサーが20cm以内なら最も開けた方向へ上書き
                if (!isDribblerOn) {
                    loopEscapeTheta = overrideEscapeThetaIfNearWall(loopEscapeTheta);
                }

                moveTheta(100, loopEscapeTheta, true);
            }
        }
        {
            unsigned long stabilizeStart = millis();
            while (millis() - stabilizeStart < 200) {
                lineCheck(false); // バッファを消費し続ける（バッファ溢れ防止）
            }
        }
        brake(false);
        lineFlag = detectedBlock;
        digitalWrite(led, LOW);
        noTone(buzzer);
        Serial6Reset();
        return;
    }

    lineFlag = 0;
}

/* 旧式のprocessLineValues関数（新プロトコルではprocessLineAvoidanceに置き換え）
void processLineValues(bool avoid) {
    // センサーの範囲と対応するブロック番号を定義
    struct SensorConfig {
        int start;
        int end;
        int block;
        int threshold;
    };

    const SensorConfig configs[] = {
        {0, 2, 2, line2Threshold},  // 右ブロック
        {2, 4, 1, line1Threshold},  // 左ブロック
        {4, 6, 4, line4Threshold},  // 内側ブロック
        {6, 8, 3, line3Threshold},  // 後ろブロック
        {8, 16, 4, line4Threshold}  // 内側ブロック
    };

    float sumX = 0;
    float sumY = 0;
    int detectedSensorsCount = 0;
    int detectedBlock = 0;

    for (const auto& config : configs) {
        for (int i = config.start; i < config.end; i++) {
            if ((enableLineSensors[i]) && (lineValues[i] > config.threshold) && avoid) {
                float rad = lineAngles[i] * PI / 180.0;
                sumX += cos(rad);
                sumY += sin(rad);
                detectedSensorsCount++;
                if (detectedBlock == 0) {
                    detectedBlock = config.block;
                }
            }
        }
    }

    if (detectedSensorsCount > 0) {
        float averageRad = atan2(sumY, sumX);
        int averageAngle = (int)(averageRad * 180.0 / PI);
        moveTheta(100, In360(averageAngle + 180), true);

        unsigned long escapeStartTime = millis();
        const unsigned long maxEscapeTime = 500;

        while (millis() - escapeStartTime < maxEscapeTime) {
            lineCheck(false);
            bool stillOnLine = false;
            for (const auto& config : configs) {
                for (int i = config.start; i < config.end; i++) {
                    if (enableLineSensors[i] && lineValues[i] > config.threshold) {
                        stillOnLine = true;
                        break;
                    }
                }
                if (stillOnLine) break;
            }
            if (!stillOnLine) break;
        }

        brake(false);
        lineFlag = detectedBlock;
        return;
    }

    lineFlag = 0;
}
*/

/* 旧式のライン処理
void lineCheck() {
  if (line1Check() || line2Check() || line3Check() || line4Check()) {
    digitalWrite(led, HIGH);
    brake(true);
    delay(100);
    int targetTheta = In180(goalDirection - currentDirection()); // 必ずコンパスの値を使用
    allCamProcess();
    targetTheta = In180(goalColorTheta);
    if (abs(In180(targetTheta)) >= 18) {
      while (abs(In180(targetTheta)) >= 18) {
          targetTheta = In180(goalDirection - currentDirection());
          turnTheta(targetTheta);
        }
    }
    //lineleaveWall(50);
    //onlyCamleaveWall(50);
    long currentTime = millis();
    while (millis() - currentTime < 500) {
      echoPos(50);
    }
    digitalWrite(led, LOW);
  } 
}

bool line1Check(){
  return line1 > line1Threshold;
}

bool line2Check(){
  return line2 > line2Threshold;
}

bool line3Check(){
  return line3 > line3Threshold;
}

bool line4Check(){
  return line4 > line4Threshold;
}

void lineleaveWall(int speed) {
  allCamProcess();
  if (opponentColorTheta >= 90 && opponentColorTheta <= 120) {
    moveTheta(speed, 30, true);
  } else if (opponentColorTheta >= 240 && opponentColorTheta <= 270) {
    moveTheta(speed, 330, true);
  } else if(goalColorTheta >= 60 && goalColorTheta <= 90){
    moveTheta(speed, 150, true);
  } else if(goalColorTheta >= 270&& goalColorTheta <= 300){
    moveTheta(speed, 210, true);
  } else {
    if(line1Check()) {
      moveTheta(speed, 180, true);
    } else if (line3Check()) {
      allCamProcess();
      if (goalColorDistance < opponentColorDistance && goalColorDistance != -1) {
        moveTheta(speed, 180, true);
      } else {
        moveTheta(speed, 0, true);
      }
    } else if (line2Check()) {
      moveTheta(speed, 90, true);
    } else {
      moveTheta(speed, 270, true);
    }
  }
}
*/
