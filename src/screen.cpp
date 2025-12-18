#include "screen.h"
#include "state.h"
#include "config.h"
#include "notif_screen.h"
#include "reminder_screen.h"
#include "icons/icons.h"
#include "fonts/MDIOTrial_Regular8pt7b.h"
#include "fonts/MDIOTrial_Bold8pt7b.h"
#include <time.h>

TFT_eSPI tft = TFT_eSPI();

static char previousTimeStr[25] = "                    ";

// ==================== Init ====================
void initScreen() {
  tft.init();
  tft.fillScreen(COLOR_BACKGROUND);
  tft.setRotation(TFT_ROTATION);
  tft.setFreeFont(&MDIOTrial_Regular8pt7b);
  tft.setTextSize(1);

#if DEBUG_SHOW_ZONES
  drawDebugZones();
#endif
}

// ==================== Debug: Zone Boundaries ====================
void drawDebugZones() {
  uint16_t debugColor = TFT_WHITE;

  // Title zone border
  tft.drawRect(ZONE_TITLE_X_START, ZONE_TITLE_Y_START,
               ZONE_TITLE_X_END - ZONE_TITLE_X_START + 1,
               ZONE_TITLE_Y_END - ZONE_TITLE_Y_START + 1, debugColor);

  // Clock zone border
  tft.drawRect(ZONE_CLOCK_X_START, ZONE_CLOCK_Y_START,
               ZONE_CLOCK_X_END - ZONE_CLOCK_X_START + 1,
               ZONE_CLOCK_Y_END - ZONE_CLOCK_Y_START + 1, debugColor);

  // Status zone border
  tft.drawRect(ZONE_STATUS_X_START, ZONE_STATUS_Y_START,
               ZONE_STATUS_X_END - ZONE_STATUS_X_START + 1,
               ZONE_STATUS_Y_END - ZONE_STATUS_Y_START + 1, debugColor);

  // Content zone border
  tft.drawRect(ZONE_CONTENT_X_START, ZONE_CONTENT_Y_START,
               ZONE_CONTENT_X_END - ZONE_CONTENT_X_START + 1,
               ZONE_CONTENT_Y_END - ZONE_CONTENT_Y_START + 1, debugColor);

  // Notification content slots (3 slots of 65px each)
  // Slot 1: Y 45-109
  tft.drawRect(ZONE_CONTENT_X_START, ZONE_NOTIF1_Y_START,
               ZONE_CONTENT_X_END - ZONE_CONTENT_X_START + 1,
               ZONE_NOTIF1_Y_END - ZONE_NOTIF1_Y_START + 1, debugColor);
  // Slot 2: Y 110-174
  tft.drawRect(ZONE_CONTENT_X_START, ZONE_NOTIF2_Y_START,
               ZONE_CONTENT_X_END - ZONE_CONTENT_X_START + 1,
               ZONE_NOTIF2_Y_END - ZONE_NOTIF2_Y_START + 1, debugColor);
  // Slot 3: Y 175-239
  tft.drawRect(ZONE_CONTENT_X_START, ZONE_NOTIF3_Y_START,
               ZONE_CONTENT_X_END - ZONE_CONTENT_X_START + 1,
               ZONE_NOTIF3_Y_END - ZONE_NOTIF3_Y_START + 1, debugColor);
}

