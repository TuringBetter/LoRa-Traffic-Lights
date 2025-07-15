#include <Arduino.h>
#include "RadarModule.h"
#include "LED_WS2812Module.h"
#include "SyncTime.h"

// 定义GPIO引脚
static const int RADAR_GPIO_PIN         = 16     ;  // GPIO16引脚

static bool     _vehicleDetected        = false ;   // 车辆检测状态
static bool     waitingForDelay         = false ;   // 灯光延迟闪烁状态
static const uint32_t   LED_ON_DELAY    = 5000  ;   // 车辆超时时间（毫秒）

TaskHandle_t            radarTaskHandle = NULL  ;   // 雷达任务句柄

// 函数声明
static void processRadarData(); // 处理雷达数据

/* 初始化雷达模块 */
void Radar_init() 
{
    pinMode(RADAR_GPIO_PIN, INPUT); // 设置GPIO16为输入模式
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

/* 判断雷达是否激活或处于延长闪烁状态 */
bool Radar_IsActiveOrExtending() {
    Serial.println("Radar_IsActiveOrExtending() called_1");
    return _vehicleDetected || waitingForDelay;
}


/* 处理雷达数据 */
static void processRadarData() {
    int radarValue = digitalRead(RADAR_GPIO_PIN); // 读取GPIO6的电平状态
    static LED_Control_t last_state;    // 保存雷达未激活时的 normal状态
    static TickType_t vehicleLeaveTime = 0;
    
    static const LED_Control_t RED_ON_STATE
    {
        .isBlinking     = true,
        .blinkRate      = BLINK_RATE_60,
        .brightness     = 200,
        .color          = COLOR_RED
    };

    // 检查是否有车辆接近
    if (radarValue == HIGH) 
    {
        if (!_vehicleDetected) 
        {
            Serial.println("vehicle detected...");
            _vehicleDetected = true; // 标记为检测到车辆
            // 如果当前不在延长闪烁状态，说明是新的雷达触发
            if (!waitingForDelay) {
                LED_WS2812_GetState(last_state);    // 获取 normal LED状态
            }
            LED_WS2812_ForceSetState(RED_ON_STATE);     // 设置 LED 为 Radar灯状态

            // 重置延长相关标志和时间，因为现在雷达是激活的
            waitingForDelay = false;
            vehicleLeaveTime = 0;
        }
    } 
    else
    {
        if (_vehicleDetected) 
        {
            // Serial.println("vehicle left...");
            _vehicleDetected = false; // 标记车辆已经离开
            waitingForDelay = true;
            vehicleLeaveTime = xTaskGetTickCount();
        }
    }
/** */
    // 检查是否需要恢复LED状态
    if (waitingForDelay) 
    {
        // 使用 xTaskGetTickCount() 计算经过的时间，与 pdMS_TO_TICKS(LED_ON_DELAY) 比较
        if ((xTaskGetTickCount() - vehicleLeaveTime) >= pdMS_TO_TICKS(LED_ON_DELAY))
        {
            // 延时已结束，恢复到保存的状态
            LED_WS2812_ApplyPendingOrRestore(last_state);
            waitingForDelay = false; // 延时结束，重置标志
            vehicleLeaveTime = 0; // 重置时间戳
        }
    }
/* **/
}

