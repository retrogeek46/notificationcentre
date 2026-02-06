#include "api_handlers.h"
#include "state.h"
#include "config.h"
#include "notif_screen.h"
#include "reminder_screen.h"
#include "timer_screen.h"
#include "todo_screen.h"
#include "led_control.h"
#include "motor_control.h"
#include "storage.h"

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

  // Calendar month
  server.on("/calmonth", HTTP_POST, handleCalendarMonth);

  // Todo list
  server.on("/todo", HTTP_POST, handleTodoList);
  server.on("/todos", HTTP_GET, handleListTodos);
  server.on("/completeTask", HTTP_POST, handleCompleteTask);

  // Root
  server.on("/", HTTP_GET, handleRoot);

  // Enable CORS for local dashboard
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

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

  // GitHub notification formatter: restructure from/message for better display
  // Input: from="@user wrote a review comment on your pull request", message="org/repo #123"
  // Output: from="user", message="review comment on org/repo #123"
  if (app.indexOf("github") >= 0 && from.startsWith("@")) {
    // Extract username (everything between @ and first space)
    int spaceIdx = from.indexOf(' ', 1);
    if (spaceIdx > 1) {
      String username = from.substring(1, spaceIdx);  // Remove @ prefix
      String action = from.substring(spaceIdx + 1);   // Rest is the action
      
      // Clean up common action patterns
      action.replace("wrote a ", "");                     // "wrote a review comment" -> "review comment"
      action.replace("approved your pull request", "approved");
      action.replace("requested changes on your pull request", "requested changes");
      action.replace("commented on your pull request", "commented");
      action.replace("mentioned you on ", "mentioned in ");
      action.replace("on your pull request", "");
      action.replace("on your ", "");
      action.replace("your ", "");
      action.trim();
      
      // Combine action with repo info
      if (action.length() > 0 && message.length() > 0) {
        message = action + " on " + message;
      } else if (action.length() > 0) {
        message = action;
      }
      from = username;  // Just the username without @
    }
  }

  Serial.printf("FormNotify - app: [%s], from: [%s], message: [%s], priority: [%s]\n",
                app.c_str(), from.c_str(), message.c_str(), priority.c_str());

  addNotification(app, from, message, getPriorityColor(priority.c_str()));
  request->send(200, "application/json", "{\"status\":\"OK\"}");
}

