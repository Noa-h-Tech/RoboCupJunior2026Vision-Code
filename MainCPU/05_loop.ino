//メインループ
void loop() {
  if (!toggle3) {
    debugOLED();
  } else {
    readyForPlay();
    while (toggle3) {
      if (!toggle1) {  // 一時停止中
        started = false;  // スタートフラグをリセット
        // スイッチ2でモード切り替え
        if (switch2) {
          isDribblerMode = !isDribblerMode;
          analogWrite(buzzer, 128);
          delay(200);  // チャタリング防止
          analogWrite(buzzer, 0);
        }
        // スイッチ3でデバッグモード切り替え
        if (switch3) {
          debugMode = !debugMode;
          analogWrite(buzzer, 128);
          delay(200);  // チャタリング防止
          analogWrite(buzzer, 0);
        }
        // スイッチ4でゴール方向を再セット（toggle2で青/黄を選択してから押す）
        if (switch4) {
          setForwardDirection();
        }

        updateDebugModeNeoPixelIndicator();
        // OLED表示の更新
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.println("Paused");
        display.print("Mode: ");
        display.println(isDribblerMode ? "Dribbler" : "Kicker");
        display.print("Debug: ");
        display.println(debugMode ? "ON" : "OFF");
        display.print("Dir: ");
        display.println(forwardDirectionSet ? "SET" : "NOT SET");
        display.display();
        
        turnOffDribbler();
        clearNeoPixelRing();
        Serial6Reset();
        brake(true);
        delay(0);
      } else {
        if (isDribblerMode) {
          if (!started) {
            /*
            long time = millis();
            while (millis() - time < 700) {
              moveTheta(80, 0, true);
              lineCheck(true); // ラインセンサーチェック
            }
            */
            started = true;
            playResumedAt = millis();
          }
          dribblerPlay();
        } else {
          if (!started) {
            started = true;
            playResumedAt = millis();
            kickerUseCompassForGoal = true;
          }
          play();
          /*
          allCamProcess();
          int targetTheta = In180(goalDirection - currentDirection());
          while(abs(In180(targetTheta)) >= 10) {
            turnTheta(targetTheta);
            targetTheta = In180(goalDirection - currentDirection());
          }
          moveTheta(80, 180, true);
          lineCheck(true); // ラインセンサーチェック
          */
        }
      }
    }
    turnOffDribbler();
    clearNeoPixelRing();
    brake(true);
  }
}

//試合
void play() {
  if (!toggle1) {
    brake(true);
    delay(0);
    return;
  }

  commonPlaySetup();
  if (debugMode) {
    updateDirectionNeoPixelDebug();
  } else {
    clearNeoPixelRing();
  }
  lineCheck(true);

  // 開始/一時停止解除後1000ms間の特別処理
  if (millis() - playResumedAt < 1000) {
    int compassDiff = In180(goalDirection - currentDirection());
    if (abs(compassDiff) < 90) {
      if (ballTheta != -1 && (ballTheta < 90 || ballTheta > 270)) {
        chase(80, getBallChasingDirection(), true);
      } else {
        moveTheta(80, 0, true);
      }
      if (ballSensor < kickerThreshold) {
        kick();
      }
      lineCheck(true);
      return;
    }
  }

  if (ballTheta != -1) {
    if ((ballDistance > 30 && !lastChaseStrategy) || (ballDistance > 40 && lastChaseStrategy)) {
      lastChaseStrategy = false; // ボールが遠い場合の処理
      handleLongDistanceBall(); // ボールが遠い場合の処理
    } else {
      lastChaseStrategy = true;
      handleShortDistanceBall(); // ボールが近い場合の処理
    }
  } else {
    handleNoBallVisible(); // ボールが見えない場合の処理
  }

  // キッカーの制御
  if (ballSensor < kickerThreshold) {
    kick();
  }
  lineCheck(true);
}

