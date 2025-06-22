#include "LED_WS2812Module.h"
#include "SyncTime.h"

const int NUM_LEDS = 576; // LED数量
const int DATA_PIN = 45;    // 选择你的GPIO引脚

TaskHandle_t        LED_WS2812_TaskHandle          =         NULL;
TaskHandle_t        LED_StatusChange_TaskHandle    =         NULL;
SemaphoreHandle_t   ledControlMutex                =         NULL;

static LED_Control_t       ledControl{false,30,10,COLOR_YELLOW}; // 初始ledControl状态：不闪烁，闪烁频率30次/min，亮度10，颜色黄色
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
    
    // 检查互斥锁是否创建成功
    if (ledControlMutex == NULL) {
        Serial.println("错误：无法创建LED控制互斥锁");
        return;
    }

    setColor(COLOR_YELLOW);
    setBright(10);
}

void LED_WS2812_GetState(LED_Control_t& curState)
{
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        curState = ledControl;
        xSemaphoreGive(ledControlMutex);
    }
}

void LED_WS2812_SetState(const LED_Control_t &newState)
{
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        ledControl = newState;
        xSemaphoreGive(ledControlMutex);
    }
}

void LED_WS2812_SetColor(uint32_t color)
{
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        ledControl.color = color;
        xSemaphoreGive(ledControlMutex);
    }
}

void LED_WS2812_SetBrightness(uint8_t brightness)
{
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        ledControl.brightness = brightness;
        xSemaphoreGive(ledControlMutex);
    }
}

void LED_WS2812_SetBlink(bool isBlinking)
{
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        ledControl.isBlinking = isBlinking;
        ledControl.blinkRate = BLINK_RATE_60; // 默认闪烁频率
        xSemaphoreGive(ledControlMutex);
    }
}

void LED_WS2812_SetBlinkRate(uint8_t blinkRate)
{
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        ledControl.isBlinking = true;
        ledControl.blinkRate = blinkRate;
        xSemaphoreGive(ledControlMutex);
    }
}

void LED_WS2812_Task(void *pvParameters)
{   
    while(1) 
    {
        update_LED_WS2812();
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms延时，控制更新频率
    }
}

static void update_LED_WS2812(void)
{
    static TickType_t lastBlinkTime = 0; // 上一次闪烁状态切换的时间点
    // 闪烁状态:true为亮，false为灭。初始为true，表示闪烁周期开始时默认亮
    static bool ledState = true; 
    // 记录上一次的LED控制状态。
    // 确保初始值与ledControl的初始值匹配
    static LED_Control_t lastState{false,30,10,COLOR_YELLOW}; 

    LED_Control_t currentState;
    
    // 获取当前LED控制状态，使用互斥锁确保线程安全
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        currentState = ledControl;
        xSemaphoreGive(ledControlMutex);
    } 
    else 
    {
        // 如果无法获取互斥锁，直接返回，避免操作不一致的数据
        return;
    }
    
    // 检查闪烁状态是否改变
    // 这个条件确保同步逻辑只在状态改变时执行一次，不影响后续循环闪烁的性能。
    bool needsTimeSynchronization = false;       

    if (!lastState.isBlinking && currentState.isBlinking) {     //首次进入闪烁
        needsTimeSynchronization = true; 
    } else if (lastState.isBlinking && currentState.isBlinking) {       // 已经在闪烁，但闪烁参数发生变化
        if (lastState.blinkRate != currentState.blinkRate ||            // 闪烁频率发生变化
            lastState.color != currentState.color ||                    // 颜色发生变化
            lastState.brightness != currentState.brightness) {          // 亮度发生变化
            needsTimeSynchronization = true;
        }
    }

    if (needsTimeSynchronization)  // 闪烁参数变化
    {


        uint64_t current_time_ms = getTime_ms(); 
        //如果要切换为gettime()修改这里为uint64_t current_time_ms = gettime_ms();
        uint64_t current_seconds = getTime_s();

        /*
        // 2. 计算下一个偶数秒的毫秒时间作为闪烁的起始同步点。
        // 这样可以确保无论何时启动闪烁，第一次动作都会对齐到最近的未来偶数整秒。
        */
        uint64_t next_even_second_ms;
        if (current_seconds % 2 == 0) 
        {
            // 当前秒是偶数，下一个偶数秒是当前秒 + 2秒
            next_even_second_ms = (current_seconds + 2) * 1000;
        } 
        else 
        {
            // 当前秒是奇数，下一个偶数秒是当前秒 + 1秒
            next_even_second_ms = (current_seconds + 1) * 1000;
        }
        /*
        // 3. 设置 lastBlinkTime 为这个计算出的同步时间点对应的 TickType_t 值。
        // 这将使得LED在达到 next_even_second_ms 时才进行第一次闪烁状态切换。
        */
        lastBlinkTime = pdMS_TO_TICKS(next_even_second_ms);
        
        // 4. 初始化 ledState 为 false (灭)，并立即熄灭LED。
        // 这样在等待同步点到来之前，LED会保持熄灭状态，避免不一致。
        ledState = false; 
        clearStrip(); // 确保LED在等待期间是灭的

        /**** 添加串口输出：同步事件 ****/
        Serial.print("[Sync Event] Entered blinking mode at ");
        Serial.print(current_time_ms);
        Serial.print(" ms (");
        Serial.print(current_seconds);
        Serial.print("s). Next even second is ");
        Serial.print(next_even_second_ms / 1000);
        Serial.print("s. Sync time set to ");
        Serial.print(lastBlinkTime);
        Serial.println(" ms.");
        /**/
    }
    
    if(currentState.isBlinking) 
    {
        // 计算闪烁间隔（毫秒）：一个完整亮灭周期的一半
        uint32_t blinkInterval = (60000 / currentState.blinkRate) / 2;
        
        // 判断是否到达下一个闪烁状态切换点
        // 只有当经过的时间超过一个"亮"或"灭"的持续时间时，才切换状态。
        if((xTaskGetTickCount() - lastBlinkTime) >= pdMS_TO_TICKS(blinkInterval)) 
        {
            ledState = !ledState; // 翻转LED状态 (亮 -> 灭 或 灭 -> 亮)
            lastBlinkTime = xTaskGetTickCount(); // 更新上一次状态切换的时间为当前时间
            
            if(ledState) 
            {
                // 如果当前是亮状态，设置LED为指定颜色和亮度
                setColor(currentState.color);
                setBright(currentState.brightness);
            } 
            else 
            {
                // 如果当前是灭状态，关闭所有LED（设置为黑色）
                clearStrip();
            }
        }
    } 
    else // 常亮模式 (currentState.isBlinking == false)
    {
        // 只有当LED状态（常亮/闪烁状态、颜色、亮度）发生变化时才更新LED。
        // 这确保了从闪烁模式切换回常亮时，LED能立即更新。
        if (lastState.isBlinking != currentState.isBlinking || // 从闪烁切换到常亮，或常亮参数改变
            lastState.color      != currentState.color     || 
            lastState.brightness != currentState.brightness )
        {
            setColor(currentState.color);
            setBright(currentState.brightness);
        }
        
        // 当处于常亮模式时，ledState 和 lastBlinkTime 保持其值，
        // 在下次切换到闪烁模式时，它们会被同步逻辑重新计算和设置，无需在这里额外重置。
    }
    
    // 在函数末尾，更新 lastState 为当前的 currentState，为下一次循环做准备
    lastState = currentState;
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

