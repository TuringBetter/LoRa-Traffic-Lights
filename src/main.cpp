#include <Arduino.h>
#include "LoRaModule.h"

// put function declarations here:

LoRaModule lora;

void setup() {
  Serial.begin(115200);
  lora.begin(); // 初始化LoRa模块并读取启动信息

  // 测试设置本地地址
  int localAddress = 12; // 要设置的本地地址
  if (lora.setLocalAddress(localAddress)) 
  {
    Serial.println("[Debug Serial]:Local address set successfully."); // 设置成功
  } 
  else 
  {
    Serial.println("[Debug Serial]:Failed to set local address."); // 设置失败
  }
  // 测试设置接收配置
  LoRaRxConfigStruct rxConfig;
  rxConfig.freq = 868100000; // 设置频率
  rxConfig.dataRate = SF7; // 设置数据速率
  rxConfig.bandwidth = BW_125KHz; // 设置带宽
  rxConfig.codeRate = CR_4_5; // 设置编码率
  rxConfig.iqConverted = 0; //设置IQ调制开关

  if (lora.setRxConfig(&rxConfig)) 
  {
    Serial.println("[Debug Serial]:Rx configuration set successfully."); // 设置成功
  } 
  else 
  {
    Serial.println("[Debug Serial]:Failed to set Rx configuration."); // 设置失败
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
