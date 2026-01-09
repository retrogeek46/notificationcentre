#ifndef ICONS_H
#define ICONS_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <pgmspace.h>
#include "../config.h"

// Icon data
#include "slack_icon.h"
#include "github_icon.h"
#include "jira.h"

// External TFT reference
extern TFT_eSPI tft;

// Draw the appropriate app icon at position
void drawAppIcon(int x, int y, String app);

// Individual icon drawers
void drawSlackIcon(int x, int y);
void drawGitHubIcon(int x, int y);
void drawJiraIcon(int x, int y);
void drawDiscIcon(int x, int y, int frame, bool spinning);

#endif
