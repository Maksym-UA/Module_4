#include <cstring>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "at24c32.h"
#include "logger.h"

static const char *TAG = "main";

namespace {
constexpr gpio_num_t kButtonPin = GPIO_NUM_0;
constexpr gpio_num_t kI2cSdaPin = GPIO_NUM_8;
constexpr gpio_num_t kI2cSclPin = GPIO_NUM_9;

i2c_master_bus_handle_t g_i2c_bus_handle = nullptr;

esp_err_t I2cInit()
{
    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port = I2C_NUM_0;
    bus_cfg.sda_io_num = kI2cSdaPin;
    bus_cfg.scl_io_num = kI2cSclPin;
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;

    return i2c_new_master_bus(&bus_cfg, &g_i2c_bus_handle);
}

esp_err_t ButtonInit()
{
    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << kButtonPin;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_DISABLE;
    return gpio_config(&io);
}
} // namespace

extern "C" void app_main(void)
{
    esp_err_t err = I2cInit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(err));
        return;
    }

    err = at24c32_init(g_i2c_bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AT24C32 init failed: %s", esp_err_to_name(err));
        return;
    }

    err = logger_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Logger init failed: %s", esp_err_to_name(err));
        return;
    }

    err = ButtonInit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Button init failed: %s", esp_err_to_name(err));
        return;
    }

    logger_write("System startup");

    while (true) {
        if (gpio_get_level(kButtonPin) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            if (gpio_get_level(kButtonPin) == 0) {
                logger_dump_to_uart();
                while (gpio_get_level(kButtonPin) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}