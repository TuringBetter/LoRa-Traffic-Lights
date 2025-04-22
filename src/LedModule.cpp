#include "LedModule.h"
#include "LoRaModule.h"
static const uint8_t          RED_PIN    = 45;    // 使用GPIO7作为红色LED控制
static const uint8_t          YELLOW_PIN = 46; // 使用GPIO8作为黄色LED控制

static       LedColor        _currentColor;
static       uint32_t        _brightness;  // PWM亮度值
static       uint16_t        _frequency;   // 闪烁频率(Hz)
static       unsigned long   _lastToggleTime;
static       bool            _isBlinking{false};
static       bool            _currentState;

TaskHandle_t LedTestTaskHandle = NULL;
TaskHandle_t LedTaskHandle     = NULL;

bool                _ledStateChanged{false};
SemaphoreHandle_t   _ledStateMutex;
volatile LedState   ledstate{
                    .color=LedColor::YELLOW,
                    .brightness=5000,
                    .frequency=0};

static void updatePWM();
static void update();

void Led_init()
{
    // 配置GPIO引脚
    pinMode(RED_PIN, OUTPUT);
    pinMode(YELLOW_PIN, OUTPUT);
    
    // 设置LED PWM属性
    // 使用低速通道，频率1KHz，8位分辨率
    ledcSetup(0, 1000, 8);  // 通道0，1KHz PWM，8位分辨率
    ledcSetup(1, 1000, 8);  // 通道1，1KHz PWM，8位分辨率
    
    // 将LED引脚附加到PWM通道
    ledcAttachPin(RED_PIN, 0);
    ledcAttachPin(YELLOW_PIN, 1);
    
    _currentColor=LedColor::YELLOW;
    _brightness=500;
    _frequency=0;
    _isBlinking=false;
    _lastToggleTime=0;
    _currentState=false;

    _ledStateMutex = xSemaphoreCreateMutex();

    updatePWM();
}

void setColor(LedColor color)
{
    if (_currentColor != color) 
    {
        // 关闭当前LED
        ledcWrite(_currentColor == LedColor::RED ? 0 : 1, 0);
        
        _currentColor = color;
        updatePWM();
    }
}

void setBrightness(uint32_t brightness)
{
    // 验证亮度值是否合法
    if (brightness > 7000) return;
    _brightness = brightness;
    updatePWM();
}

void setFrequency(uint16_t freq)
{   
    _frequency = freq;
    _isBlinking = (freq > 0);
    _currentState=(freq==0);
    updatePWM();
}

void setState(LedColor color, uint32_t brightness, uint16_t freq)
{
    if (_currentColor != color) 
    {
        // 关闭当前LED
        ledcWrite(_currentColor == LedColor::RED ? 0 : 1, 0);        
        _currentColor = color;
    }

    if (brightness > 7000) return;
    _brightness = brightness;

    _frequency = freq;
    _isBlinking = (freq > 0);
    _currentState=(freq==0);

    update();
}

void setState(const volatile LedState &ledstate)
{
    if (_currentColor != ledstate.color) 
    {
        // 关闭当前LED
        ledcWrite(_currentColor == LedColor::RED ? 0 : 1, 0);        
        _currentColor = ledstate.color;
    }

    if (ledstate.brightness > 7000) return;
    _brightness = ledstate.brightness;

    _frequency = ledstate.frequency;
    _isBlinking = (ledstate.frequency > 0);
    _currentState=(ledstate.frequency==0);

    updatePWM();
}

void updatePWM()
{
    uint32_t pwmValue = _currentState ? _brightness : 0;
    uint8_t channel = (_currentColor == LedColor::RED) ? 0 : 1;
    
    // 将亮度值映射到8位PWM范围(0-255)
    pwmValue = map(pwmValue, 0, 7000, 0, 255);
    ledcWrite(channel, pwmValue);
}

void update()
{
    if (!_isBlinking) return;
    
    unsigned long currentTime = millis();
    if (currentTime - _lastToggleTime >= (1000 / _frequency)) 
    {
        _currentState = !_currentState;
        _lastToggleTime = currentTime;
        updatePWM();
    }
}

