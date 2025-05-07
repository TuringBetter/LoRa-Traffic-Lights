#include "LoRaModule.h"
#include "LedModule.h"
#include "FlashingLightModule.h"
// 内部局部变量
static const uint64_t   LoRa_RX = 18;
static const uint64_t   LoRa_TX = 17;

static LedColor _last_color;
static uint16_t _last_frequency;
static uint16_t _last_brightness;
static unsigned long _messageReceiveTime = 0;  // 记录车辆通过的时间

static uint32_t LoRa_Connect_Delay = 800;    // 通信延迟时间
static uint32_t LoRa_Send_TIME = 0;        // 发送时间
static uint32_t LoRa_Recv_TIME = 0;        // 接收时间
static bool     waitingForResponse=false;

static const uint32_t SYNC_DELAY_MS = 1000;  // 同步延迟时间（1秒）
static ScheduledCommand scheduledCommand;  // 存储待执行的命令
static bool hasScheduledCommand = false;   // 是否有待执行的命

// 外部使用的变量
SemaphoreHandle_t latencySemaphore;  // 延迟测量完成信号量
TaskHandle_t loraTestTaskHandle  = NULL;
TaskHandle_t latencyTaskHandle   = NULL;  // 延迟测量任务句柄


// 声明外部变量
extern volatile LedState              ledstate;
extern bool                 _ledStateChanged;
extern SemaphoreHandle_t    _ledStateMutex;


static void receiveData();
static void measureLatency();
static void scheduleCommand(uint8_t port, const String& payload, uint32_t delay_ms);
static void handlePayload(uint8_t port, const String& payload);
static uint32_t getLatency();

void sendData(const String &payload);

void LoRa_init()
{
    Serial1.begin(9600, SERIAL_8N1, LoRa_RX, LoRa_TX);
    // sendData("1111");
    // delay(1000);
    sendData("1111");
    delay(1000);
    // 创建延迟测量信号量
    latencySemaphore = xSemaphoreCreateBinary();
}

void sendData(const String &payload)
{
    // 构建AT指令
    String command = "AT+DTRX=";
    command += String(0);
    command += ",";
    command += String(1);
    command += ",";
    command += String(payload.length());
    command += ",";
    command += payload;

    // Serial.println(command);
    // 发送命令
    Serial1.println(command);
}

uint32_t getLatency()
{
    // 等待延迟测量完成
    if (xSemaphoreTake(latencySemaphore, pdMS_TO_TICKS(5000)) == pdTRUE) 
    {
        return LoRa_Connect_Delay;
    } 
    else 
    {
        // Serial.println("获取延迟值超时");
        return 0;
    }
}

static void receiveData()
{
    static int parseState = 0;  // 0: 等待rx行, 1: 等待payload行
    static uint8_t currentPort = 0;
/** */
    // 检查是否有待执行的命令
    if (hasScheduledCommand && millis() >= scheduledCommand.executeTime) 
    {
        handlePayload(scheduledCommand.port, scheduledCommand.payload);
        hasScheduledCommand = false;
    }
/** */
    if (Serial1.available()) 
    {
        String response = Serial1.readStringUntil('\n'); // 读取一行响应
        response.trim();  // 移除首尾空格
        // Serial.println("[LoRa]: "+response);

        // 检查是否是rx行
        if (response.startsWith("rx:")) {
            /** *
            Serial.println("[LoRa]: "+response);
            /** */
            parseState = 1;
            // 解析port值
            int portIndex = response.indexOf("port =");
            if (portIndex >= 0) {
                currentPort = response.substring(portIndex + 6).toInt();
            }
            /**/
            // 如果是延迟测量响应
            if (waitingForResponse) {
                LoRa_Recv_TIME = millis();
                LoRa_Connect_Delay = (LoRa_Recv_TIME - LoRa_Send_TIME) / 2;
                waitingForResponse = false;
                // 释放信号量，表示测量完成
                xSemaphoreGive(latencySemaphore);
            }
            /**/
        }
        // 检查是否是payload行（以0x开头）
        else if (parseState == 1 && response.indexOf("0x") >= 0) {
            /** *
            Serial.println("[LoRa]: "+response);
            /** */
            parseState = 0;
            /** */
            // 计算延迟执行时间
            uint32_t compensationDelay = SYNC_DELAY_MS - LoRa_Connect_Delay;
            scheduleCommand(currentPort, response, compensationDelay);
            /** */
            handlePayload(currentPort,response);
        }
    }    
}

