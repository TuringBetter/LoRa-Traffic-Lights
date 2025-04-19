#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define I2C_ADDR        0x51   // 7位设备地址
#define CMD_REG         0x02   // 命令寄存器地址
#define DIST_HIGH_REG   0x00   // 距离高字节寄存器
#define DIST_LOW_REG    0x01   // 距离低字节寄存器

extern TaskHandle_t         laserTaskHandle;


void laserTask(void *pvParameters);

void Laser_I2C_init();
void LaserStart();