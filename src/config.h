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

// Default screen on startup (SCREEN_NOTIFS, SCREEN_REMINDER, SCREEN_CALENDAR)
#define DEFAULT_SCREEN SCREEN_CALENDAR

// ===== Screen Zone Boundaries =====
// Title zone: 0-105 x 0-24
#define ZONE_TITLE_X_START 0
#define ZONE_TITLE_X_END 105
#define ZONE_TITLE_Y_START 0
#define ZONE_TITLE_Y_END 24

// Clock zone: 106-319 x 0-24
#define ZONE_CLOCK_X_START 106
#define ZONE_CLOCK_X_END 319
#define ZONE_CLOCK_Y_START 0
#define ZONE_CLOCK_Y_END 24

// Status zone: 0-319 x 25-44
#define ZONE_STATUS_X_START 0
#define ZONE_STATUS_X_END 319
#define ZONE_STATUS_Y_START 25
#define ZONE_STATUS_Y_END 44

// Content zones (3 slots of 65px each)
// Content1: 0-319 x 45-109
#define ZONE_CONTENT1_X_START 0
#define ZONE_CONTENT1_X_END 319
#define ZONE_CONTENT1_Y_START 45
#define ZONE_CONTENT1_Y_END 109

// Content2: 0-319 x 110-174
#define ZONE_CONTENT2_X_START 0
#define ZONE_CONTENT2_X_END 319
#define ZONE_CONTENT2_Y_START 110
#define ZONE_CONTENT2_Y_END 174

// Content3: 0-319 x 175-239
#define ZONE_CONTENT3_X_START 0
#define ZONE_CONTENT3_X_END 319
#define ZONE_CONTENT3_Y_START 175
#define ZONE_CONTENT3_Y_END 239

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
#define MAX_REMINDERS 50

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
#define NOW_PLAYING_MIN_CHARS 24       // Minimum characters for track info (pads with spaces)

// ===== PC Stats Configuration =====
#define PC_STATS_TIMEOUT 3000          // ms - show idle if no stats for this long
#define CPU_TEMP_WARN 85               // Flash red if CPU temp exceeds this
#define GPU_TEMP_WARN 75               // Flash red if GPU temp exceeds this

// ===== Album Art Configuration =====
#define ALBUM_ART_SIZE 18              // 18x18 pixels for status zone (with 1px border = 20px)

// ===== Calendar Layout Configuration =====
// Header / Title (Month Year)
#define CAL_TITLE_X 5          // X position for title (top left)
#define CAL_TITLE_Y 48         // Y position for title (top left of content zone + padding)

#define CAL_X_START 5          // Left margin for the whole calendar
#define CAL_Y_HEADER 25        // Y padding for day headers (Mo, Tu...)
#define CAL_COL_W 47           // Width of each day column
#define CAL_ROW_H 30           // Height of each date row
#define CAL_SEP_Y_OFFSET 10    // Vertical offset for the bg separator line from header Y
#define CAL_GRID_Y_OFFSET 15   // Start of the date grid below the separator line
#define CAL_TEXT_X_OFFSET 2    // Fine-tuning X position of text within a cell
#define CAL_TEXT_Y_OFFSET 0    // Fine-tuning Y position of text within a cell

// Highlight Box (Today's date)
#define CAL_HL_W 30            // Width of the highlight box
#define CAL_HL_H 26            // Height of the highlight box
#define CAL_HL_X_OFF -2        // X offset for highlight box alignment
#define CAL_HL_Y_OFF -4        // Y offset for highlight box alignment
#define CAL_HL_ROUND 4         // Corner radius of the highlight box

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

// Calendar Colors
#define COLOR_CAL_TITLE       TFT_MAROON    // Month/Year text
#define COLOR_CAL_DAY_HEADER  TFT_YELLOW    // Mo, Tu, We... text
#define COLOR_CAL_DATE        TFT_WHITE       // Standard date numbers
#define COLOR_CAL_TODAY_BG    COLOR_HEADER    // Today's highlight box color
#define COLOR_CAL_TODAY_TEXT  TFT_NAVY       // Date number color inside today's highlight

// Notification colors
#define COLOR_NOTIF_MSG TFT_LIGHTGREY

// Reminder colors
#define COLOR_REMINDER_ACTIVE TFT_YELLOW
#define COLOR_REMINDER_INACTIVE TFT_LIGHTGREY
#define COLOR_REMINDER_ICON_ACTIVE TFT_RED
#define COLOR_REMINDER_ICON_INACTIVE TFT_DARKGREY
#define COLOR_REMINDER_DUE TFT_LIGHTGREY

// Priority colors
#define COLOR_PRIORITY_HIGH 0xD5BF  // lavender
#define COLOR_PRIORITY_MEDIUM TFT_ORANGE
#define COLOR_PRIORITY_NORMAL TFT_LIGHTGREY

// App icon colors
#define COLOR_WHATSAPP TFT_GREEN
#define COLOR_TELEGRAM TFT_BLUE
#define COLOR_ICON_DEFAULT TFT_DARKGREY
#define COLOR_ICON_BORDER TFT_WHITE

#endif
