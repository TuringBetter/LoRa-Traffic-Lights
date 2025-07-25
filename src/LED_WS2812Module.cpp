#include "LED_WS2812Module.h"
#include "SyncTime.h"
#include "RadarModule.h" 

#define LED_POWER_GPIO_PIN  40     // GPIO40用于使能 LED

const int DATA_PIN  =   45;         // GPIO45用于控制 LED

TaskHandle_t        LED_WS2812_TaskHandle          =         NULL;
TaskHandle_t        LED_StatusChange_TaskHandle    =         NULL;
TaskHandle_t        LED_Test_TaskHandle            =         NULL;
SemaphoreHandle_t   ledControlMutex                =         NULL;

// 这是实际由 update_LED_WS2812() 任务读取并控制硬件显示的变量，也是当前实际显示的状态
// RadarModule 通过修改此变量实现高优先级覆盖
static LED_Control_t    actualYellowState; 
static LED_Control_t    actualRedState;

// normalState 这个变量始终保存 LoRa 命令所期望的 normal 状态
// 主要由 LED_WS2812_SetState() 函数修改，该接收来自LoRaHandler 中的 Set函数的指令
static LED_Control_t    normalState;

// pendingNormalState当雷达灯激活时，LoRa 命令会暂时缓存到这里
// pendingNormalStateUpdated标记 pendingNormalState是否待处理
static LED_Control_t    pendingNormalState;
static bool             pendingNormalStateUpdated = false;

// 用于请求闪烁重同步
static bool g_trigger_resync = false;


static Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

// 内部辅助函数声明
static void setPixelColorRange(uint16_t first, uint16_t count, uint32_t color);
static void setPixelBrightnessRange(uint16_t first, uint16_t count, uint8_t brightness);
static void clearPixelColorRange(uint16_t first, uint16_t count);
static void handleSectionBlink(LED_Control_t& sectionState, uint32_t& lastBlinkTime, 
    bool& isWaitingForSync, bool& physicalOn, 
    uint16_t startIdx, uint16_t count, uint32_t syncTargetTime_ms);
static void update_LED_WS2812(void);
void LED_WS2812_switch(bool enable);
void LED_WS2812_TriggerBlinkResync(void);

// 初始化LED模块
void LED_WS2812_init()
{
    // 配置 GPIO40为输出模式
    pinMode(LED_POWER_GPIO_PIN, OUTPUT);
    // 使能 LED
    LED_WS2812_switch(1);
    vTaskDelay(pdMS_TO_TICKS(10));

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
        .blinkRate      = BLINK_RATE_30, // 初始值，常亮不关心频率
        .brightness     = 10,
        .color          = COLOR_YELLOW
    };
    actualYellowState = normalState; // 黄区初始为黄灯常亮
    actualRedState = {false, 30, 0, COLOR_OFF}; // 红区初始为熄灭
    pendingNormalStateUpdated = false; // 初始时，没有待处理的LoRa指令

    

    // 清空所有灯，确保初始状态
    strip.clear();
    strip.show();
}

// 返回 LoRa 命令控制的 normal状态。
void LED_WS2812_GetState(LED_Control_t& curState)
{
    // if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    // {
        if (pendingNormalStateUpdated) {
            curState = pendingNormalState; // 如果有待处理的LoRa命令，返回它
        } 
        else {
            curState = normalState; // 否则，返回当前已生效的 normal状态
        }
    //    xSemaphoreGive(ledControlMutex);
    //} 
    //else {
    //    Serial.println("[LED] Warning: GetState failed to acquire mutex!");
    //    curState = normalState; 
    //}
}

