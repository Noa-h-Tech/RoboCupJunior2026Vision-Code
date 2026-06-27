void move(int speed1, int speed2, int speed3, int speed4, float maximize) {  //通常のモーター制御
  
  // 速度を-100から100の範囲に制限
  speed1 = constrain(speed1, -100, 100);
  speed2 = constrain(speed2, -100, 100);
  speed3 = constrain(speed3, -100, 100);
  speed4 = constrain(speed4, -100, 100);
  
  int actualSpeed1 = speed1;
  int actualSpeed2 = speed2;
  int actualSpeed3 = speed3;
  int actualSpeed4 = speed4;
  int maxSpeed;
  float speedRatio;

  if(maximize > 0) { // モーターの出力最大値を超えない範囲で比を保ちながら出力を最大化
    maxSpeed = max(max(max(abs(actualSpeed1), abs(actualSpeed2)), max(abs(actualSpeed3), abs(actualSpeed4))), 1);
    if (maxSpeed > 0) {
      float targetMax = 100.0f * maximize;
      speedRatio = targetMax / abs(maxSpeed);
      if (abs(maxSpeed) < targetMax) {
        actualSpeed1 = actualSpeed1 * speedRatio;
        actualSpeed2 = actualSpeed2 * speedRatio;
        actualSpeed3 = actualSpeed3 * speedRatio;
        actualSpeed4 = actualSpeed4 * speedRatio;
      }
    }
  }

  // botSpeedを適用した実際の速度を計算
  actualSpeed1 = actualSpeed1 * botSpeed * 0.01;
  actualSpeed2 = actualSpeed2 * botSpeed * 0.01;
  actualSpeed3 = actualSpeed3 * botSpeed * 0.01;
  actualSpeed4 = actualSpeed4 * botSpeed * 0.01;
  
  // 最終的な速度を0-200の範囲に変換（100を中心とする）
  byte finalSpeed1 = constrain((actualSpeed1 + 100), 0, 200);
  byte finalSpeed2 = constrain((actualSpeed2 + 100), 0, 200);
  byte finalSpeed3 = constrain((actualSpeed3 + 100), 0, 200);
  byte finalSpeed4 = constrain((actualSpeed4 + 100), 0, 200);
  
  Serial2.write(255);
  Serial2.write(finalSpeed1);
  Serial2.write(finalSpeed2);
  Serial2.write(finalSpeed3);
  Serial2.write(finalSpeed4);
  
  if (debugMode) {
    Serial.println(finalSpeed1);
    Serial.println(finalSpeed2);
    Serial.println(finalSpeed3);
    Serial.println(finalSpeed4);
  }
}

