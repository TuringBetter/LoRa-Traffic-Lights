#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

enum class LedColor 
{
    RED,
    YELLOW
};

struct LedState
{
    LedColor color;
    uint32_t brightness;
    uint16_t frequency;
};


extern TaskHandle_t LedTestTaskHandle;
extern TaskHandle_t LedTaskHandle;
void ledTestTask(void *pvParameters);
void ledTask(void* pvParameters);

extern bool                _ledStateChanged;
extern SemaphoreHandle_t   _ledStateMutex;
extern volatile LedState             ledstate;

void Led_init();
void setState(const volatile LedState& ledstate);
void update();
