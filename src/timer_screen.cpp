#include "timer_screen.h"
#include "state.h"
#include "config.h"
#include "led_control.h"
#include "screen.h"
#include "fonts/MDIOTrial_Bold10pt7b.h"
#include "fonts/MDIOTrial_Regular9pt7b.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

// LED flash state for timer complete
static unsigned long lastLedFlash = 0;
static bool ledFlashState = false;

// Timer display state (file-scope so reset function can access)
static bool timerFirstDraw = true;
static int timerLastDisplayedMinutes = -1;

void drawTimerContent() {
  // Dial center coordinates (screen coordinates)
  int cx = TIMER_DIAL_CX;  // 160
  int cy = TIMER_DIAL_CY;  // 142

  // First draw or switching to timer screen - clear entire content area
  if (timerFirstDraw) {
    tft.fillRect(ZONE_CONTENT1_X_START, ZONE_CONTENT1_Y_START,
                 ZONE_CONTENT1_X_END - ZONE_CONTENT1_X_START + 1,
                 ZONE_CONTENT3_Y_END - ZONE_CONTENT1_Y_START + 1,
                 COLOR_BACKGROUND);

    // Draw dial circles (static - only on first draw)
    tft.drawCircle(cx, cy, TIMER_DIAL_RADIUS, COLOR_TIMER_DIAL);
    tft.drawCircle(cx, cy, TIMER_DIAL_RADIUS - 1, COLOR_TIMER_DIAL);

    // Draw 60 tick marks (static - only on first draw)
    for (int i = 0; i < 60; i++) {
      float angle = (i * 6 - 90) * PI / 180.0;
      int innerR = (i % 5 == 0) ? TIMER_TICK_MAJOR_INNER : TIMER_TICK_INNER;
      int outerR = TIMER_TICK_OUTER;

      int x1 = cx + (int)(innerR * cos(angle));
      int y1 = cy + (int)(innerR * sin(angle));
      int x2 = cx + (int)(outerR * cos(angle));
      int y2 = cy + (int)(outerR * sin(angle));

      uint16_t tickColor = (i % 5 == 0) ? TFT_WHITE : COLOR_TIMER_DIAL;
      tft.drawLine(x1, y1, x2, y2, tickColor);
    }

    timerFirstDraw = false;
    timerLastDisplayedMinutes = -1;  // Force arc redraw
  }

  // Only redraw arc if minutes changed
  int displayMinutes = timerMinutes;
  if (displayMinutes != timerLastDisplayedMinutes) {
    // Clear arc area first
    tft.fillCircle(cx, cy, TIMER_ARC_OUTER, COLOR_BACKGROUND);

    // Redraw tick marks inside arc area
    for (int i = 0; i < 60; i++) {
      float angle = (i * 6 - 90) * PI / 180.0;
      int innerR = (i % 5 == 0) ? TIMER_TICK_MAJOR_INNER : TIMER_TICK_INNER;
      int outerR = TIMER_TICK_OUTER;

      int x1 = cx + (int)(innerR * cos(angle));
      int y1 = cy + (int)(innerR * sin(angle));
      int x2 = cx + (int)(outerR * cos(angle));
      int y2 = cy + (int)(outerR * sin(angle));

      uint16_t tickColor = (i % 5 == 0) ? TFT_WHITE : COLOR_TIMER_DIAL;
      tft.drawLine(x1, y1, x2, y2, tickColor);
    }

    // Draw filled arc for selected/remaining time
    if (displayMinutes > 0) {
      float endAngle = (displayMinutes * 6.0);
      for (float a = 0; a < endAngle; a += 1.0) {
        float rad = (a - 90) * PI / 180.0;
        for (int r = TIMER_ARC_INNER; r <= TIMER_ARC_OUTER; r += 2) {
          int px = cx + (int)(r * cos(rad));
          int py = cy + (int)(r * sin(rad));
          tft.drawPixel(px, py, COLOR_TIMER_FILL);
        }
      }
    }

    timerLastDisplayedMinutes = displayMinutes;
  }

  // Clear center text area and redraw time
  tft.fillRect(cx - TIMER_CENTER_TEXT_W, cy - TIMER_CENTER_TEXT_H,
               TIMER_CENTER_TEXT_W * 2, TIMER_CENTER_TEXT_H * 2, COLOR_BACKGROUND);

  char timeStr[8];
  if (timerRunning) {
    if (timerEndMs > millis()) {
      unsigned long remaining = timerEndMs - millis();
      int mins = remaining / 60000;
      int secsRemaining = (remaining % 60000) / 1000;
      snprintf(timeStr, sizeof(timeStr), "%d:%02d", mins, secsRemaining);
    } else {
      snprintf(timeStr, sizeof(timeStr), "0:00");
    }
  } else {
    snprintf(timeStr, sizeof(timeStr), "%d", timerMinutes);
  }

  tft.setFreeFont(&MDIOTrial_Bold10pt7b);
  tft.setTextDatum(MC_DATUM);
  uint16_t textColor = timerRunning ? COLOR_TIMER_RUNNING : COLOR_TIMER_TEXT;
  tft.setTextColor(textColor);
  tft.drawString(timeStr, cx, cy);

  // Clear and redraw instruction text
  tft.fillRect(cx - TIMER_INSTR_HALF_W, cy + TIMER_DIAL_RADIUS + TIMER_INSTR_Y_CLEAR,
               TIMER_INSTR_HALF_W * 2, 20, COLOR_BACKGROUND);
  tft.setFreeFont(&MDIOTrial_Regular9pt7b);
  tft.setTextColor(COLOR_TIMER_DIAL);
  if (!timerRunning) {
    tft.drawString("Rotate: Set | Press: Start", cx, cy + TIMER_DIAL_RADIUS + TIMER_INSTR_Y_TEXT);
  } else {
    tft.drawString("Press: Cancel", cx, cy + TIMER_DIAL_RADIUS + TIMER_INSTR_Y_TEXT);
  }

  tft.setTextDatum(TL_DATUM);

#if DEBUG_NO_BG_CONTENT_ZONE
  // Draw debug border for content zone
  tft.drawRect(ZONE_CONTENT1_X_START, ZONE_CONTENT1_Y_START,
               ZONE_CONTENT1_X_END - ZONE_CONTENT1_X_START + 1,
               ZONE_CONTENT3_Y_END - ZONE_CONTENT1_Y_START + 1, TFT_WHITE);
#endif
}

