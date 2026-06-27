// 距離推定方法の選択フラグ（true: 多項式回帰、false: 検索方式）
bool usePolynomialDistance = true;

// 個別のラインセンサーの有効/無効フラグ（デフォルトですべて有効）
bool enableLineSensors[16] = {
    true, true, true, true,     // 0-3
    true, true, true, true,     // 4-7
    true, true, true, true,     // 8-11
    true, true, true, true      // 12-15
};

void optimization() {  //ロボットの個体差などに対する最適化
  switch (botNam) {
    case 1:
      break;
    case 2:
      break;
  }
}
