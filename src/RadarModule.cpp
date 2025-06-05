#include <Arduino.h>
#include "RadarModule.h"
#include "LED_WS2812Module.h"

// 定义GPIO引脚
static const int RADAR_GPIO_PIN         = 16     ; // GPIO16引脚
static const int LED_GPIO_PIN           = 45    ; // GPIO45引脚

static bool     _vehicleDetected        = false ; // 车辆检测状态
static const uint32_t   LED_ON_DELAY    = 2500  ;  // 车辆超时时间（毫秒）

TaskHandle_t            radarTaskHandle = NULL  ; // 雷达任务句柄

// 函数声明
static void processRadarData(); // 处理雷达数据

/* 初始化雷达模块 */
void Radar_init() 
{
    pinMode(RADAR_GPIO_PIN, INPUT); // 设置GPIO6为输入模式
    // pinMode(LED_GPIO_PIN, OUTPUT); // 设置GPIO45为输出模式
}

/* 雷达任务 */
void radarTask(void *pvParameters) {
    // 任务主循环
    while (1) 
    {
        processRadarData(); // 处理雷达数据
        vTaskDelay(pdMS_TO_TICKS(10)); // 延时10毫秒
    }
}

/* 处理雷达数据 */
static void processRadarData() {
    int radarValue = digitalRead(RADAR_GPIO_PIN); // 读取GPIO6的电平状态
    static LED_Control_t last_state;
    static TickType_t vehicleLeaveTime = 0;
    static bool waitingForDelay = false;
    
    static const LED_Control_t RED_ON_STATE
    {
        .isBlinking     = false,
        .blinkRate      = BLINK_RATE_60,
        .brightness     = 255,
        .color          = COLOR_RED
    };

    // 检查是否有车辆接近
    if (radarValue == HIGH) 
    {
        if (!_vehicleDetected) 
        {
            // Serial.println("vehicle detected...");
            _vehicleDetected = true; // 标记为检测到车辆
            // waitingForDelay = false;
            LED_WS2812_GetState(last_state);
            LED_WS2812_SetState(RED_ON_STATE);
        }
    } 
    else
    {
        if (_vehicleDetected) 
        {
            // Serial.println("vehicle left...");
            _vehicleDetected = false; // 标记车辆已经离开
            LED_WS2812_SetState(last_state);
            /* *
            vehicleLeaveTime = xTaskGetTickCount();
            waitingForDelay = true;
            /* */
        }
    }
/** *
    // 检查是否需要恢复LED状态
    if (waitingForDelay && 
        (xTaskGetTickCount() - vehicleLeaveTime >= pdMS_TO_TICKS(LED_ON_DELAY)))
    {
        LED_WS2812_SetState(last_state);
        waitingForDelay = false;
    }
/* **/
}

