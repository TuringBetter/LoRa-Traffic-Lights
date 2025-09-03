/**
  ******************************************************************************
  * @file    ButtonModule.h
  * @author  陕西交通电子工程科技有限公司
  * @version V1.0.0
  * @date    2025-04-10
  * @brief   物理按键处理模块，用于手动触发报警。
  * @details
  * > 模块职责:
  * 处理物理按键的按下事件。
  *
  ******************************************************************************
  */

#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t ButtonTaskHandle;

void Button_init();
void buttonTask(void* pvParameters);