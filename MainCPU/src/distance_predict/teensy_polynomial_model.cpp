/*
 * Teensy 4.1 多項式回帰モデル実装 (17次)
 * 自動生成元: models/20250721_233056/polynomial_degree17_mae0.90.joblib
 * 生成日時: 2025-07-27 15:11:00
 *
 * ARM Cortex-M7 ハードウェアFPU向けに最適化
 * 目標実行時間: 100マイクロ秒未満
 * メモリ使用量: Flash約3KB、RAM約1KB
 */

#include "teensy_polynomial_model.h"
#include <pgmspace.h>

// Teensy 4.1向けに最適化されたメイン予測関数 (倍精度のみ)
TEENSY_FAST float predict_distance_teensy(float under_y, float theta) {
    // 入力値検証
    if (!validate_input_range(under_y, theta)) {
        return -1.0f;  // エラー指標
    }

    // 最大精度を確保するため、全計算に倍精度を使用
    double features[FEATURE_COUNT];

    // 倍精度で多項式特徴量を生成
    generate_polynomial_features_double(under_y, theta, features);

    // 倍精度でStandardScaler正規化を適用
    apply_standard_scaling_double(features);

    // 倍精度とKahan総和法で最終予測を計算
    return (float)compute_linear_combination_double(features);
}

