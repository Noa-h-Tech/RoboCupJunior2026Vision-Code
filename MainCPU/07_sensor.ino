// 関数のプロトタイプ宣言を追加
bool isCalibrated();
bool waitForCalibration(unsigned long timeout);

int currentDirection() {  //BNO055で取得されたロボットの向いている方向
  // BNO055 VECTOR_EULER: x = yaw (IMUPLUS mode: ジャイロ基準の相対方位, CW正, 0~360度)
  // gyroOffset を引くことで sw4ボタンでセットした「前方」を 0° とする相対角度になる
  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  return In360((int)euler.x() - gyroOffset);
}

int In360(int theta) {  //角度を0~360の範囲で表現
  return (theta + 36000) % 360;
}

int In180(int theta) {  //角度を-180~180の範囲で表現
  if (In360(theta) > 180) {
    return In360(theta) - 360;
  } else {
    return In360(theta);
  }
}

long getEcho(int trig, int echo) {  //指したピン番号の超音波センサの値を取得
  // ピン番号が-1の場合は未使用センサーとして読み取りをスキップ
  if (trig < 0 || echo < 0) {
    return -1;
  }
  turnOffDribbler();
  long duration, cm;
  pinMode(trig, OUTPUT);
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(5);
  digitalWrite(trig, LOW);
  pinMode(echo, INPUT);
  duration = pulseIn(echo, HIGH, 25000UL);  // 25msタイムアウト（デフォルト1秒ブロック防止）
  cm = microsecondsToCentimeters(duration);
  // delay(20);
  return cm;
}

long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2;
}

bool isCalibrated() {
    uint8_t system, gyro, accel, mag;
    bno.getCalibration(&system, &gyro, &accel, &mag);
    return (gyro >= 1);
}

bool waitForCalibration(unsigned long timeout = 0) {
    unsigned long startTime = millis();
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    
    while (!isCalibrated()) {
        uint8_t system, gyro, accel, mag;
        bno.getCalibration(&system, &gyro, &accel, &mag);

        display.clearDisplay();
        display.setCursor(0,0);
        display.println("Calibrating...");
        display.print("Accel:");
        display.println(accel);
        display.print("Gyro:");
        display.println(gyro);
        display.display();
        
        if (timeout > 0 && (millis() - startTime) > timeout) {
            return false;
        }
        
        delay(100);
    }
    return true;
}

bool loadBnoCalibrationOffsets() {
    if (!loggingMode || !SD.exists(bnoCalibrationFile)) {
        return false;
    }

    FsFile calibrationFile = SD.open(bnoCalibrationFile, FILE_READ);
    if (!calibrationFile) {
        return false;
    }

    int16_t values[11];
    char buffer[32];
    int valueCount = 0;

    while (calibrationFile.available() && valueCount < 11) {
        if (!calibrationFile.fgets(buffer, sizeof(buffer))) {
            continue;
        }

        char* newline = strchr(buffer, '\n');
        if (newline) *newline = 0;
        values[valueCount++] = (int16_t)atoi(buffer);
    }
    calibrationFile.close();

    if (valueCount != 11) {
        return false;
    }

    adafruit_bno055_offsets_t offsets = {
        values[0], values[1], values[2],
        values[3], values[4], values[5],
        values[6], values[7], values[8],
        values[9], values[10]
    };

    bno.setSensorOffsets(offsets);
    delay(10);
    return true;
}

bool saveBnoCalibrationOffsets() {
    if (!loggingMode) {
        return false;
    }

    adafruit_bno055_offsets_t offsets;
    if (!bno.getSensorOffsets(offsets)) {
        return false;
    }

    FsFile calibrationFile = SD.open(bnoCalibrationFile, O_RDWR | O_CREAT | O_TRUNC);
    if (!calibrationFile) {
        return false;
    }

    calibrationFile.println(offsets.accel_offset_x);
    calibrationFile.println(offsets.accel_offset_y);
    calibrationFile.println(offsets.accel_offset_z);
    calibrationFile.println(offsets.mag_offset_x);
    calibrationFile.println(offsets.mag_offset_y);
    calibrationFile.println(offsets.mag_offset_z);
    calibrationFile.println(offsets.gyro_offset_x);
    calibrationFile.println(offsets.gyro_offset_y);
    calibrationFile.println(offsets.gyro_offset_z);
    calibrationFile.println(offsets.accel_radius);
    calibrationFile.println(offsets.mag_radius);
    calibrationFile.close();

    return true;
}

void Serial6Reset() {
    while (Serial6.available() > 0) {
        Serial6.read();
    }
}
bool isHoldingBall() {
  static unsigned long lastDetectedMs = 0;
  static bool hasDetectedOnce = false;
  const unsigned long holdGraceMs = 300UL;

  bool detectedNow = (ballSensor < kickerThreshold);
  unsigned long now = millis();

  if (detectedNow) {
    lastDetectedMs = now;
    hasDetectedOnce = true;
    return true;
  }

  if (!hasDetectedOnce) {
    return false;
  }

  return (now - lastDetectedMs) <= holdGraceMs;
}
