#include "reminder_screen.h"
#include "state.h"
#include "config.h"
#include "screen.h"
#include "led_control.h"
#include "storage.h"
#include <time.h>
#include "fonts/MDIOTrial_Regular8pt7b.h"
#include "fonts/MDIOTrial_Bold8pt7b.h"

// ==================== Draw Content ====================
void drawReminderContent() {
  // Build sorted list of active reminders
  int listIdx[MAX_REMINDERS];
  time_t listTime[MAX_REMINDERS];
  bool listTriggered[MAX_REMINDERS];
  int count = 0;

  for (int i = 0; i < MAX_REMINDERS; i++) {
    Reminder& r = reminders[i];
    if (r.id == 0 || r.completed) continue;

    time_t eff = r.triggered ? (r.nextReviewTime != 0 ? r.nextReviewTime : r.when) : r.when;
    listIdx[count] = i;
    listTime[count] = eff;
    listTriggered[count] = r.triggered;
    count++;
  }

  // Sort: triggered first, then by time
  for (int a = 0; a < count - 1; a++) {
    int best = a;
    for (int b = a + 1; b < count; b++) {
      if (listTriggered[b] && !listTriggered[best]) {
        best = b;
      } else if (listTriggered[b] == listTriggered[best] && listTime[b] < listTime[best]) {
        best = b;
      }
    }
    if (best != a) {
      int ti = listIdx[a]; listIdx[a] = listIdx[best]; listIdx[best] = ti;
      time_t tt = listTime[a]; listTime[a] = listTime[best]; listTime[best] = tt;
      bool tb = listTriggered[a]; listTriggered[a] = listTriggered[best]; listTriggered[best] = tb;
    }
  }

  // Display up to 3 reminders (matching 65px slots)
  const int slotYStarts[] = {ZONE_NOTIF1_Y_START, ZONE_NOTIF2_Y_START, ZONE_NOTIF3_Y_START};
  int shown = 0;
  time_t now = time(nullptr);

  for (int s = 0; s < count && shown < 3; s++) {
    int y = slotYStarts[shown] + 5; // Match notif_screen padding
    Reminder& rm = reminders[listIdx[s]];

    // Icon (Centered at X=11 to match 14x14 icon alignment)
    uint16_t iconColor = rm.triggered ? COLOR_REMINDER_ICON_ACTIVE : COLOR_REMINDER_ICON_INACTIVE;
    tft.fillCircle(11, y + 7, REMINDER_ICON_RADIUS, iconColor);
    tft.drawCircle(11, y + 7, REMINDER_ICON_RADIUS, COLOR_ICON_BORDER);

    // Line 1: [id] + due time (Bold, starts at X=27)
    tft.setFreeFont(&MDIOTrial_Bold8pt7b);
    tft.setTextColor(COLOR_REMINDER_DUE);
    time_t effTime = listTime[s];
    char buf[32];

    if (rm.triggered) {
      strcpy(buf, "due now");
    } else {
      long diff = effTime - now;
      long days = diff / 86400;
      long hours = (diff % 86400) / 3600;
      long mins = (diff % 3600) / 60;

      strcpy(buf, "due in ");
      if (days > 0) sprintf(buf + strlen(buf), "%ldD ", days);
      if (hours > 0) sprintf(buf + strlen(buf), "%ldH ", hours);
      if (mins > 0 || (days == 0 && hours == 0)) sprintf(buf + strlen(buf), "%ldM", mins);
    }

    String line1 = "[" + String(rm.id) + "] " + String(buf);
    tft.drawString(line1, 27, y);

    // Message (Starting from X=5 for more space, match notif_screen logic)
    tft.setFreeFont(&MDIOTrial_Regular8pt7b);
    tft.setTextColor(rm.triggered ? COLOR_REMINDER_ACTIVE : COLOR_REMINDER_INACTIVE);
    String msg = rm.message;
    if (msg.length() > REMINDER_MSG_MAX_CHARS - 1) {
      msg = msg.substring(0, REMINDER_MSG_MAX_CHARS - 1) + "...";
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

    shown++;
  }
}

// ==================== Check Reminders ====================
void checkReminders() {
  time_t now = time(nullptr);

  for (int i = 0; i < MAX_REMINDERS; i++) {
    Reminder& r = reminders[i];
    if (r.id == 0 || r.completed) continue;

    // Initial trigger
    if (!r.triggered && r.when != 0 && now >= r.when) {
      r.triggered = true;
      r.reviewCount = 0;
      if (r.limitMinutes > 0) {
        r.nextReviewTime = r.when + (time_t)r.limitMinutes * 60;
      }
      Serial.printf("Reminder triggered id=%d msg=%s\n", r.id, r.message.c_str());

      // Visual feedback
      updateLedForScreen(SCREEN_REMINDER);
      blinkLed(3, 150);
      currentScreen = SCREEN_REMINDER;
      setZoneDirty(ZONE_TITLE);
      setZoneDirty(ZONE_CONTENT);
    }
    // Follow-up trigger
    else if (r.triggered && r.limitMinutes > 0 && r.nextReviewTime != 0 && now >= r.nextReviewTime) {
      r.reviewCount++;
      r.nextReviewTime = now + (time_t)r.limitMinutes * 60;
      Serial.printf("Reminder follow-up id=%d reviewCount=%d\n", r.id, r.reviewCount);

      blinkLed(2, 100);
      setZoneDirty(ZONE_CONTENT);
    }
  }
}

// ==================== Add Reminder ====================
int addReminder(String msg, time_t when, int limitMins, uint16_t color) {
  // Find free slot
  int idx = -1;
  for (int i = 0; i < MAX_REMINDERS; i++) {
    if (reminders[i].id == 0) {
      idx = i;
      break;
    }
  }

  if (idx == -1) return -1;  // No free slot

  reminders[idx].id = nextReminderId++;
  reminders[idx].message = msg;
  reminders[idx].when = when;
  reminders[idx].limitMinutes = max(0, limitMins);
  reminders[idx].completed = false;
  reminders[idx].color = color;
  reminders[idx].triggered = false;
  reminders[idx].nextReviewTime = 0;
  reminders[idx].reviewCount = 0;

  setZoneDirty(ZONE_CONTENT);
  saveReminders();  // Persist to flash

  return reminders[idx].id;
}

// ==================== Complete Reminder ====================
bool completeReminder(int id) {
  for (int i = 0; i < MAX_REMINDERS; i++) {
    if (reminders[i].id == id) {
      reminders[i].completed = true;
      reminders[i].triggered = false;
      reminders[i].nextReviewTime = 0;
      reminders[i].reviewCount = 0;
      ledOff();
      setZoneDirty(ZONE_CONTENT);
      saveReminders();  // Persist to flash
      Serial.printf("Reminder %d completed\n", id);
      return true;
    }
  }
  return false;
}

// ==================== List Reminders JSON ====================
String listRemindersJson() {
  String out = "[";
  bool first = true;

  for (int i = 0; i < MAX_REMINDERS; i++) {
    Reminder& r = reminders[i];
    if (r.id == 0) continue;

    if (!first) out += ",";
    first = false;

    struct tm tm;
    localtime_r(&r.when, &tm);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm);

    out += "{";
    out += "\"id\":" + String(r.id) + ",";
    out += "\"message\":\"" + r.message + "\",";
    out += "\"time\":\"" + String(buf) + "\",";
    out += "\"limit\":" + String(r.limitMinutes) + ",";
    out += "\"completed\":" + String(r.completed ? "true" : "false") + ",";
    out += "\"priority\":" + String((r.color == COLOR_PRIORITY_HIGH) ? "\"high\"" :
                                    ((r.color == COLOR_PRIORITY_MEDIUM) ? "\"medium\"" : "\"normal\""));
    out += "}";
  }

  out += "]";
  return out;
}

// ==================== Parse DateTime ====================
time_t parseDateTime(const String& dt) {
  if (dt.length() < 16) return 0;

  int year = dt.substring(0, 4).toInt();
  int month = dt.substring(5, 7).toInt();
  int day = dt.substring(8, 10).toInt();
  int hour = dt.substring(11, 13).toInt();
  int minute = dt.substring(14, 16).toInt();

  if (year < 2000 || month < 1 || month > 12 || day < 1) return 0;

  struct tm tm;
  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = 0;
  tm.tm_isdst = -1;

  return mktime(&tm);
}
