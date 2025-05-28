#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t LED_WS2812_TaskHandle;

void LED_WS2812_init();
void LED_WS2812_Task(void *pvParameters);
