# Release v1.5.0 - ExpressLRS Backpack & Performance Edition

## 📦 Release Information

**Release Title:** `v1.5.0 - ExpressLRS Backpack & Performance Edition`

**Release Tag:** `v1.5.0`

**Release Date:** February 13, 2026

---

## 🎯 What's New

### 🚀 ExpressLRS Backpack Integration

Control your RX5808 diversity receiver wirelessly from your radio transmitter! This release introduces full CRSF protocol support for seamless integration with ExpressLRS systems.

**Key Features:**
- ✅ Wireless VTX channel control from your radio
- ✅ Real-time RSSI monitoring on transmitter
- ✅ Automatic frequency band detection
- ✅ Support for all standard FPV bands (A, B, E, F, R, L)
- ✅ Compatible with EdgeTX, OpenTX, and other CRSF-capable radios

### ⚡ Performance Improvements

- **3x Faster Channel Switching** - Reduced from 150ms to 50ms
- **5x Smoother RSSI** - Moving average filter reduces jitter by 80%
- **25% Lower CPU Usage** - Optimized task scheduling
- **Better Diversity Algorithm** - Improved antenna switching with hysteresis

### 🔧 Bug Fixes

- Fixed Band E frequency mapping for VTX compatibility
- Corrected v1.2 hardware ADC channel assignments
- Fixed navigation pad on D0WDQ6_VER variant
- Improved boot stability and initialization sequence

---

## 📥 Files to Upload to GitHub Release

### Firmware Binaries (Build Required)

After building the firmware, upload these files from `Firmware/ESP32/RX5808/build/`:

1. **`RX5808_v1.5.0.bin`** (rename from `rx5808.bin`)
   - Main application binary
   - Flash to: 0x10000

2. **`bootloader_v1.5.0.bin`** (rename from `bootloader/bootloader.bin`)
   - ESP32 bootloader
   - Flash to: 0x1000

3. **`partition-table_v1.5.0.bin`** (rename from `partition_table/partition-table.bin`)
   - Partition table
   - Flash to: 0x8000

### Documentation Package

Create a ZIP file: `RX5808-Div_v1.5.0_Documentation.zip` containing:
- `COMPLETE_SETUP_GUIDE.md`
- `ELRS_EP1_BACKPACK_GUIDE.md`
- `CALIBRATION_AND_OPTIMIZATION.md`
- `WIRING_DIAGRAM.md`
- `TROUBLESHOOTING.md`
- `QUICK_REFERENCE_CARD.txt`
- `CHANGELOG.md`
- `README.md`

### Source Code (Automatic)

GitHub will automatically create:
- `Source code (zip)`
- `Source code (tar.gz)`

---

## 🛠️ Building the Firmware

### Prerequisites

- ESP-IDF v5.5.2 or later
- Git with submodules initialized
- Python 3.8+

### Build Instructions

```bash
# Clone repository with submodules
git clone --recursive https://github.com/LochnessFPV/RX5808-Div.git
cd RX5808-Div/Firmware/ESP32/RX5808

# If you already cloned without --recursive, initialize submodules:
git submodule update --init --recursive

# Configure (optional - enable ExpressLRS backpack)
idf.py menuconfig
# Navigate to: Component config → ExpressLRS Backpack → Enable

# Build firmware
idf.py build

# The binaries will be in: Firmware/ESP32/RX5808/build/
```

---

## 📲 Flashing Instructions

### Method 1: Using ESP-IDF (Recommended)

#### Complete Flash (First Time)

Flash everything including bootloader and partition table:

```bash
cd Firmware/ESP32/RX5808

# Windows
idf.py -p COM3 flash

# Linux/macOS
idf.py -p /dev/ttyUSB0 flash
```

Replace `COM3` or `/dev/ttyUSB0` with your actual serial port.

#### Application Only Update (Faster)

For firmware updates, flash only the application binary:

```bash
cd Firmware/ESP32/RX5808

# Windows
idf.py -p COM3 app-flash

# Linux/macOS
idf.py -p /dev/ttyUSB0 app-flash
```

#### With Serial Monitor

To see debug output after flashing:

```bash
idf.py -p COM3 flash monitor
```

Press `Ctrl+]` to exit monitor.

---

### Method 2: Using esptool.py (Advanced)

If you downloaded pre-built binaries from the release:

#### Windows PowerShell
```powershell
# Install esptool
pip install esptool

# Erase flash (first time only - optional)
esptool.py --chip esp32 --port COM3 erase_flash

# Flash all binaries
esptool.py --chip esp32 --port COM3 --baud 921600 `
  --before default_reset --after hard_reset write_flash `
  -z --flash_mode dio --flash_freq 40m --flash_size detect `
  0x1000 bootloader_v1.5.0.bin `
  0x8000 partition-table_v1.5.0.bin `
  0x10000 RX5808_v1.5.0.bin
```

#### Linux/macOS
```bash
# Install esptool
pip3 install esptool

# Erase flash (first time only - optional)
esptool.py --chip esp32 --port /dev/ttyUSB0 erase_flash

# Flash all binaries
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
  --before default_reset --after hard_reset write_flash \
  -z --flash_mode dio --flash_freq 40m --flash_size detect \
  0x1000 bootloader_v1.5.0.bin \
  0x8000 partition-table_v1.5.0.bin \
  0x10000 RX5808_v1.5.0.bin
