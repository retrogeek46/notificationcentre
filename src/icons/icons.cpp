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

// ==================== Disc Icon (Spinning Triangles) ====================
void drawDiscIcon(int x, int y, int frame, bool spinning) {
  int cx = x + 8;  // Center X
  int cy = y + 8;  // Center Y

  // Outer circle (outline only)
  tft.drawCircle(cx, cy, 7, TFT_WHITE);

  // Small center circle
  tft.fillCircle(cx, cy, 2, TFT_WHITE);

  // Spinning triangles - two opposite triangles that rotate based on frame
  if (spinning) {
    float angle = (frame * 45.0) * PI / 180.0;  // 8 frames: 0, 45, 90, etc.

    // Triangle 1 - pointing outward from center
    int t1x1 = cx + (int)(3 * cos(angle));
    int t1y1 = cy + (int)(3 * sin(angle));
    int t1x2 = cx + (int)(6 * cos(angle - 0.4));
    int t1y2 = cy + (int)(6 * sin(angle - 0.4));
    int t1x3 = cx + (int)(6 * cos(angle + 0.4));
    int t1y3 = cy + (int)(6 * sin(angle + 0.4));
    tft.fillTriangle(t1x1, t1y1, t1x2, t1y2, t1x3, t1y3, TFT_WHITE);

    // Triangle 2 - opposite side (180 degrees rotated)
    float angle2 = angle + PI;
    int t2x1 = cx + (int)(3 * cos(angle2));
    int t2y1 = cy + (int)(3 * sin(angle2));
    int t2x2 = cx + (int)(6 * cos(angle2 - 0.4));
    int t2y2 = cy + (int)(6 * sin(angle2 - 0.4));
    int t2x3 = cx + (int)(6 * cos(angle2 + 0.4));
    int t2y3 = cy + (int)(6 * sin(angle2 + 0.4));
    tft.fillTriangle(t2x1, t2y1, t2x2, t2y2, t2x3, t2y3, TFT_WHITE);
  } else {
    // Static triangles when not spinning (at default position)
    tft.fillTriangle(cx + 3, cy, cx + 6, cy - 2, cx + 6, cy + 2, TFT_WHITE);
    tft.fillTriangle(cx - 3, cy, cx - 6, cy - 2, cx - 6, cy + 2, TFT_WHITE);
  }
}
