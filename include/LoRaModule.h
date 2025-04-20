#pragma once
#include <Arduino.h>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct ScheduledCommand 
{
    uint8_t port;
    String payload;
    uint32_t executeTime;
};


extern TaskHandle_t loraTestTaskHandle;
extern TaskHandle_t latencyTaskHandle;  // 延迟测量任务句柄
void loraTestTask(void* pvParameters);
void latencyTask(void* pvParameters);  // 延迟测量任务函数

void LoRa_init();
void sendData(const String& payload);
/**
class LoRa {
public:
    enum SendMode {
        UNCONFIRMED = 0,
        CONFIRMED = 1
    };

    void begin();
    void sendData(SendMode mode, uint8_t trials, const String& payload);
    void receiveData();
    void measureLatency();  // 测量通信延迟
    uint32_t getLatency();  // 获取当前延迟值

private:
    void handlePayload(uint8_t port, const String& payload);
    void scheduleCommand(uint8_t port, const String& payload, uint32_t delay_ms);  // 延迟执行命令
    
    // 延迟测量相关变量
    uint32_t LoRa_Connect_Delay = 0;    // 通信延迟时间
    uint32_t LoRa_Send_TIME = 0;        // 发送时间
    uint32_t LoRa_Recv_TIME = 0;        // 接收时间
    bool waitingForResponse = false;     // 是否在等待响应
    SemaphoreHandle_t latencySemaphore;  // 延迟测量完成信号量

    // 同步控制相关变量
    static const uint32_t SYNC_DELAY_MS = 1000;  // 同步延迟时间（1秒）
    ScheduledCommand scheduledCommand;  // 存储待执行的命令
    bool hasScheduledCommand = false;   // 是否有待执行的命令
};
/**/