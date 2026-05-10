#pragma once

#include <stddef.h>
#include <stdint.h>

#include "driver/i2c.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "hal/i2c_types.h"

#define I2C_PORT        I2C_NUM_0
#define I2C_SDA         8
#define I2C_SCL         9
#define I2C_FREQ_HZ     100000

#define AT24C32_ADDR    0x50
#define AT24C32_PAGE    32

esp_err_t at24c32_write(uint16_t mem_addr, const uint8_t *data, size_t len);
esp_err_t at24c32_read(uint16_t mem_addr, uint8_t *data, size_t len);