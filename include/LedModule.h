#pragma once
#include <Arduino.h>

class Led
{
public:
    void begin(uint8_t pin);
    void on();
    void off();
private:
    uint8_t _pin;
};
