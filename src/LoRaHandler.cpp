#include "LoRaHandler.h"
#include "LED_WS2812Module.h"
#include "FlashingLightModule.h"

typedef void (*portHandler)(const String& payload);

static void portHandler10(const String& payload);
static void portHandler11(const String& payload);
static void portHandler12(const String& payload);
static void portHandler13(const String& payload);
static void portHandler14(const String& payload);
static void portHandler15(const String& payload);

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
    NULL,                // 9
    portHandler10,       // 10
    portHandler11,       // 11
    portHandler12,       // 12
    portHandler13,       // 13
    portHandler14,       // 14
    portHandler15,       // 15
};


void handlePayload(uint8_t port, const String& payload)
{
    if(port < 0 || port >= sizeof(portHandlers) / sizeof(portHandlers[0])) return;

    portHandler cur_port_handler = portHandlers[port];
    if(cur_port_handler!=NULL)
        cur_port_handler(payload);
}


void portHandler10(const String &payload)
{
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

void portHandler11(const String &payload)
{
    uint8_t color = strtol(payload.substring(payload.indexOf("0x")).c_str(), NULL, 16);
    if(color==0x00) LED_WS2812_SetColor(COLOR_RED);
    else LED_WS2812_SetColor(COLOR_YELLOW);
}

void portHandler12(const String &payload)
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

void portHandler13(const String &payload)
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

void portHandler14(const String &payload)
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

void portHandler15(const String &payload)
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

    LED_WS2812_SetState(new_led_control);
}
