#ifndef STATE_H
#define STATE_H

#include "types.h"

// ==================== Screen State ====================
extern Screen currentScreen;
extern bool zoneDirty[ZONE_COUNT];

// ==================== Timing ====================
extern unsigned long lastClockUpdate;
extern unsigned long lastReminderRefresh;

// ==================== Notifications ====================
extern Notification notifications[MAX_NOTIFICATIONS];

// ==================== Reminders ====================
extern Reminder reminders[MAX_REMINDERS];
extern int nextReminderId;

// ==================== Now Playing ====================
extern String nowPlayingSong;
extern String nowPlayingArtist;
extern unsigned long nowPlayingUpdated;

// ==================== Helper Functions ====================
void initState();
void setZoneDirty(Zone zone);
void setAllZonesDirty();
void clearZoneDirty(Zone zone);
bool isZoneDirty(Zone zone);

#endif
