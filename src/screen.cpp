#include "screen.h"
#include "state.h"
#include "config.h"
#include "notif_screen.h"
#include "reminder_screen.h"
#include "icons/icons.h"
#include "fonts/MDIOTrial_Regular8pt7b.h"
#include "fonts/MDIOTrial_Regular9pt7b.h"
#include "fonts/MDIOTrial_Bold8pt7b.h"
#include "fonts/MDIOTrial_Bold9pt7b.h"
#include "sprites/sprite_title.h"
#include "sprites/sprite_clock.h"
#include "sprites/sprite_status.h"
#include "sprites/sprite_content1.h"
#include "sprites/sprite_content2.h"
#include "sprites/sprite_content3.h"
#include <time.h>


TFT_eSPI tft = TFT_eSPI();

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

  // Content zone borders (3 slots)
  tft.drawRect(ZONE_CONTENT1_X_START, ZONE_CONTENT1_Y_START,
               ZONE_CONTENT1_X_END - ZONE_CONTENT1_X_START + 1,
               ZONE_CONTENT1_Y_END - ZONE_CONTENT1_Y_START + 1, debugColor);
  tft.drawRect(ZONE_CONTENT2_X_START, ZONE_CONTENT2_Y_START,
               ZONE_CONTENT2_X_END - ZONE_CONTENT2_X_START + 1,
               ZONE_CONTENT2_Y_END - ZONE_CONTENT2_Y_START + 1, debugColor);
  tft.drawRect(ZONE_CONTENT3_X_START, ZONE_CONTENT3_Y_START,
               ZONE_CONTENT3_X_END - ZONE_CONTENT3_X_START + 1,
               ZONE_CONTENT3_Y_END - ZONE_CONTENT3_Y_START + 1, debugColor);
}

// ==================== Zone Helpers ====================
void clearZone(Zone zone) {
  switch (zone) {
    case ZONE_TITLE:
      // Title sprite is drawn by drawTitle() - just mark dirty
      tft.pushImage(ZONE_TITLE_X_START, ZONE_TITLE_Y_START,
                    SPRITE_TITLE_WIDTH, SPRITE_TITLE_HEIGHT, SPRITE_TITLE);
      break;
    case ZONE_CLOCK:
      // Clock sprite is drawn by updateClock() - just mark dirty
      tft.pushImage(ZONE_CLOCK_X_START, ZONE_CLOCK_Y_START,
                    SPRITE_CLOCK_WIDTH, SPRITE_CLOCK_HEIGHT, SPRITE_CLOCK);
      break;
    case ZONE_STATUS:
      // Status sprite is drawn by drawNowPlaying/drawPcStats
      tft.pushImage(ZONE_STATUS_X_START, ZONE_STATUS_Y_START,
                    SPRITE_STATUS_WIDTH, SPRITE_STATUS_HEIGHT, SPRITE_STATUS);
      break;
    case ZONE_CONTENT1:
      tft.pushImage(ZONE_CONTENT1_X_START, ZONE_CONTENT1_Y_START,
                    SPRITE_CONTENT1_WIDTH, SPRITE_CONTENT1_HEIGHT, SPRITE_CONTENT1);
      break;
    case ZONE_CONTENT2:
      tft.pushImage(ZONE_CONTENT2_X_START, ZONE_CONTENT2_Y_START,
                    SPRITE_CONTENT2_WIDTH, SPRITE_CONTENT2_HEIGHT, SPRITE_CONTENT2);
      break;
    case ZONE_CONTENT3:
      tft.pushImage(ZONE_CONTENT3_X_START, ZONE_CONTENT3_Y_START,
                    SPRITE_CONTENT3_WIDTH, SPRITE_CONTENT3_HEIGHT, SPRITE_CONTENT3);
      break;
    default:
      return;
  }

#if DEBUG_SHOW_ZONES
  // Draw debug borders on top
  int x_start, x_end, y_start, y_end;
  switch (zone) {
    case ZONE_TITLE:
      x_start = ZONE_TITLE_X_START; x_end = ZONE_TITLE_X_END;
      y_start = ZONE_TITLE_Y_START; y_end = ZONE_TITLE_Y_END;
      break;
    case ZONE_CLOCK:
      x_start = ZONE_CLOCK_X_START; x_end = ZONE_CLOCK_X_END;
      y_start = ZONE_CLOCK_Y_START; y_end = ZONE_CLOCK_Y_END;
      break;
    case ZONE_STATUS:
      x_start = ZONE_STATUS_X_START; x_end = ZONE_STATUS_X_END;
      y_start = ZONE_STATUS_Y_START; y_end = ZONE_STATUS_Y_END;
      break;
    case ZONE_CONTENT1:
      x_start = ZONE_CONTENT1_X_START; x_end = ZONE_CONTENT1_X_END;
      y_start = ZONE_CONTENT1_Y_START; y_end = ZONE_CONTENT1_Y_END;
      break;
    case ZONE_CONTENT2:
      x_start = ZONE_CONTENT2_X_START; x_end = ZONE_CONTENT2_X_END;
      y_start = ZONE_CONTENT2_Y_START; y_end = ZONE_CONTENT2_Y_END;
      break;
    case ZONE_CONTENT3:
      x_start = ZONE_CONTENT3_X_START; x_end = ZONE_CONTENT3_X_END;
      y_start = ZONE_CONTENT3_Y_START; y_end = ZONE_CONTENT3_Y_END;
      break;
    default:
      return;
  }
  tft.drawRect(x_start, y_start, x_end - x_start + 1, y_end - y_start + 1, TFT_WHITE);
#endif
}

