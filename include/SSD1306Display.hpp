#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

#include "I2CScanner.hpp"

namespace app {

class SSD1306Display {
public:
    SSD1306Display() : u8g2_(U8G2_R2, U8X8_PIN_NONE) {}

    void begin() {
        u8g2_.begin();
        u8g2_.enableUTF8Print();
        sendStartupSequence();
    }

    void renderScanResults(const I2CScanResult& scanResult, uint32_t frame) {
        u8g2_.clearBuffer();
        u8g2_.setFont(u8g2_font_unifont_t_cyrillic);
        u8g2_.drawUTF8(2, 10, u8"Модуль 4.3:");
        u8g2_.setFont(u8g2_font_6x12_tr);
        u8g2_.drawStr(2, 24, "I2C scan results:");

        if (scanResult.count == 0) {
            u8g2_.drawStr(2, 38, "No I2C devices found");
        } else {
            for (uint8_t i = 0; i < scanResult.count; ++i) {
                u8g2_.setCursor(2, 38 + (i * 10));
                u8g2_.print("0x");
                if (scanResult.addresses[i] < 16) {
                    u8g2_.print('0');
                }
                u8g2_.print(scanResult.addresses[i], HEX);
            }
        }

        u8g2_.drawFrame(0, 0, 128, 64);
        u8g2_.sendBuffer();

        (void)frame;
    }

private:
    void sendStartupSequence() {
        u8g2_.sendF("c",  0xAE);
        u8g2_.sendF("ca", 0xA8, 0x3F);
        u8g2_.sendF("ca", 0xD3, 0x00);
        u8g2_.sendF("c",  0x40);
        u8g2_.sendF("ca", 0x20, 0x00);
        u8g2_.sendF("c",  0xA1);
        u8g2_.sendF("c",  0xC8);
        u8g2_.sendF("ca", 0xDA, 0x12);
        u8g2_.sendF("ca", 0x81, 0x7F);
        u8g2_.sendF("ca", 0xD9, 0x22);
        u8g2_.sendF("ca", 0xDB, 0x20);
        u8g2_.sendF("ca", 0x8D, 0x14);
        u8g2_.sendF("ca", 0xA4, 0x00);
        u8g2_.sendF("c",  0xAF);
    }

    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2_;
};

} // namespace app
