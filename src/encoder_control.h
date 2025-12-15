#ifndef ENCODER_CONTROL_H
#define ENCODER_CONTROL_H

#include <Arduino.h>

// Initialize encoder pins
void initEncoder();

// Check encoder and handle actions (call in loop)
void checkEncoder();

#endif
