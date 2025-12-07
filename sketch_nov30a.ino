#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <pgmspace.h>
#include "slack_icon.h"
#include <Adafruit_NeoPixel.h>
#include <AsyncTCP.h>
// #define WEBSERVER_H
#include <ESPAsyncWebServer.h>

#define LED_PIN   32
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

int motorSpeed = 50;     // start slow (0-255)

TFT_eSPI tftMain = TFT_eSPI();
AsyncWebServer server(80);

struct Notification {
  String app;
  String from;
  String message;
  uint16_t color;

  Notification() : app(""), from(""), message(""), color(TFT_WHITE) {}
};

Notification notifications[5];
bool screenDirty = true;
unsigned long lastClockUpdate = 0;

uint16_t getPriorityColor(const char* priority);
void drawAppIcon(int x, int y, String app);
void drawSlackIcon(int x, int y);
void handleSimpleNotify(AsyncWebServerRequest *request);
void handleFormNotify(AsyncWebServerRequest *request);
void handleClearAll(AsyncWebServerRequest *request);
void handleRoot(AsyncWebServerRequest *request);
void setNotifLed(uint8_t r, uint8_t g, uint8_t b);
String extractSender(String msg);
void addStructuredNotification(String app, String from, String msg, uint16_t color);
void refreshNotifications();
void resetPreviousTimeStr();
void updateClock();
// void setMotorRaw(int speed);
// void handleMotorSet(AsyncWebServerRequest *request);

void setup() {
  Serial.begin(115200);

  tftMain.init();
  tftMain.fillScreen(TFT_BLACK);
  tftMain.setRotation(3);

  // --- Motor setup ---
  // pinMode(MOTOR_ENA, OUTPUT);  // Already HIGH = full enable
  // pinMode(MOTOR_IN1, OUTPUT);
  // pinMode(MOTOR_IN2, OUTPUT);
  // digitalWrite(MOTOR_ENA, HIGH);  // Full enable always ON
  // Serial.println("Motor PWM ready: " + String(motorSpeed));

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
    // ESP.restart();
  }

  delay(1000);

  tftMain.fillScreen(TFT_BLACK);
  tftMain.setTextColor(TFT_GREEN);
  tftMain.drawString("WiFi OK!", 10, 10);
  tftMain.drawString(WiFi.localIP().toString(), 10, 30);

  WiFi.setSleep(false);  // Optimize WiFi performance

  // NTP Setup for IST (+5:30)
  configTime(5.5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP...");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println(" OK");

  // Initialize notifications
  for (int i = 0; i < 5; i++) {
    notifications[i] = Notification();
  }

  if (!MDNS.begin("notification")) {
    Serial.println("MDNS failed - use IP!");
  } else {
    MDNS.addService("http", "tcp", 80);
  }

  delay(2000);

  server.on("/notify", HTTP_ANY, handleFormNotify);
  server.on("/clear", HTTP_POST, handleClearAll);
  server.on("/", HTTP_GET, handleRoot);
  // server.on("/motor", HTTP_POST, handleMotorSet);
  server.begin();

  refreshNotifications();
  Serial.println("Ready! http://notification.local/notify");
}

void loop() {
  // Serial.printf("Free heap: %d\n", ESP.getFreeHeap());

  // Update clock every second
  if (millis() - lastClockUpdate > 1000) {
    updateClock();
    lastClockUpdate = millis();
  }

  // Refresh screen only when needed (non-blocking for web requests)
  if (screenDirty) {
    refreshNotifications();
    screenDirty = false;
  }

  static unsigned long wifiCheck = 0;
  if (millis() - wifiCheck > 30000 && WiFi.status() != WL_CONNECTED) {  // Check every 30s
    WiFi.reconnect();
    wifiCheck = millis();
  }

  yield();
}

