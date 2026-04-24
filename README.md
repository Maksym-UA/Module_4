# SSD1306 OLED on ESP32-S3 (I2C, U8g2)

## Project description

This project drives a 128x64 SSD1306 OLED display from an ESP32-S3 using I2C and the U8g2 library.

Current firmware behavior:
- Initializes USB serial monitor at 115200 baud.
- Initializes I2C on custom pins (`SDA=GPIO8`, `SCL=GPIO9`).
- Scans the I2C bus and prints detected device addresses.
- Initializes the OLED (`SSD1306`, rotation `U8G2_R2`).
- Draws `"Module 4.2"` and a screen frame, updates every 500 ms.
- Prints a frame counter to serial monitor (`Display frame: ...`).

## Hardware

| Component | Details |
|---|---|
| MCU | ESP32-S3-WROOM-1 (DevKit) |
| Display | SSD1306 128x64 OLED (I2C) |

## Wiring

### I2C connection

```
ESP32-S3      SSD1306 OLED
---------     ------------
GPIO8 (SDA) → SDA
GPIO9 (SCL) → SCL
3V3         → VCC
GND         → GND
```

Notes:
- Use common ground.
- Most SSD1306 I2C modules use address `0x3C` (sometimes `0x3D`).

## Software requirements

- VS Code
- PlatformIO extension
- Arduino framework for ESP32
- U8g2 library (`olikraus/U8g2`)

## Build and run

Build:

```bash
pio run
```

Upload:

```bash
pio run -t upload
```

Monitor:

```bash
pio device monitor -b 115200
```

Expected startup logs:
- `Setup start`
- `I2C scan start...`
- `Found I2C device at 0x3C` (or `0x3D`)

## Configuration

- Framework: `arduino`
- Monitor speed: `115200`
- Build flags:
  - `ARDUINO_USB_MODE=1`
  - `ARDUINO_USB_CDC_ON_BOOT=1`
- Source filter: `build_src_filter = +<main.cpp>`

## Project structure

```
src/
  main.cpp      - OLED init, I2C scan, and display rendering loop
platformio.ini  - PlatformIO board/framework/build settings
```

## Contact

Feedback: max.savin3@gmail.com