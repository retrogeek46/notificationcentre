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
extern int nowPlayingScrollPixel;      // Current scroll pixel offset
extern unsigned long lastScrollUpdate; // Last scroll tick
extern int discFrame;                  // Current disc animation frame (0-3)
extern unsigned long lastDiscUpdate;   // Last disc animation tick
extern int idleDiscX;                  // Idle disc X position for traveling
extern int idleDiscDirection;          // 1 = right, -1 = left
extern unsigned long lastIdleDiscMove; // Last idle disc position update
extern bool nowPlayingActive;          // Is music currently playing?

// ==================== Album Art ====================
#define ALBUM_ART_MAX_WIDTH 64                         // Max width for 16:9 thumbnails
#define ALBUM_ART_MAX_PIXELS (ALBUM_ART_MAX_WIDTH * ALBUM_ART_SIZE)  // 64*18 = 1152 pixels max
extern uint16_t albumArt[ALBUM_ART_MAX_PIXELS];  // RGB565 buffer (2304 bytes max)
extern int albumArtWidth;           // Actual width of current art
extern int albumArtHeight;          // Actual height of current art (always 18)
extern bool albumArtValid;          // True when buffer has valid art

// ==================== PC Stats (Gaming Mode) ====================
extern bool gamingMode;                // Is gaming mode active?
extern int pcCpuTemp;                  // CPU temperature in °C
extern int pcCpuUsage;                 // CPU usage percentage
extern float pcCpuSpeed;               // CPU speed in GHz
extern int pcRamUsed;                  // RAM used in GB
extern int pcRamTotal;                 // RAM total in GB
extern int pcGpuTemp;                  // GPU temperature in °C
extern int pcGpuUsage;                 // GPU usage percentage
extern float pcNetDown;                // Download speed in Mbps
extern float pcNetUp;                  // Upload speed in Mbps
extern unsigned long pcStatsUpdated;   // Last update timestamp

// ==================== Calendar View ====================
extern int calViewMonth;               // Month to display (0-11), -1 = current
extern int calViewYear;                // Year to display, 0 = current

// ==================== Helper Functions ====================
void initState();
void setZoneDirty(Zone zone);
void setAllZonesDirty();
void setAllContentDirty();  // Helper to mark all 3 content zones dirty
void clearZoneDirty(Zone zone);
bool isZoneDirty(Zone zone);

#endif
