#ifndef SCREEN_H
#define SCREEN_H

#include <TFT_eSPI.h>
#include "types.h"

extern TFT_eSPI tft;

void initScreen();
void drawDebugZones();  // Debug: draw white zone boundaries
void refreshScreen();
void updateClock();
void clearZone(Zone zone);
void drawTitle();
void drawNowPlaying();
void updateNowPlayingTicker();

#endif
