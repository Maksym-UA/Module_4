#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "DS1307clock.hpp"

namespace oled_app {

class SSD1306Display {
public:
    SSD1306Display() : u8g2_(U8G2_R0, U8X8_PIN_NONE) {}

    void begin() {
        u8g2_.begin();
        u8g2_.enableUTF8Print();
        u8g2_.setI2CAddress(OLED_ADDR << 1);
        u8g2_.setBusClock(100000);
    }

    void showStartupMessage(const char* message) {
        u8g2_.clearBuffer();
        u8g2_.setFont(u8g2_font_6x12_tr);
        u8g2_.drawStr(2, 12, message);
        u8g2_.sendBuffer();
    }

    void showError(const char* message) {
        u8g2_.clearBuffer();
        u8g2_.setFont(u8g2_font_6x12_tr);
        u8g2_.drawStr(2, 20, message);
        u8g2_.sendBuffer();
    }

    void showDateTime(const clock_app::DateTime& dateTime) {
        char timeText[9];
        char dateText[20];

        snprintf(
            timeText,
            sizeof(timeText),
            "%02u:%02u:%02u",
            dateTime.hour,
            dateTime.minute,
            dateTime.second);

        snprintf(
            dateText,
            sizeof(dateText),
            "%s %02u.%02u.%04u",
            clock_app::DS1307clock::dayToShortName(dateTime.dayOfWeek),
            dateTime.dayOfMonth,
            dateTime.month,
            dateTime.year);

        u8g2_.clearBuffer();
        u8g2_.setFont(u8g2_font_6x12_tr);
        u8g2_.drawStr(2, 12, dateText);
        u8g2_.setFont(u8g2_font_logisoso20_tn);
        u8g2_.drawStr(2, 50, timeText);
        u8g2_.sendBuffer();
    }

private:
    static constexpr uint8_t OLED_ADDR = 0x3C;
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2_;
};

} // namespace oled_app
