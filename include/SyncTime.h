#ifndef SYNCTIME_H
#define SYNCTIME_H

#include <Arduino.h>
#include <esp_timer.h>

// 时间结构体，用于getTime()的直观可视化输出
typedef struct {
    uint8_t  hours;
    uint8_t  minutes;
    uint8_t  seconds;
    uint16_t milliseconds;
} SyncTime_Visual_t;

//取当前时间，以结构体形式返回小时、分钟、秒和毫秒。
SyncTime_Visual_t getTime();

//获取当前秒数，以秒为单位（向下取整）
uint64_t getTime_s();

//获取当前毫秒数，以毫秒为单位（向下取整）。
uint64_t getTime_ms();

//获取当前微秒数，以微秒为单位（向下取整）。
uint64_t getTime_us();

//测试函数
void SyncTime_Test_Task(void *pvParameters);

#endif // SYNCTIME_H