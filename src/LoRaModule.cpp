#include "LoRaModule.h"
#include "LoRaHandler.h"
#include "driver/uart.h"
// 内部局部变量
static const uint64_t   LoRa_RX = 18;
static const uint64_t   LoRa_TX = 17;


// 外部使用的变量

TaskHandle_t        loraReceiveTaskHandle      = NULL;
TaskHandle_t        heartBeatTaskHandle        = NULL;  // 心跳任务句柄


static void receiveData_Test();
static void receiveData();
static void receiveData_IDF();
static void sendData_Arduino(const String &payload);
static void sendData_IDF(const String &payload);

void LoRa_init()
{
    Serial1.begin(9600, SERIAL_8N1, LoRa_RX, LoRa_TX);
    delay(10000);
    sendData("1");
}

void sendData(const String &payload)
{
    // sendData_Arduino(payload);
    sendData_IDF(payload);
}

void LoRa_init_IDF()
{
    // 初始化串口
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .source_clk = UART_SCLK_APB,
    };
    // 初始化UART1
    uart_param_config(UART_NUM_1, &uart_config);
    uart_driver_install(UART_NUM_1, 1024, 0, 0, NULL, 0);
    uart_set_pin(UART_NUM_1, LoRa_TX, LoRa_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    delay(500);
    addMuticast_IDF("01651ed0", "e4449b09cca06c405a1c1509f7d9a40b", "aa97576782b624e2cbeb82f5f8ab066b");
}


void joinNetwork_IDF(bool joinMode){
    // 构建AT指令 - 加入网络
    String command = "AT+CJOIN=";
    command += String(joinMode ? 1 : 0);  // 根据布尔值设置第一个参数
    command += ",0,8,2";  // 固定参数
    command += "\n";  // 添加换行符

    // 使用ESP-IDF UART API发送数据
    uart_write_bytes(UART_NUM_1, command.c_str(), command.length());
}

void addMuticast_IDF(const String &DevAddr, const String &AppSKey, const String &NwkSKey){
    // 添加多播配置
    String command = "AT+CADDMUTICAST=";
    command += DevAddr;  // DevAddr
    command += ",";
    command += AppSKey;  // AppSKey
    command += ",";
    command += NwkSKey;  // NwkSKey
    command += "\n";  // 添加换行符

    // 使用ESP-IDF UART API发送数据
    joinNetwork_IDF(0);
    delay(500);
    uart_write_bytes(UART_NUM_1, command.c_str(), command.length());
    delay(500);
    joinNetwork_IDF(1);
    delay(10000);
    sendData("1");
    delay(2000);
}

void sendData_IDF(const String &payload)
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
    command += "\n";  // 添加换行符

    // 使用ESP-IDF UART API发送数据
    uart_write_bytes(UART_NUM_1, command.c_str(), command.length());
}

void sendData_Arduino(const String &payload)
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

void receiveData()
{
    static int parseState = 0;  // 0: 等待rx行, 1: 等待payload行
    static uint8_t currentPort = 0;
/** *
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
        if (response.startsWith("rx:")) 
        {
            /** *
            Serial.println("[LoRa]: "+response);
            /** */
            parseState = 1;
            // 解析port值
            int portIndex = response.indexOf("port =");
            if (portIndex >= 0) 
            {
                currentPort = response.substring(portIndex + 6).toInt();
            }
            /**
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
        else if (parseState == 1 && response.indexOf("0x") >= 0) 
        {
            /** *
            Serial.println("[LoRa]: "+response);
            /** */
            parseState = 0;
            /** *
            // 计算延迟执行时间
            uint32_t compensationDelay = SYNC_DELAY_MS - LoRa_Connect_Delay;
            scheduleCommand(currentPort, response, compensationDelay);
            /** */
            // Serial.println("port    = "+String(currentPort));
            // Serial.println("payload = "+response);
            handlePayload(currentPort,response);
        }
    }    
}

