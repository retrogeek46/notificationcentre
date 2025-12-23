#include "api_handlers.h"
#include "state.h"
#include "config.h"
#include "notif_screen.h"
#include "reminder_screen.h"
#include "led_control.h"
#include "motor_control.h"

AsyncWebServer server(80);

// ==================== Setup Routes ====================
void setupApiRoutes() {
  // Screen control
  server.on("/screen", HTTP_POST, handleScreenSwitch);

  // Notification endpoints
  server.on("/notify", HTTP_ANY, handleFormNotify);
  server.on("/clear", HTTP_POST, handleClearAll);

  // Reminder endpoints
  server.on("/addreminder", HTTP_POST, handleAddReminder);
  server.on("/reminders", HTTP_GET, handleListReminders);
  server.on("/completeReminder", HTTP_POST, handleCompleteReminder);

  // Now playing
  server.on("/nowplaying", HTTP_POST, handleNowPlaying);

  // Motor control
  server.on("/motor", HTTP_POST, handleMotorSet);

  // Gaming mode / PC stats
  server.on("/gaming", HTTP_POST, handleGamingMode);
  server.on("/pcstats", HTTP_POST, handlePcStats);

  // Root
  server.on("/", HTTP_GET, handleRoot);

  server.begin();
  Serial.println("Ready! http://notification.local/");
}

// ==================== Notification Handlers ====================
void handleFormNotify(AsyncWebServerRequest* request) {
  String app = request->hasParam("app", true) ? request->getParam("app", true)->value() : "App";
  String from_raw = request->hasParam("from", true) ? request->getParam("from", true)->value() : "";
  String message = request->hasParam("message", true) ? request->getParam("message", true)->value() : "Notification";
  String priority = request->hasParam("priority", true) ? request->getParam("priority", true)->value() : "";

  String from = extractSender(from_raw);

  Serial.printf("FormNotify - app: [%s], from: [%s], message: [%s], priority: [%s]\n",
                app.c_str(), from.c_str(), message.c_str(), priority.c_str());

  addNotification(app, from, message, getPriorityColor(priority.c_str()));
  request->send(200, "application/json", "{\"status\":\"OK\"}");
}

void handleClearAll(AsyncWebServerRequest* request) {
  Serial.println("=== CLEAR ALL NOTIFICATIONS ===");
  clearAllNotifications();
  request->send(200, "application/json", "{\"status\":\"cleared\"}");
}

// ==================== Reminder Handlers ====================
void handleAddReminder(AsyncWebServerRequest* request) {
  String message = request->hasParam("message", true) ? request->getParam("message", true)->value()
                                                      : (request->hasParam("message") ? request->getParam("message")->value() : "");
  String timestr = request->hasParam("time", true) ? request->getParam("time", true)->value()
                                                   : (request->hasParam("time") ? request->getParam("time")->value() : "");
  String limitStr = request->hasParam("limit", true) ? request->getParam("limit", true)->value()
                                                     : (request->hasParam("limit") ? request->getParam("limit")->value() : "0");
  String priority = request->hasParam("priority", true) ? request->getParam("priority", true)->value()
                                                        : (request->hasParam("priority") ? request->getParam("priority")->value() : "normal");

  if (timestr.length() == 0) {
    request->send(400, "application/json", "{\"error\":\"Missing time (yyyy-mm-dd hh:mm)\"}");
    return;
  }

  time_t when = parseDateTime(timestr);
  if (when == 0) {
    request->send(400, "application/json", "{\"error\":\"Invalid time format, use yyyy-mm-dd hh:mm\"}");
    return;
  }

  int limitMins = limitStr.toInt();
  int id = addReminder(message, when, limitMins, getPriorityColor(priority.c_str()));

  if (id == -1) {
    request->send(500, "application/json", "{\"error\":\"Max reminders reached\"}");
    return;
  }

  Serial.printf("Added reminder id=%d msg=%s when=%ld limit=%d\n", id, message.c_str(), when, limitMins);
  request->send(200, "application/json", String("{\"status\":\"added\",\"id\":") + String(id) + String("}"));
}

void handleListReminders(AsyncWebServerRequest* request) {
  request->send(200, "application/json", listRemindersJson());
}

void handleCompleteReminder(AsyncWebServerRequest* request) {
  String idStr = request->hasParam("id", true) ? request->getParam("id", true)->value()
                                               : (request->hasParam("id") ? request->getParam("id")->value() : "");
  if (idStr.length() == 0) {
    request->send(400, "application/json", "{\"error\":\"Missing id\"}");
    return;
  }

  int id = idStr.toInt();
  if (completeReminder(id)) {
    request->send(200, "application/json", "{\"status\":\"completed\"}");
  } else {
    request->send(404, "application/json", "{\"error\":\"not found\"}");
  }
}

// ==================== Now Playing Handler ====================
void handleNowPlaying(AsyncWebServerRequest* request) {
  String song = request->hasParam("song", true) ? request->getParam("song", true)->value() : "";
  String artist = request->hasParam("artist", true) ? request->getParam("artist", true)->value() : "";

  // If song is empty, clear now playing (but preserve disc frame state)
  if (song.length() == 0) {
    nowPlayingSong = "";
    nowPlayingArtist = "";
    nowPlayingActive = false;
    nowPlayingScrollPixel = 0;
    // Don't reset discFrame - keep current position for resume
    setZoneDirty(ZONE_STATUS);

    Serial.println("Now Playing: cleared");
    request->send(200, "application/json", "{\"status\":\"cleared\"}");
    return;
  }

  // New song - reset scroll position but preserve disc frame
  nowPlayingSong = song;
  nowPlayingArtist = artist;
  nowPlayingUpdated = millis();
  nowPlayingScrollPixel = 0;
  lastScrollUpdate = millis();
  // Don't reset discFrame - keep spinning from current position
  lastDiscUpdate = millis();
  nowPlayingActive = true;
  setZoneDirty(ZONE_STATUS);

  Serial.printf("Now Playing: %s - %s\n", song.c_str(), artist.c_str());
  request->send(200, "application/json", "{\"status\":\"ok\"}");
}

