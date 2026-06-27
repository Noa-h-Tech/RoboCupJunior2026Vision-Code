// コマンド受信処理
void processCommands() {
    while (Serial1.available() > 0) {
        byte data = Serial1.read();

        if (!cmdReceiving) {
            if (data == START_MARKER) {
                cmdReceiving = true;
                cmdEscapeNext = false;
                cmdIndex = 0;
            }
            continue;
        }

        if (cmdEscapeNext) {
            data ^= 0x20;
            cmdEscapeNext = false;
        } else if (data == ESCAPE_MARKER) {
            cmdEscapeNext = true;
            continue;
        } else if (data == START_MARKER) {
            // 受信途中に開始マーカーが来たら同期し直す
            cmdEscapeNext = false;
            cmdIndex = 0;
            continue;
        } else if (data == END_MARKER) {
            executeCommand();
            cmdReceiving = false;
            cmdEscapeNext = false;
            cmdIndex = 0;
            continue;
        }

        if (cmdIndex < CMD_BUFFER_SIZE) {
            cmdBuffer[cmdIndex++] = data;
        } else {
            // バッファオーバー時はフレーム破棄
            cmdReceiving = false;
            cmdEscapeNext = false;
            cmdIndex = 0;
        }
    }
}

// チェックサム検証
bool verifyChecksum() {
    if (cmdIndex < 3) return false; // 最低限 CMD_ID + LENGTH + CHECKSUM

    byte length = cmdBuffer[1];
    if (cmdIndex != 2 + length + 1) return false; // ID + LENGTH + PAYLOAD + CHECKSUM

    byte checksum = 0;
    for (int i = 0; i < 2 + length; i++) {
        checksum ^= cmdBuffer[i];
    }

    return checksum == cmdBuffer[2 + length];
}

// コマンド実行
void executeCommand() {
    if (cmdIndex < 2) return; // 最低限 CMD_ID + LENGTH

    byte cmdId = cmdBuffer[0];
    byte length = cmdBuffer[1];

    // チェックサム検証
    if (!verifyChecksum()) {
        return; // チェックサムエラー時は無視
    }

    switch (cmdId) {
        case CMD_SET_ALL_THRESHOLDS:
            if (length != 8) return;
            handleSetAllThresholds();
            break;
        case CMD_REQUEST_DEBUG:
            if (length != 0) return;
            currentMode = MODE_DEBUG;
            sendAck(cmdId);
            break;
        case CMD_REQUEST_NORMAL:
            if (length != 0) return;
            currentMode = MODE_NORMAL;
            sendAck(cmdId);
            break;
        default:
            break; // 未知のコマンドは無視
    }
}

// 全閾値一括設定ハンドラ
void handleSetAllThresholds() {
    // ペイロード: 4閾値 × 2バイト = 8バイト
    for (int i = 0; i < 4; i++) {
        int value = (cmdBuffer[2 + i * 2] << 8) | cmdBuffer[3 + i * 2];
        receivedThresholds[i] = constrain(value, 0, 1023);
    }
    sendAck(CMD_SET_ALL_THRESHOLDS);
}

// ACK送信
void sendAck(byte cmdId) {
    byte checksum = RESP_ACK ^ 1 ^ cmdId;
    Serial1.write(START_MARKER);
    writeEscapedByte(RESP_ACK);
    writeEscapedByte((byte)1); // length
    writeEscapedByte(cmdId);
    writeEscapedByte(checksum);
    Serial1.write(END_MARKER);
}
