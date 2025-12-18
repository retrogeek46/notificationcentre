"""
PC Watcher for ESP32 Notification Center

Unified script that handles:
- Media watching: Sends now playing info when NOT gaming
- PC stats: Sends hardware stats when gaming

Gaming mode activates automatically when:
- Active window matches a game in games.txt
- Active window is fullscreen

Usage: pythonw pc_watcher.py  (hidden window)
   or: python pc_watcher.py   (with console)
"""

import asyncio
import logging
import os
from datetime import datetime
from logging.handlers import RotatingFileHandler

import psutil
import requests

# Windows Media Session API
from winrt.windows.media.control import (
    GlobalSystemMediaTransportControlsSessionManager as MediaManager,
    GlobalSystemMediaTransportControlsSessionPlaybackStatus as PlaybackStatus,
)

# NVIDIA GPU monitoring
try:
    import pynvml
    NVML_AVAILABLE = True
except ImportError:
    NVML_AVAILABLE = False
    print("WARNING: pynvml not installed - GPU stats unavailable")

# Windows API for active window detection
try:
    import win32gui
    import win32process
    import win32api
    import win32con
    WIN32_AVAILABLE = True
except ImportError:
    WIN32_AVAILABLE = False
    print("WARNING: pywin32 not installed - game detection unavailable")

# CPU temperature via WMI (Windows)
try:
    import wmi
    WMI_AVAILABLE = True
except ImportError:
    WMI_AVAILABLE = False

# Configuration
ESP32_IP = "192.168.1.246"
ESP32_NOWPLAYING_URL = f"http://{ESP32_IP}/nowplaying"
ESP32_STATS_URL = f"http://{ESP32_IP}/pcstats"
ESP32_GAMING_URL = f"http://{ESP32_IP}/gaming"

POLL_INTERVAL = 0.25  # seconds - fast for media detection
STATS_INTERVAL = 1.0  # seconds - slower for PC stats
REQUEST_TIMEOUT = 2   # seconds

# Paths
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
GAMES_FILE = os.path.join(SCRIPT_DIR, "games.txt")
LOG_FILE = os.path.join(SCRIPT_DIR, "pc_watcher.log")
LOG_MAX_BYTES = 1024 * 1024  # 1 MB
LOG_BACKUP_COUNT = 2


def setup_logging():
    logger = logging.getLogger("PCWatcher")
    logger.setLevel(logging.DEBUG)

    file_handler = RotatingFileHandler(
        LOG_FILE, maxBytes=LOG_MAX_BYTES, backupCount=LOG_BACKUP_COUNT, encoding="utf-8"
    )
    file_handler.setLevel(logging.DEBUG)
    file_format = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s", datefmt="%Y-%m-%d %H:%M:%S")
    file_handler.setFormatter(file_format)

    console_handler = logging.StreamHandler()
    console_handler.setLevel(logging.INFO)
    console_format = logging.Formatter("[%(levelname)s] %(message)s")
    console_handler.setFormatter(console_format)

    logger.addHandler(file_handler)
    logger.addHandler(console_handler)

    return logger


log = setup_logging()


def load_games_list():
    """Load list of game executables from games.txt"""
    games = set()
    if os.path.exists(GAMES_FILE):
        with open(GAMES_FILE, "r") as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith("#"):
                    games.add(line.lower())
        log.info(f"Loaded {len(games)} games from games.txt")
    else:
        log.warning(f"games.txt not found at {GAMES_FILE}")
    return games


class HWiNFOReader:
    """Helper class to read sensor data from HWiNFO64 registry"""

    REGISTRY_PATH = r"SOFTWARE\HWiNFO64\VSB"

    def __init__(self):
        self.available = False
        self._check_available()

    def _check_available(self):
        """Check if HWiNFO registry is accessible"""
        try:
            import winreg
            key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, self.REGISTRY_PATH)
            winreg.CloseKey(key)
            self.available = True
            log.info("HWiNFO64 registry detected")
        except Exception:
            self.available = False

    def read_sensors(self):
        """Read all sensor data from registry, returns dict of {label: raw_value}"""
        if not self.available:
            return {}

        try:
            import winreg
            key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, self.REGISTRY_PATH)

            labels = {}
            raw_values = {}
            i = 0

            while True:
                try:
                    name, data, _ = winreg.EnumValue(key, i)
                    if name.startswith("Label"):
                        idx = name[5:]  # Get number after "Label"
                        labels[idx] = data
                    elif name.startswith("ValueRaw"):
                        idx = name[8:]  # Get number after "ValueRaw"
                        raw_values[idx] = data
                    i += 1
                except OSError:
                    break

            winreg.CloseKey(key)

            # Build dict mapping label -> raw value
            sensors = {}
            for idx, label in labels.items():
                if idx in raw_values:
                    sensors[label] = raw_values[idx]

            return sensors

        except Exception as e:
            log.debug(f"HWiNFO registry read error: {e}")
            return {}

    def get_sensor_by_keywords(self, sensors, *keywords):
        """Find a sensor value by matching keywords in label (case-insensitive)"""
        for label, value in sensors.items():
            label_lower = label.lower()
            if all(kw.lower() in label_lower for kw in keywords):
                return value
        return None


