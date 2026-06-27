// 座標から距離を検索するための分割数
const int NUM_DISTANCE_FILES = 10;

// 多項式回帰の係数とパラメータをSDカードから読み込む
void loadWeights() {
  if (!loggingMode) {
       Serial.println("Error: Cannot load weights (logging mode is disabled)");
    return;
  }

  // SDカードの状態確認
  if (!SD.card()->errorCode()) {
       Serial.println("Info: SD card is working properly");
  } else {
       Serial.println("Error: SD card error: " + String(SD.card()->errorCode()));
    return;
  }

  // ファイルの存在確認
  if (!SD.exists("weights.csv")) {
       Serial.println("Error: weights.csv does not exist in SD card root");
    return;
  }

  // メモリの解放（以前の割り当てがある場合）
  if (coefficients != nullptr) {
    delete[] coefficients;
    coefficients = nullptr;  // 安全のためnullptr設定
  }

  // ファイルを開く
  FsFile weightsFile = SD.open("weights.csv", FILE_READ);
  if (!weightsFile) {
       Serial.println("Error: Failed to open weights.csv");
       Serial.println("Info: File exists but could not be opened");
    return;
  }

  // 新しいメモリの割り当て
  coefficients = new float[num_features];
  if (coefficients == nullptr) {
       Serial.println("Error: Failed to allocate memory for coefficients");
    weightsFile.close();
    return;
  }

  char buffer[64];
  bool success = true;

  // パラメータヘッダーをスキップ
  weightsFile.readStringUntil('\n');  // Skip "parameter,value"

  // 標準化パラメータの読み込み
  for (int i = 0; i < 6 && success; i++) {
    if (weightsFile.fgets(buffer, sizeof(buffer))) {
      String line = String(buffer);
      int commaIndex = line.indexOf(',');
      if (commaIndex != -1) {
        String value = line.substring(commaIndex + 1);
        float param = value.toFloat();
        
        switch(i) {
          case 0: y_mean = param; break;
          case 1: y_std = param; break;
          case 2: theta_mean = param; break;
          case 3: theta_std = param; break;
          case 4: distance_mean = param; break;
          case 5: distance_std = param; break;
        }
      } else {
        success = false;
      }
    } else {
      success = false;
    }
  }

  if (!success) {
       Serial.println("Error: Failed to read standardization parameters");
    delete[] coefficients;
    coefficients = nullptr;
    weightsFile.close();
    return;
  }

  // 係数ヘッダーをスキップ
  weightsFile.readStringUntil('\n');  // Skip "index,weight"

  // 係数の読み込み
  for (int i = 0; i < num_features && success; i++) {
    if (weightsFile.fgets(buffer, sizeof(buffer))) {
      String line = String(buffer);
      int commaIndex = line.indexOf(',');
      if (commaIndex != -1) {
        String value = line.substring(commaIndex + 1);
        coefficients[i] = value.toFloat();
      } else {
        success = false;
      }
    } else {
      success = false;
    }
  }

  weightsFile.close();

  if (!success) {
       Serial.println("Error: Failed to read coefficients");
    delete[] coefficients;
    coefficients = nullptr;
    return;
  }

  // デバッグ出力（debugMode時のみ）
  if (debugMode) {
       Serial.println("標準化パラメータ:");
       Serial.println("y_mean: " + String(y_mean));
       Serial.println("y_std: " + String(y_std));
       Serial.println("theta_mean: " + String(theta_mean));
       Serial.println("theta_std: " + String(theta_std));
       Serial.println("distance_mean: " + String(distance_mean));
       Serial.println("distance_std: " + String(distance_std));
    
       Serial.println("\n最初の5つの係数:");
    for (int i = 0; i < 5 && i < num_features; i++) {
         Serial.println("係数[" + String(i) + "]: " + String(coefficients[i]));
    }
  }

     Serial.println("Info: Weights loaded successfully");
}

/**
 * 安全な累乗計算を行う関数
 * @param base 底
 * @param exp 指数
 * @return 累乗値（数値安定化処理済み）
 */
float safe_power(float base, int exp) {
    if (exp == 0) return 1.0;
    if (exp == 1) return base;
    if (exp == 2) return base * base;

    // 数値の範囲チェック
    if (abs(base) < 1e-6) return 0.0;
    if (abs(base) > 1e6) {
        return (base > 0) ? 1e6 : -1e6;
    }

    float result = base;
    float max_allowed = 1e6 / abs(base);  // オーバーフロー防止の閾値
    
    for (int i = 2; i <= exp; i++) {
        // 次の乗算で閾値を超える場合
        if (abs(result) > max_allowed) {
            return (result > 0) ? 1e6 : -1e6;
        }
        result *= base;
    }
    return result;
}

