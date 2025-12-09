#ifndef ICONS_H
#define ICONS_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <pgmspace.h>
#include "../config.h"

// Icon data
#include "slack_icon.h"

// External TFT reference
extern TFT_eSPI tft;

// Draw the appropriate app icon at position
void drawAppIcon(int x, int y, String app);

// Individual icon drawers
void drawSlackIcon(int x, int y);

#endif
