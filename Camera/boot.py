import sensor
import image
import time
import ustruct
from fpioa_manager import fm
from machine import UART
from modules import ws2812

# =========================================================
# 1) ここだけ変更すればOK：デバッグ表示（枠＋print）切替
# =========================================================
SHOW_DEBUG = False  # True: debug draw/print, False: max throughput

# =============================
# Tuning constants (調整ポイント)
# =============================
# OV7740 手動露光: 0x10 = AEC[7:0], 0-x0F = AEC[15:8]
MANUAL_EXPOSURE = 0x0200  # 例: 512（環境に合わせて調整）

# OV7740 手動ゲイン: 0x00 = AGC[7:0], 0x15[1:0] = AGC[9:8]
MANUAL_GAIN_10B = 0x0110  # 例: 256（環境に合わせて調整）

# RGBゲイン（緑かぶり等の色味調整はここ）
# 一般に緑かぶり → R/Bを上げる or Gを下げる
MANUAL_RGAIN = 0x40
MANUAL_GGAIN = 0x33
MANUAL_BGAIN = 0x60

# 起動時の押し込み（初期フレームで勝手に掴んだ値を上書きで潰す）
INIT_ENFORCE_LOOPS = 8
INIT_ENFORCE_SLEEP_MS = 15
RUNTIME_ENFORCE_EVERY_FRAMES = 10

# snapshot リトライ
SNAPSHOT_RETRIES = 8
SNAPSHOT_RETRY_DELAY_MS = 10

# デバッグprint間隔（SHOW_DEBUG=Trueのときのみ有効）
DEBUG_PRINT_EVERY = 20

# UART/TX pacing (keep MainCPU packet protocol unchanged)
UART_BAUD = 38400
UART_PACKET_LEN = 40
_UART_BITS_PER_BYTE = 10
_TX_INTERVAL_MARGIN_MS = 2
TX_INTERVAL_MS = ((UART_PACKET_LEN * _UART_BITS_PER_BYTE * 1000) + UART_BAUD - 1) // UART_BAUD + _TX_INTERVAL_MARGIN_MS

# =============================
# Register helpers (MaixPy sensor)
# =============================
def _r(addr):
    try:
        return sensor.__read_reg(addr)
    except Exception:
        return None

def _w(addr, val):
    try:
        sensor.__write_reg(addr, val & 0xFF)
        return True
    except Exception:
        return False

# =============================
# OV7740 deterministic lock
# =============================
def ov7740_write_manual_exp_gain_rgb(exposure16, gain10, rgain, ggain, bgain):
    # COM8(0x13)でAEC/AGC/AWB等をOFF（manualへ）
    # AEC enable: 0x13[0], AGC enable: 0x13[2]（0=manual） :contentReference[oaicite:3]{index=3}
    _w(0x13, 0x00)

    # Manual exposure: 0x0F/0x10 :contentReference[oaicite:4]{index=4}
    _w(0x0F, (exposure16 >> 8) & 0xFF)
    _w(0x10, exposure16 & 0xFF)

    # Manual gain: 0x15[1:0] + 0x00 :contentReference[oaicite:5]{index=5}
    g_lsb = gain10 & 0xFF
    g_msb = (gain10 >> 8) & 0x03
    v15 = _r(0x15) or 0
    v15 = (v15 & ~0x03) | g_msb
    _w(0x15, v15)
    _w(0x00, g_lsb)

    # RGB gains（環境によって差が出るので固定値で運用）
    _w(0x02, rgain)  # RGAIN
    _w(0x03, ggain)  # GGAIN
    _w(0x01, bgain)  # BGAIN

def ov7740_dump_core():
    # SHOW_DEBUG=Trueのときだけ出す
    if not SHOW_DEBUG:
        return
    print("REG 0x13(COM8) =", _r(0x13))
    print("REG 0x0F(AEC_H) =", _r(0x0F))
    print("REG 0x10(AEC_L) =", _r(0x10))
    print("REG 0x15(GAIN_MSB) =", _r(0x15))
    print("REG 0x00(GAIN_LSB) =", _r(0x00))
    print("REG 0x02(RGAIN) =", _r(0x02))
    print("REG 0x03(GGAIN) =", _r(0x03))
    print("REG 0x01(BGAIN) =", _r(0x01))

# =============================
# IO / globals
# =============================
# WS2812 LED
class_ws2812 = ws2812(8, 1)
class_ws2812.set_led(0, (0, 0, 0))
class_ws2812.display()

# UART
fm.register(34, fm.fpioa.UART1_TX, force=True)
fm.register(35, fm.fpioa.UART1_RX, force=True)
uart = UART(UART.UART1, UART_BAUD, 8, 0, 1, timeout=1000, read_buf_len=4096)

# detection globals
colorx1 = [0, 0, 0]   # top-left x (b.x() + 100)
colory1 = [0, 0, 0]   # top-left y (b.y() + 100)
colorx2 = [0, 0, 0]   # bottom-right x (b.x() + b.w() + 100)
colory2 = [0, 0, 0]   # bottom-right y (b.y() + b.h() + 100)
pixels = [2, 10, 10]
areas = [2, 10, 10]
merge_margins = [10, 30, 30]

# thresholds (あなたの現状値)
threshold = [
    (45, 81, 18, 78, 50, 84),      # ball
    (6, 14, 15, 43, -57, -32),      # blue goal
    (55, 77, -12, 5, 56, 79)    # yellow goal
]
THRESHOLD_SETS = ([threshold[0]], [threshold[1]], [threshold[2]])

def safe_snapshot():
    for _ in range(SNAPSHOT_RETRIES):
        try:
            return sensor.snapshot()
        except Exception:
            time.sleep_ms(SNAPSHOT_RETRY_DELAY_MS)
    return None