def get_active_window_exe():
    """Get the executable name of the currently active window"""
    if not WIN32_AVAILABLE:
        return None

    try:
        hwnd = win32gui.GetForegroundWindow()
        if hwnd == 0:
            return None

        _, pid = win32process.GetWindowThreadProcessId(hwnd)
        if pid <= 0:
            return None

        try:
            proc = psutil.Process(pid)
            return proc.name().lower()
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            return None
    except Exception as e:
        log.debug(f"Error getting active window: {e}")
        return None


def is_fullscreen():
    """Check if the active window is fullscreen"""
    if not WIN32_AVAILABLE:
        return False

    try:
        hwnd = win32gui.GetForegroundWindow()
        if hwnd == 0:
            return False

        # Get window rect
        rect = win32gui.GetWindowRect(hwnd)
        window_width = rect[2] - rect[0]
        window_height = rect[3] - rect[1]

        # Get screen dimensions
        screen_width = win32api.GetSystemMetrics(win32con.SM_CXSCREEN)
        screen_height = win32api.GetSystemMetrics(win32con.SM_CYSCREEN)

        # Check if window covers the entire screen
        is_fs = (window_width >= screen_width and window_height >= screen_height)

        # Also check for borderless fullscreen (window position at 0,0)
        if is_fs or (rect[0] <= 0 and rect[1] <= 0 and
                     window_width >= screen_width - 2 and
                     window_height >= screen_height - 2):
            return True

        return False
    except Exception as e:
        log.debug(f"Error checking fullscreen: {e}")
        return False


