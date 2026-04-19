# UART communication between ESP32 S3 and STM32-STM32F411CEU6-Black-Pill

Project
При натисненні кнопки на стороні ESP32 змінюється стан світлодіода на стороні STM32 (Вже зробили протягом уроку)
При натисненні кнопки на стороні STM32 змінюється стан світлодіода на стороні ESP32.

## Hardware

- Board: ESP32-S3-DevKitC-1
- Board: STM32-STM32F411CEU6
- ST-Link V2 programmer

## Wiring

```
Encoder:
  - CLK - Channel B - GIO4
  - DT - Channel A - GPIO5
  - SW - Button - GPIO6
  - VCC - 3.3V
  - GND - GND (common ground with ESP32-S3)
```

## Software requirements

- VS Code
- PlatformIO extension
- ESP-IDF toolchain (installed by PlatformIO for this environment)

## Build and run

From the project root:

```bash
pio run
```

Upload firmware:

```bash
pio run -t upload
```

Open serial monitor (115200 baud):

```bash
pio device monitor -b 115200
```

## Configuration

### Configuration notes

- Framework: `espidf`
- Monitor speed: `115200`
- Flash mode/size: `qio`, `16MB`

## Project structure

- `src/main.cpp` - ESP-IDF `app_main()` with UART1 setup, logging, and echo logic
- `platformio.ini` - board and build settings
- `sdkconfig.esp32-s3-devkitc-1` - ESP-IDF configuration


## Contact

Feedback: max.savin3@gmail.com