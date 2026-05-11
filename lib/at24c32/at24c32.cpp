#include "at24c32.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
    static const char* TAG = "at24c32";
    static i2c_master_dev_handle_t s_deviceHandle = nullptr;
}

esp_err_t at24c32_init(i2c_master_bus_handle_t bus_handle)
{
    if (bus_handle == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_deviceHandle != nullptr) {
        return ESP_OK;
    }

    i2c_device_config_t devConfig = {};
    devConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;// AT24C32 uses 7-bit addressing
    devConfig.device_address = AT24C32_ADDR;
    devConfig.scl_speed_hz = 100000;// Standard I2C speed for EEPROMs
    devConfig.scl_wait_us = 0;
    devConfig.flags.disable_ack_check = false;

    // Add the device to the I2C bus
    esp_err_t err = i2c_master_bus_add_device(bus_handle, &devConfig, &s_deviceHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add device: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "AT24C32 initialized");
    return ESP_OK;
}

// Writes data to the EEPROM starting at the specified memory address
esp_err_t at24c32_write(uint16_t mem_addr, const uint8_t* data, size_t len)
{
    if (s_deviceHandle == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    if (data == nullptr && len > 0) {
        return ESP_ERR_INVALID_ARG;
    }

    while (len > 0) {
        size_t chunkSize = AT24C32_PAGE - (mem_addr % AT24C32_PAGE);
        if (chunkSize > len) {
            chunkSize = len;
        }

        uint8_t buffer[2 + AT24C32_PAGE] = {};
        buffer[0] = static_cast<uint8_t>((mem_addr >> 8) & 0xFF);
        buffer[1] = static_cast<uint8_t>(mem_addr & 0xFF);

        for (size_t i = 0; i < chunkSize; ++i) {
            buffer[2 + i] = data[i];
        }
        // Send the memory address followed by the data chunk
        esp_err_t err = i2c_master_transmit(
            s_deviceHandle,
            buffer,
            2 + chunkSize,
            pdMS_TO_TICKS(1000));

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Write failed at address 0x%04X: %s", mem_addr, esp_err_to_name(err));
            return err;
        }

        vTaskDelay(pdMS_TO_TICKS(10));// EEPROM write cycle time

        mem_addr = static_cast<uint16_t>(mem_addr + chunkSize);
        data += chunkSize;
        len -= chunkSize;
    }

    return ESP_OK;
}

esp_err_t at24c32_read(uint16_t mem_addr, uint8_t* data, size_t len)
{
    if (s_deviceHandle == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    if (data == nullptr && len > 0) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t addrBuffer[2] = {
        static_cast<uint8_t>((mem_addr >> 8) & 0xFF),
        static_cast<uint8_t>(mem_addr & 0xFF)
    };

    esp_err_t err = i2c_master_transmit_receive(
        s_deviceHandle,
        addrBuffer,
        sizeof(addrBuffer),
        data,
        len,
        pdMS_TO_TICKS(1000));

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Read failed at address 0x%04X: %s", mem_addr, esp_err_to_name(err));
    }

    return err;
}