// 距離推定（標準化処理を含む）
// グローバル配列として特徴量を保持
float features[15];  // weights.csvの係数の数に合わせる

// 累乗値を保持する配列（計算の最適化用）
float y_powers[5];     // 0次から4次までのY累乗
float theta_powers[5]; // 0次から4次までのtheta累乗

float calculateDistance(float theta, float y, int height) {
  if (debugMode) {
       Serial.println("入力値:");
       Serial.println("theta: " + String(theta));
       Serial.println("y: " + String(y));
       Serial.println("height: " + String(height));
  }

  // 物体の下限のy座標を計算
  y = constrain(y + (float(height) / 2.0), 0.0f, 100.0f);

  /*
  // 入力値の妥当性チェック
  if (y < 0 || y > 120) {
    if (debugMode) {
         Serial.println("警告: y座標が範囲外です");
    }
    return -1;
  }

  // 特徴量の生成と標準化
  float y_normalized = (y - y_mean) / y_std;
  float theta_normalized = (theta - theta_mean) / theta_std;

  // 数値の妥当性チェック
  if (isnan(y_normalized) || isnan(theta_normalized) || 
      isinf(y_normalized) || isinf(theta_normalized)) {
    if (debugMode) {
         Serial.println("エラー: 正規化値が無効です");
    }
    return -1;
  }

  if (debugMode) {
       Serial.println("正規化された値:");
       Serial.println("y_normalized: " + String(y_normalized));
       Serial.println("theta_normalized: " + String(theta_normalized));
  }

  // 重みの数から多項式の次数を決定（15個の特徴量用）
  const int degree = 4;  // 4次の多項式まで
  int idx = 0;
  
  // 累乗値の事前計算
  y_powers[0] = 1.0;
  theta_powers[0] = 1.0;
  y_powers[1] = y_normalized;
  theta_powers[1] = theta_normalized;
  
  for (int i = 2; i <= degree; i++) {
    y_powers[i] = safe_power(y_normalized, i);
    theta_powers[i] = safe_power(theta_normalized, i);
    
    // 数値の妥当性チェック
    if (isnan(y_powers[i]) || isnan(theta_powers[i]) ||
        isinf(y_powers[i]) || isinf(theta_powers[i])) {
      if (debugMode) {
           Serial.println("エラー: 累乗計算で無効な値が発生しました");
      }
      return -1;
    }
  }
  
  // バイアス項
  features[idx++] = 1.0;  // weight_0用
  
  // 一次の項
  features[idx++] = y_normalized;  // weight_1用
  features[idx++] = theta_normalized;  // weight_2用
  
  // 二次の項
  features[idx++] = y_powers[2];  // weight_3用
  features[idx++] = theta_powers[2];  // weight_4用
  features[idx++] = y_normalized * theta_normalized;  // weight_5用
  
  // 三次の項
  features[idx++] = y_powers[3];  // weight_6用
  features[idx++] = theta_powers[3];  // weight_7用
  features[idx++] = y_powers[2] * theta_normalized;  // weight_8用
  features[idx++] = y_normalized * theta_powers[2];  // weight_9用
  
  // 四次の項
  features[idx++] = y_powers[4];  // weight_10用
  features[idx++] = theta_powers[4];  // weight_11用
  features[idx++] = y_powers[3] * theta_normalized;  // weight_12用
  features[idx++] = y_powers[2] * theta_powers[2];  // weight_13用
  features[idx++] = y_normalized * theta_powers[3];  // weight_14用

  // 一部の特徴量をデバッグ出力
  if (debugMode) {
       Serial.println("主要な特徴量の値:");
       Serial.println("バイアス項: " + String(features[0]));
       Serial.println("y^1: " + String(features[1]));
       Serial.println("theta^1: " + String(features[2]));
       Serial.println("y^2: " + String(features[3]));
       Serial.println("theta^2: " + String(features[4]));
       Serial.println("y*theta: " + String(features[5]));
       Serial.println("theta*y: " + String(features[6]));
  }

  // 予測値の計算（部分和による安定化）
  float normalized_distance = 0.0;
  float sum_positive = 0.0;
  float sum_negative = 0.0;

  for (int i = 0; i < num_features; i++) {
    float term = coefficients[i] * features[i];
    if (term >= 0) {
      sum_positive += term;
    } else {
      sum_negative += term;
    }
    
    // 中間結果の妥当性チェック
    if (isnan(sum_positive) || isnan(sum_negative) ||
        isinf(sum_positive) || isinf(sum_negative)) {
      if (debugMode) {
           Serial.println("警告: 項 " + String(i) + " で無効な値が計算されました");
      }
      return -1;
    }
  }
  
  normalized_distance = sum_positive + sum_negative;

  if (debugMode) {
       Serial.println("正規化された予測距離: " + String(normalized_distance));
  }
  
  // 予測値の逆標準化
  float predicted_distance = normalized_distance * distance_std + distance_mean;

  if (debugMode) {
       Serial.println("予測された実際の距離: " + String(predicted_distance));
  }
  
  // 予測値の範囲チェック（負の距離や異常に大きな値を防ぐ）
  if (predicted_distance < 0 || predicted_distance > 500) {
    if (debugMode) {
         Serial.println("警告: 予測値が範囲外です: " + String(predicted_distance));
    }
    return -1;
  }
  */
  float predicted_distance = predict_distance_teensy(y, theta);
  return predicted_distance;
}

