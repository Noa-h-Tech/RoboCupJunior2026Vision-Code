bool isValidEcho(long value) {
  return value >= 0;
}

bool isEchoGreater(long left, long right) {
  if (!isValidEcho(left) && !isValidEcho(right)) {
    return false;
  }
  if (!isValidEcho(left)) {
    return false;
  }
  if (!isValidEcho(right)) {
    return true;
  }
  return left > right;
}

void echoPos(int speed) { // 超音波センサーを用いたホームポジション
  turnOffDribbler();
  long front = echoF;
  long back = echoB;
  long left = echoL;
  long right = echoR;

  if (isValidEcho(front) && front < 80) {
    moveTheta(speed, 180, true);
  } else if (isValidEcho(back) && back < 80) {
    moveTheta(speed, 0, true);
  } else if (isValidEcho(left) && left < 50) {
    moveTheta(speed, 90, true);
  } else if (isValidEcho(right) && right < 50) {
    moveTheta(speed, 270, true);
  } else {
    brake(true);
  }
}

void echoPos2(int speed) { // 超音波センサーを用いたホームポジション
  turnOffDribbler();
  int targetTheta = In180(goalDirection - currentDirection());
  long front = echoF;
  long back = echoB;
  long left = echoL;
  long right = echoR;

  bool nearFront = isValidEcho(front) && front < 80;
  bool nearBack = isValidEcho(back) && back < 80;
  bool nearLeft = isValidEcho(left) && left < 50;
  bool nearRight = isValidEcho(right) && right < 50;

  if (nearFront || nearBack || nearLeft || nearRight) {
    if (abs(In180(targetTheta)) >= 25) {
    rotateToTarget(targetTheta, 500UL);
    }
    if (isEchoGreater(front, back)) {
      if (isEchoGreater(left, right)) {
        moveTheta(speed, 315, true);
      } else {
        moveTheta(speed, 45, true);
      }
    } else {
      if (isEchoGreater(left, right)) {
        moveTheta(speed, 225, true);
      } else {
        moveTheta(speed, 135, true);
      }
    }
  } else {
    brake(true);
    //rotate(70);
  }
}
