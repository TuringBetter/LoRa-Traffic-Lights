#include "LED_WS2812Module.h"
#include "SyncTime.h"
#include "RadarModule.h"

const int NUM_LEDS = 576; // LED数量
const int DATA_PIN = 45;    // 选择你的GPIO引脚

TaskHandle_t        LED_WS2812_TaskHandle          =         NULL;
TaskHandle_t        LED_StatusChange_TaskHandle    =         NULL;
TaskHandle_t        LED_Test_TaskHandle            =         NULL;
SemaphoreHandle_t   ledControlMutex                =         NULL;

// 初始 ledControl状态：不闪烁，闪烁频率30次/min，亮度10，颜色黄色，
static LED_Control_t    ledControl; // LED 灯实际应用的参数，专供 update_LED_WS2812使用
static LED_Control_t    normalState{false,30,10,COLOR_YELLOW};    // 始终保存 LED 灯的 normal状态，LoRa命令接收到这里
static LED_Control_t    pendingNormalState; // 当雷达灯激活时，LoRa 命令会暂时缓存到这里
static bool             pendingNormalStateUpdated = false;  // 标记是否有待处理的缓存命令
static Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

static void setColor(uint32_t color);
static void setBright(uint32_t brightness);
static void setBlinkRate(uint32_t blinkRate);
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
    ledControl = normalState;
    setColor(normalState.color);
    // LED_WS2812_SetBlinkRate(30);
    setBright(normalState.brightness);
}

void LED_WS2812_GetState(LED_Control_t& curState)
{
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        // 如果有待处理的 LoRa 命令，返回该命令
        if (pendingNormalStateUpdated) {
            curState = pendingNormalState;
        }
        else {  // 否则，返回当前状态
            curState = normalState;
        }
        xSemaphoreGive(ledControlMutex);
    }
}

void LED_WS2812_SetState(const LED_Control_t &newState)
{
    Serial.println("LED_WS2812_SetState() called");
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        // 检查雷达灯是否激活
        Serial.println("Radar_IsActiveOrExtending() called_2");
        if (Radar_IsActiveOrExtending()) {
            // 雷达灯激活时，将新的平时灯状态存储为待处理
            Serial.println("Radar_IsActiveOrExtending() called_3");
            pendingNormalState = newState;
            pendingNormalStateUpdated = true;   //待处理标志
            Serial.println("LoRa command received during Radar light, storing as pending.");
        } else {
            // 雷达灯未激活，立即应用新的平时灯状态
            Serial.println("Radar_IsActiveOrExtending() called_4");
            ledControl = newState;
            pendingNormalStateUpdated = false;
            Serial.println("LoRa command received, applying immediately.");
        }
        xSemaphoreGive(ledControlMutex);
    }
}

// 供 RadarModule 强制设置雷达灯状态 (绕过缓存逻辑)
void LED_WS2812_ForceSetState(const LED_Control_t& newState) {
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) {
        ledControl = newState; // 直接设置 ledControl
        xSemaphoreGive(ledControlMutex);
    }
}

// 供 RadarModule 在雷达灯结束时应用缓存或恢复之前状态
void LED_WS2812_ApplyPendingOrRestore(const LED_Control_t& restoreState) {
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) {
        if (pendingNormalStateUpdated) {
            // 有待处理的LoRa指令时
            normalState = pendingNormalState;   // 更新 normal状态
            ledControl = normalState;           // 应用 normal状态
            pendingNormalStateUpdated = false; // 清除待处理标志
        } else {
            // 没有待处理的LoRa指令，恢复到雷达激活前的状态
            ledControl = restoreState;
        }
        xSemaphoreGive(ledControlMutex);
    }
}

void LED_WS2812_SetColor(uint32_t color)
{
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        LED_Control_t curState;
        LED_WS2812_GetState(curState); // 获取当前的 normal 状态
        curState.color = color;
        LED_WS2812_SetState(curState); // 传递修改后的 normal 状态
    }
}

void LED_WS2812_SetBrightness(uint8_t brightness)
{
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        LED_Control_t curState;
        LED_WS2812_GetState(curState); // 获取当前的 normal 状态
        curState.brightness = brightness;
        LED_WS2812_SetState(curState); // 传递修改后的 normal 状态
    }
}

