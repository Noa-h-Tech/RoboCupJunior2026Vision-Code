static bool isNeoPixelEnabled() {
  return neopixel_pin >= 0;
}

static int thetaToNeoPixelIndex(int theta) {
  int normalized = In360(theta);
  const int step = 360 / neopixel_count;
  const int halfStep = step / 2;
  return ((normalized + halfStep) / step) % neopixel_count;
}

// fromThetaからtoThetaまで時計回りにLEDを点灯（ゴール幅の可視化用）
void setArcMarker(
  uint8_t *rBuf,
  uint8_t *gBuf,
  uint8_t *bBuf,
  int fromTheta,
  int toTheta,
  uint8_t r,
  uint8_t g,
  uint8_t b
) {
  if (fromTheta == -1 || toTheta == -1) return;
  int fromIdx = thetaToNeoPixelIndex(fromTheta);
  int toIdx   = thetaToNeoPixelIndex(toTheta);
  for (int i = fromIdx; ; i = (i + 1) % neopixel_count) {
    rBuf[i] = max(rBuf[i], r);
    gBuf[i] = max(gBuf[i], g);
    bBuf[i] = max(bBuf[i], b);
    if (i == toIdx) break;
    if (((i + 1) % neopixel_count) == fromIdx) break;  // 安全装置（無限ループ防止）
  }
}

static void setDirectionMarker(
  uint8_t *rBuf,
  uint8_t *gBuf,
  uint8_t *bBuf,
  int theta,
  uint8_t r,
  uint8_t g,
  uint8_t b
) {
  if (theta == -1) {
    return;
  }

  int idx = thetaToNeoPixelIndex(theta);
  rBuf[idx] = max(rBuf[idx], r);
  gBuf[idx] = max(gBuf[idx], g);
  bBuf[idx] = max(bBuf[idx], b);
}

void initNeoPixelRing() {
  if (!isNeoPixelEnabled()) {
    return;
  }
  neoRing.begin();
  neoRing.setBrightness(40);
  neoRing.clear();
  neoRing.show();
}

void clearNeoPixelRing() {
  if (!isNeoPixelEnabled()) {
    return;
  }
  neoRing.clear();
  neoRing.show();
}

void updateDebugModeNeoPixelIndicator() {
  if (!isNeoPixelEnabled()) {
    return;
  }

  if (!debugMode) {
    clearNeoPixelRing();
    return;
  }

  for (int i = 0; i < neopixel_count; i++) {
    neoRing.setPixelColor(i, neoRing.Color(32, 32, 32));
  }
  neoRing.show();
}

void updateDirectionNeoPixelDebug() {
  if (!isNeoPixelEnabled()) {
    return;
  }
  if (!debugMode) {
    clearNeoPixelRing();
    return;
  }
  uint8_t rBuf[neopixel_count] = {0};
  uint8_t gBuf[neopixel_count] = {0};
  uint8_t bBuf[neopixel_count] = {0};

  // adjustBallAngle3 output: white dot
  setDirectionMarker(rBuf, gBuf, bBuf, debugAdjustBallAngle3, 255, 255, 255);
  // Ball: red dot
  setDirectionMarker(rBuf, gBuf, bBuf, ballTheta, 255, 0, 0);
  // Blue goal: 薄い青でアーク → 中心を明るい青で上書き
  setArcMarker(rBuf, gBuf, bBuf, blueLeftEdge, blueRightEdge, 0, 0, 60);
  setDirectionMarker(rBuf, gBuf, bBuf, blueTheta, 0, 0, 255);
  // Yellow goal: 薄い黄でアーク → 中心を明るい緑で上書き
  setArcMarker(rBuf, gBuf, bBuf, yellowLeftEdge, yellowRightEdge, 60, 60, 0);
  setDirectionMarker(rBuf, gBuf, bBuf, yellowTheta, 0, 255, 0);

  for (int i = 0; i < neopixel_count; i++) {
    neoRing.setPixelColor(i, neoRing.Color(rBuf[i], gBuf[i], bBuf[i]));
  }
  neoRing.show();
}

void updateChaseThetaNeoPixel(int theta) {
  if (!isNeoPixelEnabled()) {
    return;
  }
  if (!debugMode) {
    clearNeoPixelRing();
    return;
  }

  neoRing.clear();
  if (theta != -1) {
    int idx = thetaToNeoPixelIndex(theta);
    neoRing.setPixelColor(idx, neoRing.Color(255, 255, 0));
  }
  neoRing.show();
}
