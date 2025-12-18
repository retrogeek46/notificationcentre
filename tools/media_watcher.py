"""
Media Watcher for ESP32 Notification Center

Polls Windows Media Session for currently playing media and sends updates
to ESP32 via HTTP. Runs continuously in background.

Usage: pythonw media_watcher.py  (hidden window)
   or: python media_watcher.py   (with console)
"""

import asyncio
import logging
import os
from datetime import datetime
from logging.handlers import RotatingFileHandler

import requests

# Use the new modular winrt packages (winrt-runtime + winrt-Windows.Media.Control)
from winrt.windows.media.control import (
    GlobalSystemMediaTransportControlsSessionManager as MediaManager,
    GlobalSystemMediaTransportControlsSessionPlaybackStatus as PlaybackStatus,
)

# Configuration
# NOTE: Also update src/network.cpp if you change this IP
ESP32_IP = "192.168.1.246"
ESP32_URL = f"http://{ESP32_IP}/nowplaying"
POLL_INTERVAL = 0.25  # seconds
REQUEST_TIMEOUT = 2  # seconds
LOG_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "media_watcher.log")
LOG_MAX_BYTES = 1024 * 1024  # 1 MB
LOG_BACKUP_COUNT = 3  # Keep 3 old log files

# Setup logging - both file and console
def setup_logging():
    logger = logging.getLogger("MediaWatcher")
    logger.setLevel(logging.DEBUG)

    # File handler with rotation
    file_handler = RotatingFileHandler(
        LOG_FILE, maxBytes=LOG_MAX_BYTES, backupCount=LOG_BACKUP_COUNT, encoding="utf-8"
    )
    file_handler.setLevel(logging.DEBUG)
    file_format = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s", datefmt="%Y-%m-%d %H:%M:%S")
    file_handler.setFormatter(file_format)

    # Console handler
    console_handler = logging.StreamHandler()
    console_handler.setLevel(logging.INFO)
    console_format = logging.Formatter("[%(levelname)s] %(message)s")
    console_handler.setFormatter(console_format)

    logger.addHandler(file_handler)
    logger.addHandler(console_handler)

    return logger


log = setup_logging()


class MediaWatcher:
    def __init__(self):
        self.last_song = ""
        self.last_artist = ""
        self.last_playing = False
        self.session_mgr = None

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

    def send_to_esp32(self, song: str, artist: str):
        """Send now playing info to ESP32."""
        try:
            data = {"song": song, "artist": artist}
            response = requests.post(ESP32_URL, data=data, timeout=REQUEST_TIMEOUT)
            if response.status_code == 200:
                if song:
                    log.info(f"PLAYING: {song} - {artist}")
                else:
                    log.info("PAUSED: Cleared display")
            else:
                log.warning(f"ESP32 returned status: {response.status_code}")
        except requests.exceptions.RequestException as e:
            log.warning(f"ESP32 unreachable: {e}")

    async def run(self):
        """Main polling loop."""
        log.info(f"Media Watcher started - target: {ESP32_IP}")
        log.info(f"Log file: {LOG_FILE}")
        log.info("Press Ctrl+C to stop")

        while True:
            song, artist, is_playing = await self.get_media_info()

            # Determine if we need to send an update
            song_changed = (song != self.last_song or artist != self.last_artist)
            state_changed = (is_playing != self.last_playing)

            if song_changed or state_changed:
                log.debug(f"State change detected: song_changed={song_changed}, state_changed={state_changed}")

                if is_playing and song:
                    # Playing new or different song
                    self.send_to_esp32(song, artist)
                elif not is_playing and self.last_playing:
                    # Just paused/stopped - clear display
                    self.send_to_esp32("", "")

                self.last_song = song or ""
                self.last_artist = artist or ""
                self.last_playing = is_playing

            await asyncio.sleep(POLL_INTERVAL)


def main():
    log.info("=" * 50)
    log.info(f"Session started at {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    watcher = MediaWatcher()
    try:
        asyncio.run(watcher.run())
    except KeyboardInterrupt:
        log.info("Stopped by user")


if __name__ == "__main__":
    main()
