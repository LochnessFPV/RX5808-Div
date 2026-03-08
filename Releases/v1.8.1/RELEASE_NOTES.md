# RX5808-Div Firmware v1.8.1 - Critical OSD Fix

**Release Date:** March 8, 2026  
**Type:** Bug Fix Release

## 🐛 Critical Bug Fix

### OSD Display Logic Corrected

**Fixed inverted OSD behavior** - The OSD (On-Screen Display) toggle was working backwards:

- **Previous Behavior (BROKEN):**
  - When **LOCKED** (Red button) → OSD was **ON** ❌  
  - When **UNLOCKED** (Grey button) → OSD was **OFF** ❌

- **Fixed Behavior:**
  - When **LOCKED** (Red button) → OSD is **OFF**, video passthrough active ✅
  - When **UNLOCKED** (Grey button) → OSD is **ON**, UI overlay displayed ✅

### Technical Details

**Root Cause:** Logic negation bug in [page_main.c:367-368](../../Firmware/ESP32/RX5808/main/gui/lvgl_app/page_main.c#L367-L368)

**Changes Made:**
- Removed incorrect `!` negation operator from `video_composite_switch(!lock_flag)` calls
- Added explicit OSD initialization on boot for state synchronization
- OSD now properly aligns with lock/unlock button behavior

**Files Modified:**
- `Firmware/ESP32/RX5808/main/gui/lvgl_app/page_main.c`  
  - Line 367: `video_composite_switch(lock_flag)` (was `!lock_flag`)  
  - Line 368: `video_composite_sync_switch(lock_flag)` (was `!lock_flag`)  
  - Lines 873-876: Added OSD state initialization

### Impact

**Severity:** HIGH - Core OSD functionality was completely inverted

**Affected Users:** Anyone using OSD feature in v1.7.x - v1.8.0

**Upgrade Recommendation:** **STRONGLY RECOMMENDED** for all OSD users

## 📦 Building from Source

**Important:** Pre-compiled binaries are not included in this release due to build environment setup issues.

### Prerequisites

1. **ESP-IDF v5.3.x** - [Installation Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
2. **Python 3.9+**
3. **Git**

### Build Instructions

```bash
# 1. Clone repository (if not already done)
git clone https://github.com/Ft-Available/RX5808-Div
cd RX5808-Div/Firmware/ESP32/RX5808

# 2. Set up ESP-IDF environment
# Windows:
C:\path\to\esp-idf\export.ps1

# Linux/macOS:
. $HOME/esp/esp-idf/export.sh

# 3. Build firmware
idf.py build

# 4. Flash to device
idf.py -p COMx flash  # Replace COMx with your port
```

### Output Files

After successful build, binaries are located in `build/`:
- `RX5808.bin` - Main firmware
- `bootloader/bootloader.bin` - Bootloader
- `partition_table/partition-table.bin` - Partition table

### Flash Addresses

```
0x1000  bootloader.bin
0x8000  partition-table.bin
0x10000 RX5808.bin
```

## 🔄 Upgrade Path

**From v1.7.x - v1.8.0:**
1. Build firmware from source (see above)
2. Flash all three binaries using ESP-IDF flash tool or esptool.py
3. OSD will now behave correctly

**Alternative:** Use existing v1.8.0 binaries if you don't use the OSD feature

## 📝 Known Issues

- None specific to v1.8.1

## 🙏 Credits

- **OSD Fix:** Bug identified and fixed based on user feedback
- **Original OSD Implementation:** ChisBread (林面包)

## 📨 Support

- **Issues:** https://github.com/Ft-Available/RX5808-Div/issues
- **Discussions:** https://github.com/Ft-Available/RX5808-Div/discussions

---

**Note:** This is a source-only release. Binaries must be built locally due to build environment configuration requirements.
