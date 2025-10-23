/**
  ******************************************************************************
  * @file    LoRaModule.cpp
  * @author  陕西交通电子工程科技有限公司
  * @version V2.0.0
  * @date    2025-10-15
  * @brief   LoRa通信模块底层驱动。
  * @details
  * > 模块职责:
  * 本模块负责与LoRa硬件模组进行底层的串口(UART)通信，通过发送AT指令
  * 来控制LoRa网络操作。
  *
  * > 主要功能:
  * - 初始化与LoRa模组的UART通信。
  * - 封装设备入网、数据发送/接收等核心AT指令。
  * - 提供可靠的入网检测机制，基于串口日志实时判断入网状态。
  * - 支持自动重试机制，失败时会自动重新尝试入网。
  * - 提供`loraReceiveTask`任务，在后台持续监听并解析来自LoRa模组的原始数据。
  * - 支持从NVS加载配置并加入多播组。
  *
  * > V2.0.0 更新说明 (2025-10-15):
  * - 实现基于日志的可靠入网检测（检测"+CJOIN:OK"和"Joined"标志）
  * - 添加入网失败自动重试机制（默认最多5次）
  * - 优化代码结构，消除冗余代码
  * - 改进错误处理和日志输出
  * - 新增函数：
  *   * readLineFromUART() - 从UART读取一行数据
  *   * waitForJoinSuccess() - 等待并检测入网成功
  *   * performReliableJoin() - 执行可靠的入网流程
  *
  ******************************************************************************
  */

#include "LoRaModule.h"
#include "LoRaHandler.h"
#include "driver/uart.h"
#include "NVSManager.h"
// 内部局部变量
static const uint64_t   LoRa_RX = 18;       //LoRa设备上的 TX 接 ESP32上的 RX也就是GPIO18, 对于初版型号此处应为 17
static const uint64_t   LoRa_TX = 17;       //LoRa设备上的 RX 接 ESP32上的 TX也就是GPIO17，对于初版型号此处应为 18


// 外部使用的变量

TaskHandle_t        loraReceiveTaskHandle      = NULL;
TaskHandle_t        heartBeatTaskHandle        = NULL;  // 心跳任务句柄


static void receiveData_Test();
static void receiveData();
static void receiveData_IDF();

/**
 * @brief 从串口读取一行数据，用于入网过程中的日志监听
 * @param line_buffer 行缓冲区
 * @param line_len 当前行长度指针
 * @param timeout_ms 超时时间（毫秒）
 * @return 读取到的完整行，如果超时返回空字符串
 */
static String readLineFromUART(char* line_buffer, int* line_len, uint32_t timeout_ms)
{
    uint32_t start_time = millis();
    char rx_buffer[256];
    
    while (millis() - start_time < timeout_ms)
    {
        int length = 0;
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_1, (size_t*)&length));
        
        if (length > 0)
        {
            length = uart_read_bytes(UART_NUM_1, (uint8_t*)rx_buffer, 
                                    (length < sizeof(rx_buffer) - 1) ? length : sizeof(rx_buffer) - 1, 0);
            rx_buffer[length] = '\0';
            
            for (int i = 0; i < length; ++i)
            {
                if (rx_buffer[i] == '\n' || rx_buffer[i] == '\r')
                {
                    line_buffer[*line_len] = '\0';
                    String result = String(line_buffer);
                    *line_len = 0;  // 重置行长度
                    result.trim();
                    return result;
                }
                else
                {
                    if (*line_len < 511)  // 防止缓冲区溢出
                    {
                        line_buffer[(*line_len)++] = rx_buffer[i];
                    }
                }
            }
        }
        delay(10);  // 短暂延时，避免CPU占用过高
    }
    return "";  // 超时返回空字符串
}

/**
 * @brief 改进的串口日志检测函数，持续监听直到检测到目标字符串
 * @param target_string 要检测的目标字符串
 * @param timeout_ms 超时时间（毫秒）
 * @return true表示检测到目标字符串，false表示超时
 */
static bool waitForStringInUART(const String& target_string, uint32_t timeout_ms)
{
    uint32_t start_time = millis();
    char rx_buffer[256];
    String accumulated_data = "";
    
    Serial.println("[LoRaModule] Waiting for: " + target_string);
    
    while (millis() - start_time < timeout_ms)
    {
        int length = 0;
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_1, (size_t*)&length));
        
        if (length > 0)
        {
            length = uart_read_bytes(UART_NUM_1, (uint8_t*)rx_buffer, 
                                    (length < sizeof(rx_buffer) - 1) ? length : sizeof(rx_buffer) - 1, 0);
            rx_buffer[length] = '\0';
            
            // 将新数据添加到累积字符串
            accumulated_data += String(rx_buffer);
            
            // 打印接收到的原始数据
            Serial.print("[LoRa Raw]: ");
            Serial.print(rx_buffer);
            
            // 检查是否包含目标字符串
            if (accumulated_data.indexOf(target_string) >= 0)
            {
                Serial.println();
                Serial.println("[LoRaModule] ✓ Detected: " + target_string);
                return true;
            }
            
            // 如果累积数据太长，截断以节省内存
            if (accumulated_data.length() > 1000)
            {
                accumulated_data = accumulated_data.substring(accumulated_data.length() - 500);
            }
        }
        delay(10);  // 短暂延时，避免CPU占用过高
    }
    
    Serial.println();
    Serial.println("[LoRaModule] Timeout waiting for: " + target_string);
    return false;
}

/**
 * @brief 等待并检测LoRa入网成功的串口日志
 * @param timeout_ms 超时时间（毫秒）
 * @return true表示入网成功，false表示超时失败
 */
