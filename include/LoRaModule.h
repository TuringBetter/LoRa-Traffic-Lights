/**
  ******************************************************************************
  * @file    LoRaModule.h
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
  * > V2.0.0 更新说明:
  * - 实现基于日志的可靠入网检测（检测"+CJOIN:OK"和"Joined"标志）
  * - 添加入网失败自动重试机制（默认最多5次）
  * - 优化代码结构，消除冗余代码
  * - 改进错误处理和日志输出
  *
  ******************************************************************************
  */

#pragma once
#include <Arduino.h>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>



extern TaskHandle_t loraReceiveTaskHandle;
extern TaskHandle_t heartBeatTaskHandle;  // 心跳任务句柄


void loraReceiveTask(void* pvParameters);
void heartBeatTask(void* pvParameters);  // 心跳任务函数

void sendData(const String& payload);

void LoRa_init();

void joinNetwork_IDF(bool joinMode);
void addMuticast_IDF(const String& DevAddr, const String& AppSKey, const String& NwkSKey);
