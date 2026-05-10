#include "logger.h"

#include <cstring>
#include <cstdio>

#include "at24c32.h"
#include "esp_log.h"

static const char *TAG = "logger";

static logger_state_t g_logger_state = {};

esp_err_t logger_init(void)
{
    // Read logger state from first 4 bytes of EEPROM
    uint8_t state_buf[4] = {0};
    esp_err_t err = at24c32_read(EEPROM_START, state_buf, sizeof(state_buf));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read logger state, initializing fresh");
        g_logger_state.log_count = 0;
        g_logger_state.current_index = 0;
    } else {
        g_logger_state.log_count = (state_buf[0] << 8) | state_buf[1];
        g_logger_state.current_index = (state_buf[2] << 8) | state_buf[3];

        // Validate state
        if (g_logger_state.current_index >= MAX_LOGS) {
            ESP_LOGW(TAG, "Invalid logger state, resetting");
            g_logger_state.log_count = 0;
            g_logger_state.current_index = 0;
        }
    }

    ESP_LOGI(TAG, "Logger initialized: count=%u, index=%u",
             g_logger_state.log_count, g_logger_state.current_index);

    return ESP_OK;
}

static esp_err_t logger_write_state(void)
{
    uint8_t state_buf[4] = {
        (uint8_t)((g_logger_state.log_count >> 8) & 0xFF),
        (uint8_t)(g_logger_state.log_count & 0xFF),
        (uint8_t)((g_logger_state.current_index >> 8) & 0xFF),
        (uint8_t)(g_logger_state.current_index & 0xFF),
    };

    return at24c32_write(EEPROM_START, state_buf, sizeof(state_buf));
}

esp_err_t logger_write(const char *message)
{
    if (!message) {
        return ESP_ERR_INVALID_ARG;
    }

    // Format log entry: "#LOG_NUM Message" padded to LOG_SIZE
    char log_entry[LOG_SIZE] = {0};

    // Create log number (wraps at MAX_LOGS)
    uint16_t log_num = g_logger_state.log_count % MAX_LOGS;
    int written = snprintf(log_entry, LOG_SIZE, "#%u %s", log_num, message);

    if (written < 0 || written >= LOG_SIZE) {
        ESP_LOGW(TAG, "Log entry truncated");
    }

    // Pad with null terminators
    while (written < LOG_SIZE) {
        log_entry[written++] = '\0';
    }

    // Calculate memory address: skip state (4 bytes), then find position
    uint16_t write_addr = 4 + (g_logger_state.current_index * LOG_SIZE);

    // Write to EEPROM
    esp_err_t err = at24c32_write(write_addr, (const uint8_t *)log_entry, LOG_SIZE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write log to EEPROM");
        return err;
    }

    // Update state
    g_logger_state.log_count++;
    g_logger_state.current_index = (g_logger_state.current_index + 1) % MAX_LOGS;

    // Write updated state back
    err = logger_write_state();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write logger state");
    }

    return ESP_OK;
}

esp_err_t logger_read(uint16_t index, char *log_buffer)
{
    if (!log_buffer || index >= MAX_LOGS) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t read_addr = 4 + (index * LOG_SIZE);

    return at24c32_read(read_addr, (uint8_t *)log_buffer, LOG_SIZE);
}

esp_err_t logger_read_last(char *log_buffer)
{
    if (!log_buffer || g_logger_state.log_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    // Last log is at (current_index - 1), with wrap-around
    uint16_t last_index = (g_logger_state.current_index > 0) ?
                          (g_logger_state.current_index - 1) :
                          (MAX_LOGS - 1);

    return logger_read(last_index, log_buffer);
}

uint16_t logger_get_count(void)
{
    return g_logger_state.log_count;
}
