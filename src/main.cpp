#include <Arduino.h>
#include <Wire.h>

#include "DS1307clock.hpp"
#include "SSD1306Display.hpp"

namespace {
constexpr uint8_t kOledSdaPin = 8;
constexpr uint8_t kOledSclPin = 9;
constexpr uint16_t kStartupMessageMs = 2000;
}  // namespace

clock_app::DS1307clock rtc;
clock_app::DateTime dateTime;
oled_app::SSD1306Display display;


void setup() {
    Serial.begin(115200);
    Wire.begin(kOledSdaPin, kOledSclPin);
    Wire.setClock(100000);

    display.begin();
    display.showStartupMessage("OLED initialized");

    delay(kStartupMessageMs);

    Serial.println("System initialized!");
}


void loop() {
    if (!rtc.readDateTime(dateTime)) {
        Serial.println("RTC read error");
        display.showError("RTC read error");
        delay(1000);
        return;
    }

    Serial.printf("%02u:%02u:%02u\n", dateTime.hour, dateTime.minute, dateTime.second);
    Serial.printf(
        "%s %02u.%02u.%04u\n",
        clock_app::DS1307clock::dayToShortName(dateTime.dayOfWeek),
        dateTime.dayOfMonth,
        dateTime.month,
        dateTime.year);

    display.showDateTime(dateTime);
    delay(1000);
}