// 这是 LoRaHandler 修改 normal状态的主要入口。
// 根据雷达灯是否激活来决定是立即应用还是缓存。
void LED_WS2812_SetState(const LED_Control_t &newState)
{
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        bool needsResync =  (newState.isBlinking && !normalState.isBlinking) ||
                            (newState.color != normalState.color) ||
                            (newState.blinkRate != normalState.blinkRate) ||
                            (newState.brightness != normalState.brightness);
        if (needsResync) {
            g_trigger_resync = true;
        }
        bool radarActive = false;
        if(radarModuleEnabled == true){
            radarActive = Radar_IsActiveOrExtending();
        }
        else{
            radarActive = false;
        }
        
        // 无论如何，LoRa指令都会立即更新 normalState
        normalState = newState; 
        
         // 分区更新 actualYellowState 和 actualRedState
         if (newState.color == COLOR_YELLOW) {              // normal状态在黄区
            actualYellowState = newState;                   // 直接更新黄区实际状态
            actualRedState = {false, 30, 0, COLOR_OFF};     // 红区熄灭
            pendingNormalStateUpdated = false;              // 清除任何针对红区的缓存
            Serial.println("[LED] LoRa: Yellow command applied directly to normalState and actualYellowState.");
        } else {                                            // normal状态在红区或全部熄灭
            actualYellowState = {false, 30, 0, COLOR_OFF};  // 黄区熄灭

            if (radarActive) {
                // 雷达活跃时，红色或关灯指令会被缓存
                pendingNormalState = newState;
                pendingNormalStateUpdated = true;
                // actualRedState 不在这里被修改，因为它被雷达优先覆盖
                Serial.println("[LED] LoRa: Red/OFF command cached for actualRedState due to active radar.");
            } else {
                // 雷达不活跃，红色或关灯指令直接更新红区实际状态
                actualRedState = newState; 
                pendingNormalStateUpdated = false;
                Serial.println("[LED] LoRa: Red/OFF command applied directly to normalState and actualRedState, cache cleared.");
            }
        }
        xSemaphoreGive(ledControlMutex);
    }
}

// 通过设置标志位来请求 update_LED_WS2812 任务执行重同步
void LED_WS2812_TriggerBlinkResync(void)
{
    // 获取互斥锁以安全地修改标志位
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) {
        g_trigger_resync = true;
        xSemaphoreGive(ledControlMutex);
    }
}

// 供 RadarModule 强制设置雷达灯状态
// 只修改 actualRedState，不影响 actualYellowState和 normalState
void LED_WS2812_ForceSetState(const LED_Control_t& newState) {
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) {
        // 强制状态亮度等同 normal 亮度
        LED_Control_t forceSetState = newState;
        forceSetState.brightness = normalState.brightness;
        // 强制 actualRedState 为强制状态
        actualRedState = forceSetState; 
        // Serial.println("[LED] ForceSetState: Forcing actualRedState to Radar state.");
        xSemaphoreGive(ledControlMutex);
    }
}

// 供 RadarModule 在雷达灯结束时调用
// 根据是否有缓存的 LoRa 命令来决定是应用该命令还是恢复到雷达激活前的 normal状态
void LED_WS2812_ApplyPendingOrRestore() { // 不再需要 restoreState 参数
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) {
        Serial.println("[LED] LED_WS2812_ApplyPendingOrRestore called.");
        if (pendingNormalStateUpdated) {
            // 有待处理的LoRa指令 (针对红色区域的 LoRa 命令被缓存了)
            actualRedState = pendingNormalState; // 将缓存的 LoRa 命令应用到 actualRedState
            normalState = pendingNormalState; // 同时更新 normalState
            pendingNormalStateUpdated = false; // 清除待处理标志
            if (actualRedState.isBlinking) {
                g_trigger_resync = true;
            }
            Serial.println("[LED] ApplyPendingOrRestore: Applying pending LoRa command for Red zone to actualRedState and normalState.");
        } else {
            // 没有待处理的LoRa指令，恢复到当前 normalState 所期望的红色部分
            // 不涉及黄灯部分，只需恢复红灯部分。
            if (normalState.color == COLOR_RED || normalState.color == COLOR_OFF) {
                actualRedState = normalState; // 如果 normalState 期望红色或关闭，则恢复
                if (actualRedState.isBlinking) {
                    g_trigger_resync = true;
                }
            } 
            else {
                actualRedState = {false, 30, 0, COLOR_OFF}; // 如果 normalState 期望黄色，则红区熄灭
            }
            Serial.println("[LED] ApplyPendingOrRestore: No pending command, restoring actualRedState based on current normalState.");
        }
        xSemaphoreGive(ledControlMutex);
    }
}

