#include "LoRaModule.h"
#include <Arduino.h>

/*初始化LoRa模块*/
void LoRaModule::begin() {
    Serial1.begin(9600, SERIAL_8N1, 18, 17); // 初始化UART，使用GPIO18作为RX1，GPIO17作为TX1
}

/*设置发送参数*/
void LoRaModule::setTxConfig(long freq, int dataRate, int bandwidth, int codeRate, int power, int iqConverted) {
    String command = "AT+CTX=" + String(freq) + "," + String(dataRate) + "," + String(bandwidth) + "," +
                     String(codeRate) + "," + String(power) + "," + String(iqConverted);
    Serial1.println(command); // 发送配置发射参数的AT指令
}

/*设置接收参数*/
bool LoRaModule::setRxConfig(const LoRaRxConfigStruct *pConfig)
{
    if(pConfig==nullptr)
    {
        Serial.println("[Debug Serial]:LoRaRxConfigStruct Error");
        return false;
    }
    String command = "AT+CRX=" + String(pConfig->freq) + "," + String(pConfig->dataRate) + "," +
                     String(pConfig->bandwidth) + "," + String(pConfig->codeRate) + "," + String(pConfig->iqConverted);
    Serial1.println(command); // 发送配置接收参数的AT指令

    // 等待响应
    unsigned long startTime = millis();
    while (millis() - startTime < 2000) { // 等待最多2秒
        if (Serial1.available()) {
            String response = Serial1.readStringUntil('\n'); // 读取一行响应
            Serial.println("[LoRa  Serial]:" + response);
            // 检查响应内容
            if (response.startsWith("params num: 5 ,start to recv package")) {
                // Serial.println("[Debug Serial]:set local address: " + String(localAddr));
                return true; // 设置成功
            }
        }
    }
    Serial.println("[Debug Serial]:UART Time Out Error");
    return false; // 设置失败
}

/*设置本地地址*/
bool LoRaModule::setLocalAddress(int localAddr) {
    String command = "AT+CADDRSET=" + String(localAddr);
    Serial1.println(command); // 发送配置本地地址的AT指令

    // 等待响应
    unsigned long startTime = millis();
    while (millis() - startTime < 2000) { // 等待最多2秒
        if (Serial1.available()) {
            String response = Serial1.readStringUntil('\n'); // 读取一行响应
            Serial.println("[LoRa  Serial]:" + response);
            // 检查响应内容
            if (response.startsWith("set local address:")) {
                Serial.println("[Debug Serial]:set local address: " + String(localAddr));
                return true; // 设置成功
            } else if (response.startsWith("+CMD ERROR")) {
                // 如果收到错误响应，则重新发送AT指令
                Serial.println("[Debug Serial]:Error received, retrying...");
                Serial1.println(command); // 重新发送AT指令
                // 重传次数限制
                static int retryCount = 0;
                if (retryCount < 5) {
                    retryCount++;
                } else {
                    Serial.println("[Debug Serial]:Maximum retry count reached, giving up.");
                    return false; // 重传次数达到上限，设置失败
                }
            }
        }
    }
    return false; // 设置失败
}

/*设置目标地址*/
bool LoRaModule::setTargetAddress(int targetAddr) {
    String command = "AT+CTXADDRSET=" + String(targetAddr);
    Serial1.println(command); // 发送配置目标地址的AT指令

    // 等待响应
    unsigned long startTime = millis();
    while (millis() - startTime < 2000) { // 等待最多2秒
        if (Serial1.available()) {
            String response = Serial1.readStringUntil('\n'); // 读取一行响应
            Serial.println("[LoRa  Serial]:" + response);
            // 检查响应内容
            if (response.startsWith("set target address:")) {
                Serial.println("[Debug Serial]:set target address: " + String(targetAddr));
                return true; // 设置成功
            }
        }
    }
    return false; // 设置失败

}

/*休眠模式*/
void LoRaModule::setSleepMode(int sleepMode) {
    String command = "AT+CSLEEP=" + String(sleepMode);
    Serial1.println(command); // 发送设置睡眠模式的AT指令
}