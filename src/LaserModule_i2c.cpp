#include "LaserModule_i2c.h"
#include <Wire.h>
#include "LedModule.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


static const uint64_t   LASER_I2C_SDA_PIN = 8;
static const uint64_t   LASER_I2C_SCL_PIN = 7;
static const int        VEHICLE_DETECTION_THRESHOLD = 500;    // 车辆检测阈值（厘米）
static const uint32_t   VEHICLE_TIMEOUT             = 500;  // 车辆超时时间（毫秒）

static bool             _vehicleDetected;
static uint32_t         _lastVehicleDetectionTime;

extern SemaphoreHandle_t   _ledStateMutex;
extern LedState            _ledState;
extern bool                _ledStateChanged;

static void processLaserData(int16_t distance);

void Laser_I2C_init()
{
    Wire.begin(LASER_I2C_SDA_PIN, LASER_I2C_SCL_PIN);       // 初始化I2C，ESP32常用引脚为21(SDA),22(SCL)
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

// 激光测距任务
void laserTask(void *pvParameters) 
{
    /** *
  // 初始化激光模块
    // Serial.println("Laser init...");
    laser.begin();
    laser.sendReadCommand();
    /** */
  // 任务主循环
    while (1) {
        int16_t distance = readDistance();
        /* *
        Serial.print("Distance: ");
        Serial.print(distance);
        Serial.println(" mm");
        /* */
        if (distance != -1) 
        {
            // Serial.println(distance); 
            processLaserData(distance);
        }
        
        // 添加一个小延时，避免过于频繁的读取
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void processLaserData(int16_t distance)
{
    // 检查是否有车辆接近
    if (distance < VEHICLE_DETECTION_THRESHOLD) 
    {
        if (!_vehicleDetected) 
        {
            _vehicleDetected = true;
            _lastVehicleDetectionTime = millis();
            
            // 发送车辆接近消息
            // sendLoRaMessage(MessageType::VEHICLE_APPROACHING, _lightId + 1);
            // sendLoRaMessage(MessageType::VEHICLE_APPROACHING, _lightId + 2);
            
            // 灯亮
            Serial.println("vehicle detected!");
            if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) 
            {
                // 更新灯状态
                _ledState.color=LedColor::RED;
                _ledState.frequency=60;
                _ledState.brightness=7000;

                _ledStateChanged=true;

                xSemaphoreGive(_ledStateMutex);
            }
        }
    } 
    else 
    {
        // 检查车辆是否已经通过
        if (_vehicleDetected && (millis() - _lastVehicleDetectionTime > VEHICLE_TIMEOUT)) {
            _vehicleDetected = false;
            
            // 发送车辆已通过消息
            // sendLoRaMessage(MessageType::VEHICLE_PASSED, _lightId + 1);
            // sendLoRaMessage(MessageType::VEHICLE_PASSED, _lightId + 2);
            
            // 更新LED状态
            Serial.println("vehicle left");
            if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) 
            {
                // 更新灯状态
                _ledState.color=LedColor::YELLOW;
                _ledState.frequency=0;
                _ledState.brightness=1000;

                _ledStateChanged=true;
                
                xSemaphoreGive(_ledStateMutex);
            }
        }
    }
}