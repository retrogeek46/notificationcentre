#include "calendar_screen.h"
#include "state.h"
#include "config.h"
#include "screen.h"
#include "types.h"
#include <time.h>
#include "fonts/MDIOTrial_Regular9pt7b.h"
#include "fonts/MDIOTrial_Bold9pt7b.h"
#include "fonts/MDIOTrial_Bold10pt7b.h"

static const char* DAY_NAMES[] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};

// ==================== Helper: Get Days in Month ====================
int getDaysInMonth(int month, int year) {
  if (month == 1) { // February
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) return 29;
    return 28;
  }
  if (month == 3 || month == 5 || month == 8 || month == 10) return 30;
  return 31;
}

// ==================== Helper: Get Sprint Info ====================
// Returns sprint index (relative to anchor) and day index (0-13)
// Anchor: 11-Feb-2026
void getSprintInfo(time_t date, int* sprintIndex, int* dayIndex) {
  static time_t anchorTime = 0;
  if (anchorTime == 0) {
    struct tm anchorTm = {0};
    anchorTm.tm_year = 2026 - 1900;
    anchorTm.tm_mon = 1; // February
    anchorTm.tm_mday = 11;
    anchorTm.tm_hour = 12; anchorTm.tm_min = 0; anchorTm.tm_sec = 0; // Use noon for safety
    anchorTime = mktime(&anchorTm);
  }

  double diffSeconds = difftime(date, anchorTime);
  int diffDays = (int)floor(diffSeconds / 86400.0); // Use floor to handle negatives correctly
  
  if (diffDays >= 0) {
    *sprintIndex = diffDays / 14;
    *dayIndex = diffDays % 14;
  } else {
    // Handle days before anchor
    // e.g. -1 day -> sprint -1, day 13
    // -14 days -> sprint -1, day 0
    // -15 days -> sprint -2, day 13
    
    // Mathematical modulo of negative numbers in C is dependent on implementation (usually truncates towards zero)
    // -1 / 14 = 0, -1 % 14 = -1. We want sprint -1, day 13.
    // We can use a small adjustment
    int adjustedDays = diffDays + 1; // shift so -14 becomes -13
    *sprintIndex = (adjustedDays / 14) - 1;
    int rem = diffDays % 14;
    *dayIndex = (rem < 0) ? (rem + 14) : rem;
  }
}

static TFT_eSprite calSprite = TFT_eSprite(&tft);
static bool calSpriteCreated = false;
static bool calSpriteAttempted = false;

