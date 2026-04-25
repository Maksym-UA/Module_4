#pragma once

#include <Arduino.h>
#include <Wire.h>

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

    static uint8_t bcdToDec(uint8_t bcd) {
        return static_cast<uint8_t>(((bcd >> 4) * 10) + (bcd & 0x0F));
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

    bool readTime(uint8_t* hour, uint8_t* minute, uint8_t* second, uint8_t* day, uint8_t* date, uint8_t* month, uint16_t* year, TwoWire& wire = Wire) const {
        if (hour == nullptr || minute == nullptr || second == nullptr || day == nullptr || date == nullptr || month == nullptr || year == nullptr) {
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

        data[0] = wire.read();
        data[1] = wire.read();
        data[2] = wire.read();
        data[3] = wire.read();
        data[4] = wire.read();
        data[5] = wire.read();
        data[6] = wire.read();

        *second = bcdToDec(data[0] & 0x7F);
        *minute = bcdToDec(data[1] & 0x7F);
        *hour = decodeHour24(data[2]);
        *day = bcdToDec(data[3] & 0x07);
        *date = bcdToDec(data[4] & 0x3F);
        *month = bcdToDec(data[5] & 0x1F);
        *year = static_cast<uint16_t>(2000U + bcdToDec(data[6]));

        return true;
    }
};

} // namespace clock_app