// x座標から対応するファイルインデックスを計算
int getFileIndex(int x) {
  int split_size = ceil(161.0 / NUM_DISTANCE_FILES);
  return min(x / split_size, NUM_DISTANCE_FILES - 1);
}

// x座標から対応するファイル内での相対的なx座標を計算
int getRelativeX(int x) {
  int split_size = ceil(161.0 / NUM_DISTANCE_FILES);
  int file_index = getFileIndex(x);
  return x - (file_index * split_size);
}

// 検索による距離推定
float getDistanceFromCoordinates(int x, int y, int height) {
  // 物体の下限のy座標を計算
  y = constrain(y + (height / 2), 0, 120);
  FsFile distfile;
  
  // 座標の範囲チェック
  if (x < 0 || x > 160 || y < 0 || y > 120) {
       Serial.println("Error: Invalid input - x must be 0-160, y must be 0-120");
    return -1;
  }

  // ファイルインデックスを計算
  int fileIndex = getFileIndex(x);
  String filename = "distance_" + String(fileIndex) + ".csv";

  distfile = SD.open(filename.c_str(), O_RDONLY);
  if (!distfile) {
       Serial.println("Error: Could not open file: " + filename);
    return -1;
  }

  // ヘッダー行をスキップ
  distfile.readStringUntil('\n');

  // 相対的なx座標を計算
  int relativeX = getRelativeX(x);

  // 目的の行までスキップ (relativeX * 121 + y)
  long lineToSkip = (long)relativeX * 121 + y;
  for (long i = 0; i < lineToSkip; i++) {
    distfile.readStringUntil('\n');
  }

  String line = distfile.readStringUntil('\n');
  if (line == "") {
       Serial.println("Error: Failed to read line from file");
    distfile.close();
    return -1;
  }

  // カンマで分割して distance を取得
  int firstCommaIndex = line.indexOf(',');
  int secondCommaIndex = line.indexOf(',', firstCommaIndex + 1);
  String distanceStr = line.substring(secondCommaIndex + 1);
  
  float distance = distanceStr.toFloat();
  distfile.close();

  if (distanceStr.length() > 0) {
    return distance;
  } else {
       Serial.println("Error: Distance not found in file");
    return -1;
  }
}



// RCJ Soccer フィールド寸法（cm）- playing field 基準（白線内側）
// 座標系:
//   原点   = フィールド中央
//   +Y     = 敵ゴール方向（前方）
//   -Y     = 味方ゴール方向（後方）
//   +X     = フィールド右方向（敵ゴールを前として）
//   -X     = フィールド左方向
// ゴール中心座標: 味方ゴール=(0, -FIELD_HALF_LENGTH), 敵ゴール=(0, +FIELD_HALF_LENGTH)
const float FIELD_HALF_LENGTH = 121.5f;  // 243cm / 2
const float FIELD_HALF_WIDTH  =  91.0f;  // 182cm / 2

// 自己位置推定の信頼度と有効性（predictCoordinatesGlobal() で更新）
// positionConfidence: 0.0=無効, 0.5=片方ゴールのみ可視, 1.0=両ゴール可視
float positionConfidence = 0.0f;
bool  positionValid      = false;

float degToRad(float deg) {
  return deg * (PI / 180.0f);
}

