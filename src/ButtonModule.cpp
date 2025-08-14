#include "ButtonModule.h"
#include "LoRaModule.h"

volatile static bool           buttonPressed   = false;
volatile static unsigned long  buttonPressTime = 0;  // 按键按下的时间戳
static const int               BUTTON_PIN      = 8;
static const unsigned long     DEBOUNCE_TIME   = 20;      // 消抖时间（毫秒）

TaskHandle_t ButtonTaskHandle  = NULL;

// 按键中断处理函数
void IRAM_ATTR buttonISR();

void Button_init()
{
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(BUTTON_PIN, buttonISR, FALLING);
}

void buttonTask(void *pvParameters)
{
    // UBaseType_t uxHighWaterMark;  // 堆栈监控变量
    while(true) {
        if(buttonPressed) {
            // 检查是否超过消抖时间
            if(millis() - buttonPressTime >= DEBOUNCE_TIME) 
            {
                // 上传云端报警
                // Serial.println("pressed");
                sendData("07");
                buttonPressed = false;
            }
        }
        // 堆栈使用量监控代码（已注释）
        /*
        static unsigned long lastCheckTime = 0;
        if (millis() - lastCheckTime >= 5000) {
            uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
            Serial.printf("ButtonTask Stack High Water Mark: %u bytes remaining\n", uxHighWaterMark);
            lastCheckTime = millis();
        }
        */
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms延时
    }
}

// 按键中断处理函数
void IRAM_ATTR buttonISR() {
    buttonPressTime = millis();
    buttonPressed = true;
}