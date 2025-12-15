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
// Store previous text for erase-by-redraw technique
static String prevNowPlayingText = "";
static bool npFirstDraw = true;

void drawNowPlaying() {
  const int textX = NOW_PLAYING_TEXT_X;
  const int textY = 22;

  // Always draw disc icon directly (it's small and fast)
  drawDiscIcon(3, 20, discFrame, nowPlayingActive);

  // If not active or no song, erase previous text and clear state
  if (!nowPlayingActive || nowPlayingSong.length() == 0) {
    if (prevNowPlayingText.length() > 0) {
      // Erase by drawing previous text in black
      tft.setTextColor(COLOR_BACKGROUND);
      tft.drawString(prevNowPlayingText, textX, textY);
      prevNowPlayingText = "";
    }
    npFirstDraw = true;
    return;
  }

  // Check timeout
  if (millis() - nowPlayingUpdated > NOW_PLAYING_TIMEOUT) {
    nowPlayingActive = false;
    if (prevNowPlayingText.length() > 0) {
      tft.setTextColor(COLOR_BACKGROUND);
      tft.drawString(prevNowPlayingText, textX, textY);
      prevNowPlayingText = "";
    }
    npFirstDraw = true;
    return;
  }

  // Build full display string
  String fullText = nowPlayingSong;
  if (nowPlayingArtist.length() > 0) {
    fullText += " - " + nowPlayingArtist;
  }
  fullText += "    ";

  // Calculate visible portion
  int textLen = fullText.length();
  int scrollPos = nowPlayingScrollPos % textLen;

  // Create visible substring
  String visible = "";
  int charsNeeded = 26;
  for (int i = 0; i < charsNeeded; i++) {
    int idx = (scrollPos + i) % textLen;
    visible += fullText.charAt(idx);
  }

  // Text-erase technique:
  // 1. Erase previous text by drawing it in black (only if different or first draw)
  if (prevNowPlayingText != visible || npFirstDraw) {
    if (prevNowPlayingText.length() > 0 && !npFirstDraw) {
      tft.setTextColor(COLOR_BACKGROUND);
      tft.drawString(prevNowPlayingText, textX, textY);
    }

    // 2. Draw new text in color
    tft.setTextColor(TFT_MAGENTA);
    tft.drawString(visible, textX, textY);

    // 3. Remember for next frame
    prevNowPlayingText = visible;
    npFirstDraw = false;
  }
}

// ==================== Now Playing Ticker Update ====================
void updateNowPlayingTicker() {
  unsigned long now = millis();

  // Always update disc animation (even when not playing, for smooth transitions)
  static bool lastDiscNeedsRedraw = false;
  bool discNeedsRedraw = false;

  if (now - lastDiscUpdate >= NOW_PLAYING_DISC_SPEED) {
    if (nowPlayingActive) {
      discFrame = (discFrame + 1) % 8;  // 8 frames for smoother animation
      discNeedsRedraw = true;
    }
    lastDiscUpdate = now;
  }

  // If not active, only redraw disc if state changed
  if (!nowPlayingActive || nowPlayingSong.length() == 0) {
    if (discNeedsRedraw != lastDiscNeedsRedraw) {
      setZoneDirty(ZONE_STATUS);
      lastDiscNeedsRedraw = discNeedsRedraw;
    }
    return;
  }

  // Check for timeout
  if (millis() - nowPlayingUpdated > NOW_PLAYING_TIMEOUT) {
    nowPlayingActive = false;
    setZoneDirty(ZONE_STATUS);
    return;
  }

  bool needsRedraw = discNeedsRedraw;

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
