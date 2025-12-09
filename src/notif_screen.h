#ifndef NOTIF_SCREEN_H
#define NOTIF_SCREEN_H

#include <Arduino.h>
#include "types.h"

void drawNotifContent();
void addNotification(String app, String from, String msg, uint16_t color);
void clearAllNotifications();
uint16_t getPriorityColor(const char* priority);
String extractSender(String msg);

#endif
