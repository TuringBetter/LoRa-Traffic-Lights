#include "LaserModule_i2c.h"
#include <Wire.h>

const uint64_t I2C_SDA_PIN = 8;
const uint64_t I2C_SCL_PIN = 7;

void Laser_I2C_init()
{
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);       // 初始化I2C，ESP32常用引脚为21(SDA),22(SCL)
    // Serial.println("init i2c pin...");
    delay(100);                                 // 等待初始化完成    
}
/** */

void LaserStart()
{
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(CMD_REG);                        // 写入命令寄存器地址
    Wire.write(0x01);                           // 启动测量
    // Serial.println("send start...");
    if (Wire.endTransmission() != 0) 
    {
        Serial.println("Start command failed");
        delay(1000);
        return;
    }
    delay(100);                                 // 等待测量完成（根据模块需求调整）
}

int16_t readDistance()
{
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(DIST_HIGH_REG); // 设置读取起始地址
    if (Wire.endTransmission(false) != 0) 
    { // 保持I2C连接
        Serial.println("Set register failed");
        return -1;
    }

    // 请求读取2字节数据
    uint8_t bytesRead = Wire.requestFrom(I2C_ADDR, 2);
    if (bytesRead != 2) 
    {
        Serial.println("Incomplete data");
        return -1;
    }
    uint16_t distance = (Wire.read() << 8) | Wire.read();
    return distance;
}
