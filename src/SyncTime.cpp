#include "SyncTime.h"
#include "LoRaHandler.h" 
#include "LoRaLantency.h"
#include "LED_WS2812Module.h"

// 定义一天总共有多少毫秒
// 1天 = 24小时 * 60分钟/小时 * 60秒/分钟 * 1000毫秒/秒 = 86,400,000 毫秒
#define MS_PER_DAY (24ULL * 60 * 60 * 1000)

// 定义一天总共有多少微秒
// 1天 = 24小时 * 60分钟/小时 * 60秒/分钟 * 1000毫秒/秒 * 1000微秒/毫秒 = 86,400,000,000 微秒
#define US_PER_DAY (MS_PER_DAY * 1000ULL)

TaskHandle_t SyncTime_Test_TaskHandle = NULL;

// 定义互斥锁
SemaphoreHandle_t syncTimeMutex = NULL; // 初始化为NULL

/*
* 用于存储 esp_timer_get_time() 当前值与“当天”实际时间之间的同步偏移量（微秒）。
* 这个偏移量会相对较小，反映的是esp_timer相对于当天0点0分0秒的偏差。
* 初始值为0，表示刚上电时，认为esp_timer就是实际时间。
* 每次收到校准信息时更新。
*/
// 存储系统时间与真实时间的偏移量，使用 volatile 防止优化
static volatile int64_t g_current_day_offset_us = 0; 

// 记录上次同步时 LoRa 返回的原始 getRealTimeMs() 值，用于检测其是否更新
static uint32_t g_last_known_lora_real_ms = 0;

// 记录上次同步时 LoRa 返回的原始 getLantency() 值，用于检测其是否更新
static uint32_t g_last_known_lora_latency_ms = 0;

// SyncTime 模块的初始化函数
void SyncTime_init() {
    if (syncTimeMutex == NULL) { // 避免重复创建
        syncTimeMutex = xSemaphoreCreateMutex(); //
        if (syncTimeMutex == NULL) {
            Serial.println("[SyncTime] 错误：无法创建时间同步互斥锁！系统可能不稳定。"); //
            while(1) { vTaskDelay(pdMS_TO_TICKS(100)); } // 阻止系统继续运行
        }
        Serial.println("[SyncTime] 时间同步互斥锁创建成功。"); //
    }
}

// 外部调用以触发时间同步的函数
// 这个函数现在是唯一会根据 LoRa 数据更新 g_current_day_offset_us 的地方。
void triggerTimeSynchronization() {
    uint32_t current_lora_real_ms = getRealTimeMs(); // 从LoRaHandler获取时间
    uint32_t current_lora_latency_ms = getLantency(); // 从LoRaLantency获取延迟

    // 只有当 LoRa 真实时间或延迟数据发生变化时才进行校准
    if (current_lora_real_ms != g_last_known_lora_real_ms ||
        current_lora_latency_ms != g_last_known_lora_latency_ms)
    {
        // 计算LoRa的“当天”微秒数
        uint64_t lora_day_micros = (uint64_t)(current_lora_real_ms - current_lora_latency_ms) * 1000ULL;
        lora_day_micros %= US_PER_DAY; // 确保 LoRa 时间也在一天周期内

        // 获取当前esp_timer的“当天”微秒数
        uint64_t current_esp_timer_absolute_us = esp_timer_get_time();
        uint64_t current_esp_timer_day_micros = current_esp_timer_absolute_us % US_PER_DAY;

        // 计算当前esp_timer的“当天”微秒数与LoRa提供的“当天”微秒数之间的差
        int64_t calculated_offset = (int64_t)lora_day_micros - (int64_t)current_esp_timer_day_micros;

        // 处理跨越半天周期（或午夜附近）的差异：
        if (calculated_offset > US_PER_DAY / 2) {
            calculated_offset -= US_PER_DAY;
        } else if (calculated_offset < -US_PER_DAY / 2) {
            calculated_offset += US_PER_DAY;
        }

        // 使用互斥锁保护对 g_current_day_offset_us 的写入
        if (syncTimeMutex != NULL && xSemaphoreTake(syncTimeMutex, portMAX_DELAY) == pdTRUE) { //
            g_current_day_offset_us = calculated_offset; //
            xSemaphoreGive(syncTimeMutex); //
        } else {
            Serial.println("[SyncTime] 警告：triggerTimeSynchronization 无法获取互斥锁！"); //
        }

        // 更新上次已知的 LoRa 数据，避免重复校准
        g_last_known_lora_real_ms = current_lora_real_ms;
        g_last_known_lora_latency_ms = current_lora_latency_ms;
        Serial.printf("[Time Sync] Calibrated by LoRa data update! LoRa day micros: %llu us, ESP timer day micros: %llu us, New effective offset: %lld us\n",lora_day_micros, current_esp_timer_day_micros, g_current_day_offset_us);
    } else {
        // Serial.println("[Time Sync] LoRa data unchanged, no re-calibration needed."); // 可以根据需要打印
    }
}