// 精度確保のための倍精度多項式特徴量生成
TEENSY_FAST void generate_polynomial_features_double(float under_y, float theta, double* features) {
    // PC版scikit-learnの精度と一致させるため倍精度を使用
    double under_y_d = (double)under_y;
    double theta_d = (double)theta;

    // 倍精度でべき乗を事前計算
    double under_y_powers[18];  // 0から17
    double theta_powers[18];    // 0から17

    under_y_powers[0] = 1.0;
    theta_powers[0] = 1.0;
    under_y_powers[1] = under_y_d;
    theta_powers[1] = theta_d;

    // 数値安定性を向上させた高次べき乗の計算
    // 極値での精度向上のため直接乗算を使用
    for (int i = 2; i <= 17; i++) {
        under_y_powers[i] = under_y_powers[i-1] * under_y_d;
        theta_powers[i] = theta_powers[i-1] * theta_d;
    }

    // scikit-learn順序で171個の全特徴量を精度向上して生成
    features[0] = 1.0;  // バイアス項
    features[1] = under_y_d;  // under_y^1 * theta^0 (精度のため直接代入)
    features[2] = theta_d;    // under_y^0 * theta^1 (精度のため直接代入)
    features[3] = under_y_powers[2] * theta_powers[0];  // under_y^2 * theta^0
    features[4] = under_y_powers[1] * theta_powers[1];  // under_y^1 * theta^1
    features[5] = under_y_powers[0] * theta_powers[2];  // under_y^0 * theta^2
    features[6] = under_y_powers[3] * theta_powers[0];  // under_y^3 * theta^0
    features[7] = under_y_powers[2] * theta_powers[1];  // under_y^2 * theta^1
    features[8] = under_y_powers[1] * theta_powers[2];  // under_y^1 * theta^2
    features[9] = under_y_powers[0] * theta_powers[3];  // under_y^0 * theta^3
    features[10] = under_y_powers[4] * theta_powers[0];  // under_y^4 * theta^0
    features[11] = under_y_powers[3] * theta_powers[1];  // under_y^3 * theta^1
    features[12] = under_y_powers[2] * theta_powers[2];  // under_y^2 * theta^2
    features[13] = under_y_powers[1] * theta_powers[3];  // under_y^1 * theta^3
    features[14] = under_y_powers[0] * theta_powers[4];  // under_y^0 * theta^4
    features[15] = under_y_powers[5] * theta_powers[0];  // under_y^5 * theta^0
    features[16] = under_y_powers[4] * theta_powers[1];  // under_y^4 * theta^1
    features[17] = under_y_powers[3] * theta_powers[2];  // under_y^3 * theta^2
    features[18] = under_y_powers[2] * theta_powers[3];  // under_y^2 * theta^3
    features[19] = under_y_powers[1] * theta_powers[4];  // under_y^1 * theta^4
    features[20] = under_y_powers[0] * theta_powers[5];  // under_y^0 * theta^5
    features[21] = under_y_powers[6] * theta_powers[0];  // under_y^6 * theta^0
    features[22] = under_y_powers[5] * theta_powers[1];  // under_y^5 * theta^1
    features[23] = under_y_powers[4] * theta_powers[2];  // under_y^4 * theta^2
    features[24] = under_y_powers[3] * theta_powers[3];  // under_y^3 * theta^3
    features[25] = under_y_powers[2] * theta_powers[4];  // under_y^2 * theta^4
    features[26] = under_y_powers[1] * theta_powers[5];  // under_y^1 * theta^5
    features[27] = under_y_powers[0] * theta_powers[6];  // under_y^0 * theta^6
    features[28] = under_y_powers[7] * theta_powers[0];  // under_y^7 * theta^0
    features[29] = under_y_powers[6] * theta_powers[1];  // under_y^6 * theta^1
    features[30] = under_y_powers[5] * theta_powers[2];  // under_y^5 * theta^2
    features[31] = under_y_powers[4] * theta_powers[3];  // under_y^4 * theta^3
    features[32] = under_y_powers[3] * theta_powers[4];  // under_y^3 * theta^4
    features[33] = under_y_powers[2] * theta_powers[5];  // under_y^2 * theta^5
    features[34] = under_y_powers[1] * theta_powers[6];  // under_y^1 * theta^6
    features[35] = under_y_powers[0] * theta_powers[7];  // under_y^0 * theta^7
    features[36] = under_y_powers[8] * theta_powers[0];  // under_y^8 * theta^0
    features[37] = under_y_powers[7] * theta_powers[1];  // under_y^7 * theta^1
    features[38] = under_y_powers[6] * theta_powers[2];  // under_y^6 * theta^2
    features[39] = under_y_powers[5] * theta_powers[3];  // under_y^5 * theta^3
    features[40] = under_y_powers[4] * theta_powers[4];  // under_y^4 * theta^4
    features[41] = under_y_powers[3] * theta_powers[5];  // under_y^3 * theta^5
    features[42] = under_y_powers[2] * theta_powers[6];  // under_y^2 * theta^6
    features[43] = under_y_powers[1] * theta_powers[7];  // under_y^1 * theta^7
    features[44] = under_y_powers[0] * theta_powers[8];  // under_y^0 * theta^8
    features[45] = under_y_powers[9] * theta_powers[0];  // under_y^9 * theta^0
    features[46] = under_y_powers[8] * theta_powers[1];  // under_y^8 * theta^1
    features[47] = under_y_powers[7] * theta_powers[2];  // under_y^7 * theta^2
    features[48] = under_y_powers[6] * theta_powers[3];  // under_y^6 * theta^3
    features[49] = under_y_powers[5] * theta_powers[4];  // under_y^5 * theta^4
    features[50] = under_y_powers[4] * theta_powers[5];  // under_y^4 * theta^5
    features[51] = under_y_powers[3] * theta_powers[6];  // under_y^3 * theta^6
    features[52] = under_y_powers[2] * theta_powers[7];  // under_y^2 * theta^7
    features[53] = under_y_powers[1] * theta_powers[8];  // under_y^1 * theta^8
    features[54] = under_y_powers[0] * theta_powers[9];  // under_y^0 * theta^9
    features[55] = under_y_powers[10] * theta_powers[0];  // under_y^10 * theta^0
    features[56] = under_y_powers[9] * theta_powers[1];  // under_y^9 * theta^1
    features[57] = under_y_powers[8] * theta_powers[2];  // under_y^8 * theta^2
    features[58] = under_y_powers[7] * theta_powers[3];  // under_y^7 * theta^3
    features[59] = under_y_powers[6] * theta_powers[4];  // under_y^6 * theta^4
    features[60] = under_y_powers[5] * theta_powers[5];  // under_y^5 * theta^5
    features[61] = under_y_powers[4] * theta_powers[6];  // under_y^4 * theta^6
    features[62] = under_y_powers[3] * theta_powers[7];  // under_y^3 * theta^7
    features[63] = under_y_powers[2] * theta_powers[8];  // under_y^2 * theta^8
    features[64] = under_y_powers[1] * theta_powers[9];  // under_y^1 * theta^9
    features[65] = under_y_powers[0] * theta_powers[10];  // under_y^0 * theta^10
    features[66] = under_y_powers[11] * theta_powers[0];  // under_y^11 * theta^0
    features[67] = under_y_powers[10] * theta_powers[1];  // under_y^10 * theta^1
    features[68] = under_y_powers[9] * theta_powers[2];  // under_y^9 * theta^2
    features[69] = under_y_powers[8] * theta_powers[3];  // under_y^8 * theta^3
    features[70] = under_y_powers[7] * theta_powers[4];  // under_y^7 * theta^4
    features[71] = under_y_powers[6] * theta_powers[5];  // under_y^6 * theta^5
    features[72] = under_y_powers[5] * theta_powers[6];  // under_y^5 * theta^6
    features[73] = under_y_powers[4] * theta_powers[7];  // under_y^4 * theta^7
    features[74] = under_y_powers[3] * theta_powers[8];  // under_y^3 * theta^8
    features[75] = under_y_powers[2] * theta_powers[9];  // under_y^2 * theta^9
    features[76] = under_y_powers[1] * theta_powers[10];  // under_y^1 * theta^10
    features[77] = under_y_powers[0] * theta_powers[11];  // under_y^0 * theta^11
    features[78] = under_y_powers[12] * theta_powers[0];  // under_y^12 * theta^0
    features[79] = under_y_powers[11] * theta_powers[1];  // under_y^11 * theta^1
    features[80] = under_y_powers[10] * theta_powers[2];  // under_y^10 * theta^2
    features[81] = under_y_powers[9] * theta_powers[3];  // under_y^9 * theta^3
    features[82] = under_y_powers[8] * theta_powers[4];  // under_y^8 * theta^4
    features[83] = under_y_powers[7] * theta_powers[5];  // under_y^7 * theta^5
    features[84] = under_y_powers[6] * theta_powers[6];  // under_y^6 * theta^6
    features[85] = under_y_powers[5] * theta_powers[7];  // under_y^5 * theta^7
    features[86] = under_y_powers[4] * theta_powers[8];  // under_y^4 * theta^8
    features[87] = under_y_powers[3] * theta_powers[9];  // under_y^3 * theta^9
    features[88] = under_y_powers[2] * theta_powers[10];  // under_y^2 * theta^10
    features[89] = under_y_powers[1] * theta_powers[11];  // under_y^1 * theta^11
    features[90] = under_y_powers[0] * theta_powers[12];  // under_y^0 * theta^12
    features[91] = under_y_powers[13] * theta_powers[0];  // under_y^13 * theta^0
    features[92] = under_y_powers[12] * theta_powers[1];  // under_y^12 * theta^1
    features[93] = under_y_powers[11] * theta_powers[2];  // under_y^11 * theta^2
    features[94] = under_y_powers[10] * theta_powers[3];  // under_y^10 * theta^3
    features[95] = under_y_powers[9] * theta_powers[4];  // under_y^9 * theta^4
    features[96] = under_y_powers[8] * theta_powers[5];  // under_y^8 * theta^5
    features[97] = under_y_powers[7] * theta_powers[6];  // under_y^7 * theta^6
    features[98] = under_y_powers[6] * theta_powers[7];  // under_y^6 * theta^7
    features[99] = under_y_powers[5] * theta_powers[8];  // under_y^5 * theta^8
    features[100] = under_y_powers[4] * theta_powers[9];  // under_y^4 * theta^9
    features[101] = under_y_powers[3] * theta_powers[10];  // under_y^3 * theta^10
    features[102] = under_y_powers[2] * theta_powers[11];  // under_y^2 * theta^11
    features[103] = under_y_powers[1] * theta_powers[12];  // under_y^1 * theta^12
    features[104] = under_y_powers[0] * theta_powers[13];  // under_y^0 * theta^13
    features[105] = under_y_powers[14] * theta_powers[0];  // under_y^14 * theta^0
    features[106] = under_y_powers[13] * theta_powers[1];  // under_y^13 * theta^1
    features[107] = under_y_powers[12] * theta_powers[2];  // under_y^12 * theta^2
    features[108] = under_y_powers[11] * theta_powers[3];  // under_y^11 * theta^3
    features[109] = under_y_powers[10] * theta_powers[4];  // under_y^10 * theta^4
    features[110] = under_y_powers[9] * theta_powers[5];  // under_y^9 * theta^5
    features[111] = under_y_powers[8] * theta_powers[6];  // under_y^8 * theta^6
    features[112] = under_y_powers[7] * theta_powers[7];  // under_y^7 * theta^7
    features[113] = under_y_powers[6] * theta_powers[8];  // under_y^6 * theta^8
    features[114] = under_y_powers[5] * theta_powers[9];  // under_y^5 * theta^9
    features[115] = under_y_powers[4] * theta_powers[10];  // under_y^4 * theta^10
    features[116] = under_y_powers[3] * theta_powers[11];  // under_y^3 * theta^11
    features[117] = under_y_powers[2] * theta_powers[12];  // under_y^2 * theta^12
    features[118] = under_y_powers[1] * theta_powers[13];  // under_y^1 * theta^13
    features[119] = under_y_powers[0] * theta_powers[14];  // under_y^0 * theta^14
    features[120] = under_y_powers[15] * theta_powers[0];  // under_y^15 * theta^0
    features[121] = under_y_powers[14] * theta_powers[1];  // under_y^14 * theta^1
    features[122] = under_y_powers[13] * theta_powers[2];  // under_y^13 * theta^2
    features[123] = under_y_powers[12] * theta_powers[3];  // under_y^12 * theta^3
    features[124] = under_y_powers[11] * theta_powers[4];  // under_y^11 * theta^4
    features[125] = under_y_powers[10] * theta_powers[5];  // under_y^10 * theta^5
    features[126] = under_y_powers[9] * theta_powers[6];  // under_y^9 * theta^6
    features[127] = under_y_powers[8] * theta_powers[7];  // under_y^8 * theta^7
    features[128] = under_y_powers[7] * theta_powers[8];  // under_y^7 * theta^8
    features[129] = under_y_powers[6] * theta_powers[9];  // under_y^6 * theta^9
    features[130] = under_y_powers[5] * theta_powers[10];  // under_y^5 * theta^10
    features[131] = under_y_powers[4] * theta_powers[11];  // under_y^4 * theta^11
    features[132] = under_y_powers[3] * theta_powers[12];  // under_y^3 * theta^12
    features[133] = under_y_powers[2] * theta_powers[13];  // under_y^2 * theta^13
    features[134] = under_y_powers[1] * theta_powers[14];  // under_y^1 * theta^14
    features[135] = under_y_powers[0] * theta_powers[15];  // under_y^0 * theta^15
    features[136] = under_y_powers[16] * theta_powers[0];  // under_y^16 * theta^0
    features[137] = under_y_powers[15] * theta_powers[1];  // under_y^15 * theta^1
    features[138] = under_y_powers[14] * theta_powers[2];  // under_y^14 * theta^2
    features[139] = under_y_powers[13] * theta_powers[3];  // under_y^13 * theta^3
    features[140] = under_y_powers[12] * theta_powers[4];  // under_y^12 * theta^4
    features[141] = under_y_powers[11] * theta_powers[5];  // under_y^11 * theta^5
    features[142] = under_y_powers[10] * theta_powers[6];  // under_y^10 * theta^6
    features[143] = under_y_powers[9] * theta_powers[7];  // under_y^9 * theta^7
    features[144] = under_y_powers[8] * theta_powers[8];  // under_y^8 * theta^8
    features[145] = under_y_powers[7] * theta_powers[9];  // under_y^7 * theta^9
    features[146] = under_y_powers[6] * theta_powers[10];  // under_y^6 * theta^10
    features[147] = under_y_powers[5] * theta_powers[11];  // under_y^5 * theta^11
    features[148] = under_y_powers[4] * theta_powers[12];  // under_y^4 * theta^12
    features[149] = under_y_powers[3] * theta_powers[13];  // under_y^3 * theta^13
    features[150] = under_y_powers[2] * theta_powers[14];  // under_y^2 * theta^14
    features[151] = under_y_powers[1] * theta_powers[15];  // under_y^1 * theta^15
    features[152] = under_y_powers[0] * theta_powers[16];  // under_y^0 * theta^16
    features[153] = under_y_powers[17] * theta_powers[0];  // under_y^17 * theta^0
    features[154] = under_y_powers[16] * theta_powers[1];  // under_y^16 * theta^1
    features[155] = under_y_powers[15] * theta_powers[2];  // under_y^15 * theta^2
    features[156] = under_y_powers[14] * theta_powers[3];  // under_y^14 * theta^3
    features[157] = under_y_powers[13] * theta_powers[4];  // under_y^13 * theta^4
    features[158] = under_y_powers[12] * theta_powers[5];  // under_y^12 * theta^5
    features[159] = under_y_powers[11] * theta_powers[6];  // under_y^11 * theta^6
    features[160] = under_y_powers[10] * theta_powers[7];  // under_y^10 * theta^7
    features[161] = under_y_powers[9] * theta_powers[8];  // under_y^9 * theta^8
    features[162] = under_y_powers[8] * theta_powers[9];  // under_y^8 * theta^9
    features[163] = under_y_powers[7] * theta_powers[10];  // under_y^7 * theta^10
    features[164] = under_y_powers[6] * theta_powers[11];  // under_y^6 * theta^11
    features[165] = under_y_powers[5] * theta_powers[12];  // under_y^5 * theta^12
    features[166] = under_y_powers[4] * theta_powers[13];  // under_y^4 * theta^13
    features[167] = under_y_powers[3] * theta_powers[14];  // under_y^3 * theta^14
    features[168] = under_y_powers[2] * theta_powers[15];  // under_y^2 * theta^15
    features[169] = under_y_powers[1] * theta_powers[16];  // under_y^1 * theta^16
    features[170] = under_y_powers[0] * theta_powers[17];  // under_y^0 * theta^17
}