/*测试函数，用于测试LED状态切换*/
void LED_Test_Task(void *pvParameters)
{
    uint8_t state = 0; // 0: 闪烁模式, 1: 不闪烁模式
    uint8_t subState = 0; // 0: 初始状态, 1: 15秒后的状态
    LED_Control_t newState;
    
    while(1) {
        if (state == 0) { // 闪烁模式
            if (subState == 0) { // 0-15秒：30次/min，亮度100，红色
                newState.isBlinking = true;
                newState.blinkRate = BLINK_RATE_30;
                newState.brightness = 100;
                newState.color = COLOR_RED;
            } else { // 15-30秒：120次/min，亮度10，黄色
                newState.isBlinking = true;
                newState.blinkRate = BLINK_RATE_120;
                newState.brightness = 10;
                newState.color = COLOR_YELLOW;
            }
        } else { // 不闪烁模式
            if (subState == 0) { // 0-15秒：亮度100，黄色
                newState.isBlinking = false;
                newState.brightness = 100;
                newState.color = COLOR_YELLOW;
            } else { // 15-30秒：亮度10，红色
                newState.isBlinking = false;
                newState.brightness = 10;
                newState.color = COLOR_RED;
            }
        }
        
        // 更新LED状态
        LED_WS2812_SetState(newState);
        
        // 输出当前状态信息
        Serial.print("[LED Test] State: ");
        Serial.print(state == 0 ? "Blinking" : "Steady");
        Serial.print(", SubState: ");
        Serial.print((uint64_t)xTaskGetTickCount() * portTICK_PERIOD_MS);
        Serial.print("  /  ");
        Serial.print(getTime_ms());
        Serial.print("ms");
        Serial.print(", Color: ");
        Serial.print(newState.color == COLOR_RED ? "Red" : "Yellow");
        Serial.print(", Brightness: ");
        Serial.print(newState.brightness);
        if (newState.isBlinking) {
            Serial.print(", BlinkRate: ");
            Serial.print(newState.blinkRate);
            Serial.print("/min");
        }
        Serial.println();
        
        // 等待15秒
        vTaskDelay(pdMS_TO_TICKS(14500));
        
        // 更新子状态
        subState = (subState + 1) % 2;
        
        // 如果子状态回到0，说明30秒周期结束，切换主状态
        if (subState == 0) {
            state = (state + 1) % 2;
        }
    }
}
/**/