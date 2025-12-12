#include "motor_control.h"
#include "config.h"

static int motorSpeed = 0;  // Current speed (0-255)

void initMotor() {
  // Setup PWM on ENA pin for speed control
  ledcSetup(MOTOR_PWM_CHANNEL, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
  ledcAttachPin(MOTOR_ENA, MOTOR_PWM_CHANNEL);

  // Direction pin (IN1 only, IN2 is hardwired to GND)
  pinMode(MOTOR_IN1, OUTPUT);
  digitalWrite(MOTOR_IN1, HIGH);  // Always forward

  // Start with motor off
  ledcWrite(MOTOR_PWM_CHANNEL, 0);

  Serial.println("Motor PWM ready (single direction, 1kHz, 8-bit)");
}

void setMotorRaw(int speed) {
  speed = constrain(speed, 0, 255);
  motorSpeed = speed;

  // Set PWM duty cycle (IN1 stays HIGH, IN2 hardwired to GND)
  ledcWrite(MOTOR_PWM_CHANNEL, speed);
}

int getMotorSpeed() {
  return motorSpeed;
}
