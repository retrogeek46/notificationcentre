#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <pgmspace.h>
#include "slack_icon.h"
#include <Adafruit_NeoPixel.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define LED_PIN 32
#define LED_COUNT 1

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

#define ICON_HEIGHT 16
#define ICON_WIDTH 16

// --- Motor driver pins (L298N Motor A) ---
// #define MOTOR_ENA 4    // PWM-capable
// #define MOTOR_IN1 16
// #define MOTOR_IN2 17

// const int MOTOR_PWM_CHANNEL = 0;
// const int MOTOR_PWM_FREQ    = 1000;  // 1 kHz
// const int MOTOR_PWM_RES     = 8;     // 0-255

// int motorSpeed = 50;     // start slow (0-255)

TFT_eSPI tftMain = TFT_eSPI();
AsyncWebServer server(80);

// ----------------- Notifications (existing) -----------------
struct Notification {
  String app;
  String from;
  String message;
  uint16_t color;
  Notification() : app(""), from(""), message(""), color(TFT_WHITE) {}
};
Notification notifications[5];

// ----------------- Screen state -----------------
enum Screen { SCREEN_NOTIFS = 0, SCREEN_REMINDER = 1 };
Screen currentScreen = SCREEN_NOTIFS;
bool screenDirty = true;
unsigned long lastClockUpdate = 0;

// ----------------- Reminders -----------------
struct Reminder {
  int id;
  String title;
  String message;
  time_t when;      // epoch time to remind
  int limitMinutes; // time-box: re-remind every limitMinutes if not completed (0 = none)
  bool completed;
  uint16_t color;
  bool triggered;        // whether initial trigger happened
  time_t nextReviewTime; // next scheduled follow-up (0 = not scheduled)
  int reviewCount;
  Reminder() : id(0), title(""), message(""), when(0), limitMinutes(0), completed(false),
               color(TFT_WHITE), triggered(false), nextReviewTime(0), reviewCount(0) {}
};

const int MAX_REMINDERS = 10;
Reminder reminders[MAX_REMINDERS];
int reminderCount = 0;
int nextReminderId = 1;

// ----------------- Prototypes -----------------
uint16_t getPriorityColor(const char *priority);
void drawAppIcon(int x, int y, String app);
void drawSlackIcon(int x, int y);
String extractSender(String msg);
void addStructuredNotification(String app, String from, String msg, uint16_t color);
void refreshNotifications();
void updateClock();
void resetPreviousTimeStr();
void setNotifLed(uint8_t r, uint8_t g, uint8_t b);

// screen handlers
void handleScreenSwitch(AsyncWebServerRequest *request);

// notif handlers
void handleFormNotify(AsyncWebServerRequest *request);
void handleClearAll(AsyncWebServerRequest *request);
void handleRoot(AsyncWebServerRequest *request);

// Reminder API handlers
void handleAddReminder(AsyncWebServerRequest *request);
void handleListReminders(AsyncWebServerRequest *request);
void handleCompleteReminder(AsyncWebServerRequest *request);

// Helpers
time_t parseDateTime(const String &dt); // expects "yyyy-mm-dd hh:mm"
void scheduleReminder(Reminder &r);
void checkReminders();          // called from loop()
void refreshRemindersDisplay(); // draw reminder screen
void triggerReminderVisuals(const Reminder &r);

