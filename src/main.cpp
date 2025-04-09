#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "LoRaModule.h"
#include "LedModule.h"
#include "LaserModule.h"
// put function declarations here:
void printEspId();

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
      Serial.print("测量的距离: ");
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
  // 主循环为空，因为所有工作都在FreeRTOS任务中完成
  // vTaskDelay(pdMS_TO_TICKS(1000));
}

// put function definitions here:

void printEspId()
{
    // 读取芯片ID
  uint64_t chipId = ESP.getEfuseMac(); // 获取芯片 MAC 地址（作为芯片ID）
  
  // 将64位整数分成两个32位整数以便打印
  uint32_t chipId_high = (uint32_t)(chipId >> 32);
  uint32_t chipId_low = (uint32_t)chipId;
  
  // 打印芯片ID（十六进制格式）
  Serial.printf("ESP32 Chip ID = %08X%08X\n", chipId_high, chipId_low);
  
  // 也可以分别打印高32位和低32位
  Serial.printf("High = 0x%08X\n", chipId_high);
  Serial.printf("Low = 0x%08X\n", chipId_low);
}
