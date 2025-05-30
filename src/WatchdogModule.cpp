#include "WatchdogModule.h"

TaskHandle_t WatchdogTaskHandle = NULL;

// 看门狗配置参数
static const uint32_t WDT_TIMEOUT_SECONDS = 30;    // 看门狗超时时间（秒）
static const uint32_t FEED_INTERVAL_MS = 2000;    // 喂狗间隔（毫秒）

void Watchdog_init()
{
    // 初始化任务看门狗
    esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true); // 启用看门狗，超时后重启系统
}

void watchdogTask(void *pvParameters)
{
    // 将看门狗任务自身添加到TWDT
    esp_task_wdt_add(NULL);
    
    while(true) {
        // 喂狗
        esp_task_wdt_reset();
        
        // 任务延时
        vTaskDelay(pdMS_TO_TICKS(FEED_INTERVAL_MS));
    }
}

void subscribeTaskToWatchdog(TaskHandle_t taskHandle)
{
    if (taskHandle != NULL) {
        esp_task_wdt_add(taskHandle);
        Serial.printf("Task 0x%08x subscribed to watchdog\n", (uint32_t)taskHandle);
    }
}

void feedTaskWatchdog()
{
    //esp_task_wdt_reset();
    ets_printf("reboot\n");
    esp_restart();
} 