#include "LaserModule.h"

static uint16_t crc16_table[256];

static void crc16_init() {
    uint16_t crc;
    for (int i = 0; i < 256; i++) 
    {
        crc = i;
        for (int j = 8; j > 0; j--) 
        {
            if (crc & 0x0001) 
                crc = (crc >> 1) ^ 0xA001;
            else 
                crc >>= 1;
        }
        crc16_table[i] = crc;
    }
}

static uint16_t calculateCRC16(uint8_t *data, size_t length){
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc16_table[index];
    }
    return crc;
}

void Laser::begin()
{
    Serial1.begin(921600, SERIAL_8N1, RX_PIN, TX_PIN);
    crc16_init();
}

void Laser::sendReadCommand()
{
    static uint8_t data[] = {0xA5, 0x03, 0x20, 0x01, 0x00, 0x00, 0x00, 0x02, 0x6E};
    Serial1.write(data, sizeof(data));
}

void Laser::sendoverCommand(){
    static uint8_t data[] = {0xA5, 0x03, 0x20, 0x02, 0x00, 0x00, 0x00, 0x46, 0x6E};
    Serial1.write(data, sizeof(data));
}


int16_t Laser::receiveReadResponse(){
    int available = Serial1.available();
    /*
    Serial.print("Available bytes: ");
    Serial.println(available);
    */
    // 如果数据太多，清空缓冲区
    if (available >= READ_DATA_LENGTH + 10) {
        Serial.println("Buffer overflow, clearing...");
        while(Serial1.available()) {
            Serial1.read();
        }
        return -1;
    }
    
    int16_t mainPeakCentroid;
    if (available >= READ_DATA_LENGTH && available < READ_DATA_LENGTH+10) 
    {
        uint8_t buffer[25];
        Serial1.readBytes(buffer, READ_DATA_LENGTH);
        
        /*
        // 打印接收到的原始数据
        Serial.print("Received data: ");
        for(int i = 0; i < READ_DATA_LENGTH; i++) {
            Serial.print(buffer[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        */
        const size_t data_length = READ_DATA_LENGTH - 2;
        uint8_t *datapart = buffer;
        uint16_t received_crc = (buffer[data_length] << 8) | buffer[data_length + 1];
        uint16_t computed_crc = calculateCRC16(datapart, data_length);
        
        // Serial.print("Computed CRC: 0x");
        // Serial.println(computed_crc, HEX);
        // Serial.print("Received CRC: 0x");
        // Serial.println(received_crc, HEX);
        
        uint8_t mpcdata[3];
        if(computed_crc == received_crc)
        {
            for (int i = 0; i < 2; i++) 
                mpcdata[i] = buffer[13 + i];
            mainPeakCentroid = (mpcdata[1] << 8) | mpcdata[0];
            return mainPeakCentroid;
        }
        else
        {
            Serial.println("CRC ERROR");
            return -1;
        }
    }
    else
    {
        Serial.println("Data length out of range");
        return -1;
    }
}