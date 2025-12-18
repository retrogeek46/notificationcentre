#include "notif_screen.h"
#include "state.h"
#include "config.h"
#include "screen.h"
#include "led_control.h"
#include "icons/icons.h"

// ==================== Draw Content ====================
void drawNotifContent() {
  tft.setTextSize(1);

  // Y start positions for each notification slot (only 3 visible slots)
  const int slotYStarts[] = {ZONE_NOTIF1_Y_START, ZONE_NOTIF2_Y_START, ZONE_NOTIF3_Y_START};
  const int numSlots = 3;  // Only 3 visible slots on screen

  for (int i = 0; i < min(MAX_NOTIFICATIONS, numSlots); i++) {
    int y = slotYStarts[i] + 5;  // 3px padding from zone top

    if (notifications[i].message != "") {
      // Draw app icon
      drawAppIcon(5, y, notifications[i].app);

      // Draw sender
      tft.setTextColor(notifications[i].color);
      String sender = notifications[i].from;
      if (sender.length() > NOTIF_SENDER_MAX_CHARS) {
        sender = sender.substring(0, NOTIF_SENDER_MAX_CHARS);
      }
      tft.drawString(sender + ":", 27, y);

      // Draw message (starting from screen edge for more chars)
      tft.setTextColor(COLOR_NOTIF_MSG);
      String msg = notifications[i].message;
      if (msg.length() > NOTIF_MSG_MAX_CHARS - 1) {
        msg = msg.substring(0, NOTIF_MSG_MAX_CHARS - 1) + "...";
      }

      // Line 1
      String msgLine1 = msg.substring(0, min(NOTIF_MSG_LINE_CHARS, (int)msg.length()));
      msgLine1.trim();
      tft.drawString(msgLine1, 5, y + 20);

      // Line 2
      if (msg.length() > NOTIF_MSG_LINE_CHARS) {
        String msgLine2 = msg.substring(NOTIF_MSG_LINE_CHARS);
        msgLine2.trim();
        tft.drawString(msgLine2, 5, y + 40);
      }
    }
  }
}

// ==================== Add Notification ====================
void addNotification(String app, String from, String msg, uint16_t color) {
  // Shift existing notifications down
  for (int i = MAX_NOTIFICATIONS - 1; i > 0; i--) {
    notifications[i] = notifications[i - 1];
  }

  // Add new notification at top
  notifications[0].app = app;
  notifications[0].from = from;
  notifications[0].message = msg.substring(0, NOTIF_MSG_MAX_CHARS);
  notifications[0].color = color;

  // Update LED and screen
  updateLedForScreen(SCREEN_NOTIFS);
  blinkLed(2, 100);  // Blink twice on new notification

  // Switch to notifications screen
  currentScreen = SCREEN_NOTIFS;
  setZoneDirty(ZONE_TITLE);
  setZoneDirty(ZONE_CONTENT);
}

// ==================== Clear All ====================
void clearAllNotifications() {
  for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
    notifications[i] = Notification();
  }
  ledOff();
  setZoneDirty(ZONE_CONTENT);
}

// ==================== Helpers ====================
uint16_t getPriorityColor(const char* priority) {
  if (!priority) return COLOR_PRIORITY_NORMAL;
  if (strcmp(priority, "high") == 0) return COLOR_PRIORITY_HIGH;
  if (strcmp(priority, "medium") == 0) return COLOR_PRIORITY_MEDIUM;
  return COLOR_PRIORITY_NORMAL;
}

String extractSender(String msg) {
  if (msg.length() == 0) return "Unknown";
  int colonIndex = msg.lastIndexOf(':');
  if (colonIndex != -1) {
    String sender = msg.substring(colonIndex + 1);
    sender.trim();
    return sender.length() > 0 ? sender : "Unknown";
  }
  return msg.length() > 0 ? msg : "Unknown";
}
