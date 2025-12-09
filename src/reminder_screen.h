#ifndef REMINDER_SCREEN_H
#define REMINDER_SCREEN_H

#include <Arduino.h>
#include "types.h"

void drawReminderContent();
void checkReminders();
int addReminder(String msg, time_t when, int limitMins, uint16_t color);
bool completeReminder(int id);
String listRemindersJson();
time_t parseDateTime(const String& dt);

#endif
