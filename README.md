# Bidirectional LED/Blink Control: ESP32-S3 ↔ STM32F411CEU6 via UART

## Project description

Two-way button-controlled LED/blink system over UART:

- Pressing the **button on ESP32-S3** sends command `'T'` to STM32 → toggles STM32 LED blinking.
- Pressing the **button on STM32** sends command `'T'` to ESP32 → toggles ESP32 onboard LED blinking (500 ms interval).

Both sides use the same single-byte command protocol: `'T'` = toggle blink state.

## Hardware

| Component | Details |
|---|---|
| MCU 1 | ESP32-S3-DevKitC-1 |
| MCU 2 | STM32F411CEU6 (Black Pill) |
| Programmer | ST-Link V2 (for STM32) |

## Wiring

### UART connection (ESP32-S3 ↔ STM32)

```
ESP32-S3    STM32F411
--------    ---------
GPIO17 TX → RX (e.g. PA10 / USART1)
GPIO18 RX ← TX (e.g. PA9  / USART1)
GND       — GND  (common ground required)
```

### ESP32-S3 pins

```
GPIO0  - BOOT button (active low, built-in pull-up)
GPIO2  - Onboard LED (output)
GPIO17 - UART1 TX → STM32 RX
GPIO18 - UART1 RX ← STM32 TX
```

## Protocol

| Sender | Byte | Effect on receiver |
|---|---|---|
| ESP32 button pressed | `'T'` (0x54) | STM32 toggles its LED blink |
| STM32 button pressed | `'T'` (0x54) | ESP32 toggles its LED blink |

- Baud rate: **115200** on both sides (8N1, no flow control)
- Debounce: 40 ms edge-triggered on ESP32 side

## Software requirements

- VS Code
- PlatformIO extension
- Arduino framework for ESP32 (installed automatically by PlatformIO)

## Build and run

Build firmware:

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

- Framework: `arduino`
- Monitor speed: `115200`
- Flash mode/size: `qio`, `16 MB`

## Project structure

```
src/
  main.cpp        - setup()/loop(): UART1 init, button polling, LED blink control
platformio.ini    - board and build settings (Arduino framework)
```

## Contact

Feedback: max.savin3@gmail.com