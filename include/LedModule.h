#pragma once
#include <Arduino.h>

enum class LedColor {
    RED,
    YELLOW
};

class Led {
private:
    static const uint8_t RED_PIN = 7;    // 使用GPIO7作为红色LED控制
    static const uint8_t YELLOW_PIN = 8; // 使用GPIO8作为黄色LED控制
    
    LedColor currentColor;
    uint32_t brightness;  // PWM亮度值
    uint16_t frequency;   // 闪烁频率(Hz)
    bool isBlinking;
    unsigned long lastToggleTime;
    bool currentState;

    void updatePWM();
    uint8_t getCurrentPin() const;

public:
    Led();
    
    // 设置当前LED颜色
    void setColor(LedColor color);
    
    // 设置LED亮度
    void setBrightness(uint32_t value);  // 500/1000/2000/4000/7000
    
    // 设置闪烁频率
    void setFrequency(uint16_t freq);    // 30/60/120
    
    // 需要在主循环中调用以实现闪烁效果
    void update();
};
