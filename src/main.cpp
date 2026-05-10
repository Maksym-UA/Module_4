#include <cstring>

#include "driver/i2c.h"
#include "esp_log.h"

#include "at24c32.h"
#include "logger.h"

static const char *TAG = "main";

namespace {
    constexpr uint8_t kI2cSdaPin = 8;
    constexpr uint8_t kI2cSclPin = 9;
    constexpr uint32_t kI2cFreqHz = 100000;
    constexpr uint16_t kStartupMessageMs = 2000;
    constexpr uint16_t kLoopIntervalMs = 1000;
    constexpr uint16_t kRtcErrorDisplayMs = 2000;
    constexpr uint8_t kRtcFailureThreshold = 3;
    constexpr uint8_t kRtcReadRetries = 3;
    constexpr unsigned long kRtcDataStaleMs = 30000UL; // 30 seconds, after which RTC fallback is considered stale and not used
    constexpr unsigned long kBmeDataStaleMs = 10000UL; // 10 seconds, after which BME280 fallback is considered stale and not used

    esp_err_t I2cInit(void)
    {
        i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = kI2cSdaPin,
            .scl_io_num = kI2cSclPin,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master = {
                .clk_speed = kI2cFreqHz,
            },
            .clk_flags = 0,
        };

        esp_err_t err = i2c_param_config(I2C_NUM_0, &conf);
        if (err != ESP_OK) {
            return err;
        }

        return i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
    }
}

extern "C" void app_main(void)
{
    // Initialize I2C bus
    esp_err_t err = I2cInit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(err));
        return;
    }

    // Initialize logger system
    err = logger_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Logger init failed: %s", esp_err_to_name(err));
        return;
    }

    // Test AT24C32 and logger
    const char test_msg[] = "AT24C32 OK";
    uint8_t read_back[sizeof(test_msg)] = {0};

    err = at24c32_write(0x1000, reinterpret_cast<const uint8_t *>(test_msg), sizeof(test_msg));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AT24C32 write failed: %s", esp_err_to_name(err));
        return;
    }

    err = at24c32_read(0x1000, read_back, sizeof(read_back));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AT24C32 read failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "AT24C32 test: %s", reinterpret_cast<char *>(read_back));

    // Write a test log entry
    err = logger_write("System startup");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Logger write failed: %s", esp_err_to_name(err));
        return;
    }

    // Read and display the last log
    char log_buffer[32] = {0};
    err = logger_read_last(log_buffer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Logger read failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Last log: %s", log_buffer);
}