// ==================== Zone Sprite Helper ====================
/**
 * Prepare a sprite with background - either sprite image or solid fill
 * @param sprite    - TFT_eSprite to render into (must already be created)
 * @param bgSprite  - PROGMEM sprite array
 * @param bgW, bgH  - Sprite dimensions
 */
void prepareZoneSprite(TFT_eSprite& sprite, const uint16_t* bgSprite, int bgW, int bgH) {
#if SPRITE_BG_ENABLED
  sprite.pushImage(0, 0, bgW, bgH, bgSprite);
#else
  sprite.fillSprite(COLOR_BACKGROUND);
#endif
}

// ==================== Title Zone ====================
static TFT_eSprite titleSprite = TFT_eSprite(&tft);
static bool titleSpriteCreated = false;

void drawTitle() {
  // Create sprite once
  if (!titleSpriteCreated) {
    titleSprite.createSprite(SPRITE_TITLE_WIDTH, SPRITE_TITLE_HEIGHT);
    titleSprite.setFreeFont(&MDIOTrial_Bold9pt7b);
    titleSpriteCreated = true;
  }

  // Prepare background (sprite or solid fill based on SPRITE_BG_ENABLED)
  prepareZoneSprite(titleSprite, SPRITE_TITLE, SPRITE_TITLE_WIDTH, SPRITE_TITLE_HEIGHT);

  // Overlay text
  titleSprite.setTextSize(1);
  titleSprite.setTextColor(COLOR_HEADER);
  const char* title = (currentScreen == SCREEN_NOTIFS) ? "NOTIFS" : "REMINDER";
  titleSprite.drawString(title, 5, 5);

  // Push to screen
  titleSprite.pushSprite(ZONE_TITLE_X_START, ZONE_TITLE_Y_START);
}

// ==================== Clock Zone ====================
static TFT_eSprite clockSprite = TFT_eSprite(&tft);
static bool clockSpriteCreated = false;
static char previousTimeStr[25] = "";

static void resetPreviousTimeStr() {
  memset(previousTimeStr, 0, sizeof(previousTimeStr));
}

