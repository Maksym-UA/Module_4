#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_adc/adc_continuous.h"
#include "esp_log.h"
#include "esp_timer.h"

#define UART_PORT               UART_NUM_0
#define ADC_UNIT                ADC_UNIT_1
#define ADC_CHAN                ADC_CHANNEL_0         // GPIO1 on ESP32-S3
#define ADC_SAMPLE_RATE_HZ      20000                 // 20 kHz
#define ADC_BUFFER_SIZE         256                   // Conversion frame size in bytes
#define UART_BAUD_RATE          115200
#define PRINT_INTERVAL_MS       1000                  // Print stats every 1 second

static const char *TAG = "ADC_DMA_UART";

// ADC continuous handle
adc_continuous_handle_t adc_handle = NULL;

// Callback function for ADC
static bool IRAM_ATTR adc_on_conv_done_cb(adc_continuous_handle_t handle,
                                           const adc_continuous_evt_data_t *edata,
                                           void *user_data) {
    // Data is ready in internal buffer, will be read by main task
    return false;
}

static void init_uart(void) {
    uart_config_t uart_cfg{};
    uart_cfg.baud_rate = UART_BAUD_RATE;
    uart_cfg.data_bits = UART_DATA_8_BITS;
    uart_cfg.parity = UART_PARITY_DISABLE;
    uart_cfg.stop_bits = UART_STOP_BITS_1;
    uart_cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_cfg.source_clk = UART_SCLK_DEFAULT;

    uart_driver_install(UART_PORT, 1024, 1024, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_cfg);
    uart_set_pin(UART_PORT, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG, "UART initialized at %d baud", UART_BAUD_RATE);
}

static void init_adc(void) {
    // ADC continuous handle config
    adc_continuous_handle_cfg_t adc_config{};
    adc_config.max_store_buf_size = ADC_BUFFER_SIZE * 2;
    adc_config.conv_frame_size = ADC_BUFFER_SIZE;
    adc_config.flags.flush_pool = 0;
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &adc_handle));

    // ADC channel pattern
    adc_digi_pattern_config_t pattern{};
    pattern.atten = ADC_ATTEN_DB_12;
    pattern.channel = ADC_CHAN;
    pattern.unit = ADC_UNIT;
    pattern.bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

    // ADC continuous config
    adc_continuous_config_t dig_cfg{};
    dig_cfg.pattern_num = 1;
    dig_cfg.adc_pattern = &pattern;
    dig_cfg.sample_freq_hz = ADC_SAMPLE_RATE_HZ;
    dig_cfg.conv_mode = ADC_CONV_SINGLE_UNIT_1;
    dig_cfg.format = ADC_DIGI_OUTPUT_FORMAT_TYPE2;

    ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &dig_cfg));

    // Register callback
    adc_continuous_evt_cbs_t cbs{};
    cbs.on_conv_done = adc_on_conv_done_cb;
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc_handle, &cbs, NULL));

    ESP_LOGI(TAG, "ADC initialized at %d Hz, channel=%d", ADC_SAMPLE_RATE_HZ, ADC_CHAN);
}

extern "C" void app_main() {
    // Initialize UART and ADC with DMA
    init_uart();
    init_adc();

    // Start ADC continuous mode
    ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
    ESP_LOGI(TAG, "ADC DMA started, reading values...");

    uint8_t result[ADC_BUFFER_SIZE];
    uint32_t ret_num = 0;
    uint64_t sample_count = 0;
    int64_t print_timer = esp_timer_get_time();

    while (1) {
        // Read ADC data from DMA buffer (non-blocking with timeout)
        esp_err_t ret = adc_continuous_read(adc_handle, result, ADC_BUFFER_SIZE, &ret_num, pdMS_TO_TICKS(100));

        if (ret == ESP_OK && ret_num > 0) {
            // Process ADC results
            for (uint32_t i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
                if (i + SOC_ADC_DIGI_RESULT_BYTES <= ret_num) {
                    adc_digi_output_data_t *p = (adc_digi_output_data_t *)&result[i];
                    uint32_t channel = p->type2.channel;
                    uint32_t data = p->type2.data;

                    sample_count++;

                    // Print raw ADC value to UART console using DMA (uart_write_bytes uses FIFO/DMA)
                    char buf[32];
                    int len = snprintf(buf, sizeof(buf), "CH%" PRIu32 ": %04" PRIu32 "\n", channel, data);
                    uart_write_bytes(UART_PORT, (const char *)buf, len);
                }
            }

            // Print statistics every PRINT_INTERVAL_MS
            int64_t now = esp_timer_get_time();
            if ((now - print_timer) >= (PRINT_INTERVAL_MS * 1000)) {
                uint32_t sample_rate = (uint32_t)((sample_count * 1000000) / (now - print_timer));
                ESP_LOGI(TAG, "Samples/sec: %u, Total: %llu", sample_rate, sample_count);
                print_timer = now;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}