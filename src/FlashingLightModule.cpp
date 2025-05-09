#include "FlashingLightModule.h"

static const int               FLASHING_LIGHT_PIN      = 40;

void FlashingLight_init()
{
    pinMode(FLASHING_LIGHT_PIN,OUTPUT);
}

void FlashingLight_on()
{
    Serial.println("Flashing on...");
    digitalWrite(FLASHING_LIGHT_PIN,LOW);
}

void FlashingLight_off()
{
    Serial.println("Flashing off...");
    digitalWrite(FLASHING_LIGHT_PIN,HIGH);
}
