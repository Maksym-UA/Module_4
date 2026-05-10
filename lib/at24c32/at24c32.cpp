#include "at24c32.h"

#include <driver/i2c.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

esp_err_t at24c32_write(uint16_t mem_addr, const uint8_t *data, size_t len)
{
    while (len > 0) {
        size_t chunk = AT24C32_PAGE - (mem_addr % AT24C32_PAGE);
        if (chunk > len) chunk = len;

        uint8_t buf[2 + AT24C32_PAGE];
        buf[0] = (mem_addr >> 8) & 0xFF;
        buf[1] = mem_addr & 0xFF;
        for (size_t i = 0; i < chunk; i++) {
            buf[2 + i] = data[i];
        }

        esp_err_t err = i2c_master_write_to_device(
            I2C_PORT,
            AT24C32_ADDR,
            buf,
            2 + chunk,
            pdMS_TO_TICKS(1000)
        );
        if (err != ESP_OK) return err;

        vTaskDelay(pdMS_TO_TICKS(10)); // wait write cycle

        mem_addr += chunk;
        data += chunk;
        len -= chunk;
    }

    return ESP_OK;
}

esp_err_t at24c32_read(uint16_t mem_addr, uint8_t *data, size_t len)
{
    uint8_t addr_buf[2] = {
        (uint8_t)((mem_addr >> 8) & 0xFF),
        (uint8_t)(mem_addr & 0xFF)
    };

    return i2c_master_write_read_device(
        I2C_PORT,
        AT24C32_ADDR,
        addr_buf,
        2,
        data,
        len,
        pdMS_TO_TICKS(1000)
    );
}

