/**
  ******************************************************************************
  * @file    RadarModule.h
  * @author  陕西交通电子工程科技有限公司
  * @version V1.0.0
  * @date    2025-07-17
  * @brief   毫米波雷达感应模块，用于检测车辆等移动目标。
  * @details
  * > 模块职责:
  * 判断毫米波雷达是否探测到目标。
  *
  * > 核心作用:
  * 为`LED_WS2812Module`提供车辆检测状态，用于触发高优先级的红色警示灯效。
  *
  ******************************************************************************
  */
 
#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

extern TaskHandle_t         radarTaskHandle;
extern SemaphoreHandle_t    radarStateMutex; // 确保这个是 extern

void radarTask(void *pvParameters);

void Radar_init();

// 新增：雷达模块是否已启用标志
extern bool radarModuleEnabled;

// 判断雷达是否激活或处于延长闪烁状态的函数
extern bool Radar_IsActiveOrExtending();

// 雷达开关控制函数
void Radar_Disable();
void Radar_Enable();