#pragma once
#include <Arduino.h>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>



extern TaskHandle_t loraReceiveTaskHandle;
extern TaskHandle_t heartBeatTaskHandle;  // 心跳任务句柄


void loraReceiveTask(void* pvParameters);
void heartBeatTask(void* pvParameters);  // 心跳任务函数

void LoRa_init();
void sendData(const String& payload);

void LoRa_init_IDF();