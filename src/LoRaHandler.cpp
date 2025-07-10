#include "LoRaHandler.h"
#include "LED_WS2812Module.h"
#include "LoRaLantency.h"
#include "SyncTime.h"
#include "LoRaModule.h"
#include "NVSManager.h"

typedef void (*portHandler)(const String& payload);

static void measureLantency(const String& payload);
static void setFreq(const String& payload);
static void setColor(const String& payload);
static void setManner(const String& payload);
static void setBrightness(const String& payload);
static void setSwitch(const String& payload);
static void setAll(const String& payload);
static void joinGroup(const String& payload);

static uint32_t REAL_TIME_MS = 0;

static const portHandler portHandlers[] = 
{
    NULL,                // 0
    NULL,                // 1
    NULL,                // 2
    NULL,                // 3
    NULL,                // 4
    NULL,                // 5
    NULL,                // 6
    NULL,                // 7
    NULL,                // 8
    measureLantency,     // 9
    setFreq,             // 10
    setColor,            // 11
    setManner,           // 12
    setBrightness,       // 13
    setSwitch,           // 14
    setAll,              // 15
    joinGroup,           // 16
};


void handlePayload(uint8_t port, const String& payload)
{
    if(port < 0 || port >= sizeof(portHandlers) / sizeof(portHandlers[0])) return;

    portHandler cur_port_handler = portHandlers[port];
    if(cur_port_handler!=NULL)
    {
        /*
        Serial.print("port: ");
        Serial.println(port);
        Serial.print("payload: ");
        Serial.println(payload);
        */
        cur_port_handler(payload);
    }
}

void measureLantency(const String &payload)
{
    // 1. 分割payload
    int idx1 = payload.indexOf(' ');
    int idx2 = payload.indexOf(' ', idx1 + 1);
    int idx3 = payload.indexOf(' ', idx2 + 1);

    String s0 = payload.substring(0, idx1);
    String s1 = payload.substring(idx1 + 1, idx2);
    String s2 = payload.substring(idx2 + 1, idx3);
    String s3 = payload.substring(idx3 + 1);

    // 2. 转为uint8_t
    uint8_t b0 = strtol(s0.c_str(), NULL, 16);
    uint8_t b1 = strtol(s1.c_str(), NULL, 16);
    uint8_t b2 = strtol(s2.c_str(), NULL, 16);
    uint8_t b3 = strtol(s3.c_str(), NULL, 16);

    // 3. 按大端序拼成uint32_t
    REAL_TIME_MS = ((uint32_t)b0 << 24) | ((uint32_t)b1 << 16) | ((uint32_t)b2 << 8) | b3;
    CalcLantency();
    
    // 4. 触发时间同步
    triggerTimeSynchronization();
}

uint32_t getRealTimeMs() {
    return REAL_TIME_MS;
}

void setFreq(const String &payload)
{
    // delay(getDelay());
    uint8_t payload_freq = strtol(payload.substring(payload.indexOf("0x")).c_str(), NULL, 16);
    switch(payload_freq) 
    {
        case 0x1E: // 30Hz
            LED_WS2812_SetBlinkRate(BLINK_RATE_30);
            break;
        case 0x3C: // 60Hz
            LED_WS2812_SetBlinkRate(BLINK_RATE_60);
            break;
        case 0x78: // 120Hz
            LED_WS2812_SetBlinkRate(BLINK_RATE_120);
            break;
    }
}

void setColor(const String &payload)
{
    uint8_t color = strtol(payload.substring(payload.indexOf("0x")).c_str(), NULL, 16);
    if(color==0x00) LED_WS2812_SetColor(COLOR_RED);
    else LED_WS2812_SetColor(COLOR_YELLOW);
}

