# ESP32-S3 EEPROM Logger (ESP-IDF + PlatformIO)

This project implements a logger on ESP32-S3 using external EEPROM AT24C32 (DS1307 RTC modules) over I2C.

## Features


- Fixed-size log records: 32 bytes per log
- Ring buffer behavior (overwrite oldest logs when memory is full)
- Read last log
- Find last log number and EEPROM page address
- Dump logs to UART on button press (newest -> oldest)

## EEPROM Capacity

AT24C32 size is *2 Kbit = 4096 bytes (4 KB).

- Record size: 32 bytes (1 page)
- Metadata reserve: 1 page (32 bytes)
- Data bytes: `4096 - 32 = 4064`
- Max stored logs: `4064 / 32 = 127`

> 1024 logs × 32 bytes would require a larger EEPROM.

## Hardware

- ESP32-S3 dev board
- DS1307 RTC module with AT24C32 EEPROM
- Push button (active-low, default `GPIO_NUM_0`)
- (Optional) BME280, SSD1306

## Software

- VS Code
- PlatformIO extension
- ESP-IDF toolchain (installed by PlatformIO)

## Default Pin Configuration

Defined in `src/main.cpp`:

- I2C SDA: `GPIO_NUM_8`
- I2C SCL: `GPIO_NUM_9`
- Button: `GPIO_NUM_0` (active-low)

## Build and Flash

Build:
```bash
pio run -e esp32-s3-devkitc-1
```

Clean + build:
```bash
pio run -t clean
pio run -e esp32-s3-devkitc-1
```

Flash:
```bash
pio run -e esp32-s3-devkitc-1 -t upload
```

Serial monitor (115200):
```bash
pio device monitor -b 115200
```

## Project Structure

- `src/main.cpp` — app startup, I2C init, button handling, UART dump trigger
- `lib/at24c32/at24c32.h/.cpp` — EEPROM driver
- `lib/logger/logger.h/.cpp` — logger logic (ring buffer + metadata)
- `src/CMakeLists.txt` — ESP-IDF component registration
- `platformio.ini` — board/framework/build settings


## Contact

Feedback: `max.savin3@gmail.com`
