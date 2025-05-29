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
#define COLOR_RED       0xFF0000
#define COLOR_YELLOW    0xFFFF00

// 闪烁频率定义
#define BLINK_RATE_30      30
#define BLINK_RATE_60      60
#define BLINK_RATE_120     120

extern TaskHandle_t      LED_WS2812_TaskHandle;
extern TaskHandle_t      LED_StatusChange_TaskHandle;
extern LED_Control_t     ledControl;
extern SemaphoreHandle_t ledControlMutex;

// 初始化函数
void LED_WS2812_init();

// 任务函数
void LED_WS2812_Task(void *pvParameters);
void LED_StatusChange_Task(void *pvParameters);

// 外部接口函数
bool LED_WS2812_SetState(LED_Control_t newState);
