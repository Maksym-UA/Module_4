#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

// Logger constants
#define LOG_SIZE            32      // Each log entry is 32 bytes
#define LOG_FORMAT_PREFIX   "#"     // Log begins with "#NUM"
#define MAX_LOGS            1024    // 32KB / 32 bytes per log
#define EEPROM_START        0x0000

typedef struct {
    uint16_t log_count;      // Total logs written (wraps at MAX_LOGS)
    uint16_t current_index;  // Current write position in ring buffer
} logger_state_t;

/// Initialize the logger - reads state from EEPROM and sets up ring buffer
esp_err_t logger_init(void);

/// Write a log entry to EEPROM with automatic ring buffer wrapping
/// @param message Log message (will be formatted as "#NUM Message\0", padded to LOG_SIZE)
/// @return ESP_OK on success
esp_err_t logger_write(const char *message);

/// Read a log entry from EEPROM at the specified index
/// @param index Log index (0 = oldest in ring, not absolute log number)
/// @param log_buffer Buffer to store log (must be at least LOG_SIZE bytes)
/// @return ESP_OK on success
esp_err_t logger_read(uint16_t index, char *log_buffer);

/// Find the last (most recent) log entry
/// @param log_buffer Buffer to store the log (must be at least LOG_SIZE bytes)
/// @return ESP_OK on success
esp_err_t logger_read_last(char *log_buffer);

/// Get total number of logs written (useful for UI display)
/// @return Number of logs
uint16_t logger_get_count(void);
