# ESP32-S3 Wi-Fi + MQTT Control Demo

ESP32-S3 project based on **ESP-IDF** (built with **PlatformIO**).
The device connects to Wi-Fi (STA mode), connects to an MQTT broker, subscribes to command topic, and controls onboard logic (LED + status reporting).

## Features

- Wi-Fi station initialization with reconnect retries
- MQTT client start + event handling
- Subscribe to command topic and process incoming commands
- Publish periodic heartbeat messages
- Publish device status on command
- Safe MQTT topic/data copy with bounded buffers in event callback

## MQTT Topics

Defined in [`mqtt.h`](lib/mqtt/mqtt.h):

- `MQTT_TOPIC`: `esp32s3/test` (periodic publish)
- `MQTT_COMMANDS`: `esp32s3/commands` (incoming commands)
- `MQTT_STATUS`: `esp32s3/status` (status replies)

Default broker URI:

- `mqtt://broker.hivemq.com:1883`

## Supported Commands

Handled in [`handle_mqtt_message`](src/main.cpp):

- `ON` — set LED ON
- `OFF` — set LED OFF
- `STATUS` — publish `"ESP32-S3 is running"` to status topic

## Current Pin Usage

From [`main.cpp`](src/main.cpp), [`servo.cpp`](src/servo.cpp), [`buzzer.cpp`](src/buzzer.cpp), [`encoder.cpp`](src/encoder.cpp):

- LED: `GPIO_NUM_16`
- Servo PWM output: `GPIO_NUM_18`
- Buzzer PWM output: `GPIO_NUM_17`
- Encoder A: `GPIO_NUM_5`
- Encoder B: `GPIO_NUM_4`
- Encoder button: `GPIO_NUM_6`

## Project Structure

- [`src/main.cpp`](src/main.cpp) — app entry, NVS init, LED setup, Wi-Fi + MQTT startup, periodic publish
- [`lib/wifi/wifi_setup.cpp`](lib/wifi/wifi_setup.cpp) / [`lib/wifi/wifi_setup.h`](lib/wifi/wifi_setup.h) — Wi-Fi STA connection logic
- [`lib/mqtt/mqtt.cpp`](lib/mqtt/mqtt.cpp) / [`lib/mqtt/mqtt.h`](lib/mqtt/mqtt.h) — MQTT client/event handling and message callback registration
- [`src/servo.cpp`](src/servo.cpp) / [`include/servo.hpp`](include/servo.hpp) — servo control via LEDC
- [`src/buzzer.cpp`](src/buzzer.cpp) / [`include/buzzer.hpp`](include/buzzer.hpp) — buzzer beeps via LEDC
- [`src/encoder.cpp`](src/encoder.cpp) / [`include/encoder.hpp`](include/encoder.hpp) — quadrature encoder using PCNT
- [`lib/credentials/credentials.h`](lib/credentials/credentials.h) — Wi-Fi credentials
- [`platformio.ini`](platformio.ini) — PlatformIO environment configuration

## Build / Flash / Monitor

From project root:

```bash
pio run -e esp32-s3-devkitc-1
pio run -e esp32-s3-devkitc-1 -t upload
pio device monitor -b 115200
```

You can also use VS Code tasks from [`.vscode/tasks.json`](.vscode/tasks.json):

- `PlatformIO: Build (Module_4)`
- `PlatformIO: Upload (Module_4)`
- `PlatformIO: Monitor (Module_4)`

## Configuration Notes

1. Set valid Wi-Fi credentials in [`lib/credentials/credentials.h`](lib/credentials/credentials.h).
2. Verify broker/topic constants in [`lib/mqtt/mqtt.h`](lib/mqtt/mqtt.h).
3. Framework/tooling settings are in [`platformio.ini`](platformio.ini) and [`.vscode/settings.json`](.vscode/settings.json).

## Troubleshooting

- If Wi-Fi does not connect, check SSID/password and AP availability.
- If MQTT connects but no command handling occurs, verify topic matches `esp32s3/commands`.
- If upload works but monitor is silent, confirm baud rate `115200`.