// LED_WS2812_SetX 函数现在会调用 LED_WS2812_GetState 获取 normal状态
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
    // 各分区独立的闪烁计时器
    static uint32_t lastYellowBlinkTime = 0;    
    static uint32_t lastRedBlinkTime = 0; 

    // 各分区独立的等待同步标志
    static bool isYellowWaitingForSync = false;
    static bool isRedWaitingForSync = false;

    // 每个分区固定的目标同步时间
    static uint32_t yellowSyncTargetTime_ms = 0;
    static uint32_t redSyncTargetTime_ms = 0;

    // 各分区独立的亮灭状态（用于闪烁时翻转）
    static bool yellowPhysicalOn = false; // true: 亮, false: 灭
    static bool redPhysicalOn = false;    // true: 亮, false: 灭

    // 用于检测实际状态变化，以触发闪烁同步重置
    static LED_Control_t prevActualYellowState; 
    static LED_Control_t prevActualRedState; 

    LED_Control_t currentActualYellowState; 
    LED_Control_t currentActualRedState; 
    
    // 获取当前 actualYellowState 和 actualRedState，使用互斥锁确保线程安全
    if (xSemaphoreTake(ledControlMutex, portMAX_DELAY) == pdTRUE) 
    {
        if (g_trigger_resync) {
            uint32_t current_s = getTime_s();

            if (actualYellowState.isBlinking) {
                isYellowWaitingForSync = true;
                // MODIFIED: 计算一次目标时间并存储
                yellowSyncTargetTime_ms = (current_s % 2 == 0) ? (current_s + 2) * 1000 : (current_s + 1) * 1000;
                clearPixelColorRange(YELLOW_LED_START_IDX, NUM_YELLOW_LEDS); // 熄灭作为反馈
            }
            if (actualRedState.isBlinking) {
                isRedWaitingForSync = true;
                // MODIFIED: 计算一次目标时间并存储
                redSyncTargetTime_ms = (current_s % 2 == 0) ? (current_s + 2) * 1000 : (current_s + 1) * 1000;
                clearPixelColorRange(RED_LED_START_IDX, NUM_RED_LEDS); // 熄灭作为反馈
            }
            
            strip.show(); // 立即应用熄灭效果
            g_trigger_resync = false;
        }

        currentActualYellowState = actualYellowState;
        currentActualRedState = actualRedState; 
        xSemaphoreGive(ledControlMutex);
    } 
    else 
    {
        Serial.println("[LED] Warning: update_LED_WS2812 failed to acquire mutex!");
        return;
    }

    uint32_t currentTime = getTime_ms(); // 获取当前时间

    
    // 将计算好的目标时间传递给 handleSectionBlink 函数
    handleSectionBlink(currentActualYellowState, lastYellowBlinkTime, isYellowWaitingForSync,
        yellowPhysicalOn, YELLOW_LED_START_IDX, NUM_YELLOW_LEDS, yellowSyncTargetTime_ms);

    handleSectionBlink(currentActualRedState, lastRedBlinkTime, isRedWaitingForSync,
        redPhysicalOn, RED_LED_START_IDX, NUM_RED_LEDS, redSyncTargetTime_ms);

    // 更新 prev 状态，用于下一次循环的检测
    prevActualYellowState = currentActualYellowState;
    prevActualRedState = currentActualRedState;

    strip.show(); // 集中更新LED灯带
}

