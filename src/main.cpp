#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "LedModule.h"
#include "LaserModule.h"
#include "AccelerometerModule.h"
// put function declarations here:

// 配置参数
static const int        VEHICLE_DETECTION_THRESHOLD = 500;    // 车辆检测阈值（厘米）
static const uint32_t   VEHICLE_TIMEOUT             = 10000;  // 车辆超时时间（毫秒）



Laser laser;
Accelerometer accelerometer;

// 任务句柄
TaskHandle_t laserTaskHandle = NULL;
TaskHandle_t AccTaskHandle = NULL;
TaskHandle_t LedTaskHandle = NULL;

// 任务函数
void laserTask(void *pvParameters);
void accelerometerTask(void* pvParameters);
// void ledTask(void* pvParameters);

// 辅助函数
// void processLaserData(int16_t distance);


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
    Serial.println("激光模块初始化完成");
    
  // 任务主循环
    while (1) {
        int16_t distance = laser.receiveReadResponse();
        if (distance != -1) {
            Serial.println(distance); 
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