#include "state.h"

// ==================== Screen State ====================
Screen currentScreen = SCREEN_NOTIFS;
bool zoneDirty[ZONE_COUNT] = {true, true, true, true, true, true};

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
int nowPlayingScrollPixel = 0;
unsigned long lastScrollUpdate = 0;
int discFrame = 0;
unsigned long lastDiscUpdate = 0;
int idleDiscX = 11;              // Start at center left
int idleDiscDirection = 1;       // Start moving right
unsigned long lastIdleDiscMove = 0;
bool nowPlayingActive = false;

// ==================== Album Art ====================
uint16_t albumArt[ALBUM_ART_MAX_PIXELS] = {0};
int albumArtWidth = ALBUM_ART_SIZE;   // Default to square
int albumArtHeight = ALBUM_ART_SIZE;
bool albumArtValid = false;

// ==================== PC Stats (Gaming Mode) ====================
bool gamingMode = false;
int pcCpuTemp = 0;
int pcCpuUsage = 0;
float pcCpuSpeed = 0.0;
int pcRamUsed = 0;
int pcRamTotal = 0;
int pcGpuTemp = 0;
int pcGpuUsage = 0;
float pcNetDown = 0.0;
float pcNetUp = 0.0;
unsigned long pcStatsUpdated = 0;

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
  nowPlayingScrollPixel = 0;
  lastScrollUpdate = 0;
  discFrame = 0;
  lastDiscUpdate = 0;
  idleDiscX = 11;
  idleDiscDirection = 1;
  lastIdleDiscMove = 0;
  nowPlayingActive = false;
  // Clear album art
  memset(albumArt, 0, sizeof(albumArt));
  albumArtValid = false;
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

void setAllContentDirty() {
  zoneDirty[ZONE_CONTENT1] = true;
  zoneDirty[ZONE_CONTENT2] = true;
  zoneDirty[ZONE_CONTENT3] = true;
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