class PCWatcher:
    def __init__(self):
        # Gaming detection
        self.games_list = load_games_list()
        self.gaming_mode = False

        # Media state
        self.last_song = ""
        self.last_artist = ""
        self.last_playing = False
        self.session_mgr = None

        # PC stats state
        self.last_net_recv = 0
        self.last_net_sent = 0
        self.last_net_time = 0
        self.last_stats_update = 0
        self.nvml_initialized = False
        self.wmi_instance = None

        # Initialize HWiNFO reader (primary source for CPU stats)
        self.hwinfo = HWiNFOReader()

        # Initialize NVML for GPU monitoring
        if NVML_AVAILABLE:
            try:
                pynvml.nvmlInit()
                self.nvml_initialized = True
                log.info("NVML initialized successfully")
            except Exception as e:
                log.warning(f"Failed to initialize NVML: {e}")

        # Initialize WMI for CPU temp (fallback)
        if WMI_AVAILABLE:
            try:
                self.wmi_instance = wmi.WMI(namespace="root\\wmi")
                log.info("WMI initialized successfully")
            except Exception as e:
                log.warning(f"Failed to initialize WMI: {e}")

    # ==================== Gaming Detection ====================

    def check_gaming_mode(self):
        """Check if gaming mode should be active"""
        # Check active window against games list (partial match)
        active_exe = get_active_window_exe()
        if active_exe:
            for game in self.games_list:
                if game in active_exe or active_exe in game:
                    return True

        # Check for fullscreen
        if is_fullscreen():
            return True

        return False

    def send_gaming_mode(self, enabled: bool):
        """Send gaming mode toggle to ESP32"""
        try:
            data = {"enabled": "1" if enabled else "0"}
            response = requests.post(ESP32_GAMING_URL, data=data, timeout=REQUEST_TIMEOUT)
            if response.status_code == 200:
                log.info(f"Gaming mode: {'ON' if enabled else 'OFF'}")
            else:
                log.warning(f"ESP32 gaming returned: {response.status_code}")
        except requests.exceptions.RequestException as e:
            log.warning(f"ESP32 unreachable: {e}")

    # ==================== Media Watching ====================

    async def get_media_info(self):
        """Get current media session info."""
        try:
            if not self.session_mgr:
                self.session_mgr = await MediaManager.request_async()
                log.debug("Media session manager initialized")

            session = self.session_mgr.get_current_session()
            if not session:
                return None, None, False

            # Get playback status
            playback_info = session.get_playback_info()
            is_playing = playback_info.playback_status == PlaybackStatus.PLAYING

            # Get media properties
            props = await session.try_get_media_properties_async()
            title = props.title or ""
            artist = props.artist or ""

            return title, artist, is_playing

        except Exception as e:
            log.error(f"Error getting media info: {e}", exc_info=True)
            return None, None, False

    def send_now_playing(self, song: str, artist: str):
        """Send now playing info to ESP32."""
        try:
            data = {"song": song, "artist": artist}
            response = requests.post(ESP32_NOWPLAYING_URL, data=data, timeout=REQUEST_TIMEOUT)
            if response.status_code == 200:
                if song:
                    log.info(f"PLAYING: {song} - {artist}")
                else:
                    log.info("PAUSED: Cleared display")
            else:
                log.warning(f"ESP32 returned status: {response.status_code}")
        except requests.exceptions.RequestException as e:
            log.warning(f"ESP32 unreachable: {e}")

    # ==================== PC Stats Collection ====================

    def get_cpu_temp(self):
        """Get CPU temperature in Celsius"""
        # Try HWiNFO64 first (uses ValueRaw for clean numeric values)
        if self.hwinfo.available:
            sensors = self.hwinfo.read_sensors()
            # Look for CPU temp sensor (try various common label patterns)
            temp = self.hwinfo.get_sensor_by_keywords(sensors, "cpu", "temp")
            if temp is None:
                temp = self.hwinfo.get_sensor_by_keywords(sensors, "package", "temp")
            if temp is None:
                temp = self.hwinfo.get_sensor_by_keywords(sensors, "core", "temp")
            if temp is not None:
                try:
                    return int(float(temp))
                except (ValueError, TypeError):
                    pass

        # Fallback: Try WMI thermal zone
        if self.wmi_instance:
            try:
                temps = self.wmi_instance.MSAcpi_ThermalZoneTemperature()
                if temps:
                    temp_k = temps[0].CurrentTemperature / 10.0
                    temp_c = temp_k - 273.15
                    if 0 < temp_c < 150:
                        return int(temp_c)
            except Exception:
                pass

        # Fallback: try psutil sensors
        try:
            temps = psutil.sensors_temperatures()
            if "coretemp" in temps:
                return int(temps["coretemp"][0].current)
            elif "cpu_thermal" in temps:
                return int(temps["cpu_thermal"][0].current)
        except Exception:
            pass

        return 0  # Unknown

    def get_cpu_usage(self):
        """Get CPU usage percentage"""
        # Try HWiNFO64 first
        if self.hwinfo.available:
            sensors = self.hwinfo.read_sensors()
            usage = self.hwinfo.get_sensor_by_keywords(sensors, "cpu", "usage")
            if usage is None:
                usage = self.hwinfo.get_sensor_by_keywords(sensors, "total", "usage")
            if usage is not None:
                try:
                    return int(float(usage))
                except (ValueError, TypeError):
                    pass

        # Fallback to psutil
        return int(psutil.cpu_percent(interval=None))

    def get_cpu_speed(self):
        """Get current CPU frequency in GHz"""
        try:
            freq = psutil.cpu_freq()
            if freq:
                return round(freq.current / 1000.0, 1)  # MHz to GHz
        except Exception:
            pass
        return 0.0

    def get_ram_usage(self):
        """Get RAM usage (used, total) in GB"""
        mem = psutil.virtual_memory()
        used_gb = int(mem.used / (1024 ** 3))
        total_gb = int(mem.total / (1024 ** 3))
        return used_gb, total_gb

    def get_gpu_stats(self):
        """Get GPU temperature and usage from NVIDIA GPU"""
        if not self.nvml_initialized:
            return 0, 0

        try:
            handle = pynvml.nvmlDeviceGetHandleByIndex(0)  # First GPU
            temp = pynvml.nvmlDeviceGetTemperature(handle, pynvml.NVML_TEMPERATURE_GPU)
            util = pynvml.nvmlDeviceGetUtilizationRates(handle)
            return int(temp), int(util.gpu)
        except Exception as e:
            log.debug(f"Error getting GPU stats: {e}")
            return 0, 0

    def get_net_speed(self):
        """Get current download and upload speed in Mbps"""
        try:
            net = psutil.net_io_counters()
            current_recv = net.bytes_recv
            current_sent = net.bytes_sent
            current_time = asyncio.get_event_loop().time()

            if self.last_net_time > 0:
                elapsed = current_time - self.last_net_time
                if elapsed > 0:
                    recv_per_sec = (current_recv - self.last_net_recv) / elapsed
                    sent_per_sec = (current_sent - self.last_net_sent) / elapsed
                    down_mbps = (recv_per_sec * 8) / (1024 * 1024)
                    up_mbps = (sent_per_sec * 8) / (1024 * 1024)
                    self.last_net_recv = current_recv
                    self.last_net_sent = current_sent
                    self.last_net_time = current_time
                    return round(down_mbps, 1), round(up_mbps, 1)

            self.last_net_recv = current_recv
            self.last_net_sent = current_sent
            self.last_net_time = current_time
            return 0.0, 0.0
        except Exception:
            return 0.0, 0.0

    def send_stats(self):
        """Collect and send PC stats to ESP32"""
        cpu_temp = self.get_cpu_temp()
        cpu_usage = self.get_cpu_usage()
        cpu_speed = self.get_cpu_speed()
        ram_used, ram_total = self.get_ram_usage()
        gpu_temp, gpu_usage = self.get_gpu_stats()
        net_down, net_up = self.get_net_speed()

        stats = {
            "cpu_temp": cpu_temp,
            "cpu_usage": cpu_usage,
            "cpu_speed": cpu_speed,
            "ram_used": ram_used,
            "ram_total": ram_total,
            "gpu_temp": gpu_temp,
            "gpu_usage": gpu_usage,
            "net_down": net_down,
            "net_up": net_up
        }

        try:
            response = requests.post(ESP32_STATS_URL, data=stats, timeout=REQUEST_TIMEOUT)
            if response.status_code == 200:
                log.debug(f"Stats: CPU {cpu_temp}°/{cpu_usage}%/{cpu_speed}G "
                         f"GPU {gpu_temp}°/{gpu_usage}% "
                         f"RAM {ram_used}/{ram_total}G "
                         f"NET ↓{net_down}M ↑{net_up}M")
            else:
                log.warning(f"ESP32 stats returned: {response.status_code}")
        except requests.exceptions.RequestException as e:
            log.warning(f"ESP32 unreachable: {e}")

    # ==================== Main Loop ====================

    async def run(self):
        """Main polling loop"""
        log.info(f"PC Watcher started - target: {ESP32_IP}")
        log.info(f"Games file: {GAMES_FILE}")
        log.info(f"Log file: {LOG_FILE}")
        log.info("Press Ctrl+C to stop")

        while True:
            current_time = asyncio.get_event_loop().time()

            # Check if gaming mode should be active
            should_game = self.check_gaming_mode()

            # Handle mode transitions
            if should_game != self.gaming_mode:
                self.gaming_mode = should_game
                self.send_gaming_mode(self.gaming_mode)

                # Reset media state when entering gaming mode
                if self.gaming_mode:
                    self.last_song = ""
                    self.last_artist = ""
                    self.last_playing = False

            if self.gaming_mode:
                # GAMING MODE: Send PC stats at slower interval (media is ignored)
                if current_time - self.last_stats_update >= STATS_INTERVAL:
                    self.send_stats()
                    self.last_stats_update = current_time
            else:
                # NORMAL MODE: Watch media AND send stats
                # Always send stats so ESP32 can show them when idle
                if current_time - self.last_stats_update >= STATS_INTERVAL:
                    self.send_stats()
                    self.last_stats_update = current_time

                # Also watch media
                song, artist, is_playing = await self.get_media_info()

                # Determine if we need to send an update
                song_changed = (song != self.last_song or artist != self.last_artist)
                state_changed = (is_playing != self.last_playing)

                if song_changed or state_changed:
                    log.debug(f"Media state change: song_changed={song_changed}, state_changed={state_changed}")

                    if is_playing and song:
                        # Playing new or different song
                        self.send_now_playing(song, artist)
                    elif not is_playing and self.last_playing:
                        # Just paused/stopped - clear display
                        self.send_now_playing("", "")

                    self.last_song = song or ""
                    self.last_artist = artist or ""
                    self.last_playing = is_playing

            await asyncio.sleep(POLL_INTERVAL)

    def cleanup(self):
        """Clean up resources"""
        if self.nvml_initialized:
            try:
                pynvml.nvmlShutdown()
            except Exception:
                pass


def main():
    log.info("=" * 50)
    log.info(f"Session started at {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")

    watcher = PCWatcher()
    try:
        asyncio.run(watcher.run())
    except KeyboardInterrupt:
        log.info("Stopped by user")
    finally:
        watcher.cleanup()


if __name__ == "__main__":
    main()
