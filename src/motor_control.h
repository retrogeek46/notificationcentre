#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>

// Initialize motor driver pins
void initMotor();

// Set motor speed (0-255, 0 = off)
void setMotorRaw(int speed);

// Get current motor speed
int getMotorSpeed();

#endif