void forced_move(int speed1, int speed2, int speed3, int speed4, float maximize) {  //強制モーター制御
  /* 強制的なモーター制御（通常の制限を無視）
    
    使用例：
    - 緊急停止時
    - デバッグ時の動作テスト
    - 特殊な動作が必要な場合
    
    注意：
    - 機械的な制限は考慮していないので注意
    - 必要な場合のみ使用すること
    */
  
  // 速度を-100から100の範囲に制限
  speed1 = constrain(speed1, -100, 100);
  speed2 = constrain(speed2, -100, 100);
  speed3 = constrain(speed3, -100, 100);
  speed4 = constrain(speed4, -100, 100);
  
    int actualSpeed1 = speed1;
  int actualSpeed2 = speed2;
  int actualSpeed3 = speed3;
  int actualSpeed4 = speed4;
  float maxSpeed = 0; // 初期化
  float speedRatio = 1.0; // 初期化
  //debug
  // int tmp_acturalSpeed1; // 未使用のためコメントアウト
  // int tmp_acturalSpeed2;
  // int tmp_acturalSpeed3;
  // int tmp_acturalSpeed4;

  
  if(maximize > 0) { // モーターの出力最大値を超えない範囲で比を保ちながら出力を最大化
    // tmp_acturalSpeed1 = actualSpeed1; // 未使用のためコメントアウト
    // tmp_acturalSpeed2 = actualSpeed2;
    // tmp_acturalSpeed3 = actualSpeed3;
    maxSpeed = max(max(max(abs(actualSpeed1), abs(actualSpeed2)), max(abs(actualSpeed3), abs(actualSpeed4))), 1);
    if (maxSpeed > 0) {
      float targetMax = 100.0f * maximize;
      speedRatio = targetMax / abs(maxSpeed);
      if (abs(maxSpeed) < targetMax) {
        actualSpeed1 = actualSpeed1 * speedRatio;
        actualSpeed2 = actualSpeed2 * speedRatio;
        actualSpeed3 = actualSpeed3 * speedRatio;
        actualSpeed4 = actualSpeed4 * speedRatio;
      }
    }
  }
    // botSpeedを適用した実際の速度を計算
  actualSpeed1 = actualSpeed1 * botSpeed * 0.01;
  actualSpeed2 = actualSpeed2 * botSpeed * 0.01;
  actualSpeed3 = actualSpeed3 * botSpeed * 0.01;
  actualSpeed4 = actualSpeed4 * botSpeed * 0.01;
  
  // 最終的な速度を0-200の範囲に変換（100を中心とする）
  byte finalSpeed1 = constrain((actualSpeed1 + 100), 0, 200);
  byte finalSpeed2 = constrain((actualSpeed2 + 100), 0, 200);
  byte finalSpeed3 = constrain((actualSpeed3 + 100), 0, 200);
  byte finalSpeed4 = constrain((actualSpeed4 + 100), 0, 200);
  
  Serial2.write(252);
  Serial2.write(finalSpeed1);
  Serial2.write(finalSpeed2);
  Serial2.write(finalSpeed3);
  Serial2.write(finalSpeed4);

  if (debugMode) {
    int f1 = finalSpeed1 - 100;
    int f2 = finalSpeed2 - 100;
    int f3 = finalSpeed3 - 100;
    int f4 = finalSpeed4 - 100;

    
    Serial.print("1 : ");
    Serial.print(f1);
    Serial.print("  2 : ");
    Serial.print(f2);
    Serial.print("  3 : ");
    Serial.print(f3);
    Serial.print("  4 : ");
    Serial.print(f4);
    Serial.print("  speedRatio : ");
    Serial.print(speedRatio);
    Serial.print("  maxSpeed : ");
    Serial.println(maxSpeed);
  }
}

void rotate(int speed) {  //正の値を代入した場合、左回転
  if (speed > 0) {
    speed = constrain(speed, minRotatePowerLeft, 100);
  } else if (speed < 0) {
    speed = constrain(speed, -100, -minRotatePowerRight);
  } else {
    brake(true);
    return;
  }
  forced_move(speed * -1, speed, speed * -1, speed, false);
}

void brake(bool type) {  //trueでロック、falseで解放
  if (type) {
    Serial2.write(254);
    Serial2.write(100);
    Serial2.write(100);
    Serial2.write(100);
    Serial2.write(100);
  } else {
    Serial2.write(253);
    Serial2.write(100);
    Serial2.write(100);
    Serial2.write(100);
    Serial2.write(100);
  }
}

void moveTheta(int speed, int targetTheta, bool forced, float maximize) {  //4軸オムニホイールを用いて指定した方向にロボットを移動
  /* 全方向移動の制御関数
    引数：
    speed: 移動速度（0-100）
    targetTheta: 移動方向（度、0-360）
        0: 前進
        90: 右
        180: 後退
        270: 左
    forced: true=強制動作, false=通常動作
    maximize: true=出力を最大化, false=最大化しない
    
    動作原理：
    1. 移動方向を三角関数で4つのモーターの出力に分解
    2. 斜め移動時は補正値を加えて滑らかに動作
    3. 各モーターの出力をシリアルでモーター制御基板に送信
    */
  while (targetTheta > 360 || targetTheta < 0) {
    if (targetTheta > 360) {
      targetTheta -= 360;
    } else {
      targetTheta += 360;
    }
  }
  float tiltPower;  //斜め移動時のパワー補正
  tiltPower = targetTheta % 90;
  tiltPower = abs(tiltPower - 45);
  speed = speed + (0.25 * (45 - tiltPower));

  int a = speed * (cos((targetTheta) * (PI / 180)) + sin((targetTheta) * (PI / 180)));
  int b = speed * (cos((targetTheta) * (PI / 180)) - sin((targetTheta) * (PI / 180)));
  if (forced) {
    forced_move(a, b, b, a, maximize);
  } else {
    move(a, b, b, a, maximize);
  }
  lastMotorTheta = targetTheta;
}