// ----------------- Setup -----------------
void setup() {
  Serial.begin(115200);

  tftMain.init();
  tftMain.fillScreen(TFT_BLACK);
  tftMain.setRotation(3);
  tftMain.setFreeFont(&FreeMono9pt7b);
  tftMain.setTextSize(1);
  tftMain.setTextColor(TFT_YELLOW);
  tftMain.drawString("Connect: NotificationSetup", 10, 10);
  tftMain.drawString("192.168.4.1", 10, 30);

  strip.begin();
  strip.setBrightness(50);
  strip.show();

  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(1800);

  if (!wifiManager.autoConnect("NotificationSetup")) {
    tftMain.fillScreen(TFT_RED);
    tftMain.drawString("TIMEOUT!", 10, 50);
    delay(5000);
  }

  delay(1000);
  tftMain.fillScreen(TFT_BLACK);
  tftMain.setTextColor(TFT_GREEN);
  tftMain.drawString("WiFi OK!", 10, 10);
  tftMain.drawString(WiFi.localIP().toString(), 10, 30);
  WiFi.setSleep(false);

  // NTP
  configTime(5.5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP...");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println(" OK");

  // init arrays
  for (int i = 0; i < 5; i++) notifications[i] = Notification();
  for (int i = 0; i < MAX_REMINDERS; i++) reminders[i] = Reminder();

  if (!MDNS.begin("notification")) {
    Serial.println("MDNS failed - use IP!");
  } else {
    MDNS.addService("http", "tcp", 80);
  }

  delay(2000);

  // screen control
  server.on("/screen", HTTP_GET, handleScreenSwitch);

  // notification endpoints (existing)
  server.on("/notify", HTTP_ANY, handleFormNotify);
  server.on("/clear", HTTP_POST, handleClearAll);

  // new reminder endpoints
  server.on("/addreminder", HTTP_POST, handleAddReminder);
  server.on("/reminders", HTTP_GET, handleListReminders);
  server.on("/completeReminder", HTTP_POST, handleCompleteReminder);

  // root
  server.on("/", HTTP_GET, handleRoot);
  // server.on("/motor", HTTP_POST, handleMotorSet);
  server.begin();

  refreshNotifications();
  Serial.println("Ready! http://notification.local/notify");
}

// ----------------- Loop -----------------
void loop() {
  // Update clock every second
  if (millis() - lastClockUpdate > 1000) {
    updateClock();
    lastClockUpdate = millis();
  }

  // Check reminders and handle triggers
  checkReminders();

  // Refresh screen only when needed
  if (screenDirty) {
    if (currentScreen == SCREEN_NOTIFS)
      refreshNotifications();
    else
      refreshRemindersDisplay();
    screenDirty = false;
  }

  static unsigned long wifiCheck = 0;
  if (millis() - wifiCheck > 30000 && WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    wifiCheck = millis();
  }

  yield();
}

// ----------------- Drawing helpers -----------------
void drawSlackIcon(int x, int y) {
  static uint16_t lineBuf[ICON_WIDTH];
  const uint8_t *p = slack_icon_map;
  for (int row = 0; row < ICON_HEIGHT; row++) {
    for (int col = 0; col < ICON_WIDTH; col++) {
      uint8_t lo = pgm_read_byte(p++);
      uint8_t hi = pgm_read_byte(p++);
      uint16_t rgb565 = (lo << 8) | hi;
      lineBuf[col] = rgb565;
    }
    tftMain.pushImage(x, y + row, ICON_WIDTH, 1, lineBuf);
  }
}

void drawAppIcon(int x, int y, String app) {
  app.toLowerCase();
  if (app.indexOf("slack") >= 0)
    drawSlackIcon(x, y);
  else if (app.indexOf("whatsapp") >= 0) {
    tftMain.fillCircle(x + 8, y + 8, 7, TFT_GREEN);
    tftMain.drawCircle(x + 8, y + 8, 7, TFT_WHITE);
  } else if (app.indexOf("telegram") >= 0) {
    tftMain.fillCircle(x + 8, y + 8, 7, TFT_BLUE);
    tftMain.drawCircle(x + 8, y + 8, 7, TFT_WHITE);
  } else {
    tftMain.fillRect(x, y, ICON_WIDTH, ICON_HEIGHT, TFT_DARKGREY);
    tftMain.drawRect(x, y, ICON_WIDTH, ICON_HEIGHT, TFT_WHITE);
  }
}

char previousTimeStr[25] = "                    ";
void resetPreviousTimeStr() { memset(previousTimeStr, 0, sizeof(previousTimeStr)); }

void updateClock() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  char timeStr[25];
  strftime(timeStr, sizeof(timeStr), "%a,%d-%b,%H:%M:%S", &timeinfo);

  tftMain.setTextSize(1);
  tftMain.setTextColor(TFT_CYAN);

  int x = 100, y = 5;
  int charHeight = tftMain.fontHeight();
  int maxCharWidth = tftMain.textWidth("W");
  int cursorX = x;

  for (int i = 0; i < strlen(timeStr); i++) {
    if (timeStr[i] != previousTimeStr[i] || previousTimeStr[i] == '\0') {
      tftMain.fillRect(cursorX, y, maxCharWidth, charHeight, TFT_BLACK);
      char ch[2] = {timeStr[i], '\0'};
      tftMain.drawString(ch, cursorX, y);
    }
    previousTimeStr[i] = timeStr[i];
    cursorX += maxCharWidth;
  }
}

// ----------------- Notification screen -----------------
void refreshNotifications() {
  tftMain.fillScreen(TFT_BLACK);
  tftMain.setTextSize(1);
  tftMain.setTextColor(TFT_YELLOW);
  tftMain.drawString("NOTIFS", 5, 5);
  resetPreviousTimeStr();
  updateClock();

  tftMain.setTextSize(1);
  for (int i = 0; i < 5; i++) {
    int y = 40 + i * 55;
    if (notifications[i].message != "") {
      drawAppIcon(5, y, notifications[i].app);
      tftMain.setTextColor(notifications[i].color);
      String sender = notifications[i].from;
      if (sender.length() > 24)
        sender = sender.substring(0, 24);
      tftMain.drawString(sender + ":", 27, y);

      tftMain.setTextColor(TFT_LIGHTGREY);
      String msg = notifications[i].message;
      if (msg.length() > 47)
        msg = msg.substring(0, 47) + "...";

      String msgLine1 = msg.substring(0, min(24, (int)msg.length()));
      msgLine1.trim();
      tftMain.drawString(msgLine1, 27, y + 18);

      if (msg.length() > 24) {
        String msgLine2 = msg.substring(24);
        msgLine2.trim();
        tftMain.drawString(msgLine2, 27, y + 36);
      }
    }
  }
}

// ----------------- Reminder screen -----------------
void refreshRemindersDisplay() {
  tftMain.fillScreen(TFT_BLACK);
  tftMain.setTextSize(1);
  tftMain.setTextColor(TFT_YELLOW);
  tftMain.drawString("REMINDER", 5, 5);
  resetPreviousTimeStr();
  updateClock();

  // Build a small list of active reminders and sort by effective time (soonest first).
  int listIdx[MAX_REMINDERS];
  time_t listTime[MAX_REMINDERS];
  bool listTriggered[MAX_REMINDERS];
  int count = 0;

  for (int i = 0; i < MAX_REMINDERS; i++) {
    Reminder &r = reminders[i];
    if (r.id == 0 || r.completed) continue;
    // effective time: for triggered reminders prefer nextReviewTime if set, otherwise use original when
    time_t eff = r.triggered ? (r.nextReviewTime != 0 ? r.nextReviewTime : r.when) : r.when;
    listIdx[count] = i;
    listTime[count] = eff;
    listTriggered[count] = r.triggered;
    count++;
  }

  // sort: triggered reminders first, then by time within each group
  for (int a = 0; a < count - 1; a++) {
    int best = a;
    for (int b = a + 1; b < count; b++) {
      // triggered reminders come first
      if (listTriggered[b] && !listTriggered[best]) {
        best = b;
      } else if (listTriggered[b] == listTriggered[best] && listTime[b] < listTime[best]) {
        // same triggered status: sort by time
        best = b;
      }
    }
    if (best != a) {
      int ti = listIdx[a]; listIdx[a] = listIdx[best]; listIdx[best] = ti;
      time_t tt = listTime[a]; listTime[a] = listTime[best]; listTime[best] = tt;
      bool tb = listTriggered[a]; listTriggered[a] = listTriggered[best]; listTriggered[best] = tb;
    }
  }

  // display up to 5 reminders (triggered first, then soonest)
  int y = 40;
  int shown = 0;
  time_t now = time(nullptr);

  for (int s = 0; s < count && shown < 5; s++) {
    Reminder &rm = reminders[listIdx[s]];
    // icon color: active (triggered) -> red; non-triggered -> grey
    uint16_t iconColor = rm.triggered ? TFT_RED : TFT_DARKGREY;
    tftMain.fillCircle(10, y + 10, 8, iconColor);
    tftMain.drawCircle(10, y + 10, 8, TFT_WHITE);

    // title text: active (triggered) -> white; non-triggered -> slightly grey
    tftMain.setTextColor(rm.triggered ? TFT_WHITE : TFT_LIGHTGREY);
    String title = rm.title;
    if (title.length() > 0) {
      if (title.length() > 24) title = title.substring(0, 24) + "...";
    }
    // Show ID and title (if present)
    String titleLine = "[" + String(rm.id) + "]";
    if (title.length() > 0) {
      titleLine += " " + title;
    }
    tftMain.drawString(titleLine, 30, y);

    // time display: show relative time (xD yH zM) for future reminders, absolute time for triggered
    time_t effTime = listTime[s];
    char buf[20];
    if (rm.triggered) {
      // for triggered, show absolute time
      struct tm tm;
      localtime_r(&effTime, &tm);
      strftime(buf, sizeof(buf), "%H:%M", &tm);
    } else {
      // for future, show relative time
      long diff = effTime - now;
      long days = diff / 86400;
      long hours = (diff % 86400) / 3600;
      long mins = (diff % 3600) / 60;

      buf[0] = '\0';
      if (days > 0) {
        sprintf(buf + strlen(buf), "%ldD ", days);
      }
      if (hours > 0) {
        sprintf(buf + strlen(buf), "%ldH ", hours);
      }
      if (mins > 0 || buf[0] == '\0') {
        sprintf(buf + strlen(buf), "%ldM", mins);
      }
    }
    tftMain.setTextColor(TFT_LIGHTGREY);
    tftMain.drawString(String(buf), 30, y + 18);

    // message (single line) - use same color as title for readability
    tftMain.setTextColor(rm.triggered ? TFT_WHITE : TFT_LIGHTGREY);
    String msg = rm.message;
    if (msg.length() > 32) msg = msg.substring(0, 32) + "...";
    tftMain.drawString(msg, 30, y + 36);

    shown++;
    y += 55;
  }
}

