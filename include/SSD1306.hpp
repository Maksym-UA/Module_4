#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

class SSD1306 {
public:
    static constexpr uint8_t kWidth = 128;
    static constexpr uint8_t kHeight = 64;
    static constexpr uint8_t kPages = kHeight / 8;
    static constexpr uint16_t kBufferSize = kWidth * kPages;
    static constexpr uint8_t kDefaultAddress = 0x3C;

    using I2CWriteFn = std::function<bool(uint8_t address, const uint8_t* data, std::size_t length)>;

    explicit SSD1306(I2CWriteFn writeFn, uint8_t address = kDefaultAddress);

    bool init();
    bool update();
    bool setContrast(uint8_t contrast);

    void clear(bool color = false);
    void setPixel(uint8_t x, uint8_t y, bool color = true);

    void drawHLine(uint8_t x, uint8_t y, uint8_t w, bool color = true);
    void drawVLine(uint8_t x, uint8_t y, uint8_t h, bool color = true);
    void drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color = true);

    const std::array<uint8_t, kBufferSize>& buffer() const { return buffer_; }

private:
    bool sendCommand(uint8_t cmd);
    bool sendCommands(const uint8_t* cmds, std::size_t count);
    bool sendData(const uint8_t* data, std::size_t len);

    I2CWriteFn writeFn_;
    uint8_t address_;
    std::array<uint8_t, kBufferSize> buffer_{};
};