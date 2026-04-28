# Module 4.4

Arduino/PlatformIO project for reading date/time from a **DS1307 RTC** and environment data from a **BME280** sensor over **I2C**, displaying everything on an **SSD1306 128x64 OLED**.

## Features

- Reads full date and time from the DS1307
- Converts RTC hour data to **24-hour format**
- Reads temperature (°C), humidity (%RH), and pressure (hPa) from BME280
- Shows time as `HH:MM:SS`, date as `EEE DD.MM.YYYY`, and sensor data on a single OLED screen
- Duplicates all displayed values to Serial
- Scans the I2C bus on startup and reports found devices
- Uses helper headers to keep `main.cpp` clean
- Uses `Wire.h` for I2C communication
- Uses `U8g2lib.h` for OLED rendering

## Hardware

- ESP32-S3 board using Arduino framework
- DS1307 RTC module
- BME280 temperature/humidity/pressure sensor
- SSD1306 128x64 OLED display
- All devices share the same I2C bus

## I2C configuration

Current pins used in [src/main.cpp](src/main.cpp):

- SDA: `8`
- SCL: `9`

I2C device addresses:

- DS1307 RTC: `0x68`
- SSD1306 OLED: `0x3C`
- BME280 sensor: `0x76`

## Output format

Serial output per loop iteration:

```
15:33:59
Sat 28.04.2026
T: 23.4 C RH: 45.0% P: 1013.2 hPa
```

OLED layout (top to bottom):

```
Sat 28.04.2026
15:33:59          ← large font
T:23.4C H:45% P:1013hPa
```

## Project structure

- [src/main.cpp](src/main.cpp) — application entry point, setup, and main loop
- [include/DS1307clock.hpp](include/DS1307clock.hpp) — RTC helper, `DateTime` structure, BCD conversion, 24-hour decoding, and weekday name lookup
- [include/SSD1306Display.hpp](include/SSD1306Display.hpp) — OLED display helper for startup, error, date/time, and BME280 data rendering
- [include/BME280.hpp](include/BME280.hpp) — BME280 sensor helper with `BME280Data` struct and I2C pin-aware init
- [include/I2CScanner.hpp](include/I2CScanner.hpp) — I2C bus scanner, runs once on startup

## How it works

1. `setup()` initializes Serial, I2C (via `bme280.begin()`), OLED, and scans the I2C bus.
2. `loop()` reads date/time from DS1307 and environment data from BME280 every second.
3. All values are printed to Serial and rendered on the OLED simultaneously.
4. If RTC read fails, an error is shown on the OLED and the loop retries after 1 s.
5. If BME280 read fails, the OLED falls back to showing date/time only.

## Dependencies

Libraries used:

- `Wire`
- `U8g2`
- `Adafruit BME280 Library`
- `Adafruit Unified Sensor`

All dependencies are declared in [platformio.ini](platformio.ini).

## Build and upload

PlatformIO commands:

```bash
platformio run
platformio run --target upload
platformio device monitor
```

## Notes

- The DS1307 must contain valid date/time data before use; if time shows `00:00:00`, the RTC needs to be set.
- If the OLED output looks incorrect, verify wiring, I2C address, and display rotation settings.
- BME280 default I2C address is `0x76`; some modules use `0x77` — update `kDefaultAddress` in [include/BME280.hpp](include/BME280.hpp) if needed.
