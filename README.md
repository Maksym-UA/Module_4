# ESP32-S3 ADC DMA → UART DMA (ESP-IDF + PlatformIO)

Reads analog values from the ADC in continuous DMA mode and streams them to the serial console via UART using DMA (hardware FIFO). Both the acquisition and the transmission path avoid CPU-copy overhead.

## Hardware

| Component | Model / Notes |
|---|---|
| Board | ESP32-S3-DevKitC-1 (N16R8, 16 MB Flash, 8 MB OPI PSRAM) |
| Analog input | Any 0–3.3 V signal connected to GPIO 1 (ADC1 Channel 0) |

## Wiring

```
Analog signal source:
  Signal  →  GPIO 1  (ADC1_CH0)
  GND     →  GND
```

> **Note:** GPIO 1 is ADC1 Channel 0 on the ESP32-S3. Do not exceed 3.3 V on this pin.

## How it works

1. **ADC continuous + DMA** — the ADC peripheral samples at 20 kHz and pushes results into an internal DMA ring buffer via `adc_continuous_start()`.
2. **Conversion done callback** — an ISR callback (`adc_on_conv_done_cb`) fires each time a conversion frame is ready in the buffer.
3. **Main task reads** — `adc_continuous_read()` drains the DMA buffer into a local `result[]` array and iterates over each `adc_digi_output_data_t` sample (TYPE2 format for ESP32-S3).
4. **UART DMA output** — each raw ADC value is formatted and sent with `uart_write_bytes()`, which uses the UART hardware FIFO backed by DMA to keep the CPU free.
5. **Statistics** — every second the effective sample rate and total sample count are printed via `ESP_LOGI`.

## Output format

Each sample line sent over UART:

```
CH0: 2048
CH0: 2051
CH0: 2044
...
I (1000) ADC_DMA_UART: Samples/sec: 1230, Total: 1230
```

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

## Configuration

Key constants in `src/main.cpp`:

| Constant | Default | Description |
|---|---|---|
| `ADC_CHAN` | `ADC_CHANNEL_0` | ADC channel (GPIO 1 on ESP32-S3) |
| `ADC_SAMPLE_RATE_HZ` | `20000` | ADC sampling frequency in Hz |
| `ADC_BUFFER_SIZE` | `256` | DMA conversion frame size in bytes |
| `UART_BAUD_RATE` | `115200` | UART baud rate |
| `PRINT_INTERVAL_MS` | `1000` | Statistics print interval in ms |

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
