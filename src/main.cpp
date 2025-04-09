#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "LoRaModule.h"
#include "LedModule.h"
#include "LaserModule.h"
// put function declarations here:

// LoRaModule lora;
// Led led;
Laser laser;

// 任务句柄
TaskHandle_t laserTaskHandle = NULL;

// 激光测距任务
void laserTask(void *pvParameters) {
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
    
  // 删除setup任务，因为不再需要
    vTaskDelete(NULL);
}

void loop() {

}

// put function definitions here: