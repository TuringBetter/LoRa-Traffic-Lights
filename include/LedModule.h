#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

enum class LedColor 
{
    RED,
    YELLOW
};

struct LedState
{
    LedColor color;
    uint32_t brightness;
    uint16_t freq;
};


extern TaskHandle_t LedTestTaskHandle;
extern TaskHandle_t LedTaskHandle;
void ledTestTask(void *pvParameters);
void ledTask(void* pvParameters);

extern bool                _ledStateChanged;
extern SemaphoreHandle_t   _ledStateMutex;
extern LedState             ledstate;

void Led_init();
void setState(const LedState& ledstate);
// void setColor(LedColor color);
// void setBrightness(uint32_t brightness);  // 500/1000/2000/4000/7000
// void setFrequency(uint16_t freq);    // 30/60/120

// void setState(LedColor color,uint32_t brightness,uint16_t freq);

/**
class Led {
private:
    static const uint8_t RED_PIN    = 45;    // 使用GPIO7作为红色LED控制
    static const uint8_t YELLOW_PIN = 46; // 使用GPIO8作为黄色LED控制
    
    LedColor        currentColor;
    uint32_t        brightness;  // PWM亮度值
    uint16_t        frequency;   // 闪烁频率(Hz)
    bool            isBlinking;
    unsigned long   lastToggleTime;
    bool            currentState;

    void updatePWM();
    uint8_t getCurrentPin() const;

public:
    Led();
    void begin();

    // 设置当前LED颜色
    void setColor(LedColor color);
    
    // 设置LED亮度
    void setBrightness(uint32_t value);  // 500/1000/2000/4000/7000
    
    // 设置闪烁频率
    void setFrequency(uint16_t freq);    // 30/60/120

    // void setState(const LedState& ledState);
    
    // 需要在主循环中调用以实现闪烁效果
    void update();
};
/**/