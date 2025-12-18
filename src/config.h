#ifndef CONFIG_H
#define CONFIG_H

// ===== Hardware Pins =====
#define LED_PIN 32
#define LED_COUNT 1

// Display rotation (0=portrait, 1=landscape, 2=portrait inverted, 3=landscape inverted)
#define TFT_ROTATION 1

// Motor driver pins (L298N Motor A) - Left side only
#define MOTOR_ENA 33   // PWM speed control
#define MOTOR_IN1 13   // Direction (always HIGH for forward)
// IN2 should be hardwired to GND on L298N (single direction)

// Motor PWM settings
#define MOTOR_PWM_CHANNEL 0
#define MOTOR_PWM_FREQ    1000   // 1 kHz
#define MOTOR_PWM_RES     8      // 8-bit = 0-255

// Button pins (input-only GPIOs, external pull-up required)
#define BTN_CLEAR_NOTIFS  35     // Clear all notifications
#define BTN_DEBOUNCE_MS   50     // Debounce delay in ms

// Rotary encoder pins (input-only GPIOs, external pull-up required)
#define ENCODER_CLK       36     // Rotation signal A
#define ENCODER_DT        39     // Rotation signal B
#define ENCODER_SW        34     // Push button
#define ENCODER_SPEED_STEP 15    // Speed change per click
#define ENCODER_MIN_SPEED  50    // Minimum motor speed

// ===== Icon Dimensions =====
#define ICON_HEIGHT 16
#define ICON_WIDTH 16
#define REMINDER_ICON_RADIUS 5

// ===== Display Limits =====
#define MAX_NOTIFICATIONS 5
#define MAX_REMINDERS 10

// ===== Text Truncation Limits =====
#define NOTIF_SENDER_MAX_CHARS 24
#define NOTIF_MSG_MAX_CHARS 56
#define NOTIF_MSG_LINE_CHARS 28
#define REMINDER_MSG_MAX_CHARS 28

// ===== Timing (milliseconds) =====
#define CLOCK_UPDATE_INTERVAL 1000
#define REMINDER_REFRESH_INTERVAL 60000
#define WIFI_CHECK_INTERVAL 30000
#define WIFI_PORTAL_TIMEOUT 1800

// ===== Now Playing Configuration =====
#define NOW_PLAYING_SCROLL_SPEED 200   // ms between scroll steps (slower)
#define NOW_PLAYING_DISC_SPEED 400     // ms between disc rotation frames (slower)
#define NOW_PLAYING_TIMEOUT 300000     // 5 min auto-clear
#define NOW_PLAYING_TEXT_X 22          // Text start X (after disc icon)
#define NOW_PLAYING_TEXT_WIDTH 298     // Available width for text (320 - 22)

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
