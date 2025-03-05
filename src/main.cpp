#include <Arduino.h>
#include "LoRaModule.h"

// put function declarations here:

LoRaModule lora;

void setup() {
  Serial.begin(115200);
  lora.begin(); // 初始化LoRa模块并读取启动信息
  
  // 测试设置本地地址
  int localAddress = 15; // 要设置的本地地址
  int targetAddress = 12;
  if (lora.setLocalAddress(localAddress)) {
    Serial.println("[Debug Serial]:Local address set successfully."); // 设置成功
  } 
  else {
    Serial.println("[Debug Serial]:Failed to set local address."); // 设置失败
  }
  if (lora.setTargetAddress(targetAddress)) {
    Serial.println("[Debug Serial]:Target address set successfully."); // 设置成功
  } 
  else {
    Serial.println("[Debug Serial]:Failed to set target address."); // 设置失败
  }
  // 测试设置接收配置
  LoRaTransConfigStruct txConfig;
  txConfig.freq = 470500000; // 设置频率
  txConfig.dataRate = SF7; // 设置数据速率
  txConfig.bandwidth = BW_125KHz; // 设置带宽
  txConfig.codeRate = CR_4_5; // 设置编码率
  txConfig.iqConverted = IQ_ON;
  txConfig.power = 21;

  if (lora.setTxConfig(&txConfig)) {
    Serial.println("[Debug Serial]:Tx configuration set successfully."); // 设置成功
  } 
  else {
    Serial.println("[Debug Serial]:Failed to set Tx configuration."); // 设置失败
  }

  String dataToSend = "1122334455ABC6677889900";
  if (lora.sendData(dataToSend)) {
        Serial.println("[Debug Serial]:Data sent successfully."); // 发送成功
    } else {
        Serial.println("[Debug Serial]:Failed to send data."); // 发送失败
    }

}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