// ==================== Screen Switch Handler ====================
void handleScreenSwitch(AsyncWebServerRequest* request) {
  String name = request->hasParam("name") ? request->getParam("name")->value() : "";

  if (name == "reminder") {
    currentScreen = SCREEN_REMINDER;
    updateLedForScreen(SCREEN_REMINDER);
  } else {
    currentScreen = SCREEN_NOTIFS;
    updateLedForScreen(SCREEN_NOTIFS);
  }

  setZoneDirty(ZONE_TITLE);
  setAllContentDirty();

  request->send(200, "application/json",
    "{\"status\":\"ok\",\"screen\":\"" + String((currentScreen == SCREEN_REMINDER) ? "reminder" : "notifs") + "\"}");
}

// ==================== Root Handler ====================
void handleRoot(AsyncWebServerRequest* request) {
  String html = "<h1>Notification Center</h1>";
  html += "<p>Use <b>/addreminder</b> POST to add reminders</p>";
  html += "<p>Use <b>/reminders</b> GET to list reminders</p>";
  html += "<p>Use <b>/completeReminder?id=...</b> POST to mark done</p>";
  html += "<p>Use <b>/screen?name=notifs|reminder</b> POST to switch</p>";
  html += "<p>Use <b>/nowplaying</b> POST with song, artist</p>";
  html += "<p>Use <b>/motor</b> POST with speed=0..255</p>";
  html += "<p>Use <b>/gaming</b> POST with enabled=0|1</p>";
  html += "<p>Use <b>/pcstats</b> POST with cpu_temp, cpu_usage, cpu_speed, ram_used, ram_total, gpu_temp, gpu_usage, net_speed</p>";
  request->send(200, "text/html", html);
}

// ==================== Motor Handler ====================
void handleMotorSet(AsyncWebServerRequest* request) {
  if (!request->hasParam("speed", true)) {
    request->send(400, "application/json", "{\"error\":\"Missing speed\"}");
    return;
  }

  String s = request->getParam("speed", true)->value();
  int val = s.toInt();
  val = constrain(val, 0, 255);
  setMotorRaw(val);

  String resp = "{\"speed\":" + String(val) + "}";
  request->send(200, "application/json", resp);
}

// ==================== Gaming Mode Handler ====================
void handleGamingMode(AsyncWebServerRequest* request) {
  String enabled = request->hasParam("enabled", true) ? request->getParam("enabled", true)->value() : "";

  if (enabled == "1" || enabled == "true") {
    gamingMode = true;
    setZoneDirty(ZONE_STATUS);
    Serial.println("Gaming mode: ON");
    request->send(200, "application/json", "{\"gaming\":true}");
  } else {
    gamingMode = false;
    setZoneDirty(ZONE_STATUS);
    Serial.println("Gaming mode: OFF");
    request->send(200, "application/json", "{\"gaming\":false}");
  }
}

// ==================== PC Stats Handler ====================
void handlePcStats(AsyncWebServerRequest* request) {
  // Always accept stats (display logic decides what to show)

  // Parse all stats from request
  pcCpuTemp = request->hasParam("cpu_temp", true) ? request->getParam("cpu_temp", true)->value().toInt() : pcCpuTemp;
  pcCpuUsage = request->hasParam("cpu_usage", true) ? request->getParam("cpu_usage", true)->value().toInt() : pcCpuUsage;
  pcCpuSpeed = request->hasParam("cpu_speed", true) ? request->getParam("cpu_speed", true)->value().toFloat() : pcCpuSpeed;
  pcRamUsed = request->hasParam("ram_used", true) ? request->getParam("ram_used", true)->value().toInt() : pcRamUsed;
  pcRamTotal = request->hasParam("ram_total", true) ? request->getParam("ram_total", true)->value().toInt() : pcRamTotal;
  pcGpuTemp = request->hasParam("gpu_temp", true) ? request->getParam("gpu_temp", true)->value().toInt() : pcGpuTemp;
  pcGpuUsage = request->hasParam("gpu_usage", true) ? request->getParam("gpu_usage", true)->value().toInt() : pcGpuUsage;
  pcNetDown = request->hasParam("net_down", true) ? request->getParam("net_down", true)->value().toFloat() : pcNetDown;
  pcNetUp = request->hasParam("net_up", true) ? request->getParam("net_up", true)->value().toFloat() : pcNetUp;

  pcStatsUpdated = millis();
  setZoneDirty(ZONE_STATUS);

  Serial.printf("PC Stats: CPU %d°/%d%%/%.1fG GPU %d°/%d%% RAM %d/%dG NET ↓%.1f ↑%.1fM\n",
                pcCpuTemp, pcCpuUsage, pcCpuSpeed, pcGpuTemp, pcGpuUsage,
                pcRamUsed, pcRamTotal, pcNetDown, pcNetUp);

  request->send(200, "application/json", "{\"status\":\"ok\"}");
}
