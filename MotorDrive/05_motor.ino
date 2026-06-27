void move(int speed1, int speed2, int speed3, int speed4) {  //モーター制御
  // botSpeedを適用
  speed1 = (int)(speed1 * botSpeed * 0.01);
  speed2 = (int)(speed2 * botSpeed * 0.01);
  speed3 = (int)(speed3 * botSpeed * 0.01);
  speed4 = (int)(speed4 * botSpeed * 0.01);

  switch (motorFlag) {
    case 0:
      motorFL.setSpeed(speed1);
      motorFR.setSpeed(speed2);
      motorBL.setSpeed(speed3);
      motorBR.setSpeed(speed4);
      break;
    case 1:
      motorFL.brake(BRAKE);
      motorFR.brake(BRAKE);
      motorBL.brake(BRAKE);
      motorBR.brake(BRAKE);
      break;
    case 2:
      motorFL.brake(COAST);
      motorFR.brake(COAST);
      motorBL.brake(COAST);
      motorBR.brake(COAST);
      break;
    case 3:
      motorFL.brake(BRAKE);
      motorFR.brake(BRAKE);
      motorBL.brake(BRAKE);
      motorBR.brake(BRAKE);
      break;
    case 4:
      motorFL.setSpeed(speed1);
      motorFR.setSpeed(speed2);
      motorBL.setSpeed(speed3);
      motorBR.setSpeed(speed4);
      // デバッグ用シリアル出力は削除
      break;
  }
}

void lineBrake() {
  motorFL.brake(BRAKE);
  motorFR.brake(BRAKE);
  motorBL.brake(BRAKE);
  motorBR.brake(BRAKE);
  motorFlag = 1;
}
