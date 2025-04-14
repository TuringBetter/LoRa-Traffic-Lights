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

private:
    void handlePayload(uint8_t port, const String& payload);
};
