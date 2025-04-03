#include <Arduino.h>
#include "LaserModule.h"

Laser laser(Serial1);

void setup() {
    // 初始化串口
    Serial.begin(115200);
    delay(1000);  // 等待串口稳定
    Serial1.begin(921600, SERIAL_8N1, 18, 17);
    
    // 初始化激光模块
    laser.begin();
    Serial.println("激光模块初始化完成");
    laser.sendReadCommand();
}

void loop() {
    laser.sendReadCommand();  // 发送读取命令
    
    int16_t result = laser.receiveReadResponse();
    switch(result) {
        case LASER_ERR_NO_DATA:
            Serial.println("错误: 未接收到数据，请检查连接");
            break;
        case LASER_ERR_BUFFER_OVERFLOW:
            Serial.println("错误: 接收缓冲区溢出");
            break;
        case LASER_ERR_DATA_LENGTH:
            Serial.println("错误: 数据长度异常");
            break;
        case LASER_ERR_CRC:
            Serial.println("错误: 数据校验失败");
            break;
        default:
            if(result >= 0) {
                Serial.print("测量的距离: ");
                Serial.print(result);
                Serial.println(" mm");
            }
            break;
    }
    
    delay(50);  // 测量间隔
}
