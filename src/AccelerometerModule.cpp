#include "AccelerometerModule.h"
#include <Arduino.h>

Accelerometer::Accelerometer(uint8_t address) : _address(address) {}

bool Accelerometer::begin(Range range) {
    writeRegister(0x20, 0x47);  // CTRL_REG1: 50Hz, XYZ enable
    writeRegister(0x23, static_cast<uint8_t>(range) << 4);  // CTRL_REG4: Set range
    
    // Verify configuration (optional)
    // uint8_t ctrl_reg1 = readRegister(0x20);
    // if (ctrl_reg1 != 0x47) return false;
    return true;
}

void Accelerometer::readRaw(int16_t &x, int16_t &y, int16_t &z) {
    Wire.beginTransmission(_address);
    Wire.write(0x28 | 0x80);
    if (Wire.endTransmission(false) != 0) {
        Serial.println("I2C communication error!");
        return;
    }

    Wire.requestFrom(_address, 6);
    if (Wire.available() == 6) {
        x = Wire.read() | (Wire.read() << 8);
        y = Wire.read() | (Wire.read() << 8);
        z = Wire.read() | (Wire.read() << 8);
    }
}

float Accelerometer::getScaleFactor() const {
    uint8_t range = (readRegister(0x23) >> 4) & 0x03;
    switch (static_cast<Range>(range)) {
        case RANGE_2G:  return 0.061f / 1000;
        case RANGE_4G:  return 0.122f / 1000;
        case RANGE_8G:  return 0.244f / 1000;
        case RANGE_16G: return 0.488f / 1000;
        default:        return 0.061f / 1000;
    }
}

void Accelerometer::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(_address);
    Wire.write(reg);
    Wire.write(value);
    if (Wire.endTransmission() != 0) {
        Serial.println("Write register failed!");
    }
}

uint8_t Accelerometer::readRegister(uint8_t reg) const {
    Wire.beginTransmission(_address);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(_address, 1);
    return Wire.read();
}