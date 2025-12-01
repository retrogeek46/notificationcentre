# ESP32 Notification Display

WiFi-enabled ESP32 notification center with 2.4" TFT display. Shows app-specific icons, sender names, and messages with priority colors. Perfect for Slack/WhatsApp forwarding via Tasker.

## ✨ Features
- 5 notification slots with smooth scrolling
- App-specific icons (Slack, WhatsApp, Telegram + default)
- Dual-color display: **Sender** (priority color) + **Message** (lighter shade)
- Real-time IST clock
- WiFiManager captive portal setup
- mDNS support (`notification.local`)
- Priority colors: **High=Red**, **Medium=Yellow**, **Normal=White**

## 🛠️ Hardware Requirements
```
ESP32 (tested: ESP32-WROOM-32)
2.4" ILI9341 TFT (240x320 SPI)

Connections:
├── VCC → 3.3V
├── GND → GND
├── CS → GPIO 5
├── RST → GPIO 16
├── DC → GPIO 17
├── MOSI→ GPIO 23
├── SCK → GPIO 18
└── LED → 3.3V
```

## 📦 Software Setup
### 1. Install Libraries (Arduino IDE)
WiFiManager (tzapu)
TFT_eSPI (Bodmer)
ESPmDNS

### 2. Configure TFT_eSPI
Edit `Arduino/libraries/TFT_eSPI/User_Setup.h`:
```
#define ILI9341_DRIVER
#define TFT_WIDTH 320
#define TFT_HEIGHT 240
#define TFT_CS 5
#define TFT_DC 17
#define TFT_RST 16
#define SPI_FREQUENCY 40000000
```

### 3. Add Icon File
Create `slack_icon.h` with 16x16 RGB565 bitmap:
```
const uint8_t slack_icon_map[] PROGMEM = {
// 512 bytes (16x16x2) - convert PNG at https://lvgl.io/tools/imageconverter
};
```


## 🚀 API Usage
### POST `/notify` (Form Data)
```
curl -X POST http://notification.local/notify
-d "app=slack&from=Alice&message=Lunch ready in 5min&priority=high"
```

### POST `/clear`
```
curl -X POST http://notification.local/clear
```

**Parameters:**
| Param     | Description              | Example      |
|-----------|--------------------------|--------------|
| `app`     | App name (icon trigger) | `slack`      |
| `from`    | Sender name             | `Alice`      |
| `message` | Message (37 chars max)  | `Lunch!`     |
| `priority`| `high`/`medium`/default | `high`       |

## 📱 Tasker Integration (Android)
1. **Profile** → Event → **UI > Notification**
2. **Task** → Use variables:
   - `%ntitle` = Username
   - `%ntext` = Message
3. **HTTP Post** to `http://notification.local/notify`

## 🔧 Display Layout
```
┌──────────────────────┐ ← NOTIFICATIONS (Yellow) + Clock (Cyan)
│ [Slack] Alice:       │ ← Red sender (high priority)
│ Lunch ready in 5min… │ ← Pink message
│ [WhatsApp] Bob:      │ ← Yellow sender (medium)
│ Meeting now...       │ ← Orange message
│ [Icon] Team:         │ ← White sender (normal)
│ Update complete      │ ← Light grey message
└──────────────────────┘
```

## 🐛 Troubleshooting
- **No display**: Check TFT_eSPI `User_Setup.h` pins
- **Wrong colors**: Verify `slack_icon.h` RGB565 format
- **WiFi timeout**: Initial setup shows `192.168.4.1`
- **API timeout**: Response sent before TFT redraw

## 📸 Screenshots
<img width="2050" height="1154" alt="image" src="https://github.com/user-attachments/assets/a16ce0ad-30aa-4b26-bcbe-0ae15a4fd407" />

## 📝 License
MIT License - Feel free to use and modify!
