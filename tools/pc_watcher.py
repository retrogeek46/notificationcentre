"""
PC Watcher for ESP32 Notification Center

Unified script that handles:
- Media watching: Sends now playing info when NOT gaming
- PC stats: Sends hardware stats via LibreHardwareMonitor

Gaming mode activates automatically when:
- Active window matches a game in games.txt
- Active window is fullscreen

Requirements:
- LibreHardwareMonitor running as Administrator with WMI enabled
- pynvml for NVIDIA GPU stats (optional fallback)

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

# NVIDIA GPU monitoring (fallback if LHM GPU fails)
try:
    import pynvml
    NVML_AVAILABLE = True
except ImportError:
    NVML_AVAILABLE = False

# Windows API for active window detection
try:
    import win32gui
    import win32process
    import win32api
    import win32con
    WIN32_AVAILABLE = True
except ImportError:
    WIN32_AVAILABLE = False

# WMI for LibreHardwareMonitor access
try:
    import wmi
    WMI_AVAILABLE = True
except ImportError:
    WMI_AVAILABLE = False

# ==================== Configuration ====================

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


# ==================== Logging Setup ====================

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


# ==================== Utilities ====================

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

        rect = win32gui.GetWindowRect(hwnd)
        window_width = rect[2] - rect[0]
        window_height = rect[3] - rect[1]

        screen_width = win32api.GetSystemMetrics(win32con.SM_CXSCREEN)
        screen_height = win32api.GetSystemMetrics(win32con.SM_CYSCREEN)

        is_fs = (window_width >= screen_width and window_height >= screen_height)

        if is_fs or (rect[0] <= 0 and rect[1] <= 0 and
                     window_width >= screen_width - 2 and
                     window_height >= screen_height - 2):
            return True

        return False
    except Exception as e:
        log.debug(f"Error checking fullscreen: {e}")
        return False


# ==================== LibreHardwareMonitor Reader ====================

class LibreHardwareMonitorReader:
    """Read sensor data from LibreHardwareMonitor via WMI (optimized queries)"""

    def __init__(self):
        self.available = False
        self.wmi_lhm = None
        self._init()

    def _init(self):
        """Initialize connection to LibreHardwareMonitor WMI"""
        if not WMI_AVAILABLE:
            log.warning("WMI not available - LibreHardwareMonitor won't work")
            return

        try:
            self.wmi_lhm = wmi.WMI(namespace=r"root\LibreHardwareMonitor")
            # Quick test query
            test = self.wmi_lhm.query("SELECT Name FROM Sensor WHERE SensorType='Temperature'")
            if test:
                self.available = True
                log.info("LibreHardwareMonitor connected")
            else:
                log.warning("LibreHardwareMonitor: no sensors found (run as Admin?)")
        except Exception as e:
            log.error(f"Failed to connect to LibreHardwareMonitor: {e}")

    def _query_sensor(self, sensor_type, name_like):
        """Query a single sensor value using WMI WHERE clause"""
        if not self.available:
            return None
        try:
            query = f"SELECT Value FROM Sensor WHERE SensorType='{sensor_type}' AND Name LIKE '%{name_like}%'"
            results = self.wmi_lhm.query(query)
            if results:
                return results[0].Value
        except Exception as e:
            log.debug(f"Query error ({sensor_type}/{name_like}): {e}")
        return None

    def _query_sensors(self, sensor_type, name_like):
        """Query multiple sensor values using WMI WHERE clause"""
        if not self.available:
            return []
        try:
            query = f"SELECT Value FROM Sensor WHERE SensorType='{sensor_type}' AND Name LIKE '%{name_like}%'"
            results = self.wmi_lhm.query(query)
            return [r.Value for r in results if r.Value is not None]
        except Exception as e:
            log.debug(f"Query error ({sensor_type}/{name_like}): {e}")
        return []

    # ---- CPU Sensors ----

    def get_cpu_temp(self):
        """Get CPU temperature (Package preferred)"""
        # Try CPU Package first
        temp = self._query_sensor("Temperature", "CPU Package")
        if temp:
            return int(temp)
        # Try Core Max
        temp = self._query_sensor("Temperature", "Core Max")
        if temp:
            return int(temp)
        return 0

    def get_cpu_usage(self):
        """Get CPU total usage percentage"""
        usage = self._query_sensor("Load", "CPU Total")
        if usage is not None:
            return int(usage)
        return 0

    def get_cpu_clock(self):
        """Get average CPU clock speed in GHz"""
        clocks = self._query_sensors("Clock", "CPU Core")
        if clocks:
            avg_mhz = sum(clocks) / len(clocks)
            return round(avg_mhz / 1000.0, 1)
        return 0.0

    # ---- GPU Sensors ----

    def get_gpu_temp(self):
        """Get GPU temperature"""
        temp = self._query_sensor("Temperature", "GPU Core")
        if temp:
            return int(temp)
        return 0

    def get_gpu_usage(self):
        """Get GPU core usage percentage"""
        usage = self._query_sensor("Load", "GPU Core")
        if usage is not None:
            return int(usage)
        return 0

    # ---- Memory ----

    def get_ram_used_gb(self):
        """Get RAM used in GB"""
        used = self._query_sensor("Data", "Memory Used")
        if used is not None:
            return int(used)
        return int(psutil.virtual_memory().used / (1024 ** 3))

    def get_ram_total_gb(self):
        """Get total RAM in GB"""
        return int(psutil.virtual_memory().total / (1024 ** 3))




# ==================== PC Watcher ====================

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

        # Network speed tracking
        self.last_net_recv = 0
        self.last_net_sent = 0
        self.last_net_time = 0
        self.last_stats_update = 0

        # Initialize LibreHardwareMonitor
        self.lhm = LibreHardwareMonitorReader()



        # NVML as fallback for GPU if LHM doesn't have it
        self.nvml_initialized = False
        if NVML_AVAILABLE:
            try:
                pynvml.nvmlInit()
                self.nvml_initialized = True
                log.info("NVML initialized (fallback for GPU)")
            except Exception as e:
                log.debug(f"NVML init failed: {e}")

    # ==================== Gaming Detection ====================

    def check_gaming_mode(self):
        """Check if gaming mode should be active"""
        active_exe = get_active_window_exe()
        if active_exe:
            for game in self.games_list:
                if game in active_exe or active_exe in game:
                    return True

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

            playback_info = session.get_playback_info()
            is_playing = playback_info.playback_status == PlaybackStatus.PLAYING

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
        if self.lhm.available:
            temp = self.lhm.get_cpu_temp()
            if temp > 0:
                return temp
        return 0

    def get_cpu_usage(self):
        """Get CPU usage percentage"""
        if self.lhm.available:
            usage = self.lhm.get_cpu_usage()
            if usage > 0:
                return usage
        return int(psutil.cpu_percent(interval=None))

    def get_cpu_speed(self):
        """Get current CPU frequency in GHz"""
        if self.lhm.available:
            speed = self.lhm.get_cpu_clock()
            if speed > 0:
                return speed
        # Fallback to psutil
        try:
            freq = psutil.cpu_freq()
            if freq:
                return round(freq.current / 1000.0, 1)
        except Exception:
            pass
        return 0.0

    def get_ram_usage(self):
        """Get RAM usage (used, total) in GB"""
        if self.lhm.available:
            used = self.lhm.get_ram_used_gb()
            total = self.lhm.get_ram_total_gb()
            return used, total
        # Fallback
        mem = psutil.virtual_memory()
        return int(mem.used / (1024 ** 3)), int(mem.total / (1024 ** 3))

    def get_gpu_stats(self):
        """Get GPU temperature and usage"""
        # Try LibreHardwareMonitor first
        if self.lhm.available:
            temp = self.lhm.get_gpu_temp()
            usage = self.lhm.get_gpu_usage()
            if temp > 0 or usage > 0:
                return temp, usage

        # Fallback to NVML for NVIDIA GPUs
        if self.nvml_initialized:
            try:
                handle = pynvml.nvmlDeviceGetHandleByIndex(0)
                temp = pynvml.nvmlDeviceGetTemperature(handle, pynvml.NVML_TEMPERATURE_GPU)
                util = pynvml.nvmlDeviceGetUtilizationRates(handle)
                return int(temp), int(util.gpu)
            except Exception as e:
                log.debug(f"NVML error: {e}")

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

        while True:
            current_time = asyncio.get_event_loop().time()

            # Check if gaming mode should be active
            should_game = self.check_gaming_mode()

            # Handle mode transitions
            if should_game != self.gaming_mode:
                self.gaming_mode = should_game
                self.send_gaming_mode(self.gaming_mode)

                if self.gaming_mode:
                    self.last_song = ""
                    self.last_artist = ""
                    self.last_playing = False

            # Always send stats at the configured interval
            if current_time - self.last_stats_update >= STATS_INTERVAL:
                self.send_stats()
                self.last_stats_update = current_time

            # Watch media when not gaming
            if not self.gaming_mode:
                song, artist, is_playing = await self.get_media_info()

                song_changed = (song != self.last_song or artist != self.last_artist)
                state_changed = (is_playing != self.last_playing)

                if song_changed or state_changed:
                    if is_playing and song:
                        self.send_now_playing(song, artist)
                    elif not is_playing and self.last_playing:
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


# ==================== Entry Point ====================

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
