#include "LoRaModule.h"
/**
//初始化LoRa模块
void LoRa::begin() 
{
    Serial1.begin(9600, SERIAL_8N1, 18, 17);
    sendData(LoRa::SendMode::UNCONFIRMED,1,"111");
    // 创建延迟测量信号量
    latencySemaphore = xSemaphoreCreateBinary();
}

void LoRa::sendData(SendMode mode, uint8_t trials, const String& payload) {
    // 验证重试次数
    if (trials < 1 || trials > 7) {
        trials = 1;  // 默认值
    }

    // 构建AT指令
    String command = "AT+DTRX=";
    command += String(mode);
    command += ",";
    command += String(trials);
    command += ",";
    command += String(payload.length());
    command += ",";
    command += payload;

    // Serial.println(command);
    // 发送命令
    Serial1.println(command);

    // 记录发送时间
    LoRa_Send_TIME = millis();
    waitingForResponse = true;
}

void LoRa::receiveData()
{
    static int parseState = 0;  // 0: 等待rx行, 1: 等待payload行
    static uint8_t currentPort = 0;
/** *
    // 检查是否有待执行的命令
    if (hasScheduledCommand && millis() >= scheduledCommand.executeTime) {
        handlePayload(scheduledCommand.port, scheduledCommand.payload);
        hasScheduledCommand = false;
    }
/** *
    if (Serial1.available()) 
    {
        String response = Serial1.readStringUntil('\n'); // 读取一行响应
        response.trim();  // 移除首尾空格

        // 检查是否是rx行
        if (response.startsWith("rx:")) {
            /** *
            Serial.println("[LoRa]: "+response);
            /** *
            parseState = 1;
            // 解析port值
            int portIndex = response.indexOf("port =");
            if (portIndex >= 0) {
                currentPort = response.substring(portIndex + 6).toInt();
            }

            // 如果是延迟测量响应
            if (waitingForResponse) {
                LoRa_Recv_TIME = millis();
                LoRa_Connect_Delay = (LoRa_Recv_TIME - LoRa_Send_TIME) / 2;
                waitingForResponse = false;
                // 释放信号量，表示测量完成
                xSemaphoreGive(latencySemaphore);
            }
        }
        // 检查是否是payload行（以0x开头）
        else if (parseState == 1 && response.indexOf("0x") >= 0) {
            /** *
            Serial.println("[LoRa]: "+response);
            /** *
            parseState = 0;
            /** *
            // 计算延迟执行时间
            uint32_t compensationDelay = SYNC_DELAY_MS - LoRa_Connect_Delay;
            scheduleCommand(currentPort, response, compensationDelay);
            /** *
        }
    }
}

// 延迟执行命令
void LoRa::scheduleCommand(uint8_t port, const String& payload, uint32_t delay_ms) {
    scheduledCommand.port = port;
    scheduledCommand.payload = payload;
    scheduledCommand.executeTime = millis() + delay_ms;
    hasScheduledCommand = true;
}

// 测量通信延迟
void LoRa::measureLatency() {
    // 发送一个简单的测试消息
    sendData(LoRa::SendMode::UNCONFIRMED,1,"06");
}

// 获取当前延迟值
uint32_t LoRa::getLatency() {
    // 等待延迟测量完成
    if (xSemaphoreTake(latencySemaphore, pdMS_TO_TICKS(5000)) == pdTRUE) {
        return LoRa_Connect_Delay;
    } else {
        // Serial.println("获取延迟值超时");
        return 0;
    }
}

void LoRa::handlePayload(uint8_t port, const String& payload) {
    if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) {
        switch(port) {
            case 10: {
                // 设置闪烁频率
                uint8_t freq = strtol(payload.substring(payload.indexOf("0x")).c_str(), NULL, 16);
                switch(freq) {
                    case 0x1E: 
                        // Serial.println("设置闪烁频率: 30Hz");
                        _ledState.frequency = 30;
                        break;
                    case 0x3C: 
                        // Serial.println("设置闪烁频率: 60Hz");
                        _ledState.frequency = 60;
                        break;
                    case 0x78: 
                        // Serial.println("设置闪烁频率: 120Hz");
                        _ledState.frequency = 120;
                        break;
                }
                _ledStateChanged = true;
                break;
            }
            case 11: {
                // 设置LED颜色
                uint8_t color = strtol(payload.substring(payload.indexOf("0x")).c_str(), NULL, 16);
                _ledState.color = color == 0 ? LedColor::RED : LedColor::YELLOW;
                // Serial.println(color == 0 ? "设置颜色: 红色" : "设置颜色: 黄色");
                _ledStateChanged = true;
                break;
            }
            case 12: {
                // 设置是否闪烁
                uint8_t mode = strtol(payload.substring(payload.indexOf("0x")).c_str(), NULL, 16);
                _ledState.frequency = mode == 0 ? 60 : 0; // 闪烁时默认60Hz
                // Serial.println(mode == 0 ? "设置模式: 闪烁" : "设置模式: 常亮");
                _ledStateChanged = true;
                break;
            }
            case 13: {
                // 设置亮度
                String payloadStr = payload;
                int firstHex = payloadStr.indexOf("0x");
                int secondHex = payloadStr.indexOf("0x", firstHex + 2);
                if (firstHex >= 0 && secondHex >= 0) {
                    uint8_t high = strtol(payloadStr.substring(firstHex, firstHex + 4).c_str(), NULL, 16);
                    uint8_t low = strtol(payloadStr.substring(secondHex, secondHex + 4).c_str(), NULL, 16);
                    uint16_t brightness = (high << 8) | low;
                    _ledState.brightness = brightness;
                    // Serial.print("设置亮度: ");
                    // Serial.println(brightness);
                    _ledStateChanged = true;
                }
                break;
            }
            case 20:
                Serial.println("设置为车辆通过状态: 红色 + 亮度7000 + 120Hz");
                _ledState.color = LedColor::RED;
                _ledState.brightness = 7000;
                _ledState.frequency = 120;
                _ledStateChanged = true;
                break;
            case 21:
                Serial.println("设置为车辆离开状态: 黄色 + 亮度1000 + 常亮");
                _ledState.color = LedColor::YELLOW;
                _ledState.brightness = 1000;
                _ledState.frequency = 0;
                _ledStateChanged = true;
                break;
        }
        xSemaphoreGive(_ledStateMutex);
    }
}
/**/
/*==================================================================================*/