# =============================
# Camera init
# =============================
def init_camera():
    # ov7740は24MHzで色ノイズが出ることがあり、20MHz推奨例あり :contentReference[oaicite:6]{index=6}
    sensor.reset(freq=20000000, set_regs=True, dual_buff=True)
    sensor.set_jb_quality(100)
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QQVGA)

    sensor.set_contrast(-7)
    sensor.set_saturation(-5)
    sensor.set_brightness(3)

    # 起動直後の“掴み”を上書きで潰す（ここが効く）
    for _ in range(INIT_ENFORCE_LOOPS):
        ov7740_write_manual_exp_gain_rgb(
            MANUAL_EXPOSURE,
            MANUAL_GAIN_10B,
            MANUAL_RGAIN, MANUAL_GGAIN, MANUAL_BGAIN
        )
        time.sleep_ms(INIT_ENFORCE_SLEEP_MS)

    sensor.skip_frames(time=300)

    for _ in range(INIT_ENFORCE_LOOPS):
        ov7740_write_manual_exp_gain_rgb(
            MANUAL_EXPOSURE,
            MANUAL_GAIN_10B,
            MANUAL_RGAIN, MANUAL_GGAIN, MANUAL_BGAIN
        )
        time.sleep_ms(INIT_ENFORCE_SLEEP_MS)

    ov7740_dump_core()

# =============================
# Main loop
# =============================
init_camera()
clock = time.clock()
frame = 0
# TX pacing from UART line-rate (40 bytes @ 38400bps + margin).
_last_tx_ms = time.ticks_ms()
pkt = bytearray(UART_PACKET_LEN)
pkt[0] = 255

try:
    while True:
        clock.tick()
        img = safe_snapshot()
        if img is None:
            continue

        frame += 1

        # 定期的に押し戻し（ドライバ/センサが書き戻す個体対策）
        if frame % RUNTIME_ENFORCE_EVERY_FRAMES == 0:
            ov7740_write_manual_exp_gain_rgb(
                MANUAL_EXPOSURE,
                MANUAL_GAIN_10B,
                MANUAL_RGAIN, MANUAL_GGAIN, MANUAL_BGAIN
            )

        # init per-frame outputs
        for i in range(3):
            colorx1[i] = 0
            colory1[i] = 0
            colorx2[i] = 0
            colory2[i] = 0

        # detect
        for color_index in range(3):
            blobs = img.find_blobs(THRESHOLD_SETS[color_index],
                                   pixels_threshold=pixels[color_index],
                                   area_threshold=areas[color_index],
                                   merge=True,
                                   margin=merge_margins[color_index])
            if blobs:
                b = max(blobs, key=lambda bb: bb.area())

                colorx1[color_index] = b.x() + 100
                colory1[color_index] = b.y() + 100
                colorx2[color_index] = b.x() + b.w() + 100
                colory2[color_index] = b.y() + b.h() + 100

                # =========================================================
                # SHOW_DEBUG が True のときだけ枠を描く
                # blob.rect()/cx()/cy() はOpenMV互換仕様 :contentReference[oaicite:7]{index=7}
                # =========================================================
                if SHOW_DEBUG:
                    if color_index == 0:
                        c = (255, 255, 255)  # ball
                    elif color_index == 1:
                        c = (0, 0, 255)      # blue
                    else:
                        c = (255, 255, 0)    # yellow
                    img.draw_rectangle(b.rect(), color=c)
                    img.draw_cross(b.cx(), b.cy(), color=c)

        # debug print
        fps_raw = clock.fps()
        fps = max(0.0, fps_raw)  # MaixPy ticks overflow 等で負値になる場合を防ぐ
        if SHOW_DEBUG and (frame % DEBUG_PRINT_EVERY == 0):
            print(fps)

        # UART送信（対角2点フォーマット: x1,y1,x2,y2 各3桁 = 12バイト/物体、合計40バイト）
        # 40個の個別write()ではなく1回のwrite()で送信（TX FIFOの溢れ防止）
        idx = 1
        for i in range(3):
            v = int(colorx1[i])
            pkt[idx]     = (v // 100) % 10
            pkt[idx + 1] = (v // 10)  % 10
            pkt[idx + 2] = v % 10
            idx += 3

            v = int(colory1[i])
            pkt[idx]     = (v // 100) % 10
            pkt[idx + 1] = (v // 10)  % 10
            pkt[idx + 2] = v % 10
            idx += 3

            v = int(colorx2[i])
            pkt[idx]     = (v // 100) % 10
            pkt[idx + 1] = (v // 10)  % 10
            pkt[idx + 2] = v % 10
            idx += 3

            v = int(colory2[i])
            pkt[idx]     = (v // 100) % 10
            pkt[idx + 1] = (v // 10)  % 10
            pkt[idx + 2] = v % 10
            idx += 3

        fps_adj = min(max(int(fps + 100), 0), 999)
        pkt[37] = int(fps_adj / 100) % 10
        pkt[38] = int(fps_adj / 10)  % 10
        pkt[39] = fps_adj % 10

        # Non-blocking TX pacing near UART line-rate limit.
        _now = time.ticks_ms()
        if time.ticks_diff(_now, _last_tx_ms) >= TX_INTERVAL_MS:
            uart.write(pkt)
            _last_tx_ms = _now

except KeyboardInterrupt:
    pass
finally:
    try:
        class_ws2812.set_led(0, (0, 0, 0))
        class_ws2812.display()
    except Exception:
        pass
    try:
        uart.deinit()
    except Exception:
        pass
    try:
        sensor.run(0)
        sensor.shutdown()
    except Exception:
        pass
