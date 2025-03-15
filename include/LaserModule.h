#pragma once

#include <Arduino.h>

#define READ_DATA_LENGTH 23


class Laser {
public:
    Laser(HardwareSerial &serial, uint32_t baudRate = 921600): _serial(serial), _baudRate(baudRate) {}
    void begin();
    void sendReadCommand();
    void sendoverCommand();
    int16_t receiveReadResponse();

private:
    HardwareSerial &_serial;
    uint32_t _baudRate;
};