void dribblerPlay() {
  if (!toggle1) {
    brake(true);
    return;
  }

  commonPlaySetup();
  if (debugMode) {
    updateDirectionNeoPixelDebug();
  } else {
    clearNeoPixelRing();
  }
  lineCheck(true);

  if (ballTheta != -1) {
    if ((ballDistance > 30 && !lastChaseStrategy) || (ballDistance > 40 && lastChaseStrategy)) {
      lastChaseStrategy = false;
      handleLongDistanceBall();  // playと同じ
    } else {
      lastChaseStrategy = true;
      handleShortDistanceBallWithDribbler();  // ドリブラー専用処理
    }
  } else {
    handleNoBallVisible();
    //brake(true); // ボールが見えない場合は停止
  }
  lineCheck(true);
}

void kick() {
  if (ballSensor < kickerThreshold) {
    allCamProcess();

    // ゴール横エリア対応: convertToAbsoluteAngleを適用した絶対角度が±45°外の場合
    // ドリブラーで保持しながら斜め後退してゴールを正面に向ける
    {
        int goalAbsAngle;
        int goalRelAngle;
        if (goalColorTheta != -1) {
            goalAbsAngle = In180(convertToAbsoluteAngle(goalColorTheta));
            goalRelAngle = In180(goalColorTheta);
        } else {
            goalRelAngle = In180(goalDirection - currentDirection());
            goalAbsAngle = goalRelAngle;
        }

        if (abs(goalAbsAngle) > 45) {
            unsigned long lateralStartTime = millis();
            const unsigned long lateralTimeout = 3000UL;

            while (abs(goalAbsAngle) > 40 && ballSensor < kickerThreshold) {
                allCamProcess();
                turnOnDribbler();

                if (goalColorTheta != -1) {
                    goalAbsAngle = In180(convertToAbsoluteAngle(goalColorTheta));
                    goalRelAngle = In180(goalColorTheta);
                } else {
                    goalRelAngle = In180(goalDirection - currentDirection());
                    goalAbsAngle = goalRelAngle;
                }

                // 絶対角度の符号でどちら側か判定: 正→右斜め後ろ(135°)、負→左斜め後ろ(225°)
                int moveTheta_dir = (goalAbsAngle > 0) ? 135 : 225;

                // 斜め後退しながらゴール方向に回転
                rotateAndMoveTheta(25, moveTheta_dir, goalRelAngle, true, false);

                lineCheck(true);

                if (!toggle1) { brake(true); return; }
                if (millis() - lateralStartTime >= lateralTimeout) break;
            }
            brake(true);
        }
    }

    int targetTheta = In180(getBallChasingDirection());
    unsigned long aimStartTime = millis();
    const unsigned long aimTimeout = 1500UL;
    while (abs(In180(targetTheta)) >= 10 && ballSensor < kickerThreshold) {
        turnOnDribbler();
        allCamProcess();
        targetTheta = In180(getBallChasingDirection());
        if (abs(In180(targetTheta)) > 30) {
          turnTheta(targetTheta);
        } else {
          int moveDirection = (goalColorTheta != -1)
              ? goalColorTheta
              : In360(goalDirection - currentDirection());
          rotateAndMoveTheta(60, moveDirection, targetTheta, true, false);
        }
        lineCheck(true);
        if (lineFlag != 0 || !toggle1 || millis() - aimStartTime >= aimTimeout) {
          turnOffDribbler();
          brake(true);
          return;
        }
    }
    turnOffDribbler();
    brake(true);
    if (ballSensor >= kickerThreshold || !toggle1) {
      return;
    }
    digitalWrite(kicker, HIGH);
    long time = millis();
    while (millis() - time < 30) {
      // 30ms間キッカーをONにする
      delay(0); // 30ms待機
      lineCheck(true); // ラインセンサーチェック
      chase(80, getBallChasingDirection(), true);
    }
    digitalWrite(kicker, LOW);
    time = millis();
    while (millis() - time < 200) {
      // 200ms間キッカーをOFFにする
      delay(0); // 200ms待機
      lineCheck(true); // ラインセンサーチェック
    } 
  }
}
