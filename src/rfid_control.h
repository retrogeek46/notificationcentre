#ifndef RFID_CONTROL_H
#define RFID_CONTROL_H

#include <Arduino.h>

void initRFID();
void checkRFID();

bool registerRfidCard(String uid, String action, String param);
bool unregisterRfidCard(String uid);
String getRegisteredCardsJson();

#endif
