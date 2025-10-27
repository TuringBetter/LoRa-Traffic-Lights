/**
  ******************************************************************************
  * @file    main.cpp
  * @author  陕西交通电子工程科技有限公司
  * @version V1.0.0
  * @date    2025-08-02
  * @brief   主应用程序入口文件。基于FreeRTOS，负责系统初始化与创建所有核心任务。
  * @details
  * > 系统概述: 
  *   本程序为基于ESP32的多功能集成系统，集成了LoRa通信、传感器数据采集、人机交互
  *   及时间同步等功能模块。
  * 
  * > 硬件依赖:
  *   - ESP32开发板
  *   - LoRa通信模块（如SX1276/SX1262）
  *   - LED灯环（WS2812）
  *   - 加速度计传感器
  *   - 毫米波雷达传感器
  * 
  * > 任务核心分配:
  *   - 核心0 (Core 0): 运行通信及后台任务（LoRa接收、延迟测量、时间同步测试）
  *   - 核心1 (Core 1): 运行实时性要求高的感知与控制任务（按键、雷达、加速度计、LED、心跳）
  * 
  ******************************************************************************
  */


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
#include "NVSManager.h"



void setup() {
    Serial.begin(115200);
/** */
    //NVS_init();
    //LoRa_init();
    //Button_init();
    //Acc_init();
    //Radar_init();
    LED_WS2812_init();
    SyncTime_init();

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
/** *
  // 创建LoRa任务
    xTaskCreatePinnedToCore(
        loraReceiveTask,           // 任务函数
        "LoraReceiveTask",         // 任务名称
        4096,                   // 堆栈大小
        NULL,                   // 任务参数
        1,                      // 任务优先级
        &loraReceiveTaskHandle,    // 任务句柄
        0                       // 运行核心 (1 = 核心1)
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
/** *
  // 创建延迟测量任务
    xTaskCreatePinnedToCore(
        latencyTask,           // 任务函数
        "LatencyTask",         // 任务名称
        4096,                  // 堆栈大小
        NULL,                  // 任务参数
        1,                     // 任务优先级
        &latencyTaskHandle,    // 任务句柄
        0                      // 运行核心 (1 = 核心1)
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
/** */
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
    // vTaskDelay(pdMS_TO_TICKS(15000));
    // 删除setup任务，因为不再需要
    vTaskDelete(NULL);
/** */

}

void loop() {

}