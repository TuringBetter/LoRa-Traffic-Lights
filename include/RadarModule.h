#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

extern TaskHandle_t         radarTaskHandle;
extern SemaphoreHandle_t    radarStateMutex; // 确保这个是 extern

void radarTask(void *pvParameters);

void Radar_init();

// 判断雷达是否激活或处于延长闪烁状态的函数
extern bool Radar_IsActiveOrExtending();