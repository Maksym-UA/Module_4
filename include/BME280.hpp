#pragma once

#include <Arduino.h>
#include <Wire.h>

namespace bme280_app {

    struct BME280Data {
        float temperatureC = 0.0F;
        float humidityPercent = 0.0F;
        float pressureHpa = 0.0F;
    };

    class BME280 {
    private:
        static constexpr uint8_t REG_CHIP_ID = 0xD0;
        static constexpr uint8_t REG_CTRL_HUM = 0xF2;
        static constexpr uint8_t REG_CTRL_MEAS = 0xF4;
        static constexpr uint8_t REG_CONFIG = 0xF5;
        static constexpr uint8_t REG_DATA_START = 0xF7;
        static constexpr uint8_t REG_CALIB00 = 0x88;
        static constexpr uint8_t REG_CALIB26 = 0xE1;

        struct CalibData {
            uint16_t dig_T1 = 0;
            int16_t dig_T2 = 0;
            int16_t dig_T3 = 0;
            uint16_t dig_P1 = 0;
            int16_t dig_P2 = 0;
            int16_t dig_P3 = 0;
            int16_t dig_P4 = 0;
            int16_t dig_P5 = 0;
            int16_t dig_P6 = 0;
            int16_t dig_P7 = 0;
            int16_t dig_P8 = 0;
            int16_t dig_P9 = 0;
            uint8_t dig_H1 = 0;
            int16_t dig_H2 = 0;
            uint8_t dig_H3 = 0;
            int16_t dig_H4 = 0;
            int16_t dig_H5 = 0;
            int8_t dig_H6 = 0;
        };

        TwoWire* wire_ = nullptr;
        uint8_t address_ = 0x76;
        bool initialized_ = false;
        CalibData calib_;
        mutable int32_t tFine_ = 0;

        // Helper functions to read little-endian values from the calibration data.
        static uint16_t readU16LE(const uint8_t* data) {
            return static_cast<uint16_t>(data[0]) |
                   (static_cast<uint16_t>(data[1]) << 8);
        }

        static int16_t readS16LE(const uint8_t* data) {
            return static_cast<int16_t>(readU16LE(data));
        }

        bool readRegister(uint8_t reg, uint8_t* data, uint8_t len) const {
            if (wire_ == nullptr) {
                return false;
            }

            wire_->beginTransmission(address_);
            wire_->write(reg);
            if (wire_->endTransmission(false) != 0) {
                return false;
            }

            if (wire_->requestFrom(address_, len) != len) {
                return false;
            }

            for (uint8_t i = 0; i < len; ++i) {
                if (!wire_->available()) {
                    return false;
                }
                data[i] = wire_->read();
            }

            return true;
        }

        bool writeRegister(uint8_t reg, uint8_t value) const {
            if (wire_ == nullptr) {
                return false;
            }

            wire_->beginTransmission(address_);
            wire_->write(reg);
            wire_->write(value);
            return wire_->endTransmission() == 0;
        }

        bool readCalibData() {
            uint8_t calib1[26] = {};
            uint8_t calib2[7] = {};

            if (!readRegister(REG_CALIB00, calib1, sizeof(calib1))) {
                return false;
            }

            if (!readRegister(REG_CALIB26, calib2, sizeof(calib2))) {
                return false;
            }

            // Read a signed 16-bit value in little-endian byte order.
            calib_.dig_T1 = readU16LE(&calib1[0]);
            calib_.dig_T2 = readS16LE(&calib1[2]);
            calib_.dig_T3 = readS16LE(&calib1[4]);

            calib_.dig_P1 = readU16LE(&calib1[6]);
            calib_.dig_P2 = readS16LE(&calib1[8]);
            calib_.dig_P3 = readS16LE(&calib1[10]);
            calib_.dig_P4 = readS16LE(&calib1[12]);
            calib_.dig_P5 = readS16LE(&calib1[14]);
            calib_.dig_P6 = readS16LE(&calib1[16]);
            calib_.dig_P7 = readS16LE(&calib1[18]);
            calib_.dig_P8 = readS16LE(&calib1[20]);
            calib_.dig_P9 = readS16LE(&calib1[22]);

            calib_.dig_H1 = calib1[25];
            calib_.dig_H2 = readS16LE(&calib2[0]);
            calib_.dig_H3 = calib2[2];

            calib_.dig_H4 = static_cast<int16_t>(
                (static_cast<int16_t>(calib2[3]) << 4) |
                (static_cast<int16_t>(calib2[4]) & 0x0F)
            );

            calib_.dig_H5 = static_cast<int16_t>(
                (static_cast<int16_t>(calib2[5]) << 4) |
                (static_cast<int16_t>(calib2[4]) >> 4)
            );

            if ((calib_.dig_H4 & 0x0800) != 0) {
                calib_.dig_H4 |= static_cast<int16_t>(0xF000);
            }

            if ((calib_.dig_H5 & 0x0800) != 0) {
                calib_.dig_H5 |= static_cast<int16_t>(0xF000);
            }

            calib_.dig_H6 = static_cast<int8_t>(calib2[6]);

            return true;
        }

        int32_t compensateTemp(int32_t adc_T) const {
            const int32_t var1 =
                ((((adc_T >> 3) - (static_cast<int32_t>(calib_.dig_T1) << 1))) *
                 static_cast<int32_t>(calib_.dig_T2)) >> 11;

            const int32_t var2 =
                (((((adc_T >> 4) - static_cast<int32_t>(calib_.dig_T1)) *
                   ((adc_T >> 4) - static_cast<int32_t>(calib_.dig_T1))) >> 12) *
                 static_cast<int32_t>(calib_.dig_T3)) >> 14;

            tFine_ = var1 + var2;
            return (tFine_ * 5 + 128) >> 8;
        }

