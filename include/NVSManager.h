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