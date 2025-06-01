#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void LedTest_init();
void LedTest_task(void *pvParameters);

extern TaskHandle_t LedTestTaskHandle;