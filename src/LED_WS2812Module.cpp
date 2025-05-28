#include "LED_WS2812Module.h"

const int NUM_LEDS = 576; // LED数量
const int DATA_PIN = 5;    // 选择你的GPIO引脚

extern TaskHandle_t LED_WS2812_TaskHandle;

static Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

static void setColor(uint32_t color);
static void setBright(uint32_t brightness);
static void clearStrip();

void LED_WS2812_init()
{
    strip.begin();
}

void LED_WS2812_Task(void *pvParameters)
{
}

void setColor(uint32_t color) {
    for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}

void setBright(uint32_t brightness) {
    for (int i = 0; i < strip.numPixels(); i++) {
        strip.setBrightness(brightness);
    }
    strip.show();
}


void clearStrip() {
    for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0)); // 关闭LED
    }
    strip.show();
}