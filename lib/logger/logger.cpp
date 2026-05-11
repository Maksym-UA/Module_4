#include "logger.h"

#include <cstdio>
#include <cstring>

#include "at24c32.h"
#include "esp_log.h"

static const char *TAG = "logger";

namespace {
constexpr uint16_t kEepromSizeBytes = 4096;  // AT24C32 = 32 Kbit = 4 KB
constexpr uint16_t kPageSize = AT24C32_PAGE; // 32 bytes
constexpr uint16_t kMetaPageAddr = 0x0000;
constexpr uint16_t kDataBaseAddr = kPageSize; // page 0 reserved for metadata
constexpr uint16_t kMaxEntries = (kEepromSizeBytes - kDataBaseAddr) / kPageSize;
constexpr uint32_t kMagic = 0x4C4F4747; // 'LOGG'

struct LoggerMeta {
    uint32_t magic;
    uint16_t head;      // next write index
    uint16_t count;     // stored entries
    uint32_t next_seq;   // log number
};

static_assert(sizeof(LoggerMeta) <= AT24C32_PAGE, "Metadata must fit one page");

LoggerMeta g_meta{};
bool g_initialized = false;

uint16_t record_addr(uint16_t index)
{
    return static_cast<uint16_t>(kDataBaseAddr + index * kPageSize);
}

esp_err_t load_meta()
{
    uint8_t page[kPageSize] = {};
    esp_err_t err = at24c32_read(kMetaPageAddr, page, sizeof(page));
    if (err != ESP_OK) {
        return err;
    }

    std::memcpy(&g_meta, page, sizeof(g_meta));

    if (g_meta.magic != kMagic || g_meta.head >= kMaxEntries || g_meta.count > kMaxEntries) {
        g_meta.magic = kMagic;
        g_meta.head = 0;
        g_meta.count = 0;
        g_meta.next_seq = 1;
    }

    return ESP_OK;
}

esp_err_t save_meta()
{
    uint8_t page[kPageSize] = {};
    std::memcpy(page, &g_meta, sizeof(g_meta));
    return at24c32_write(kMetaPageAddr, page, sizeof(page));
}

esp_err_t read_record_by_index(uint16_t index, char *out, size_t out_len)
{
    if (!out || out_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t page[kPageSize] = {};
    esp_err_t err = at24c32_read(record_addr(index), page, sizeof(page));
    if (err != ESP_OK) {
        return err;
    }

    std::snprintf(out, out_len, "%s", reinterpret_cast<char *>(page));
    return ESP_OK;
}

bool parse_log_no(const char *text, uint32_t *log_no)
{
    if (!text || text[0] != '#') {
        return false;
    }

    unsigned long value = 0;
    if (std::sscanf(text, "#%lu", &value) != 1) {
        return false;
    }

    if (log_no) {
        *log_no = static_cast<uint32_t>(value);
    }
    return true;
}
} // namespace

esp_err_t logger_init(void)
{
    if (g_initialized) {
        return ESP_OK;
    }

    esp_err_t err = load_meta();
    if (err != ESP_OK) {
        return err;
    }

    if (g_meta.magic != kMagic) {
        g_meta.magic = kMagic;
        g_meta.head = 0;
        g_meta.count = 0;
        g_meta.next_seq = 1;
        err = save_meta();
        if (err != ESP_OK) {
            return err;
        }
    }

    g_initialized = true;
    ESP_LOGI(TAG, "Logger initialized, max entries: %u", static_cast<unsigned>(kMaxEntries));
    return ESP_OK;
}

esp_err_t logger_write(const char *msg)
{
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!msg) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t page[kPageSize] = {};
    const int written = std::snprintf(
        reinterpret_cast<char *>(page),
        sizeof(page),
        "#%lu %s",
        static_cast<unsigned long>(g_meta.next_seq),
        msg
    );

    if (written < 0) {
        return ESP_FAIL;
    }
    if (written >= static_cast<int>(sizeof(page))) {
        return ESP_ERR_INVALID_SIZE;
    }

    esp_err_t err = at24c32_write(record_addr(g_meta.head), page, sizeof(page));
    if (err != ESP_OK) {
        return err;
    }

    g_meta.head = static_cast<uint16_t>((g_meta.head + 1) % kMaxEntries);
    if (g_meta.count < kMaxEntries) {
        ++g_meta.count;
    }
    ++g_meta.next_seq;

    return save_meta();
}

esp_err_t logger_read_last(char *out, size_t out_len)
{
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!out || out_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (g_meta.count == 0) {
        out[0] = '\0';
        return ESP_ERR_NOT_FOUND;
    }

    const uint16_t last_index = static_cast<uint16_t>((g_meta.head + kMaxEntries - 1) % kMaxEntries);
    return read_record_by_index(last_index, out, out_len);
}

esp_err_t logger_find_last_log(uint32_t *log_no, uint16_t *page_addr)
{
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (g_meta.count == 0) {
        return ESP_ERR_NOT_FOUND;
    }

    const uint16_t last_index = static_cast<uint16_t>((g_meta.head + kMaxEntries - 1) % kMaxEntries);
    if (page_addr) {
        *page_addr = record_addr(last_index);
    }

    char line[kPageSize + 1] = {};
    esp_err_t err = read_record_by_index(last_index, line, sizeof(line));
    if (err != ESP_OK) {
        return err;
    }

    if (!parse_log_no(line, log_no)) {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

esp_err_t logger_dump_to_uart(void)
{
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "=== EEPROM LOG DUMP (newest -> oldest) ===");

    if (g_meta.count == 0) {
        ESP_LOGI(TAG, "No logs stored");
        return ESP_OK;
    }

    char line[kPageSize + 1] = {};
    for (uint16_t i = 0; i < g_meta.count; ++i) {
        const uint16_t idx = static_cast<uint16_t>((g_meta.head + kMaxEntries - 1 - i) % kMaxEntries);
        esp_err_t err = read_record_by_index(idx, line, sizeof(line));
        if (err != ESP_OK) {
            return err;
        }

        if (line[0] != '\0') {
            ESP_LOGI(TAG, "%s", line);
        }
    }

    return ESP_OK;
}
