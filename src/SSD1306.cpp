#include "SSD1306.hpp"

#include <algorithm>
#include <array>

namespace {
constexpr uint8_t kControlCommand = 0x00;
constexpr uint8_t kControlData = 0x40;

// SSD1306 commands
constexpr uint8_t kDisplayOff = 0xAE;
constexpr uint8_t kDisplayOn = 0xAF;
constexpr uint8_t kSetDisplayClockDiv = 0xD5;
constexpr uint8_t kSetMultiplex = 0xA8;
constexpr uint8_t kSetDisplayOffset = 0xD3;
constexpr uint8_t kSetStartLine = 0x40;
constexpr uint8_t kChargePump = 0x8D;
constexpr uint8_t kMemoryMode = 0x20;
constexpr uint8_t kSegRemap = 0xA1;
constexpr uint8_t kComScanDec = 0xC8;
constexpr uint8_t kSetComPins = 0xDA;
constexpr uint8_t kSetContrast = 0x81;
constexpr uint8_t kSetPrecharge = 0xD9;
constexpr uint8_t kSetVComDetect = 0xDB;
constexpr uint8_t kResumeToRAM = 0xA4;
constexpr uint8_t kNormalDisplay = 0xA6;
constexpr uint8_t kColumnAddr = 0x21;
constexpr uint8_t kPageAddr = 0x22;
} // namespace

SSD1306::SSD1306(I2CWriteFn writeFn, uint8_t address)
    : writeFn_(std::move(writeFn)), address_(address) {}

bool SSD1306::init() {
    if (!writeFn_) {
        return false;
    }

    const uint8_t initSeq[] = {
        kDisplayOff,
        kSetDisplayClockDiv, 0x80,
        kSetMultiplex, 0x3F,         // 64 lines
        kSetDisplayOffset, 0x00,
        static_cast<uint8_t>(kSetStartLine | 0x00),
        kChargePump, 0x14,           // enable pump for internal VCC
        kMemoryMode, 0x00,           // horizontal addressing mode
        kSegRemap,                   // column address 127 mapped to SEG0
        kComScanDec,                 // remapped mode
        kSetComPins, 0x12,
        kSetContrast, 0xCF,
        kSetPrecharge, 0xF1,
        kSetVComDetect, 0x40,
        kResumeToRAM,
        kNormalDisplay,
        kDisplayOn
    };

    if (!sendCommands(initSeq, sizeof(initSeq))) {
        return false;
    }

    clear(false);
    return update();
}

bool SSD1306::setContrast(uint8_t contrast) {
    const uint8_t seq[] = {kSetContrast, contrast};
    return sendCommands(seq, sizeof(seq));
}

void SSD1306::clear(bool color) {
    std::fill(buffer_.begin(), buffer_.end(), color ? 0xFF : 0x00);
}

void SSD1306::setPixel(uint8_t x, uint8_t y, bool color) {
    if (x >= kWidth || y >= kHeight) {
        return;
    }

    const uint16_t index = static_cast<uint16_t>(x) + static_cast<uint16_t>((y / 8) * kWidth);
    const uint8_t mask = static_cast<uint8_t>(1U << (y % 8));

    if (color) {
        buffer_[index] |= mask;
    } else {
        buffer_[index] &= static_cast<uint8_t>(~mask);
    }
}

void SSD1306::drawHLine(uint8_t x, uint8_t y, uint8_t w, bool color) {
    if (y >= kHeight || x >= kWidth || w == 0) {
        return;
    }

    const uint16_t end = std::min<uint16_t>(static_cast<uint16_t>(x) + w, kWidth);
    for (uint16_t xx = x; xx < end; ++xx) {
        setPixel(static_cast<uint8_t>(xx), y, color);
    }
}

void SSD1306::drawVLine(uint8_t x, uint8_t y, uint8_t h, bool color) {
    if (x >= kWidth || y >= kHeight || h == 0) {
        return;
    }

    const uint16_t end = std::min<uint16_t>(static_cast<uint16_t>(y) + h, kHeight);
    for (uint16_t yy = y; yy < end; ++yy) {
        setPixel(x, static_cast<uint8_t>(yy), color);
    }
}

void SSD1306::drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color) {
    if (w == 0 || h == 0) {
        return;
    }

    drawHLine(x, y, w, color);
    drawHLine(x, static_cast<uint8_t>(y + h - 1), w, color);
    drawVLine(x, y, h, color);
    drawVLine(static_cast<uint8_t>(x + w - 1), y, h, color);
}

bool SSD1306::update() {
    const uint8_t addrCmd[] = {
        kColumnAddr, 0x00, static_cast<uint8_t>(kWidth - 1),
        kPageAddr,   0x00, static_cast<uint8_t>(kPages - 1)
    };

    if (!sendCommands(addrCmd, sizeof(addrCmd))) {
        return false;
    }

    return sendData(buffer_.data(), buffer_.size());
}

bool SSD1306::sendCommand(uint8_t cmd) {
    const uint8_t packet[] = {kControlCommand, cmd};
    return writeFn_ && writeFn_(address_, packet, sizeof(packet));
}

bool SSD1306::sendCommands(const uint8_t* cmds, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        if (!sendCommand(cmds[i])) {
            return false;
        }
    }
    return true;
}

bool SSD1306::sendData(const uint8_t* data, std::size_t len) {
    if (!writeFn_) {
        return false;
    }

    // Send in small chunks: [0x40][data...]
    std::array<uint8_t, 17> packet{}; // 1 control + 16 data
    packet[0] = kControlData;

    std::size_t offset = 0;
    while (offset < len) {
        const std::size_t chunk = std::min<std::size_t>(16, len - offset);
        for (std::size_t i = 0; i < chunk; ++i) {
            packet[1 + i] = data[offset + i];
        }

        if (!writeFn_(address_, packet.data(), 1 + chunk)) {
            return false;
        }
        offset += chunk;
    }

    return true;
}