// ----------------- Reminder logic -----------------
time_t parseDateTime(const String &dt) {
  // Expect "yyyy-mm-dd hh:mm" (24-hour). Return 0 on failure.
  if (dt.length() < 16)
    return 0;
  int year = dt.substring(0, 4).toInt();
  int month = dt.substring(5, 7).toInt();
  int day = dt.substring(8, 10).toInt();
  int hour = dt.substring(11, 13).toInt();
  int minute = dt.substring(14, 16).toInt();
  if (year < 2000 || month < 1 || month > 12 || day < 1)
    return 0;
  struct tm tm;
  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = 0;
  tm.tm_isdst = -1;
  time_t t = mktime(&tm);
  return t;
}

void scheduleReminder(Reminder &r) {
  // nothing heavy here — used if we want to set nextReviewTime
  if (r.limitMinutes > 0 && r.triggered && !r.completed && r.nextReviewTime == 0) {
    r.nextReviewTime = r.when + (time_t)r.limitMinutes * 60;
  }
}

void triggerReminderVisuals(const Reminder &r) {
  // Map color to RGB roughly
  uint8_t r8 = 80, g8 = 80, b8 = 80;
  if (r.color == TFT_RED) {
    r8 = 255;
    g8 = 0;
    b8 = 0;
  } else if (r.color == TFT_YELLOW) {
    r8 = 255;
    g8 = 80;
    b8 = 0;
  } else {
    r8 = 80;
    g8 = 80;
    b8 = 80;
  }
  setNotifLed(r8, g8, b8);
  // switch screen
  currentScreen = SCREEN_REMINDER;
  screenDirty = true;
}

void checkReminders() {
  time_t now = time(nullptr);
  for (int i = 0; i < MAX_REMINDERS; i++) {
    Reminder &r = reminders[i];
    if (r.id == 0)
      continue;
    if (r.completed)
      continue;

    // initial trigger
    if (!r.triggered && r.when != 0 && now >= r.when) {
      r.triggered = true;
      r.reviewCount = 0;
      if (r.limitMinutes > 0)
        r.nextReviewTime = r.when + (time_t)r.limitMinutes * 60;
      Serial.printf("Reminder triggered id=%d title=%s\n", r.id, r.title.c_str());
      triggerReminderVisuals(r);
    } else if (r.triggered && r.limitMinutes > 0 && r.nextReviewTime != 0 && now >= r.nextReviewTime) {
      // follow-up / review reminder
      r.reviewCount++;
      r.nextReviewTime = now + (time_t)r.limitMinutes * 60; // schedule next one
      Serial.printf("Reminder follow-up id=%d reviewCount=%d\n", r.id, r.reviewCount);
      triggerReminderVisuals(r);
    }
  }
}

// ----------------- Notification helpers -----------------
uint16_t getPriorityColor(const char *priority) {
  if (!priority)
    return TFT_WHITE;
  if (strcmp(priority, "high") == 0)
    return TFT_RED;
  if (strcmp(priority, "medium") == 0)
    return TFT_YELLOW;
  return TFT_WHITE;
}