void setManner(const String &payload)
{
    uint8_t manner = strtol(payload.substring(payload.indexOf("0x")).c_str(), NULL, 16);
    // 常量
    if(manner == 0x01)
    {
        LED_WS2812_SetBlink(false);
    }     
    // 开启闪烁，默认60次/min
    else
    {
        LED_WS2812_SetBlinkRate(BLINK_RATE_60);
    }
}

void setBrightness(const String &payload)
{
    String payloadStr = payload;
    int firstHex = payloadStr.indexOf("0x");
    int secondHex = payloadStr.indexOf("0x", firstHex + 2);
    if (firstHex >= 0 && secondHex >= 0) 
    {
        uint8_t high = strtol(payloadStr.substring(firstHex, firstHex + 4).c_str(), NULL, 16);
        uint8_t low = strtol(payloadStr.substring(secondHex, secondHex + 4).c_str(), NULL, 16);
        uint16_t payload_brightness = (high << 8) | low;
        
        // 将0~7000的亮度值映射到0~255
        uint8_t led_brightness = (uint8_t)((payload_brightness * 255) / 7000);
        
        LED_WS2812_SetBrightness(led_brightness);
    }
}

void setSwitch(const String &payload)
{
    /** */
    const static LED_Control_t LED_OFF = {false, 60, 0, COLOR_OFF};
    const static LED_Control_t LED_ON  = {false, 30, 10, COLOR_YELLOW};
    uint8_t status = strtol(payload.substring(payload.indexOf("0x")).c_str(), NULL, 16);
    // 开启
    if(status == 0x01)
    {
        LED_WS2812_SetState(LED_ON);
    }     
    // 关闭
    else
    {
        LED_WS2812_SetState(LED_OFF);
    }
    /* */
}

void setAll(const String &payload)
{
    static LED_Control_t new_led_control;
    String payloadStr = payload;
    
    // 解析颜色（第1字节）
    int firstHex = payloadStr.indexOf("0x");
    uint8_t color = strtol(payloadStr.substring(firstHex, firstHex + 4).c_str(), NULL, 16);
    new_led_control.color = (color == 0x00) ? COLOR_RED : COLOR_YELLOW;

    // 解析频率（第2字节）
    int secondHex = payloadStr.indexOf("0x", firstHex + 4);
    uint8_t freq = strtol(payloadStr.substring(secondHex, secondHex + 4).c_str(), NULL, 16);
    switch(freq) 
    {
        case 0x1E: new_led_control.blinkRate = BLINK_RATE_30; break;
        case 0x3C: new_led_control.blinkRate = BLINK_RATE_60; break;
        case 0x78: new_led_control.blinkRate = BLINK_RATE_120; break;
    }

    // 解析亮度（第3-4字节）
    int thirdHex = payloadStr.indexOf("0x", secondHex + 4);
    int fourthHex = payloadStr.indexOf("0x", thirdHex + 4);
    uint8_t high = strtol(payloadStr.substring(thirdHex, thirdHex + 4).c_str(), NULL, 16);
    uint8_t low = strtol(payloadStr.substring(fourthHex, fourthHex + 4).c_str(), NULL, 16);
    uint16_t brightness = (high << 8) | low;
    // 将0~7000的亮度值映射到0~255
    new_led_control.brightness = (uint8_t)((brightness * 255) / 7000);

    // 解析亮灯方式（第5字节）
    int fifthHex = payloadStr.indexOf("0x", fourthHex + 4);
    uint8_t manner = strtol(payloadStr.substring(fifthHex, fifthHex + 4).c_str(), NULL, 16);
    new_led_control.isBlinking = (manner == 0x00);  // 0x00闪烁，0x01常亮

    // if(manner == 0x00) delay(getDelay());

    LED_WS2812_SetState(new_led_control);
}