void updateClock() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  char timeStr[25];
  strftime(timeStr, sizeof(timeStr), "%a,%d-%b,%H:%M:%S", &timeinfo);

  // Skip if nothing changed
  if (strcmp(timeStr, previousTimeStr) == 0) {
    return;
  }
  strcpy(previousTimeStr, timeStr);

  // Create sprite once
  if (!clockSpriteCreated) {
    clockSprite.createSprite(SPRITE_CLOCK_WIDTH, SPRITE_CLOCK_HEIGHT);
    clockSprite.setFreeFont(&MDIOTrial_Regular9pt7b);
    clockSpriteCreated = true;
  }

  // Prepare background (sprite or solid fill based on SPRITE_BG_ENABLED)
  prepareZoneSprite(clockSprite, SPRITE_CLOCK, SPRITE_CLOCK_WIDTH, SPRITE_CLOCK_HEIGHT);

  // Overlay text
  clockSprite.setTextSize(1);
  clockSprite.setTextColor(COLOR_CLOCK);
  clockSprite.drawString(timeStr, 8, 5);

  // Push to screen
  clockSprite.pushSprite(ZONE_CLOCK_X_START, ZONE_CLOCK_Y_START);
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
    npSprite.setFreeFont(&MDIOTrial_Regular9pt7b);
    textSprite.createSprite(zoneW - 22, zoneH);  // Text area after disc icon
    textSprite.setFreeFont(&MDIOTrial_Regular9pt7b);
    npSpriteCreated = true;
  }

  // Prepare background (sprite or solid fill based on SPRITE_BG_ENABLED)
  prepareZoneSprite(npSprite, SPRITE_STATUS, SPRITE_STATUS_WIDTH, SPRITE_STATUS_HEIGHT);
  npSprite.setTextSize(1);

  int x = 5;
  int y = 3;

  bool flashOn = (millis() / 300) % 2 == 0;

  // Compact format: "65c 45% 4.2GHz | 72c 95% | 8G | ↓12↑2"
  // CPU stats - flash red if overheating, blue if sensor error (0)
  bool cpuOverheat = (pcCpuTemp > CPU_TEMP_WARN);
  bool cpuSensorError = (pcCpuTemp == 0);

  // Draw CPU temp - always show, flash blue if 0 (sensor error), red if overheating
  uint16_t cpuTempColor = COLOR_CPU;
  if (cpuSensorError && flashOn) {
    cpuTempColor = TFT_BLUE;  // Flash blue for sensor error
  } else if (cpuOverheat && flashOn) {
    cpuTempColor = TFT_RED;   // Flash red for overheating
  }
  npSprite.setTextColor(cpuTempColor);
  char cpuTempStr[8];
  snprintf(cpuTempStr, sizeof(cpuTempStr), "%dc ", pcCpuTemp);
  npSprite.drawString(cpuTempStr, x, y);
  x += npSprite.textWidth(cpuTempStr);

  // Draw CPU usage and speed (always orange)
  npSprite.setTextColor(COLOR_CPU);
  char cpuRestStr[16];
  snprintf(cpuRestStr, sizeof(cpuRestStr), " %d%% %.1fGHz", pcCpuUsage, pcCpuSpeed);
  npSprite.drawString(cpuRestStr, x, y);
  x += npSprite.textWidth(cpuRestStr);

  // Separator after CPU
  npSprite.setTextColor(COLOR_SEP);
  npSprite.drawString("| ", x, y);
  x += npSprite.textWidth("| ");

  // RAM as pie chart (moved before GPU)
  int ramCx = x + 7;
  int ramCy = zoneH / 2;
  int ramRadius = 6;
  float ramPercent = (pcRamTotal > 0) ? (float)pcRamUsed / pcRamTotal : 0;
  int ramAngle = (int)(ramPercent * 360);

  // Draw empty circle
  npSprite.drawCircle(ramCx, ramCy, ramRadius, COLOR_RAM);
  // Fill pie segment (0 degrees = top, clockwise)
  if (ramAngle > 0) {
    for (int a = -90; a < -90 + ramAngle; a++) {
      float rad = a * PI / 180.0;
      int px = ramCx + (int)(ramRadius * cos(rad));
      int py = ramCy + (int)(ramRadius * sin(rad));
      npSprite.drawLine(ramCx, ramCy, px, py, COLOR_RAM);
    }
  }
  x += 16;  // Pie chart width

  // Separator after RAM
  npSprite.setTextColor(COLOR_SEP);
  npSprite.drawString("|", x, y);
  x += npSprite.textWidth("|");

  // GPU stats - only flash temp red, keep usage magenta
  bool gpuOverheat = (pcGpuTemp > GPU_TEMP_WARN);

  // Draw GPU temp (flashing if overheating)
  uint16_t gpuTempColor = (gpuOverheat && flashOn) ? TFT_RED : COLOR_GPU;
  npSprite.setTextColor(gpuTempColor);
  char gpuTempStr[8];
  snprintf(gpuTempStr, sizeof(gpuTempStr), "%dc ", pcGpuTemp);
  npSprite.drawString(gpuTempStr, x, y);
  x += npSprite.textWidth(gpuTempStr);

  // Draw GPU usage (always magenta)
  npSprite.setTextColor(COLOR_GPU);
  char gpuUsageStr[8];
  snprintf(gpuUsageStr, sizeof(gpuUsageStr), "%d%%", pcGpuUsage);
  npSprite.drawString(gpuUsageStr, x, y);
  x += npSprite.textWidth(gpuUsageStr);

  // Separator after GPU
  npSprite.setTextColor(COLOR_SEP);
  npSprite.drawString("|", x, y);
  x += npSprite.textWidth("|");

  // Network: Download (green), Upload (cyan) with M suffix
  npSprite.setTextColor(COLOR_NET);
  char downStr[8];
  if (pcNetDown >= 10) {
    snprintf(downStr, sizeof(downStr), "%.0fM", pcNetDown);
  } else {
    snprintf(downStr, sizeof(downStr), "%.1fM", pcNetDown);
  }
  npSprite.drawString(downStr, x, y);
  x += npSprite.textWidth(downStr);

  npSprite.setTextColor(TFT_CYAN);
  char upStr[8];
  if (pcNetUp >= 10) {
    snprintf(upStr, sizeof(upStr), "%.0fM", pcNetUp);
  } else {
    snprintf(upStr, sizeof(upStr), "%.1fM", pcNetUp);
  }
  npSprite.drawString(upStr, x, y);

