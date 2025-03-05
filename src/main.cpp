#include <Arduino.h>
#include "LoRaModule.h"

// put function declarations here:

LoRaModule lora;

void setup() {
  Serial.begin(115200);
  lora.begin(); // 初始化LoRa模块并读取启动信息

  // 测试设置本地地址
  int localAddress = 12; // 要设置的本地地址
  int targetAddress = 13;
  lora.setLocalAddress(localAddress);
  lora.setTargetAddress(targetAddress);
  // 测试设置接收配置
  LoRaTransConfigStruct rxConfig;
  rxConfig.freq = 470500000; // 设置频率
  rxConfig.dataRate = SF7; // 设置数据速率
  rxConfig.bandwidth = BW_125KHz; // 设置带宽
  rxConfig.codeRate = CR_4_5; // 设置编码率
  rxConfig.iqConverted = IQ_ON;

  // 测试设置发送配置
  LoRaTransConfigStruct txConfig;
  txConfig.freq = 470500000; // 设置频率
  txConfig.dataRate = SF7; // 设置数据速率
  txConfig.bandwidth = BW_125KHz; // 设置带宽
  txConfig.codeRate = CR_4_5; // 设置编码率
  txConfig.power = 21;
  txConfig.iqConverted = IQ_ON;

  lora.setTxConfig(&txConfig);
  lora.sendData("0505");
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
