#include <Arduino.h>
#include "RadarModule.h"

// 定义GPIO引脚
static const int RADAR_GPIO_PIN     = 6; // GPIO6引脚
static const int LED_GPIO_PIN       = 45; // GPIO45引脚

static bool     _vehicleDetected    = false; // 车辆检测状态

TaskHandle_t     radarTaskHandle   = NULL; // 雷达任务句柄

// 函数声明
static void processRadarData(); // 处理雷达数据

/* 初始化雷达模块 */
void Radar_init() 
{
    pinMode(RADAR_GPIO_PIN, INPUT); // 设置GPIO6为输入模式
    pinMode(LED_GPIO_PIN, OUTPUT); // 设置GPIO45为输出模式
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

    // 检查是否有车辆接近
    if (radarValue == HIGH) 
    {
        if (!_vehicleDetected) 
        {
            _vehicleDetected = true; // 标记为检测到车辆
            Serial.println("Detected vehicle");
            digitalWrite(LED_GPIO_PIN, HIGH); // 点亮LED
        }
    } 
    else
    {
        if (_vehicleDetected) 
        {
            _vehicleDetected = false; // 标记为未检测到车辆
            Serial.println("No vehicle detected");
            digitalWrite(LED_GPIO_PIN, LOW); // 熄灭LED
        }
    }
}

