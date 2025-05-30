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
        /* 检测按键状态 *
        if (digitalRead(WDT_TEST_BTN_PIN) == LOW) {
            Serial.println("看门狗测试按键按下，延迟1秒后喂狗");
            // 按下按键后延迟6秒再喂狗，这会触发看门狗重启
            vTaskDelay(pdMS_TO_TICKS(1000));
            feedTaskWatchdog();
        }
        
        // 正常情况下每100ms检测一次按键
        vTaskDelay(pdMS_TO_TICKS(100));

        /* 模拟任务卡死 */
        Serial.println("卡死任务开始执行...");

        // 模拟卡死，进入无限循环
        while (1) {

            // 重置看门狗
            //esp_task_wdt_reset();
            // 这里可以添加一些调试信息
            Serial.println("卡死中...");
            // 你可以选择添加一个延时，模拟任务的某种处理
            vTaskDelay(pdMS_TO_TICKS(1000)); // 可选
        }
        /**/
    }
} 