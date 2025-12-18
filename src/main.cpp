/**
 * Notification Center - ESP32
 *
 * A modular notification and reminder display system
 * with zone-based screen rendering for minimal flicker.
 */

#include <Arduino.h>
#include "config.h"
#include "state.h"
#include "storage.h"
#include "led_control.h"
#include "motor_control.h"
#include "button_control.h"
#include "encoder_control.h"
#include "network.h"
#include "screen.h"
#include "api_handlers.h"
#include "notif_screen.h"
#include "reminder_screen.h"

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);

  // Initialize modules
  initLed();
  initMotor();
  initButtons();
  initEncoder();
  initScreen();
  initState();
  initStorage();  // Load persisted reminders

  // Network (shows status on screen)
  initWiFi();
  initNTP();

  // Start HTTP server
  setupApiRoutes();

  // Initial screen draw
  setAllZonesDirty();
  refreshScreen();

  Serial.println("Notification Center ready!");
}

// ==================== Loop ====================
void loop() {
  // Update clock every second
  static unsigned long lastClock = 0;
  if (millis() - lastClock > CLOCK_UPDATE_INTERVAL) {
    updateClock();
    lastClock = millis();
  }

  // Check reminders
  checkReminders();

  // Refresh reminder screen periodically (for countdown updates)
  static unsigned long lastReminderRefresh = 0;
  if (currentScreen == SCREEN_REMINDER && millis() - lastReminderRefresh > REMINDER_REFRESH_INTERVAL) {
    setZoneDirty(ZONE_CONTENT);
    lastReminderRefresh = millis();
  }

  // Update now playing scrolling ticker
  updateNowPlayingTicker();

  // Refresh dirty zones
  refreshScreen();

  // DEBUG: Draw zone boundaries (comment out when done)
  // drawDebugZones();

  // WiFi reconnect check
  checkWiFiReconnect();

  // Check physical buttons
  checkButtons();

  // Check rotary encoder
  checkEncoder();

  yield();
}
