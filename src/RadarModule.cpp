#include <Arduino.h>
// #include "LedModule.h"
// #include "LoRaModule.h"
#include "RadarModule.h"

// 定义GPIO引脚
static const int RADAR_GPIO_PIN = 6; // GPIO6引脚

static bool _vehicleDetected = false; // 车辆检测状态

// extern SemaphoreHandle_t _ledStateMutex; // LED状态互斥量
// extern bool _ledStateChanged; // LED状态变化标志
// extern volatile LedState ledstate; // LED状态

TaskHandle_t radarTaskHandle   = NULL; // 激光任务句柄

// 函数声明
static void processRadarData(); // 处理雷达数据

/* 初始化雷达模块 */
void Radar_init() {
    pinMode(RADAR_GPIO_PIN, INPUT); // 设置GPIO6为输入模式
    // Serial.println("Radar module initialized...");
}

/* 雷达任务 */
void radarTask(void *pvParameters) {
    // 任务主循环
    while (1) 
    {
        processRadarData(); // 处理雷达数据
//        vTaskDelay(pdMS_TO_TICKS(10)); // 延时10毫秒
    }
}

/* 处理雷达数据 */
static void processRadarData() {

    // static LedColor _last_color; // 上一个LED颜色
    // static uint16_t _last_frequency; // 上一个LED频率
    // static uint16_t _last_brightness; // 上一个LED亮度

    int radarValue = digitalRead(RADAR_GPIO_PIN); // 读取GPIO6的电平状态

    // 检查是否有车辆接近
    if (radarValue == HIGH) { // 高电平表示有车辆
        if (!_vehicleDetected) {
            _vehicleDetected = true; // 标记为检测到车辆
            Serial.println("Detected vehicle");

            // 保存当前LED状态
            // _last_color = ledstate.color; // 保存当前LED颜色
            // _last_frequency = ledstate.frequency; // 保存当前LED频率
            // _last_brightness = ledstate.brightness; // 保存当前LED亮度

            // 控制LED灯状态
            // if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) {
                // 更新灯状态
                // ledstate.color = LedColor::RED; // 设置LED颜色为红色
                // ledstate.frequency = 0; // 设置LED频率
                // ledstate.brightness = 7000; // 设置LED亮度

            // _ledStateChanged = true; // 标记LED状态已改变
            // xSemaphoreGive(_ledStateMutex); // 释放互斥量
            // }
        }
    } 
    else if(radarValue == LOW) { // 低电平表示无车辆
        if (_vehicleDetected) {
            _vehicleDetected = false; // 标记为未检测到车辆
            Serial.println("No vehicle detected");
            // 更新LED状态
            // if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) 
            // {
                // 恢复为历史状态
                // ledstate.color = _last_color; // 恢复LED颜色
                // ledstate.frequency = _last_frequency; // 恢复LED频率
                // ledstate.brightness = _last_brightness; // 恢复LED亮度

                // _ledStateChanged=true; // 标记LED状态已改变
                /**/
                // xSemaphoreGive(_ledStateMutex); // 释放互斥量
            // }
        }
    }
    else 
    {
        Serial.println("Radar ERROR!");
    }
}

