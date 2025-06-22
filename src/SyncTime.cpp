#include "SyncTime.h"
#include "LoRaHandler.h"
#include "LoRaLantency.h"

// 定义一天总共有多少毫秒
// 1天 = 24小时 * 60分钟/小时 * 60秒/分钟 * 1000毫秒/秒 = 86,400,000 毫秒
#define MS_PER_DAY (24ULL * 60 * 60 * 1000)

// 定义一天总共有多少微秒
// 1天 = 24小时 * 60分钟/小时 * 60秒/分钟 * 1000毫秒/秒 * 1000微秒/毫秒 = 86,400,000,000 微秒
#define US_PER_DAY (MS_PER_DAY * 1000ULL)

/*
* 如需使用系统内部时钟，使用来自esp_timer的esp_timer_get_time()获取自系统启动以来的微秒数
* 如需使用外部校准时钟，使用来自LoRaHandler的getLoraMs()获取当日的现实时间，加上来自LoraLantency的getLantency()消除误差
*/


SyncTime_Visual_t getTime() {
    SyncTime_Visual_t visualTime;

    // 获取自系统启动以来的微秒数
    uint32_t current_millis = getLoraMs() + getLantency();

    // 将微秒转换为毫秒，并对一天的毫秒数取模，实现循环
    uint32_t total_ms_in_day = current_millis % MS_PER_DAY;

    // 从总毫秒数中提取时间分量
    visualTime.milliseconds = total_ms_in_day % 1000;
    uint32_t total_seconds_in_day = total_ms_in_day / 1000;

    visualTime.seconds = total_seconds_in_day % 60;
    uint32_t total_minutes_in_day = total_seconds_in_day / 60;

    visualTime.minutes = total_minutes_in_day % 60;
    visualTime.hours = total_minutes_in_day / 60; // 剩余的就是小时

    return visualTime;
}

uint32_t getTime_s() {
    // 获取自系统启动以来的微秒数
    uint32_t current_millis = getLoraMs() + getLantency();
    // 将微秒转换为秒，并对一天的秒数取模，实现循环
    // 1天 = 24小时 * 60分钟/小时 * 60秒/分钟 = 86,400 秒
    return ((current_millis / 1000) % (24ULL * 60 * 60));
}

uint32_t getTime_ms() {
    // 获取自系统启动以来的微秒数
    uint64_t current_millis = getLoraMs() + getLantency();

    // 将微秒转换为毫秒，并对一天的毫秒数取模，实现循环
    return (current_millis % MS_PER_DAY);
}

/*微秒*
uint64_t getTime_us() {
    // 获取自系统启动以来的微秒数
    uint64_t current_micros = esp_timer_get_time();

    // 对一天的微秒数取模，实现循环
    return current_micros % US_PER_DAY;
}

/*测试*/
void SyncTime_Test_Task(void *pvParameters) {
    (void)pvParameters; // 避免未使用参数警告

    Serial.println("SyncTime_Test_Task started. Press Ctrl+C to stop.");

    while (1) {
        
        SyncTime_Visual_t visual_time = getTime();
        uint64_t seconds = getTime_s();
        uint64_t milliseconds = getTime_ms();
        // uint64_t microseconds = getTime_us();

        Serial.printf("[TEST] Visual: %02d:%02d:%02d.%03d | Seconds: %llu s| Milliseconds: %llu ms\n", // 修改这里
                      visual_time.hours, visual_time.minutes, visual_time.seconds, visual_time.milliseconds, // 
                      seconds, milliseconds);

        vTaskDelay(pdMS_TO_TICKS(random(1000, 10001)));
    }
}