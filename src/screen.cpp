#include "screen.h"
#include "state.h"
#include "config.h"
#include "notif_screen.h"
#include "reminder_screen.h"
#include <time.h>

TFT_eSPI tft = TFT_eSPI();

static char previousTimeStr[25] = "                    ";

// ==================== Constants for Now Playing ====================
const unsigned long NOW_PLAYING_TIMEOUT = 300000; // 5 minutes

// ==================== Init ====================
void initScreen() {
  tft.init();
  tft.fillScreen(COLOR_BACKGROUND);
  tft.setRotation(3);
  tft.setFreeFont(&FreeMono9pt7b);
  tft.setTextSize(1);
}

// ==================== Zone Helpers ====================
void clearZone(Zone zone) {
  int y_start, y_end;
  int x_start = 0;
  int width = 320;
  
  switch (zone) {
    case ZONE_TITLE:
      y_start = ZONE_TITLE_Y_START;
      y_end = ZONE_TITLE_Y_END;
      width = 95;  // Left portion for title
      break;
    case ZONE_CLOCK:
      y_start = ZONE_CLOCK_Y_START;
      y_end = ZONE_CLOCK_Y_END;
      x_start = 100;  // Right portion for clock
      width = 220;
      break;
    case ZONE_STATUS:
      y_start = ZONE_STATUS_Y_START;
      y_end = ZONE_STATUS_Y_END;
      break;
    case ZONE_CONTENT:
      y_start = ZONE_CONTENT_Y_START;
      y_end = ZONE_CONTENT_Y_END;
      break;
    default:
      return;
  }
  
  tft.fillRect(x_start, y_start, width, y_end - y_start + 1, COLOR_BACKGROUND);
}

// ==================== Title Zone ====================
void drawTitle() {
  tft.setTextSize(1);
  tft.setTextColor(COLOR_HEADER);
  
  const char* title = (currentScreen == SCREEN_NOTIFS) ? "NOTIFS" : "REMINDER";
  tft.drawString(title, 5, 5);
}

// ==================== Clock Zone ====================
static void resetPreviousTimeStr() {
  memset(previousTimeStr, 0, sizeof(previousTimeStr));
}

void updateClock() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  char timeStr[25];
  strftime(timeStr, sizeof(timeStr), "%a,%d-%b,%H:%M:%S", &timeinfo);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_CLOCK);

  int x = 100, y = 5;
  int charHeight = tft.fontHeight();
  int maxCharWidth = tft.textWidth("W");
  int cursorX = x;

  // Partial update - only redraw changed characters
  for (size_t i = 0; i < strlen(timeStr); i++) {
    if (timeStr[i] != previousTimeStr[i] || previousTimeStr[i] == '\0') {
      tft.fillRect(cursorX, y, maxCharWidth, charHeight, COLOR_BACKGROUND);
      char ch[2] = {timeStr[i], '\0'};
      tft.drawString(ch, cursorX, y);
    }
    previousTimeStr[i] = timeStr[i];
    cursorX += maxCharWidth;
  }
}

// ==================== Status Zone (Now Playing) ====================
void drawNowPlaying() {
  // Check timeout
  if (nowPlayingSong.length() == 0 || (millis() - nowPlayingUpdated > NOW_PLAYING_TIMEOUT)) {
    return;
  }
  
  tft.setTextColor(TFT_MAGENTA);
  String display = nowPlayingSong;
  if (nowPlayingArtist.length() > 0) {
    display += " - " + nowPlayingArtist;
  }
  
  // Truncate if too long
  if (display.length() > 30) {
    display = display.substring(0, 30) + "...";
  }
  
  tft.drawString(display, 5, 22);
}

// ==================== Main Refresh ====================
void refreshScreen() {
  // Title zone
  if (isZoneDirty(ZONE_TITLE)) {
    clearZone(ZONE_TITLE);
    drawTitle();
    resetPreviousTimeStr();  // Force clock redraw after title change
    clearZoneDirty(ZONE_TITLE);
  }
  
  // Clock is handled separately by updateClock() for partial updates
  
  // Status zone (Now Playing)
  if (isZoneDirty(ZONE_STATUS)) {
    clearZone(ZONE_STATUS);
    drawNowPlaying();
    clearZoneDirty(ZONE_STATUS);
  }
  
  // Content zone
  if (isZoneDirty(ZONE_CONTENT)) {
    clearZone(ZONE_CONTENT);
    if (currentScreen == SCREEN_NOTIFS) {
      drawNotifContent();
    } else {
      drawReminderContent();
    }
    clearZoneDirty(ZONE_CONTENT);
  }
}
