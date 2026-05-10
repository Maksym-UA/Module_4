# ESP32-S3 Universal logger project (ESP-IDF + PlatformIO)

Fetching and saving logs from EEPROM AT24C32 of the **DS1307 RTC** clock:
- initiation
- errors
- status.... etc

## Hardware

- ESP32-S3 board using Arduino framework
- DS1307 RTC module
- BME280 temperature/humidity/pressure sensor
- SSD1306 128x64 OLED display


## Software requirements

- VS Code
- PlatformIO extension
- ESP-IDF toolchain (installed automatically by PlatformIO)

## Build and run

Build:

```bash
pio run
```

Flash firmware:

```bash
pio run -t upload
```

Open serial monitor (115200 baud):

```bash
pio device monitor -b 115200
```


Platform settings (`platformio.ini`): framework `espidf`, flash mode `qio`, flash size `16MB`, monitor `115200` baud.

## Project structure

```
src/
  main.cpp          # ADC continuous DMA init, UART DMA output, main loop
include/
platformio.ini                    # Board and build settings
sdkconfig.esp32-s3-devkitc-1     # ESP-IDF Kconfig options
```

## Contact

Feedback: max.savin3@gmail.com
