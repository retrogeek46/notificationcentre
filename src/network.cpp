#include "network.h"
#include "config.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <TFT_eSPI.h>

// External TFT reference for status display
extern TFT_eSPI tft;

static unsigned long lastWifiCheck = 0;

void initWiFi() {
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT);

  // Show connection instructions on screen
  tft.fillScreen(COLOR_BACKGROUND);
  tft.setTextColor(COLOR_HEADER);
  tft.drawString("Connect: NotificationSetup", 10, 10);
  tft.drawString("192.168.4.1", 10, 30);

  if (!wifiManager.autoConnect("NotificationSetup")) {
    tft.fillScreen(COLOR_ERROR);
    tft.drawString("TIMEOUT!", 10, 50);
    delay(5000);
    ESP.restart();
  }

  // Connected successfully
  tft.fillScreen(COLOR_BACKGROUND);
  tft.setTextColor(COLOR_SUCCESS);
  tft.drawString("WiFi OK!", 10, 10);
  tft.drawString(WiFi.localIP().toString(), 10, 30);
  WiFi.setSleep(false);

  // Setup mDNS
  if (!MDNS.begin("notification")) {
    Serial.println("MDNS failed - use IP!");
  } else {
    MDNS.addService("http", "tcp", 80);
  }

  delay(2000);
}

void initNTP() {
  configTime(NTP_TIMEZONE_OFFSET, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP...");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println(" OK");
}

void checkWiFiReconnect() {
  if (millis() - lastWifiCheck > WIFI_CHECK_INTERVAL && WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    lastWifiCheck = millis();
  }
}

String getLocalIP() {
  return WiFi.localIP().toString();
}
