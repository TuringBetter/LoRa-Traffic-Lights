#include "LedController.h"

// 构造函数，初始化LED控制器
LedController::LedController(uint8_t pin) : 
    _pin(pin),                      // 设置LED引脚
    _isOn(false),                  // 初始化LED状态为关闭
    _blinkInterval(1000),          // 默认闪烁间隔为1秒
    _lastToggleTime(0),            // 上次状态切换时间初始化为0
    _brightness(255),              // 默认最大亮度
    _isBlinking(false)             // 初始化为非闪烁模式
{
}

// 初始化LED引脚
void LedController::begin() {
    pinMode(_pin, OUTPUT);          // 设置引脚为输出模式
    digitalWrite(_pin, LOW);        // 确保LED初始状态为关闭
}

// 设置LED闪烁频率
void LedController::setBlinkFrequency(uint32_t frequency_ms) {
    _blinkInterval = frequency_ms;  // 更新闪烁间隔
    _isBlinking = true;              // 设置为闪烁模式
}

// 设置LED亮度
void LedController::setBrightness(uint8_t brightness) {
    _brightness = brightness;        // 更新亮度值
    if (_isOn) {                    // 如果LED当前是开启状态
        analogWrite(_pin, _brightness); // 设置LED的亮度
    }
}

// 开启LED
void LedController::turnOn() {
    _isOn = true;                   // 更新状态为开启
    _isBlinking = false;            // 关闭闪烁模式
    analogWrite(_pin, _brightness); // 设置LED亮度
}

// 关闭LED
void LedController::turnOff() {
    _isOn = false;                  // 更新状态为关闭
    _isBlinking = false;            // 关闭闪烁模式
    digitalWrite(_pin, LOW);        // 确保LED关闭
}

// 更新LED状态（非阻塞）
void LedController::update() {
    if (!_isBlinking) return;       // 如果不是闪烁模式，直接返回
    
    uint32_t currentTime = millis(); // 获取当前时间
    // 检查是否到达下一个闪烁时间
    if (currentTime - _lastToggleTime >= _blinkInterval) {
        _isOn = !_isOn;             // 切换LED状态
        _lastToggleTime = currentTime; // 更新上次切换时间
        if (_isOn) {
            analogWrite(_pin, _brightness); // 如果开启，设置亮度
        } else {
            digitalWrite(_pin, LOW); // 如果关闭，确保LED关闭
        }
    }
} 