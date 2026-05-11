#include "at24c32.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "at24c32";

static i2c_master_dev_handle_t g_at24c32_handle = nullptr;

esp_err_t at24c32_init(i2c_master_bus_handle_t bus_handle)
{
    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = AT24C32_ADDR;
    dev_cfg.scl_speed_hz = 100000;
    dev_cfg.scl_wait_us = 0;
    dev_cfg.flags.disable_ack_check = false;

    esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &g_at24c32_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add AT24C32 device to bus");
        return err;
    }

    ESP_LOGI(TAG, "AT24C32 device initialized");
    return ESP_OK;
}

esp_err_t at24c32_write(uint16_t mem_addr, const uint8_t *data, size_t len)
{
    if (!g_at24c32_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    while (len > 0) {
        size_t chunk = AT24C32_PAGE - (mem_addr % AT24C32_PAGE);
        if (chunk > len) chunk = len;

        uint8_t buf[2 + AT24C32_PAGE];
        buf[0] = (mem_addr >> 8) & 0xFF;
        buf[1] = mem_addr & 0xFF;
        for (size_t i = 0; i < chunk; i++) {
            buf[2 + i] = data[i];
        }

        esp_err_t err = i2c_master_transmit(g_at24c32_handle, buf, 2 + chunk, pdMS_TO_TICKS(1000));
        if (err != ESP_OK) {
            return err;
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // wait write cycle

        mem_addr += chunk;
        data += chunk;
        len -= chunk;
    }

    return ESP_OK;
}

esp_err_t at24c32_read(uint16_t mem_addr, uint8_t *data, size_t len)
{
    if (!g_at24c32_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t addr_buf[2] = {
        (uint8_t)((mem_addr >> 8) & 0xFF),
        (uint8_t)(mem_addr & 0xFF)
    };

    return i2c_master_transmit_receive(g_at24c32_handle, addr_buf, 2, data, len, pdMS_TO_TICKS(1000));
}

