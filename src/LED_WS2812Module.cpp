#include "LED_WS2812Module.h"

const int NUM_LEDS = 576; // LED数量
const int DATA_PIN = 5;    // 选择你的GPIO引脚

TaskHandle_t        LED_WS2812_TaskHandle          =         NULL;
TaskHandle_t        LED_StatusChange_TaskHandle    =         NULL;
LED_Control_t       ledControl;
SemaphoreHandle_t   ledControlMutex                =         NULL;

static Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

static void setColor(uint32_t color);
static void setBright(uint32_t brightness);
static void clearStrip();
static void update_LED_WS2812(void);

void LED_WS2812_init()
{
    strip.begin();
    // 创建互斥锁
    ledControlMutex = xSemaphoreCreateMutex();
}

bool LED_WS2812_SetState(LED_Control_t newState)
{
    if (ledControlMutex == NULL) return false;
    
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        ledControl = newState;
        xSemaphoreGive(ledControlMutex);
        return true;
    }
    return false;
}

void LED_WS2812_Task(void *pvParameters)
{
    TickType_t lastBlinkTime = 0;
    // 闪烁状态:true为亮，false为灭
    bool ledState = true;
    
    while(1) 
    {
        update_LED_WS2812();
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms延时
    }
}

static void update_LED_WS2812(void)
{
    static TickType_t lastBlinkTime = 0;
    // 闪烁状态:true为亮，false为灭
    static bool ledState = true;
    LED_Control_t currentState;
    
    // 获取当前状态
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        currentState = ledControl;
        xSemaphoreGive(ledControlMutex);
    } 
    else 
    {
        return;
    }
    
    if(currentState.isBlinking) 
    {
        // 计算闪烁间隔（毫秒）
        uint32_t blinkInterval = (60000 / currentState.blinkRate) / 2;
        
        if((xTaskGetTickCount() - lastBlinkTime) >= pdMS_TO_TICKS(blinkInterval)) 
        {
            ledState = !ledState;
            lastBlinkTime = xTaskGetTickCount();
            
            if(ledState) 
            {
                setColor(currentState.color);
                setBright(currentState.brightness);
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
        setColor(currentState.color);
        setBright(currentState.brightness);
    }
}

void LED_StatusChange_Task(void *pvParameters)
{
    uint8_t state = 0;
    LED_Control_t newState;
    
    while(1) {
        switch(state) {
            case 0: // 红色最大亮度常亮
                newState.isBlinking = false;
                newState.color = COLOR_RED;
                newState.brightness = 255;
                break;
                
            case 1: // 红色最小亮度常亮
                newState.isBlinking = false;
                newState.color = COLOR_RED;
                newState.brightness = 50;
                break;
                
            case 2: // 黄色最大亮度常亮
                newState.isBlinking = false;
                newState.color = COLOR_YELLOW;
                newState.brightness = 255;
                break;
                
            case 3: // 黄色最小亮度常亮
                newState.isBlinking = false;
                newState.color = COLOR_YELLOW;
                newState.brightness = 50;
                break;
                
            case 4: // 红色最大亮度30次/min闪烁
                newState.isBlinking = true;
                newState.color = COLOR_RED;
                newState.brightness = 255;
                newState.blinkRate = BLINK_RATE_30;
                break;
                
            case 5: // 红色最大亮度60次/min闪烁
                newState.isBlinking = true;
                newState.color = COLOR_RED;
                newState.brightness = 255;
                newState.blinkRate = BLINK_RATE_60;
                break;
        }
        
        // 使用新的接口函数更新状态
        LED_WS2812_SetState(newState);
        
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