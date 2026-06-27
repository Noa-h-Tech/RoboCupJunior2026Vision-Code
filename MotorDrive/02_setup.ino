void setup() {
  //シリアル設定
  Serial.begin(9600);
  Serial1.begin(115200);
  //モーター設定
  motorFL.setMaxSpeed(255);  //motorFLの最大スピード設定
  motorFR.setMaxSpeed(255);  //motorFRの最大スピード設定
  motorBL.setMaxSpeed(255);  //motorBLの最大スピード設定
  motorBR.setMaxSpeed(255);  //motorBRの最大スピード設定

  //pinMode(brakePin, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(brakePin), lineBrake, FALLING);
}