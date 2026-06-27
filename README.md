# RoboCupJunior 2026 Vision Code

This repository contains control and computer-vision code for an autonomous soccer robot developed for RoboCupJunior 2026 Incheon.  
The robot uses a Teensy 4.1 as the main CPU, together with four cameras, a line-sensor sub-CPU, a motor-drive sub-CPU, a BNO055 IMU, an SD card, an OLED display, NeoPixel LEDs, and other hardware.

This repository is intended for public release. The code reflects the in-development state as of June 2026.

## Overview

Main features:

- Ball, blue-goal, and yellow-goal detection using four cameras
- Orientation and heading measurement using a BNO055
- Omnidirectional movement control with four omni wheels
- Kicker / dribbler control
- Line detection and recovery behavior using 16 line sensors
- Settings loading, logging, and crash-report saving through the SD card
- Angle and distance estimation from camera coordinates
- Debug output using OLED, USB Serial, and NeoPixel LEDs

## Directory Structure

```text
.
├── Camera/                 # MaixPy code for the camera modules
│   └── boot.py
├── Line/                   # Sub-CPU code for the line sensors
│   ├── Line.ino
│   └── 01_...04_*.ino
├── MainCPU/                # Main control code for Teensy 4.1
│   ├── MainCPU.ino
│   ├── 00_...15_*.ino
│   ├── SD/                 # Settings and prediction data placed on the SD card
│   ├── pythonTools/        # Utility scripts for distance-prediction data
│   └── src/distance_predict/
└── MotorDrive/             # Sub-CPU code for motor drive control
    ├── MotorDrive.ino
    └── 01_...05_*.ino
```

In the Arduino IDE, all `.ino` files in each folder are combined and built as a single sketch.

## Hardware

This code assumes the following hardware configuration:

- MainCPU: Teensy 4.1
- MotorDrive: Seeeduino XIAO RP2040
- Line: Seeeduino XIAO RP2040
- Camera: UnitV AI Camera
- IMU: Adafruit BNO055
- Display: SSD1306 OLED 128x32
- Storage: Teensy 4.1 built-in SDIO SD card
- LED: NeoPixel ring
- Line sensor: 16-channel analog line sensor through CD74HC4067
- Motor driver: DRV8870

## Dependencies

Main Arduino / Teensy libraries:

- `Wire`
- `SPI`
- `Adafruit_SSD1306`
- `Adafruit_Sensor`
- `Adafruit_BNO055`
- `SdFat`
- `Adafruit_NeoPixel`
- `CD74HC4067`
- `Noa.h-DRV8870`

`Noa.h-DRV8870` is a modified version of a publicly available DRV8870 library, adjusted for the hardware configuration used by Noa.h.

The camera code uses the following MaixPy modules:

- `sensor`
- `image`
- `time`
- `ustruct`
- `fpioa_manager`
- `machine.UART`
- `modules.ws2812`

## SD Card Settings

`MainCPU/SD/` contains settings files and distance-prediction data read by the MainCPU at startup.  
The distance-prediction data is a remnant of an older implementation.

Main files:

- `setting.txt`: robot settings
- `weights.csv`: weights for the distance-prediction model
- `distance.csv`, `distance_0.csv` ... `distance_9.csv`: distance-prediction data
- `検証用データ_theta.csv`: validation data

`setting.txt` is read as one value per line. The values correspond to the following variables in the code:

```text
1: blueDirection
2: yellowDirection
3: botSpeed
4: kickerThreshold
5: line1Threshold
6: line2Threshold
7: line3Threshold
8: line4Threshold
9: dribblerSpeed
10: blueGoalEnabled
11: yellowGoalEnabled
12: minRotatePowerLeft
13: minRotatePowerRight
```

If the current `setting.txt` uses an older format, missing values fall back to the initial values defined in the code. For public sharing, it is recommended to explicitly include all 13 lines.

## Communication Overview

### Camera to MainCPU

- UART: 38400 bps
- Packet size: 40 bytes
- Header: `255`
- Payload: bounding box coordinates and FPS for the ball, blue goal, and yellow goal
- Coordinate values are sent as three-digit BCD-like data with `+100` applied, then reconstructed on the MainCPU side.

### MainCPU to MotorDrive

- UART: 115200 bps
- Packet size: 5 bytes
- Headers:
  - `255`: normal run
  - `254`: coast brake
  - `253`: brake
  - `252`: forced run
- The following 4 bytes are the motor outputs. The MainCPU converts `-100..100` to `0..200` before sending.

### MainCPU to Line

- UART: 115200 bps
- Frame markers:
  - start: `254`
  - end: `253`
  - escape: `252`
- Frames use an XOR checksum and are used to set thresholds and switch between normal and debug modes.

## Distance Estimation

`MainCPU/src/distance_predict/` contains a polynomial regression model for Teensy 4.1.  
It is used to estimate distance from camera-detected coordinates and blob height.

`MainCPU/pythonTools/split_distance_csv.py` is a helper script that splits `SD/distance.csv` into multiple files by x-coordinate range.

```powershell
python MainCPU/pythonTools/split_distance_csv.py
```

To specify the number of output splits:

```powershell
python MainCPU/pythonTools/split_distance_csv.py 10
```

## Operation Notes

On the MainCPU side, switches and toggles are used to change modes.

- `toggle3`: switch between debug and match operation
- `toggle1`: pause / start operation
- `switch2`: switch kicker / dribbler-related mode
- `switch3`: toggle debug mode
- `switch4`: register the current heading as the goal direction
- `toggle2`: select blue goal / yellow goal

Check the pin definitions in `MainCPU/01_include-variable.ino` for the actual assignments.

## Notes

- This code is highly dependent on the actual robot hardware.
- Camera color thresholds are strongly affected by venue lighting.
- BNO055 calibration results are saved to the SD card.
