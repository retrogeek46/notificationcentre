#ifndef TIMER_SCREEN_H
#define TIMER_SCREEN_H

#include <Arduino.h>

// Draw timer dial and content in content zones
void drawTimerContent();

// Update timer countdown (call in main loop)
void updateTimerTick();

// Start the timer countdown
void startTimer();

// Stop/reset the timer
void stopTimer();

// Adjust timer minutes (called by encoder)
void adjustTimerMinutes(int delta);

// Reset timer screen state (call when entering timer screen)
void resetTimerScreen();

#endif