// 倍精度StandardScaler変換
TEENSY_FAST void apply_standard_scaling_double(double* features) {
    // Teensy 4.1 ARM Cortex-M7では直接アクセスがより効率的
    for (int i = 0; i < FEATURE_COUNT; i++) {
        double mean = SCALER_MEAN[i];
        double scale = SCALER_SCALE[i];
        features[i] = (features[i] - mean) / scale;
    }
}

// 精度確保のためのKahan総和法による倍精度線形結合
TEENSY_FAST double compute_linear_combination_double(const double* features) {
    // 数値精度向上のためKahan総和アルゴリズムを使用
    double sum = MODEL_INTERCEPT;
    double c = 0.0;  // 下位ビット欠落の補償値

    for (int i = 0; i < FEATURE_COUNT; i++) {
        double coeff = MODEL_COEFFICIENTS[i];
        double product = coeff * features[i];

        // Kahan総和法
        double y = product - c;
        double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }

    return sum;
}



// 入力検証関数
bool validate_input_range(float under_y, float theta) {
    return (under_y >= UNDER_Y_MIN && under_y <= UNDER_Y_MAX &&
            theta >= THETA_MIN && theta <= THETA_MAX);
}

// デバッグ用ユーティリティ関数
void print_model_info() {
    Serial.println("=== Teensy 4.1 Polynomial Model Info (Degree 17) ===");
    Serial.print("Polynomial Degree: "); Serial.println(POLY_DEGREE);
    Serial.print("Feature Count: "); Serial.println(FEATURE_COUNT);
    Serial.print("Input Features: "); Serial.println(INPUT_FEATURES);
    Serial.print("Model Intercept: "); Serial.println((float)MODEL_INTERCEPT, 8);
    Serial.print("Training MAE: "); Serial.println(0.239984);
    Serial.print("Validation MAE: "); Serial.println(0.900133);
    Serial.println("Precision: Double precision (64-bit) + Kahan summation");
    Serial.println("Memory Usage: ~14KB Flash, ~2.2KB RAM");
    Serial.println("Expected Performance: <120μs per prediction");
    Serial.println("Optimization: ARM Cortex-M7 FPU + unified memory access");
    Serial.println("====================================================");
}



void print_feature_values_double(const double* features) {
    Serial.println("Feature values (first 20 and last 10):");
    // Print first 20 features
    for (int i = 0; i < 20 && i < FEATURE_COUNT; i++) {
        Serial.print("  Feature "); Serial.print(i); 
        Serial.print(": "); Serial.println((float)features[i], 6);
    }
    if (FEATURE_COUNT > 30) {
        Serial.println("  ... (middle features omitted) ...");
        // Print last 10 features
        for (int i = FEATURE_COUNT - 10; i < FEATURE_COUNT; i++) {
            Serial.print("  Feature "); Serial.print(i); 
            Serial.print(": "); Serial.println((float)features[i], 6);
        }
    }
}
