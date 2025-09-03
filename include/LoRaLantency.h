/**
  ******************************************************************************
  * @file    LoRaLantency.h
  * @author  陕西交通电子工程科技有限公司
  * @version V1.0.0
  * @date    2025-07-25
  * @brief   LoRa通信延迟测量模块。
  * @details
  * > 模块职责:
  * 本模块用于测量设备与LoRa网络服务器之间的通信延迟。
  *
  * > 工作流程:
  * - `latencyTask`任务会周期性地向服务器发送一个特定的探测包。
  * - 当服务器回应此探测包时，`LoRaHandler`会调用本模块的`CalcLantency`函数。
  * - `CalcLantency`函数通过记录的发送时间和接收时间，计算出通信往返时延的
  * 一半，作为单向延迟的估算值。
  *
  * > 数据用途:
  * 计算出的延迟值是`SyncTime`模块实现高精度时间同步的关键补偿参数。
  *
  ******************************************************************************
  */

#pragma once
#include <Arduino.h>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t latencyTaskHandle;  // 延迟测量任务句柄

void latencyTask(void* pvParameters);  // 延迟测量任务函数

uint32_t getDelay();

void CalcLantency();

uint32_t getLantency();