#!/usr/bin/env python3
"""
Font Generator for MDIO Trial Fonts with Extended Character Range

This script uses the fonttools library to generate Adafruit GFX-compatible font headers
from TTF files, supporting extended ASCII characters (0x20-0xFF).

Usage:
    python generate_fonts.py <path_to_ttf_regular> <path_to_ttf_bold>

Example:
    python generate_fonts.py "C:/Fonts/MDIOTrial-Regular.ttf" "C:/Fonts/MDIOTrial-Bold.ttf"
"""

import sys
import os
import subprocess
import tempfile
import shutil
from pathlib import Path

# Font sizes to generate
FONT_SIZES = [8, 9, 10]

# Character range: 0x20 (space) to 0xFF (extended Latin)
FIRST_CHAR = 0x20  # 32 - space
LAST_CHAR = 0xFF   # 255 - extended chars (includes °, ±, ×, etc.)

# Output directory (relative to this script)
OUTPUT_DIR = Path(__file__).parent.parent / "src" / "fonts"


def check_fontconvert():
    """Check if fontconvert is available, provide instructions if not."""
    # Try to find fontconvert in common locations
    possible_paths = [
        "fontconvert",
        "./fontconvert",
        str(Path(__file__).parent / "fontconvert"),
        str(Path(__file__).parent / "fontconvert.exe"),
    ]

    for path in possible_paths:
        if shutil.which(path) or os.path.isfile(path):
            return path

    return None


def download_and_build_fontconvert():
    """Provide instructions for getting fontconvert."""
    print("\n" + "="*70)
    print("fontconvert not found!")
    print("="*70)
    print("""
To generate fonts, you need Adafruit's fontconvert tool. Here's how to get it:

OPTION 1: Download pre-built (Windows)
  1. Download from: https://github.com/ArtifexSoftware/mupdf/releases
     (mupdf includes a fontconvert-like tool)

OPTION 2: Build from source (requires make/gcc)
  1. git clone https://github.com/adafruit/Adafruit-GFX-Library.git
  2. cd Adafruit-GFX-Library/fontconvert
  3. make
  4. Copy 'fontconvert' executable to this tools folder

OPTION 3: Use online converter
  Visit: https://rop.nl/truetype2gfx/
  - Upload your TTF file
  - Set size (8, 9, 10)
  - Set first char: 32
  - Set last char: 255
  - Download each font header file

After getting fontconvert, place it in:
  """ + str(Path(__file__).parent))
    print("="*70 + "\n")
    return False


def generate_font(fontconvert_path, ttf_path, font_name, size, output_path):
    """Generate a single font header file."""
    cmd = [
        fontconvert_path,
        str(ttf_path),
        str(size),
        str(FIRST_CHAR),
        str(LAST_CHAR)
    ]

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)

        # The output needs some cleanup for our naming convention
        output_content = result.stdout

        # Write to output file
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(output_content)

        print(f"  ✓ Generated: {output_path.name}")
        return True

    except subprocess.CalledProcessError as e:
        print(f"  ✗ Failed to generate {output_path.name}: {e.stderr}")
        return False
    except FileNotFoundError:
        print(f"  ✗ fontconvert not found at: {fontconvert_path}")
        return False


def main():
    print("\n" + "="*70)
    print("MDIO Trial Font Generator - Extended Character Range (0x20-0xFF)")
    print("="*70)

    if len(sys.argv) < 3:
        print(f"""
Usage: python {sys.argv[0]} <regular_ttf> <bold_ttf>

Example:
    python {sys.argv[0]} "MDIOTrial-Regular.ttf" "MDIOTrial-Bold.ttf"

This will generate font headers with extended characters including:
  - Basic ASCII (space through ~)
  - Extended symbols: ° ± × ÷ © ® « » ½ ¼ ¾
  - Accented characters: é ñ ü ä ö etc.
""")
        sys.exit(1)

    regular_ttf = Path(sys.argv[1])
    bold_ttf = Path(sys.argv[2])

    # Validate TTF files exist
    if not regular_ttf.exists():
        print(f"Error: Regular font not found: {regular_ttf}")
        sys.exit(1)
    if not bold_ttf.exists():
        print(f"Error: Bold font not found: {bold_ttf}")
        sys.exit(1)

    print(f"\nInput fonts:")
    print(f"  Regular: {regular_ttf}")
    print(f"  Bold:    {bold_ttf}")
    print(f"\nOutput directory: {OUTPUT_DIR}")
    print(f"Character range: 0x{FIRST_CHAR:02X} - 0x{LAST_CHAR:02X} ({LAST_CHAR - FIRST_CHAR + 1} characters)")
    print(f"Sizes: {FONT_SIZES}pt")

    # Check for fontconvert
    fontconvert = check_fontconvert()
    if not fontconvert:
        download_and_build_fontconvert()
        sys.exit(1)

    print(f"\nUsing fontconvert: {fontconvert}")
    print("\nGenerating fonts...")

    # Ensure output directory exists
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    # Generate all font variants
    fonts_to_generate = [
        (regular_ttf, "MDIOTrial_Regular"),
        (bold_ttf, "MDIOTrial_Bold"),
    ]

    success_count = 0
    total_count = 0

    for ttf_path, base_name in fonts_to_generate:
        for size in FONT_SIZES:
            output_name = f"{base_name}{size}pt7b.h"
            output_path = OUTPUT_DIR / output_name
            total_count += 1

            if generate_font(fontconvert, ttf_path, base_name, size, output_path):
                success_count += 1

    print(f"\nCompleted: {success_count}/{total_count} fonts generated")

    if success_count == total_count:
        print("\n✓ All fonts generated successfully!")
        print("\nNew characters available:")
        print("  ° (0xB0) - degree symbol")
        print("  ± (0xB1) - plus-minus")
        print("  × (0xD7) - multiplication")
        print("  ÷ (0xF7) - division")
        print("  © (0xA9) - copyright")
        print("  And many more extended Latin characters!")
        print("\nUsage in code:")
        print('  tft.print("25\\xB0C");  // Prints "25°C"')
    else:
        print("\n⚠ Some fonts failed to generate. Check errors above.")


if __name__ == "__main__":
    main()
