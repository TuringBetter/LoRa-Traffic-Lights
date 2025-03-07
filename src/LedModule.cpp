#include "LedModule.h"

void Led::begin(uint8_t pin)
{
    _pin = pin;
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);  // 设置默认为低电平
}

void Led::on()
{
    digitalWrite(_pin, HIGH);  // 输出高电平点亮 LED
}

void Led::off()
{
    digitalWrite(_pin, LOW);   // 输出低电平熄灭 LED
}