#ifndef API_HANDLERS_H
#define API_HANDLERS_H

#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;

void setupApiRoutes();

// Individual handlers
void handleFormNotify(AsyncWebServerRequest* request);
void handleClearAll(AsyncWebServerRequest* request);
void handleRoot(AsyncWebServerRequest* request);
void handleAddReminder(AsyncWebServerRequest* request);
void handleListReminders(AsyncWebServerRequest* request);
void handleCompleteReminder(AsyncWebServerRequest* request);
void handleNowPlaying(AsyncWebServerRequest* request);
void handleScreenSwitch(AsyncWebServerRequest* request);
void handleMotorSet(AsyncWebServerRequest* request);

#endif
