#include <Arduino.h>
#include "TrafficLightController.h"

// 创建交通灯控制器实例
// 使用ESP32的MAC地址作为交通灯ID
TrafficLightController* trafficLightController = nullptr;

void setup() {
    // 初始化串口
    Serial.begin(115200);
    Serial.println("Traffic Light System Starting...");
    
    // 获取ESP32的MAC地址作为交通灯ID
    uint64_t chipId = ESP.getEfuseMac();
    uint32_t lightId = (uint32_t)chipId;  // 使用低32位作为ID
    
    // 创建交通灯控制器
    trafficLightController = new TrafficLightController(lightId);
    
    // 初始化交通灯控制器
    trafficLightController->begin();
}

void loop() {
    // 更新交通灯控制器
    if (trafficLightController != nullptr) {
        trafficLightController->update();
    }
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
