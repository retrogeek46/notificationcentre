#include "encoder_control.h"
#include "config.h"
#include "motor_control.h"

// Encoder state
static int lastCLK = HIGH;
static bool motorRunning = false;
static int targetSpeed = 0;  // Default mid-speed

// Button state
static bool lastBtnState = HIGH;
static unsigned long lastBtnDebounce = 0;

void initEncoder() {
#if ENCODER_ENABLED
  // Input-only pins, no internal pull-up - external pull-ups required
  pinMode(ENCODER_CLK, INPUT);
  pinMode(ENCODER_DT, INPUT);
  pinMode(ENCODER_SW, INPUT);

  lastCLK = digitalRead(ENCODER_CLK);

  Serial.println("Encoder initialized");
#else
  Serial.println("Encoder disabled in config");
#endif
}

void checkEncoder() {
#if !ENCODER_ENABLED
  return;
#endif
  // --- Rotation handling ---
  int currentCLK = digitalRead(ENCODER_CLK);

  if (currentCLK != lastCLK && currentCLK == LOW) {
    // CLK changed, check direction via DT
    int dtValue = digitalRead(ENCODER_DT);

    if (dtValue != currentCLK) {
      // Clockwise - increase speed
      targetSpeed = min(255, targetSpeed + ENCODER_SPEED_STEP);
    } else {
      // Counter-clockwise - decrease speed
      targetSpeed = max(ENCODER_MIN_SPEED, targetSpeed - ENCODER_SPEED_STEP);
    }

    Serial.printf("Encoder: speed=%d\n", targetSpeed);

    // Update motor if running
    if (motorRunning) {
      setMotorRaw(targetSpeed);
    }
  }
  lastCLK = currentCLK;

  // --- Button handling ---
  bool currentBtn = digitalRead(ENCODER_SW);

  if (currentBtn != lastBtnState) {
    if (millis() - lastBtnDebounce > BTN_DEBOUNCE_MS) {
      lastBtnDebounce = millis();

      // Button pressed (HIGH -> LOW with pull-up)
      if (currentBtn == LOW) {
        motorRunning = !motorRunning;

        if (motorRunning) {
          setMotorRaw(targetSpeed);
          Serial.printf("Motor ON at speed %d\n", targetSpeed);
        } else {
          setMotorRaw(0);
          Serial.println("Motor OFF");
        }
      }

      lastBtnState = currentBtn;
    }
  }
}
