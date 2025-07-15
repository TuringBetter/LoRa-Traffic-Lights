#include "LED_WS2812Module.h"
#include "SyncTime.h"
#include "RadarModule.h" // 引入 RadarModule.h 以便访问其状态判断函数

const int NUM_LEDS = 576; // LED数量
const int DATA_PIN = 45;    // 选择你的GPIO引脚

TaskHandle_t        LED_WS2812_TaskHandle          =         NULL;
TaskHandle_t        LED_StatusChange_TaskHandle    =         NULL;
TaskHandle_t        LED_Test_TaskHandle            =         NULL;
SemaphoreHandle_t   ledControlMutex                =         NULL;

// ledControl 这是实际由 update_LED_WS2812任务读取并控制硬件显示的变量
static LED_Control_t    ledControl; 

// normalState 这个变量始终保存 LoRa 命令所期望的 normal 状态
// LoRaHandler 中的 Set函数会基于这个状态进行修改
static LED_Control_t    normalState;

// pendingNormalState当雷达灯激活时，LoRa 命令会暂时缓存到这里
// pendingNormalStateUpdated标记 pendingNormalState是否待处理
static LED_Control_t    pendingNormalState;
static bool             pendingNormalStateUpdated = false;

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
        Serial.println("[LED] 错误：无法创建LED控制互斥锁！系统可能不稳定。");
        while(1) { vTaskDelay(pdMS_TO_TICKS(100)); } // 阻止系统继续运行
    }

    // 初始时， normal和实际显示的灯光都设置为黄灯常亮
    normalState = {
        .isBlinking     = false,
        .blinkRate      = BLINK_RATE_30,
        .brightness     = 10,
        .color          = COLOR_YELLOW
    };
    ledControl = normalState;

    setColor(normalState.color); 
    setBright(normalState.brightness); 
}

// 返回 LoRa 命令所认为的 normal状态。
void LED_WS2812_GetState(LED_Control_t& curState)
{
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        if (pendingNormalStateUpdated) {
            curState = pendingNormalState; // 如果有待处理的LoRa命令，返回它
        } else {
            curState = normalState; // 否则，返回当前已生效的 normal状态。
        }
        xSemaphoreGive(ledControlMutex);
    }
}

// 这是 LoRaHandler 修改 normal状态的主要入口。
// 根据雷达灯是否激活来决定是立即应用还是缓存。
void LED_WS2812_SetState(const LED_Control_t &newState)
{
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        // 检查雷达灯是否激活 (雷达激活或处于延时状态)
        if (Radar_IsActiveOrExtending()) {
            // 雷达灯激活，将新的 normal状态（由 LoRa 命令指定）存储为待处理。
            // 不影响当前的 ledControl (雷达灯继续显示)。
            pendingNormalState = newState;
            pendingNormalStateUpdated = true;   // 标记有待处理命令
        } else {
            // 雷达灯未激活，立即应用新的 normal状态。
            // 更新 normalState 和 ledControl。
            normalState = newState; // 更新 normal状态
            ledControl = normalState; // 直接应用到实际显示
            pendingNormalStateUpdated = false; // 清除待处理标志
        }
        xSemaphoreGive(ledControlMutex);
    }
}

// 供 RadarModule 强制设置雷达灯状态
// 直接修改 ledControl，确保雷达灯立即生效，不涉及 LoRa 命令的缓存
void LED_WS2812_ForceSetState(const LED_Control_t& newState) {
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) {
        ledControl = newState; // 直接设置 ledControl为雷达灯状态。
        xSemaphoreGive(ledControlMutex);
    }
}

// 供 RadarModule 在雷达灯结束时调用
// 根据是否有缓存的 LoRa 命令来决定是应用该命令还是恢复到雷达激活前的 normal状态
void LED_WS2812_ApplyPendingOrRestore(const LED_Control_t& restoreState) {
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) {
        Serial.println("[LED] LED_WS2812_ApplyPendingOrRestore called.");
        if (pendingNormalStateUpdated) {
            // 有待处理的LoRa指令
            normalState = pendingNormalState; // 将缓存的 LoRa 命令设为新的 normal状态
            ledControl = normalState;         // 将新的 normal状态应用到实际显示
            pendingNormalStateUpdated = false; // 清除待处理标志

        } else {
            // 没有待处理的LoRa指令，恢复到雷达激活前的 normal状态。
            normalState = restoreState; // 恢复 last_state 到 normalState
            ledControl = normalState;   // 将恢复的 normal状态应用到实际显示
        }
        xSemaphoreGive(ledControlMutex);
    }
}

