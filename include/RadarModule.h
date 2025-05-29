#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t         radarTaskHandle;

void radarTask(void *pvParameters);

void Radar_init();
