#pragma once
#include <Arduino.h>
#include <string>
#include "LedModule.h"

// 声明外部变量
extern LedState _ledState;
extern bool _ledStateChanged;
extern SemaphoreHandle_t _ledStateMutex;

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
    
    // 延迟测量相关变量
    uint32_t LoRa_Connect_Delay = 0;    // 通信延迟时间
    uint32_t LoRa_Send_TIME = 0;        // 发送时间
    uint32_t LoRa_Recv_TIME = 0;        // 接收时间
    bool waitingForResponse = false;     // 是否在等待响应
};
