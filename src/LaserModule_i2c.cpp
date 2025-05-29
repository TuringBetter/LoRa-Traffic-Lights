#include "LaserModule_i2c.h"
#include <Wire.h>
#include "LedModule.h"
#include "LoRaModule.h"

// 定义I2C引脚
static const uint64_t   LASER_I2C_SDA_PIN = 8; // I2C数据引脚
static const uint64_t   LASER_I2C_SCL_PIN = 7; // I2C时钟引脚
static const int        VEHICLE_DETECTION_THRESHOLD = 500;    // 车辆检测阈值（厘米）
static const uint32_t   VEHICLE_TIMEOUT             = 500;  // 车辆超时时间（毫秒）

static bool             _vehicleDetected; // 车辆检测状态
static uint32_t         _lastVehicleDetectionTime; // 上次检测时间

extern SemaphoreHandle_t   _ledStateMutex; // LED状态互斥量
extern bool                _ledStateChanged; // LED状态变化标志
extern volatile LedState            ledstate; // LED状态

TaskHandle_t laserTaskHandle   = NULL; // 激光任务句柄

// 函数声明
static void processLaserData(int16_t distance); // 处理激光数据
static int16_t readDistance(); // 读取距离

// 初始化激光模块
void Laser_I2C_init()
{
    Wire.begin(LASER_I2C_SDA_PIN, LASER_I2C_SCL_PIN); // 初始化I2C，ESP32常用引脚为21(SDA),22(SCL)
    // Serial.println("init i2c pin...");
    delay(100); // 等待初始化完成    
}

// 启动激光测量
void LaserStart()
{
    Wire.beginTransmission(I2C_ADDR); // 开始I2C传输
    Wire.write(CMD_REG); // 写入命令寄存器地址
    Wire.write(0x01); // 启动测量
    // Serial.println("send start...");
    if (Wire.endTransmission() != 0) 
    {
        Serial.println("Start command failed"); // 启动命令失败
        delay(1000);
        return;
    }
    delay(100); // 等待测量完成（根据模块需求调整）
}

// 读取距离
int16_t readDistance()
{
    Wire.beginTransmission(I2C_ADDR); // 开始I2C传输
    Wire.write(DIST_HIGH_REG); // 设置读取起始地址
    if (Wire.endTransmission(false) != 0) 
    { // 保持I2C连接
        Serial.println("Set register failed"); // 设置寄存器失败
        return -1;
    }

    // 请求读取2字节数据
    uint8_t bytesRead = Wire.requestFrom(I2C_ADDR, 2);
    if (bytesRead != 2) 
    {
        Serial.println("Incomplete data"); // 数据不完整
        return -1;
    }
    uint16_t distance = (Wire.read() << 8) | Wire.read(); // 读取距离数据
    return distance; // 返回距离
}

// 激光测距任务
void laserTask(void *pvParameters) 
{
    /** *
    // 初始化激光模块
    // Serial.println("Laser init...");
    laser.begin(); // 初始化激光模块
    laser.sendReadCommand(); // 发送读取命令
    /** */
    
    // 任务主循环
    while (1) {
        int16_t distance = readDistance(); // 读取距离
        /* *
        Serial.print("Distance: ");
        Serial.print(distance);
        Serial.println(" mm");
        /* */
        if (distance != -1) 
        {
            // Serial.println(distance); 
            processLaserData(distance); // 处理激光数据
        }
        
        // 添加一个小延时，避免过于频繁的读取
        vTaskDelay(pdMS_TO_TICKS(10)); // 延时10毫秒
    }
}

// 处理激光数据
static void processLaserData(int16_t distance)
{
    static LedColor _last_color; // 上一个LED颜色
    static uint16_t _last_frequency; // 上一个LED频率
    static uint16_t _last_brightness; // 上一个LED亮度

    // 检查是否有车辆接近
    if (distance < VEHICLE_DETECTION_THRESHOLD) 
    {
        if (!_vehicleDetected) 
        {
            _vehicleDetected = true; // 标记为检测到车辆
            _lastVehicleDetectionTime = millis(); // 记录检测时间
            
            // 发送车辆接近消息
            // sendLoRaMessage(MessageType::VEHICLE_APPROACHING, _lightId + 1);
            // sendLoRaMessage(MessageType::VEHICLE_APPROACHING, _lightId + 2);
            sendData("05"); // 发送数据
            // 灯亮
            // Serial.println("vehicle detected!");
            if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) 
            {
                // 保存历史状态
                _last_color = ledstate.color;
                _last_frequency = ledstate.frequency;
                _last_brightness = ledstate.brightness;

                // 更新灯状态
                /**/
                ledstate.color=LedColor::RED; // 设置LED颜色为红色
                ledstate.frequency=60; // 设置LED频率
                ledstate.brightness=7000; // 设置LED亮度

                _ledStateChanged=true; // 标记LED状态已改变
                /**/
                xSemaphoreGive(_ledStateMutex); // 释放互斥量
            }
        }
    } 
    else 
    {
        // 检查车辆是否已经通过
        if (_vehicleDetected && (millis() - _lastVehicleDetectionTime > VEHICLE_TIMEOUT)) {
            _vehicleDetected = false; // 标记为未检测到车辆
            
            // 发送车辆已通过消息
            // sendLoRaMessage(MessageType::VEHICLE_PASSED, _lightId + 1);
            // sendLoRaMessage(MessageType::VEHICLE_PASSED, _lightId + 2);
            
            // 更新LED状态
            // Serial.println("vehicle left");
            if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) 
            {
                // 恢复为历史状态
                ledstate.color = _last_color; // 恢复LED颜色
                ledstate.frequency = _last_frequency; // 恢复LED频率
                ledstate.brightness = _last_brightness; // 恢复LED亮度

                _ledStateChanged=true; // 标记LED状态已改变
                /**/
                xSemaphoreGive(_ledStateMutex); // 释放互斥量
            }
        }
    }
}