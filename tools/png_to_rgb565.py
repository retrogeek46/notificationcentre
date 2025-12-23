#!/usr/bin/env python3
"""
Convert PNG images to RGB565 C header arrays for TFT_eSPI sprites.
Outputs PROGMEM-compatible arrays for ESP32.
"""

import os
import sys
from PIL import Image

def rgb_to_565(r, g, b):
    """Convert 8-bit RGB to 16-bit RGB565 (byte-swapped for TFT_eSPI)."""
    rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    # Byte-swap for TFT_eSPI (little-endian)
    return ((rgb565 & 0xFF) << 8) | ((rgb565 >> 8) & 0xFF)

def convert_png_to_header(input_path, output_path, array_name):
    """Convert a PNG file to a C header with RGB565 array."""
    img = Image.open(input_path).convert('RGB')
    width, height = img.size
    pixels = list(img.getdata())
    
    # Convert to RGB565
    rgb565_data = [rgb_to_565(r, g, b) for r, g, b in pixels]
    
    # Generate header content
    header = f"""// Auto-generated from {os.path.basename(input_path)}
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
    
    print(f"[OK] {os.path.basename(input_path)} -> {os.path.basename(output_path)} ({width}x{height}, {len(rgb565_data)*2} bytes)")

def main():
    # Define sprite conversions
    sprites = [
        ("src/sprites/fixed_title.png", "src/sprites/sprite_title.h", "SPRITE_TITLE"),
        ("src/sprites/fixed_clock.png", "src/sprites/sprite_clock.h", "SPRITE_CLOCK"),
        ("src/sprites/fixed_status.png", "src/sprites/sprite_status.h", "SPRITE_STATUS"),
        ("src/sprites/fixed_content.png", "src/sprites/sprite_content.h", "SPRITE_CONTENT"),
    ]
    
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(base_dir)
    
    print("Converting sprites to RGB565 headers...")
    print("=" * 50)
    
    for input_path, output_path, array_name in sprites:
        if os.path.exists(input_path):
            convert_png_to_header(input_path, output_path, array_name)
        else:
            print(f"[MISSING] {input_path} not found")
    
    print("=" * 50)
    print("Done!")

if __name__ == "__main__":
    main()
