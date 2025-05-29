#include <Arduino.h>
<<<<<<< HEAD
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#include "LoRaModule.h"
<<<<<<< Updated upstream
#include "LedModule.h"
#include "LaserModule.h"
=======
#include "WatchdogModule.h"
#include "WdtTestModule.h"
>>>>>>> Stashed changes
// put function declarations here:
void printEspId();


/**定义引脚**/
#define TFT_RST   15  // 复位RST
#define TFT_DC    16  // 数据/命令D/C
#define TFT_MOSI  17  // SPI 数据SDA
#define TFT_SCK   18  // SPI 时钟SCL
#define TFT_CS    21   // 片选CS


// **初始化屏幕**
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
// LoRaModule lora;
// Led led;
Laser laser(Serial1);
=======
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "LedModule.h"
#include "LaserModule_i2c.h"
#include "AccelerometerModule.h"
#include "ButtonModule.h"
#include "LoRaModule.h"
// put function declarations here:
>>>>>>> xr-FreeRTOS

void setup() {
    Serial.begin(115200);
    Serial.println("系统初始化");

<<<<<<< HEAD
<<<<<<< Updated upstream
  // lora.begin(); // 初始化LoRa模块并读取启动信息
  // led.begin(4);
  // **初始化 SPI**
    SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);

    // **初始化 ST7735S**
    tft.initR(INITR_BLACKTAB); // 选择 ST7735S 类型
    tft.setRotation(1); // 旋转方向

    // **填充屏幕**
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
  /*
  // 测试设置本地地址
  int localAddress = 12; // 要设置的本地地址
  // int targetAddress = 13;
  lora.setLocalAddress(localAddress);
  // lora.setTargetAddress(targetAddress);
  // 测试设置接收配置
  LoRaTransConfigStruct rxConfig;
  rxConfig.freq = 470500000; // 设置频率
  rxConfig.dataRate = SF7; // 设置数据速率
  rxConfig.bandwidth = BW_125KHz; // 设置带宽
  rxConfig.codeRate = CR_4_5; // 设置编码率
  rxConfig.iqConverted = IQ_ON;

  // 测试设置发送配置
  LoRaTransConfigStruct txConfig;
  txConfig.freq = 470500000; // 设置频率
  txConfig.dataRate = SF7; // 设置数据速率
  txConfig.bandwidth = BW_125KHz; // 设置带宽
  txConfig.codeRate = CR_4_5; // 设置编码率
  txConfig.power = 21;
  txConfig.iqConverted = IQ_ON;

  lora.setRxConfig(&rxConfig);
  // lora.sendData("0505");
  RecvInfo test;
  lora.receiveData(test);
  Serial.println(test.message);
  Serial.println("from="+String(test.fromAddr));
  Serial.println("rssi="+String(test.rssi)+" snr="+String(test.snr));
  */
=======
    Led_init();
    Button_init();
    Laser_I2C_init();
    LaserStart();
    Acc_init();
    LoRa_init();
>>>>>>> xr-FreeRTOS

/** */
    // 创建按键检测任务
    xTaskCreatePinnedToCore(
        buttonTask,        // 任务函数
        "ButtonTask",      // 任务名称
        4096,             // 堆栈大小
        NULL,             // 任务参数
        1,                // 任务优先级
        &ButtonTaskHandle,// 任务句柄
        1                 // 运行核心 (1 = 核心1)
    );

<<<<<<< HEAD
=======
    Led_init();
//    Button_init();
//    Laser_I2C_init();
//    LaserStart();
//    Acc_init();
    LoRa_init();
    Watchdog_init();  // 初始化看门狗

    // 创建看门狗任务（优先级设置为最高）
    xTaskCreatePinnedToCore(
        watchdogTask,         // 任务函数
        "WatchdogTask",       // 任务名称
        4096,                 // 堆栈大小
        NULL,                 // 任务参数
        configMAX_PRIORITIES - 1,  // 最高优先级
        &WatchdogTaskHandle,  // 任务句柄
        1                     // 运行核心 (1 = 核心1)
    );

    // 创建看门狗测试任务
    xTaskCreatePinnedToCore(
        wdtTestTask,          // 任务函数
        "WdtTestTask",        // 任务名称
        4096,                 // 堆栈大小
        NULL,                 // 任务参数
        1,                    // 任务优先级
        &WdtTestTaskHandle,   // 任务句柄
        1                     // 运行核心 (1 = 核心1)
    );