static void joinGroup(const String& payloadStr)
{
    // 预期二进制数据总长度：4 (DevAddr) + 16 (AppSKey) + 16 (NwkSKey) = 36 字节
    const size_t EXPECTED_DATA_LEN_BYTES = 36;
    uint8_t dataBytes[EXPECTED_DATA_LEN_BYTES]; // 用于存储解析后的二进制字节
    size_t currentByteIndex = 0; // 用于跟踪当前填充的字节数

    int currentHexStart = payloadStr.indexOf("0x"); // 查找第一个 "0x" 的位置

    // 循环提取所有字节
    while (currentHexStart >= 0 && currentByteIndex < EXPECTED_DATA_LEN_BYTES) {
        // 确保能提取到两个十六进制字符 (例如 "0xAB" 需要4个字符长度)
        if (currentHexStart + 4 <= payloadStr.length()) {
            // 提取 "0xAB" 这样的子串，并转换为字节
            uint8_t byteValue = strtol(payloadStr.substring(currentHexStart, currentHexStart + 4).c_str(), NULL, 16);
            dataBytes[currentByteIndex++] = byteValue;
        } else {
            Serial.println("[LoRaHandler] Multicast Join: Incomplete hex byte found in payload string.");
            break; // 数据不完整，退出循环
        }
        // 查找下一个 "0x"，从当前 0x 的结束位置 + 1 开始查找 (为了跳过空格或下一个0x)
        currentHexStart = payloadStr.indexOf("0x", currentHexStart + 4); 
    }

    // 校验解析后的字节数是否符合预期
    if (currentByteIndex != EXPECTED_DATA_LEN_BYTES) {
        Serial.print("[LoRaHandler] Multicast Join: Parsed ");
        Serial.print(currentByteIndex);
        Serial.print(" bytes, expected ");
        Serial.print(EXPECTED_DATA_LEN_BYTES);
        Serial.println(" bytes. Payload parsing error due to incorrect length or format.");
        return;
    }

    Serial.println("[LoRaHandler] Received valid Multicast Join Group Data.");

    // 提取 DevAddr (4字节)
    char devAddrHex[9]; // 4 bytes * 2 hex chars + null terminator
    sprintf(devAddrHex, "%02X%02X%02X%02X", dataBytes[0], dataBytes[1], dataBytes[2], dataBytes[3]);
    String devAddrStr = String(devAddrHex);

    // 提取 AppSKey (16字节)
    char appSKeyHex[33]; // 16 bytes * 2 hex chars + null terminator
    for(int i = 0; i < 16; ++i) {
        sprintf(&appSKeyHex[i*2], "%02X", dataBytes[4 + i]); // Offset: 4 (DevAddr)
    }
    String appSKeyStr = String(appSKeyHex);

    // 提取 NwkSKey (16字节)
    char nwkSKeyHex[33]; // 16 bytes * 2 hex chars + null terminator
    for(int i = 0; i < 16; ++i) {
        sprintf(&nwkSKeyHex[i*2], "%02X", dataBytes[20 + i]); // Offset: 4 (DevAddr) + 16 (AppSKey) = 20
    }
    String nwkSKeyStr = String(nwkSKeyHex);

    Serial.print("[LoRaHandler] DevAddr: "); //
    Serial.println(devAddrStr); //
    Serial.print("[LoRaHandler] AppSKey: "); //
    Serial.println(appSKeyStr); //
    Serial.print("[LoRaHandler] NwkSKey: "); //
    Serial.println(nwkSKeyStr); //

    // 调用NVS管理器保存组播信息
    if (NVS_saveLoRaMulticast(devAddrStr, appSKeyStr, nwkSKeyStr)) { // 调用 NVSManager 中的保存函数
        Serial.println("[LoRaHandler] Multicast config successfully saved to NVS.");
    } else {
        Serial.println("[LoRaHandler] Failed to save multicast config to NVS.");
    }

    // 调用 LoRaModule.cpp 中定义的函数来添加多播组配置
    addMuticast_IDF(devAddrStr, appSKeyStr, nwkSKeyStr);
    Serial.println("[LoRaHandler] Successfully joined multicast group.");
}