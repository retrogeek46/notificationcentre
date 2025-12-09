#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>

void initWiFi();
void initNTP();
void checkWiFiReconnect();
String getLocalIP();

#endif
