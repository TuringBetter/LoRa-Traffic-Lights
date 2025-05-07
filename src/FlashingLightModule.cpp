#include "FlashingLightModule.h"

static const int               FLASHING_LIGHT_PIN      = 40;

void FlashingLight_init()
{
    pinMode(FLASHING_LIGHT_PIN,OUTPUT);
}

void FlashingLight_on()
{
    digitalWrite(FLASHING_LIGHT_PIN,LOW);
}

void FlashingLight_off()
{
    digitalWrite(FLASHING_LIGHT_PIN,HIGH);
}
