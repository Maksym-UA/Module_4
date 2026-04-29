/*
- Зчитувати з BME280: температуру (°C), вологість (%RH), тиск (hPa).
- Додати до вже наявного виводу на екран (попереднє ДЗ) три нові поля: T, RH, P (наприклад: T: 23.4°C RH: 45% P: 1013 hPa).
- Оновлювати дані з BME280 не рідше 1 разу на секунду (можна окремим таймером/міткою часу), без delay().
- Дублювати всі значення показані на екрані через систему логування.
*/


#include <Arduino.h>
#include <Wire.h>
#include <time.h>
#include <stdio.h>
#include <cmath>

#include "DS1307clock.hpp"
#include "SSD1306Display.hpp"
#include "BME280.hpp"
#include "I2CScanner.hpp"


namespace {
    constexpr uint8_t kI2cSdaPin = 8;
    constexpr uint8_t kI2cSclPin = 9;
    constexpr uint16_t kStartupMessageMs = 2000;
    constexpr uint16_t kLoopIntervalMs = 1000;
    constexpr uint16_t kRtcErrorDisplayMs = 2000;
    constexpr int32_t kUtcOffsetSeconds = 3 * 3600;
    constexpr unsigned long kRtcDataStaleMs = 30000UL; // 30 seconds, after which RTC fallback is considered stale and not used
    constexpr unsigned long kBmeDataStaleMs = 10000UL; // 10 seconds, after which BME280 fallback is considered stale and not used

    struct LastKnownGoodData {
        clock_app::DateTime rtc = {};
        unsigned long rtcUpdatedAtMs = 0;
        bool hasRtc = false;

        bme280_app::BME280Data bme = {};
        unsigned long bmeUpdatedAtMs = 0;
        bool hasBme = false;
    };

    bool isRtcDataValid(const clock_app::DateTime& value) {
        return value.second <= 59U && value.minute <= 59U && value.hour <= 23U
            && value.dayOfWeek >= 1U && value.dayOfWeek <= 7U
            && value.dayOfMonth >= 1U && value.dayOfMonth <= 31U
            && value.month >= 1U && value.month <= 12U
            && value.year >= 2000U;
    }

    bool isBmeDataValid(const bme280_app::BME280Data& value) {
        return std::isfinite(value.temperatureC)
            && std::isfinite(value.humidityPercent)
            && std::isfinite(value.pressureHpa)
            && value.temperatureC >= -40.0F
            && value.temperatureC <= 85.0F
            && value.humidityPercent >= 0.0F
            && value.humidityPercent <= 100.0F
            && value.pressureHpa >= 300.0F
            && value.pressureHpa <= 1100.0F;
    }

    bool isDataFresh(unsigned long updatedAtMs, unsigned long maxAgeMs, unsigned long nowMs) {
        return (nowMs - updatedAtMs) <= maxAgeMs;
    }
}  // namespace

clock_app::DS1307clock rtc;
clock_app::DateTime dateTime;
oled_app::SSD1306Display display;
bme280_app::BME280 bme280;
scanner_app::I2CScanResult i2cScanResult;
bool rtcErrorActive = false;
unsigned long rtcErrorStartedAtMs = 0;
bool rtcPresent = true;
LastKnownGoodData lastKnownGood;

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
    const unsigned long nowMs = millis();

    const bool rtcReadOk = rtcPresent && rtc.readDateTime(dateTime);
    const bool rtcOk = rtcReadOk && isRtcDataValid(dateTime);

    if (rtcOk) {
        lastKnownGood.rtc = dateTime;
        lastKnownGood.rtcUpdatedAtMs = nowMs;
        lastKnownGood.hasRtc = true;
    }

    // Even if RTC read is successful, the data might be invalid (e.g. due to RTC battery failure),
    // so we check validity separately and only log valid data.
    bme280_app::BME280Data currentBmeData;
    const bool bmeReadOk = bme280.readData(currentBmeData);
    const bool bmeOk = bmeReadOk && isBmeDataValid(currentBmeData);
    if (bmeOk) {
        lastKnownGood.bme = currentBmeData;
        lastKnownGood.bmeUpdatedAtMs = nowMs;
        lastKnownGood.hasBme = true;
    }

    const bool bmeFallbackFresh = lastKnownGood.hasBme
        && isDataFresh(lastKnownGood.bmeUpdatedAtMs, kBmeDataStaleMs, nowMs);
    const bme280_app::BME280Data* bmeToDisplay = bmeFallbackFresh ? &lastKnownGood.bme : nullptr;

    if (!rtcOk) {
        if (!rtcErrorActive) {
            rtcErrorActive = true;
            rtcErrorStartedAtMs = millis();
            Serial.println(rtcPresent ? "RTC read/validation error" : "RTC not connected");
        }

        const unsigned long errorDurationMs = millis() - rtcErrorStartedAtMs;
        if (errorDurationMs < kRtcErrorDisplayMs) {
            display.showError("RTC read error");
        } else {
            const bool rtcFallbackFresh = lastKnownGood.hasRtc
                && isDataFresh(lastKnownGood.rtcUpdatedAtMs, kRtcDataStaleMs, nowMs);

            if (rtcFallbackFresh) {
                Serial.println("RTC fallback: using last known good RTC value");
                if (bmeToDisplay != nullptr) {
                    Serial.printf(
                        "T: %.1f C RH: %.1f%% P: %.1f hPa\n",
                        bmeToDisplay->temperatureC,
                        bmeToDisplay->humidityPercent,
                        bmeToDisplay->pressureHpa);
                } else {
                    Serial.println("BME280 fallback unavailable or stale");
                }
                display.showDateTime(lastKnownGood.rtc, bmeToDisplay);
            } else {
                char systemDateTime[20] = {0};
                rtc.get_datetime(systemDateTime, sizeof(systemDateTime));
                Serial.printf("System time fallback: %s\n", systemDateTime);

                if (bmeToDisplay != nullptr) {
                    Serial.printf(
                        "T: %.1f C RH: %.1f%% P: %.1f hPa\n",
                        bmeToDisplay->temperatureC,
                        bmeToDisplay->humidityPercent,
                        bmeToDisplay->pressureHpa);
                } else {
                    Serial.println("BME280 fallback unavailable or stale");
                }
                display.showRtcFallbackTime(systemDateTime, bmeToDisplay);
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

        if (bmeToDisplay != nullptr) {
            Serial.printf(
                "T: %.1f C RH: %.1f%% P: %.1f hPa\n",
                bmeToDisplay->temperatureC,
                bmeToDisplay->humidityPercent,
                bmeToDisplay->pressureHpa);
            display.showDateTime(dateTime, bmeToDisplay);
        } else {
            Serial.println("BME280 read/validation error and no fresh fallback");
            display.showDateTime(dateTime);
        }
    }

    delay(kLoopIntervalMs);
}
