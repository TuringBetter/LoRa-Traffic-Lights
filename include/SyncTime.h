#pragma once

#include <Arduino.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// 时间结构体，用于getTime()的直观可视化输出
typedef struct {
    uint8_t  hours;
    uint8_t  minutes;
    uint8_t  seconds;
    uint16_t milliseconds;
} Time_t;

//取当前时间，以结构体形式返回小时、分钟、秒和毫秒。
Time_t getCurrentTime();

//获取当前秒数，以秒为单位（向下取整）
uint32_t getTime_s();

//获取当前毫秒数，以毫秒为单位（向下取整）。
uint32_t getTime_ms();

//获取当前微秒数，以微秒为单位（向下取整）。
//uint64_t getTime_us();

// 计算时间差，避免跨日时间回绕问题
// currentTime必须是在总时间轴上晚于/将来于/大于lastTime的时间，最好是用当前时间与过去时间计算
uint32_t getSafeTimeDiff_ms(uint32_t currentTime, uint32_t lastTime);

//测试函数
void SyncTime_Test_Task(void *pvParameters);