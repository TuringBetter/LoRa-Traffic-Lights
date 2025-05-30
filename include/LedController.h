#pragma once
#include <Arduino.h>

class LedController {
public:
    LedController(uint8_t pin);
    void begin();
    void setBlinkFrequency(uint32_t frequency_ms);  // 设置闪烁频率（毫秒）
    void setBrightness(uint8_t brightness);         // 设置亮度（0-255）
    void turnOn();                                  // 开启LED
    void turnOff();                                // 关闭LED
    void update();                                 // 更新LED状态（非阻塞）

private:
    uint8_t _pin;                    // LED引脚
    bool _isOn;                      // LED当前状态
    uint32_t _blinkInterval;         // 闪烁间隔（毫秒）
    uint32_t _lastToggleTime;        // 上次状态切换时间
    uint8_t _brightness;             // LED亮度
    bool _isBlinking;                // 是否在闪烁模式
}; 