void receiveData_IDF()
{
    static int parseState = 0;  // 0: 等待rx行, 1: 等待payload行
    static uint8_t currentPort = 0;
    static char line_buffer[512];  // 用于存储未处理完的数据
    static int line_len = 0;       // 当前line_buffer中的数据长度
    char rx_buffer[256];          // 临时接收缓冲区
    int length = 0;

    // 检查UART是否有数据可读
    ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_1, (size_t*)&length));
    
    if (length > 0) 
    {
        // 读取数据
        length = uart_read_bytes(UART_NUM_1, (uint8_t*)rx_buffer, (length < sizeof(rx_buffer) - 1) ? length : sizeof(rx_buffer) - 1, 0);
        rx_buffer[length] = '\0';  // 确保字符串结束

        // 处理接收到的数据
        for (int i = 0; i < length; ++i) 
        {
            if (rx_buffer[i] == '\n') 
            {
                // 找到完整的一行，处理它
                line_buffer[line_len] = '\0';  // 确保字符串结束
                String response = String(line_buffer);
                response.trim();  // 移除首尾空格
                // Serial.println("[LoRa]:"+response);
                // 检查是否是rx行
                if (response.startsWith("rx:")) 
                {
                    parseState = 1;
                    // 解析port值
                    int portIndex = response.indexOf("port =");
                    if (portIndex >= 0) 
                    {
                        currentPort = response.substring(portIndex + 6).toInt();
                    }

                }
                // 检查是否是payload行（以0x开头）
                else if (parseState == 1 && response.indexOf("0x") >= 0) 
                {
                    parseState = 0;
                    /**
                    Serial.print("port   : ");
                    Serial.println(currentPort);
                    Serial.print("payload: ");
                    Serial.println(response);
                    /**/
                    handlePayload(currentPort, response);
                }

                // 重置line_buffer
                line_len = 0;
            } 
            else 
            {
                // 将字符添加到line_buffer
                if (line_len < sizeof(line_buffer) - 1) 
                {
                    line_buffer[line_len++] = rx_buffer[i];
                }
            }
        }
    }
}


void loraReceiveTask(void *pvParameters)
{
    while(true)
    {
        receiveData_IDF();
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms延时
    }
}

void heartBeatTask(void *pvParameters)
{
    // const TickType_t baseDelay = pdMS_TO_TICKS(30*60*1000);  // 基础延时时间
    const TickType_t baseDelay = pdMS_TO_TICKS(1*2*1000);  // 基础延时时间
    while(true)
    {
        sendData("09");
        // 生成随机延时时间，范围为基础延时时间的5%
        TickType_t randomDelay = baseDelay + pdMS_TO_TICKS((rand() % 101) - 50); // 生成-5%到5%的随机延时
        vTaskDelay(randomDelay);
    }
}

void receiveData_Test()
{
    static int parseState = 0;  // 0: 等待rx行, 1: 等待payload行
    static uint8_t currentPort = 0;
    static char line_buffer[512];  // 用于存储未处理完的数据
    static int line_len = 0;       // 当前line_buffer中的数据长度
    char rx_buffer[256];          // 临时接收缓冲区
    int length = 0;

    // 检查UART是否有数据可读
    ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_1, (size_t*)&length));
    
    if (length > 0) 
    {
        // 读取数据
        length = uart_read_bytes(UART_NUM_1, (uint8_t*)rx_buffer, (length < sizeof(rx_buffer) - 1) ? length : sizeof(rx_buffer) - 1, 0);
        rx_buffer[length] = '\0';  // 确保字符串结束

        // 处理接收到的数据
        for (int i = 0; i < length; ++i) 
        {
            if (rx_buffer[i] == '\n') 
            {
                // 找到完整的一行，处理它
                line_buffer[line_len] = '\0';  // 确保字符串结束
                String response = String(line_buffer);
                response.trim();  // 移除首尾空格
                Serial.println("[LoRa]:"+response);
/** *
                // 检查是否是rx行
                if (response.startsWith("rx:")) 
                {
                    parseState = 1;
                    // 解析port值
                    int portIndex = response.indexOf("port =");
                    if (portIndex >= 0) 
                    {
                        currentPort = response.substring(portIndex + 6).toInt();
                    }

                }
                // 检查是否是payload行（以0x开头）
                else if (parseState == 1 && response.indexOf("0x") >= 0) 
                {
                    parseState = 0;
                    handlePayload(currentPort, response);
                }
/** */
                // 重置line_buffer
                line_len = 0;
            } 
            else 
            {
                // 将字符添加到line_buffer
                if (line_len < sizeof(line_buffer) - 1) 
                {
                    line_buffer[line_len++] = rx_buffer[i];
                }
            }
        }
    }
}