// ==================== Zone Helpers ====================
void clearZone(Zone zone) {
  int x_start, x_end, y_start, y_end;

  switch (zone) {
    case ZONE_TITLE:
      x_start = ZONE_TITLE_X_START;
      x_end = ZONE_TITLE_X_END;
      y_start = ZONE_TITLE_Y_START;
      y_end = ZONE_TITLE_Y_END;
      break;
    case ZONE_CLOCK:
      x_start = ZONE_CLOCK_X_START;
      x_end = ZONE_CLOCK_X_END;
      y_start = ZONE_CLOCK_Y_START;
      y_end = ZONE_CLOCK_Y_END;
      break;
    case ZONE_STATUS:
      x_start = ZONE_STATUS_X_START;
      x_end = ZONE_STATUS_X_END;
      y_start = ZONE_STATUS_Y_START;
      y_end = ZONE_STATUS_Y_END;
      break;
    case ZONE_CONTENT:
      x_start = ZONE_CONTENT_X_START;
      x_end = ZONE_CONTENT_X_END;
      y_start = ZONE_CONTENT_Y_START;
      y_end = ZONE_CONTENT_Y_END;
      break;
    default:
      return;
  }

  tft.fillRect(x_start, y_start, x_end - x_start + 1, y_end - y_start + 1, COLOR_BACKGROUND);

#if DEBUG_SHOW_ZONES
  tft.drawRect(x_start, y_start, x_end - x_start + 1, y_end - y_start + 1, TFT_WHITE);

  // Draw internal slot lines if clearing content zone
  if (zone == ZONE_CONTENT) {
    // Slot separators at 109 and 174
    tft.drawFastHLine(x_start, ZONE_NOTIF1_Y_END, x_end - x_start + 1, TFT_WHITE);
    tft.drawFastHLine(x_start, ZONE_NOTIF2_Y_END, x_end - x_start + 1, TFT_WHITE);
  }
#endif
}

