#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "LedModule.h"
#include "LaserModule_i2c.h"
#include "AccelerometerModule.h"
#include "ButtonModule.h"
#include "LoRaModule.h"
// put function declarations here:

Led             led;

TaskHandle_t LedTaskHandle     = NULL;
TaskHandle_t LedTestTaskHandle = NULL;

void ledTask(void* pvParameters);
void ledTestTask(void* pvParameters);


// =========================辅助变量======================

// Led相关变量
LedState            _ledState{LedColor::YELLOW,500,0};
bool                _ledStateChanged{false};
SemaphoreHandle_t   _ledStateMutex;

/**/
void setup() {
    Serial.begin(115200);
    Serial.println("系统初始化");
/** */
    // 初始化led
    _ledStateChanged=false;
    _ledStateMutex = xSemaphoreCreateMutex();


    // 初始化lora
    // lora.begin();
/* */
    Button_init();
    Laser_I2C_init();
    LaserStart();
    Acc_init();
    LoRa_init();

/** */
    // 创建按键检测任务
    xTaskCreatePinnedToCore(
        buttonTask,        // 任务函数
        "ButtonTask",      // 任务名称
        4096,             // 堆栈大小
        NULL,             // 任务参数
        1,                // 任务优先级
        &ButtonTaskHandle,// 任务句柄
        1                 // 运行核心 (1 = 核心1)
    );

/** */
  // 创建激光测距任务
    xTaskCreatePinnedToCore(
        laserTask,           // 任务函数
        "LaserTask",         // 任务名称
        4096,               // 堆栈大小
        NULL,                // 任务参数
        1,                   // 任务优先级
        &laserTaskHandle,    // 任务句柄
        1                    // 运行核心 (1 = 核心1)
    );
/** */
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
/** *
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
/** *
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
/** */
  // 创建LoRa测试任务
    xTaskCreatePinnedToCore(
        loraTestTask,           // 任务函数
        "LoraTestTask",         // 任务名称
        4096,                   // 堆栈大小
        NULL,                   // 任务参数
        1,                      // 任务优先级
        &loraTestTaskHandle,    // 任务句柄
        1                       // 运行核心 (1 = 核心1)
    );
/** */
  // 创建延迟测量任务
    xTaskCreatePinnedToCore(
        latencyTask,           // 任务函数
        "LatencyTask",         // 任务名称
        4096,                  // 堆栈大小
        NULL,                  // 任务参数
        1,                     // 任务优先级
        &latencyTaskHandle,    // 任务句柄
        1                      // 运行核心 (1 = 核心1)
    );
/** */
  // 删除setup任务，因为不再需要
    vTaskDelete(NULL);
/** */
}

void loop() {

}

// put function definitions here:

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
