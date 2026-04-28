#pragma once

#include <Arduino.h>
#include <Wire.h>

namespace scanner_app {

constexpr uint8_t kMaxFoundDevices = 16;

struct I2CScanResult {
    uint8_t addresses[kMaxFoundDevices] = {0};
    uint8_t count = 0;
};

inline void scanI2CDevices(TwoWire& wire, Stream& serial, I2CScanResult& result) {
    result.count = 0;
    serial.println("I2C scan start...");

    for (uint8_t addr = 1; addr < 127; ++addr) {
        wire.beginTransmission(addr);
        if (wire.endTransmission() == 0) {
            serial.print("Found I2C device at 0x");
            if (addr < 16) {
                serial.print('0');
            }
            serial.println(addr, HEX);

            if (result.count < kMaxFoundDevices) {
                result.addresses[result.count] = addr;
                ++result.count;
            }
        }
    }

    if (result.count == 0) {
        serial.println("No I2C devices found");
    }
}

}
