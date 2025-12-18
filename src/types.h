#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"

// ==================== Screen Zones ====================
enum Zone {
  ZONE_TITLE = 0,
  ZONE_CLOCK = 1,
  ZONE_STATUS = 2,
  ZONE_CONTENT = 3,
  ZONE_COUNT = 4
};

// Zone boundaries (with 2px padding for background border)
// Title zone: 2-120 x 2-24
const int ZONE_TITLE_X_START = 2;
const int ZONE_TITLE_X_END = 120;
const int ZONE_TITLE_Y_START = 2;
const int ZONE_TITLE_Y_END = 24;

// Clock zone: 121-318 x 2-24
const int ZONE_CLOCK_X_START = 121;
const int ZONE_CLOCK_X_END = 318;
const int ZONE_CLOCK_Y_START = 2;
const int ZONE_CLOCK_Y_END = 24;

// Status zone: 2-318 x 25-44
const int ZONE_STATUS_X_START = 2;
const int ZONE_STATUS_X_END = 318;
const int ZONE_STATUS_Y_START = 25;
const int ZONE_STATUS_Y_END = 44;

// Content zone: 2-318 x 45-238
const int ZONE_CONTENT_X_START = 2;
const int ZONE_CONTENT_X_END = 318;
const int ZONE_CONTENT_Y_START = 45;
const int ZONE_CONTENT_Y_END = 238;

// ==================== Screen Types ====================
enum Screen {
  SCREEN_NOTIFS = 0,
  SCREEN_REMINDER = 1
};

// ==================== Notification ====================
struct Notification {
  String app;
  String from;
  String message;
  uint16_t color;

  Notification() : app(""), from(""), message(""), color(TFT_WHITE) {}
};

// ==================== Reminder ====================
struct Reminder {
  int id;
  String message;
  time_t when;
  int limitMinutes;
  bool completed;
  uint16_t color;
  bool triggered;
  time_t nextReviewTime;
  int reviewCount;
  
  Reminder() : id(0), message(""), when(0), limitMinutes(0), completed(false),
               color(TFT_WHITE), triggered(false), nextReviewTime(0), reviewCount(0) {}
};

#endif
