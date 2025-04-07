#include <Arduino.h>
#include "LoRaModule.h"
#include "LedModule.h"
#include "LaserModule.h"
#include "AccelerometerModule.h"

#define I2C_SDA 6
#define I2C_SCL 5

Accelerometer accelerometer;

// put function declarations here:
void printEspId();



void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  
  if (!accelerometer.begin(Accelerometer::RANGE_2G)) 
  {
      Serial.println("Failed to initialize accelerometer!");
      while(1);
  }
  Serial.println("Succeeded to initialize accelerometer!");
}

void loop() {
    int16_t x, y, z;
    accelerometer.readRaw(x, y, z);

    Serial.print("Raw Data - X:");
    Serial.print(x);
    Serial.print(" Y:");
    Serial.print(y);
    Serial.print(" Z:");
    Serial.println(z);

    float scale = accelerometer.getScaleFactor();
    Serial.print("Acceleration(g) - X:");
    Serial.print(x * scale, 4);
    Serial.print(" Y:");
    Serial.print(y * scale, 4);
    Serial.print(" Z:");
    Serial.println(z * scale, 4);

    delay(500);
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
