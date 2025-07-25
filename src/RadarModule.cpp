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
SemaphoreHandle_t       radarStateMutex = NULL  ;   // 定义互斥锁

bool radarModuleEnabled                 = false ;   // 雷达使能状态

// 函数声明
static void processRadarData();

/* 初始化雷达模块 */
void Radar_init()
{
    pinMode(RADAR_GPIO_PIN, INPUT); // 设置GPIO16为输入模式
    // 创建互斥锁
    radarStateMutex = xSemaphoreCreateMutex();
    if (radarStateMutex == NULL) {
        Serial.println("[Radar] 错误：无法创建雷达状态互斥锁！系统可能不稳定。");
        while(1) { vTaskDelay(pdMS_TO_TICKS(100)); } // 阻止系统继续运行
    }
    radarModuleEnabled = true;
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
    bool isActive = false;
    // 使用互斥锁保护对 _vehicleDetected 和 waitingForDelay 的读取
    if (xSemaphoreTake(radarStateMutex, portMAX_DELAY) == pdTRUE) {
        isActive = _vehicleDetected || waitingForDelay;
        xSemaphoreGive(radarStateMutex);
    } else {
        // Serial.println("[Radar] Warning: Radar_IsActiveOrExtending failed to acquire mutex!");
    }

    return isActive;
}


/* 处理雷达数据 */
static void processRadarData() {
    int radarValue = digitalRead(RADAR_GPIO_PIN);   // 读取GPIO16的电平状态
    // `last_state_for_restore` 不再需要，因为 LED_WS2812_ApplyPendingOrRestore 不再接收参数
    // static LED_Control_t last_state_for_restore; 

    static TickType_t vehicleLeaveTime = 0;

    // 此结构体定义了雷达触发时红色区域的固定参数
    static const LED_Control_t RED_ON_STATE_FOR_RADAR_ZONE 
    {
        .isBlinking     = true,
        .blinkRate      = BLINK_RATE_60,
        .brightness     = 200,
        .color          = COLOR_RED // 必须是红色
    };

    // 使用互斥锁保护所有对 _vehicleDetected 和 waitingForDelay 的读写
    if (xSemaphoreTake(radarStateMutex, portMAX_DELAY) == pdTRUE) {
        if (radarValue == HIGH)
        {
            if (!_vehicleDetected)
            {
                _vehicleDetected = true;
                // 调用 LED_WS2812_ForceSetState，直接将雷达灯状态设置给 actualRedState
                LED_WS2812_ForceSetState(RED_ON_STATE_FOR_RADAR_ZONE); 

                waitingForDelay = false;
                vehicleLeaveTime = 0;
            }
        }
        else // 雷达信号为 LOW (没有车辆)
        {
            if (_vehicleDetected)
            {
                _vehicleDetected = false;
                waitingForDelay = true;
                vehicleLeaveTime = xTaskGetTickCount();
            }
        }
        xSemaphoreGive(radarStateMutex); // 释放互斥锁
    } else {
        // Serial.println("[Radar] Warning: processRadarData failed to acquire mutex for state update!");
    }

    // 检查是否需要恢复LED状态 (延迟计时部分)
    // 再次获取互斥锁来读取和修改 waitingForDelay
    bool currentWaitingForDelay = false;
    if (xSemaphoreTake(radarStateMutex, portMAX_DELAY) == pdTRUE) {
        currentWaitingForDelay = waitingForDelay;
        xSemaphoreGive(radarStateMutex);
    } else {
        // Serial.println("[Radar] Warning: processRadarData (delay check) failed to acquire mutex for read!");
    }

    if (currentWaitingForDelay)
    {
        if ((xTaskGetTickCount() - vehicleLeaveTime) >= pdMS_TO_TICKS(LED_ON_DELAY))
        {
            LED_WS2812_ApplyPendingOrRestore(); 

            // 修正：在修改 waitingForDelay 时也使用互斥锁
            if (xSemaphoreTake(radarStateMutex, portMAX_DELAY) == pdTRUE) {
                waitingForDelay = false; // 延时结束，重置标志
                vehicleLeaveTime = 0; // 重置时间戳
                xSemaphoreGive(radarStateMutex);
            } else {
                // Serial.println("[Radar] Warning: processRadarData (delay end) failed to acquire mutex for write!");
            }
        }
    }
}