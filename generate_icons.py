#!/usr/bin/env python3
"""Generate LVGL 8.x compatible menu icons matching the existing style."""

def create_icon_header(name):
    """Create the C file header."""
    upper_name = name.upper()
    return f'''#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif
#ifndef LV_ATTRIBUTE_IMG_{upper_name}
#define LV_ATTRIBUTE_IMG_{upper_name}
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_IMG_{upper_name} uint8_t {name}_map[] = {{
'''

def rgb565_encode(r, g, b):
    """Convert 8-bit RGB to 16-bit RGB565 format (little endian)."""
    # RGB565: 5 bits red, 6 bits green, 5 bits blue
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    rgb565 = (r5 << 11) | (g6 << 5) | b5
    # Return as little endian bytes
    return rgb565 & 0xFF, (rgb565 >> 8) & 0xFF

def pixel_8bit(color=0x1c, alpha=0xff):
    """Create an 8-bit pixel (color, alpha)."""
    return (color, alpha)

def empty_pixel_8bit():
    """Create a transparent pixel."""
    return (0x00, 0x00)

def create_calibration_icon():
    """Create a signal bars calibration icon - four vertical bars of increasing height."""
    pixels = []
    
    for y in range(50):
        row = []
        for x in range(50):
            # Four bars: tiny, small, medium, large (signal strength visualization)
            # Bar 1: x 8-11, height from y 40-44 (5px tall)
            # Bar 2: x 16-19, height from y 32-44 (13px tall)  
            # Bar 3: x 24-27, height from y 22-44 (23px tall)
            # Bar 4: x 32-35, height from y 10-44 (35px tall)
            
            pixel = empty_pixel_8bit()
            
            # Bar 1 (tiny, far left)
            if 8 <= x <= 11 and 40 <= y <= 44:
                pixel = pixel_8bit(0x1c, 0xff)
            # Bar 2 (small, left-center)
            elif 16 <= x <= 19 and 32 <= y <= 44:
                pixel = pixel_8bit(0x1c, 0xff)
            # Bar 3 (medium, right-center)
            elif 24 <= x <= 27 and 22 <= y <= 44:
                pixel = pixel_8bit(0x1c, 0xff)
            # Bar 4 (tall, far right)
            elif 32 <= x <= 35 and 10 <= y <= 44:
                pixel = pixel_8bit(0x1c, 0xff)
            
            row.append(pixel)
        pixels.append(row)
    
    return pixels

def create_finder_icon():
    """Create a location/search finder icon with concentric circles."""
    pixels = []
    center_x, center_y = 25, 30  # Center point
    
    for y in range(50):
        row = []
        for x in range(50):
            pixel = empty_pixel_8bit()
            
            dx = x - center_x
            dy = y - center_y
            dist = (dx*dx + dy*dy) ** 0.5
            
            # Three concentric arcs (top half only - radar style)
            # Outer arc (radius ~18)
            if 17 <= dist <= 19 and y <= center_y:
                pixel = pixel_8bit(0x1c, 0xff)
            # Middle arc (radius ~13)
            elif 12 <= dist <= 14 and y <= center_y:
                pixel = pixel_8bit(0x1c, 0xff)
            # Inner arc (radius ~8)
            elif 7 <= dist <= 9 and y <= center_y:
                pixel = pixel_8bit(0x1c, 0xff)
            
            # Center dot (3x3)
            elif center_x-1 <= x <= center_x+1 and center_y-1 <= y <= center_y+1:
                pixel = pixel_8bit(0x1c, 0xff)
            
            # Vertical line from center downward (antenna)
            elif x == center_x and center_y+2 <= y <= 44:
                pixel = pixel_8bit(0x1c, 0xff)
            
            row.append(pixel)
        pixels.append(row)
    
    return pixels

def create_spectrum_icon():
    """Create spectrum analyzer icon - 7 vertical bars of varying heights."""
    pixels = []
    
    # Bar positions and heights (creating a wave pattern)
    bars = [
        {'x': 8, 'height': 15},   # Bar 1
        {'x': 14, 'height': 25},  # Bar 2
        {'x': 20, 'height': 35},  # Bar 3 (tallest)
        {'x': 26, 'height': 30},  # Bar 4
        {'x': 32, 'height': 20},  # Bar 5
        {'x': 38, 'height': 27},  # Bar 6
        {'x': 44, 'height': 18},  # Bar 7
    ]
    
    bar_width = 4  # Width of each bar
    base_y = 40    # Base line for all bars
    
    for y in range(50):
        row = []
        for x in range(50):
            pixel = empty_pixel_8bit()
            
            # Draw each bar
            for bar in bars:
                bar_start_x = bar['x']
                bar_end_x = bar['x'] + bar_width
                bar_start_y = base_y - bar['height']
                bar_end_y = base_y
                
                if bar_start_x <= x < bar_end_x and bar_start_y <= y <= bar_end_y:
                    pixel = pixel_8bit(0x1c, 0xff)
                    break
            
            # Draw baseline
            if y == base_y + 1 and 5 <= x <= 47:
                pixel = pixel_8bit(0x1c, 0xff)
            
            row.append(pixel)
        pixels.append(row)
    
    return pixels