void addStructuredNotification(String app, String from, String msg, uint16_t color) {
  for (int i = 4; i > 0; i--) notifications[i] = notifications[i - 1];
  notifications[0].app = app;
  notifications[0].from = from;
  notifications[0].message = msg.substring(0, 48);
  notifications[0].color = color;

  uint8_t rr = 80, gg = 80, bb = 80;
  if (color == TFT_RED) {
    rr = 255;
    gg = 0;
    bb = 0;
  } else if (color == TFT_YELLOW) {
    rr = 255;
    gg = 80;
    bb = 0;
  }
  setNotifLed(rr, gg, bb);
  // Switch to notifications screen when a notification arrives
  currentScreen = SCREEN_NOTIFS;
  screenDirty = true;
}

void setNotifLed(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

// ----------------- Request handlers -----------------
void handleFormNotify(AsyncWebServerRequest *request) {
  String app = request->hasParam("app", true) ? request->getParam("app", true)->value() : "App";
  String from_raw = request->hasParam("from", true) ? request->getParam("from", true)->value() : "";
  String message = request->hasParam("message", true) ? request->getParam("message", true)->value() : "Notification";
  String priority = request->hasParam("priority", true) ? request->getParam("priority", true)->value() : "";

  String from = extractSender(from_raw);

  Serial.printf("FormNotify - app: [%s], from: [%s], message: [%s], priority: [%s]\n",
                app.c_str(), from.c_str(), message.c_str(), priority.c_str());

  addStructuredNotification(app, from, message, getPriorityColor(priority.c_str()));
  request->send(200, "application/json", "{\"status\":\"OK\"}");
}

void handleClearAll(AsyncWebServerRequest *request) {
  Serial.println("=== CLEAR ALL NOTIFICATIONS ===");
  for (int i = 0; i < 5; i++) notifications[i] = Notification();
  screenDirty = true;
  setNotifLed(0, 0, 0);
  request->send(200, "application/json", "{\"status\":\"cleared\"}");
}

void handleRoot(AsyncWebServerRequest *request) {
  String html = "<h1>Notification Center</h1>";
  html += "<p>Use <b>/addreminder</b> POST to add reminders</p>";
  html += "<p>Use <b>/reminders</b> GET to list reminders</p>";
  html += "<p>Use <b>/completeReminder?id=...</b> POST to mark done</p>";
  html += "<p>Use <b>/screen?name=notifs</b> or <b>/screen?name=reminder</b> to switch</p>";
  request->send(200, "text/html", html);
}

String extractSender(String msg) {
  if (msg.length() == 0)
    return "Unknown";
  int colonIndex = msg.lastIndexOf(':');
  if (colonIndex != -1) {
    String sender = msg.substring(colonIndex + 1);
    sender.trim();
    return sender.length() > 0 ? sender : "Unknown";
  }
  return msg.length() > 0 ? msg : "Unknown";
}

// ----------------- Reminder endpoints -----------------
void handleAddReminder(AsyncWebServerRequest *request) {
  // Accept params either in body (form) or query
  String title = request->hasParam("title", true) ? request->getParam("title", true)->value()
                                                  : (request->hasParam("title") ? request->getParam("title")->value() : "");
  String message = request->hasParam("message", true) ? request->getParam("message", true)->value()
                                                      : (request->hasParam("message") ? request->getParam("message")->value() : "");
  String timestr = request->hasParam("time", true) ? request->getParam("time", true)->value()
                                                   : (request->hasParam("time") ? request->getParam("time")->value() : "");
  String limitStr = request->hasParam("limit", true) ? request->getParam("limit", true)->value()
                                                     : (request->hasParam("limit") ? request->getParam("limit")->value() : "0");
  String priority = request->hasParam("priority", true) ? request->getParam("priority", true)->value()
                                                        : (request->hasParam("priority") ? request->getParam("priority")->value() : "normal");

  // title is now optional; only time is required
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
  // find free slot
  int idx = -1;
  for (int i = 0; i < MAX_REMINDERS; i++)
    if (reminders[i].id == 0) {
      idx = i;
      break;
    }
  if (idx == -1) {
    request->send(500, "application/json", "{\"error\":\"Max reminders reached\"}");
    return;
  }

  reminders[idx].id = nextReminderId++;
  reminders[idx].title = title;
  reminders[idx].message = message;
  reminders[idx].when = when;
  reminders[idx].limitMinutes = max(0, limitMins);
  reminders[idx].completed = false;
  reminders[idx].color = getPriorityColor(priority.c_str());
  reminders[idx].triggered = false;
  reminders[idx].nextReviewTime = 0;
  reminders[idx].reviewCount = 0;
  reminderCount++;

  scheduleReminder(reminders[idx]);

  // Force screen refresh to show new reminder immediately
  screenDirty = true;

  Serial.printf("Added reminder id=%d title=%s when=%ld limit=%d\n", reminders[idx].id, reminders[idx].title.c_str(), reminders[idx].when, reminders[idx].limitMinutes);
  request->send(200, "application/json", String("{\"status\":\"added\",\"id\":") + String(reminders[idx].id) + String("}"));
}

void handleListReminders(AsyncWebServerRequest *request) {
  String out = "[";
  bool first = true;
  for (int i = 0; i < MAX_REMINDERS; i++) {
    Reminder &r = reminders[i];
    if (r.id == 0)
      continue;
    if (!first)
      out += ",";
    first = false;
    struct tm tm;
    localtime_r(&r.when, &tm);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm);
    out += "{";
    out += "\"id\":" + String(r.id) + ",";
    out += "\"title\":\"" + r.title + "\",";
    out += "\"message\":\"" + r.message + "\",";
    out += "\"time\":\"" + String(buf) + "\",";
    out += "\"limit\":" + String(r.limitMinutes) + ",";
    out += "\"completed\":" + String(r.completed ? "true" : "false") + ",";
    out += "\"priority\":" + String((r.color == TFT_RED) ? "\"high\"" : ((r.color == TFT_YELLOW) ? "\"medium\"" : "\"normal\""));
    out += "}";
  }
  out += "]";
  request->send(200, "application/json", out);
}

