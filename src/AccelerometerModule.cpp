#include "AccelerometerModule.h"
#include <Arduino.h>
#include "LoRaModule.h"

static const uint64_t   ACC_I2C_SDA_PIN     = 6;
static const uint64_t   ACC_I2C_SCL_PIN     = 5;
static const uint8_t    ADDR                = 0x18;
static const int        COLLISION_THRESHOLD = 3;     // 碰撞检测阈值
static const uint32_t   COLLISION_TIMEOUT   = 2000;   // 碰撞超时时间（毫秒）
static       bool       _collisionDetected  = false;
static       uint32_t   _lastCollisionDetectionTime=0;

TaskHandle_t AccTaskHandle          = NULL;
TaskHandle_t AccMonitorTaskHandle   = NULL;

static void writeRegister(uint8_t reg, uint8_t value);
static void processDate(int16_t x, int16_t y, int16_t z);
static void readRaw(int16_t &x, int16_t &y, int16_t &z);
static void reportData(int16_t x, int16_t y, int16_t z);

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
            // Serial.printf("Collision detected! Acceleration: %.3lf\n", acceleration);
            sendData("08");
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

void reportData(int16_t x, int16_t y, int16_t z)
{
    /*
    // 构造上报字符串，格式：10 x y z
    String payload = "10 ";
    payload += String(x);
    payload += " ";
    payload += String(y);
    payload += " ";
    payload += String(z);
    sendData(payload);
    */
    sendData("05");
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
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void accMonitorTask(void *pvParameters)
{
    static uint8_t payload[7] = {0x05};
    static float scale {0.061f / 1000};
    const int sampleIntervalMs = 200; // 采样间隔200ms
    const int reportIntervalMs = 6000; // 上报间隔6秒
    const int sampleCount = reportIntervalMs / sampleIntervalMs; // 采样次数

    while (true)
    {
        float maxNorm = 0.0f;
        int16_t maxX = 0, maxY = 0, maxZ = 0;

        for (int i = 0; i < sampleCount; ++i)
        {
            int16_t x, y, z;
            readRaw(x, y, z);

            // 计算加速度矢量模长
            float fx = x * scale;
            float fy = y * scale;
            float fz = z * scale;
            float norm = sqrtf(fx * fx + fy * fy + fz * fz);

            if (norm > maxNorm) {
                maxNorm = norm;
                maxX = x;
                maxY = y;
                maxZ = z;
            }

            vTaskDelay(pdMS_TO_TICKS(sampleIntervalMs));
        }

        // 6秒后，上报最大加速度对应的x, y, z
        payload[1] = maxX & 0xFF;
        payload[2] = (maxX >> 8) & 0xFF;
        payload[3] = maxY & 0xFF;
        payload[4] = (maxY >> 8) & 0xFF;
        payload[5] = maxZ & 0xFF;
        payload[6] = (maxZ >> 8) & 0xFF;

        String hexPayload = "";
        for (int i = 0; i < 7; ++i) {
            if (payload[i] < 0x10) hexPayload += "0";
            hexPayload += String(payload[i], HEX);
        }
        /*
        Serial.print("Max Acc Norm: ");
        Serial.println(maxNorm, 4);
        Serial.print("Report Payload: ");
        Serial.println(hexPayload);
        */
        sendData(hexPayload);
    }
}
