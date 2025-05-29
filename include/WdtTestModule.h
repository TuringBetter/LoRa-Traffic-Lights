#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 看门狗测试任务句柄
extern TaskHandle_t WdtTestTaskHandle;

// 看门狗测试按键引脚定义
#define WDT_TEST_BTN_PIN 1

// 看门狗测试模块初始化
void WdtTest_init();

// 看门狗测试任务
void wdtTestTask(void* pvParameters); 