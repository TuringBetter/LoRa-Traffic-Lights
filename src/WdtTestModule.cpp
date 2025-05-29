#include "WdtTestModule.h"
#include "WatchdogModule.h"

TaskHandle_t WdtTestTaskHandle = NULL;

void WdtTest_init()
{
    // 初始化测试按键
    pinMode(WDT_TEST_BTN_PIN, INPUT_PULLUP);
    Serial.println("看门狗测试模块初始化完成");
}

void wdtTestTask(void *pvParameters)
{
    // 初始化按键
    WdtTest_init();
    
    while(true) {
        // 检测按键状态
        if (digitalRead(WDT_TEST_BTN_PIN) == LOW) {
            Serial.println("看门狗测试按键按下，延迟1秒后喂狗");
            // 按下按键后延迟6秒再喂狗，这会触发看门狗重启
            vTaskDelay(pdMS_TO_TICKS(1000));
            feedTaskWatchdog();
        }
        
        // 正常情况下每100ms检测一次按键
        vTaskDelay(pdMS_TO_TICKS(100));
    }
} 