#include "application.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "at24c32.h"
#include "logger.h"

namespace {
    static const char* TAG = "Application";
}

esp_err_t Application::initI2c()
{
    i2c_master_bus_config_t busConfig = {};
    busConfig.i2c_port = I2C_NUM_0;
    busConfig.sda_io_num = kSdaPin;
    busConfig.scl_io_num = kSclPin;
    busConfig.clk_source = I2C_CLK_SRC_DEFAULT;
    busConfig.glitch_ignore_cnt = 7;
    busConfig.flags.enable_internal_pullup = true;

    return i2c_new_master_bus(&busConfig, &i2cBusHandle_);
}

esp_err_t Application::initButton()
{
    gpio_config_t ioConfig = {};
    ioConfig.pin_bit_mask = 1ULL << kButtonPin;
    ioConfig.mode = GPIO_MODE_INPUT;
    ioConfig.pull_up_en = GPIO_PULLUP_ENABLE;
    ioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ioConfig.intr_type = GPIO_INTR_DISABLE; // No interrupts needed for polling

    return gpio_config(&ioConfig);
}

// Initializes the AT24C32 EEPROM and the logger module
esp_err_t Application::initModules()
{
    esp_err_t err = at24c32_init(i2cBusHandle_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AT24C32 init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = logger_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Logger init failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

void Application::run()
{
    logger_write("System is starting up...");

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

void Application::start()
{
    esp_err_t err = initI2c();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(err));
        return;
    }

    err = initButton();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Button init failed: %s", esp_err_to_name(err));
        return;
    }

    err = initModules();
    if (err != ESP_OK) {
        return;
    }

    ESP_LOGI(TAG, "Application started");
    run();
}