        uint32_t compensatePress(int32_t adc_P) const {
            int64_t var1 = static_cast<int64_t>(tFine_) - 128000;
            int64_t var2 = var1 * var1 * static_cast<int64_t>(calib_.dig_P6);
            var2 += (var1 * static_cast<int64_t>(calib_.dig_P5)) << 17;
            var2 += static_cast<int64_t>(calib_.dig_P4) << 35;

            var1 = ((var1 * var1 * static_cast<int64_t>(calib_.dig_P3)) >> 8) +
                   ((var1 * static_cast<int64_t>(calib_.dig_P2)) << 12);

            var1 = (((static_cast<int64_t>(1) << 47) + var1) *
                    static_cast<int64_t>(calib_.dig_P1)) >> 33;

            if (var1 == 0) {
                return 0;
            }

            int64_t p = 1048576 - adc_P;
            p = (((p << 31) - var2) * 3125) / var1;

            var1 = (static_cast<int64_t>(calib_.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
            var2 = (static_cast<int64_t>(calib_.dig_P8) * p) >> 19;

            p = ((p + var1 + var2) >> 8) + (static_cast<int64_t>(calib_.dig_P7) << 4);
            return static_cast<uint32_t>(p);
        }

        uint32_t compensateHumid(int32_t adc_H) const {
            int32_t v_x1_u32r = tFine_ - 76800;

            v_x1_u32r =
                (((((adc_H << 14) - (static_cast<int32_t>(calib_.dig_H4) << 20) -
                    (static_cast<int32_t>(calib_.dig_H5) * v_x1_u32r)) + 16384) >> 15) *
                 (((((((v_x1_u32r * static_cast<int32_t>(calib_.dig_H6)) >> 10) *
                      (((v_x1_u32r * static_cast<int32_t>(calib_.dig_H3)) >> 11) + 32768)) >> 10) +
                    2097152) *
                   static_cast<int32_t>(calib_.dig_H2) + 8192) >> 14));

            v_x1_u32r -=
                (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                  static_cast<int32_t>(calib_.dig_H1)) >> 4);

            if (v_x1_u32r < 0) {
                v_x1_u32r = 0;
            }

            if (v_x1_u32r > 419430400) {
                v_x1_u32r = 419430400;
            }

            return static_cast<uint32_t>(v_x1_u32r >> 12);
        }

    public:
        static constexpr uint8_t kDefaultAddress = 0x76;
        static constexpr uint32_t kDefaultI2cClockHz = 100000;

        bool begin(
            TwoWire& wire,
            uint8_t sdaPin,
            uint8_t sclPin,
            uint8_t address = kDefaultAddress,
            uint32_t i2cClockHz = kDefaultI2cClockHz) {
            wire.begin(sdaPin, sclPin);
            wire.setClock(i2cClockHz);

            wire_ = &wire;
            address_ = address;
            initialized_ = false;

            uint8_t chipId = 0;
            if (!readRegister(REG_CHIP_ID, &chipId, 1)) {
                return false;
            }

            if (chipId != 0x60) {
                return false;
            }

            if (!readCalibData()) {
                return false;
            }

            if (!writeRegister(REG_CTRL_HUM, 0x01)) {
                return false;
            }

            if (!writeRegister(REG_CTRL_MEAS, 0x27)) {
                return false;
            }

            if (!writeRegister(REG_CONFIG, 0xA0)) {
                return false;
            }

            initialized_ = true;
            delay(50);
            return true;
        }

        bool readData(BME280Data& data, TwoWire& wire = Wire) {
            if (!initialized_) {
                return false;
            }

            if (wire_ == nullptr) {
                wire_ = &wire;
            }

            uint8_t buffer[8] = {};

            wire.beginTransmission(address_);
            wire.write(REG_DATA_START);
            if (wire.endTransmission(false) != 0) {
                return false;
            }

            if (wire.requestFrom(address_, static_cast<uint8_t>(8)) != 8) {
                return false;
            }

            for (uint8_t i = 0; i < 8; ++i) {
                if (!wire.available()) {
                    return false;
                }
                buffer[i] = wire.read();
            }

            const int32_t adc_P =
                (static_cast<int32_t>(buffer[0]) << 12) |
                (static_cast<int32_t>(buffer[1]) << 4) |
                (static_cast<int32_t>(buffer[2]) >> 4);

            const int32_t adc_T =
                (static_cast<int32_t>(buffer[3]) << 12) |
                (static_cast<int32_t>(buffer[4]) << 4) |
                (static_cast<int32_t>(buffer[5]) >> 4);

            const int32_t adc_H =
                (static_cast<int32_t>(buffer[6]) << 8) |
                static_cast<int32_t>(buffer[7]);

            const int32_t tempX100 = compensateTemp(adc_T);
            const uint32_t pressQ24_8 = compensatePress(adc_P);
            const uint32_t humQ22_10 = compensateHumid(adc_H);

            data.temperatureC = static_cast<float>(tempX100) / 100.0F;
            data.pressureHpa = static_cast<float>(pressQ24_8) / 25600.0F;
            data.humidityPercent = static_cast<float>(humQ22_10) / 1024.0F;

            return true;
        }
    };
}