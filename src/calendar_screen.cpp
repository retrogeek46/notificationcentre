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

  // Determine which month/year to display
  int displayMonth, displayYear;
  int todayDay = tm.tm_mday;  // Actual current day
  int todayMonth = tm.tm_mon;
  int todayYear = tm.tm_year + 1900;

  if (calViewMonth >= 0 && calViewMonth <= 11) {
    displayMonth = calViewMonth;
  } else {
    displayMonth = todayMonth;
  }

  if (calViewYear > 0) {
    displayYear = calViewYear;
  } else {
    displayYear = todayYear;
  }

  // Check if we're viewing the current month (for highlighting today)
  bool isCurrentMonth = (displayMonth == todayMonth && displayYear == todayYear);

  // Calculate first day of displayed month
  struct tm firstDayTm = {0};
  firstDayTm.tm_year = displayYear - 1900;
  firstDayTm.tm_mon = displayMonth;
  firstDayTm.tm_mday = 1;
  mktime(&firstDayTm);
  int firstDayOfWeek = firstDayTm.tm_wday;
  int startOffset = (firstDayOfWeek == 0) ? 6 : (firstDayOfWeek - 1);
  int daysInMonth = getDaysInMonth(displayMonth, displayYear);

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

  // Render Dates
  canvas.setFreeFont(&MDIOTrial_Regular9pt7b);
  for (int d = 1; d <= daysInMonth; d++) {
    int col = (d + startOffset - 1) % 7;
    int row = (d + startOffset - 1) / 7;

    int x = CAL_X_START + (col * CAL_COL_W) + CAL_TEXT_X_OFFSET;
    int y = lineY + CAL_GRID_Y_OFFSET + (row * CAL_ROW_H) + CAL_TEXT_Y_OFFSET;

    // Highlight today only if viewing current month
    if (isCurrentMonth && d == todayDay) {
      canvas.setTextColor(COLOR_CAL_TODAY_TEXT);
      canvas.fillRoundRect(x + CAL_HL_X_OFF, y + CAL_HL_Y_OFF, CAL_HL_W, CAL_HL_H, CAL_HL_ROUND, COLOR_CAL_TODAY_BG);
      canvas.drawString(String(d), x, y);
    } else {
      canvas.setTextColor(COLOR_CAL_DATE);
      canvas.drawString(String(d), x, y);
    }
  }

  // Draw month/year at the top left
  canvas.setFreeFont(&MDIOTrial_Bold10pt7b);
  canvas.setTextColor(COLOR_CAL_TITLE);
  char monthBuf[32];
  // Use the displayed month/year for the title
  struct tm titleTm = {0};
  titleTm.tm_year = displayYear - 1900;
  titleTm.tm_mon = displayMonth;
  titleTm.tm_mday = 1;
  mktime(&titleTm);
  strftime(monthBuf, sizeof(monthBuf), "%B %Y", &titleTm);
  // Title remains at fixed title pos, adjusted for content offset if in sprite
  int titleY = yOffset + (CAL_TITLE_Y - zoneY);
  canvas.drawString(monthBuf, CAL_TITLE_X, titleY);

  // Push to screen if using sprite
  if (calSpriteCreated) {
    calSprite.pushSprite(0, zoneY);
  }
}
