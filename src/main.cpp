#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
// #include "LedModule.h"
// #include "AccelerometerModule.h"
// #include "ButtonModule.h"
// #include "LoRaModule.h"
// #include "WatchdogModule.h"
#include "RadarModule.h"

void setup() {
    Serial.begin(115200);
    // Serial.println("系统初始化");

    // Led_init();
//    Button_init();
//    Acc_init();
//    LoRa_init();
    Radar_init();
/** *
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
/**/

    /**
    Watchdog_init();  // 初始化看门狗

    // 创建看门狗任务（优先级设置为最高）
    xTaskCreatePinnedToCore(
        watchdogTask,         // 任务函数
        "WatchdogTask",       // 任务名称
        4096,                 // 堆栈大小
        NULL,                 // 任务参数
        configMAX_PRIORITIES - 1,  // 最高优先级
        &WatchdogTaskHandle,  // 任务句柄
        1                     // 运行核心 (1 = 核心1)
    );

    /* 创建看门狗测试任务 *
    xTaskCreatePinnedToCore(
        wdtTestTask,          // 任务函数
        "WdtTestTask",        // 任务名称
        4096,                 // 堆栈大小
        NULL,                 // 任务参数
        1,                    // 任务优先级
        &WdtTestTaskHandle,   // 任务句柄
        1                     // 运行核心 (1 = 核心1)
    );

/** *
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

/** *
/** */
  // 创建激光测距任务
    xTaskCreatePinnedToCore(
        radarTask,           // 任务函数
        "RadarTask",         // 任务名称
        4096,               // 堆栈大小
        NULL,                // 任务参数
        1,                   // 任务优先级
        &radarTaskHandle,    // 任务句柄
        1                    // 运行核心 (1 = 核心1)
    );
/** *
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
/** *
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
/** *
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
