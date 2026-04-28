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

#include <time.h>
#include <stdio.h>

namespace {
constexpr uint8_t kI2cSdaPin = 8;
constexpr uint8_t kI2cSclPin = 9;
constexpr uint16_t kStartupMessageMs = 2000;
constexpr uint16_t kLoopIntervalMs = 1000;
constexpr uint16_t kRtcErrorDisplayMs = 2000;
constexpr int32_t kUtcOffsetSeconds = 3 * 3600;
}  // namespace

clock_app::DS1307clock rtc;
clock_app::DateTime dateTime;
oled_app::SSD1306Display display;
bme280_app::BME280 bme280;
scanner_app::I2CScanResult i2cScanResult;
bool rtcErrorActive = false;
unsigned long rtcErrorStartedAtMs = 0;
bool rtcPresent = true;

bool isDeviceFound(const scanner_app::I2CScanResult& scanResult, uint8_t address) {
    for (uint8_t i = 0; i < scanResult.count; ++i) {
        if (scanResult.addresses[i] == address) {
            return true;
        }
    }

    return false;
}


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
    rtcPresent = isDeviceFound(i2cScanResult, clock_app::DS1307clock::kAddress);
    if (!rtcPresent) {
        Serial.println("RTC not connected, fallback mode enabled");
    }


    rtc.initSystemTimeFromBuild();

    Serial.println("System initialized!");
}


void loop() {
    const bool rtcOk = rtcPresent && rtc.readDateTime(dateTime);
    if (!rtcOk) {
        if (!rtcErrorActive) {
            rtcErrorActive = true;
            rtcErrorStartedAtMs = millis();
            Serial.println(rtcPresent ? "RTC read error" : "RTC not connected");
        }

        const unsigned long errorDurationMs = millis() - rtcErrorStartedAtMs;
        if (errorDurationMs < kRtcErrorDisplayMs) {
            display.showError("RTC read error");
        } else {
            char systemDateTime[20] = {0};
            rtc.get_datetime(systemDateTime, sizeof(systemDateTime));
            Serial.printf("System time fallback: %s\n", systemDateTime);

            bme280_app::BME280Data bme280Data;
            if (bme280.readData(bme280Data)) {
                Serial.printf(
                    "T: %.1f C RH: %.1f%% P: %.1f hPa\n",
                    bme280Data.temperatureC,
                    bme280Data.humidityPercent,
                    bme280Data.pressureHpa);
                display.showRtcFallbackTime(systemDateTime, bme280Data);
            } else {
                Serial.println("BME280 read error");
                display.showRtcFallbackTime(systemDateTime);
            }
        }
    } else {
        rtcErrorActive = false;

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
    }

    delay(kLoopIntervalMs);
}