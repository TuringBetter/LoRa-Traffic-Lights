#include "AccelerometerModule.h"
#include <Arduino.h>
#include "LoRaModule.h"


/** *
extern LoRa lora;

Accelerometer::Accelerometer(uint8_t address) : _address(address) {}

bool Accelerometer::begin(Range range) {
    Wire1.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    writeRegister(0x20, 0x47);  // CTRL_REG1: 50Hz, XYZ enable
    writeRegister(0x23, static_cast<uint8_t>(range) << 4);  // CTRL_REG4: Set range
    
    // Verify configuration (optional)
    // uint8_t ctrl_reg1 = readRegister(0x20);
    // if (ctrl_reg1 != 0x47) return false;
    return true;
}

void Accelerometer::readRaw(int16_t &x, int16_t &y, int16_t &z) {
    Wire1.beginTransmission(_address);
    Wire1.write(0x28 | 0x80);
    if (Wire1.endTransmission(false) != 0) {
        Serial.println("I2C communication error!");
        return;
    }

    Wire1.requestFrom(_address, 6);
    if (Wire1.available() == 6) {
        x = Wire1.read() | (Wire1.read() << 8);
        y = Wire1.read() | (Wire1.read() << 8);
        z = Wire1.read() | (Wire1.read() << 8);
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

void Accelerometer::processDate(int16_t x, int16_t y, int16_t z)
{
    static float scale{0.061f / 1000};
    // 计算加速度的绝对值
    double acceleration = abs(x * scale)*abs(x * scale) + abs(y * scale)*abs(y * scale) + abs(z * scale)*abs(z * scale);

    // 检查是否检测到碰撞
    if (acceleration > COLLISION_THRESHOLD) {
        if (!_collisionDetected) {
            _collisionDetected = true;
            _lastCollisionDetectionTime = millis();
            
            // 打印碰撞信息
            Serial.printf("Collision detected! Acceleration: %.3lf\n", acceleration);
            // lora.sendData(LoRa::SendMode::UNCONFIRMED,1,"08");
        }
    } else {
        // 检查碰撞状态是否已经恢复
        if (_collisionDetected && (millis() - _lastCollisionDetectionTime > COLLISION_TIMEOUT)) {
            _collisionDetected = false;
        }
    }
}

void Accelerometer::writeRegister(uint8_t reg, uint8_t value) {
    Wire1.beginTransmission(_address);
    Wire1.write(reg);
    Wire1.write(value);
    if (Wire1.endTransmission() != 0) {
        Serial.println("Write register failed!");
    }
}

uint8_t Accelerometer::readRegister(uint8_t reg) const {
    Wire1.beginTransmission(_address);
    Wire1.write(reg);
    Wire1.endTransmission(false);
    Wire1.requestFrom(_address, 1);
    return Wire1.read();
}
/** */
/*====================================================================================*/
static const uint64_t   ACC_I2C_SDA_PIN     = 6;
static const uint64_t   ACC_I2C_SCL_PIN     = 5;
static const uint8_t    ADDR                = 0x18;
static const int        COLLISION_THRESHOLD = 3;     // 碰撞检测阈值
static const uint32_t   COLLISION_TIMEOUT   = 2000;   // 碰撞超时时间（毫秒）
static       bool       _collisionDetected  = false;
static       uint32_t   _lastCollisionDetectionTime=0;

TaskHandle_t AccTaskHandle     = NULL;

static void writeRegister(uint8_t reg, uint8_t value);

/**/
void Acc_init()
{
    Wire1.begin(ACC_I2C_SDA_PIN, ACC_I2C_SCL_PIN);
    writeRegister(0x20, 0x47);  // CTRL_REG1: 50Hz, XYZ enable
    writeRegister(0x23, static_cast<uint8_t>(0x00) << 4);  // CTRL_REG4: Set range
}
/**/
void processDate(int16_t x, int16_t y, int16_t z)
{
    static float scale{0.061f / 1000};
    // 计算加速度的绝对值
    double acceleration = abs(x * scale)*abs(x * scale) + abs(y * scale)*abs(y * scale) + abs(z * scale)*abs(z * scale);

    // 检查是否检测到碰撞
    if (acceleration > COLLISION_THRESHOLD) {
        if (!_collisionDetected) {
            _collisionDetected = true;
            _lastCollisionDetectionTime = millis();
            
            // 打印碰撞信息
            Serial.printf("Collision detected! Acceleration: %.3lf\n", acceleration);
            // lora.sendData(LoRa::SendMode::UNCONFIRMED,1,"08");
        }
    } else {
        // 检查碰撞状态是否已经恢复
        if (_collisionDetected && (millis() - _lastCollisionDetectionTime > COLLISION_TIMEOUT)) {
            _collisionDetected = false;
        }
    }
}

void readRaw(int16_t &x, int16_t &y, int16_t &z)
{
    Wire1.beginTransmission(ADDR);
    Wire1.write(0x28 | 0x80);
    if (Wire1.endTransmission(false) != 0) 
    {
        Serial.println("I2C communication error!");
        return;
    }

    Wire1.requestFrom(ADDR, static_cast<uint8_t>(6));
    if (Wire1.available() == 6) {
        x = Wire1.read() | (Wire1.read() << 8);
        y = Wire1.read() | (Wire1.read() << 8);
        z = Wire1.read() | (Wire1.read() << 8);
    }
}

static void writeRegister(uint8_t reg, uint8_t value)
{
    Wire1.beginTransmission(ADDR);
    Wire1.write(reg);
    Wire1.write(value);
    if (Wire1.endTransmission() != 0) {
        Serial.println("Write register failed!");
    }   
}

void accelerometerTask(void* pvParameters)
{
    while(true)
    {
        int16_t x, y, z;
        // 读取加速度计数据
        readRaw(x, y, z);
        
        // 处理加速度数据
        processDate(x, y, z);
        
        // 任务延时
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms
    }
}