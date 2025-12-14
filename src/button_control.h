#ifndef BUTTON_CONTROL_H
#define BUTTON_CONTROL_H

#include <Arduino.h>

// Initialize button pins
void initButtons();

// Check buttons and handle actions (call in loop)
void checkButtons();

#endif
