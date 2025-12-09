#include "icons.h"

// ==================== Slack Icon ====================
void drawSlackIcon(int x, int y) {
  static uint16_t lineBuf[ICON_WIDTH];
  const uint8_t* p = slack_icon_map;
  
  for (int row = 0; row < ICON_HEIGHT; row++) {
    for (int col = 0; col < ICON_WIDTH; col++) {
      uint8_t lo = pgm_read_byte(p++);
      uint8_t hi = pgm_read_byte(p++);
      uint16_t rgb565 = (lo << 8) | hi;
      lineBuf[col] = rgb565;
    }
    tft.pushImage(x, y + row, ICON_WIDTH, 1, lineBuf);
  }
}

// ==================== App Icon Dispatcher ====================
void drawAppIcon(int x, int y, String app) {
  app.toLowerCase();
  
  if (app.indexOf("slack") >= 0) {
    drawSlackIcon(x, y);
  }
  else if (app.indexOf("whatsapp") >= 0) {
    tft.fillCircle(x + 8, y + 8, 7, COLOR_WHATSAPP);
    tft.drawCircle(x + 8, y + 8, 7, COLOR_ICON_BORDER);
  }
  else if (app.indexOf("telegram") >= 0) {
    tft.fillCircle(x + 8, y + 8, 7, COLOR_TELEGRAM);
    tft.drawCircle(x + 8, y + 8, 7, COLOR_ICON_BORDER);
  }
  else {
    // Default icon
    tft.fillRect(x, y, ICON_WIDTH, ICON_HEIGHT, COLOR_ICON_DEFAULT);
    tft.drawRect(x, y, ICON_WIDTH, ICON_HEIGHT, COLOR_ICON_BORDER);
  }
}
