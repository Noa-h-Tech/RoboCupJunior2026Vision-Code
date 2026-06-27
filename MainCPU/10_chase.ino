// ボール追跡のための角度調整関数
int adjustBallAngle(int cam, int x, float theta, float thetamax) {
  if (cam == 2 && x < (camX_max / 2)) {
    return In360(120);
  } else if (cam == 4 && x > (camX_max / 2)) {
    return In360(-120);
  } else if (cam == 1 && abs(In180(theta)) > 30) {
    /*
    if (In180(theta) > 0) {
      return In360(80);
    } else {
      return In360(-80);
    }
    */
    float angleAdjustmentFactor = (1.8 * sq(abs(theta) / thetamax)) + 1; // 最小値は1倍
    return In360(int(In180(int(theta)) * angleAdjustmentFactor));
  } else {
    return In360(int(theta));
  }
}

int adjustBallAngle2(int cam, int x, float theta) {
  if (cam == 2 && x < (camX_max / 2)) {
    return In360(100);
  } else if (cam == 4 && x > (camX_max / 2)) {
    return In360(-100);
  } else if (cam == 1 && abs(In180(theta)) > 14) {
    if (In180(theta) > 0) {
      //＋
      //return In360((In180(theta) * 4.2) - 60);
      return In360(120);
    } else {
      //−
      //return In360((In180(theta) * 4.2) + 60);
      return In360(-120);
    }
  } else {
    return In360(int(In180(theta) * 1));
  }
}

int adjustBallAngle3(int cam, int x, int y, float theta) {
  if (cam != 1) {
    return adjustBallAngle2(cam, x, theta);
  }

  float yFloat = float(y);
  float thetaMaxCam1 = 56.9
                       + (-1.32 * yFloat)
                       + (0.0393 * sq(yFloat))
                       + (-3.8e-4 * pow(yFloat, 3))
                       + (1.4e-6 * pow(yFloat, 4));
  float normalizedX = (((x - 3) - (camX_max / 2.0)) / (camX_max / 2.0));
  float adjustedTheta = thetaMaxCam1 * normalizedX;

  if (abs(In180(theta)) <= 10) {
    return In360(int(theta));
  }

  return In360(int(adjustedTheta));
}

int getCam1IntakeBallTheta(int chaseBallTheta) {
  if (chaseBallTheta == -1 || ballDistance < 0) {
    return chaseBallTheta;
  }

  const float intakeOffsetCm = 9.0f;
  const float thetaRad = float(chaseBallTheta) * (float)M_PI / 180.0f;
  const float ballXWorld = (float(ballDistance) + intakeOffsetCm) * sinf(thetaRad);
  const float ballYWorld = (float(ballDistance) + intakeOffsetCm) * cosf(thetaRad);
  const float intakeTheta = atan2f(ballXWorld, ballYWorld - intakeOffsetCm) * 180.0f / (float)M_PI;
  return In360((int)roundf(intakeTheta));
}

int getLegacyChaseAngle(int optimizedBallTheta) {
  optimizedBallTheta = In360(optimizedBallTheta); // 入力値を正規化

  if (abs(In180(ballTheta)) <= 15) {
    return ballTheta;
  }

  int cal_theta = (optimizedBallTheta + 180) % 360;
  int cal2_theta = In180(cal_theta);
  if (cal_theta > 180) {
    cal2_theta = (cal_theta - 90);
  } else {
    cal2_theta = (cal_theta + 90);
  }
  return In360(cal2_theta + (In180(cal_theta) + 0.0) / 2); // 整数除算を避けるために 0.0 を加算
}


void chase(int speed, int targetTheta, bool maximize) {
  allCamProcess();
  // ボールの実際の角度から追跡用の最適化された角度を計算
  int cam = find(0);
  int optimizedBallTheta;
  int chaseBallTheta = ballTheta;
  // chaseBallTheta = convertToAbsoluteAngle(chaseBallTheta);
  if (cam == 0) { // ボールが見えない場合は、最後に見えた角度を元に追跡（それも見えない場合は0度）
    int fallbackTheta = (chaseBallTheta == -1) ? lastChaseTheta : chaseBallTheta;
    optimizedBallTheta = In360(fallbackTheta);
  } else {
    int x = ballX[cam - 1];
    int y = ballY[cam - 1];
    optimizedBallTheta = adjustBallAngle3(cam, x, y, float(chaseBallTheta));
  }
  debugAdjustBallAngle3 = (cam != 0) ? optimizedBallTheta : -1;
  if (debugMode) {
    updateDirectionNeoPixelDebug();
  } else {
    clearNeoPixelRing();
  }
  lastChaseTheta = optimizedBallTheta;
  rotateAndMoveTheta(speed, getChaseAngle(cam, optimizedBallTheta), In180(targetTheta) * 0.6, true, maximize);
}

int getChaseAngle(int cam, int optimizedBallTheta) {
  if (cam == 1) {
    int intakeBallTheta = getCam1IntakeBallTheta(optimizedBallTheta);
    if (intakeBallTheta != -1) {
      if (abs(In180(optimizedBallTheta)) > 15) {
        return In360((int)roundf(float(In180(intakeBallTheta)) * 1.5f));
      } else {
        return In360((int)roundf(float(In180(intakeBallTheta)) * 1.1f));
      }
    }
  }

  return getLegacyChaseAngle(optimizedBallTheta);
}

int getApproachAngle(int memo) {
    memo = In180(memo);
    if (ballDistance <= 0) {
      return memo;
    }
    float value = 30.0f / ballDistance;
    float x_rad = atanf(value);
    float x_deg = x_rad * (180.0f / M_PI);
    return (int)roundf(memo + copysignf(x_deg, (float)memo));//山本クオリティXD 
}


int convertToAbsoluteAngle(int relativeTheta) {
  return In360(relativeTheta - (goalDirection - currentDirection()));
}