// Reset first draw flag when entering timer screen
void resetTimerScreen() {
  timerFirstDraw = true;
  timerLastDisplayedMinutes = -1;
}

void updateTimerTick() {
  if (!timerRunning && !timerComplete) {
    return;
  }

  unsigned long now = millis();

  // Handle timer completion LED flash
  if (timerComplete) {
    // Flash LED at 500ms intervals
    if (now - lastLedFlash >= TIMER_COMPLETE_FLASH_MS) {
      ledFlashState = !ledFlashState;
      if (ledFlashState) {
        setLedColor(255, 0, 0);  // Red flash
      } else {
        ledOff();
      }
      lastLedFlash = now;
    }

    // Stop flashing after 10 seconds
    if (now - timerCompleteStartMs > 10000) {
      timerComplete = false;
      ledOff();
    }
    return;
  }

  // Check if timer has completed
  if (timerRunning && now >= timerEndMs) {
    timerRunning = false;
    timerMinutes = 0;
    timerComplete = true;
    timerCompleteStartMs = now;
    lastLedFlash = now;
    ledFlashState = true;
    setLedColor(255, 0, 0);  // Start with LED on

    // Mark timer screen dirty
    if (currentScreen == SCREEN_TIMER) {
      setAllContentDirty();
    }
    setZoneDirty(ZONE_STATUS);  // Update status zone
    return;
  }

  // Update remaining time for display
  if (timerRunning && timerEndMs > now) {
    unsigned long remaining = timerEndMs - now;
    timerMinutes = (remaining + 59999) / 60000;  // Round up to nearest minute

    // Redraw timer screen every second for countdown
    static unsigned long lastTimerRedraw = 0;
    if (now - lastTimerRedraw >= 1000) {
      if (currentScreen == SCREEN_TIMER) {
        setAllContentDirty();
      }
      lastTimerRedraw = now;
    }
  }
}

void startTimer() {
  if (timerMinutes > 0 && !timerRunning) {
    timerOriginalMinutes = timerMinutes;
    timerEndMs = millis() + (timerMinutes * 60000UL);
    timerRunning = true;
    timerComplete = false;
    setAllContentDirty();
    setZoneDirty(ZONE_STATUS);
    Serial.printf("Timer started: %d minutes\n", timerMinutes);
  }
}

void stopTimer() {
  if (timerRunning) {
    timerRunning = false;
    timerMinutes = 0;
    timerEndMs = 0;
    setAllContentDirty();
    setZoneDirty(ZONE_STATUS);
    Serial.println("Timer stopped");
  }
  // Also stop completion flash if active
  if (timerComplete) {
    timerComplete = false;
    ledOff();
  }
}

void adjustTimerMinutes(int delta) {
  if (timerRunning) {
    return;  // Don't adjust while running
  }

  timerMinutes += delta;
  if (timerMinutes < 0) timerMinutes = 0;
  if (timerMinutes > TIMER_MAX_MINUTES) timerMinutes = TIMER_MAX_MINUTES;

  Serial.printf("Timer set to: %d minutes\n", timerMinutes);
  setAllContentDirty();
}
