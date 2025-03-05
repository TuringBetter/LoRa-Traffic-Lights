#include "LoRaModule.h"

/*初始化LoRa模块*/
void LoRaModule::begin() {
    Serial1.begin(9600, SERIAL_8N1, 18, 17); // 初始化UART，使用GPIO18作为RX1，GPIO17作为TX1
    Serial1.println("+++");
    this->_currentTransferMode=TransferMode::NONE;
}

/*设置发送参数*/
bool LoRaModule::setTxConfig(const LoRaTransConfigStruct* pConfig) 
{
    if(pConfig==nullptr)
    {
        Serial.println("[Debug Serial]:LoRaRxConfigStruct Error");
        return false;
    }
    String command = "AT+CTX="  + String(pConfig->freq)
                        +  ","  + String(pConfig->dataRate) 
                        +  ","  + String(pConfig->bandwidth) 
                        +  ","  + String(pConfig->codeRate) 
                        +  ","  + String(pConfig->power) 
                        +  ","  + String(pConfig->iqConverted);
    Serial1.println(command); // 发送配置发射参数的AT指令

    // 等待响应
    unsigned long startTime = millis();
    while (millis() - startTime < 2000) { // 等待最多2秒
        if (Serial1.available()) {
            String response = Serial1.readStringUntil('\n'); // 读取一行响应
            Serial.println("[LoRa  Serial]:" + response);
            // 检查响应内容
            if (response.indexOf(">") != -1) 
            {
                // 保存接收配置的数据
                _currentFreq         = pConfig->freq;
                _currentDataRate     = pConfig->dataRate;
                _currentBandwidth    = pConfig->bandwidth;
                _currentCodeRate     = pConfig->codeRate;
                _power               = pConfig->power;
                _currentIqConverted  = pConfig->iqConverted;

                _currentTransferMode = TransferMode::TX_MODE;
                // Serial.println("[Debug Serial]:set local address: " + String(localAddr));
                return true; // 设置成功
            } 
            else if (response.indexOf("ERROR") != -1) 
            {
                // 如果收到错误响应，则重新发送AT指令
                Serial.println("[Debug Serial]:Error received, retrying...");
                Serial1.println(command); // 重新发送AT指令
                // 重传次数限制
                static int retryCount = 0;
                if (retryCount < 5) 
                    retryCount++;
                else 
                {
                    Serial.println("[Debug Serial]:Maximum retry count reached, giving up.");
                    return false; // 重传次数达到上限，设置失败
                }
            }
        }
    }

    Serial.println("[Debug Serial]:UART Time Out Error");
    return false; // 设置失败
}

/*设置接收参数*/
bool LoRaModule::setRxConfig(const LoRaTransConfigStruct *pConfig)
{
    if(pConfig==nullptr)
    {
        Serial.println("[Debug Serial]:LoRaRxConfigStruct Error");
        return false;
    }
    String command = "AT+CRXS=" + String(pConfig->freq) 
                        +  ","  + String(pConfig->dataRate) 
                        +  ","  + String(pConfig->bandwidth) 
                        +  ","  + String(pConfig->codeRate)
                        +  ","  + String(pConfig->iqConverted);
    Serial1.println(command); // 发送配置接收参数的AT指令

    // 等待响应
    unsigned long startTime = millis();
    while (millis() - startTime < 2000) { // 等待最多2秒
        if (Serial1.available()) {
            String response = Serial1.readStringUntil('\n'); // 读取一行响应
            Serial.println("[LoRa  Serial]:" + response);
            // 检查响应内容
            if (response.indexOf("start") != -1) 
            {
                // Serial.println("[Debug Serial]:set local address: " + String(localAddr));

                // 保存接收配置的数据
                _currentFreq         = pConfig->freq;
                _currentDataRate     = pConfig->dataRate;
                _currentBandwidth    = pConfig->bandwidth;
                _currentCodeRate     = pConfig->codeRate;
                _currentIqConverted  = pConfig->iqConverted;

                _currentTransferMode = TransferMode::RX_MODE;

                return true; // 设置成功
            } 
            else if (response.indexOf("ERROR") != -1) 
            {
                // 如果收到错误响应，则重新发送AT指令
                Serial.println("[Debug Serial]:Error received, retrying...");
                Serial1.println(command); // 重新发送AT指令
                // 重传次数限制
                static int retryCount = 0;
                if (retryCount < 5) 
                    retryCount++;
                else 
                {
                    Serial.println("[Debug Serial]:Maximum retry count reached, giving up.");
                    return false; // 重传次数达到上限，设置失败
                }
            }
        }
    }

    Serial.println("[Debug Serial]:UART Time Out Error");
    return false; // 设置失败
}

