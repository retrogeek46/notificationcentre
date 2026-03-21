# ESP32 Notification Display

[![View Documentation](https://img.shields.io/badge/View-Documentation-cyan?style=for-the-badge&logo=github)](https://retrogeek46.github.io/notificationcentre/)

WiFi-enabled ESP32 notification center with 2.4" TFT display. Shows app-specific icons, sender names, and messages with priority colors. Includes **Now Playing** display and **PC Stats Gaming Overlay**.

## ✨ Features
- 5 notification slots with smooth scrolling
- App-specific icons (Slack, WhatsApp, Telegram + default)
- Dual-color display: **Sender** (priority color) + **Message** (lighter shade)
- Real-time IST clock
- **Now Playing** - Shows currently playing media from Windows
- **Gaming Mode** - Shows PC stats (CPU/GPU temp & usage, RAM, network speed)
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

### 4. PC Watcher Setup (Windows)
The `tools/` folder contains a unified Python script that handles:
- **Now Playing** - Sends currently playing media when not gaming
- **PC Stats** - Sends hardware stats when gaming (auto-detected)

**Prerequisites:**
- Python 3.10+ installed
- ESP32 connected to same network
- (Optional) HWiNFO64 for CPU temperature

**Installation:**
```bash
cd tools
pip install -r requirements.txt
```

**Configuration:**
1. Edit `tools/pc_watcher.py` and update the ESP32 IP:
```python
ESP32_IP = "192.168.1.246"  # Change to your ESP32's IP
```

2. Add your games to `tools/games.txt` (partial matching supported):
```
# Example games.txt
titanfall
celeste
forza
beamng
```

**Running:**
```bash
# Option 1: Run in console (with logs)
python pc_watcher.py

# Option 2: Run hidden in background
pythonw pc_watcher.py

# Option 3: Use the batch script
run_pc_watcher.bat          # Console mode
run_pc_watcher.bat hidden   # Background mode
run_pc_watcher.bat install  # Add to Windows startup
run_pc_watcher.bat remove   # Remove from startup
```

**Logs:** Saved to `tools/pc_watcher.log` (auto-rotates at 1MB)

### 5. HWiNFO Setup (for CPU Temperature)
To get CPU temperature, install [HWiNFO64](https://www.hwinfo.com/) and enable sensor sharing:

1. Open HWiNFO64 → Settings
2. Enable **"Shared Memory Support"** (Gadget support)
3. Restart HWiNFO

The script reads from registry: `HKCU\SOFTWARE\HWiNFO64\VSB`

## 🎮 Gaming Mode
Gaming mode automatically activates when:
- **Active window matches a game** in `games.txt` (partial match)
- **Fullscreen app** is detected

When active, the status bar shows:
```
65 45% 4.2|72 95%|8G|↓12↑2
└─CPU──────┘└─GPU─┘└RAM┘└NET─┘
  (orange)  (magenta)(yellow)(green)
```
- CPU: temp, usage%, speed (GHz)
- GPU: temp, usage%
- RAM: used (GB)
- NET: ↓download ↑upload (Mbps)

## 🚀 API Usage
### POST `/notify` (Form Data)
```
curl -X POST http://notification.local/notify \
  -d "app=slack&from=Alice&message=Lunch ready in 5min&priority=high"
```

### POST `/clear`
```
curl -X POST http://notification.local/clear
```

### POST `/nowplaying`
```
curl -X POST http://notification.local/nowplaying \
  -d "song=Never Gonna Give You Up&artist=Rick Astley"
```

### POST `/gaming`
```
curl -X POST http://notification.local/gaming -d "enabled=1"
curl -X POST http://notification.local/gaming -d "enabled=0"
```

### POST `/pcstats`
```
curl -X POST http://notification.local/pcstats \
  -d "cpu_temp=65&cpu_usage=45&cpu_speed=4.2&gpu_temp=72&gpu_usage=95&ram_used=8&ram_total=16&net_down=12&net_up=2"
```

**Parameters:**
| Endpoint | Param | Description | Example |
|----------|-------|-------------|---------|
| `/notify` | `app` | App name (icon trigger) | `slack` |
| `/notify` | `from` | Sender name | `Alice` |
| `/notify` | `message` | Message (37 chars max) | `Lunch!` |
| `/notify` | `priority` | `high`/`medium`/default | `high` |
| `/nowplaying` | `song` | Song title | `Song Name` |
| `/nowplaying` | `artist` | Artist name | `Artist` |
| `/gaming` | `enabled` | `1` or `0` | `1` |

## 📱 Tasker Integration (Android)
1. **Profile** → Event → **UI > Notification**
2. **Task** → Use variables:
   - `%ntitle` = Username
   - `%ntext` = Message
3. **HTTP Post** to `http://notification.local/notify`

## 🔧 Display Layout
```
┌──────────────────────┐ ← NOTIFICATIONS (Yellow) + Clock (Cyan)
├──────────────────────┤ ← Now Playing / PC Stats bar
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
- **CPU temp shows 0**: Enable HWiNFO Shared Memory Support
- **Gaming mode not activating**: Check `games.txt` has correct game names

## 📸 Screenshots
<img width="2050" height="1154" alt="image" src="https://github.com/user-attachments/assets/a16ce0ad-30aa-4b26-bcbe-0ae15a4fd407" />

## 📝 License
MIT License - Feel free to use and modify!
