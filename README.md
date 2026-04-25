# Module 4

Arduino/PlatformIO project for reading date and time from a **DS1307 RTC** over **I2C** and showing it on an **SSD1306 128x64 OLED** display.

## Features

- Reads full date and time from the DS1307
- Converts RTC hour data to **24-hour format**
- Shows time as `HH:MM:SS`
- Shows date as `EEE DD.MM.YYYY`
- Uses helper headers to keep `main.cpp` clean
- Uses `Wire.h` for I2C communication
- Uses `U8g2lib.h` for OLED rendering

## Hardware

- ESP32-S3 board using Arduino framework
- DS1307 RTC module
- SSD1306 128x64 OLED display
- I2C wiring

## I2C configuration

Current pins used in [src/main.cpp](src/main.cpp):

- SDA: `8`
- SCL: `9`

I2C device addresses:

- DS1307 RTC: `0x68`
- SSD1306 OLED: `0x3C`

## Output format

Serial output and OLED output use the same layout:

- Time: `15:33:59`
- Date: `Sat 14.02.2026`

## Project structure

- [src/main.cpp](src/main.cpp) — application entry point, setup, and main loop
- [include/DS1307clock.hpp](include/DS1307clock.hpp) — RTC helper, `DateTime` structure, BCD conversion, 24-hour decoding, and weekday text conversion
- [include/SSD1306Display.hpp](include/SSD1306Display.hpp) — OLED display helper for startup, error, and date/time rendering
- [include/I2CScanner.hpp](include/I2CScanner.hpp) — optional I2C scan helper

## How it works

1. `setup()` initializes Serial and I2C.
2. The OLED displays a short startup message.
3. `loop()` reads date/time from the DS1307 through `DS1307clock`.
4. The formatted result is printed to Serial and rendered on the OLED through `SSD1306Display`.

## Dependencies

Libraries used:

- `Wire`
- `U8g2`

The U8g2 dependency is already declared in [platformio.ini](platformio.ini).

## Build and upload

PlatformIO commands:

```bash
platformio run
platformio run --target upload
platformio device monitor
```

## Notes

- The DS1307 must contain valid date/time data.
- If the displayed time is wrong, the RTC likely needs to be set first.
- If the OLED output looks incorrect, verify wiring, I2C address, and display rotation settings.