void handleCompleteReminder(AsyncWebServerRequest *request) {
  String idStr = request->hasParam("id", true) ? request->getParam("id", true)->value()
                                               : (request->hasParam("id") ? request->getParam("id")->value() : "");
  if (idStr.length() == 0) {
    request->send(400, "application/json", "{\"error\":\"Missing id\"}");
    return;
  }
  int id = idStr.toInt();
  for (int i = 0; i < MAX_REMINDERS; i++) {
    if (reminders[i].id == id) {
      reminders[i].completed = true;
      reminders[i].triggered = false;
      reminders[i].nextReviewTime = 0;
      reminders[i].reviewCount = 0;
      setNotifLed(0, 0, 0);
      screenDirty = true;
      request->send(200, "application/json", "{\"status\":\"completed\"}");
      Serial.printf("Reminder %d completed\n", id);
      return;
    }
  }
  request->send(404, "application/json", "{\"error\":\"not found\"}");
}

void handleScreenSwitch(AsyncWebServerRequest *request) {
  String name = request->hasParam("name") ? request->getParam("name")->value() : "";
  if (name == "reminder")
    currentScreen = SCREEN_REMINDER;
  else
    currentScreen = SCREEN_NOTIFS;
  screenDirty = true;
  request->send(200, "application/json", "{\"status\":\"ok\",\"screen\":\"" + String((currentScreen == SCREEN_REMINDER) ? "reminder" : "notifs") + "\"}");
}

// motor stuff
// void setMotorRaw(int speed) {
//   speed = constrain(speed, 0, 255);
//   motorSpeed = speed;

//   if (speed == 0) {
//     digitalWrite(MOTOR_IN1, LOW);    // OFF
//     digitalWrite(MOTOR_IN2, LOW);
//     return;
//   }
//   digitalWrite(MOTOR_IN1, HIGH);     // FULL SPEED FORWARD
//   digitalWrite(MOTOR_IN2, LOW);
// }

// POST /motor  body: speed=0..255
// void handleMotorSet(AsyncWebServerRequest *request) {
//   if (!request->hasParam("speed", true)) {
//     request->send(400, "application/json", "{\"error\":\"Missing speed\"}");
//     return;
//   }

//   String s = request->getParam("speed", true)->value();
//   int val = s.toInt();
//   val = constrain(val, 0, 255);
//   setMotorRaw(val);

//   String resp = "{\"speed\":" + String(val) + "}";
//   request->send(200, "application/json", resp);
// }
