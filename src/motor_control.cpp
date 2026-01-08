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

  // Start with default motor speed
  ledcWrite(MOTOR_PWM_CHANNEL, 200);

  Serial.println("Motor PWM ready (single direction, 1kHz, 8-bit)");
}

void setMotorRaw(int speed) {
  speed = constrain(speed, 0, 255);
  
  // Soft ramp to target speed (prevents motor jumping)
  int currentPWM = motorSpeed;
  int step = (speed > currentPWM) ? 5 : -5;
  
  while (currentPWM != speed) {
    currentPWM += step;
    // Clamp to target
    if ((step > 0 && currentPWM > speed) || (step < 0 && currentPWM < speed)) {
      currentPWM = speed;
    }
    ledcWrite(MOTOR_PWM_CHANNEL, currentPWM);
    delay(2);  // ~100ms total ramp time for full range
  }
  
  motorSpeed = speed;
}

int getMotorSpeed() {
  return motorSpeed;
}