// ==================== Title Zone ====================
void drawTitle() {
  tft.setTextSize(1);
  tft.setFreeFont(&MDIOTrial_Bold8pt7b);  // Bold font for header
  tft.setTextColor(COLOR_HEADER);

  const char* title = (currentScreen == SCREEN_NOTIFS) ? "NOTIFS" : "REMINDER";
  // Draw title within title zone (2-120 x 2-24)
  tft.drawString(title, ZONE_TITLE_X_START + 5, ZONE_TITLE_Y_START + 5);

  tft.setFreeFont(&MDIOTrial_Regular8pt7b);  // Reset to regular font
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

  // Clock zone: 141-319 x 0-24 (text position unchanged)
  int x = ZONE_CLOCK_X_START + 7;
  int y = ZONE_CLOCK_Y_START + 5;
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
// ==================== Status Zone (Now Playing / PC Stats) ====================
// Sprites for flicker-free rendering - dimensions calculated from zone boundaries
static const int STATUS_ZONE_W = ZONE_STATUS_X_END - ZONE_STATUS_X_START + 1;  // 317
static const int STATUS_ZONE_H = ZONE_STATUS_Y_END - ZONE_STATUS_Y_START + 1;  // 20
static TFT_eSprite npSprite = TFT_eSprite(&tft);       // Full status zone
static TFT_eSprite textSprite = TFT_eSprite(&tft);     // Text-only zone
static bool npSpriteCreated = false;
static bool npZoneCleared = false;  // One-time zone clear

// Color definitions for PC stats
#define COLOR_CPU    0xFD20   // Orange
#define COLOR_GPU    0xF81F   // Magenta
#define COLOR_RAM    TFT_YELLOW
#define COLOR_NET    TFT_GREEN
#define COLOR_SEP    0x4208   // Dark gray for separators

void drawPcStats() {
  const int zoneX = ZONE_STATUS_X_START;
  const int zoneY = ZONE_STATUS_Y_START;
  const int zoneW = STATUS_ZONE_W;
  const int zoneH = STATUS_ZONE_H;

  // One-time zone clear
  if (!npZoneCleared) {
    tft.fillRect(zoneX, zoneY, zoneW, zoneH, COLOR_BACKGROUND);
    npZoneCleared = true;
  }

  // Create sprite if needed
  if (!npSpriteCreated) {
    npSprite.createSprite(zoneW, zoneH);
    npSprite.setFreeFont(&MDIOTrial_Regular8pt7b);
    textSprite.createSprite(zoneW - 22, zoneH);  // Text area after disc icon
    textSprite.setFreeFont(&MDIOTrial_Regular8pt7b);
    npSpriteCreated = true;
  }

  // Clear sprite
  npSprite.fillSprite(COLOR_BACKGROUND);
  npSprite.setTextSize(1);

  int x = 2;
  int y = 3;

  // Compact format: "65 45% 4.2|72 95%|8G|↓12↑2"
  // CPU stats in orange
  npSprite.setTextColor(COLOR_CPU);
  char cpuStr[16];
  if (pcCpuTemp > 0) {
    snprintf(cpuStr, sizeof(cpuStr), "%d %d%% %.1f", pcCpuTemp, pcCpuUsage, pcCpuSpeed);
  } else {
    snprintf(cpuStr, sizeof(cpuStr), "%d%% %.1f", pcCpuUsage, pcCpuSpeed);
  }
  npSprite.drawString(cpuStr, x, y);
  x += npSprite.textWidth(cpuStr);

  // Separator
  npSprite.setTextColor(COLOR_SEP);
  npSprite.drawString("|", x, y);
  x += npSprite.textWidth("|");

  // GPU stats in magenta
  npSprite.setTextColor(COLOR_GPU);
  char gpuStr[12];
  snprintf(gpuStr, sizeof(gpuStr), "%d %d%%", pcGpuTemp, pcGpuUsage);
  npSprite.drawString(gpuStr, x, y);
  x += npSprite.textWidth(gpuStr);

  // Separator
  npSprite.setTextColor(COLOR_SEP);
  npSprite.drawString("|", x, y);
  x += npSprite.textWidth("|");

  // RAM in yellow
  npSprite.setTextColor(COLOR_RAM);
  char ramStr[6];
  snprintf(ramStr, sizeof(ramStr), "%dG", pcRamUsed);
  npSprite.drawString(ramStr, x, y);
  x += npSprite.textWidth(ramStr);

  // Separator
  npSprite.setTextColor(COLOR_SEP);
  npSprite.drawString("|", x, y);
  x += npSprite.textWidth("|");

  // Network: down arrow + down speed, up arrow + up speed (all in green)
  npSprite.setTextColor(COLOR_NET);

  // Down arrow
  int arrowX = x + 3;
  int arrowY = y + 5;
  npSprite.fillTriangle(arrowX, arrowY + 4, arrowX - 3, arrowY - 2, arrowX + 3, arrowY - 2, COLOR_NET);
  x += 8;

  // Down speed
  char downStr[6];
  if (pcNetDown >= 10) {
    snprintf(downStr, sizeof(downStr), "%.0f", pcNetDown);
  } else {
    snprintf(downStr, sizeof(downStr), "%.1f", pcNetDown);
  }
  npSprite.drawString(downStr, x, y);
  x += npSprite.textWidth(downStr) + 2;

  // Up arrow
  arrowX = x + 3;
  npSprite.fillTriangle(arrowX, arrowY - 4, arrowX - 3, arrowY + 2, arrowX + 3, arrowY + 2, COLOR_NET);
  x += 8;

  // Up speed
  char upStr[6];
  if (pcNetUp >= 10) {
    snprintf(upStr, sizeof(upStr), "%.0f", pcNetUp);
  } else {
    snprintf(upStr, sizeof(upStr), "%.1f", pcNetUp);
  }
  npSprite.drawString(upStr, x, y);

#if DEBUG_SHOW_ZONES
  npSprite.drawRect(0, 0, zoneW, zoneH, TFT_WHITE);
#endif

  // Push to screen at status zone position
  npSprite.pushSprite(ZONE_STATUS_X_START, ZONE_STATUS_Y_START);
}

void drawNowPlaying() {
  // If gaming mode is active, show PC stats instead
  if (gamingMode) {
    drawPcStats();
    return;
  }

  const int zoneX = ZONE_STATUS_X_START;
  const int zoneY = ZONE_STATUS_Y_START;
  const int zoneW = STATUS_ZONE_W;
  const int zoneH = STATUS_ZONE_H;
  const int textZoneX = NOW_PLAYING_TEXT_X;  // 22 (after disc icon)
  const int textZoneW = zoneW - textZoneX;   // Remaining width for text

  // One-time zone clear to remove setup messages (WiFi OK, etc)
  if (!npZoneCleared) {
    tft.fillRect(zoneX, zoneY, zoneW, zoneH, COLOR_BACKGROUND);
    npZoneCleared = true;
  }

  // Create sprites once
  if (!npSpriteCreated) {
    npSprite.createSprite(zoneW, zoneH);
    npSprite.setFreeFont(&MDIOTrial_Regular8pt7b);
    textSprite.createSprite(textZoneW, zoneH);
    textSprite.setFreeFont(&MDIOTrial_Regular8pt7b);
    npSpriteCreated = true;
  }

  // Render everything to sprite first
  npSprite.fillSprite(COLOR_BACKGROUND);

  // Draw disc icon to sprite - always spinning, color depends on state
  // Center disc in left portion of sprite (roughly 11px from left, centered vertically)
  int cx = 11;
  int cy = zoneH / 2;  // Center vertically in zone
  uint16_t discColor = nowPlayingActive ? TFT_WHITE : 0x2104;  // Very dark gray when idle

  npSprite.drawCircle(cx, cy, 7, discColor);
  npSprite.fillCircle(cx, cy, 2, discColor);

  // Always draw spinning triangles (using current discFrame)
  float angle = (discFrame * 5.625) * PI / 180.0;
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

  // Draw scrolling text if active (to separate clipped sprite)
  if (nowPlayingActive && nowPlayingSong.length() > 0) {

    String fullText = nowPlayingSong;
    if (nowPlayingArtist.length() > 0) {
      fullText += " - " + nowPlayingArtist;
    }
    fullText += "    ";  // Gap before repeat

    // Clear text sprite
    textSprite.fillSprite(COLOR_BACKGROUND);

    // Calculate text width in pixels
    int textWidth = textSprite.textWidth(fullText);

    // Wrap scroll position when we've scrolled past the full text
    int scrollPixel = nowPlayingScrollPixel % textWidth;

    // Draw text at offset within text sprite (starting at X=0 in sprite coords)
    int textX = -scrollPixel;
    textSprite.setTextColor(TFT_MAGENTA);
    textSprite.drawString(fullText, textX, 3);

    // Draw second copy for seamless wrap
    if (textX + textWidth < textZoneW) {
      textSprite.drawString(fullText, textX + textWidth, 3);
    }

    // Push text sprite to main sprite at correct position
    textSprite.pushToSprite(&npSprite, textZoneX, 0);
  }

#if DEBUG_SHOW_ZONES
  npSprite.drawRect(0, 0, zoneW, zoneH, TFT_WHITE);
#endif

  // Push to screen atomically at status zone position
  npSprite.pushSprite(zoneX, zoneY);
}

// ==================== Now Playing Ticker Update ====================
void updateNowPlayingTicker() {
  unsigned long now = millis();

  // Always update disc animation (spinning even when not playing)
  if (now - lastDiscUpdate >= NOW_PLAYING_DISC_SPEED) {
    discFrame = (discFrame + 1) % 64;  // 64 frames for ultra-smooth animation
    lastDiscUpdate = now;
    setZoneDirty(ZONE_STATUS);  // Always redraw when disc frame changes
  }

  // If not active, we still need to redraw for disc animation
  if (!nowPlayingActive || nowPlayingSong.length() == 0) {
    return;
  }

  bool needsRedraw = false;

  // Update scroll position (pixel-based)
  if (now - lastScrollUpdate >= NOW_PLAYING_SCROLL_SPEED) {
    nowPlayingScrollPixel += NOW_PLAYING_SCROLL_STEP;
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

  // Clock zone
  if (isZoneDirty(ZONE_CLOCK)) {
    clearZone(ZONE_CLOCK);
    resetPreviousTimeStr();  // Force updateClock to redraw all characters
    clearZoneDirty(ZONE_CLOCK);
  }

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
