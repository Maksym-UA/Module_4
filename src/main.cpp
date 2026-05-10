/* 5. Вимоги logger.cpp
-  Ініціалізації логера
-  Запис логу в пам'ть
- Считування логу з пам'яті
- Пошук останнього логу і сторінки пам'яті в який він записаний.

6. Використання бібліотек
- Бібліотека для роботи з EEPROM AT24C32

7. Розмір і формат логів
- Розмір одного лога: 32 байти
- Текстовий формат з номером логу
- Уся кількість логів, що зберігаються, залежить від розміру EEPROM (AT24C32 має 32 Кб, що дозволяє зберігати до 1024 логів по 32 байти кожен).
- при заповненні всій пам'яті, починати запис поверх найстаршого логу - принцип ring buffer.

8. Виведення логів в послідовний інтерфейс
- по натисканню кнопки
- порядок виведення з останного до початкового


Приклад лога:
"#156 Error: Failed to read memory\0" -> "#156 Error: Failed to read mem\0" */

#include <Arduino.h>
#include <Wire.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <cmath>
#include <cstdlib>

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
    constexpr uint8_t kRtcFailureThreshold = 3;
    constexpr uint8_t kRtcReadRetries = 3;
    constexpr unsigned long kRtcDataStaleMs = 30000UL; // 30 seconds, after which RTC fallback is considered stale and not used
    constexpr unsigned long kBmeDataStaleMs = 10000UL; // 10 seconds, after which BME280 fallback is considered stale and not used

    // Struct to hold the last known good RTC and BME280 data along with their update timestamps.
    struct LastKnownGoodData {
        clock_app::DateTime rtc = {};
        unsigned long rtcUpdatedAtMs = 0;
        bool hasRtc = false;

        bme280_app::BME280Data bme = {};
        unsigned long bmeUpdatedAtMs = 0;
        bool hasBme = false;
    };

    bool isRtcDataValid(const clock_app::DateTime& value) {
        return clock_app::DS1307clock::isDateTimeInRange(value)
            && !clock_app::DS1307clock::isResetDefaultDate(value);
    }

    time_t epochFromDateTime(const clock_app::DateTime& value) {
        struct tm timeInfo = {};
        timeInfo.tm_year = static_cast<int>(value.year) - 1900;
        timeInfo.tm_mon = static_cast<int>(value.month) - 1;
        timeInfo.tm_mday = static_cast<int>(value.dayOfMonth);
        timeInfo.tm_hour = static_cast<int>(value.hour);
        timeInfo.tm_min = static_cast<int>(value.minute);
        timeInfo.tm_sec = static_cast<int>(value.second);
        timeInfo.tm_isdst = -1;
        return mktime(&timeInfo);
    }

    bool readRtcWithRetry(clock_app::DS1307clock& rtc, clock_app::DateTime& value) {
        for (uint8_t attempt = 0; attempt < kRtcReadRetries; ++attempt) {
            clock_app::DateTime candidate = {};
            const bool readOk = rtc.readDateTime(candidate);
            if (readOk && isRtcDataValid(candidate)) {
                value = candidate;
                return true;
            }
            delay(5);
        }

        return false;
    }

    void syncRtcOnBoot(clock_app::DS1307clock& rtc, bool rtcPresent) {
        if (!rtcPresent) return;

        // 1. Wait for power to stabilize
        delay(500);

        clock_app::DateTime rtcAtBoot = {};
        if (readRtcWithRetry(rtc, rtcAtBoot)) {
            // Success: Set the ESP32 system clock to the actual Hardware RTC time
            const time_t rtcEpoch = epochFromDateTime(rtcAtBoot);
            const struct timeval tv = {.tv_sec = rtcEpoch, .tv_usec = 0};
            settimeofday(&tv, nullptr);
            Serial.println("System clock synced to Hardware RTC.");
        } else {
            // Fail: The RTC is struggling. DO NOT overwrite it.
            Serial.println("RTC read failed. Hardware time preserved (not overwritten).");
        }
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

    void logBmeDataOrFallback(const bme280_app::BME280Data* bmeData) {
        if (bmeData != nullptr) {
            Serial.printf(
                "T: %.1f C RH: %.1f%% P: %.1f hPa\n",
                bmeData->temperatureC,
                bmeData->humidityPercent,
                bmeData->pressureHpa);
        } else {
            Serial.println("BME280 fallback unavailable or stale");
        }
    }

    void showRtcFallbackTime(
        const clock_app::DS1307clock& rtc,
        const LastKnownGoodData& lastKnownGood,
        unsigned long nowMs,
        oled_app::SSD1306Display& display,
        const bme280_app::BME280Data* bmeToDisplay,
        bool withSerialLogging) {

        const bool rtcFallbackFresh = lastKnownGood.hasRtc
            && isDataFresh(lastKnownGood.rtcUpdatedAtMs, kRtcDataStaleMs, nowMs);

        if (rtcFallbackFresh) {
            if (withSerialLogging) {
                Serial.println("RTC fallback: using last known good RTC value");
                logBmeDataOrFallback(bmeToDisplay);
            }
            display.showDateTime(lastKnownGood.rtc, bmeToDisplay);
            return;
        }

        char systemDateTime[20] = {0};
        rtc.get_datetime(systemDateTime, sizeof(systemDateTime));
        if (withSerialLogging) {
            Serial.printf("System time fallback: %s\n", systemDateTime);
            logBmeDataOrFallback(bmeToDisplay);
        }
        display.showRtcFallbackTime(systemDateTime, bmeToDisplay);
    }
}  // namespace


clock_app::DS1307clock rtc;
clock_app::DateTime dateTime;
oled_app::SSD1306Display display;
bme280_app::BME280 bme280;
scanner_app::I2CScanResult i2cScanResult;
bool rtcErrorActive = false;
unsigned long rtcErrorStartedAtMs = 0;
uint8_t rtcConsecutiveFailures = 0;
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
        rtc.initSystemTimeFromBuild();
    }

    syncRtcOnBoot(rtc, rtcPresent);

    Serial.println("System initialized!");
}


void loop() {
    const unsigned long nowMs = millis();

    const bool rtcOk = rtcPresent && readRtcWithRetry(rtc, dateTime);

    if (rtcOk) {
        rtcConsecutiveFailures = 0;
        lastKnownGood.rtc = dateTime;
        lastKnownGood.rtcUpdatedAtMs = nowMs;
        lastKnownGood.hasRtc = true;
    } else if (rtcConsecutiveFailures < 255U) {
        ++rtcConsecutiveFailures;
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

    const bool rtcFailurePersistent = rtcConsecutiveFailures >= kRtcFailureThreshold;

    if (!rtcOk && rtcFailurePersistent) {
        if (!rtcErrorActive) {
            rtcErrorActive = true;
            rtcErrorStartedAtMs = millis();
            Serial.println(rtcPresent ? "RTC read/validation error" : "RTC not connected");
        }

        const unsigned long errorDurationMs = millis() - rtcErrorStartedAtMs;
        if (errorDurationMs < kRtcErrorDisplayMs) {
            display.showError("RTC read error");
        } else {
            showRtcFallbackTime(rtc, lastKnownGood, nowMs, display, bmeToDisplay, true);
        }
    } else if (!rtcOk) {
        showRtcFallbackTime(rtc, lastKnownGood, nowMs, display, bmeToDisplay, false);
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