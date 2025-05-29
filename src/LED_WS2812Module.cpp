#include "LED_WS2812Module.h"

const int NUM_LEDS = 576; // LED数量
const int DATA_PIN = 5;    // 选择你的GPIO引脚

TaskHandle_t LED_WS2812_TaskHandle          =         NULL;
TaskHandle_t LED_StatusChange_TaskHandle    =         NULL;
LED_Control_t ledControl;

static Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

static void setColor(uint32_t color);
static void setBright(uint32_t brightness);
static void clearStrip();

void LED_WS2812_init()
{
    strip.begin();
    // 初始化LED控制结构体
    ledControl.isBlinking   = false;
    ledControl.blinkRate    = BLINK_RATE_30;
    ledControl.brightness   = 255;
    ledControl.color        = COLOR_RED;
}

void LED_WS2812_Task(void *pvParameters)
{
    TickType_t lastBlinkTime = 0;
    // 闪烁状态:true为亮，false为灭
    bool ledState = true;
    
    while(1) {
        if(ledControl.isBlinking) 
        {
            // 计算闪烁间隔（毫秒）
            uint32_t blinkInterval = (60000 / ledControl.blinkRate) / 2;
            // 如果当前时间减去上次闪烁时间大于等于闪烁间隔，则更新闪烁状态
            if((xTaskGetTickCount() - lastBlinkTime) >= pdMS_TO_TICKS(blinkInterval)) 
            {
                ledState = !ledState;
                lastBlinkTime = xTaskGetTickCount();
                // 如果闪烁状态为亮，则设置颜色和亮度，否则清除LED
                if(ledState) 
                {
                    setColor(ledControl.color);
                    setBright(ledControl.brightness);
                } 
                else 
                {
                    clearStrip();
                }
            }
        } 
        else 
        {
            // 常亮模式
            setColor(ledControl.color);
            setBright(ledControl.brightness);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms延时
    }
}

void LED_StatusChange_Task(void *pvParameters)
{
    uint8_t state = 0;
    
    while(1) {
        switch(state) {
            case 0: // 红色最大亮度常亮
                ledControl.isBlinking = false;
                ledControl.color = COLOR_RED;
                ledControl.brightness = 255;
                break;
                
            case 1: // 红色最小亮度常亮
                ledControl.isBlinking = false;
                ledControl.color = COLOR_RED;
                ledControl.brightness = 50;
                break;
                
            case 2: // 黄色最大亮度常亮
                ledControl.isBlinking = false;
                ledControl.color = COLOR_YELLOW;
                ledControl.brightness = 255;
                break;
                
            case 3: // 黄色最小亮度常亮
                ledControl.isBlinking = false;
                ledControl.color = COLOR_YELLOW;
                ledControl.brightness = 50;
                break;
                
            case 4: // 红色最大亮度30次/min闪烁
                ledControl.isBlinking = true;
                ledControl.color = COLOR_RED;
                ledControl.brightness = 255;
                ledControl.blinkRate = BLINK_RATE_30;
                break;
                
            case 5: // 红色最大亮度60次/min闪烁
                ledControl.isBlinking = true;
                ledControl.color = COLOR_RED;
                ledControl.brightness = 255;
                ledControl.blinkRate = BLINK_RATE_60;
                break;
        }
        
        state = (state + 1) % 6;
        vTaskDelay(pdMS_TO_TICKS(5000)); // 每个状态持续5秒
    }
}

void setColor(uint32_t color) {
    for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}

void setBright(uint32_t brightness) {
    strip.setBrightness(brightness);
    strip.show();
}

void clearStrip() {
    for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();
}