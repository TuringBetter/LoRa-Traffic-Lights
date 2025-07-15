#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t         radarTaskHandle;

void radarTask(void *pvParameters);

void Radar_init();

// 新增：判断雷达是否激活或处于延长闪烁状态的函数
extern bool Radar_IsActiveOrExtending();