// 機体ローカル角度 -> フィールド座標系の角度に変換
float fieldBearingFromLocal(int localTheta) {
  int objectTheta = In360(currentDirection() + In180(localTheta));
  int difTheta = In180(objectTheta - goalDirection);  // フィールド前方(敵ゴール方向)を基準
  return float(difTheta);
}

void predictCoordinatesGlobal(int* cx, int* cy){
  /*
  両ゴール可視時は距離の逆数重み付き平均で推定（安定性向上）
  片方のみ可視時はそのゴール基準で推定
  どちらも見えない場合は cx=cy=255, positionValid=false を返す

  座標系（FIELD_HALF_LENGTH/FIELD_HALF_WIDTH で定義済み）:
    敵ゴール中心: (0, +FIELD_HALF_LENGTH)
    味方ゴール中心: (0, -FIELD_HALF_LENGTH)
  */
  float sumX = 0.0f;
  float sumY = 0.0f;
  float weightSum = 0.0f;

  bool goalVisible     = (goalColorDistance > 0 && goalColorTheta != -1);
  bool opponentVisible = (opponentColorDistance > 0 && opponentColorTheta != -1);

  if (goalVisible) {
    float thetaField = fieldBearingFromLocal(goalColorTheta);
    float r = degToRad(thetaField);
    // 敵ゴール(+Y端)基準: 機体位置 = ゴール位置 - 機体→ゴールベクトル
    float gx = -float(goalColorDistance) * sinf(r);
    float gy =  FIELD_HALF_LENGTH - float(goalColorDistance) * cosf(r);
    // 距離の逆数を重みとして使用（近いゴールほど信頼度が高い）
    float w = 1.0f / (float(goalColorDistance) + 1.0f);
    sumX += gx * w;
    sumY += gy * w;
    weightSum += w;
  }

  if (opponentVisible) {
    float thetaField = fieldBearingFromLocal(opponentColorTheta);
    float r = degToRad(thetaField);
    // 味方ゴール(-Y端)基準: 機体位置 = ゴール位置 - 機体→ゴールベクトル
    float gx = -float(opponentColorDistance) * sinf(r);
    float gy = -FIELD_HALF_LENGTH - float(opponentColorDistance) * cosf(r);
    float w = 1.0f / (float(opponentColorDistance) + 1.0f);
    sumX += gx * w;
    sumY += gy * w;
    weightSum += w;
  }

  if (weightSum == 0.0f) {
    *cx = 255;
    *cy = 255;
    positionConfidence = 0.0f;
    positionValid = false;
    return;
  }

  // 信頼度設定: 両ゴール可視=1.0, 片方のみ=0.5
  positionConfidence = (goalVisible && opponentVisible) ? 1.0f : 0.5f;
  positionValid = true;

  float fx = sumX / weightSum;
  float fy = sumY / weightSum;

  // フィールド外クランプ（推定値が飛んでもフィールド内に収める）
  fx = constrain(fx, -FIELD_HALF_WIDTH, FIELD_HALF_WIDTH);
  fy = constrain(fy, -FIELD_HALF_LENGTH, FIELD_HALF_LENGTH);

  // EMAフィルタ（alpha=0.3: 急激な飛びを抑制、遅延を最小限に）
  static float smoothX = 0.0f, smoothY = 0.0f;
  static bool firstEstimate = true;
  if (firstEstimate) {
    smoothX = fx;
    smoothY = fy;
    firstEstimate = false;
  } else {
    const float alpha = 0.3f;
    smoothX = alpha * fx + (1.0f - alpha) * smoothX;
    smoothY = alpha * fy + (1.0f - alpha) * smoothY;
  }

  *cx = (int)roundf(smoothX);
  *cy = (int)roundf(smoothY);
}

void predictCoordinatesLocal(){
  /*
  機体の位置を0,0としたローカル座標空間の計算
  ballTheta, ballDistance, blueTheta, blueDistance, yellowTheta, yellowDistance
  goalColorTheta
    goalColorDistance 
    opponentColorTheta
    opponentColorDistance 
  */
  delay(1000);

}


int parallelTheta(int theta){
  /*
  機体の向きを考慮して角度を補正
  コンパスのズレから機体が正面を向いていた時にThetaが何度かを計算
  */
  //ローカル角度をグローバル角度に変換
  int objectTheta = In360(currentDirection() + In180(theta));

  //目標方向との角度差を計算（-180～180度の範囲で表現）
  int difTheta = In180(objectTheta - goalDirection);
  
  return difTheta;
}
