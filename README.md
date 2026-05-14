# ESP32-S3 Wi-Fi + MQTT Control Demo

ESP32-S3 project based on **ESP-IDF** (built with **PlatformIO**).
The device connects to Wi-Fi (STA mode), connects to an MQTT broker, subscribes to a command topic, and controls an LED while publishing periodic heartbeat messages.

## Features

- Wi-Fi station initialization with up to 10 reconnect retries
- Blocks in `wifi_init_sta()` until connected or all retries exhausted
- MQTT client with event-driven handling
- Subscribes to command topic on connect
- Processes `ON` / `OFF` / `STATUS` commands to control GPIO and publish replies
- Publishes a periodic heartbeat every 10 seconds
- Bounded buffer copy in MQTT event handler to prevent overflow

## MQTT Topics

Defined in [`lib/mqtt/mqtt.h`](lib/mqtt/mqtt.h):

| Constant        | Value              | Direction           |
|-----------------|--------------------|---------------------|
| `MQTT_TOPIC`    | `esp32s3/test`     | Publish (heartbeat) |
| `MQTT_COMMANDS` | `esp32s3/commands` | Subscribe           |
| `MQTT_STATUS`   | `esp32s3/status`   | Publish (reply)     |

Default broker: `mqtt://broker.hivemq.com:1883`

## Supported Commands

Send to `esp32s3/commands`:

| Command  | Action                                          |
|----------|-------------------------------------------------|
| `ON`     | Set LED high (`GPIO_NUM_16`)                    |
| `OFF`    | Set LED low (`GPIO_NUM_16`)                     |
| `STATUS` | Publish `"ESP32-S3 is running"` to status topic |

## Pin Usage

| Signal | GPIO          |
|--------|---------------|
| LED    | `GPIO_NUM_16` |

## Project Structure

```
src/
  main.cpp          — app_main: NVS init, LED GPIO, Wi-Fi + MQTT startup, heartbeat loop
  application.cpp   — (reserved)
lib/
  wifi/
    wifi_setup.cpp  — Wi-Fi STA init, event handler, connection wait with retries
    wifi_setup.h
  mqtt/
    mqtt.cpp        — MQTT client init, event handler, message callback dispatch
    mqtt.h          — topic/broker constants, public API
  credentials/
    credentials.h   — WIFI_SSID / WIFI_PASSWORD defines
platformio.ini      — board: esp32-s3-devkitc-1, framework: espidf
```

## Build / Flash / Monitor

```bash
pio run -e esp32-s3-devkitc-1
pio run -e esp32-s3-devkitc-1 -t upload
pio device monitor -b 115200
```

Or use the VS Code tasks (`PlatformIO: Build/Upload/Monitor (Module_4)`).

## Configuration

1. Set Wi-Fi credentials in [`lib/credentials/credentials.h`](lib/credentials/credentials.h):
   ```cpp
   #define WIFI_SSID     "your_ssid"
   #define WIFI_PASSWORD "your_password"
   ```
2. Change broker URI or topic names in [`lib/mqtt/mqtt.h`](lib/mqtt/mqtt.h).
3. Change the publish interval via `PUBLISH_INTERVAL_MS` in [`src/main.cpp`](src/main.cpp) (default: 10 s).

## Troubleshooting

| Symptom | Likely cause |
|---------|--------------|
| `Retry WiFi connection (N/10)...` then `Connection failed` | Wrong SSID/password, or AP out of range |
| `getaddrinfo() returns 202` on MQTT connect | Wi-Fi not connected when MQTT starts — fix credentials first |
| `Losing qos0 data when client not connected` | MQTT broker unreachable; confirm internet access and broker URI |
| Monitor is silent after upload | Wrong baud rate — use `115200` |
