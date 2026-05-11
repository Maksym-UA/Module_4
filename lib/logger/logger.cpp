#include "logger.h"

#include <cstdio>
#include <cstring>

#include "at24c32.h"
#include "esp_log.h"

namespace {
static const char* TAG = "logger";

constexpr uint16_t kEepromSizeBytes = 4096;
constexpr uint16_t kPageSize = AT24C32_PAGE;
constexpr uint16_t kMetaPageAddr = 0x0000;
constexpr uint16_t kDataBaseAddr = kPageSize;
constexpr uint16_t kMaxEntries = (kEepromSizeBytes - kDataBaseAddr) / kPageSize;
constexpr uint32_t kMagic = 0x4C4F4747;

struct LoggerMeta {
    uint32_t magic;
    uint16_t head;
    uint16_t count;
    uint32_t next_seq;
};

static_assert(sizeof(LoggerMeta) <= AT24C32_PAGE, "Metadata must fit into one EEPROM page");

LoggerMeta s_meta{};
bool s_initialized = false;

uint16_t recordAddress(uint16_t index)
{
    return static_cast<uint16_t>(kDataBaseAddr + index * kPageSize);
}

void resetMetadata()
{
    s_meta.magic = kMagic;
    s_meta.head = 0;
    s_meta.count = 0;
    s_meta.next_seq = 1;
}

esp_err_t loadMetadata()
{
    uint8_t page[kPageSize] = {};
    esp_err_t err = at24c32_read(kMetaPageAddr, page, sizeof(page));
    if (err != ESP_OK) {
        return err;
    }

    std::memcpy(&s_meta, page, sizeof(s_meta));

    if (s_meta.magic != kMagic || s_meta.head >= kMaxEntries || s_meta.count > kMaxEntries) {
        resetMetadata();
    }

    return ESP_OK;
}

esp_err_t saveMetadata()
{
    uint8_t page[kPageSize] = {};
    std::memcpy(page, &s_meta, sizeof(s_meta));
    return at24c32_write(kMetaPageAddr, page, sizeof(page));
}

esp_err_t readRecordByIndex(uint16_t index, char* out, size_t out_len)
{
    if (out == nullptr || out_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t page[kPageSize] = {};
    esp_err_t err = at24c32_read(recordAddress(index), page, sizeof(page));
    if (err != ESP_OK) {
        return err;
    }

    std::snprintf(out, out_len, "%s", reinterpret_cast<char*>(page));
    return ESP_OK;
}

bool parseLogNumber(const char* text, uint32_t* log_no)
{
    if (text == nullptr || text[0] != '#') {
        return false;
    }

    unsigned long value = 0;
    if (std::sscanf(text, "#%lu", &value) != 1) {
        return false;
    }

    if (log_no != nullptr) {
        *log_no = static_cast<uint32_t>(value);
    }

    return true;
}
} // namespace

esp_err_t logger_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    esp_err_t err = loadMetadata();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load metadata: %s", esp_err_to_name(err));
        return err;
    }

    if (s_meta.magic != kMagic) {
        resetMetadata();
        err = saveMetadata();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to save metadata: %s", esp_err_to_name(err));
            return err;
        }
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Logger initialized, max entries: %u", static_cast<unsigned>(kMaxEntries));
    return ESP_OK;
}

esp_err_t logger_write(const char* msg)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (msg == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t page[kPageSize] = {};
    const int written = std::snprintf(
        reinterpret_cast<char*>(page),
        sizeof(page),
        "#%lu %s",
        static_cast<unsigned long>(s_meta.next_seq),
        msg);

    if (written < 0) {
        return ESP_FAIL;
    }
    if (written >= static_cast<int>(sizeof(page))) {
        return ESP_ERR_INVALID_SIZE;
    }

    esp_err_t err = at24c32_write(recordAddress(s_meta.head), page, sizeof(page));
    if (err != ESP_OK) {
        return err;
    }

    s_meta.head = static_cast<uint16_t>((s_meta.head + 1) % kMaxEntries);
    if (s_meta.count < kMaxEntries) {
        ++s_meta.count;
    }
    ++s_meta.next_seq;

    return saveMetadata();
}

esp_err_t logger_read_last(char* out, size_t out_len)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (out == nullptr || out_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_meta.count == 0) {
        out[0] = '\0';
        return ESP_ERR_NOT_FOUND;
    }

    const uint16_t lastIndex = static_cast<uint16_t>((s_meta.head + kMaxEntries - 1) % kMaxEntries);
    return readRecordByIndex(lastIndex, out, out_len);
}

esp_err_t logger_find_last_log(uint32_t* log_no, uint16_t* page_addr)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_meta.count == 0) {
        return ESP_ERR_NOT_FOUND;
    }

    const uint16_t lastIndex = static_cast<uint16_t>((s_meta.head + kMaxEntries - 1) % kMaxEntries);

    if (page_addr != nullptr) {
        *page_addr = recordAddress(lastIndex);
    }

    char line[kPageSize + 1] = {};
    esp_err_t err = readRecordByIndex(lastIndex, line, sizeof(line));
    if (err != ESP_OK) {
        return err;
    }

    if (!parseLogNumber(line, log_no)) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

esp_err_t logger_dump_to_uart(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "=== EEPROM LOG DUMP (newest -> oldest) ===");

    if (s_meta.count == 0) {
        ESP_LOGI(TAG, "No logs stored");
        return ESP_OK;
    }

    char line[kPageSize + 1] = {};
    for (uint16_t i = 0; i < s_meta.count; ++i) {
        const uint16_t index = static_cast<uint16_t>((s_meta.head + kMaxEntries - 1 - i) % kMaxEntries);

        esp_err_t err = readRecordByIndex(index, line, sizeof(line));
        if (err != ESP_OK) {
            return err;
        }

        if (line[0] != '\0') {
            ESP_LOGI(TAG, "%s", line);
        }
    }

    return ESP_OK;
}