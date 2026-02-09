"""
Generate a gzipped, minified HTML header file for ESP32.
This script:
1. Reads the dashboard HTML
2. Minifies it (removes whitespace, comments)
3. Gzips it
4. Outputs a C header with the byte array
"""

import gzip
import re
import os

# Read HTML file
html_path = 'tools/esp32_dashboard.html'
output_path = 'src/dashboard_html.h'

with open(html_path, 'r', encoding='utf-8') as f:
    html = f.read()

original_size = len(html)
print(f"Original size: {original_size} bytes")

# Minify HTML
def minify_html(html):
    # Remove HTML comments
    html = re.sub(r'<!--.*?-->', '', html, flags=re.DOTALL)
    
    # Remove CSS comments
    html = re.sub(r'/\*.*?\*/', '', html, flags=re.DOTALL)
    
    # Remove JS single-line comments (careful with URLs)
    html = re.sub(r'(?<!:)//(?!["\'])[^\n]*', '', html)
    
    # Collapse multiple whitespace to single space
    html = re.sub(r'\s+', ' ', html)
    
    # Remove space around tags
    html = re.sub(r'>\s+<', '><', html)
    
    # Remove space after opening tags
    html = re.sub(r'>\s+', '>', html)
    
    # Remove space before closing tags  
    html = re.sub(r'\s+<', '<', html)
    
    # Trim
    html = html.strip()
    
    return html

minified = minify_html(html)
minified_size = len(minified)
print(f"Minified size: {minified_size} bytes ({100*minified_size/original_size:.1f}%)")

# Gzip compress
gzipped = gzip.compress(minified.encode('utf-8'), compresslevel=9)
gzipped_size = len(gzipped)
print(f"Gzipped size: {gzipped_size} bytes ({100*gzipped_size/original_size:.1f}%)")

# Generate C header
def bytes_to_c_array(data):
    lines = []
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        hex_values = ', '.join(f'0x{b:02x}' for b in chunk)
        lines.append(f'  {hex_values},')
    return '\n'.join(lines)

header_content = f'''#pragma once
// Auto-generated gzipped dashboard HTML
// Original: {original_size} bytes -> Minified: {minified_size} bytes -> Gzipped: {gzipped_size} bytes
// Compression ratio: {100*gzipped_size/original_size:.1f}%

#include <Arduino.h>

const size_t DASHBOARD_HTML_GZ_LEN = {gzipped_size};

const uint8_t DASHBOARD_HTML_GZ[] PROGMEM = {{
{bytes_to_c_array(gzipped)}
}};
'''

# Write header file
with open(output_path, 'w', encoding='utf-8') as f:
    f.write(header_content)

print(f"\nCreated {output_path}")
print(f"Final size: {gzipped_size} bytes (saved {original_size - gzipped_size} bytes)")