/** *
    // 创建按键检测任务
    xTaskCreatePinnedToCore(
        buttonTask,        // 任务函数
        "ButtonTask",      // 任务名称
        4096,             // 堆栈大小
        NULL,             // 任务参数
        1,                // 任务优先级
        &ButtonTaskHandle,// 任务句柄
        1                 // 运行核心 (1 = 核心1)
    );

/** *
=======
/** */
>>>>>>> xr-FreeRTOS
  // 创建激光测距任务
    xTaskCreatePinnedToCore(
        laserTask,           // 任务函数
        "LaserTask",         // 任务名称
        4096,               // 堆栈大小
        NULL,                // 任务参数
        1,                   // 任务优先级
        &laserTaskHandle,    // 任务句柄
        1                    // 运行核心 (1 = 核心1)
    );
<<<<<<< HEAD
/** *
=======
/** */
>>>>>>> xr-FreeRTOS
  // 创建加速度计任务
    xTaskCreatePinnedToCore(
        accelerometerTask,   // 任务函数
        "AccelerometerTask", // 任务名称
        4096,                // 堆栈大小
        NULL,                // 任务参数
        1,                   // 任务优先级
        &AccTaskHandle,      // 任务句柄
        1                    // 运行核心 (1 = 核心1)
    );
/** *
  // 创建灯光测试任务
    xTaskCreatePinnedToCore(
        ledTestTask,         // 任务函数
        "LedTestTask",       // 任务名称
        4096,                // 堆栈大小
        NULL,                // 任务参数
        1,                   // 任务优先级
        &LedTestTaskHandle,  // 任务句柄
        1                    // 运行核心 (1 = 核心1)
    );
/** */
  // 创建灯光任务
    xTaskCreatePinnedToCore(
        ledTask,         // 任务函数
        "LedTask",       // 任务名称
        4096,            // 堆栈大小
        NULL,            // 任务参数
        1,               // 任务优先级
        &LedTaskHandle,  // 任务句柄
        1                // 运行核心 (1 = 核心1)
    );
/** */
  // 创建LoRa测试任务
    xTaskCreatePinnedToCore(
        loraTestTask,           // 任务函数
        "LoraTestTask",         // 任务名称
        4096,                   // 堆栈大小
        NULL,                   // 任务参数
        1,                      // 任务优先级
        &loraTestTaskHandle,    // 任务句柄
        1                       // 运行核心 (1 = 核心1)
    );
/** */
  // 创建延迟测量任务
    xTaskCreatePinnedToCore(
        latencyTask,           // 任务函数
        "LatencyTask",         // 任务名称
        4096,                  // 堆栈大小
        NULL,                  // 任务参数
        1,                     // 任务优先级
        &latencyTaskHandle,    // 任务句柄
        1                      // 运行核心 (1 = 核心1)
    );
/** */
  // 删除setup任务，因为不再需要
    vTaskDelete(NULL);
/** */
<<<<<<< HEAD
>>>>>>> Stashed changes
=======
>>>>>>> xr-FreeRTOS
}

void loop() {

<<<<<<< HEAD
  // **动态显示**
    tft.fillScreen(ST77XX_BLUE);
    delay(500);
    
    tft.fillScreen(ST77XX_RED);
    delay(500);

  int16_t distance = laser.receiveReadResponse();
  if (distance != -1) {
    Serial.print("测量的距离: ");
    Serial.println(distance);
    tft.setCursor(10, 10);
    tft.println(distance);
  }
  
  // 添加一个小延时，避免过于频繁的读取
  delay(10);
=======
>>>>>>> xr-FreeRTOS
}

// put function definitions here:
