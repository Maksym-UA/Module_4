#include <cstring>

#include "driver/i2c_master.h"
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

    i2c_master_bus_handle_t g_i2c_bus_handle = nullptr;

    esp_err_t I2cInit(void)
    {
        i2c_master_bus_config_t bus_cfg = {
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .i2c_port = I2C_NUM_0,
            .scl_io_num = kI2cSclPin,
            .sda_io_num = kI2cSdaPin,
            .glitch_ignore_cnt = 7,
            .flags = {
                .enable_internal_pullup = true,
            },
        };

        esp_err_t err = i2c_new_master_bus(&bus_cfg, &g_i2c_bus_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create I2C master bus");
            return err;
        }

        ESP_LOGI(TAG, "I2C master bus initialized");
        return ESP_OK;
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

    // Initialize AT24C32 on I2C bus
    err = at24c32_init(g_i2c_bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AT24C32 init failed: %s", esp_err_to_name(err));
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

