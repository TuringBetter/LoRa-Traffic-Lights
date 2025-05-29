#pragma once

#include <Arduino.h>

#define READ_DATA_LENGTH 23


class Laser_uart {
public:
    void begin();
    void sendReadCommand();
    void sendoverCommand();
    int16_t receiveReadResponse();


private:
    static const uint8_t RX_PIN = 18;
    static const uint8_t TX_PIN = 17;

        // 成员变量
    uint32_t            _lightId;
    // TrafficLightState   _currentState;

};
