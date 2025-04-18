#pragma once
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t AccTaskHandle;

enum Range {
    RANGE_2G = 0x00,
    RANGE_4G = 0x01,
    RANGE_8G = 0x02,
    RANGE_16G = 0x03
};

void Acc_init();
void processDate(int16_t x, int16_t y, int16_t z);
void readRaw(int16_t &x, int16_t &y, int16_t &z);

void accelerometerTask(void* pvParameters);
