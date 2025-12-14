#include "button_control.h"
#include "config.h"
#include "state.h"
#include "notif_screen.h"

// Button state tracking
static bool lastBtnClearNotifs = HIGH;  // Pull-up means HIGH when not pressed
static unsigned long lastDebounceTime = 0;

void initButtons() {
  // GPIO 34-39 are input-only, no internal pull-up
  // External pull-up resistor required
  pinMode(BTN_CLEAR_NOTIFS, INPUT);

  Serial.println("Buttons initialized");
}

void checkButtons() {
  // Read current state (LOW = pressed with pull-up)
  bool currentState = digitalRead(BTN_CLEAR_NOTIFS);

  // Debounce: only act if state changed and debounce time passed
  if (currentState != lastBtnClearNotifs) {
    if (millis() - lastDebounceTime > BTN_DEBOUNCE_MS) {
      lastDebounceTime = millis();

      // Button pressed (HIGH -> LOW transition)
      if (currentState == LOW) {
        Serial.println("Button: Clear Notifications");
        clearAllNotifications();
      }

      lastBtnClearNotifs = currentState;
    }
  }
}
