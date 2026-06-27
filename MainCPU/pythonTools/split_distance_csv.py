import csv
import os
import sys
import math

def split_csv(input_file, output_dir, num_splits=10):
    print(f"CSVファイルを{num_splits}分割します...")
    
    # 出力ディレクトリの作成（存在しない場合）
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # 入力ファイルの行数をカウント
    with open(input_file, 'r') as f:
        total_lines = sum(1 for _ in f)
    
    # x座標の範囲を計算（0-160）
    x_range = 161
    split_size = math.ceil(x_range / num_splits)
    
    # 分割ファイルのハンドラを準備
    output_files = []
    writers = []
    for i in range(num_splits):
        filename = os.path.join(output_dir, f'distance_{i}.csv')
        file = open(filename, 'w', newline='')
        output_files.append(file)
        writers.append(csv.writer(file))
        # ヘッダーを書き込む
        writers[i].writerow(['x', 'y', 'distance'])
    
    # 入力ファイルを読み込んで分割
    processed_lines = 0
    with open(input_file, 'r') as f:
        reader = csv.reader(f)
        next(reader)  # ヘッダーをスキップ
        
        for row in reader:
            x = int(row[0])
            file_index = min(x // split_size, num_splits - 1)
            writers[file_index].writerow(row)
            
            processed_lines += 1
            if processed_lines % 1000 == 0:
                progress = (processed_lines / total_lines) * 100
                print(f"進捗: {progress:.1f}%")
    
    # ファイルを閉じる
    for f in output_files:
        f.close()
    
    print("分割完了！")
    print(f"各ファイルのx座標範囲:")
    for i in range(num_splits):
        start_x = i * split_size
        end_x = min((i + 1) * split_size - 1, 160)
        print(f"distance_{i}.csv: x = {start_x} から {end_x}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        num_splits = int(sys.argv[1])
    else:
        num_splits = 10
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    parent_dir = os.path.dirname(script_dir)
    input_file = os.path.join(parent_dir, 'SD', 'distance.csv')
    output_dir = os.path.join(parent_dir, 'SD')
    
    split_csv(input_file, output_dir, num_splits)
