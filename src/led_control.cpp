#include "led_control.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>

static Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void initLed() {
  strip.begin();
  strip.setBrightness(50);
  strip.show();
}

void updateLedForScreen(Screen screen) {
  switch (screen) {
    case SCREEN_NOTIFS:
      setLedColor(255, 0, 0);  // Red for notifications
      break;
    case SCREEN_REMINDER:
      setLedColor(255, 80, 0);  // Orange/Yellow for reminders
      break;
    default:
      ledOff();
      break;
  }
}

void blinkLed(int times, int delayMs) {
  uint32_t currentColor = strip.getPixelColor(0);
  
  for (int i = 0; i < times; i++) {
    strip.setPixelColor(0, 0);
    strip.show();
    delay(delayMs);
    strip.setPixelColor(0, currentColor);
    strip.show();
    delay(delayMs);
  }
}

void setLedColor(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

void ledOff() {
  strip.setPixelColor(0, 0);
  strip.show();
}
