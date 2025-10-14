/**
  ******************************************************************************
  * @file    LoRaModule.h
  * @author  陕西交通电子工程科技有限公司
  * @version V1.0.0
  * @date    2025-07-10
  * @brief   LoRa通信模块底层驱动。
  * @details
  * > 模块职责:
  * 本模块负责与LoRa硬件模组进行底层的串口(UART)通信，通过发送AT指令
  * 来控制LoRa网络操作。
  *
  * > 主要功能:
  * - 初始化与LoRa模组的UART通信。
  * - 封装设备入网、数据发送/接收等核心AT指令。
  * - 提供`loraReceiveTask`任务，在后台持续监听并解析来自LoRa模组的原始数据。
  * - 支持从NVS加载配置并加入多播组。
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

void LoRa_init();
void sendData(const String& payload);

void LoRa_init_IDF();

void joinNetwork_IDF(bool joinMode);
void addMuticast_IDF(const String& DevAddr, const String& AppSKey, const String& NwkSKey);
