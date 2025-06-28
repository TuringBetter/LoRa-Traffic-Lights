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


extern TaskHandle_t loraReceiveTaskHandle;
extern TaskHandle_t latencyTaskHandle;  // 延迟测量任务句柄
extern TaskHandle_t heartBeatTaskHandle;  // 延迟测量任务句柄

void loraReceiveTask(void* pvParameters);
void latencyTask(void* pvParameters);  // 延迟测量任务函数
void heartBeatTask(void* pvParameters);  // 延迟测量任务函数

void LoRa_init();
void sendData(const String& payload);

void LoRa_init_IDF();

void joinNetwork_IDF(bool joinMode);
void addMuticast_IDF(const String& DevAddr, const String& AppSKey, const String& NwkSKey);
