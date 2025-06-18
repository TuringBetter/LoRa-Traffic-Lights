#pragma once
#include <Arduino.h>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t latencyTaskHandle;  // 延迟测量任务句柄

void latencyTask(void* pvParameters);  // 延迟测量任务函数

uint32_t getDelay();

void CalcLantency();