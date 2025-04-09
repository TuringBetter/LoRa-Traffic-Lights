#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "LedModule.h"
#include "LaserModule.h"
#include "AccelerometerModule.h"
// put function declarations here:

// 配置参数
static const int        VEHICLE_DETECTION_THRESHOLD = 500;    // 车辆检测阈值（厘米）
static const uint32_t   VEHICLE_TIMEOUT             = 500;  // 车辆超时时间（毫秒）

Laser laser;
Accelerometer accelerometer;

// 任务句柄
TaskHandle_t laserTaskHandle = NULL;
TaskHandle_t AccTaskHandle = NULL;
TaskHandle_t LedTaskHandle = NULL;

// 任务函数
void laserTask(void *pvParameters);
void accelerometerTask(void* pvParameters);
void ledTask(void* pvParameters);

// 辅助函数
void processLaserData(int16_t distance);

// 辅助变量
bool                _vehicleDetected;
uint32_t            _lastVehicleDetectionTime;

void setup() {
    Serial.begin(115200);
    Serial.println("系统初始化");


  // 创建激光测距任务
    xTaskCreatePinnedToCore(
        laserTask,           // 任务函数
        "LaserTask",         // 任务名称
        10000,               // 堆栈大小
        NULL,                // 任务参数
        1,                   // 任务优先级
        &laserTaskHandle,    // 任务句柄
        1                    // 运行核心 (1 = 核心1)
    );
  
  // 创建加速度计任务
    xTaskCreatePinnedToCore(
        accelerometerTask,   // 任务函数
        "AccelerometerTask", // 任务名称
        4096,                // 堆栈大小
        NULL,                // 任务参数
        1,                   // 任务优先级
        &AccTaskHandle,      // 任务句柄
        1                    // 运行核心 (1 = 核心1)
    );


  // 删除setup任务，因为不再需要
    vTaskDelete(NULL);
}

void loop() {

}

// put function definitions here:

// 激光测距任务
void laserTask(void *pvParameters) 
{
  // 初始化激光模块
    laser.begin();
    laser.sendReadCommand();
  // 任务主循环
    while (1) {
        int16_t distance = laser.receiveReadResponse();

        if (distance != -1) {
            // Serial.println(distance); 
            processLaserData(distance);
        }
        
        // 添加一个小延时，避免过于频繁的读取
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// 加速度计任务
void accelerometerTask(void* pvParameters)
{
    accelerometer.begin();
    while(true)
    {
        int16_t x, y, z;
        // 读取加速度计数据
        accelerometer.readRaw(x, y, z);
        
        // 处理加速度数据
        accelerometer.processDate(x, y, z);
        
        // 任务延时
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms
    }
}

void ledTask(void *pvParameters)
{

}

void processLaserData(int16_t distance)
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
        }
    }
}
