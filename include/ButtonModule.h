#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t ButtonTaskHandle;

void Button_init();
void buttonTask(void* pvParameters);