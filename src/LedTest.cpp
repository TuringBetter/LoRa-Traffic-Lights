#include "LedTest.h"

// 定义LED引脚
static const uint8_t RED_PIN = 45; // 使用GPIO45作为红色LED控制

void LedTest_init() {
    // 配置GPIO引脚
    pinMode(RED_PIN, OUTPUT); // 设置GPIO45为输出模式

    // 点亮LED
    vTaskDelay(pdMS_TO_TICKS(5000));
    digitalWrite(RED_PIN, HIGH); // 设置GPIO45为高电平，点亮LED
    Serial.println("红色LED已点亮");
}

void LedTest_task(void *pvParameters) {
    // 初始化LED
    LedTest_init();

    while (true) {
        // 这里可以添加其他逻辑或延时
        vTaskDelay(pdMS_TO_TICKS(1000)); // 每秒打印一次
    }
}