static bool waitForJoinSuccess(uint32_t timeout_ms)
{
    Serial.println("[LoRaModule] Waiting for join success indicators...");
    /** */
    // 方案一：只检测+CJOIN:OK（推荐）
    if (waitForStringInUART("+CJOIN:OK", timeout_ms))
    {
        Serial.println("[LoRaModule] Join successful! +CJOIN:OK detected.");
        return true;
    }
    /** */
    // 方案二：如果需要检测两个标志，可以这样实现：
    /** *
    uint32_t start_time = millis();
    bool found_cjoin_ok = false;
    bool found_joined = false;
    
    while (millis() - start_time < timeout_ms)
    {
        if (!found_cjoin_ok && waitForStringInUART("+CJOIN:OK", 1000))
        {
            found_cjoin_ok = true;
            Serial.println("[LoRaModule] ✓ Detected +CJOIN:OK");
        }
        
        if (!found_joined && waitForStringInUART("Joined", 1000))
        {
            found_joined = true;
            Serial.println("[LoRaModule] ✓ Detected Joined");
        }
        
        if (found_cjoin_ok && found_joined)
        {
            Serial.println("[LoRaModule] Join successful! Both indicators detected.");
            return true;
        }
    }
    /** */
    
    Serial.println("[LoRaModule] Join timeout or failed.");
    return false;
}

/**
 * @brief 执行可靠的网络入网流程，包含重试机制
 * @param useMulticast 是否使用多播模式
 * @param devAddr 多播设备地址（仅多播模式需要）
 * @param appSKey 多播AppSKey（仅多播模式需要）
 * @param nwkSKey 多播NwkSKey（仅多播模式需要）
 * @param maxRetries 最大重试次数
 * @return true表示入网成功，false表示失败
 */
static bool performReliableJoin(bool useMulticast, const String &devAddr, 
                                const String &appSKey, const String &nwkSKey, 
                                int maxRetries)
{
    int attempt = 0;
    
    while (attempt < maxRetries)
    {
        attempt++;
        Serial.println("[LoRaModule] ======================================");
        Serial.print("[LoRaModule] Join attempt ");
        Serial.print(attempt);
        Serial.print(" of ");
        Serial.println(maxRetries);
        Serial.println("[LoRaModule] ======================================");
        
        // 清空UART缓冲区
        uart_flush(UART_NUM_1);
        
        if (useMulticast)
        {
            // 多播模式：先离网，配置多播，再入网
            Serial.println("[LoRaModule] Configuring multicast...");
            joinNetwork_IDF(0);  // 离网
            delay(500);
            
            // 发送多播配置命令
            String command = "AT+CADDMUTICAST=";
            command += devAddr;
            command += ",";
            command += appSKey;
            command += ",";
            command += nwkSKey;
            command += "\n";
            uart_write_bytes(UART_NUM_1, command.c_str(), command.length());
            delay(500);
        }
        
        // 发起入网
        Serial.println("[LoRaModule] Sending join command...");
        joinNetwork_IDF(1);
        
        // 等待入网成功（超时30秒）
        if (waitForJoinSuccess(10000))
        {
            Serial.println("[LoRaModule] Network join successful!");
            // 发送测试数据
            /* *
            delay(2000);
            sendData("1");
            delay(1000);
            /* */
            return true;
        }
        
        // 入网失败，如果还有重试次数，继续重试
        if (attempt < maxRetries)
        {
            Serial.println("[LoRaModule] Join failed, retrying...");
            delay(2000);  // 重试前等待2秒
        }
    }
    
    Serial.println("[LoRaModule] All join attempts failed!");
    return false;
}

void LoRa_init()
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
    
    String savedDevAddr, savedAppSKey, savedNwkSKey;
    bool useMulticast = false;
    
    // 尝试从NVS加载组播信息
    if (NVS_loadLoRaMulticast(savedDevAddr, savedAppSKey, savedNwkSKey))
    {
        Serial.println("[LoRaModule] Successfully loaded multicast config from NVS.");
        useMulticast = true;
    }
    else
    {
        Serial.println("[LoRaModule] No saved multicast config found in NVS.");
        Serial.println("[LoRaModule] Will use standard join mode.");
    }
    
    // 执行可靠入网，最多重试5次
    const int MAX_RETRIES = 5;
    bool joinSuccess = performReliableJoin(useMulticast, savedDevAddr, savedAppSKey, savedNwkSKey, MAX_RETRIES);
    
    if (joinSuccess)
    {
        Serial.println("[LoRaModule] LoRa initialization complete - Network joined successfully!");
    }
    else
    {
        Serial.println("[LoRaModule] LoRa initialization failed - Could not join network!");
        Serial.println("[LoRaModule] Please check LoRa module and network configuration.");
    }
}

void joinNetwork_IDF(bool joinMode)
{
    // 构建AT指令 - 加入网络
    String command = "AT+CJOIN=";
    command += String(joinMode ? 1 : 0);  // 根据布尔值设置第一个参数
    command += ",0,8,2";  // 固定参数
    command += "\n";  // 添加换行符

    // 使用ESP-IDF UART API发送数据
    uart_write_bytes(UART_NUM_1, command.c_str(), command.length());
}

void addMuticast_IDF(const String &DevAddr, const String &AppSKey, const String &NwkSKey)
{
    // 此函数已被重构，现在通过 performReliableJoin 统一处理
    // 为保持兼容性，提供一个简化的包装函数
    Serial.println("[LoRaModule] addMuticast_IDF called - using reliable join mechanism");
    performReliableJoin(true, DevAddr, AppSKey, NwkSKey, 5);
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
    command += "\n";  // 添加换行符

    // 使用ESP-IDF UART API发送数据
    uart_write_bytes(UART_NUM_1, command.c_str(), command.length());
}

void receiveData()
{
    Serial.println("[LoRaModule] Receive.");
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
                    /**/
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
