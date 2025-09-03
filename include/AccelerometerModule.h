/**
  ******************************************************************************
  * @file    AccelerometerModule.h
  * @author  陕西交通电子工程科技有限公司
  * @version V1.0.0
  * @date    2025-07-19
  * @brief   三轴加速度计模块，主要用于碰撞检测和姿态检测。
  * @details
  * > 模块职责:
  * 通过I2C接口与加速度计传感器通信，实时监测设备的三轴加速度数据。
  *
  * > 主要功能:
  * - **碰撞检测**: `accelerometerTask`任务计算加速度矢量模长，若超过预设的
  * `COLLISION_THRESHOLD`阈值，则认为设备发生碰撞，并立即通过LoRa上报
  * 碰撞警报。
  * - **状态监控**: `accMonitorTask`是一个可选任务，可通过LoRa指令启动。它会
  * 周期性地上报一段时间内的最大加速度值，用于远程设备诊断和状态分析。
  *
  ******************************************************************************
  */

#pragma once
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t AccTaskHandle;
extern TaskHandle_t AccMonitorTaskHandle;

enum Range {
    RANGE_2G = 0x00,
    RANGE_4G = 0x01,
    RANGE_8G = 0x02,
    RANGE_16G = 0x03
};

void Acc_init();

void accelerometerTask(void* pvParameters);

void accMonitorTask(void* pvParameters);