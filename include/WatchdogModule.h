#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_task_wdt.h"

extern TaskHandle_t WatchdogTaskHandle;

void Watchdog_init();
void watchdogTask(void* pvParameters);

// 为其他任务提供的看门狗订阅和喂狗函数
void subscribeTaskToWatchdog(TaskHandle_t taskHandle);
void feedTaskWatchdog(); 