#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <pgmspace.h>  // For PROGMEM reading
#include "slack_icon.h"
#include <Adafruit_NeoPixel.h>

#define LED_PIN   32      // GPIO connected to Din of strip
#define LED_COUNT 1      // We'll only use the first LED

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

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
void setNotifLed();
String extractSender(String msg);
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
  
  tftMain.setFreeFont(&FreeMono9pt7b);
  tftMain.setTextSize(1);
  tftMain.setTextColor(TFT_YELLOW);
  tftMain.drawString("Connect: NotificationSetup", 10, 10);
  tftMain.drawString("192.168.4.1", 10, 30);

  strip.begin();
  strip.setBrightness(50);   // 0–255, adjust if too bright
  strip.show();
  
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

void setNotifLed(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

void handleClearAll() {
  Serial.println("=== CLEAR ALL NOTIFICATIONS ===");
  for (int i = 0; i < 5; i++) {
    notifications[i] = Notification();
  }
  refreshNotifications();
  setNotifLed(0, 0, 0);   // LED off
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

  uint8_t r = 0, g = 0, b = 0;
  if (color == TFT_RED) {
    // high
    r = 255; g = 0;   b = 0;
  } else if (color == TFT_YELLOW) {
    // medium
    r = 255; g = 80;  b = 0;
  } else {
    // low/other
    r = 80;  g = 80;  b = 80;
  }
  setNotifLed(r, g, b);
  
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

String extractSender(String msg) {
  Serial.println("extractSender called");
  
  if (msg == NULL || msg.length() == 0) {
    Serial.println("Input message is null or empty");
    return msg;  // Return empty if input is null or empty
  }
  
  Serial.print("Input message: ");
  Serial.println(msg);
  
  int colonIndex = msg.lastIndexOf(':');  // Use lastIndexOf to get colon closest to end
  Serial.print("Last colon index: ");
  Serial.println(colonIndex);
  
  if (colonIndex != -1) {
    String sender = msg.substring(colonIndex + 1);
    sender.trim(); // Remove any leading/trailing spaces
    Serial.print("Extracted sender: ");
    Serial.println(sender);
    return sender;
  }
  
  Serial.println("Failed to extract sender - no colon found");
  return msg;
}

void handleFormNotify() {
  Serial.println("=== FORM POST ===");
  
  String app = server.arg("app");
  String from = extractSender(server.arg("from"));
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
  int maxCharWidth = tftMain.textWidth("W");  // "W" is typically widest character
  int cursorX = x;

  for (int i = 0; i < strlen(timeStr); i++) {
    if (timeStr[i] != previousTimeStr[i] || previousTimeStr[i] == '\0') {
      // Always clear fixed max width to eliminate artifacts
      tftMain.fillRect(cursorX, y, maxCharWidth, charHeight, TFT_BLACK);
      
      // Draw new character
      char ch[2] = {timeStr[i], '\0'};
      tftMain.drawString(ch, cursorX, y);
    }
    previousTimeStr[i] = timeStr[i];
    cursorX += maxCharWidth;  // Fixed spacing prevents overlap
  }
}

void refreshNotifications() {
  tftMain.fillScreen(TFT_BLACK);
  // tftMain.fillRect(0, 30, tftMain.width(), tftMain.height() - 30, TFT_BLACK);
  
  // Header
  tftMain.setTextSize(1);
  tftMain.setTextColor(TFT_YELLOW);
  tftMain.drawString("NOTIFS", 5, 5);
  resetPreviousTimeStr();
  updateClock();
  
  // Notifications - FONT SIZE 2 + SENDER/MESSAGE COLORS + 27 CHAR LIMIT
  tftMain.setTextSize(1);
  for (int i = 0; i < 5; i++) {
    int y = 40 + i * 55;
    
    if (notifications[i].message != "") {
      // App icon (16x16)
      drawAppIcon(5, y, notifications[i].app);
      
      // Sender name (bold color - priority color)
      tftMain.setTextColor(notifications[i].color);
      String sender = notifications[i].from;
      if (sender.length() > 24) sender = sender.substring(0, 24);  // Max 10 chars sender
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


