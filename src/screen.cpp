#include "screen.h"
#include "state.h"
#include "config.h"
#include "notif_screen.h"
#include "reminder_screen.h"
#include "icons/icons.h"
#include <time.h>

TFT_eSPI tft = TFT_eSPI();

static char previousTimeStr[25] = "                    ";

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
// Sprite for flicker-free rendering
static TFT_eSprite npSprite = TFT_eSprite(&tft);
static bool npSpriteCreated = false;
static bool npZoneCleared = false;  // One-time zone clear

void drawNowPlaying() {
  const int zoneY = 19;
  const int zoneH = 18;

  // One-time zone clear to remove setup messages (WiFi OK, etc)
  if (!npZoneCleared) {
    tft.fillRect(0, zoneY, 320, zoneH, COLOR_BACKGROUND);
    npZoneCleared = true;
  }

  // Create sprite once (320x18 covers entire status zone)
  if (!npSpriteCreated) {
    npSprite.createSprite(320, zoneH);
    npSprite.setFreeFont(&FreeMono9pt7b);
    npSpriteCreated = true;
  }

  // Render everything to sprite first
  npSprite.fillSprite(COLOR_BACKGROUND);

  // Draw disc icon to sprite - always spinning, color depends on state
  int cx = 11;  // 3 + 8
  int cy = 9;   // 1 + 8
  uint16_t discColor = nowPlayingActive ? TFT_WHITE : 0x2104;  // Very dark gray when idle

  npSprite.drawCircle(cx, cy, 7, discColor);
  npSprite.fillCircle(cx, cy, 2, discColor);

  // Always draw spinning triangles (using current discFrame)
  float angle = (discFrame * 45.0) * PI / 180.0;
  int t1x1 = cx + (int)(3 * cos(angle));
  int t1y1 = cy + (int)(3 * sin(angle));
  int t1x2 = cx + (int)(6 * cos(angle - 0.4));
  int t1y2 = cy + (int)(6 * sin(angle - 0.4));
  int t1x3 = cx + (int)(6 * cos(angle + 0.4));
  int t1y3 = cy + (int)(6 * sin(angle + 0.4));
  npSprite.fillTriangle(t1x1, t1y1, t1x2, t1y2, t1x3, t1y3, discColor);

  float angle2 = angle + PI;
  int t2x1 = cx + (int)(3 * cos(angle2));
  int t2y1 = cy + (int)(3 * sin(angle2));
  int t2x2 = cx + (int)(6 * cos(angle2 - 0.4));
  int t2y2 = cy + (int)(6 * sin(angle2 - 0.4));
  int t2x3 = cx + (int)(6 * cos(angle2 + 0.4));
  int t2y3 = cy + (int)(6 * sin(angle2 + 0.4));
  npSprite.fillTriangle(t2x1, t2y1, t2x2, t2y2, t2x3, t2y3, discColor);

  // Draw scrolling text if active
  if (nowPlayingActive && nowPlayingSong.length() > 0 &&
      millis() - nowPlayingUpdated <= NOW_PLAYING_TIMEOUT) {

    String fullText = nowPlayingSong;
    if (nowPlayingArtist.length() > 0) {
      fullText += " - " + nowPlayingArtist;
    }
    fullText += "    ";

    int textLen = fullText.length();
    int scrollPos = nowPlayingScrollPos % textLen;

    String visible = "";
    for (int i = 0; i < 26; i++) {
      visible += fullText.charAt((scrollPos + i) % textLen);
    }

    npSprite.setTextColor(TFT_MAGENTA);
    npSprite.drawString(visible, 22, 3);
  } else if (millis() - nowPlayingUpdated > NOW_PLAYING_TIMEOUT) {
    nowPlayingActive = false;
  }

  // Push to screen atomically
  npSprite.pushSprite(0, zoneY);
}

// ==================== Now Playing Ticker Update ====================
void updateNowPlayingTicker() {
  unsigned long now = millis();

  // Always update disc animation (spinning even when not playing)
  if (now - lastDiscUpdate >= NOW_PLAYING_DISC_SPEED) {
    discFrame = (discFrame + 1) % 8;  // 8 frames for smoother animation
    lastDiscUpdate = now;
    setZoneDirty(ZONE_STATUS);  // Always redraw when disc frame changes
  }

  // If not active, we still need to redraw for disc animation
  if (!nowPlayingActive || nowPlayingSong.length() == 0) {
    return;
  }

  // Check for timeout
  if (millis() - nowPlayingUpdated > NOW_PLAYING_TIMEOUT) {
    nowPlayingActive = false;
    setZoneDirty(ZONE_STATUS);
    return;
  }

  bool needsRedraw = false;

  // Update scroll position
  if (now - lastScrollUpdate >= NOW_PLAYING_SCROLL_SPEED) {
    nowPlayingScrollPos++;
    lastScrollUpdate = now;
    needsRedraw = true;
  }

  if (needsRedraw) {
    setZoneDirty(ZONE_STATUS);
  }
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

  // Status zone (Now Playing) - no clearZone, drawNowPlaying handles its own updates
  if (isZoneDirty(ZONE_STATUS)) {
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
