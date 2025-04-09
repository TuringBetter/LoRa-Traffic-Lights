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
    static const int        COLLISION_THRESHOLD         = 3;     // 碰撞检测阈值
    static const uint32_t   COLLISION_TIMEOUT           = 2000;   // 碰撞超时时间（毫秒）

public:
    Accelerometer(uint8_t address = 0x18);
    bool begin(Range range = RANGE_2G);
    void readRaw(int16_t &x, int16_t &y, int16_t &z);
    float getScaleFactor() const;
    void processDate(int16_t x, int16_t y, int16_t z);

private:
    static const uint8_t I2C_SDA_PIN = 6;
    static const uint8_t I2C_SCL_PIN = 5; 

    uint8_t     _address;
    bool        _collisionDetected = false;
    uint32_t    _lastCollisionDetectionTime=0;

private:
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg) const;
};