void LED_WS2812_SetBlink(bool isBlinking)
{
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        LED_Control_t curState;
        LED_WS2812_GetState(curState);
        curState.isBlinking = isBlinking;
        if (isBlinking) {
            if (curState.blinkRate == 0) {
                 curState.blinkRate = BLINK_RATE_60;
            }
        }
        LED_WS2812_SetState(curState);
    }
}

void LED_WS2812_SetBlinkRate(uint8_t blinkRate)
{
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        LED_Control_t curState;
        LED_WS2812_GetState(curState);
        curState.isBlinking = true;
        curState.blinkRate = blinkRate;
        LED_WS2812_SetState(curState);
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
    static uint32_t lastBlinkTime = 0;      // 上一次闪烁状态切换的时间点
    static uint32_t nextSyncTime = 0;       // 下一个同步时间点
    static bool ledState = true;            // 记录上一次的LED控制状态，确保初始值与ledControl的初始值匹配
    static bool isWaitingForSync = false;   // 标识初始同步是否完成

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
    bool needSync = false;       

    // 首次进入闪烁
    if (!lastState.isBlinking && currentState.isBlinking) 
    {
        needSync = true; 
        printTime("needSync");
    } 
    // 已经在闪烁，但闪烁参数发生变化
    else if (lastState.isBlinking && currentState.isBlinking) 
    {
        // 闪烁频率发生变化
        if (lastState.blinkRate!= currentState.blinkRate)         
            needSync = true;
    }

    if (needSync)  // 闪烁参数变化或首次进入闪烁模式
    {
        uint32_t current_seconds = getTime_s();

        // 计算下一个偶数秒的毫秒时间作为闪烁的起始同步点。
        if (current_seconds % 2 == 0) 
        {
            nextSyncTime = (current_seconds + 2) * 1000;
        } 
        else 
        {
            nextSyncTime = (current_seconds + 1) * 1000;
        }
        
        // 设定初始状态，等待同步，LED熄灭
        isWaitingForSync = true; 
        ledState = false; 
        clearStrip();               // 确保LED在等待期间是灭的
        lastBlinkTime = 0;          // 重置 lastBlinkTime，表示尚未进入正常闪烁周期，以此判断是否在等待同步

        /**** 添加串口输出：同步事件 ****
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
        uint32_t currentTime = getTime_ms(); 
        uint32_t blinkInterval = (60000 / currentState.blinkRate) / 2;
        
        // 判断是否正在等待同步点
        if (isWaitingForSync) // 如果nextSyncTime非0，说明有一个同步点需要等待
        {
            // printTime("isWaitingForSync");
            // 检查是否已达到或超过同步点
            // 也可以用currentTime >= nextSyncTime更好理解，但带符号差值更严谨
            if ((int32_t)(currentTime - nextSyncTime) >= 0) 
            {
                // 到达同步点，LED首次点亮

                printTime("LED_Sync_Time");    // 打印当前时间

                ledState = true; 
                setColor(currentState.color);
                setBright(currentState.brightness);
                
                lastBlinkTime = nextSyncTime; // 将 lastBlinkTime 设置为实际点亮的时间
                isWaitingForSync = false;    // 清除等待同步点
                nextSyncTime = 0;            // 同步完成，清除 nextSyncTime 标志
                
                // Serial.printf("[Sync Achieved] LED ON at %u ms (Sync point: %u ms)\n", currentTime, lastBlinkTime);
            }
        }
        else // 不在等待同步(nextSyncTime为0)，执行常规闪烁逻辑
        {
            // 只有当 lastBlinkTime被有效设置过（即不是初始的0），才计算时间差
            // 首次点亮后lastBlinkTime就会被设置为 currentTime
            if (lastBlinkTime != 0 && getSafeTimeDiff_ms(currentTime, lastBlinkTime) >= blinkInterval)
            {
                ledState = !ledState; // 翻转LED状态
                // lastBlinkTime = currentTime; // 更新 lastBlinkTime 为实际切换的时间
                
                while (getSafeTimeDiff_ms(currentTime, lastBlinkTime) >= blinkInterval) {
                    lastBlinkTime += blinkInterval;
                }

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
        
        // 当切换到常亮模式时，清除闪烁相关的状态
        // 这样下次再进入闪烁模式时，会重新触发同步逻辑
        isWaitingForSync = false;   // 清除等待同步点（这一行是冗余，也可以删）
        lastBlinkTime = 0;          // 清除上次闪烁时间，确保下次闪烁重新同步
        ledState = true;            // 常亮模式下LED是亮的
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