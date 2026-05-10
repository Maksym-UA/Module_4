#pragma once

#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#define AT24C32_ADDR    0x50
#define AT24C32_PAGE    32

/// Initialize AT24C32 with I2C master bus handle
esp_err_t at24c32_init(i2c_master_bus_handle_t bus_handle);
esp_err_t at24c32_write(uint16_t mem_addr, const uint8_t *data, size_t len);
esp_err_t at24c32_read(uint16_t mem_addr, uint8_t *data, size_t len);