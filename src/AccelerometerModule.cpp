/**
  ******************************************************************************
  * @file    AccelerometerModule.cpp
  * @author  陕西交通电子工程科技有限公司
  * @version V1.0.0
  * @date    2025-07-19
  * @brief   三轴加速度计模块，主要用于碰撞检测和姿态检测。
  * @details
  * > 模块职责:
  * 通过I2C接口与加速度计传感器通信，实时监测设备的三轴加速度数据。
  *
  * > 主要功能:
  * - **碰撞检测**: `accelerometerTask`任务计算加速度矢量模长，若超过预设的
  * `COLLISION_THRESHOLD`阈值，则认为设备发生碰撞，并立即通过LoRa上报
  * 碰撞警报。
  * - **状态监控**: `accMonitorTask`是一个可选任务，可通过LoRa指令启动。它会
  * 周期性地上报一段时间内的最大加速度值，用于远程设备诊断和状态分析。
  *
  ******************************************************************************
  */
 
#include "AccelerometerModule.h"
#include <Arduino.h>
#include "LoRaModule.h"

static const uint64_t   ACC_I2C_SDA_PIN     = 6;
static const uint64_t   ACC_I2C_SCL_PIN     = 5;
static const uint8_t    ADDR                = 0x18;
static const int        COLLISION_THRESHOLD = 3;     // 碰撞检测阈值
static const uint32_t   COLLISION_TIMEOUT   = 2000;   // 碰撞超时时间（毫秒）
static const float      SKEW_THRESHOLD      = 0.8f;   // 倾斜检测阈值（余弦值）
static const uint32_t   SKEW_TIMEOUT        = 3000;   // 倾斜超时时间（毫秒）
static       bool       _collisionDetected  = false;
static       uint32_t   _lastCollisionDetectionTime=0;
static       bool       _skewDetected       = false;
static       uint32_t   _lastSkewDetectionTime=0;
static int16_t  init_x = 0;
static int16_t  init_y = 0;
static int16_t  init_z = 0;

TaskHandle_t AccTaskHandle          = NULL;
TaskHandle_t AccMonitorTaskHandle   = NULL;

static void writeRegister(uint8_t reg, uint8_t value);
static void processDate(int16_t x, int16_t y, int16_t z);
static void readRaw(int16_t &x, int16_t &y, int16_t &z);
static void reportData(int16_t x, int16_t y, int16_t z);
static float calculateCosine(int16_t x1, int16_t y1, int16_t z1, int16_t x2, int16_t y2, int16_t z2);

/**/
void Acc_init()
{
    Wire1.begin(ACC_I2C_SDA_PIN, ACC_I2C_SCL_PIN);
    writeRegister(0x20, 0x47);  // CTRL_REG1: 50Hz, XYZ enable
    writeRegister(0x23, static_cast<uint8_t>(0x00) << 4);  // CTRL_REG4: Set range
    readRaw(init_x, init_y, init_z);
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
    
    // 检测是否倾斜
    float cosineValue = calculateCosine(x, y, z, init_x, init_y, init_z);
    
    // 检查是否检测到倾斜（余弦值小于阈值表示角度变化较大）
    if (cosineValue < SKEW_THRESHOLD) {
        if (!_skewDetected) {
            _skewDetected = true;
            _lastSkewDetectionTime = millis();
            
            // 打印倾斜信息
            Serial.printf("Skew detected! Cosine value: %.3f\n", cosineValue);
            sendData("04"); // 发送倾斜检测代码
        }
    } else {
        // 检查倾斜状态是否已经恢复
        if (_skewDetected && (millis() - _lastSkewDetectionTime > SKEW_TIMEOUT)) {
            _skewDetected = false;
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

static float calculateCosine(int16_t x1, int16_t y1, int16_t z1, int16_t x2, int16_t y2, int16_t z2)
{
    // 计算两个向量的点积
    float dotProduct = (x1 * x2) + (y1 * y2) + (z1 * z2);
    
    // 计算两个向量的模长
    float magnitude1 = sqrtf(x1 * x1 + y1 * y1 + z1 * z1);
    float magnitude2 = sqrtf(x2 * x2 + y2 * y2 + z2 * z2);
    
    // 避免除零错误
    if (magnitude1 == 0.0f || magnitude2 == 0.0f) {
        return 1.0f; // 如果任一向量为零向量，返回1（认为无倾斜）
    }
    
    // 计算余弦值
    return dotProduct / (magnitude1 * magnitude2);
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
