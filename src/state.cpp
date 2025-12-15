#include "state.h"

// ==================== Screen State ====================
Screen currentScreen = SCREEN_NOTIFS;
bool zoneDirty[ZONE_COUNT] = {true, true, true, true};

// ==================== Timing ====================
unsigned long lastClockUpdate = 0;
unsigned long lastReminderRefresh = 0;

// ==================== Notifications ====================
Notification notifications[MAX_NOTIFICATIONS];

// ==================== Reminders ====================
Reminder reminders[MAX_REMINDERS];
int nextReminderId = 1;

// ==================== Now Playing ====================
String nowPlayingSong = "";
String nowPlayingArtist = "";
unsigned long nowPlayingUpdated = 0;
int nowPlayingScrollPos = 0;
unsigned long lastScrollUpdate = 0;
int discFrame = 0;
unsigned long lastDiscUpdate = 0;
bool nowPlayingActive = false;

// ==================== Helper Functions ====================
void initState() {
  currentScreen = SCREEN_NOTIFS;
  for (int i = 0; i < ZONE_COUNT; i++) {
    zoneDirty[i] = true;
  }
  for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
    notifications[i] = Notification();
  }
  for (int i = 0; i < MAX_REMINDERS; i++) {
    reminders[i] = Reminder();
  }
  nowPlayingSong = "";
  nowPlayingArtist = "";
  nowPlayingUpdated = 0;
  nowPlayingScrollPos = 0;
  lastScrollUpdate = 0;
  discFrame = 0;
  lastDiscUpdate = 0;
  nowPlayingActive = false;
  nextReminderId = 1;
}

void setZoneDirty(Zone zone) {
  if (zone >= 0 && zone < ZONE_COUNT) {
    zoneDirty[zone] = true;
  }
}

void setAllZonesDirty() {
  for (int i = 0; i < ZONE_COUNT; i++) {
    zoneDirty[i] = true;
  }
}

void clearZoneDirty(Zone zone) {
  if (zone >= 0 && zone < ZONE_COUNT) {
    zoneDirty[zone] = false;
  }
}

bool isZoneDirty(Zone zone) {
  if (zone >= 0 && zone < ZONE_COUNT) {
    return zoneDirty[zone];
  }
  return false;
}
