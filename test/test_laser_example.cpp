#include <Arduino.h>
#include "LoRaModule.h"
#include "LaserModule.h"
// put function declarations here:
Laser laser;


void setup() {
    Serial.begin(115200);
    Serial1.begin(921600, SERIAL_8N1, 18, 17);
    laser.begin();
    laser.sendReadCommand();

}

void loop() {
    int16_t distance = laser.receiveReadResponse();
    if (distance != -1) {
        Serial.print("测量的距离: ");
        Serial.println(distance);
    }
}

// put function definitions here:
