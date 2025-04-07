#include <Arduino.h>
#include "AccelerometerModule.h"

#define I2C_SDA 6
#define I2C_SCL 5

Accelerometer accelerometer;

// put function declarations here:



void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL);
    
    if (!accelerometer.begin(Accelerometer::RANGE_2G)) 
    {
        Serial.println("Failed to initialize accelerometer!");
        while(1);
    }
    Serial.println("Succeeded to initialize accelerometer!");
}

void loop() {
    int16_t x, y, z;
    accelerometer.readRaw(x, y, z);

    Serial.print("Raw Data - X:");
    Serial.print(x);
    Serial.print(" Y:");
    Serial.print(y);
    Serial.print(" Z:");
    Serial.println(z);

    float scale = accelerometer.getScaleFactor();
    Serial.print("Acceleration(g) - X:");
    Serial.print(x * scale, 4);
    Serial.print(" Y:");
    Serial.print(y * scale, 4);
    Serial.print(" Z:");
    Serial.println(z * scale, 4);

    delay(500);
}

// put function definitions here:

