#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <TFT_eSPI.h>
#include <time.h>
#include "slack_icon.h"

#define ICON_HEIGHT 16
#define ICON_WIDTH 16

TFT_eSPI tftMain = TFT_eSPI();
WebServer server(80);
String notifications[5] = {"", "", "", "", ""};
uint16_t notificationColors[5] = {TFT_WHITE, TFT_WHITE, TFT_WHITE, TFT_WHITE, TFT_WHITE};

uint16_t getPriorityColor(const char* priority);
String getIcon(String icon);
void handleSimpleNotify();
void handleFormNotify();
void handleClearAll();
void addNotification(String msg, uint16_t color);
void refreshNotifications();
void updateClock();  // Separate clock updater

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

  // slack_map is 2 bytes per pixel, row-major
  const uint8_t* p = slack_icon_map;

  for (int row = 0; row < ICON_HEIGHT; row++) {
    for (int col = 0; col < ICON_WIDTH; col++) {
      uint8_t lo = *p++;
      uint8_t hi = *p++;
      uint16_t rgb565 = (lo << 8) | hi; // LVGL uses little-endian (lo,hi)
      lineBuf[col] = rgb565;
    }
  // Draw this row
  tftMain.pushImage(x, y + row, ICON_WIDTH, 1, lineBuf);
  }
}

void handleClearAll() {
  Serial.println("=== CLEAR ALL NOTIFICATIONS ===");
  
  for (int i = 0; i < 5; i++) {
    notifications[i] = "";
    notificationColors[i] = TFT_WHITE;
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

String getIcon(String icon) {
  if (icon.indexOf("pizza") >= 0 || icon.indexOf("🍕") >= 0) return "[Pizza]";
  if (icon.indexOf("phone") >= 0 || icon.indexOf("📱") >= 0) return "[Phone]";
  return icon.length() > 1 ? "[Icon]" : icon;
}

void handleSimpleNotify() {
  String app = server.arg("app");
  String message = server.arg("message");
  String from = server.arg("from");
  String priority = server.arg("priority");
  
  if (message == "") message = "Notification";
  if (app == "") app = "App";
  if (from == "") from = "Unknown";
  
  String fullMsg = "[" + app + "] From: " + from + ": " + message;
  uint16_t color = getPriorityColor(priority.c_str());
  
  Serial.println("GET: " + fullMsg);
  addNotification(fullMsg, color);
  server.send(200, "text/plain", "OK");
}

void handleFormNotify() {
  Serial.println("=== FORM POST ===");

  String app = server.arg("app");
  String message = server.arg("message");
  String from = server.arg("from");
  String priority = server.arg("priority");

  Serial.printf("Form data - app: [%s], message: [%s], from: [%s], priority: [%s]\n", 
                app.c_str(), message.c_str(), from.c_str(), priority.c_str());

  if (message == "") message = "Notification";
  if (app == "") app = "App";
  if (from == "") from = "Unknown";

  String fullMsg = "[" + app + "] From: " + from + ": " + message;
  uint16_t color = getPriorityColor(priority.c_str());

  Serial.println("Final: " + fullMsg);
  addNotification(fullMsg, color);
  server.send(200, "application/json", "{\"status\":\"OK\"}");
}

void handleRoot() {
  String html = "<h1>Notification Center</h1>";
  html += "<p><b>Working POST (form):</b></p>";
  html += "<p>de>curl -X POST http://notification.local/notify \\<br>";
  html += "-H \"Content-Type: application/x-www-form-urlencoded\" \\<br>";
  html += "-d \"app=Slack&from=Alice&message=Lunch Ready&priority=high\"</code></p>";
  server.send(200, "text/html", html);
}

void addNotification(String msg, uint16_t color) {
  for (int i = 4; i > 0; i--) {
    notifications[i] = notifications[i-1];
    notificationColors[i] = notificationColors[i-1];
  }
  notifications[0] = msg.substring(0, 35);
  notificationColors[0] = color;
  refreshNotifications();
}

void updateClock() {
  // Only update clock area (partial redraw for smoothness)
  tftMain.fillRect(200, 5, 120, 20, TFT_BLACK);  // Clear clock area only
  
  tftMain.setTextSize(2);
  tftMain.setTextColor(TFT_CYAN);
  
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  char timeStr[20];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  
  tftMain.drawString(timeStr, 200, 5);  // Fixed position
}

void refreshNotifications() {
  tftMain.fillScreen(TFT_BLACK);
  
  // HEADER: Single line with title LEFT + clock RIGHT
  tftMain.setTextSize(2);
  
  // Title left
  tftMain.setTextColor(TFT_YELLOW);
  tftMain.drawString("NOTIFICATIONS", 5, 5);
  
  // Initial clock draw
  updateClock();
  
  // Notifications below header (y=65)
  tftMain.setTextSize(2);
  for (int i = 0; i < 5; i++) {
    int y = 30 + i * 35;
    
    if (notifications[i] != "") {
      drawSlackIcon(5, y);
      tftMain.setTextColor(notificationColors[i]);
      tftMain.drawString(notifications[i], 5 + ICON_WIDTH + 5, y);
    }
  }
}
