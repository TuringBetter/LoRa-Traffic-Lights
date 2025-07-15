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
#define COLOR_OFF       0x000000

// 闪烁频率定义
#define BLINK_RATE_30      30
#define BLINK_RATE_60      60
#define BLINK_RATE_120     120


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

// 新增：供 RadarModule 调用的函数
void LED_WS2812_ForceSetState(const LED_Control_t& newState); // 雷达激活时强制设置
void LED_WS2812_ApplyPendingOrRestore(const LED_Control_t& restoreState); // 雷达结束时应用缓存或恢复