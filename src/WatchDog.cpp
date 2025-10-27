/**
  ******************************************************************************
  * @file    WatchDog.cpp
  * @author  陕西交通电子工程科技有限公司
  * @version V1.0.0
  * @date    2025-01-02
  * @brief   看门狗模块实现
  ******************************************************************************
  */

#include "WatchDog.h"
#include "LED_WS2812Module.h"
#include <esp_log.h>

TaskHandle_t WatchDogTaskHandle = NULL;

// 任务心跳记录
struct TaskHeartbeat {
    TickType_t lastFeedTime;  // 最后喂狗时间
    bool isActive;             // 任务是否激活
};

static TaskHeartbeat ledWS2812Heartbeat = {0, false};
static TaskHeartbeat ledStatusChangeHeartbeat = {0, false};
static int failCount = 0;
static SemaphoreHandle_t watchdogMutex = NULL;  // 互斥锁保护心跳数据
static TickType_t watchdogStartTime = 0;        // 看门狗启动时间

/**
  * @brief  初始化看门狗模块
  * @note   初始化互斥锁并重置所有状态
  */
void WatchDog_init()
{
    Serial.println("[WatchDog] Initialized with feed mechanism");
    
    // 创建互斥锁
    watchdogMutex = xSemaphoreCreateMutex();
    if (watchdogMutex == NULL) {
        Serial.println("[WatchDog] ERROR: Failed to create mutex!");
        return;
    }
    
    // 获取互斥锁进行初始化
    if (xSemaphoreTake(watchdogMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        failCount = 0;
        watchdogStartTime = xTaskGetTickCount();  // 记录启动时间
        
        // 初始化时设置初始时间，避免首次检查误判
        ledWS2812Heartbeat.lastFeedTime = watchdogStartTime;
        ledWS2812Heartbeat.isActive = false;
        
        ledStatusChangeHeartbeat.lastFeedTime = watchdogStartTime;
        ledStatusChangeHeartbeat.isActive = false;
        
        xSemaphoreGive(watchdogMutex);
        
        // 打印配置信息
        Serial.printf("[WatchDog] Configuration: Check Interval=%dms, Timeout=%dms, Max Fail Count=%d, Startup Grace=%dms\n",
                     WATCHDOG_CHECK_INTERVAL_MS, WATCHDOG_TIMEOUT_MS, WATCHDOG_MAX_FAIL_COUNT, WATCHDOG_STARTUP_GRACE_MS);
        Serial.println("[WatchDog] Monitoring tasks: LED_WS2812_Task, LED_StatusChange_Task");
    } else {
        Serial.println("[WatchDog] ERROR: Failed to acquire mutex during init!");
    }
}

/**
  * @brief  喂狗函数 - 被监控任务需要定期调用
  * @param  taskName: 任务名称
  */
void WatchDog_feed(const char* taskName)
{
    if (watchdogMutex == NULL) return;  // 互斥锁未初始化
    
    TickType_t currentTime = xTaskGetTickCount();
    
    // 获取互斥锁
    if (xSemaphoreTake(watchdogMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        // 更新对应任务的心跳时间
        if (strcmp(taskName, "LED_WS2812_Task") == 0) {
            ledWS2812Heartbeat.lastFeedTime = currentTime;
            ledWS2812Heartbeat.isActive = true;
        } else if (strcmp(taskName, "LED_StatusChange_Task") == 0) {
            ledStatusChangeHeartbeat.lastFeedTime = currentTime;
            ledStatusChangeHeartbeat.isActive = true;
        }
        
        xSemaphoreGive(watchdogMutex);
    }
}

/**
  * @brief  检查任务心跳状态
  * @param  heartbeat: 任务心跳记录
  * @param  taskName: 任务名称
  * @return true: 任务心跳正常, false: 任务心跳超时
  */
static bool CheckHeartbeat(TaskHeartbeat* heartbeat, const char* taskName)
{
    if (heartbeat == NULL || watchdogMutex == NULL) {
        return false;
    }
    
    TickType_t currentTime;
    TickType_t lastFeedTime;
    bool isActive;
    
    // 获取互斥锁并读取心跳数据
    if (xSemaphoreTake(watchdogMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        currentTime = xTaskGetTickCount();
        lastFeedTime = heartbeat->lastFeedTime;
        isActive = heartbeat->isActive;
        xSemaphoreGive(watchdogMutex);
    } else {
        Serial.println("[WatchDog] WARNING: Failed to acquire mutex in CheckHeartbeat!");
        return false;
    }
    
    // 如果任务还未首次喂狗，检查是否超过启动宽限时间
    if (!isActive) {
        uint32_t timeSinceStart = (currentTime - watchdogStartTime) * portTICK_PERIOD_MS;
        if (timeSinceStart > WATCHDOG_STARTUP_GRACE_MS) {
            Serial.printf("[WatchDog] Warning: %s never sent first heartbeat after %lu ms\n", 
                         taskName, timeSinceStart);
            return false;  // 启动宽限时间已过但仍未喂狗，判定为失败
        }
        return true;  // 仍在启动宽限时间内，等待首次喂狗
    }
    
    TickType_t timeSinceFeed = currentTime - lastFeedTime;
    
    // 转换为毫秒进行比较
    uint32_t timeSinceFeedMs = timeSinceFeed * portTICK_PERIOD_MS;
    
    if (timeSinceFeedMs > WATCHDOG_TIMEOUT_MS) {
        Serial.printf("[WatchDog] Timeout: %s heartbeat expired (%lu ms)\n", 
                     taskName, timeSinceFeedMs);
        return false;
    }
    
    return true;
}

/**
  * @brief  看门狗任务主循环
  * @param  pvParameters: 任务参数（未使用）
  */
void watchdogTask(void *pvParameters)
{
    Serial.println("[WatchDog] Task started with heartbeat monitor");
    
    while (1) {
        bool allTasksOK = true;
        bool ledWS2812OK = true;
        bool ledStatusChangeOK = true;
        
        // 检查 LED_WS2812_Task 心跳
        if (!CheckHeartbeat(&ledWS2812Heartbeat, "LED_WS2812_Task")) {
            allTasksOK = false;
            ledWS2812OK = false;
        }
        
        // 检查 LED_StatusChange_Task 心跳
        if (!CheckHeartbeat(&ledStatusChangeHeartbeat, "LED_StatusChange_Task")) {
            allTasksOK = false;
            ledStatusChangeOK = false;
        }
        
        if (allTasksOK) {
            // 所有任务正常，重置失败计数
            if (failCount > 0) {
                Serial.printf("[WatchDog] All tasks recovered (failCount was %d)\n", failCount);
                failCount = 0;
            }
        } else {
            // 有任务异常，增加失败计数（仅在连续检查失败时累积）
            failCount++;
            Serial.printf("[WatchDog] Warning: Heartbeat failed (count: %d/%d)\n", 
                         failCount, WATCHDOG_MAX_FAIL_COUNT);
            
            // 如果失败次数超过阈值，触发系统重启
            if (failCount >= WATCHDOG_MAX_FAIL_COUNT) {
                Serial.println("\n[WatchDog] ===== CRITICAL ERROR ===== ");
                Serial.println("[WatchDog] System will restart due to watchdog timeout");
                Serial.println("[WatchDog] Failed task(s) detected, please check:");
                
                // 打印具体哪些任务失败了（使用之前保存的状态，避免重复调用CheckHeartbeat）
                if (!ledWS2812OK) {
                    Serial.println("[WatchDog]   - LED_WS2812_Task: TIMEOUT");
                }
                if (!ledStatusChangeOK) {
                    Serial.println("[WatchDog]   - LED_StatusChange_Task: TIMEOUT");
                }
                
                Serial.println("[WatchDog] Restarting in 2 seconds...");
                Serial.println("[WatchDog] ==============================\n");
                
                Serial.flush();  // 确保所有输出都已发送
                vTaskDelay(pdMS_TO_TICKS(2000));  // 给串口足够时间输出
                ESP.restart();  // 重启系统
            }
        }
        
        // 等待下一次检查
        vTaskDelay(pdMS_TO_TICKS(WATCHDOG_CHECK_INTERVAL_MS));
    }
}