// 获取当前校准后的微秒时间
// 这个函数现在只根据 g_current_day_offset_us 计算时间，不再触发 LoRa 数据的检查。
static uint64_t getCalibratedMicros() {
    // 获取当前 esp_timer 的微秒数（绝对值）
    uint64_t current_esp_timer_absolute_us = esp_timer_get_time();

    int64_t current_offset; //

    // 使用互斥锁保护对 g_current_day_offset_us 的读取
    if (syncTimeMutex != NULL && xSemaphoreTake(syncTimeMutex, portMAX_DELAY) == pdTRUE) { //
        current_offset = g_current_day_offset_us; //
        xSemaphoreGive(syncTimeMutex); //
    } else {
        Serial.println("[SyncTime] 警告：getCalibratedMicros 无法获取互斥锁！"); //
        current_offset = g_current_day_offset_us; // 在无法获取锁时，仍然尝试读取当前值，但这可能是竞态条件下的旧值或部分更新值
    }

    int64_t effective_day_micros = (int64_t)(current_esp_timer_absolute_us % US_PER_DAY) + current_offset; //


    // 确保结果在当天周期内（0 到 US_PER_DAY - 1）
    while (effective_day_micros < 0) {
        effective_day_micros += US_PER_DAY;
    }
    effective_day_micros %= US_PER_DAY;

    return (uint64_t)effective_day_micros;
}


// 计算毫秒时间差，避免跨日时间回绕问题，此函数用于外部调用
// 前者currentTime必须是在总时间轴上晚于/将来于/大于后者lastTime的时间，最好是用当前时间与过去时间计算
uint32_t getSafeTimeDiff_ms(uint32_t currentTime, uint32_t lastTime) {
    if (currentTime >= lastTime) {
        return currentTime - lastTime;
    } else {
        // 发生环绕
        return (MS_PER_DAY - lastTime) + currentTime; 
    }
}

Time_t getCurrentTime() {
    Time_t visualTime;

    // 获取校准后的微秒时间，这个值已经是当天内的微秒数
    uint64_t calibrated_micros_in_day = getCalibratedMicros();

    // 将微秒转换为毫秒，并从总毫秒数中提取时间分量
    uint64_t total_ms_in_day = calibrated_micros_in_day / 1000ULL;

    visualTime.milliseconds = total_ms_in_day % 1000;
    uint64_t total_seconds_in_day = total_ms_in_day / 1000;

    visualTime.seconds = total_seconds_in_day % 60;
    uint64_t total_minutes_in_day = total_seconds_in_day / 60;

    visualTime.minutes = total_minutes_in_day % 60;
    visualTime.hours = total_minutes_in_day / 60;

    return visualTime;
}

uint32_t getTime_s() {
    // 获取校准后的微秒时间，这个值已经是当天内的微秒数
    uint32_t calibrated_micros = getCalibratedMicros();
    // 直接将微秒转换为秒
    return (calibrated_micros / 1000 / 1000);
}

uint32_t getTime_ms() {
    // 获取校准后的微秒时间，这个值已经是当天内的微秒数
    uint32_t calibrated_micros = getCalibratedMicros();
    // 直接将微秒转换为毫秒
    return (calibrated_micros / 1000);
}

uint64_t getTime_us() {
    // 获取校准后的微秒时间，这个值已经是当天内的微秒数
    uint64_t calibrated_micros = getCalibratedMicros();
    return calibrated_micros;
}

void printTime(const String& msg){
    Time_t visual_time = getCurrentTime(); // 每次调用getTime()都会触发getCalibratedMicros()
    uint64_t seconds = getTime_s();
    uint64_t milliseconds = getTime_ms();
    uint64_t microseconds = getTime_us();

    Serial.printf("[%s] Visual: %02d:%02d:%02d.%03d | Seconds: %llu s| Milliseconds: %llu ms| Microseconds: %llu us\n",
                      msg.c_str(),
                      visual_time.hours, visual_time.minutes, visual_time.seconds, visual_time.milliseconds,
                      seconds, milliseconds, microseconds);
}

/*测试函数：循环打印当前时间到串口。
  每次打印间隔一个随机时间（1到10秒，毫秒级精度）。
  校准将由对 getRealTimeMs() 和 getLantency() 的值变化自动触发。
*/
void SyncTime_Test_Task(void *pvParameters) {
    (void)pvParameters; // 避免未使用参数警告

    randomSeed(esp_timer_get_time()); // 使用 esp_timer_get_time() 的低位作为随机数种子

    Serial.println("SyncTime_Test_Task started. Random delay between 1 to 10 seconds (millisecond precision).");
    Serial.println("Automatic time calibration will occur when getRealTimeMs() or getLantency() values change.");

    while (1) {
        printTime("SyncTime_Test");
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1-10秒随机延迟
    }
}

