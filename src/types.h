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

// Zone Y boundaries
const int ZONE_TITLE_Y_START = 0;
const int ZONE_TITLE_Y_END = 18;
const int ZONE_CLOCK_Y_START = 0;
const int ZONE_CLOCK_Y_END = 18;
const int ZONE_STATUS_Y_START = 19;
const int ZONE_STATUS_Y_END = 36;   // Reduced to make room for content
const int ZONE_CONTENT_Y_START = 37; // Earlier to catch icon at y-2
const int ZONE_CONTENT_Y_END = 240;

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
