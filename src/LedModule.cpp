#include "LedModule.h"

Led::Led() : 
    currentColor(LedColor::YELLOW),
    brightness(500),
    frequency(0),
    isBlinking(false),
    lastToggleTime(0),
    currentState(false) {
    
    // 配置GPIO引脚
    pinMode(RED_PIN, OUTPUT);
    pinMode(YELLOW_PIN, OUTPUT);
    
    // 设置LED PWM属性
    // 使用低速通道，频率1KHz，8位分辨率
    ledcSetup(0, 1000, 8);  // 通道0，1KHz PWM，8位分辨率
    ledcSetup(1, 1000, 8);  // 通道1，1KHz PWM，8位分辨率
    
    // 将LED引脚附加到PWM通道
    ledcAttachPin(RED_PIN, 0);
    ledcAttachPin(YELLOW_PIN, 1);
    
    updatePWM();
}

void Led::setColor(LedColor color) {
    if (currentColor != color) {
        // 关闭当前LED
        ledcWrite(currentColor == LedColor::RED ? 0 : 1, 0);
        
        currentColor = color;
        updatePWM();
    }
}

void Led::setBrightness(uint32_t value) {
    // 验证亮度值是否合法
    if (value != 500 && value != 1000 && value != 2000 && 
        value != 4000 && value != 7000) {
        return;
    }
    
    brightness = value;
    updatePWM();
}

void Led::setFrequency(uint16_t freq) {
    // 验证频率值是否合法
    if (freq != 0 && freq != 30 && freq != 60 && freq != 120) {
        return;
    }
    
    frequency = freq;
    isBlinking = (freq > 0);
    
    // 如果频率为0，确保LED保持常亮状态
    if (freq == 0) {
        currentState = true;
        updatePWM();
    }
}

void Led::update() {
    if (!isBlinking) {
        return;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastToggleTime >= (1000 / frequency)) {
        currentState = !currentState;
        lastToggleTime = currentTime;
        updatePWM();
    }
}

void Led::updatePWM() {
    uint32_t pwmValue = currentState ? brightness : 0;
    uint8_t channel = (currentColor == LedColor::RED) ? 0 : 1;
    
    // 将亮度值映射到8位PWM范围(0-255)
    pwmValue = map(pwmValue, 0, 7000, 0, 255);
    ledcWrite(channel, pwmValue);
}

uint8_t Led::getCurrentPin() const {
    return (currentColor == LedColor::RED) ? RED_PIN : YELLOW_PIN;
}
