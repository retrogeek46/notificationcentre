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
void handleGamingMode(AsyncWebServerRequest* request);
void handlePcStats(AsyncWebServerRequest* request);
void handleCalendarMonth(AsyncWebServerRequest* request);
void handleTodoList(AsyncWebServerRequest* request);
void handleListTodos(AsyncWebServerRequest* request);
void handleCompleteTask(AsyncWebServerRequest* request);
void handleDeleteTask(AsyncWebServerRequest* request);
void handleTimerSet(AsyncWebServerRequest* request);
void handleTimerStart(AsyncWebServerRequest* request);
void handleTimerStop(AsyncWebServerRequest* request);
void handleTimerStatus(AsyncWebServerRequest* request);
void handleTimerLabel(AsyncWebServerRequest* request);
void handleDashboard(AsyncWebServerRequest* request);
void handleFocusMode(AsyncWebServerRequest* request);
void handleFocusStatus(AsyncWebServerRequest* request);

#endif
