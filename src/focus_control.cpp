#include "focus_control.h"
#include "config.h"
#include "state.h"
#include <HTTPClient.h>

// Fire-and-forget POST request (non-blocking, short timeout)
static void sendFocusRequest(const String& ip, int port, bool focusOn) {
  if (ip.length() == 0) return;

  HTTPClient http;
  String url = "http://" + ip + ":" + String(port) + "/focus";
  http.begin(url);
  http.setTimeout(FOCUS_NOTIFY_TIMEOUT);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String body = "focus=" + String(focusOn ? "on" : "off");
  int code = http.POST(body);

  if (code > 0) {
    Serial.printf("Focus %s -> %s:%d = %d\n", focusOn ? "ON" : "OFF", ip.c_str(), port, code);
  } else {
    Serial.printf("Focus %s -> %s:%d FAILED: %s\n", focusOn ? "ON" : "OFF", ip.c_str(), port, http.errorToString(code).c_str());
  }

  http.end();
}

void activateFocusMode() {
  if (focusMode) return;  // Already active
  focusMode = true;
  Serial.println("Focus Mode: ACTIVATED");

  // Notify PC (auto-detected IP from pc_watcher requests)
  if (pcClientIP.length() > 0) {
    sendFocusRequest(pcClientIP, FOCUS_PC_PORT, true);
  } else {
    Serial.println("Focus: PC IP not known yet (no pc_watcher connection)");
  }

  // Notify Phone
  sendFocusRequest(String(FOCUS_PHONE_IP), FOCUS_PHONE_PORT, true);
}

void deactivateFocusMode() {
  if (!focusMode) return;  // Already inactive
  focusMode = false;
  Serial.println("Focus Mode: DEACTIVATED");

  // Notify PC
  if (pcClientIP.length() > 0) {
    sendFocusRequest(pcClientIP, FOCUS_PC_PORT, false);
  }

  // Notify Phone
  sendFocusRequest(String(FOCUS_PHONE_IP), FOCUS_PHONE_PORT, false);
}
