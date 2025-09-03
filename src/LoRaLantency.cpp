/**
  ******************************************************************************
  * @file    LoRaLantency.cpp
  * @author  陕西交通电子工程科技有限公司
  * @version V1.0.0
  * @date    2025-07-25
  * @brief   LoRa通信延迟测量模块。
  * @details
  * > 模块职责:
  * 本模块用于测量设备与LoRa网络服务器之间的通信延迟。
  *
  * > 工作流程:
  * - `latencyTask`任务会周期性地向服务器发送一个特定的探测包。
  * - 当服务器回应此探测包时，`LoRaHandler`会调用本模块的`CalcLantency`函数。
  * - `CalcLantency`函数通过记录的发送时间和接收时间，计算出通信往返时延的
  * 一半，作为单向延迟的估算值。
  *
  * > 数据用途:
  * 计算出的延迟值是`SyncTime`模块实现高精度时间同步的关键补偿参数。
  *
  ******************************************************************************
  */

#include "LoRaLantency.h"
#include "LoRaModule.h"

TaskHandle_t latencyTaskHandle = NULL;  // 延迟测量任务句柄
// static const uint32_t   SYNC_LANTENCY         = 1000 ;  // 同步延迟时间（1秒）


static uint32_t LENTENCY               = 0     ;        // 通信延迟时间
static uint32_t SEND_TIME              = 0     ;        // 发送时间
static uint32_t RECV_TIME              = 0     ;        // 接收时间


static void measureLatency();

void latencyTask(void *pvParameters)
{
    // const TickType_t xDelay = pdMS_TO_TICKS(1*30*1000);  // 每10min测量一次延迟
    const TickType_t xDelay = pdMS_TO_TICKS(30*60*1000);  // 每10min测量一次延迟

    // 初始化随机数种子，使用 esp_timer_get_time() 提供更高的随机性
    randomSeed(esp_timer_get_time());

    vTaskDelay(pdMS_TO_TICKS(random(2 * 1000, 8 * 1000)));    // 短暂延时等待LoRa初始化完成
    // Serial.println("[LatencyTask] First Sync.");
    while(true) {
        // 测量通信延迟
        measureLatency();
        /** *
        // 获取并打印延迟值
        uint32_t latency = getLatency();
        if(latency!=0)
        {
            Serial.print("当前通信延迟: ");
            Serial.print(latency);
            Serial.println(" ms");
        }
        /** */
        uint32_t randomDelayMs = random(0, 5 * 60 * 1000); // 随机生成 0 到 300000 之间的毫秒数
        TickType_t totalDelay = xDelay + pdMS_TO_TICKS(randomDelayMs);
        // 任务延时
        vTaskDelay(xDelay);
    }    
}

uint32_t getDelay()
{
    if (LENTENCY == 0) return 0;  // 或者按需求决定怎么处理 0

    uint32_t upperBound = ((LENTENCY - 1) / 2000 + 1) * 2000;
    return upperBound - LENTENCY;
}


void CalcLantency()
{
    RECV_TIME = millis();
    LENTENCY=(RECV_TIME-SEND_TIME)/2;
    /*
    Serial.print("current delay:");
    Serial.print(getDelay());
    Serial.println(" ms");
    */
}

uint32_t getLantency(){
    return LENTENCY;
}

void measureLatency()
{
    // Serial.println("send measure lantency instruction");
    sendData("06");
    SEND_TIME = millis();
}