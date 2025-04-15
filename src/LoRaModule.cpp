#include "LoRaModule.h"

/*初始化LoRa模块*/
void LoRa::begin() 
{
    Serial1.begin(9600, SERIAL_8N1, 18, 17);
    sendData(LoRa::SendMode::UNCONFIRMED,1,"111");
}

void LoRa::sendData(SendMode mode, uint8_t trials, const String& payload) {
    // 验证重试次数
    if (trials < 1 || trials > 7) {
        trials = 1;  // 默认值
    }

    // 构建AT指令
    String command = "AT+DTRX=";
    command += String(mode);
    command += ",";
    command += String(trials);
    command += ",";
    command += String(payload.length());  // 十六进制字符串长度除以2得到字节数
    command += ",";
    command += payload;

    Serial.println(command);
    // 发送命令
    Serial1.println(command);
}

void LoRa::receiveData()
{
    static int parseState = 0;  // 0: 等待rx行, 1: 等待payload行
    static uint8_t currentPort = 0;

    if (Serial1.available()) 
    {
        String response = Serial1.readStringUntil('\n'); // 读取一行响应
        response.trim();  // 移除首尾空格
        // Serial.println("[LoRa Serial]:" + response);

        // 检查是否是rx行
        if (response.startsWith("rx:")) {
            parseState = 1;
            // 解析port值
            int portIndex = response.indexOf("port =");
            if (portIndex >= 0) {
                currentPort = response.substring(portIndex + 6).toInt();
            }
        }
        // 检查是否是payload行（以0x开头）
        else if (parseState == 1 && response.indexOf("0x") >= 0) {
            parseState = 0;
            // 解析payload
            handlePayload(currentPort, response);
        }
    }
}

void LoRa::handlePayload(uint8_t port, const String& payload) {
    if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) {
        switch(port) {
            case 10: {
                // 设置闪烁频率
                uint8_t freq = strtol(payload.substring(payload.indexOf("0x")).c_str(), NULL, 16);
                switch(freq) {
                    case 0x1E: 
                        // Serial.println("设置闪烁频率: 30Hz");
                        _ledState.frequency = 30;
                        break;
                    case 0x3C: 
                        // Serial.println("设置闪烁频率: 60Hz");
                        _ledState.frequency = 60;
                        break;
                    case 0x78: 
                        // Serial.println("设置闪烁频率: 120Hz");
                        _ledState.frequency = 120;
                        break;
                }
                _ledStateChanged = true;
                break;
            }
            case 11: {
                // 设置LED颜色
                uint8_t color = strtol(payload.substring(payload.indexOf("0x")).c_str(), NULL, 16);
                _ledState.color = color == 0 ? LedColor::RED : LedColor::YELLOW;
                // Serial.println(color == 0 ? "设置颜色: 红色" : "设置颜色: 黄色");
                _ledStateChanged = true;
                break;
            }
            case 12: {
                // 设置是否闪烁
                uint8_t mode = strtol(payload.substring(payload.indexOf("0x")).c_str(), NULL, 16);
                _ledState.frequency = mode == 0 ? 60 : 0; // 闪烁时默认60Hz
                // Serial.println(mode == 0 ? "设置模式: 闪烁" : "设置模式: 常亮");
                _ledStateChanged = true;
                break;
            }
            case 13: {
                // 设置亮度
                String payloadStr = payload;
                int firstHex = payloadStr.indexOf("0x");
                int secondHex = payloadStr.indexOf("0x", firstHex + 2);
                if (firstHex >= 0 && secondHex >= 0) {
                    uint8_t high = strtol(payloadStr.substring(firstHex, firstHex + 4).c_str(), NULL, 16);
                    uint8_t low = strtol(payloadStr.substring(secondHex, secondHex + 4).c_str(), NULL, 16);
                    uint16_t brightness = (high << 8) | low;
                    _ledState.brightness = brightness;
                    // Serial.print("设置亮度: ");
                    // Serial.println(brightness);
                    _ledStateChanged = true;
                }
                break;
            }
            case 20:
                Serial.println("设置为车辆通过状态: 红色 + 亮度7000 + 120Hz");
                _ledState.color = LedColor::RED;
                _ledState.brightness = 7000;
                _ledState.frequency = 120;
                _ledStateChanged = true;
                break;
            case 21:
                Serial.println("设置为车辆离开状态: 黄色 + 亮度1000 + 常亮");
                _ledState.color = LedColor::YELLOW;
                _ledState.brightness = 1000;
                _ledState.frequency = 0;
                _ledStateChanged = true;
                break;
        }
        xSemaphoreGive(_ledStateMutex);
    }
}
