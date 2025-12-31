#!/usr/bin/env python3
"""
Album art extractor - runs as subprocess to avoid blocking main event loop.
Returns RGB565 base64-encoded album art to stdout.
Exit codes: 0=success (art on stdout), 1=no media, 2=no thumbnail, 3=error
"""

import asyncio
import base64
import io
import struct
import sys

from winsdk.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as MediaManager
from winsdk.windows.storage.streams import Buffer, InputStreamOptions, DataReader

try:
    from PIL import Image, ImageEnhance, ImageFilter
except ImportError:
    print("Pillow not installed", file=sys.stderr)
    sys.exit(3)

ALBUM_ART_SIZE = 18  # 18x18 pixels for status bar (with 1px border)


async def extract_album_art():
    """Extract album art and return RGB565 base64."""
    try:
        sessions = await MediaManager.request_async()
        current_session = sessions.get_current_session()

        if not current_session:
            return None, 1  # No media session

        props = await current_session.try_get_media_properties_async()

        if not props.thumbnail:
            return None, 2  # No thumbnail

        # Open stream
        thumb_stream_ref = props.thumbnail
        readable_stream = await thumb_stream_ref.open_read_async()

        # Read data
        buffer = Buffer(readable_stream.size)
        await readable_stream.read_async(buffer, buffer.capacity, InputStreamOptions.NONE)

        reader = DataReader.from_buffer(buffer)
        byte_array = bytearray(readable_stream.size)
        reader.read_bytes(byte_array)

        # Load image and preserve aspect ratio (height fixed at 18px)
        img = Image.open(io.BytesIO(byte_array))
        img = img.convert("RGB")
        
        # Calculate dimensions preserving aspect ratio
        orig_w, orig_h = img.size
        target_h = ALBUM_ART_SIZE  # 18px
        aspect = orig_w / orig_h
        target_w = max(1, min(64, int(target_h * aspect)))  # Width from ratio, max 64px
        
        img = img.resize((target_w, target_h), Image.Resampling.LANCZOS)
        
        # Enhance for small SPI display (boost colors and contrast)
        try:
            # Boost saturation
            converter = ImageEnhance.Color(img)
            img = converter.enhance(1.4)  # 40% saturation boost
            
            # Boost contrast
            converter = ImageEnhance.Contrast(img)
            img = converter.enhance(1.2)  # 20% contrast boost
            
            # Sharpening (helps at small sizes)
            img = img.filter(ImageFilter.SHARPEN)
        except Exception as e:
            print(f"Enhancement failed: {e}", file=sys.stderr)

        # Convert to RGB565 with byte swap for TFT_eSPI
        rgb565_data = bytearray()
        for y in range(target_h):
            for x in range(target_w):
                r, g, b = img.getpixel((x, y))
                rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                # Byte swap for TFT_eSPI (swap high and low bytes)
                rgb565_data.append((rgb565 >> 8) & 0xFF)  # High byte first
                rgb565_data.append(rgb565 & 0xFF)         # Low byte second

        art_b64 = base64.b64encode(rgb565_data).decode("ascii")
        # Return format: WxH;base64data
        return f"{target_w}x{target_h};{art_b64}", 0  # Success

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return None, 3  # Error


def main():
    art_b64, exit_code = asyncio.run(extract_album_art())
    if art_b64:
        print(art_b64)  # Output to stdout for parent process
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
