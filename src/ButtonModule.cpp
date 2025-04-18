#include "ButtonModule.h"

volatile static bool           buttonPressed   = false;
volatile static unsigned long  buttonPressTime = 0;  // 按键按下的时间戳
static const int               BUTTON_PIN      = 48;
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
    while(true) {
        if(buttonPressed) {
            // 检查是否超过消抖时间
            if(millis() - buttonPressTime >= DEBOUNCE_TIME) {
                // 上传云端报警
                Serial.println("pressed");
                // lora.sendData(LoRa::SendMode::UNCONFIRMED,1,"07");
                buttonPressed = false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms延时
    }
}

// 按键中断处理函数
void IRAM_ATTR buttonISR() {
    buttonPressTime = millis();
    buttonPressed = true;
}