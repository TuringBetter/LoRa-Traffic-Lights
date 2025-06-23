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
    const TickType_t xDelay = pdMS_TO_TICKS(1*20*1000);  // 每10min测量一次延迟
    
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