/*设置本地地址*/
bool LoRaModule::setLocalAddress(uint32_t localAddr) {
    String command = "AT+CADDRSET=" + String(localAddr);
    Serial1.println(command); // 发送配置本地地址的AT指令

    // 等待响应
    unsigned long startTime = millis();
    while (millis() -startTime < 2000) { // 等待最多2秒
        if (Serial1.available()) {
            String response = Serial1.readStringUntil('\n'); // 读取一行响应
            Serial.println("[LoRa  Serial]:" + response);
            // 检查响应内容
            if (response.startsWith("set local address:")) 
            {
                Serial.println("[Debug Serial]:set local address: " + String(localAddr));
                _localAddr=localAddr;
                return true; // 设置成功
            } 
            else if (response.startsWith("+CMD ERROR")) 
            {
                // 如果收到错误响应，则重新发送AT指令
                Serial.println("[Debug Serial]:Error received, retrying...");
                Serial1.println(command); // 重新发送AT指令
                // 重传次数限制
                static int retryCount = 0;
                if (retryCount < 5) 
                    retryCount++;
                else 
                {
                    Serial.println("[Debug Serial]:Maximum retry count reached, giving up.");
                    return false; // 重传次数达到上限，设置失败
                }
            }
        }
    }
    return false; // 设置失败
}

/*设置目标地址*/
bool LoRaModule::setTargetAddress(uint32_t targetAddr) 
{
    String command = "AT+CTXADDRSET=" + String(targetAddr);
    Serial1.println(command); // 发送配置本地地址的AT指令
    // 等待响应
    unsigned long startTime = millis();
    while (millis() -startTime < 2000) { // 等待最多2秒
        if (Serial1.available()) {
            String response = Serial1.readStringUntil('\n'); // 读取一行响应
            Serial.println("[LoRa  Serial]:" + response);
            // 检查响应内容
            if (response.startsWith("set target address:")) 
            {
                Serial.println("[Debug Serial]:set target address: " + String(targetAddr));
                _targetAddr=targetAddr;
                return true; // 设置成功
            } 
            else if (response.startsWith("+CMD ERROR")) 
            {
                // 如果收到错误响应，则重新发送AT指令
                Serial.println("[Debug Serial]:Error received, retrying...");
                Serial1.println(command); // 重新发送AT指令
                // 重传次数限制
                static int retryCount = 0;
                if (retryCount < 5) 
                    retryCount++;
                else 
                {
                    Serial.println("[Debug Serial]:Maximum retry count reached, giving up.");
                    return false; // 重传次数达到上限，设置失败
                }
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

void LoRaModule::quitTransparent(void)
{
    Serial1.println("+++");
    this->_currentTransferMode=TransferMode::NONE;
}

bool LoRaModule::sendData(const String &str)
{
    if(this->_currentTransferMode!=TransferMode::TX_MODE)
    {
        Serial.println("[Debug Serial]: Current transfer mode is not TX_MODE.");
        return false;
    }
    if (str.isEmpty()) {
        Serial.println("[Debug Serial]: Data to send is empty.");
        return false; // 数据为空，发送失败
    }
    Serial1.println(str);
    // 等待响应
    unsigned long startTime = millis();
    while (millis() -startTime < 2000) { // 等待最多2秒
        if (Serial1.available()) {
            String response = Serial1.readStringUntil('\n'); // 读取一行响应
            Serial.println("[LoRa  Serial]:" + response);
            // 检查响应内容
            if (response.startsWith("OnTxDone")) 
            {
                Serial.println("[Debug Serial]:send data successful");
                return true; // 设置成功
            } 
        }
    }
    return false; // 设置失败
}
