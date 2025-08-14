#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// LED控制结构体
typedef struct 
{
    bool        isBlinking ;     // 是否闪烁
    uint8_t     blinkRate  ;     // 闪烁频率 (30, 60, 120次/分钟)
    uint8_t     brightness ;     // 亮度 (0-255)
    uint32_t    color      ;     // 颜色
} LED_Control_t;

// 颜色定义
#define COLOR_RED       0xFF0000    // 当颜色为 FF0000时是纯红色最大亮度，880000为一半亮度的红色
#define COLOR_YELLOW    0xFF8000    // 当颜色为 FF8000时是纯黄色最大亮度，886000为一半亮度的黄色
#define COLOR_OFF       0x000000

// 闪烁频率定义
#define BLINK_RATE_30      30
#define BLINK_RATE_60      60
#define BLINK_RATE_120     120

// LED数量和分区定义
#define NUM_LEDS            1152       // 总LED数量
#define NUM_YELLOW_LEDS     576       // 黄色区域LED数量 (1-128)
#define NUM_RED_LEDS        576       // 红色区域LED数量 (129-256)
#define YELLOW_LED_START_IDX 0        // 黄色区域起始索引 (0-127)
#define RED_LED_START_IDX   576       // 红色区域起始索引 (128-255)

// 初始化函数
void LED_WS2812_init();
extern SemaphoreHandle_t ledControlMutex                ;

extern TaskHandle_t      LED_WS2812_TaskHandle          ;
extern TaskHandle_t      LED_StatusChange_TaskHandle    ;
extern TaskHandle_t      LED_Test_TaskHandle            ;

// 任务函数
void LED_WS2812_Task(void *pvParameters);

// 测试函数
void LED_StatusChange_Task(void *pvParameters);
void LED_Test_Task(void *pvParameters);

// 外部接口函数
void LED_WS2812_SetState(const LED_Control_t& newState);
void LED_WS2812_SetColor(uint32_t color);
void LED_WS2812_SetBrightness(uint8_t brightness);
void LED_WS2812_SetBlink(bool isBlinking);
void LED_WS2812_SetBlinkRate(uint8_t blinkRate);
void LED_WS2812_GetState(LED_Control_t& curState);  // 获取当前LED状态
void LED_WS2812_switch(bool enable);

void LED_WS2812_TriggerBlinkResync();

// 供 RadarModule 调用的函数
void LED_WS2812_ForceSetState(const LED_Control_t& newState); // 雷达激活时强制设置
void LED_WS2812_ApplyPendingOrRestore(); // 雷达结束时应用缓存或恢复