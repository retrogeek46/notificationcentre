#!/usr/bin/env python3
"""
Split a 320x240 sprite into zone-based C header arrays for TFT_eSPI sprites.
Zones are read dynamically from types.h to stay in sync with the C code.
"""

import os
import sys
import re
from PIL import Image


def parse_types_h(types_h_path):
    """Parse types.h to extract zone boundary constants."""
    with open(types_h_path, 'r') as f:
        content = f.read()

    # Pattern to match: const int ZONE_XXX_Y_ZZZ = NNN;
    pattern = r'const\s+int\s+(ZONE_\w+)\s*=\s*(\d+)\s*;'
    matches = re.findall(pattern, content)

    constants = {name: int(value) for name, value in matches}
    return constants


def build_zones_from_types_h(base_dir):
    """Build ZONES dictionary by reading types.h."""
    types_h_path = os.path.join(base_dir, 'src', 'types.h')

    if not os.path.exists(types_h_path):
        print(f"[ERROR] types.h not found at: {types_h_path}")
        sys.exit(1)

    constants = parse_types_h(types_h_path)
    print(f"[INFO] Parsed {len(constants)} zone constants from types.h")

    # Build zones from parsed constants
    zones = {
        'title': {
            'x_start': constants.get('ZONE_TITLE_X_START', 0),
            'x_end': constants.get('ZONE_TITLE_X_END', 99),
            'y_start': constants.get('ZONE_TITLE_Y_START', 0),
            'y_end': constants.get('ZONE_TITLE_Y_END', 24),
            'array_name': 'SPRITE_TITLE'
        },
        'clock': {
            'x_start': constants.get('ZONE_CLOCK_X_START', 100),
            'x_end': constants.get('ZONE_CLOCK_X_END', 319),
            'y_start': constants.get('ZONE_CLOCK_Y_START', 0),
            'y_end': constants.get('ZONE_CLOCK_Y_END', 24),
            'array_name': 'SPRITE_CLOCK'
        },
        'status': {
            'x_start': constants.get('ZONE_STATUS_X_START', 0),
            'x_end': constants.get('ZONE_STATUS_X_END', 319),
            'y_start': constants.get('ZONE_STATUS_Y_START', 25),
            'y_end': constants.get('ZONE_STATUS_Y_END', 44),
            'array_name': 'SPRITE_STATUS'
        },
        'content1': {
            'x_start': constants.get('ZONE_CONTENT1_X_START', 0),
            'x_end': constants.get('ZONE_CONTENT1_X_END', 319),
            'y_start': constants.get('ZONE_CONTENT1_Y_START', 45),
            'y_end': constants.get('ZONE_CONTENT1_Y_END', 109),
            'array_name': 'SPRITE_CONTENT1'
        },
        'content2': {
            'x_start': constants.get('ZONE_CONTENT2_X_START', 0),
            'x_end': constants.get('ZONE_CONTENT2_X_END', 319),
            'y_start': constants.get('ZONE_CONTENT2_Y_START', 110),
            'y_end': constants.get('ZONE_CONTENT2_Y_END', 174),
            'array_name': 'SPRITE_CONTENT2'
        },
        'content3': {
            'x_start': constants.get('ZONE_CONTENT3_X_START', 0),
            'x_end': constants.get('ZONE_CONTENT3_X_END', 319),
            'y_start': constants.get('ZONE_CONTENT3_Y_START', 175),
            'y_end': constants.get('ZONE_CONTENT3_Y_END', 239),
            'array_name': 'SPRITE_CONTENT3'
        }
    }

    return zones

def rgb_to_565(r, g, b):
    """Convert 8-bit RGB to 16-bit RGB565 (byte-swapped for TFT_eSPI)."""
    rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    # Byte-swap for TFT_eSPI (little-endian)
    return ((rgb565 & 0xFF) << 8) | ((rgb565 >> 8) & 0xFF)

def crop_zone(img, zone):
    """Crop the image to the specified zone."""
    x1 = zone['x_start']
    y1 = zone['y_start']
    x2 = zone['x_end'] + 1  # PIL crop is exclusive on right/bottom
    y2 = zone['y_end'] + 1
    return img.crop((x1, y1, x2, y2))

def save_header(zone_img, output_path, array_name, source_name):
    """Save a zone image as a C header file."""
    width, height = zone_img.size
    pixels = list(zone_img.getdata())

    # Convert to RGB565
    rgb565_data = [rgb_to_565(r, g, b) for r, g, b in pixels]

    # Generate header content
    header = f"""// Auto-generated from {source_name}
// Dimensions: {width}x{height}
#pragma once
#include <Arduino.h>

const uint16_t {array_name}_WIDTH = {width};
const uint16_t {array_name}_HEIGHT = {height};

const uint16_t {array_name}[{width * height}] PROGMEM = {{
"""

    # Format data in rows of 16 values
    lines = []
    for i in range(0, len(rgb565_data), 16):
        row = rgb565_data[i:i+16]
        line = "    " + ", ".join(f"0x{v:04X}" for v in row)
        lines.append(line)

    header += ",\n".join(lines)
    header += "\n};\n"

    with open(output_path, 'w') as f:
        f.write(header)

    return width, height, len(rgb565_data) * 2

def split_sprite(input_path, output_dir, zones):
    """Split a 320x240 sprite into zone header files."""
    # Load and validate image
    img = Image.open(input_path).convert('RGB')
    width, height = img.size

    if width != 320 or height != 240:
        print(f"[WARNING] Expected 320x240, got {width}x{height}")
        print("[WARNING] Proceeding anyway - zones may be incorrect")

    source_name = os.path.basename(input_path)
    print(f"Splitting {source_name} ({width}x{height}) into zones...")
    print("=" * 50)

    total_bytes = 0

    for zone_name, zone in zones.items():
        # Crop zone from full image
        zone_img = crop_zone(img, zone)

        # Generate output path
        output_path = os.path.join(output_dir, f"sprite_{zone_name}.h")

        # Save as header
        w, h, bytes_size = save_header(zone_img, output_path, zone['array_name'], source_name)
        total_bytes += bytes_size

        print(f"  [OK] {zone_name.upper():8} -> sprite_{zone_name}.h ({w}x{h}, {bytes_size:,} bytes)")

    print("=" * 50)
    print(f"Total: {total_bytes:,} bytes ({total_bytes / 1024:.1f} KB)")

def main():
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(base_dir)

    # Build zones dynamically from types.h
    zones = build_zones_from_types_h(base_dir)

    # Default input path - look for a full sprite
    default_inputs = [
        "src/sprites/full_sprite.png",
        "src/sprites/notifcentre_sprite.png",
        "src/sprites/dithered bg.png"
    ]

    # Check for command line argument
    if len(sys.argv) > 1:
        input_path = sys.argv[1]
    else:
        # Try to find an existing full sprite
        input_path = None
        for path in default_inputs:
            if os.path.exists(path):
                input_path = path
                break

        if input_path is None:
            print("Usage: python split_sprite.py <input_image.png>")
            print("\nNo full sprite found. Expected locations:")
            for path in default_inputs:
                print(f"  - {path}")
            sys.exit(1)

    if not os.path.exists(input_path):
        print(f"[ERROR] File not found: {input_path}")
        sys.exit(1)

    # Output to sprites directory
    output_dir = "src/sprites"
    os.makedirs(output_dir, exist_ok=True)

    split_sprite(input_path, output_dir, zones)

if __name__ == "__main__":
    main()
