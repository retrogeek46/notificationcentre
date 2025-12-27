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
  ZONE_CONTENT1 = 3,
  ZONE_CONTENT2 = 4,
  ZONE_CONTENT3 = 5,
  ZONE_COUNT = 6
};

// Zone boundaries (zones extend to screen edges)
// Title zone: 0-136 x 0-24
const int ZONE_TITLE_X_START = 0;
const int ZONE_TITLE_X_END = 136;
const int ZONE_TITLE_Y_START = 0;
const int ZONE_TITLE_Y_END = 24;

// Clock zone: 137-319 x 0-24
const int ZONE_CLOCK_X_START = 137;
const int ZONE_CLOCK_X_END = 319;
const int ZONE_CLOCK_Y_START = 0;
const int ZONE_CLOCK_Y_END = 24;

// Status zone: 0-319 x 23-42
const int ZONE_STATUS_X_START = 0;
const int ZONE_STATUS_X_END = 319;
const int ZONE_STATUS_Y_START = 23;
const int ZONE_STATUS_Y_END = 42;

// Content zones (3 slots of 65px each)
// Content1: 0-319 x 45-109
const int ZONE_CONTENT1_X_START = 0;
const int ZONE_CONTENT1_X_END = 319;
const int ZONE_CONTENT1_Y_START = 45;
const int ZONE_CONTENT1_Y_END = 109;

// Content2: 0-319 x 110-174
const int ZONE_CONTENT2_X_START = 0;
const int ZONE_CONTENT2_X_END = 319;
const int ZONE_CONTENT2_Y_START = 110;
const int ZONE_CONTENT2_Y_END = 174;

// Content3: 0-319 x 175-239
const int ZONE_CONTENT3_X_START = 0;
const int ZONE_CONTENT3_X_END = 319;
const int ZONE_CONTENT3_Y_START = 175;
const int ZONE_CONTENT3_Y_END = 239;

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
