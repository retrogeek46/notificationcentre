#ifndef CONFIG_H
#define CONFIG_H

// ===== Hardware Pins =====
#define LED_PIN 32
#define LED_COUNT 1

// ===== Icon Dimensions =====
#define ICON_HEIGHT 16
#define ICON_WIDTH 16
#define REMINDER_ICON_RADIUS 5

// ===== Display Limits =====
#define MAX_NOTIFICATIONS 5
#define MAX_REMINDERS 10

// ===== Text Truncation Limits =====
#define NOTIF_SENDER_MAX_CHARS 24
#define NOTIF_MSG_MAX_CHARS 47
#define NOTIF_MSG_LINE_CHARS 20
#define REMINDER_MSG_MAX_CHARS 20

// ===== Timing (milliseconds) =====
#define CLOCK_UPDATE_INTERVAL 1000
#define REMINDER_REFRESH_INTERVAL 60000
#define WIFI_CHECK_INTERVAL 30000
#define WIFI_PORTAL_TIMEOUT 1800

// ===== NTP Configuration =====
#define NTP_TIMEZONE_OFFSET (5.5 * 3600)  // IST +5:30

// ===== UI Colors =====
// Include TFT_eSPI before this header to use these
#define COLOR_BACKGROUND TFT_BLACK
#define COLOR_HEADER TFT_YELLOW
#define COLOR_CLOCK TFT_CYAN
#define COLOR_SUCCESS TFT_GREEN
#define COLOR_ERROR TFT_RED

// Notification colors
#define COLOR_NOTIF_MSG TFT_LIGHTGREY

// Reminder colors
#define COLOR_REMINDER_ACTIVE TFT_YELLOW
#define COLOR_REMINDER_INACTIVE TFT_LIGHTGREY
#define COLOR_REMINDER_ICON_ACTIVE TFT_RED
#define COLOR_REMINDER_ICON_INACTIVE TFT_DARKGREY
#define COLOR_REMINDER_DUE TFT_LIGHTGREY

// Priority colors
#define COLOR_PRIORITY_HIGH TFT_RED
#define COLOR_PRIORITY_MEDIUM TFT_YELLOW
#define COLOR_PRIORITY_NORMAL TFT_WHITE

// App icon colors
#define COLOR_WHATSAPP TFT_GREEN
#define COLOR_TELEGRAM TFT_BLUE
#define COLOR_ICON_DEFAULT TFT_DARKGREY
#define COLOR_ICON_BORDER TFT_WHITE

#endif
