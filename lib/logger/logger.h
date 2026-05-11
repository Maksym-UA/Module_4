#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t logger_init(void);
esp_err_t logger_write(const char* msg);
esp_err_t logger_read_last(char* out, size_t out_len);
esp_err_t logger_find_last_log(uint32_t* log_no, uint16_t* page_addr);
esp_err_t logger_dump_to_uart(void);

#ifdef __cplusplus
}
#endif