def format_row_8bit(row):
    """Format a row of 8-bit pixels as C array data."""
    values = []
    for color, alpha in row:
        values.extend([f"0x{color:02x}", f"0x{alpha:02x}"])
    
    # Format in groups for readable output
    formatted = "    "
    for i, val in enumerate(values):
        formatted += val + ", "
        if (i + 1) % 20 == 0 and i < len(values) - 1:
            formatted += "\n    "
    
    return formatted.rstrip() + "\n"

def format_row_16bit(row, swap=False):
    """Format a row of 16-bit RGB565 pixels as C array data."""
    values = []
    for color, alpha in row:
        # Convert 8-bit color to RGB
        if color == 0x1c:  # Cyan
            r, g, b = 0, 224, 224
        else:  # Transparent/black
            r, g, b = 0, 0, 0
        
        # Encode to RGB565
        low, high = rgb565_encode(r, g, b)
        
        # Swap bytes if needed
        if swap:
            values.extend([f"0x{high:02x}", f"0x{low:02x}", f"0x{alpha:02x}"])
        else:
            values.extend([f"0x{low:02x}", f"0x{high:02x}", f"0x{alpha:02x}"])
    
    # Format in groups for readable output
    formatted = "    "
    for i, val in enumerate(values):
        formatted += val + ", "
        if (i + 1) % 20 == 0 and i < len(values) - 1:
            formatted += "\n    "
    
    return formatted.rstrip() + "\n"

def generate_icon_file(filename, icon_name, pixels):
    """Generate complete icon C file with both 8-bit and 16-bit formats."""
    with open(filename, 'w') as f:
        f.write(create_icon_header(icon_name))
        
        # Write 8-bit format
        f.write('  #if LV_COLOR_DEPTH == 1 || LV_COLOR_DEPTH == 8\n')
        f.write('    /*Pixel format: Alpha 8 bit, Red: 3 bit, Green: 3 bit, Blue: 2 bit*/\n')
        for row in pixels:
            f.write(format_row_8bit(row))
        f.write('  #endif\n')
        
        # Write 16-bit format (SWAP == 0)
        f.write('  #if LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP == 0\n')
        f.write('    /*Pixel format: Alpha 8 bit, Red: 5 bit, Green: 6 bit, Blue: 5 bit*/\n')
        for row in pixels:
            f.write(format_row_16bit(row, swap=False))
        f.write('  #endif\n')
        
        # Write 16-bit format (SWAP != 0)
        f.write('  #if LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP != 0\n')
        f.write('    /*Pixel format: Alpha 8 bit, Red: 5 bit, Green: 6 bit, Blue: 5 bit  BUT the 2  color bytes are swapped*/\n')
        for row in pixels:
            f.write(format_row_16bit(row, swap=True))
        f.write('  #endif\n')
        
        # Write structure definition
        f.write('};\n\n')
        f.write(f'''const lv_img_dsc_t {icon_name} = {{
  .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 50,
  .header.h = 50,
  .data_size = 2500 * LV_IMG_PX_SIZE_ALPHA_BYTE,
  .data = {icon_name}_map,
}};
''')

# Generate the icons
print("Generating menu_calib_icon.c...")
calib_pixels = create_calibration_icon()
generate_icon_file(
    'c:/Users/DanieleLongo/Documents/GitHub/RX5808-Div/Firmware/ESP32/RX5808/main/gui/lvgl_app/resources/menu_calib_icon.c',
    'menu_calib_icon',
    calib_pixels
)

print("Generating menu_finder_icon.c...")
finder_pixels = create_finder_icon()
generate_icon_file(
    'c:/Users/DanieleLongo/Documents/GitHub/RX5808-Div/Firmware/ESP32/RX5808/main/gui/lvgl_app/resources/menu_finder_icon.c',
    'menu_finder_icon',
    finder_pixels
)

print("Generating menu_spectrum_icon.c...")
spectrum_pixels = create_spectrum_icon()
generate_icon_file(
    'c:/Users/DanieleLongo/Documents/GitHub/RX5808-Div/Firmware/ESP32/RX5808/main/gui/lvgl_app/resources/menu_spectrum_icon.c',
    'menu_spectrum_icon',
    spectrum_pixels
)

print("Done! Icons generated successfully.")