#if DEBUG_SHOW_ZONES
  npSprite.drawRect(0, 0, zoneW, zoneH, TFT_WHITE);
#endif

  // Push to screen at status zone position
  npSprite.pushSprite(ZONE_STATUS_X_START, ZONE_STATUS_Y_START);
}

void drawNowPlaying() {
  // Check if PC stats are stale (PC went to sleep)
  bool pcStatsStale = (millis() - pcStatsUpdated) > PC_STATS_TIMEOUT;

  // Priority: Music → Media, Active Stats → Stats, Stale/Idle → Idle Disc
  // 1. If music is playing, show Now Playing
  // 2. If PC stats are fresh, show stats
  // 3. If no music and stats are stale, show idle spinning disc
  if (nowPlayingActive) {
    // Continue to draw now playing below
  } else if (!pcStatsStale) {
    drawPcStats();
    return;
  }
  // If we get here with stale stats and no music, show idle disc

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
    npSprite.setFreeFont(&MDIOTrial_Regular9pt7b);
    textSprite.createSprite(textZoneW, zoneH);
    textSprite.setFreeFont(&MDIOTrial_Regular9pt7b);
    npSpriteCreated = true;
  }

  // Prepare background (sprite or solid fill based on SPRITE_BG_ENABLED)
  prepareZoneSprite(npSprite, SPRITE_STATUS, SPRITE_STATUS_WIDTH, SPRITE_STATUS_HEIGHT);

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

  // Draw scrolling text if active (directly to npSprite with clipping)
  if (nowPlayingActive && nowPlayingSong.length() > 0) {

    String fullText = nowPlayingSong;
    if (nowPlayingArtist.length() > 0) {
      fullText += " - " + nowPlayingArtist;
    }
    fullText += "    ";  // Gap before repeat

    // Calculate text width in pixels
    int textWidth = npSprite.textWidth(fullText);

    // Wrap scroll position when we've scrolled past the full text
    int scrollPixel = nowPlayingScrollPixel % textWidth;

    // Set viewport to clip text to the text zone (after disc icon)
    npSprite.setViewport(textZoneX, 0, textZoneW, zoneH);

    // Draw text at offset (coordinates are now relative to viewport)
    int textX = -scrollPixel;
    npSprite.setTextColor(TFT_MAGENTA);
    npSprite.drawString(fullText, textX, 3);

    // Draw second copy for seamless wrap
    if (textX + textWidth < textZoneW) {
      npSprite.drawString(fullText, textX + textWidth, 3);
    }

    // Reset viewport to full sprite
    npSprite.resetViewport();
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

  // Content zones (check all 3)
  bool anyContentDirty = isZoneDirty(ZONE_CONTENT1) || isZoneDirty(ZONE_CONTENT2) || isZoneDirty(ZONE_CONTENT3);
  if (anyContentDirty) {
    // Clear all dirty content zones
    if (isZoneDirty(ZONE_CONTENT1)) clearZone(ZONE_CONTENT1);
    if (isZoneDirty(ZONE_CONTENT2)) clearZone(ZONE_CONTENT2);
    if (isZoneDirty(ZONE_CONTENT3)) clearZone(ZONE_CONTENT3);
    
    // Draw content
    if (currentScreen == SCREEN_NOTIFS) {
      drawNotifContent();
    } else {
      drawReminderContent();
    }
    
    // Clear all dirty flags
    clearZoneDirty(ZONE_CONTENT1);
    clearZoneDirty(ZONE_CONTENT2);
    clearZoneDirty(ZONE_CONTENT3);
  }
}