void ledTestTask(void *pvParameters)
{
    static uint8_t test_state = 0;
    static unsigned long lastStateChange = 0;
    unsigned long currentTime = millis();
    while(true)
    {
        currentTime = millis();
        // 每5秒切换一次测试状态
        if (currentTime - lastStateChange >= 5000) {
            test_state = (test_state + 1) % 6;  // 6个测试状态循环
            lastStateChange = currentTime;
            
            switch (test_state) {
            case 0:
                Serial.println("测试1: 红色LED,亮度500,不闪烁");
                setColor(LedColor::RED);
                setBrightness(500);
                setFrequency(0);
                break;
                
            case 1:
                Serial.println("测试2: 红色LED,亮度2000,30Hz闪烁");
                setColor(LedColor::RED);
                setBrightness(2000);
                setFrequency(30);
                break;
                
            case 2:
                Serial.println("测试3: 红色LED,亮度7000,60Hz闪烁");
                setColor(LedColor::RED);
                setBrightness(7000);
                setFrequency(60);
                break;
                
            case 3:
                Serial.println("测试4: 黄色LED,亮度1000,不闪烁");
                setColor(LedColor::YELLOW);
                setBrightness(1000);
                setFrequency(0);
                break;
                
            case 4:
                Serial.println("测试5: 黄色LED,亮度4000,120Hz闪烁");
                setColor(LedColor::YELLOW);
                setBrightness(4000);
                setFrequency(120);
                break;
                
            case 5:
                Serial.println("测试6: 黄色LED,亮度2000,30Hz闪烁");
                setColor(LedColor::YELLOW);
                setBrightness(2000);
                setFrequency(30);
                break;
            }
        }
        
        // 更新LED状态（实现闪烁效果）
        update();
    }
}

void ledTask(void *pvParameters)
{
    static LedState last_ledstate;  // 保存上一次的LED状态
    String payload;

    while (true)
    {
        if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE)
        {
            // 检查状态是否发生变化
            if (ledstate.brightness != last_ledstate.brightness) 
            {
                // 亮度变化
                if ((ledstate.brightness == 0 && last_ledstate.brightness > 0) || 
                    (ledstate.brightness > 0 && last_ledstate.brightness == 0)) 
                    {
                    // 开关状态变化
                    payload = "0x09 0x";
                    payload += (ledstate.brightness > 0) ? "01" : "00";
                    sendData(payload);
                } 
                else 
                {
                    // 亮度值变化
                    payload = "0x0A 0x";
                    uint8_t high = (ledstate.brightness >> 8) & 0xFF;
                    uint8_t low = ledstate.brightness & 0xFF;
                    payload += String(high, HEX);
                    payload += " 0x";
                    payload += String(low, HEX);
                    sendData(payload);
                }
            }

            if (ledstate.frequency != last_ledstate.frequency) 
            {
                if (ledstate.frequency > 0 && last_ledstate.frequency > 0) 
                {
                    // 闪烁频率变化
                    payload = "0x0B 0x";
                    switch(ledstate.frequency) {
                        case 30: payload += "1E"; break;
                        case 60: payload += "3C"; break;
                        case 120: payload += "78"; break;
                    }
                    sendData(payload);
                } 
                else if ((ledstate.frequency == 0 && last_ledstate.frequency > 0) || 
                          (ledstate.frequency > 0 && last_ledstate.frequency == 0)) 
                          {
                    // 常亮/闪烁切换
                    payload = "0x0D 0x";
                    payload += (ledstate.frequency == 0) ? "01" : "00";
                    sendData(payload);
                }
            }

            if (ledstate.color != last_ledstate.color) 
            {
                // 颜色变化
                payload = "0x0C 0x";
                payload += (ledstate.color == LedColor::RED) ? "00" : "01";
                sendData(payload);
            }

            // 更新上一次状态
            last_ledstate.color = ledstate.color;
            last_ledstate.brightness = ledstate.brightness;
            last_ledstate.frequency = ledstate.frequency;

            // 更新LED状态
            setState(ledstate);
            xSemaphoreGive(_ledStateMutex);
        }

        // 更新LED状态（实现闪烁效果）
        update();

        // 任务延时
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms
    }
}

/**/
