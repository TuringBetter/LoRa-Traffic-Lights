#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "AccelerometerModule.h"
#include "ButtonModule.h"
#include "LoRaModule.h"
#include "LED_WS2812Module.h"
#include "RadarModule.h"
#include "LoRaLantency.h"
#include "SyncTime.h"



void setup() {
    Serial.begin(115200);
/** */
    LoRa_init_IDF();
    // Button_init();
    // Acc_init();
    // Radar_init();
    LED_WS2812_init();

/** *
    // 创建按键检测任务
    xTaskCreatePinnedToCore(
        buttonTask,        // 任务函数
        "ButtonTask",      // 任务名称
        4096,              // 堆栈大小
        NULL,              // 任务参数
        1,                 // 任务优先级
        &ButtonTaskHandle, // 任务句柄
        1                  // 运行核心 (1 = 核心1)
    );

/** *
  // 创建雷达检测任务
    xTaskCreatePinnedToCore(
        radarTask,           // 任务函数
        "RadarTask",         // 任务名称
        4096,                // 堆栈大小
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
/** */
  // 创建LoRa任务
    xTaskCreatePinnedToCore(
        loraReceiveTask,           // 任务函数
        "LoraReceiveTask",         // 任务名称
        4096,                   // 堆栈大小
        NULL,                   // 任务参数
        1,                      // 任务优先级
        &loraReceiveTaskHandle,    // 任务句柄
        1                       // 运行核心 (1 = 核心1)
    );

/** *
    // 创建心跳任务
    xTaskCreatePinnedToCore(
        heartBeatTask,           // 任务函数
        "HeartBeatTask",         // 任务名称
        4096,                   // 堆栈大小
        NULL,                   // 任务参数
        1,                      // 任务优先级
        &heartBeatTaskHandle,    // 任务句柄
        1                       // 运行核心 (1 = 核心1)
    );
/** *
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
    // 创建LED控制任务
    xTaskCreatePinnedToCore(
        LED_WS2812_Task,          // 任务函数
        "LED_WS2812_Task",        // 任务名称
        4096,                     // 堆栈大小
        NULL,                     // 任务参数
        1,                        // 任务优先级
        &LED_WS2812_TaskHandle,   // 任务句柄
        1                         // 运行核心 (1 = 核心1)
    );  
/** *
    // 创建LED测试任务
    xTaskCreatePinnedToCore(
        LED_Test_Task,            // 任务函数
        "LED_Test_Task",          // 任务名称
        4096,                     // 堆栈大小
        NULL,                     // 任务参数
        1,                        // 任务优先级
        &LED_Test_TaskHandle,     // 任务句柄
        1                         // 运行核心 (1 = 核心1)
    );
/** *
    // 创建 SyncTime 测试任务
    xTaskCreatePinnedToCore(
        SyncTime_Test_Task,      // 任务函数
        "SyncTime_Test",         // 任务名称
        4096,                    // 堆栈大小（根据需要调整）
        NULL,                    // 任务参数
        1,                       // 任务优先级 (可以根据需要调整)
        &SyncTime_Test_TaskHandle, // 任务句柄
        0                        // 运行核心 (0 = 核心0，为了负载均衡可以放到另一个核心)
    );
/** */
  // 删除setup任务，因为不再需要
    vTaskDelete(NULL);
/** */

}

void loop() {

}

// put function definitions here:
