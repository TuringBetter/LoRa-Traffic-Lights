#pragma once
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t AccTaskHandle;
extern TaskHandle_t AccMonitorTaskHandle;

enum Range {
    RANGE_2G = 0x00,
    RANGE_4G = 0x01,
    RANGE_8G = 0x02,
    RANGE_16G = 0x03
};

void Acc_init();

void accelerometerTask(void* pvParameters);

void accMonitorTask(void* pvParameters);