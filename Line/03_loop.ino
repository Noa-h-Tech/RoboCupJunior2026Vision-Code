void writeEscapedByte(byte data) {
    if (data == START_MARKER || data == END_MARKER || data == ESCAPE_MARKER) {
        Serial1.write(ESCAPE_MARKER);
        Serial1.write(data ^ 0x20);
    } else {
        Serial1.write(data);
    }
}


void sendDetectedFrame(byte detectedCount, const byte detectedSensors[16], const int detectedValues[16]) {
    // ペイロード: count(1) + [sensorId(1) + value(2)] × count
    byte length = 1 + detectedCount * 3;
    byte checksum = RESP_LINE_DETECTED ^ length ^ detectedCount;

    Serial1.write(START_MARKER);
    writeEscapedByte(RESP_LINE_DETECTED);
    writeEscapedByte(length);
    writeEscapedByte(detectedCount);

    for (int i = 0; i < detectedCount; i++) {
        writeEscapedByte(detectedSensors[i]);
        byte upper = (detectedValues[i] >> 8) & 0xFF;
        byte lower = detectedValues[i] & 0xFF;
        writeEscapedByte(upper);
        writeEscapedByte(lower);
        checksum ^= detectedSensors[i];
        checksum ^= upper;
        checksum ^= lower;
    }

    writeEscapedByte(checksum);
    Serial1.write(END_MARKER);
}

void loop() {
    // コマンド受信処理
    processCommands();

    // センサー値の読み取り
    for (int i = 0; i < 16; i++) {
        my_mux.channel(i);
        // アナログ値を0-1023の範囲に制限
        int value = constrain(analogRead(g_common_pin), 0, 1023);
        lineValues[i] = value;
    }

    //sendUsbSensorMonitor();

    // モードに応じたデータ送信
    if (currentMode == MODE_DEBUG) {
        sendDebugData();  // 全センサーデータ送信
    } else {
        sendDetectedLines();  // 反応センサーのみ送信
    }
}

void sendUsbSensorMonitor() {
    // if (!Serial) return;

    unsigned long now = millis();
    if ((now - lastUsbMonitorPrintTime) < USB_MONITOR_INTERVAL_MS) return;
    lastUsbMonitorPrintTime = now;

    for (int i = 0; i < 16; i++) {
        Serial.print(lineValues[i]);
        if (i < 15) {
            Serial.print(',');
        }
    }
    Serial.println();
}

// 検出ラインのみ送信
void sendDetectedLines() {
    // 検出センサーをカウント
    byte detectedCount = 0;
    byte detectedSensors[16];
    int detectedValues[16];

    for (int i = 0; i < 16; i++) {
        int blockIndex = sensorBlockMap[i] - 1; // 0-indexed
        if (lineValues[i] > receivedThresholds[blockIndex]) {
            detectedSensors[detectedCount] = i;
            detectedValues[detectedCount] = lineValues[i];
            detectedCount++;
        }
    }

    unsigned long now = millis();
    if (detectedCount == 0) {
        if (lastSentDetectedCount == 0 && (now - lastZeroReportTime) < ZERO_REPORT_INTERVAL_MS) {
            return;
        }
        lastZeroReportTime = now;
    } else {
        // 検出中は最大100Hzで送り、ライン回避開始までの遅延を抑える
        if ((now - lastZeroReportTime) < 10) {
            return;
        }
        lastZeroReportTime = now;
    }

    sendDetectedFrame(detectedCount, detectedSensors, detectedValues);
    lastSentDetectedCount = detectedCount;
}

// デバッグ用全データ送信
void sendDebugData() {
    byte length = 32; // 16センサー × 2バイト
    byte checksum = RESP_DEBUG_DATA ^ length;

    Serial1.write(START_MARKER);
    writeEscapedByte(RESP_DEBUG_DATA);
    writeEscapedByte(length);

    for (int i = 0; i < 16; i++) {
        byte upper = (lineValues[i] >> 8) & 0xFF;
        byte lower = lineValues[i] & 0xFF;
        writeEscapedByte(upper);
        writeEscapedByte(lower);
        checksum ^= upper;
        checksum ^= lower;
    }

    writeEscapedByte(checksum);
    Serial1.write(END_MARKER);
}
