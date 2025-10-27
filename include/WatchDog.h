/**
  ******************************************************************************
  * @file    WatchDog.h
  * @author  陕西交通电子工程科技有限公司
  * @version V1.0.0
  * @date    2025-01-02
  * @brief   看门狗模块，用于监控系统关键任务的运行状态
  * @details
  * > 功能概述:
  * 监控LED_WS2812_Task和LED_StatusChange_Task的运行状态，确保系统正常运行
  * 如果检测到任务异常，会触发相应的错误处理或系统重启
  * 
  * > 工作原理:
  * - 基于心跳机制：被监控任务需要定期调用WatchDog_feed()进行喂狗
  * - 超时检测：如果任务超过5秒未喂狗，判定为任务异常
  * - 启动保护：系统启动后8秒内不检测超时，给任务足够的启动时间
  * - 累积计数：连续3次检查失败后触发系统重启
  * - 优先级保障：看门狗任务运行在优先级2，确保监控不受其他任务影响
  * - 线程安全：使用互斥锁保护心跳数据结构，避免竞态条件
  ******************************************************************************
  */

#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// 看门狗配置参数
#define WATCHDOG_CHECK_INTERVAL_MS     1000    // 检查间隔(ms)
#define WATCHDOG_TIMEOUT_MS            5000    // 心跳超时时间(ms)
#define WATCHDOG_MAX_FAIL_COUNT        3       // 最大失败次数
#define WATCHDOG_STARTUP_GRACE_MS      8000    // 启动宽限时间(ms)，给任务足够时间完成首次喂狗

extern TaskHandle_t WatchDogTaskHandle;

// 喂狗函数 - 被监控的任务需要定期调用此函数
void WatchDog_feed(const char* taskName);

// 被监控的任务句柄（在LED模块中声明）
extern TaskHandle_t LED_WS2812_TaskHandle;
extern TaskHandle_t LED_StatusChange_TaskHandle;

void WatchDog_init();
void watchdogTask(void* pvParameters);