void drawSlackIcon(int x, int y) {
  static uint16_t lineBuf[ICON_WIDTH];
  const uint8_t* p = slack_icon_map;

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

  if (app.indexOf("slack") >= 0) {
    drawSlackIcon(x, y);
  } else if (app.indexOf("whatsapp") >= 0) {
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

void setNotifLed(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

void handleClearAll(AsyncWebServerRequest *request) {
  Serial.println("=== CLEAR ALL NOTIFICATIONS ===");
  for (int i = 0; i < 5; i++) {
    notifications[i] = Notification();
  }
  screenDirty = true;  // Mark for refresh
  setNotifLed(0, 0, 0);
  request->send(200, "application/json", "{\"status\":\"cleared\"}");
}

uint16_t getPriorityColor(const char* priority) {
  if (!priority) return TFT_WHITE;
  if (strcmp(priority, "high") == 0) return TFT_RED;
  if (strcmp(priority, "medium") == 0) return TFT_YELLOW;
  return TFT_WHITE;
}

void addStructuredNotification(String app, String from, String msg, uint16_t color) {
  for (int i = 4; i > 0; i--) {
    notifications[i] = notifications[i-1];
  }

  notifications[0].app = app;
  notifications[0].from = from;
  notifications[0].message = msg.substring(0, 48);
  notifications[0].color = color;

  uint8_t r = 0, g = 0, b = 0;
  if (color == TFT_RED) {
    r = 255; g = 0; b = 0;
  } else if (color == TFT_YELLOW) {
    r = 255; g = 80; b = 0;
  } else {
    r = 80; g = 80; b = 80;
  }
  setNotifLed(r, g, b);
  screenDirty = true;  // Mark for refresh (non-blocking)
}

void handleSimpleNotify(AsyncWebServerRequest *request) {
  String app = request->hasParam("app", true) ? request->getParam("app", true)->value() : "App";
  String from = request->hasParam("from", true) ? request->getParam("from", true)->value() : "Unknown";
  String message = request->hasParam("message", true) ? request->getParam("message", true)->value() : "Notification";
  String priority = request->hasParam("priority", true) ? request->getParam("priority", true)->value() : "";

  Serial.println("SimpleNotify: " + app + " - " + from + " - " + message);
  addStructuredNotification(app, from, message, getPriorityColor(priority.c_str()));
  request->send(200, "text/plain", "OK");
}

String extractSender(String msg) {
  if (msg.length() == 0) return "Unknown";

  int colonIndex = msg.lastIndexOf(':');
  if (colonIndex != -1) {
    String sender = msg.substring(colonIndex + 1);
    sender.trim();
    return sender.length() > 0 ? sender : "Unknown";
  }
  return msg.length() > 0 ? msg : "Unknown";
}

void handleFormNotify(AsyncWebServerRequest *request) {
  unsigned long start = millis();  // DIAGNOSTIC TIMING

  String app = request->hasParam("app", true) ? request->getParam("app", true)->value() : "App";
  String from_raw = request->hasParam("from", true) ? request->getParam("from", true)->value() : "";
  String message = request->hasParam("message", true) ? request->getParam("message", true)->value() : "Notification";
  String priority = request->hasParam("priority", true) ? request->getParam("priority", true)->value() : "";

  String from = extractSender(from_raw);  // FIXED sender extraction

  Serial.printf("FormNotify - app: [%s], from: [%s], message: [%s], priority: [%s]\n",
                app.c_str(), from.c_str(), message.c_str(), priority.c_str());

  addStructuredNotification(app, from, message, getPriorityColor(priority.c_str()));
  Serial.printf("Notification added in %lu ms\n", millis() - start);

  request->send(200, "application/json", "{\"status\":\"OK\"}");
  Serial.printf("RESPONSE SENT in %lu ms TOTAL\n", millis() - start);
}

void handleRoot(AsyncWebServerRequest *request) {
  String html = "<h1>Notification Center</h1>";
  html += "<p><b>GET:</b> /notify?app=slack&from=Alice&message=Hi&priority=high</p>";
  html += "<p><b>POST:</b> curl -X POST http://notification.local/notify -d 'app=slack&from=Bob&message=Test&priority=medium'</p>";
  request->send(200, "text/html", html);
}

char previousTimeStr[25] = "                    ";

void resetPreviousTimeStr() {
  memset(previousTimeStr, 0, sizeof(previousTimeStr));
}

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

void refreshNotifications() {
  tftMain.fillScreen(TFT_BLACK);

  // Header
  tftMain.setTextSize(1);
  tftMain.setTextColor(TFT_YELLOW);
  tftMain.drawString("NOTIFS", 5, 5);
  resetPreviousTimeStr();
  updateClock();

  // Notifications
  tftMain.setTextSize(1);
  for (int i = 0; i < 5; i++) {
    int y = 40 + i * 55;

    if (notifications[i].message != "") {
      drawAppIcon(5, y, notifications[i].app);

      tftMain.setTextColor(notifications[i].color);
      String sender = notifications[i].from;
      if (sender.length() > 24) sender = sender.substring(0, 24);
      tftMain.drawString(sender + ":", 27, y);

      tftMain.setTextColor(TFT_LIGHTGREY);
      String msg = notifications[i].message;
      if (msg.length() > 47) msg = msg.substring(0, 47) + "...";

      String msgLine1 = msg.substring(0, 24);
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