void measureLatency()
{
    sendData("06");
    LoRa_Send_TIME = millis();
    waitingForResponse = true;
}

void loraTestTask(void *pvParameters)
{
    while(true)
    {
        receiveData();
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms延时
    }
}

void latencyTask(void *pvParameters)
{
    const TickType_t xDelay = pdMS_TO_TICKS(10*60*1000);  // 每10min测量一次延迟
    // const TickType_t xDelay = pdMS_TO_TICKS(1*2*1000);  // 每10min测量一次延迟
    
    while(true) {
        // 测量通信延迟
        measureLatency();
        /** *
        // 获取并打印延迟值
        uint32_t latency = getLatency();
        if(latency!=0)
        {
            Serial.print("当前通信延迟: ");
            Serial.print(latency);
            Serial.println(" ms");
        }
        /** */
        // 任务延时
        vTaskDelay(xDelay);
    }    
}
void scheduleCommand(uint8_t port, const String &payload, uint32_t delay_ms)
{
    scheduledCommand.port = port;
    scheduledCommand.payload = payload;
    scheduledCommand.executeTime = millis() + delay_ms;
    hasScheduledCommand = true;
}

void handlePayload(uint8_t port, const String& payload)
{
    Serial.println("port: "+String(port)+" data: "+payload);
        switch(port) 
        {
            case 10: // 设置闪烁频率
            {
                if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) 
                {
                    uint8_t freq = strtol(payload.substring(payload.indexOf("0x")).c_str(), NULL, 16);
                    switch(freq) {
                        case 0x1E: // 30Hz
                            ledstate.frequency = 30;
                            break;
                        case 0x3C: // 60Hz
                            ledstate.frequency = 60;
                            break;
                        case 0x78: // 120Hz
                            ledstate.frequency = 120;
                            break;
                    }
                    _ledStateChanged = true;
                    xSemaphoreGive(_ledStateMutex);
                }
                break;
            }
            case 11: // 设置LED颜色
            {
                if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) 
                {
                    uint8_t color = strtol(payload.substring(payload.indexOf("0x")).c_str(), NULL, 16);
                    ledstate.color = (color == 0x00) ? LedColor::RED : LedColor::YELLOW;
                    _ledStateChanged = true;
                    xSemaphoreGive(_ledStateMutex);
                }
                break;
            }
            case 12: // 设置是否闪烁
            {
                if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) 
                {
                    uint8_t manner = strtol(payload.substring(payload.indexOf("0x")).c_str(), NULL, 16);
                    ledstate.frequency = (manner == 0x00) ? 60 : 0; // 闪烁时默认60Hz，常亮时频率为0
                    _ledStateChanged = true;
                    xSemaphoreGive(_ledStateMutex);
                }
                break;
            }
            case 13: // 设置亮度
            {
                if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) 
                {
                    String payloadStr = payload;
                    int firstHex = payloadStr.indexOf("0x");
                    int secondHex = payloadStr.indexOf("0x", firstHex + 2);
                    if (firstHex >= 0 && secondHex >= 0) 
                    {
                        uint8_t high = strtol(payloadStr.substring(firstHex, firstHex + 4).c_str(), NULL, 16);
                        uint8_t low = strtol(payloadStr.substring(secondHex, secondHex + 4).c_str(), NULL, 16);
                        uint16_t brightness = (high << 8) | low;
                        ledstate.brightness = brightness;
                        _ledStateChanged = true;
                    }
                    xSemaphoreGive(_ledStateMutex);
                }
                break;
            }
            case 14: // 设备开关控制
            {
                if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) 
                {
                    String payloadStr = payload;
                    int firstHex = payloadStr.indexOf("0x");
                    if (firstHex >= 0) 
                    {
                        uint8_t status = strtol(payloadStr.substring(firstHex, firstHex + 4).c_str(), NULL, 16);
                        if (status == 0x00) ledstate.brightness = 0;
                        else if (status == 0x01) ledstate.brightness = 500; // 默认亮度
                        _ledStateChanged = true;
                    }
                    xSemaphoreGive(_ledStateMutex);
                }
                break;
            }
            case 15: // 设置整体状态
            {
                if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) 
                {
                    String payloadStr = payload;
                    int firstHex = payloadStr.indexOf("0x");
                    if (firstHex >= 0) 
                    {
                        // 解析颜色（第1字节）
                        uint8_t color = strtol(payloadStr.substring(firstHex, firstHex + 4).c_str(), NULL, 16);
                        ledstate.color = (color == 0x00) ? LedColor::RED : LedColor::YELLOW;

                        // 解析频率（第2字节）
                        int secondHex = payloadStr.indexOf("0x", firstHex + 4);
                        if (secondHex >= 0) 
                        {
                            uint8_t freq = strtol(payloadStr.substring(secondHex, secondHex + 4).c_str(), NULL, 16);
                            switch(freq) 
                            {
                                case 0x1E: ledstate.frequency = 30; break;
                                case 0x3C: ledstate.frequency = 60; break;
                                case 0x78: ledstate.frequency = 120; break;
                            }

                            // 解析亮度（第3-4字节）
                            int thirdHex = payloadStr.indexOf("0x", secondHex + 4);
                            int fourthHex = payloadStr.indexOf("0x", thirdHex + 4);
                            if (thirdHex >= 0 && fourthHex >= 0) 
                            {
                                uint8_t high = strtol(payloadStr.substring(thirdHex, thirdHex + 4).c_str(), NULL, 16);
                                uint8_t low = strtol(payloadStr.substring(fourthHex, fourthHex + 4).c_str(), NULL, 16);
                                uint16_t brightness = (high << 8) | low;
                                ledstate.brightness = brightness;

                                // 解析亮灯方式（第5字节）
                                int fifthHex = payloadStr.indexOf("0x", fourthHex + 4);
                                if (fifthHex >= 0) 
                                {
                                    uint8_t manner = strtol(payloadStr.substring(fifthHex, fifthHex + 4).c_str(), NULL, 16);
                                    if (manner == 0x00) { // 闪烁
                                        // 保持之前设置的频率
                                    } else if (manner == 0x01) { // 常亮
                                        ledstate.frequency = 0;
                                    }
                                    _ledStateChanged = true;
                                }
                            }
                        }
                    }
                    xSemaphoreGive(_ledStateMutex);
                }
                break;
            }
            case 16:
            {
                FlashingLight_on();
                break;
            }
            case 17:
            {
                FlashingLight_off();
                break;
            }
            case 20: // 车辆通过状态
            {
                if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) 
                {
                    // 保存历史状态
                    _last_color = ledstate.color;
                    _last_frequency = ledstate.frequency;
                    _last_brightness = ledstate.brightness;

                    // 更新为车辆通过状态
                    ledstate.color = LedColor::RED;
                    ledstate.frequency = 120;
                    ledstate.brightness = 7000;
                    _messageReceiveTime = millis();  // 记录当前时间
                    _ledStateChanged = true;
                    xSemaphoreGive(_ledStateMutex);
                }
                break;
        }
    }   
}
void ledAutoShutDownTask(void *pvParameters)
{
    while(true)
    {
        if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE)
        {
            // 检查车辆通过状态是否超时
            if (_messageReceiveTime > 0 && (millis() - _messageReceiveTime) >= 5000) {
                // 恢复为历史状态
                ledstate.color = _last_color;
                ledstate.frequency = _last_frequency;
                ledstate.brightness = _last_brightness;
                _messageReceiveTime = 0;  // 重置时间
                _ledStateChanged = true;
            }
            xSemaphoreGive(_ledStateMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms延时
    }
}