void rotateAndMoveTheta(int speed, int targetMoveTheta, int targetRotateTheta, bool forced, float maximize) {
  // 移動角度を0-360の範囲に正規化
  targetMoveTheta = In360(int(targetMoveTheta));

  // 斜め移動の補正
  float tiltPower = targetMoveTheta % 90;
  tiltPower = abs(tiltPower - 45);
  int adjustedSpeed = speed + (0.25 * (45 - tiltPower));

  // 移動成分の計算
  float moveA = adjustedSpeed * (cos((targetMoveTheta) * (PI / 180)) + sin((targetMoveTheta) * (PI / 180))) / 2;
  float moveB = adjustedSpeed * (cos((targetMoveTheta) * (PI / 180)) - sin((targetMoveTheta) * (PI / 180))) / 2;

  // 回転成分の計算（P制御）
  float rotateSpeed = targetRotateTheta * -0.5;  // 回転のゲイン調整
  
  // 移動と回転の合成
  int finalSpeed1 = moveA - rotateSpeed;
  int finalSpeed2 = moveB + rotateSpeed;
  int finalSpeed3 = moveB - rotateSpeed;
  int finalSpeed4 = moveA + rotateSpeed;
  
  // モーター制御の実行
  if (forced) {
    forced_move(finalSpeed1, finalSpeed2, finalSpeed3, finalSpeed4, maximize);
  } else {
    move(finalSpeed1, finalSpeed2, finalSpeed3, finalSpeed4, maximize);
  }

  lastMotorTheta = targetMoveTheta;
}

void turnTheta(int targetTheta, float gain) {  //目標角度にP制御で回転する
  float power = targetTheta * gain;
  if(abs(power) < minPower){
    if(power > 0){
      power = minPower;
    }else{
      power = -minPower;
    }
  }
  rotate(int(power));
}

void rawRotate(int speed) {  // botSpeedの影響を受けない直接回転制御（正=左回転、負=右回転）
  speed = constrain(speed, -100, 100);
  byte s1 = constrain((-speed + 100), 0, 200);
  byte s2 = constrain((speed + 100), 0, 200);
  Serial2.write(252);
  Serial2.write(s1);
  Serial2.write(s2);
  Serial2.write(s1);
  Serial2.write(s2);
}

int findMinRotatePower(bool leftRotation) {  // 最低回転パワーを測定（戻り値: パワー値1-100、失敗時-1）
  for (int power = 1; power <= 100; power++) {
    rawRotate(leftRotation ? power : -power);

    unsigned long startTime = millis();
    int initialDir = currentDirection();

    while (millis() - startTime < 500) {
      if (abs(In180(currentDirection() - initialDir)) >= 3) {
        brake(true);
        delay(300);
        return power;
      }
      if (shouldExit()) { brake(true); return -1; }
    }
    brake(true);
    delay(200);
  }
  return -1;
}

void minPowerRotate(bool leftRotation) {  // 最低パワーで回転、パワー不足時は自動で上げる
  int power = leftRotation ? minRotatePowerLeft : minRotatePowerRight;
  int initialDir = currentDirection();
  rawRotate(leftRotation ? power : -power);

  unsigned long checkTime = millis();
  while (millis() - checkTime < 300) {
    if (abs(In180(currentDirection() - initialDir)) >= 2) return;
  }

  for (int p = power + 1; p <= 100; p++) {
    rawRotate(leftRotation ? p : -p);
    initialDir = currentDirection();
    checkTime = millis();
    while (millis() - checkTime < 200) {
      if (abs(In180(currentDirection() - initialDir)) >= 2) {
        if (leftRotation) minRotatePowerLeft = p;
        else minRotatePowerRight = p;
        return;
      }
    }
  }
}

void turnOnDribbler() {
  analogWrite(dribbler_pin, dribblerSpeed);
  isDribblerOn = true;
}

void turnOffDribbler() {
  analogWrite(dribbler_pin, 0);
  isDribblerOn = false;
}
