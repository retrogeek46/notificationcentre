#!/usr/bin/env python3
"""
Convert PNG image to RGB565 C header for TFT_eSPI
Usage: python png_to_rgb565.py input.png output.h
"""

from PIL import Image
import sys
import os

def rgb888_to_rgb565(r, g, b):
    """Convert RGB888 to RGB565"""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def convert_png_to_rgb565(input_path, output_path):
    # Load image
    img = Image.open(input_path)
    img = img.convert('RGB')
    width, height = img.size

    print(f"Image size: {width}x{height}")
    print(f"Total pixels: {width * height}")
    print(f"Estimated size: {width * height * 2 / 1024:.1f} KB")

    # Generate variable name from filename
    basename = os.path.splitext(os.path.basename(output_path))[0]
    var_name = basename.replace(' ', '_').replace('-', '_')

    # Convert to RGB565
    pixels = []
    for y in range(height):
        for x in range(width):
            r, g, b = img.getpixel((x, y))
            rgb565 = rgb888_to_rgb565(r, g, b)
            pixels.append(rgb565)

    # Write header file
    with open(output_path, 'w') as f:
        f.write(f"// Generated from {os.path.basename(input_path)}\n")
        f.write(f"// Size: {width}x{height} pixels\n")
        f.write(f"// Memory: {len(pixels) * 2} bytes\n\n")
        f.write("#pragma once\n")
        f.write("#include <pgmspace.h>\n\n")
        f.write(f"const uint16_t {var_name}_width = {width};\n")
        f.write(f"const uint16_t {var_name}_height = {height};\n\n")
        f.write(f"const uint16_t {var_name}[] PROGMEM = {{\n")

        # Write pixel data in rows for readability
        for i in range(0, len(pixels), 16):
            row = pixels[i:i+16]
            hex_vals = ', '.join(f'0x{p:04X}' for p in row)
            if i + 16 < len(pixels):
                f.write(f"  {hex_vals},\n")
            else:
                f.write(f"  {hex_vals}\n")

        f.write("};\n")

    print(f"Saved to: {output_path}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        # Default paths for this project
        input_path = "src/sprites/dithered bg.png"
        output_path = "src/sprites/dithered_bg.h"
    else:
        input_path = sys.argv[1]
        output_path = sys.argv[2]

    convert_png_to_rgb565(input_path, output_path)
