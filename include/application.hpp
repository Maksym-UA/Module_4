#pragma once

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_err.h"

class Application
{
public:
    void start();

private:
    static constexpr gpio_num_t kButtonPin = GPIO_NUM_0;
    static constexpr gpio_num_t kI2cSdaPin = GPIO_NUM_8;
    static constexpr gpio_num_t kI2cSclPin = GPIO_NUM_9;

    esp_err_t initI2c();
    esp_err_t initButton();
    esp_err_t initModules();
    void run();

    i2c_master_bus_handle_t i2cBusHandle_ = nullptr;
};