// ==================== Draw Content ====================
void drawCalendarContent() {
  // Use a sprite for the entire content zone (320x195) to eliminate flicker
  static const int calW = 320;
  static const int calH = 195; // 240 - 45 = 195
  const int zoneY = 45;

  if (!calSpriteAttempted) {
    calSpriteAttempted = true;
    calSprite.setColorDepth(8); // Use 8-bit color to save RAM (~62KB)
    void* ptr = calSprite.createSprite(calW, calH);
    if (ptr == nullptr) {
      Serial.println("CRITICAL: Failed to create calendar sprite (even at 8-bit). Falling back to direct drawing.");
      calSpriteCreated = false;
    } else {
      Serial.printf("SUCCESS: Calendar sprite (8-bit) created at %p\n", ptr);
      calSpriteCreated = true;
    }
  }

  // Define drawing surface
  TFT_eSPI &canvas = calSpriteCreated ? (TFT_eSPI &)calSprite : tft;
  int yOffset = calSpriteCreated ? 0 : zoneY;

  if (calSpriteCreated) {
    calSprite.fillSprite(COLOR_BACKGROUND);
  } else {
    tft.fillRect(0, zoneY, calW, calH, COLOR_BACKGROUND);
  }

  time_t now = time(nullptr);
  struct tm tm;
  localtime_r(&now, &tm);

  // Determine Current active Sprint
  int currentSprintIndex = 0;
  int currentSprintDay = 0;
  getSprintInfo(now, &currentSprintIndex, &currentSprintDay);

  // Render Day Headers (Mo Tu We Th...)
  canvas.setFreeFont(&MDIOTrial_Regular9pt7b);
  canvas.setTextColor(COLOR_CAL_DAY_HEADER);
  for (int i = 0; i < 7; i++) {
    int x = CAL_X_START + (i * CAL_COL_W) + CAL_TEXT_X_OFFSET;
    int y = yOffset + CAL_Y_HEADER + CAL_TEXT_Y_OFFSET;
    canvas.drawString(DAY_NAMES[i], x, y);
  }

  // Header separator Y
  int lineY = yOffset + CAL_Y_HEADER + CAL_SEP_Y_OFFSET;

  // --- Floating 5-week window logic ---
  // Today's week is always in row 3 (index 2).
  // Use a noon-based epoch for today to avoid DST edge cases when subtracting
  // whole-day (86400 s) offsets — same pattern used throughout this file.
  struct tm todayTm = tm;  // copy of now's localtime breakdown
  todayTm.tm_hour = 12; todayTm.tm_min = 0; todayTm.tm_sec = 0;
  time_t todayNoon = mktime(&todayTm);
  // Recalculate wday after mktime normalises (should be unchanged, but be safe)
  int todayWday = (todayTm.tm_wday == 0) ? 6 : (todayTm.tm_wday - 1); // Mon=0 … Sun=6
  // Monday of the current week at noon
  time_t mondayOfCurrentWeek = todayNoon - (time_t)todayWday * 86400;
  // Window start = Monday 2 weeks before current week (row 0)
  time_t firstMondayEpoch = mondayOfCurrentWeek - 14 * 86400;

  // Render 5 weeks (5 rows x 7 cols)
  canvas.setFreeFont(&MDIOTrial_Regular9pt7b);
  
  for (int row = 0; row < 5; row++) { // 5 rows for 5 weeks
    for (int col = 0; col < 7; col++) { // 7 days per week
      int cell = (row * 7) + col;

      int x = CAL_X_START + (col * CAL_COL_W) + CAL_TEXT_X_OFFSET;
      int y = lineY + CAL_GRID_Y_OFFSET + (row * CAL_ROW_H) + CAL_TEXT_Y_OFFSET;

      // Calculate the epoch for this specific cell
      time_t cellEpoch = firstMondayEpoch + (cell * 86400);
      struct tm cellTm;
      localtime_r(&cellEpoch, &cellTm);

      int dayNum = cellTm.tm_mday;
      bool isToday = (cellTm.tm_mday == todayTm.tm_mday &&
                      cellTm.tm_mon == todayTm.tm_mon &&
                      cellTm.tm_year == todayTm.tm_year);

      // Determine if the day is in the current month being displayed (for coloring)
      // The "displayed month" is effectively the month of the current day, but we need to check each cell's month
      bool isCurrentMonthDay = (cellTm.tm_mon == todayTm.tm_mon && cellTm.tm_year == todayTm.tm_year);

      if (isToday) {
        canvas.setTextColor(COLOR_CAL_TODAY_TEXT);
        canvas.fillRoundRect(x + CAL_HL_X_OFF, y + CAL_HL_Y_OFF, CAL_HL_W, CAL_HL_H, CAL_HL_ROUND, COLOR_CAL_TODAY_BG);
      } else if (isCurrentMonthDay) {
        canvas.setTextColor(COLOR_CAL_DATE);
      } else {
        canvas.setTextColor(COLOR_CAL_ADJACENT);
      }

      // Check for sprint markers
      int cellSprintIndex = 0;
      int cellSprintDay = 0;
      getSprintInfo(cellEpoch, &cellSprintIndex, &cellSprintDay);
      
      // Only show markers if this cell belongs to the CURRENT active sprint
      if (cellSprintIndex == currentSprintIndex) {
        bool isSprintStart = (cellSprintDay == 0);
        bool isSprintEnd = (cellSprintDay == 13);

        if (isSprintStart || isSprintEnd) {
           int by = y + CAL_HL_Y_OFF;
           int bh = CAL_HL_H;
           int r = CAL_HL_ROUND;
           int t = 3;   // Stroke thickness
           int bw = 8;  // Bracket width

           if (isSprintStart) {
             // [ bracket: flush on the left edge of highlight box
             int bx = x + CAL_HL_X_OFF - bw + 1; // overlap 1px for flush contact
             canvas.fillRoundRect(bx, by, bw, bh, r, COLOR_CAL_SPRINT);
             // Square off the right-side corners (open end) so arms are straight
             canvas.fillRect(bx + bw - r, by, r, t, COLOR_CAL_SPRINT);
             canvas.fillRect(bx + bw - r, by + bh - t, r, t, COLOR_CAL_SPRINT);
             // Inner cutout
             canvas.fillRoundRect(bx + t, by + t, bw - t, bh - 2 * t, max(r - t, 1), COLOR_BACKGROUND);
           }
           if (isSprintEnd) {
             // ] bracket: flush on the right edge of highlight box
             int bx = x + CAL_HL_X_OFF + CAL_HL_W - 1; // overlap 1px for flush contact
             canvas.fillRoundRect(bx, by, bw, bh, r, COLOR_CAL_SPRINT);
             // Square off the left-side corners (open end) so arms are straight
             canvas.fillRect(bx, by, r, t, COLOR_CAL_SPRINT);
             canvas.fillRect(bx, by + bh - t, r, t, COLOR_CAL_SPRINT);
             // Inner cutout
             canvas.fillRoundRect(bx, by + t, bw - t, bh - 2 * t, max(r - t, 1), COLOR_BACKGROUND);
           }
        }
      }

      canvas.drawString(String(dayNum), x, y);
    }
  }

  // Draw month/year at the top left (this will be the month of 'today')
  canvas.setFreeFont(&MDIOTrial_Bold10pt7b);
  canvas.setTextColor(COLOR_CAL_TITLE);
  char monthBuf[32];
  strftime(monthBuf, sizeof(monthBuf), "%B %Y", &todayTm);
  // Title remains at fixed title pos, adjusted for content offset if in sprite
  int titleY = yOffset + (CAL_TITLE_Y - zoneY);
  canvas.drawString(monthBuf, CAL_TITLE_X, titleY);

  // Push to screen if using sprite
  if (calSpriteCreated) {
    calSprite.pushSprite(0, zoneY);
  }

#if DEBUG_NO_BG_CONTENT_ZONE
  // Draw debug border for content zone
  tft.drawRect(0, zoneY, calW, calH, TFT_WHITE);
#endif
}
