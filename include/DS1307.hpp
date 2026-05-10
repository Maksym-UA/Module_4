#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <time.h>
#include <sys/time.h>
#include <cstring>


namespace clock_app {

    struct DateTime {
        uint8_t hour = 0;
        uint8_t minute = 0;
        uint8_t second = 0;
        uint8_t dayOfWeek = 0;
        uint8_t dayOfMonth = 0;
        uint8_t month = 0;
        uint16_t year = 0;
    };

    class DS1307clock {
    public:
        static constexpr uint8_t kAddress = 0x68;
        static constexpr time_t kMinValidEpoch = 946684800;
        // 2000-01-01 00:00:00 UTC, a lower bound to reject invalid RTC dates

        // Converts one byte from BCD (Binary-Coded Decimal) format into decimal value.
        static uint8_t bcdToDec(uint8_t bcd) {
            return static_cast<uint8_t>(((bcd >> 4) * 10) + (bcd & 0x0F));
        }

        static uint8_t decToBcd(uint8_t dec) {
            return static_cast<uint8_t>(((dec / 10U) << 4) | (dec % 10U));
        }

        static uint8_t decodeHour24(uint8_t hourBcd) {
            if ((hourBcd & 0x40U) != 0U) {
                uint8_t hour = bcdToDec(hourBcd & 0x1FU);
                const bool isPm = (hourBcd & 0x20U) != 0U;

                if (isPm && hour != 12U) {
                    hour = static_cast<uint8_t>(hour + 12U);
                } else if (!isPm && hour == 12U) {
                    hour = 0;
                }

                return hour;
            }

            return bcdToDec(hourBcd & 0x3FU);
        }


