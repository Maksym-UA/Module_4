#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

namespace bme280_app {

    struct BME280Data {
        float temperatureC = 0.0F;
        float humidityPercent = 0.0F;
        float pressureHpa = 0.0F;
    };

    class BME280 {
        public:
            static constexpr uint8_t kDefaultAddress = 0x76; // 0x76 or 0x77 depending on the sensor's address pin configuration
            static constexpr uint32_t kDefaultI2cClockHz = 100000;

            // Initializes the BME280 sensor. Returns true if initialization was successful.
            bool begin(
                TwoWire& wire,
                uint8_t sdaPin,
                uint8_t sclPin,
                uint8_t address = kDefaultAddress,
                uint32_t i2cClockHz = kDefaultI2cClockHz) {
                wire.begin(sdaPin, sclPin);
                wire.setClock(i2cClockHz);
                initialized_ = sensor_.begin(address, &wire);
                return initialized_;
            }

            bool begin(TwoWire& wire = Wire, uint8_t address = kDefaultAddress) {
                initialized_ = sensor_.begin(address, &wire);
                return initialized_;
            }

            bool readData(BME280Data& data) const {
                if (!initialized_) {
                    return false;
                }

                data.temperatureC = sensor_.readTemperature();
                data.humidityPercent = sensor_.readHumidity();
                data.pressureHpa = sensor_.readPressure() / 100.0F;
                return true;
            }

        private:
            mutable Adafruit_BME280 sensor_;
            bool initialized_ = false;
    };
}
