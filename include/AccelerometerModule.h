#pragma once
#include <Wire.h>

class Accelerometer {
public:
    enum Range {
        RANGE_2G = 0x00,
        RANGE_4G = 0x01,
        RANGE_8G = 0x02,
        RANGE_16G = 0x03
    };

    Accelerometer(uint8_t address = 0x18);
    bool begin(Range range = RANGE_2G);
    void readRaw(int16_t &x, int16_t &y, int16_t &z);
    float getScaleFactor() const;

private:
    uint8_t _address;
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg) const;
};