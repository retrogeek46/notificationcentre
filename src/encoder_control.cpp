#include "encoder_control.h"
#include "config.h"
#include "state.h"
#include "motor_control.h"
#include "timer_screen.h"

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
    int direction = (dtValue != currentCLK) ? 1 : -1;  // CW = 1, CCW = -1

    // If not on timer screen, switch to it
    if (currentScreen != SCREEN_TIMER) {
      currentScreen = SCREEN_TIMER;
      resetTimerScreen();  // Reset timer display state for fresh draw
      setAllZonesDirty();
      Serial.println("Encoder: Switched to Timer screen");
    }

    // Adjust timer minutes
    adjustTimerMinutes(direction);

    Serial.printf("Encoder: timer=%d min\n", timerMinutes);
  }
  lastCLK = currentCLK;

  // --- Button handling ---
  bool currentBtn = digitalRead(ENCODER_SW);

  if (currentBtn != lastBtnState) {
    if (millis() - lastBtnDebounce > BTN_DEBOUNCE_MS) {
      lastBtnDebounce = millis();

      // Button pressed (HIGH -> LOW with pull-up)
      if (currentBtn == LOW) {
        // Timer screen: start/stop timer
        if (currentScreen == SCREEN_TIMER) {
          if (timerRunning) {
            stopTimer();
            Serial.println("Encoder: Timer stopped");
          } else if (timerMinutes > 0) {
            startTimer();
            Serial.println("Encoder: Timer started");
          }
        } else {
          // Other screens: toggle motor (original behavior)
          motorRunning = !motorRunning;
          if (motorRunning) {
            setMotorRaw(targetSpeed);
            Serial.printf("Motor ON at speed %d\n", targetSpeed);
          } else {
            setMotorRaw(0);
            Serial.println("Motor OFF");
          }
        }
      }

      lastBtnState = currentBtn;
    }
  }
}