static const uint64_t   LoRa_RX = 18;
static const uint64_t   LoRa_TX = 17;

SemaphoreHandle_t latencySemaphore;  // 延迟测量完成信号量
TaskHandle_t loraTestTaskHandle  = NULL;
TaskHandle_t latencyTaskHandle   = NULL;  // 延迟测量任务句柄

static uint32_t LoRa_Connect_Delay = 800;    // 通信延迟时间
static uint32_t LoRa_Send_TIME = 0;        // 发送时间
static uint32_t LoRa_Recv_TIME = 0;        // 接收时间
static bool     waitingForResponse=false;


static const uint32_t SYNC_DELAY_MS = 1000;  // 同步延迟时间（1秒）
static ScheduledCommand scheduledCommand;  // 存储待执行的命令
static bool hasScheduledCommand = false;   // 是否有待执行的命

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
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms延时
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
void scheduleCommand(uint8_t port, const String& payload, uint32_t delay_ms) 
{
    scheduledCommand.port = port;
    scheduledCommand.payload = payload;
    scheduledCommand.executeTime = millis() + delay_ms;
    hasScheduledCommand = true;
}

void handlePayload(uint8_t port, const String& payload)
{
    if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) {
        switch(port) {
            case 10: 
            {
                Serial.println("change fre");
                break;
            }
            case 11: 
            {
                Serial.println("change color");
                break;
            }
            case 12: 
            {
                Serial.println("change manner");
                break;
            }
            case 13: 
            {
                Serial.println("change brightness");
                break;
            }
            case 20:
                Serial.println("set pass");
                break;
            case 21:
                Serial.println("set leave");
                break;
        }
        xSemaphoreGive(_ledStateMutex);
    }   
}