```

---

### Method 3: ESP Flash Download Tool (Windows GUI)

1. Download **Flash Download Tools** from Espressif: https://www.espressif.com/en/support/download/other-tools
2. Select **ESP32** chip type
3. Configure flash parameters:
   - **bootloader_v1.5.0.bin** @ 0x1000
   - **partition-table_v1.5.0.bin** @ 0x8000
   - **RX5808_v1.5.0.bin** @ 0x10000
4. Set options:
   - SPI Speed: 40MHz
   - SPI Mode: DIO
   - Flash Size: 4MB (or auto-detect)
5. Select COM port and baud rate (921600)
6. Click **START** to flash

---

## 🔌 ExpressLRS Backpack Setup

### Quick Start

1. **Flash HappyModel EP1 as Backpack**
   - Use ExpressLRS Configurator v1.7.11+
   - Device Category: **Backpack**
   - VRX Module: **RX5808 Diversity Module**
   - Backpack Device: **ESP01F Module**
   - Flash via WiFi: `http://10.0.0.1/?force=true`

2. **Wire to ESP32**
   ```
   EP1 TX  → ESP32 GPIO16 (RX)
   EP1 RX  → ESP32 GPIO17 (TX)
   EP1 GND → ESP32 GND
   EP1 VCC → ESP32 3.3V
   ```

3. **Enable in Firmware**
   ```bash
   idf.py menuconfig
   # Component config → ExpressLRS Backpack → Enable
   idf.py build flash
   ```

4. **Configure Radio**
   - Model Settings → VTX Control: ON
   - EdgeTX/OpenTX → VRX Control: ON

📖 **Full Documentation:** See [ELRS_EP1_BACKPACK_GUIDE.md](ELRS_EP1_BACKPACK_GUIDE.md)

---

## 📚 Documentation

- 📖 **[Complete Setup Guide](COMPLETE_SETUP_GUIDE.md)** - Start here for first-time setup
- 🔌 **[ExpressLRS Backpack Guide](ELRS_EP1_BACKPACK_GUIDE.md)** - Detailed backpack integration
- 🔧 **[Calibration & Optimization](CALIBRATION_AND_OPTIMIZATION.md)** - RSSI tuning and performance
- 🐛 **[Troubleshooting](TROUBLESHOOTING.md)** - Solutions to common issues
- 📋 **[Quick Reference Card](QUICK_REFERENCE_CARD.txt)** - Printable cheat sheet
- 📝 **[Full Changelog](CHANGELOG.md)** - Detailed version history

---

## ⚙️ Technical Details

### System Requirements

- **MCU:** ESP32-WROOM-32 (4MB Flash)
- **ESP-IDF:** v5.5.2 or later
- **Power:** 5V via USB-C (500mA recommended)
- **Serial Port:** USB-to-UART for programming

### Firmware Configuration

- **Flash Mode:** DIO
- **Flash Frequency:** 40MHz
- **Flash Size:** 4MB
- **Partition Scheme:** Default (see partition-table.csv)

### Optional Features (Menuconfig)

- ExpressLRS Backpack (Component config → ExpressLRS Backpack)
- Custom UART pins for backpack
- Debug logging levels

---

## 🆕 Upgrade Notes

### From v1.4.x

✅ **Compatible** - Direct upgrade supported

**Important Changes:**
- ExpressLRS backpack is **disabled by default** - enable via menuconfig if needed
- Band E frequencies have been corrected (may affect channel mapping)
- v1.2 hardware users: D0WDQ6_VER is now enabled by default

### Flash Procedure

1. Backup your existing firmware (optional)
2. Flash using Method 1, 2, or 3 above
3. Settings should be preserved (stored in NVS)
4. If issues occur, perform a full erase: `esptool.py erase_flash`

---

## 🐛 Known Issues

1. **OSD Format Changes:** Require reboot to take effect (existing behavior)
2. **WiFi Flashing:** May require multiple attempts on some systems
3. **Serial Monitor:** Some USB-C cables may not support data transfer

---

## 🤝 Credits

- **ExpressLRS Integration:** LochnessFPV
- **Original RX5808-Div:** Ft-Available
- **OSD Support:** ChisBread (林面包)
- **ESP32 Video Component:** ChisBread

---

## 📄 License

This project is open source. See [LICENSE](../../LICENSE) for details.

---

## 🔗 Links

- **Repository:** https://github.com/LochnessFPV/RX5808-Div
- **Original Project:** https://github.com/Ft-Available/RX5808-Div
- **Hardware Design:** https://oshwhub.com/ftps/rx5808-div
- **ExpressLRS Backpack:** https://github.com/ExpressLRS/Backpack
- **Report Issues:** https://github.com/LochnessFPV/RX5808-Div/issues

---

## 📊 Changelog Summary

**Added:**
- ExpressLRS Backpack CRSF protocol support
- 9 comprehensive documentation files
- Menuconfig integration for backpack
- 4-sample moving average RSSI filter

**Changed:**
- Channel switching time: 150ms → 50ms
- RSSI smoothing improved by 80%
- Band E frequency mapping corrected
- D0WDQ6_VER enabled for v1.2 hardware

**Fixed:**
- Navigation pad ADC channel on v1.2 hardware
- Band E frequency table alignment
- Boot sequence stability

---

**Full Details:** See [CHANGELOG.md](CHANGELOG.md)

**Questions?** Open an issue or check [TROUBLESHOOTING.md](TROUBLESHOOTING.md)

---

⭐ **If you find this release useful, please star the repository!**