// 辅助函数，处理单个分区闪烁逻辑
static void handleSectionBlink(LED_Control_t& sectionState, uint32_t& lastBlinkTime, 
                               bool& isWaitingForSync, bool& physicalOn, 
                               uint16_t startIdx, uint16_t count, uint32_t syncTargetTime_ms)
{
    uint32_t currentTime = getTime_ms();
    
    if (sectionState.color != COLOR_OFF) {
        if (sectionState.isBlinking) {
            uint32_t blinkInterval = (60000 / sectionState.blinkRate) / 2;

            if (isWaitingForSync) {
                // 使用 getSafeTimeDiff_ms 更安全地处理时间戳回绕问题
                if ((int32_t)(currentTime - syncTargetTime_ms) >= 0) {
                    setPixelColorRange(startIdx, count, sectionState.color);
                    setPixelBrightnessRange(startIdx, count, sectionState.brightness);
                    lastBlinkTime = syncTargetTime_ms; 
                    isWaitingForSync = false;
                    physicalOn = true;
                }
            } else {
                if (getSafeTimeDiff_ms(currentTime, lastBlinkTime) >= blinkInterval) {
                    physicalOn = !physicalOn;
                    if (physicalOn) { 
                        setPixelColorRange(startIdx, count, sectionState.color);
                        setPixelBrightnessRange(startIdx, count, sectionState.brightness);
                    } else { 
                        clearPixelColorRange(startIdx, count);
                    }
                    while (getSafeTimeDiff_ms(currentTime, lastBlinkTime) >= blinkInterval) {
                        lastBlinkTime += blinkInterval;
                    }
                }
            }
        } else {
            if ((strip.getPixelColor(startIdx) != sectionState.color) || 
                (strip.getBrightness() != sectionState.brightness) || !physicalOn) { 
                setPixelColorRange(startIdx, count, sectionState.color);
                setPixelBrightnessRange(startIdx, count, sectionState.brightness);
                physicalOn = true;
            }
            lastBlinkTime = currentTime;
            isWaitingForSync = false;
        }
    } else {
        if (strip.getPixelColor(startIdx) != 0 || physicalOn) { 
            clearPixelColorRange(startIdx, count);
            physicalOn = false;
        }
        lastBlinkTime = currentTime;
        isWaitingForSync = false;
    }
}


// 辅助函数，操作特定像素范围的颜色
static void setPixelColorRange(uint16_t first, uint16_t count, uint32_t color) {
    for (int i = 0; i < count; i++) {
        strip.setPixelColor(first + i, color);
    }
}

// 辅助函数，操作特定像素范围的亮度
static void setPixelBrightnessRange(uint16_t first, uint16_t count, uint8_t brightness) {
    // Adafruit_NeoPixel 的 setBrightness 是全局的，不能按范围设置。
    // 如果需要分区亮度，需要手动计算每个像素的颜色值。
    // 目前保持全局亮度设置，但逻辑上只影响被点亮的区域。
    // 如果未来需要更精细的分区亮度控制，此处需重构。
    strip.setBrightness(brightness);
}

// 辅助函数，清除特定像素范围的颜色（熄灭）
static void clearPixelColorRange(uint16_t first, uint16_t count) {
    for (int i = 0; i < count; i++) {
        strip.setPixelColor(first + i, strip.Color(0, 0, 0));
    }
}

// LED使能总开关
void LED_WS2812_switch(bool enable){

    if(enable == 1){
        // 拉高GPIO40，开启LED灯使能
        digitalWrite(LED_POWER_GPIO_PIN, HIGH);
        Serial.println("[LED]Switch on");
    }
    else if(enable == 0){
        vTaskDelay(pdMS_TO_TICKS(10));
        // 拉低GPIO40，关闭LED灯使能
        digitalWrite(LED_POWER_GPIO_PIN, LOW);
        Serial.println("[LED]Switch off");
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