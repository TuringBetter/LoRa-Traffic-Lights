#include "LaserModule.h"

void Laser::begin()
{
    Serial1.begin(921600, SERIAL_8N1, 18, 17); // 初始化UART，使用GPIO18作为RX1，GPIO17作为TX1
    uint8_t data[] = {0xA5, 0x03, 0x20, 0x01, 0x00, 0x00, 0x00, 0x02, 0x6E};
    Serial1.write(data, sizeof(data));
}