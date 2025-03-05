#include <Arduino.h>
#include "LoRaModule.h"
#include "LedController.h"

// put function declarations here:

LoRaModule lora;
//LedController led(4);  // 使用GPIO4作为LED引脚

void setup() {
  Serial.begin(115200);
  
  /* 初始化LED */
  led.begin();
  led.setBlinkFrequency(1000);  // 默认1秒闪烁一次
  
  /* 初始化LoRa模块 */
  lora.begin(); // 初始化LoRa模块并读取启动信息
  

  /***********以下为临时测试使用 ***********/

  /* 设置本地地址 */
  int localAddress = 15; // 要设置的本地地址
  if (lora.setLocalAddress(localAddress)) {
    Serial.println("[Debug Serial]:Local address set successfully."); // 设置成功
  } 
  else {
    Serial.println("[Debug Serial]:Failed to set local address."); // 设置失败
  }

  /* 设置目标地址 *
  int targetAddress = 12;
  if (lora.setTargetAddress(targetAddress)) {
    Serial.println("[Debug Serial]:Target address set successfully."); // 设置成功
  } 
  else {
    Serial.println("[Debug Serial]:Failed to set target address."); // 设置失败
  }

  /* 设置接收参数 */
  LoRaTransConfigStruct rxConfig;
  rxConfig.freq = 470500000;      // 设置频率
  rxConfig.dataRate = SF7;        // 设置数据速率
  rxConfig.bandwidth = BW_125KHz; // 设置带宽
  rxConfig.codeRate = CR_4_5;     // 设置编码率
  rxConfig.iqConverted = IQ_ON;   //设置IQ调制开关

  if (lora.setRxConfig(&rxConfig)) {
    Serial.println("[Debug Serial]:Rx configuration set successfully."); // 设置成功
  } 
  else {
    Serial.println("[Debug Serial]:Failed to set Rx configuration.");   // 设置失败
  }

  /* 设置发射参数 *
  LoRaTransConfigStruct txConfig;
  txConfig.freq = 470500000;      // 设置频率
  txConfig.dataRate = SF7;        // 设置数据速率
  txConfig.bandwidth = BW_125KHz; // 设置带宽
  txConfig.codeRate = CR_4_5;     // 设置编码率
  txConfig.iqConverted = IQ_ON;   // 设置IQ调制开关
  txConfig.power = 21;            // 设置发射功率

  if (lora.setTxConfig(&txConfig)) {
    Serial.println("[Debug Serial]:Tx configuration set successfully."); // 设置成功
  } 
  else {
    Serial.println("[Debug Serial]:Failed to set Tx configuration.");   // 设置失败
  }

  /* 设置发射内容 *
  String dataToSend = "1122334455 AABBCC 6677889900";
  if (lora.sendData(dataToSend)) {
        Serial.println("[Debug Serial]:Data sent successfully."); // 发送成功
    } else {
        Serial.println("[Debug Serial]:Failed to send data."); // 发送失败
    }
  
  /**********************/

}

void loop() {
  /* 以下暂定为LED测试 */
  // 更新LED状态
  led.update();
  
  // 检查串口命令
  if (Serial1.available()) {
    Serial.println("[Debug Serial]:Start to receive data.");
    Serial1.readStringUntil('\n');
    Serial1.readStringUntil('\n');
    String command = Serial1.readStringUntil('\n');
    command.trim();// 去除空格和换行符
    Serial.println(command);
    // 解析命令
    if (command.startsWith("freq")) {
      // 设置闪烁频率，格式：freq 1000
      int freq = command.substring(5).toInt();
      led.setBlinkFrequency(freq);
      Serial.println("[Debug Serial]:Set LED frequency to " + String(freq) + "ms");
    }
    else if (command == "on") {
      led.turnOn();
      Serial.println("[Debug Serial]:LED turned on");
    }
    else if (command == "off") {
      led.turnOff();
      Serial.println("[Debug Serial]:LED turned off");
    }
    else if (command.startsWith("bright")) {
      // 设置亮度，格式：bright 255
      int brightness = command.substring(7).toInt();
      led.setBrightness(brightness);
      Serial.println("[Debug Serial]:Set LED brightness to " + String(brightness));
    }
  }
  /**/
}

// put function definitions here:
