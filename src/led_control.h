#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>
#include "types.h"

void initLed();
void updateLedForScreen(Screen screen);
void blinkLed(int times, int delayMs);
void setLedColor(uint8_t r, uint8_t g, uint8_t b);
void ledOff();

#endif