        static const char* dayToShortName(uint8_t dayOfWeek) {
            static const char* kDays[] = {"???", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
            return (dayOfWeek >= 1U && dayOfWeek <= 7U) ? kDays[dayOfWeek] : "???";
        }

        // Initializes the RTC and checks if it is running. If the RTC was halted, it will be restarted
        // and the function will return false this once, but true on the next call.
        static bool isDateTimeInRange(const DateTime& dateTime) {
            return dateTime.second <= 59U && dateTime.minute <= 59U && dateTime.hour <= 23U
                && dateTime.dayOfWeek >= 1U && dateTime.dayOfWeek <= 7U
                && dateTime.dayOfMonth >= 1U && dateTime.dayOfMonth <= 31U
                && dateTime.month >= 1U && dateTime.month <= 12U
                && dateTime.year >= 2000U && dateTime.year <= 2099U;
        }

        static bool isResetDefaultDate(const DateTime& dateTime) {
            return dateTime.year == 2000U
                && dateTime.month == 1U
                && dateTime.dayOfMonth == 1U;
        }

        bool readDateTime(DateTime& dateTime, TwoWire& wire = Wire) const {
            return readTime(
                &dateTime.hour,
                &dateTime.minute,
                &dateTime.second,
                &dateTime.dayOfWeek,
                &dateTime.dayOfMonth,
                &dateTime.month,
                &dateTime.year,
                wire);
        }


        bool writeDateTime(const DateTime& dateTime, TwoWire& wire = Wire) const {
            if (!isDateTimeInRange(dateTime)) {
                return false;
            }

            wire.beginTransmission(kAddress);
            wire.write(0x00);
            wire.write(decToBcd(dateTime.second));
            wire.write(decToBcd(dateTime.minute));
            wire.write(decToBcd(dateTime.hour));
            wire.write(decToBcd(dateTime.dayOfWeek));
            wire.write(decToBcd(dateTime.dayOfMonth));
            wire.write(decToBcd(dateTime.month));
            wire.write(decToBcd(static_cast<uint8_t>(dateTime.year - 2000U)));

            return wire.endTransmission() == 0;
        }

        // Reads time and date from the RTC. Returns true if successful, false on error.
        bool readTime(uint8_t* hour, uint8_t* minute, uint8_t* second, uint8_t* day, uint8_t* date,
            uint8_t* month, uint16_t* year, TwoWire& wire = Wire) const {
            if (hour == nullptr || minute == nullptr || second == nullptr || day == nullptr
                || date == nullptr || month == nullptr || year == nullptr) {
                return false;
            }

            uint8_t data[7] = {0};

            wire.beginTransmission(kAddress);
            wire.write(0x00);
            if (wire.endTransmission(false) != 0) {
                return false;
            }

            const uint8_t readCount = wire.requestFrom(kAddress, static_cast<uint8_t>(7));
            if (readCount != 7 || wire.available() < 7) {
                return false;
            }

            // Read the 7 bytes of time data from the RTC
            data[0] = wire.read();
            data[1] = wire.read();
            data[2] = wire.read();
            data[3] = wire.read();
            data[4] = wire.read();
            data[5] = wire.read();
            data[6] = wire.read();

            const bool clockHalted = (data[0] & 0x80U) != 0U;
            if (clockHalted) {
                // Start the clock by writing 0 to the seconds register
                wire.beginTransmission(kAddress);
                wire.write(0x00);
                wire.write(data[0] & 0x7F); // Keep the seconds, clear the CH bit
                wire.endTransmission();
                Serial.println("RTC was halted - Restarting oscillator...");
                // Optionally return false this once, it will work on the next loop
                return false;
            }

            *second = bcdToDec(data[0] & 0x7F);
            *minute = bcdToDec(data[1] & 0x7F);
            *hour = decodeHour24(data[2]);
            *day = bcdToDec(data[3] & 0x07);
            *date = bcdToDec(data[4] & 0x3F);
            *month = bcdToDec(data[5] & 0x1F);
            *year = static_cast<uint16_t>(2000U + bcdToDec(data[6]));

            return true;
        }

        static int monthFromAbbrev(const char* month) {
            static const char* kMonths[] = {
                "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

            for (int i = 0; i < 12; ++i) {
                if (strcmp(month, kMonths[i]) == 0) {
                    return i;
                }
            }

            return 0;
        }

        // Initializes the system time based on the build date and time.
        // This allows the system to have a reasonable time even if the RTC is not available
        // or has an invalid date.
        void initSystemTimeFromBuild() const {
            struct tm buildTm = {};
            char monthText[4] = {0};
            int day = 1;
            int year = 1970;
            int hour = 0;
            int minute = 0;
            int second = 0;

            sscanf(__DATE__, "%3s %d %d", monthText, &day, &year);
            sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

            buildTm.tm_year = year - 1900;
            buildTm.tm_mon = monthFromAbbrev(monthText);
            buildTm.tm_mday = day;
            buildTm.tm_hour = hour;
            buildTm.tm_min = minute;
            buildTm.tm_sec = second;

            const time_t buildEpoch = mktime(&buildTm);
            if (buildEpoch <= 0) {
                return;
            }

            fallbackBaseEpoch_ = buildEpoch;
            fallbackBaseMillis_ = millis();

            const struct timeval tv = {.tv_sec = buildEpoch, .tv_usec = 0};
            settimeofday(&tv, nullptr);
        }

        void get_datetime(char* buffer, size_t len) const {
            if (buffer == nullptr || len == 0) {
                return;
            }

            time_t now = time(NULL);
            if (now < kMinValidEpoch && fallbackBaseEpoch_ >= kMinValidEpoch) {
                const unsigned long elapsedSeconds = (millis() - fallbackBaseMillis_) / 1000UL;
                now = fallbackBaseEpoch_ + static_cast<time_t>(elapsedSeconds);
            }

            if (now < kMinValidEpoch) {
                snprintf(buffer, len, "TIME NOT SET");
                return;
            }

            now += utcOffsetSeconds_;

            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            // Format: YYYY-MM-DD HH:MM:SS
            strftime(buffer, len, "%Y-%m-%d %H:%M:%S", &timeinfo);
        }

    private:
        mutable time_t fallbackBaseEpoch_ = 0;
        mutable unsigned long fallbackBaseMillis_ = 0;
        int32_t utcOffsetSeconds_ = 0;
    };
}