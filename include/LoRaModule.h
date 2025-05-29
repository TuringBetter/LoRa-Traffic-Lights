#pragma once
#include <Arduino.h>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct ScheduledCommand 
{
    uint8_t port;
    String payload;
    uint32_t executeTime;
};


extern TaskHandle_t loraTestTaskHandle;
extern TaskHandle_t latencyTaskHandle;  // 延迟测量任务句柄
extern TaskHandle_t ledAutoShutDownTaskHandle;  // 延迟测量任务句柄

void loraTestTask(void* pvParameters);
void latencyTask(void* pvParameters);  // 延迟测量任务函数
// void ledAutoShutDownTask(void* pvParameters);

void LoRa_init();
void sendData(const String& payload);