void handleClearAll(AsyncWebServerRequest* request) {
  Serial.println("=== CLEAR ALL NOTIFICATIONS ===");
  clearAllNotifications();
  
  // Switch to dynamic default screen
  currentScreen = getDefaultScreen();
  setZoneDirty(ZONE_TITLE);
  setAllContentDirty();
  
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
// Base64 decoding helper using mbedtls
#include "mbedtls/base64.h"

static bool decodeAlbumArt(const String& artData) {
  if (artData.length() == 0) {
    return false;
  }

  // Parse format: WxH;base64data
  int semiPos = artData.indexOf(';');
  if (semiPos < 0) {
    Serial.println("Album art: invalid format (no semicolon)");
    return false;
  }

  String dimStr = artData.substring(0, semiPos);
  String artB64 = artData.substring(semiPos + 1);

  // Parse dimensions (WxH)
  int xPos = dimStr.indexOf('x');
  if (xPos < 0) {
    Serial.println("Album art: invalid dimensions");
    return false;
  }

  int width = dimStr.substring(0, xPos).toInt();
  int height = dimStr.substring(xPos + 1).toInt();

  // Validate dimensions
  if (width < 1 || width > ALBUM_ART_MAX_WIDTH || height != ALBUM_ART_SIZE) {
    Serial.printf("Album art: invalid size %dx%d\n", width, height);
    return false;
  }

  // Expected size based on dimensions
  const size_t expectedSize = width * height * 2;  // 2 bytes per pixel
  size_t outputLen = 0;

  // Decode base64
  int ret = mbedtls_base64_decode(
    (unsigned char*)albumArt,
    sizeof(albumArt),
    &outputLen,
    (const unsigned char*)artB64.c_str(),
    artB64.length()
  );

  if (ret != 0 || outputLen != expectedSize) {
    Serial.printf("Album art decode failed: ret=%d, len=%d (expected %d)\n", ret, outputLen, expectedSize);
    return false;
  }

  // Store dimensions
  albumArtWidth = width;
  albumArtHeight = height;

  Serial.printf("Album art decoded: %dx%d (%d bytes)\n", width, height, outputLen);
  return true;
}

void handleNowPlaying(AsyncWebServerRequest* request) {
  String song = request->hasParam("song", true) ? request->getParam("song", true)->value() : "";
  String artist = request->hasParam("artist", true) ? request->getParam("artist", true)->value() : "";
  
  // Use arg() for 'art' as it's typically a large body parameter
  String artB64 = "";
  if (request->hasArg("art")) {
    artB64 = request->arg("art");
  } else if (request->hasParam("art", true)) {
    artB64 = request->getParam("art", true)->value();
  }

  // If song is empty, clear now playing (but preserve disc frame state)
  if (song.length() == 0) {
    nowPlayingSong = "";
    nowPlayingArtist = "";
    nowPlayingActive = false;
    nowPlayingScrollPixel = 0;
    albumArtValid = false;  // Clear album art
    setZoneDirty(ZONE_STATUS);

    Serial.println("Now Playing: cleared");
    request->send(200, "application/json", "{\"status\":\"cleared\"}");
    return;
  }

  Serial.printf("Now Playing: %s - %s (art length: %d)\n", song.c_str(), artist.c_str(), artB64.length());

  // New song - start scroll from right edge
  nowPlayingSong = song;
  nowPlayingArtist = artist;
  nowPlayingUpdated = millis();
  nowPlayingScrollPixel = -320; 
  lastScrollUpdate = millis();
  lastDiscUpdate = millis();
  nowPlayingActive = true;

  // Decode album art if provided
  if (artB64.length() > 0) {
    albumArtValid = decodeAlbumArt(artB64);
  } else {
    albumArtValid = false;
  }

  setZoneDirty(ZONE_STATUS);  // Album art displays in status zone

  Serial.printf("Now Playing: Update done, artValid=%d\n", albumArtValid);
  request->send(200, "application/json", "{\"status\":\"ok\"}");
}

// ==================== Screen Switch Handler ====================
void handleScreenSwitch(AsyncWebServerRequest* request) {
  String name = request->hasParam("name") ? request->getParam("name")->value() : "";
  String screenName = "notifs";

  if (name == "reminder") {
    currentScreen = SCREEN_REMINDER;
    screenName = "reminder";
  } else if (name == "calendar") {
    currentScreen = SCREEN_CALENDAR;
    screenName = "calendar";
  } else if (name == "timer") {
    currentScreen = SCREEN_TIMER;
    resetTimerScreen();  // Reset timer display for fresh draw
    screenName = "timer";
  } else if (name == "todo") {
    currentScreen = SCREEN_TODO;
    resetTodoScreen();  // Reset todo display for fresh draw
    screenName = "todo";
  } else {
    currentScreen = SCREEN_NOTIFS;
    screenName = "notifs";
  }

  setZoneDirty(ZONE_TITLE);
  setAllContentDirty();

  request->send(200, "application/json",
    "{\"status\":\"ok\",\"screen\":\"" + screenName + "\"}");
}

// ==================== Root Handler ====================
void handleRoot(AsyncWebServerRequest* request) {
  String html = "<h1>Notification Center</h1>";
  html += "<p>Use <b>/addreminder</b> POST to add reminders</p>";
  html += "<p>Use <b>/reminders</b> GET to list reminders</p>";
  html += "<p>Use <b>/completeReminder?id=...</b> POST to mark done</p>";
  html += "<p>Use <b>/screen?name=notifs|reminder|calendar|timer</b> POST to switch</p>";
  html += "<p>Use <b>/nowplaying</b> POST with song, artist</p>";
  html += "<p>Use <b>/motor</b> POST with speed=0..255</p>";
  html += "<p>Use <b>/gaming</b> POST with enabled=0|1</p>";
  html += "<p>Use <b>/pcstats</b> POST with cpu_temp, cpu_usage, cpu_speed, ram_used, ram_total, gpu_temp, gpu_usage, net_speed</p>";
  html += "<p>Use <b>/calmonth</b> POST with month=1-12, year=YYYY (0 to reset to current)</p>";
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

// ==================== Calendar Month Handler ====================
void handleCalendarMonth(AsyncWebServerRequest* request) {
  String monthStr = request->hasParam("month", true) ? request->getParam("month", true)->value() : "";
  String yearStr = request->hasParam("year", true) ? request->getParam("year", true)->value() : "";

  int month = monthStr.toInt();  // 1-12, 0 = reset
  int year = yearStr.toInt();    // YYYY, 0 = reset

  if (month == 0 && year == 0) {
    // Reset to current month/year
    calViewMonth = -1;
    calViewYear = 0;
    Serial.println("Calendar: reset to current month");
  } else {
    // Validate and set
    if (month >= 1 && month <= 12) {
      calViewMonth = month - 1;  // Convert to 0-11
    }
    if (year > 0) {
      calViewYear = year;
    }
    Serial.printf("Calendar: set to %d/%d\n", calViewMonth + 1, calViewYear);
  }

  // Switch to calendar screen if not already on it
  if (currentScreen != SCREEN_CALENDAR) {
    currentScreen = SCREEN_CALENDAR;
    setZoneDirty(ZONE_TITLE);
  }
  setAllContentDirty();

  request->send(200, "application/json",
    "{\"status\":\"ok\",\"month\":" + String(calViewMonth + 1) + ",\"year\":" + String(calViewYear) + "}");
}

// ==================== Todo List Handler ====================
void handleTodoList(AsyncWebServerRequest* request) {
  // Expects params where key is numeric index (0, 1, 2...) and value is task text
  // e.g., POST /todo with body: 0=Task one&1=Task two&2=Task three
  // Only updates specified indices, preserving others
  
  int updatedCount = 0;
  
  // Check for numeric params 0-19
  for (int i = 0; i < MAX_TODO_ITEMS; i++) {
    String paramName = String(i);
    
    // Check both POST body and query params
    String taskText = "";
    if (request->hasParam(paramName, true)) {
      taskText = request->getParam(paramName, true)->value();
    } else if (request->hasParam(paramName)) {
      taskText = request->getParam(paramName)->value();
    }
    
    if (taskText.length() > 0) {
      taskText.trim();
      todoItems[i].text = taskText;
      todoItems[i].completed = false;
      
      // Expand count if needed
      if (i >= todoItemCount) {
        todoItemCount = i + 1;
      }
      
      Serial.printf("Todo[%d] = %s\n", i, taskText.c_str());
      updatedCount++;
    }
  }
  
  if (updatedCount == 0) {
    request->send(400, "application/json", "{\"error\":\"No tasks provided. Use 0=text, 1=text, etc.\"}");
    return;
  }
  
  Serial.printf("Todo list updated: %d items changed, total: %d\n", updatedCount, todoItemCount);
  
  // Switch to todo screen if we have items
  if (todoItemCount > 0 && currentScreen != SCREEN_TODO) {
    currentScreen = SCREEN_TODO;
    setZoneDirty(ZONE_TITLE);
  }
  setAllContentDirty();
  saveTodos();  // Persist to flash
  
  request->send(200, "application/json",
    "{\"status\":\"ok\",\"updated\":" + String(updatedCount) + ",\"count\":" + String(todoItemCount) + "}");
}

// ==================== Complete Task Handler ====================
void handleCompleteTask(AsyncWebServerRequest* request) {
  String indexStr = request->hasParam("index", true) ? request->getParam("index", true)->value()
                                                     : (request->hasParam("index") ? request->getParam("index")->value() : "");
  
  if (indexStr.length() == 0) {
    request->send(400, "application/json", "{\"error\":\"Missing index\"}");
    return;
  }
  
  int index = indexStr.toInt();
  
  if (index < 0 || index >= todoItemCount) {
    request->send(400, "application/json", "{\"error\":\"Invalid index\"}");
    return;
  }
  
  todoItems[index].completed = true;
  Serial.printf("Task %d marked complete: %s\n", index, todoItems[index].text.c_str());
  
  setAllContentDirty();
  saveTodos();  // Persist to flash
  
  request->send(200, "application/json", "{\"status\":\"completed\",\"index\":" + String(index) + "}");
}

// ==================== List Todos Handler ====================
void handleListTodos(AsyncWebServerRequest* request) {
  String json = "[";
  bool first = true;
  
  for (int i = 0; i < todoItemCount; i++) {
    if (!first) json += ",";
    first = false;
    
    json += "{\"index\":";
    json += String(i);
    json += ",\"text\":\"";
    // Escape quotes in text
    String escapedText = todoItems[i].text;
    escapedText.replace("\"", "\\\"");
    json += escapedText;
    json += "\",\"completed\":";
    json += todoItems[i].completed ? "true" : "false";
    json += "}";
  }
  
  json += "]";
  request->send(200, "application/json", json);
}
