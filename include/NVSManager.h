/**
  ******************************************************************************
  * @file    NVSManager.h
  * @author  陕西交通电子工程科技有限公司
  * @version V1.0.0
  * @date    2025-08-02
  * @brief   非易失性存储（NVS）管理模块。
  * @details
  * > 模块职责:
  * 提供对ESP32片上非易失性存储（NVS）的读写接口，用于实现设备配置的持久化。
  *
  * > 主要功能:
  * - 初始化NVS分区。
  * - **保存LoRa多播组信息**: 将`DevAddr`, `AppSKey`, `NwkSKey`等关键网络
  * 凭证写入Flash。
  * - **加载LoRa多播组信息**: 在设备启动时，从Flash中读取已保存的网络凭证，
  * 使得设备能够断电后自动重连，无需重新配置。
  *
  ******************************************************************************
  */

#pragma once
#include <Arduino.h>
#include <nvs_flash.h>
#include <nvs.h> 

// 始化NVS Flash分区，并检查NVS分区的完整性
void NVS_init();

// 保存 LoRa多播组的凭证到 NVS
bool NVS_saveLoRaMulticast(const String& devAddr, const String& appSKey, const String& nwkSKey);

// 从 NVS读取 LoRa多播组凭证
bool NVS_loadLoRaMulticast(String& devAddr, String& appSKey, String& nwkSKey);