/*
- Зчитувати з BME280: температуру (°C), вологість (%RH), тиск (hPa).
- Додати до вже наявного виводу на екран (попереднє ДЗ) три нові поля: T, RH, P (наприклад: T: 23.4°C RH: 45% P: 1013 hPa).
- Оновлювати дані з BME280 не рідше 1 разу на секунду (можна окремим таймером/міткою часу), без delay().
- Дублювати всі значення показані на екрані через систему логування.
*/


#include <Arduino.h>
#include <Wire.h>

#include "DS1307clock.hpp"
#include "SSD1306Display.hpp"
#include "BME280.hpp"
#include "I2CScanner.hpp"

namespace {
constexpr uint8_t kI2cSdaPin = 8;
constexpr uint8_t kI2cSclPin = 9;
constexpr uint16_t kStartupMessageMs = 2000;
constexpr uint16_t kLoopIntervalMs = 1000;
}  // namespace

clock_app::DS1307clock rtc;
clock_app::DateTime dateTime;
oled_app::SSD1306Display display;
bme280_app::BME280 bme280;
scanner_app::I2CScanResult i2cScanResult;


void setup() {
    Serial.begin(115200);
    if (!bme280.begin(Wire, kI2cSdaPin, kI2cSclPin)) {
        Serial.println("BME280 init error");
        display.showError("BME280 init error");
    }

    display.begin();
    display.showStartupMessage("OLED initialized");

    delay(kStartupMessageMs);

    scanner_app::scanI2CDevices(Wire, Serial, i2cScanResult);

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

    bme280_app::BME280Data bme280Data;
    if (bme280.readData(bme280Data)) {
        Serial.printf(
            "T: %.1f C RH: %.1f%% P: %.1f hPa\n",
            bme280Data.temperatureC,
            bme280Data.humidityPercent,
            bme280Data.pressureHpa);
        display.showDateTime(dateTime, bme280Data);
    } else {
        Serial.println("BME280 read error");
        display.showDateTime(dateTime);
    }

    delay(kLoopIntervalMs);
}