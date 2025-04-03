#include <Arduino.h>
#include "LoRaModule.h"
#include "LedModule.h"
#include "LaserModule.h"
// put function declarations here:
void printEspId();

// LoRaModule lora;
// Led led;
Laser laser(Serial1);

void setup() {
  Serial.begin(115200);
  Serial.println("程序开始运行...");
  Serial1.begin(921600, SERIAL_8N1, 18, 17);
  delay(1000);  // 等待串口稳定
  laser.begin();
  laser.sendReadCommand();
  Serial.println("System initialized");
}

void loop() {
  int16_t distance = laser.receiveReadResponse();
  if (distance != -1) 
  {
    Serial.print("测量的距离: ");
    Serial.println(distance);
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
