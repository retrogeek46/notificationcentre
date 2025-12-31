#ifndef CONFIG_H
#define CONFIG_H

// ===== Hardware Pins =====
#define LED_PIN 32
#define LED_COUNT 1

// Display rotation (0=portrait, 1=landscape, 2=portrait inverted, 3=landscape inverted)
#define TFT_ROTATION 3

// Debugging
#define DEBUG_SHOW_ZONES 0
#define SPRITE_BG_ENABLED 1  // Set to 1 to enable sprite backgrounds, 0 for solid fill

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
#define ENCODER_ENABLED   0      // Set to 1 when encoder is wired
#define ENCODER_CLK       36     // Rotation signal A
#define ENCODER_DT        39     // Rotation signal B
#define ENCODER_SW        34     // Push button
#define ENCODER_SPEED_STEP 15    // Speed change per click
#define ENCODER_MIN_SPEED  50    // Minimum motor speed

// ===== Icon Dimensions =====
#define ICON_HEIGHT 14
#define ICON_WIDTH 14
#define REMINDER_ICON_RADIUS 7

// ===== Display Limits =====
#define MAX_NOTIFICATIONS 5
#define MAX_REMINDERS 10

// ===== Text Truncation Limits =====
#define NOTIF_SENDER_MAX_CHARS 24
#define NOTIF_MSG_MAX_CHARS 68
#define NOTIF_MSG_LINE_CHARS 35
#define REMINDER_MSG_MAX_CHARS 68

// ===== Timing (milliseconds) =====
#define CLOCK_UPDATE_INTERVAL 1000
#define REMINDER_REFRESH_INTERVAL 60000
#define WIFI_CHECK_INTERVAL 30000
#define WIFI_PORTAL_TIMEOUT 1800

// ===== Now Playing Configuration =====
#define NOW_PLAYING_SCROLL_SPEED 50    // ms between scroll steps (20 FPS)
#define NOW_PLAYING_SCROLL_STEP 1      // pixels to scroll per step
#define NOW_PLAYING_DISC_SPEED 50      // ms between disc rotation frames (64 frames)
#define IDLE_DISC_TRAVEL_SPEED 60      // ms between horizontal movement steps (idle mode)
#define NOW_PLAYING_TEXT_X 22          // Text start X (after disc icon)
#define NOW_PLAYING_TEXT_WIDTH 298     // Available width for text (320 - 22)
#define NOW_PLAYING_MIN_CHARS 25       // Minimum characters for track info (pads with spaces)

// ===== PC Stats Configuration =====
#define PC_STATS_TIMEOUT 3000          // ms - show idle if no stats for this long
#define CPU_TEMP_WARN 85               // Flash red if CPU temp exceeds this
#define GPU_TEMP_WARN 75               // Flash red if GPU temp exceeds this

// ===== Album Art Configuration =====
#define ALBUM_ART_SIZE 18              // 18x18 pixels for status zone (with 1px border = 20px)

// ===== NTP Configuration =====
#define NTP_TIMEZONE_OFFSET (5.5 * 3600)  // IST +5:30

// ===== UI Colors =====
// Include TFT_eSPI before this header to use these
#define COLOR_BACKGROUND TFT_BLACK
#define COLOR_HEADER TFT_YELLOW
#define COLOR_CLOCK TFT_CYAN
#define COLOR_SUCCESS TFT_GREEN
#define COLOR_ERROR TFT_RED

// PC Stats colors (matching scrolling text color)
#define COLOR_CPU    TFT_GREEN
#define COLOR_GPU    TFT_ORANGE
#define COLOR_RAM    TFT_GOLD
#define COLOR_NET    TFT_SKYBLUE
#define COLOR_SEP    TFT_PURPLE

// Now Playing color
#define COLOR_NOW_PLAYING TFT_MAGENTA

// Notification colors
#define COLOR_NOTIF_MSG TFT_LIGHTGREY

// Reminder colors
#define COLOR_REMINDER_ACTIVE TFT_YELLOW
#define COLOR_REMINDER_INACTIVE TFT_LIGHTGREY
#define COLOR_REMINDER_ICON_ACTIVE TFT_RED
#define COLOR_REMINDER_ICON_INACTIVE TFT_DARKGREY
#define COLOR_REMINDER_DUE TFT_LIGHTGREY

// Priority colors
#define COLOR_PRIORITY_HIGH 0xFC1F      // Magenta
#define COLOR_PRIORITY_MEDIUM 0xFD20    // Orange
#define COLOR_PRIORITY_NORMAL TFT_LIGHTGREY

// App icon colors
#define COLOR_WHATSAPP TFT_GREEN
#define COLOR_TELEGRAM TFT_BLUE
#define COLOR_ICON_DEFAULT TFT_DARKGREY
#define COLOR_ICON_BORDER TFT_WHITE

#endif
