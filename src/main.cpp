#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "LedModule.h"
#include "LaserModule.h"
#include "AccelerometerModule.h"
// put function declarations here:

// 配置参数
static const int        VEHICLE_DETECTION_THRESHOLD = 500;    // 车辆检测阈值（厘米）
static const uint32_t   VEHICLE_TIMEOUT             = 500;  // 车辆超时时间（毫秒）

// 传感器模块
Laser laser;
Accelerometer accelerometer;
Led led;

// 任务句柄
TaskHandle_t laserTaskHandle = NULL;
TaskHandle_t AccTaskHandle = NULL;
TaskHandle_t LedTaskHandle = NULL;
TaskHandle_t LedTestTaskHandle = NULL;

// 任务函数
void laserTask(void *pvParameters);
void accelerometerTask(void* pvParameters);
void ledTask(void* pvParameters);
void ledTestTask(void* pvParameters);

// 辅助函数
void processLaserData(int16_t distance);

// 辅助变量
bool                _vehicleDetected;
uint32_t            _lastVehicleDetectionTime;

LedState            _ledState{LedColor::YELLOW,500,0};
bool                _ledStateChanged{false};
SemaphoreHandle_t   _ledStateMutex;


void setup() {
    Serial.begin(115200);
    Serial.println("系统初始化");

    _ledStateChanged=false;
    _ledStateMutex = xSemaphoreCreateMutex();

  // 创建激光测距任务
    xTaskCreatePinnedToCore(
        laserTask,           // 任务函数
        "LaserTask",         // 任务名称
        10000,               // 堆栈大小
        NULL,                // 任务参数
        1,                   // 任务优先级
        &laserTaskHandle,    // 任务句柄
        1                    // 运行核心 (1 = 核心1)
    );

  // 创建加速度计任务
    xTaskCreatePinnedToCore(
        accelerometerTask,   // 任务函数
        "AccelerometerTask", // 任务名称
        4096,                // 堆栈大小
        NULL,                // 任务参数
        1,                   // 任务优先级
        &AccTaskHandle,      // 任务句柄
        1                    // 运行核心 (1 = 核心1)
    );

/*
  // 创建灯光测试任务
    xTaskCreatePinnedToCore(
        ledTestTask,         // 任务函数
        "LedTestTask",       // 任务名称
        4096,                // 堆栈大小
        NULL,                // 任务参数
        1,                   // 任务优先级
        &LedTestTaskHandle,  // 任务句柄
        1                    // 运行核心 (1 = 核心1)
    );
*/
  // 创建灯光任务
    xTaskCreatePinnedToCore(
        ledTask,         // 任务函数
        "LedTask",       // 任务名称
        4096,            // 堆栈大小
        NULL,            // 任务参数
        1,               // 任务优先级
        &LedTaskHandle,  // 任务句柄
        1                // 运行核心 (1 = 核心1)
    );

  // 删除setup任务，因为不再需要
    vTaskDelete(NULL);
}

void loop() {

}

// put function definitions here:

// 激光测距任务
void laserTask(void *pvParameters) 
{
  // 初始化激光模块
    laser.begin();
    laser.sendReadCommand();
  // 任务主循环
    while (1) {
        int16_t distance = laser.receiveReadResponse();

        if (distance != -1) {
            // Serial.println(distance); 
            processLaserData(distance);
        }
        
        // 添加一个小延时，避免过于频繁的读取
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// 加速度计任务
void accelerometerTask(void* pvParameters)
{
    accelerometer.begin();
    while(true)
    {
        int16_t x, y, z;
        // 读取加速度计数据
        accelerometer.readRaw(x, y, z);
        
        // 处理加速度数据
        accelerometer.processDate(x, y, z);
        
        // 任务延时
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms
    }
}

void ledTask(void *pvParameters)
{
    led.begin();
    
    while(true)
    {
        if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) 
        {
            if(_ledStateChanged)
            {
                // 更新灯状态
                led.setState(_ledState);
                
                _ledStateChanged=false;
            }
            xSemaphoreGive(_ledStateMutex);
        }


        // 更新LED状态（实现闪烁效果）
        led.update();

        // 任务延时
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms
    }

}

void ledTestTask(void *pvParameters)
{
    led.begin();
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
                led.setColor(LedColor::RED);
                led.setBrightness(500);
                led.setFrequency(0);
                break;
                
            case 1:
                Serial.println("测试2: 红色LED,亮度2000,30Hz闪烁");
                led.setColor(LedColor::RED);
                led.setBrightness(2000);
                led.setFrequency(30);
                break;
                
            case 2:
                Serial.println("测试3: 红色LED,亮度7000,60Hz闪烁");
                led.setColor(LedColor::RED);
                led.setBrightness(7000);
                led.setFrequency(60);
                break;
                
            case 3:
                Serial.println("测试4: 黄色LED,亮度1000,不闪烁");
                led.setColor(LedColor::YELLOW);
                led.setBrightness(1000);
                led.setFrequency(0);
                break;
                
            case 4:
                Serial.println("测试5: 黄色LED,亮度4000,120Hz闪烁");
                led.setColor(LedColor::YELLOW);
                led.setBrightness(4000);
                led.setFrequency(120);
                break;
                
            case 5:
                Serial.println("测试6: 黄色LED,亮度2000,30Hz闪烁");
                led.setColor(LedColor::YELLOW);
                led.setBrightness(2000);
                led.setFrequency(30);
                break;
            }
        }
        
        // 更新LED状态（实现闪烁效果）
        led.update();
    }
}

void processLaserData(int16_t distance)
{
    // 检查是否有车辆接近
    if (distance < VEHICLE_DETECTION_THRESHOLD) 
    {
        if (!_vehicleDetected) 
        {
            _vehicleDetected = true;
            _lastVehicleDetectionTime = millis();
            
            // 发送车辆接近消息
            // sendLoRaMessage(MessageType::VEHICLE_APPROACHING, _lightId + 1);
            // sendLoRaMessage(MessageType::VEHICLE_APPROACHING, _lightId + 2);
            
            // 灯亮
            Serial.println("vehicle detected!");
            if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) 
            {
                // 更新灯状态
                _ledState.color=LedColor::RED;
                _ledState.frequency=60;
                _ledState.brightness=7000;

                _ledStateChanged=true;

                xSemaphoreGive(_ledStateMutex);
            }
        }
    } 
    else 
    {
        // 检查车辆是否已经通过
        if (_vehicleDetected && (millis() - _lastVehicleDetectionTime > VEHICLE_TIMEOUT)) {
            _vehicleDetected = false;
            
            // 发送车辆已通过消息
            // sendLoRaMessage(MessageType::VEHICLE_PASSED, _lightId + 1);
            // sendLoRaMessage(MessageType::VEHICLE_PASSED, _lightId + 2);
            
            // 更新LED状态
            Serial.println("vehicle left");
            if (xSemaphoreTake(_ledStateMutex, portMAX_DELAY) == pdTRUE) 
            {
                // 更新灯状态
                _ledState.color=LedColor::YELLOW;
                _ledState.frequency=0;
                _ledState.brightness=1000;

                _ledStateChanged=true;
                
                xSemaphoreGive(_ledStateMutex);
            }
        }
    }
}
