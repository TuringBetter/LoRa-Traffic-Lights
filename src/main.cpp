#include <Arduino.h>

#include "LedModule.h"

// 创建LED模块实例
LedModule ledModule;

// 测试状态变量
unsigned long lastStateChange = 0;
uint8_t currentTestState = 0;

// put function declarations here:
void printEspId();

void setup() {
  Serial.begin(115200);
  Serial.println("LED模块测试程序启动");
  
  // 初始化LED为红色,默认亮度500,不闪烁
  ledModule.setColor(LedColor::RED);
  ledModule.setBrightness(500);
  ledModule.setFrequency(0);
}

void loop() {
  unsigned long currentTime = millis();
  
  // 每5秒切换一次测试状态
  if (currentTime - lastStateChange >= 5000) {
    currentTestState = (currentTestState + 1) % 6;  // 6个测试状态循环
    lastStateChange = currentTime;
    
    switch (currentTestState) {
      case 0:
        Serial.println("测试1: 红色LED,亮度500,不闪烁");
        ledModule.setColor(LedColor::RED);
        ledModule.setBrightness(500);
        ledModule.setFrequency(0);
        break;
        
      case 1:
        Serial.println("测试2: 红色LED,亮度2000,30Hz闪烁");
        ledModule.setColor(LedColor::RED);
        ledModule.setBrightness(2000);
        ledModule.setFrequency(30);
        break;
        
      case 2:
        Serial.println("测试3: 红色LED,亮度7000,60Hz闪烁");
        ledModule.setColor(LedColor::RED);
        ledModule.setBrightness(7000);
        ledModule.setFrequency(60);
        break;
        
      case 3:
        Serial.println("测试4: 黄色LED,亮度1000,不闪烁");
        ledModule.setColor(LedColor::YELLOW);
        ledModule.setBrightness(1000);
        ledModule.setFrequency(0);
        break;
        
      case 4:
        Serial.println("测试5: 黄色LED,亮度4000,120Hz闪烁");
        ledModule.setColor(LedColor::YELLOW);
        ledModule.setBrightness(4000);
        ledModule.setFrequency(120);
        break;
        
      case 5:
        Serial.println("测试6: 黄色LED,亮度2000,30Hz闪烁");
        ledModule.setColor(LedColor::YELLOW);
        ledModule.setBrightness(2000);
        ledModule.setFrequency(30);
        break;
    }
  }
  
  // 更新LED状态（实现闪烁效果）
  ledModule.update();
}

// put function definitions here:

void printEspId()
{
    // 读取芯片ID
  uint64_t chipId = ESP.getEfuseMac(); // 获取芯片 MAC 地址（作为芯片ID）
  
  // 将64位整数分成两个32位整数以便打印
  uint32_t chipId_high = (uint32_t)(chipId >> 32);
  uint32_t chipId_low = (uint32_t)chipId;
  
  // 打印芯片ID（十六进制格式）
  Serial.printf("ESP32 Chip ID = %08X%08X\n", chipId_high, chipId_low);
  
  // 也可以分别打印高32位和低32位
  Serial.printf("High = 0x%08X\n", chipId_high);
  Serial.printf("Low = 0x%08X\n", chipId_low);
}