// LED_WS2812_SetColor 等函数现在会调用 LED_WS2812_GetState 获取 normal状态
// 然后修改并传递给 LED_WS2812_SetState 这是确保 LoRa 命令正确缓存的关键
void LED_WS2812_SetColor(uint32_t color)
{
    LED_Control_t curState;
    LED_WS2812_GetState(curState); // 获取当前的 normal状态
    curState.color = color;
    LED_WS2812_SetState(curState); // 传递修改后的 normal状态
}

void LED_WS2812_SetBrightness(uint8_t brightness)
{
    LED_Control_t curState;
    LED_WS2812_GetState(curState);
    curState.brightness = brightness;
    LED_WS2812_SetState(curState);
}

void LED_WS2812_SetBlink(bool isBlinking)
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

void LED_WS2812_SetBlinkRate(uint8_t blinkRate)
{
    LED_Control_t curState;
    LED_WS2812_GetState(curState);
    curState.isBlinking = true;
    curState.blinkRate = blinkRate;
    LED_WS2812_SetState(curState);
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

    static LED_Control_t lastState; // 用于检测 ledControl 的变化，不再初始化为固定值
    // 确保lastState在第一次运行时与ledControl同步，避免第一次needSync判断错误
    static bool isFirstRun = true;
    if(isFirstRun) {
        if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) {
            lastState = ledControl;
            xSemaphoreGive(ledControlMutex);
        }
        isFirstRun = false;
    }


    LED_Control_t currentState;
    
    // 获取当前LED控制状态，使用互斥锁确保线程安全
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        currentState = ledControl; // 从 ledControl 获取当前实际要显示的LED状态
        xSemaphoreGive(ledControlMutex);
    } 
    else 
    {
        // 如果无法获取互斥锁，直接返回，避免操作不一致的数据
        return;
    }
    
    // 检查闪烁状态是否改变
    bool needSync = false;       

    if (currentState.isBlinking != lastState.isBlinking ||
        currentState.blinkRate != lastState.blinkRate ||
        currentState.color != lastState.color ||
        currentState.brightness != lastState.brightness)
    {
        needSync = true;
    }

    if (needSync)
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
    }
    
    if(currentState.isBlinking) 
    {
        uint32_t currentTime = getTime_ms();
        uint32_t blinkInterval = (60000 / currentState.blinkRate) / 2;
        
        // 判断是否正在等待同步点
        if (isWaitingForSync)
        {
            if ((int32_t)(currentTime - nextSyncTime) >= 0) // 使用带符号差值更严谨
            {
                ledState = true;
                setColor(currentState.color);
                setBright(currentState.brightness);
                
                lastBlinkTime = nextSyncTime; // 将 lastBlinkTime 设置为实际点亮的时间
                isWaitingForSync = false;    // 清除等待同步点
                nextSyncTime = 0;            // 同步完成，清除 nextSyncTime 标志
            }
        }
        else // 不在等待同步，执行常规闪烁逻辑
        {
            if (lastBlinkTime != 0 && getSafeTimeDiff_ms(currentTime, lastBlinkTime) >= blinkInterval)
            {
                ledState = !ledState; // 翻转LED状态
                
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
        if (lastState.isBlinking != currentState.isBlinking ||
            lastState.color      != currentState.color     || 
            lastState.brightness != currentState.brightness )
        {
            setColor(currentState.color);
            setBright(currentState.brightness);
            Serial.printf("[LED_Task] Changed to Steady state: (isBlink=%d, Bright=%d, Color=%06X).\n",
                          currentState.isBlinking, currentState.brightness, currentState.color); // 调试输出
        }
        
        isWaitingForSync = false;
        lastBlinkTime = 0;
        ledState = true;
    }
    
    lastState = currentState; // 更新 lastState 为当前的 currentState
    strip.show(); // 更新LED灯带
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
        
        LED_WS2812_SetState(newState);
        
        state = (state + 1) % 6;
        vTaskDelay(pdMS_TO_TICKS(5000)); // 每个状态持续5秒
    }
}


static void setColor(uint32_t color) {
    for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, color);
    }
    strip.show(); // 移到 update_LED_WS2812 中集中控制
}

static void setBright(uint32_t brightness) {
    strip.setBrightness(brightness);
    strip.show(); // 移到 update_LED_WS2812 中集中控制
}

static void clearStrip() {
    for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show(); // 移到 update_LED_WS2812 中集中控制
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