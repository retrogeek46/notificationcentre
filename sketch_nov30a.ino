#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <pgmspace.h>  // For PROGMEM reading
#include "slack_icon.h"

#define ICON_HEIGHT 16
#define ICON_WIDTH 16

TFT_eSPI tftMain = TFT_eSPI();
WebServer server(80);

// Structured notification system
struct Notification {
  String app;
  String from;
  String message;
  uint16_t color;
  
  Notification() : app(""), from(""), message(""), color(TFT_WHITE) {}
};

Notification notifications[5];

uint16_t getPriorityColor(const char* priority);
void drawAppIcon(int x, int y, String app);
void drawSlackIcon(int x, int y);
void handleSimpleNotify();
void handleFormNotify();
void handleClearAll();
void handleRoot();
void addStructuredNotification(String app, String from, String msg, uint16_t color);
void refreshNotifications();
void updateClock();

unsigned long lastClockUpdate = 0;

void setup() {
  Serial.begin(115200);
  
  tftMain.init();
  tftMain.fillScreen(TFT_BLACK);
  tftMain.setRotation(3);
  
  tftMain.setTextSize(1);
  tftMain.setTextColor(TFT_YELLOW);
  tftMain.drawString("Connect: NotificationSetup", 10, 10);
  tftMain.drawString("192.168.4.1", 10, 30);
  
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(1800);
  
  if (!wifiManager.autoConnect("NotificationSetup")) {
    tftMain.fillScreen(TFT_RED);
    tftMain.drawString("TIMEOUT!", 10, 50);
    delay(5000);
    ESP.restart();
  }
  
  tftMain.fillScreen(TFT_BLACK);
  tftMain.setTextColor(TFT_GREEN);
  tftMain.drawString("WiFi OK!", 10, 10);
  tftMain.drawString(WiFi.localIP().toString(), 10, 30);
  
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
  
  MDNS.begin("notification");
  MDNS.addService("http", "tcp", 80);
  
  server.on("/notify", HTTP_GET, handleSimpleNotify);
  server.on("/notify", HTTP_POST, handleFormNotify);
  server.on("/clear", HTTP_POST, handleClearAll);
  server.on("/", handleRoot);
  server.begin();
  
  refreshNotifications();
  Serial.println("Ready! http://notification.local/notify");
}

void loop() {
  server.handleClient();
  
  // Update clock every second
  if (millis() - lastClockUpdate > 1000) {
    updateClock();
    lastClockUpdate = millis();
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    tftMain.setTextSize(1);
    tftMain.setTextColor(TFT_RED);
    tftMain.drawString("WiFi Reconnect...", 10, 100);
  }
  delay(10);
}

void drawSlackIcon(int x, int y) {
  static uint16_t lineBuf[ICON_WIDTH];
  const uint8_t* p = slack_icon_map;

  for (int row = 0; row < ICON_HEIGHT; row++) {
    for (int col = 0; col < ICON_WIDTH; col++) {
      uint8_t lo = pgm_read_byte(p++);
      uint8_t hi = pgm_read_byte(p++);
      uint16_t rgb565 = (lo << 8) | hi;  // Fixed byte order for TFT_eSPI
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
    // WhatsApp placeholder (green circle)
    tftMain.fillCircle(x + 8, y + 8, 7, TFT_GREEN);
    tftMain.drawCircle(x + 8, y + 8, 7, TFT_WHITE);
  } else if (app.indexOf("telegram") >= 0) {
    // Telegram placeholder (blue circle)
    tftMain.fillCircle(x + 8, y + 8, 7, TFT_BLUE);
    tftMain.drawCircle(x + 8, y + 8, 7, TFT_WHITE);
  } else {
    // Default app icon (grey square with border)
    tftMain.fillRect(x, y, ICON_WIDTH, ICON_HEIGHT, TFT_DARKGREY);
    tftMain.drawRect(x, y, ICON_WIDTH, ICON_HEIGHT, TFT_WHITE);
  }
}

void handleClearAll() {
  Serial.println("=== CLEAR ALL NOTIFICATIONS ===");
  for (int i = 0; i < 5; i++) {
    notifications[i] = Notification();
  }
  refreshNotifications();
  server.send(200, "application/json", "{\"status\":\"cleared\"}");
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
  
  refreshNotifications();
}

void handleSimpleNotify() {
  String app = server.arg("app");
  String from = server.arg("from");
  String message = server.arg("message");
  String priority = server.arg("priority");
  
  if (message == "") message = "Notification";
  if (app == "") app = "App";
  if (from == "") from = "Unknown";
  
  Serial.println("App: " + app + ", From: " + from + ", Msg: " + message);
  addStructuredNotification(app, from, message, getPriorityColor(priority.c_str()));
  server.send(200, "text/plain", "OK");
}

void handleFormNotify() {
  Serial.println("=== FORM POST ===");
  
  String app = server.arg("app");
  String from = server.arg("from");
  String message = server.arg("message");
  String priority = server.arg("priority");
  
  Serial.printf("Form data - app: [%s], from: [%s], message: [%s], priority: [%s]\n", 
                app.c_str(), from.c_str(), message.c_str(), priority.c_str());
  
  if (message == "") message = "Notification";
  if (app == "") app = "App";
  if (from == "") from = "Unknown";
  
  addStructuredNotification(app, from, message, getPriorityColor(priority.c_str()));
  server.send(200, "application/json", "{\"status\":\"OK\"}");
}

void handleRoot() {
  String html = "<h1>Notification Center</h1>";
  html += "<p><b>POST example:</b></p>";
  html += "<p>curl -X POST http://notification.local/notify \\<br>";
  html += "-d \"app=slack&from=Alice&message=Lunch Ready&priority=high\"</code></p>";
  html += "<p><b>GET example:</b></p>";
  html += "<p>http://notification.local/notify?app=whatsapp&from=Bob&message=Meeting&priority=medium</code></p>";
  server.send(200, "text/html", html);
}

void updateClock() {
  tftMain.fillRect(200, 5, 120, 20, TFT_BLACK);
  tftMain.setTextSize(2);
  tftMain.setTextColor(TFT_CYAN);
  
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  char timeStr[20];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  
  tftMain.drawString(timeStr, 200, 5);
}

void refreshNotifications() {
  tftMain.fillScreen(TFT_BLACK);
  
  // Header
  tftMain.setTextSize(2);
  tftMain.setTextColor(TFT_YELLOW);
  tftMain.drawString("NOTIFICATIONS", 5, 5);
  updateClock();
  
  // Notifications - FONT SIZE 2 + SENDER/MESSAGE COLORS + 27 CHAR LIMIT
  tftMain.setTextSize(2);
  for (int i = 0; i < 5; i++) {
    int y = 40 + i * 55;
    
    if (notifications[i].message != "") {
      // App icon (16x16)
      drawAppIcon(5, y, notifications[i].app);
      
      // Sender name (bold color - priority color)
      tftMain.setTextColor(notifications[i].color);
      String sender = notifications[i].from;
      if (sender.length() > 10) sender = sender.substring(0, 10);  // Max 10 chars sender
      tftMain.drawString(sender + ":", 27, y);
      
      // Message text (lighter shade of priority color)
      uint16_t msgColor =  TFT_LIGHTGREY;
      // uint16_t msgColor = notifications[i].color;
      // if (msgColor == TFT_RED) msgColor = TFT_PINK;
      // else if (msgColor == TFT_YELLOW) msgColor = TFT_ORANGE;
      // else msgColor = TFT_LIGHTGREY;
      
      tftMain.setTextColor(msgColor);
      
      String msg = notifications[i].message;
      if (msg.length() > 47) {
        msg = msg.substring(0, 47) + "...";  // Truncate + ellipsis
      }
      
      // Wrap message at ~13 chars per line (fits with sender above)
      String msgLine1 = msg.substring(0, 24);
      tftMain.drawString(msgLine1, 27, y + 18);
      
      if (msg.length() > 24) {
        String msgLine2 = msg.substring(24);
        tftMain.drawString(msgLine2, 27, y + 36);
      }
    }
  }
}


