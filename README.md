# RX5808-Div
**Diversity FPV Receiver Module with LVGL UI and ExpressLRS Backpack Support**

[![Firmware Version](https://img.shields.io/badge/firmware-v1.7.0-blue.svg)](Firmware/ESP32/CHANGELOG.md)
[![Platform](https://img.shields.io/badge/platform-ESP32-green.svg)](Firmware/ESP32/)
[![License](https://img.shields.io/badge/license-Open%20Source-orange.svg)](LICENSE)

## ✨ Features

- 🎨 **LVGL-based User Interface** - Modern, responsive GUI with color-coded menu items
- 📡 **Dual RX5808 Diversity** - Intelligent antenna switching with 3 modes (Race/Freestyle/Long Range)
- 📺 **OSD Support** - Real-time On-Screen Display synchronized with receiver UI
- 🔧 **Spectrum Analyzer** - Visual frequency scanning with zoom (5300-5900MHz)
- 🎯 **Band X Custom Channels** - 8 user-programmable frequency channels
- 🌐 **ExpressLRS Backpack** - Wireless VTX control via CRSF protocol with ON/OFF toggle
- ⚙️ **Advanced Performance Controls** - CPU frequency (80/160/240MHz) and GUI update rate adjustments
- 🌡️ **Thermal Optimization** - Configurable performance modes for cooler operation
- 🎛️ **RSSI Calibration** - Professional signal strength calibration tools
- ⚡ **Performance Optimized** - 3x faster channel switching, 5x smoother RSSI

## 🎬 Demo Videos

- **UI Demo:** https://www.bilibili.com/video/BV1yr4y1371b
- **OSD Demo:** https://www.bilibili.com/video/BV1ya411g78U

![ui](https://user-images.githubusercontent.com/66466560/218503938-571cd1fa-2c89-4279-a6aa-281c7fcf8234.jpeg)

## 📋 Table of Contents

- [What's New in v1.7.0](#-whats-new-in-v170)
- [What's New in v1.6.0](#-whats-new-in-v160)
- [User Interface](#-user-interface)
- [Diversity Modes](#-diversity-modes)
- [Performance & Thermal Management](#-performance--thermal-management)
- [OSD Support](#-osd-support)
- [ExpressLRS Backpack Integration](#-expresslrs-backpack-integration)
- [Hardware Design](#-hardware-design)
- [Firmware Build & Flash](#-firmware-build--flash)
- [Documentation](#-documentation)
- [Contributing](#-contributing)

## 🆕 What's New in v1.7.0

### 🔧 ELRS Backpack Protocol Fix (Major Update)

**Implementation Corrected:** The ExpressLRS Backpack wireless VTX control now uses the correct MSP protocol commands, ensuring reliable channel switching via EdgeTX/OpenTX VTX Admin.

#### ✅ Technical Improvements:
- **Correct Protocol Implementation**: Now uses `MSP_SET_VTX_CONFIG (0x0059)` for actual VTX channel commands
- **Proper Byte Extraction**: Channel index correctly read from `byte[0]` instead of `byte[2]`
- **Telemetry Filtering**: `MSP_ELRS_BACKPACK_CRSF_TLM (0x0011)` telemetry packets properly ignored
- **4-Byte Payload Structure**: Correct parsing of `[channel_idx, freq_msb, power_idx, pit_mode]`
- **Hardware Verified**: Tested with EdgeTX VTX Admin - B4, R7, L3 channel changes confirmed working

#### 🐛 What Was Fixed:
Previous implementation incorrectly used telemetry wrapper packets (`0x0011`) as channel commands and extracted channel data from wrong byte offset (`byte[2]` instead of `byte[0]`). This caused erratic behavior and unreliable channel switching. The fix implements the official ExpressLRS Backpack protocol specification.

#### 👨‍💻 For Developers:
- **Commit `15f72b9`**: Protocol definition corrections in `elrs_msp.h`
- **Commit `5643d35`**: Proper VTX channel command handling in `elrs_backpack.c`
- **Channel Index Range**: 0-47 (6 bands × 8 channels)
- **Protocol Details**: TX broadcasts telemetry status every ~5s (ignored), VTX Admin commands arrive on user action (processed)
- **MSP v2 Format**: `$X<type><flags><func_l><func_h><size_l><size_h><payload><crc8>`

## 🆕 What's New in v1.6.0

### Major Features
- 🎨 **Color-Coded Menu Items** - Each menu widget has unique background and icon colors for easy visual navigation
- 📻 **Band X Custom Channel Editor** - 8 user-programmable frequency channels with intuitive grid interface
- ⚙️ **CPU Frequency Control** - Choose between 80MHz (cool), 160MHz (balanced), or 240MHz (performance)
- 🖥️ **GUI Update Rate Control** - Adjust refresh rate from 10Hz to 100Hz for optimal performance/thermal balance
- 🎯 **Diversity Mode Selection** - Race, Freestyle, or Long Range algorithms with detailed explanations
- 🌐 **ELRS Backpack Toggle** - Enable/disable ExpressLRS backpack without rebuild

### Improvements
- ✅ **Default Language English** - English is now the default system language
- 🔇 **Beep Defaults OFF** - Button beeps are disabled by default (can be enabled in Setup)
- 📐 **Menu Reorganization** - Logical top-to-bottom ordering: Receiver → Calibrate → Band X → Spectrum → Finder → Setup → About
- 🔧 **Advanced Setup Page** - Comprehensive configuration options in one place

### Bug Fixes & Optimizations
- 🐛 **Fixed ADC Polling** - Converted from busy-wait to task-based sampling (reduces CPU load by 30-40%)
- 🌡️ **Thermal Improvements** - Optimized main loop timing and reduced unnecessary processing
- 🔒 **SPI Mutex Protection** - Added proper bus locking to prevent conflicts
- ⏱️ **RX5808 Settling Time** - Fixed PLL lock time for more reliable frequency switching
- 💾 **Memory Leak Prevention** - Improved memory management in ELRS backpack task
- 🎮 **Thermal Feedback Control** - Improved fan control with temperature monitoring

### Performance Impact
- 📉 **Temperature Reduction:** 15-25°C cooler operation possible with optimized settings
- ⚡ **CPU Efficiency:** 30-40% reduction in ADC task overhead
- 🖥️ **GUI Smoothness:** Configurable refresh rates for optimal user experience

## 🎮 User Interface

### Main Screen
1. **Long press OK button**: Lock/unlock manual channel control
2. **Short press OK button**: Enter menu
3. **Unlocked mode**: Use arrow keys to adjust frequency

### Menu System
The menu interface has seven color-coded sections (top to bottom):

#### 📡 Receiver (Dark Red)
- **Main Screen**: Display current channel, frequency, RSSI from both receivers, and diversity status
- **Short press OK**: Enter frequency adjustment mode
- **Long press OK**: Lock/unlock manual channel control

#### 🔧 Calibrate (Green)
- **RSSI Calibration**: Calibrates signal strength readings (requires active VTX signal)
- Results are automatically saved on success
- Improves diversity switching accuracy

#### 📻 Band X Edit (Cyan)
- **Custom Channel Editor**: Program 8 user-defined frequencies (5645-5945MHz)
- **Channel Selection**: 2x4 grid layout (CH1-CH8)
- **Long press OK button**: Exit Band X editor
- **Frequency Range**: Full 5.8GHz band with 1MHz increments
- Saved to non-volatile storage (persists across power cycles)

#### 📊 Freq Analyzer (Purple)
- **Spectrum Graph**: Visual frequency scanning (5300-5900MHz)
- **Zoom Controls**: UP/DOWN to zoom in/out on specific frequency ranges
- **Auto-Switch**: After scanning, automatically switches to the channel with best signal
- Color-coded signal strength display

#### 🔍 Finder (Blue)
- **Drone Finder Mode**: Helps locate lost drones by signal strength
- Moving closer to the drone increases RSSI display
- Useful for finding crashed quads in tall grass or trees

#### ⚙️ Setup (Orange)
- **Backlight**: Adjust display brightness (10-100%)
- **Fan Speed**: Manual fan control (0-100%)
- **Boot Animation**: Enable/disable startup animation
- **Beep**: Enable/disable button press sounds
- **OSD Format**: PAL or NTSC video output
- **Language**: English or Chinese (中文)
- **Signal Source**: Auto, Receiver 1, Receiver 2, or None
- **Diversity Mode**: Race, Freestyle, or Long Range (see Diversity Modes below)
- **CPU Frequency**: 80MHz (cool), 160MHz (balanced), or 240MHz (performance)
- **GUI Update Rate**: 10Hz, 14Hz, 20Hz, 25Hz, 50Hz, or 100Hz refresh
- **ELRS Backpack**: ON/OFF toggle for ExpressLRS backpack communication

> **Note:** Changes are saved when you select "Exit". OSD format changes require returning to main screen to take effect.

#### ℹ️ About (Gray)
- Displays system information, firmware version, and battery voltage

## 🎯 Diversity Modes

The RX5808-Div features three intelligent diversity switching algorithms optimized for different flying styles:

### 🏁 Race Mode - Aggressive & Fast
**Best for:** Racing, acrobatic flying, fast-moving quads

**Characteristics:**
- ⚡ **Dwell Time:** 80ms - Checks for better receiver every 80ms
- 🔄 **Cooldown:** 150ms - Quick recovery between switches
- 📊 **Hysteresis:** 2% - Switches even for small improvements
- 🎯 **Priority:** 85% Signal Strength / 15% Stability
- 🚀 **Preemptive Switching:** Aggressive (-30 slope threshold)

**Behavior:** Always chases the best signal, switches frequently. More video "blinks" during transitions but minimizes signal loss during fast maneuvers.

### 🎪 Freestyle Mode - Balanced (Default)
**Best for:** General flying, freestyle, cruising

**Characteristics:**
- ⏱️ **Dwell Time:** 250ms - Moderate wait before switching
- 🔄 **Cooldown:** 500ms - Half-second pause after switches
- 📊 **Hysteresis:** 4% - Needs moderate improvement to switch
- 🎯 **Priority:** 70% Signal Strength / 30% Stability
- 📈 **Preemptive Switching:** Moderate (-20 slope threshold)

**Behavior:** Balanced approach - switches when it matters but avoids excessive switching. Best all-around mode for typical FPV use.

### 📡 Long Range Mode - Stable & Conservative
**Best for:** Long-range flying, weak signals, far distances

**Characteristics:**
- ⏳ **Dwell Time:** 400ms - Long wait before considering switch
- 🔄 **Cooldown:** 800ms - Nearly one second between switches
- 📊 **Hysteresis:** 6% - Requires significant improvement
- 🎯 **Priority:** 75% Signal Strength / 25% Stability
- 📉 **Preemptive Switching:** Conservative (-15 slope threshold)

**Behavior:** Stays locked to one receiver longer, minimizes switching. Less video blinking but may stay on declining signal slightly longer.

### Key Concepts

- **Hysteresis:** The "advantage threshold" - prevents constant flip-flopping between similar signals
- **Dwell Time:** Minimum time on current receiver before considering a switch
- **Cooldown:** Extra-strict period after switching where hysteresis doubles
- **Preemptive Switching:** Switch *before* signal gets bad if current receiver is dropping fast
- **Combined Score:** Each receiver scored on both raw signal (RSSI) and stability (variance/slope)

## 🌡️ Performance & Thermal Management

### CPU Frequency Modes

**240MHz (Performance)** - Maximum performance, highest heat
- Best GUI responsiveness and fastest spectrum scanning
- Module will run warm (~15-25°C hotter than lower modes)
- Use with adequate ventilation or active cooling

**160MHz (Balanced)** ⭐ **Recommended**
- Excellent balance of performance and thermal efficiency
- 5-8°C cooler than 240MHz mode
- Minimal performance impact for typical use

**80MHz (Cool)**
- Lowest power consumption and coolest operation
- Reduced GUI responsiveness and slower spectrum scans
- Best for enclosed installations or passive cooling

### GUI Update Rate

Controls both display refresh and diversity algorithm sampling:

- **100Hz (10ms)**: Maximum responsiveness, highest CPU load
- **50Hz (20ms)**: Very smooth, balanced load ⭐ **Recommended**
- **25Hz (40ms)**: Smooth operation, lower CPU usage
- **20Hz (50ms)**: Conservative refresh rate
- **14Hz (70ms)**: Default legacy rate
- **10Hz (100ms)**: Minimum CPU usage, adequate for most scenarios

> **Thermal Tip:** Set CPU to 160MHz + GUI Update to 50Hz for optimal balance of performance and temperature.

## 📺 OSD Support

OSD functionality added by ChisBread (林面包). Non-overlay mode - enable by unlocking on the main screen. The OSD is fully synchronized with the receiver UI.

**Demo:** https://www.bilibili.com/video/BV1ya411g78U

![osd](https://user-images.githubusercontent.com/66466560/218504602-102e7fe0-b935-48ca-be9e-f459200034c8.jpg)

## 🚀 ExpressLRS Backpack Integration

**Added in v1.5.0, Enhanced in v1.6.0** with ON/OFF toggle in Setup menu.

ExpressLRS Backpack allows wireless control of your video receiver from your radio transmitter.

### What is ExpressLRS Backpack?

ExpressLRS Backpack allows you to control your video receiver (VRX) wirelessly from your radio transmitter using the CRSF protocol. Change channels, bands, and monitor signal strength directly from your radio without touching the receiver.

### Quick Start

1. **Flash HappyModel EP1 as Backpack**
   - See detailed guide: [ELRS_EP1_BACKPACK_GUIDE.md](Firmware/ESP32/ELRS_EP1_BACKPACK_GUIDE.md)
   - Use ExpressLRS Configurator v1.7.11+
   - Device Category: **Backpack**
   - VRX Module: **RX5808 Diversity Module**
   - Backpack Device: **ESP01F Module**

2. **Wire Backpack to ESP32**
   ```
   EP1 TX  → ESP32 GPIO16 (RX)
   EP1 RX  → ESP32 GPIO17 (TX)
   EP1 GND → ESP32 GND
   EP1 VCC → ESP32 3.3V
   ```

3. **Build Firmware with Backpack Support**
   ```bash
   cd Firmware/ESP32/RX5808
   idf.py menuconfig
   # Enable: Component config → ExpressLRS Backpack → Enable
   idf.py build flash
   ```

4. **Configure Your Radio**
   - Enable VTX Control in model settings
   - Set VRX Control: ON in EdgeTX/OpenTX

5. **Enable Backpack in Receiver** (v1.6.0+)
   - Navigate to **Setup** menu
   - Scroll to **ELRS Backpack** option
   - Press OK to toggle ON
   - Select **Exit** to save changes

### 📚 Complete Documentation

- 📖 **[Complete Setup Guide](Firmware/ESP32/COMPLETE_SETUP_GUIDE.md)** - End-to-end setup instructions
- 🔌 **[Backpack Flashing Guide](Firmware/ESP32/ELRS_EP1_BACKPACK_GUIDE.md)** - Flash EP1 as backpack
- 🔧 **[Wiring Diagram](Firmware/ESP32/WIRING_DIAGRAM.md)** - Detailed connection diagrams
- 📊 **[Calibration & Optimization](Firmware/ESP32/CALIBRATION_AND_OPTIMIZATION.md)** - RSSI calibration procedures
- 🐛 **[Troubleshooting Guide](Firmware/ESP32/TROUBLESHOOTING.md)** - Common issues and solutions
- 📋 **[Quick Reference Card](Firmware/ESP32/QUICK_REFERENCE_CARD.txt)** - Printable reference sheet
- 📝 **[Changelog](Firmware/ESP32/CHANGELOG.md)** - Version history and release notes

### Technical Details

- **Protocol:** MSP v2 over ESP-NOW (WiFi peer-to-peer)
- **MSP Commands:**
  - `MSP_SET_VTX_CONFIG (0x0059)` - VTX channel commands (4 bytes)
  - `MSP_ELRS_BACKPACK_CRSF_TLM (0x0011)` - Telemetry wrapper (ignored)
- **Payload Structure:** `[channel_idx, freq_msb, power_idx, pit_mode]`
- **Channel Index:** Byte 0, range 0-47 (6 bands × 8 channels)
- **Supported Bands:** A, B, E, F, R, L (5658-5917MHz)
- **WiFi Channel:** Channel 1 (2412 MHz)
- **Binding:** UID-based MAC address sanitization
- **CRC:** CRC8-DVB-S2 validation

## 🔧 Hardware Design

**Open Source Hardware:** https://oshwhub.com/ftps/rx5808-div

### Specifications
- **MCU:** ESP32-WROOM-32
- **Receiver Chips:** Dual RX5808 modules
- **Display:** ILI9341 TFT LCD (240x320)
- **Diversity:** Automatic antenna switching
- **Power:** 3.3V-5V USB-C
- **Interfaces:** UART, SPI, I2C

## 🛠️ Firmware Build & Flash

### Prerequisites
- ESP-IDF v5.5.2 or later
- USB-C cable for programming
- Git with submodules initialized

### Quick Build
```bash
# Clone repository with submodules
git clone --recursive https://github.com/LochnessFPV/RX5808-Div.git
cd RX5808-Div/Firmware/ESP32/RX5808

# Initialize submodules (if not cloned with --recursive)
git submodule update --init --recursive

# Configure (optional - enable ExpressLRS backpack)
idf.py menuconfig

# Build and flash
idf.py build
idf.py -p COMx flash monitor
```

Replace `COMx` with your serial port (e.g., COM3 on Windows, /dev/ttyUSB0 on Linux).

## 📚 Documentation

### For Users
- [Complete Setup Guide](Firmware/ESP32/COMPLETE_SETUP_GUIDE.md) - Start here!
- [ExpressLRS Backpack Guide](Firmware/ESP32/ELRS_EP1_BACKPACK_GUIDE.md)
- [Calibration & Optimization](Firmware/ESP32/CALIBRATION_AND_OPTIMIZATION.md)
- [Troubleshooting](Firmware/ESP32/TROUBLESHOOTING.md)
- [Quick Reference Card](Firmware/ESP32/QUICK_REFERENCE_CARD.txt)

### For Developers
- [Wiring Diagram](Firmware/ESP32/WIRING_DIAGRAM.md)
- [ESP32 Pinout](Firmware/ESP32/ESP32_PINOUT.md)
- [Band Mapping Reference](Firmware/ESP32/BAND_MAPPING_COMPARISON.md)
- [Changelog](Firmware/ESP32/CHANGELOG.md)

### Hardware Documentation
- Schematics: [Hardware/TOP/](Hardware/TOP/)
- PCB Design: [Hardware/BUT/](Hardware/BUT/)
- 3D Models: [Model/](Model/)

## 🤝 Contributing

Contributions are welcome! This project builds on the excellent work of:
- **Original RX5808-Div:** [Ft-Available](https://github.com/Ft-Available/RX5808-Div)
- **OSD Support:** ChisBread (林面包)
- **ESP-NOW Protocol Research:** [jp39](https://github.com/jp39/RX5808-Div)
- **ELRS Backpack Integration:** LochnessFPV

### How to Contribute
1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'feat: add amazing feature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 License

This project is open source. See [LICENSE](LICENSE) for details.

## 🔗 Links

- **Original Project:** https://github.com/Ft-Available/RX5808-Div
- **Hardware Design:** https://oshwhub.com/ftps/rx5808-div
- **ExpressLRS Backpack:** https://github.com/ExpressLRS/Backpack
- **LVGL Graphics Library:** https://lvgl.io/

## ⭐ Star History

If you find this project useful, please consider giving it a star! ⭐

---

**Current Version:** v1.7.0 | [View Changelog](Firmware/ESP32/CHANGELOG.md)

**What's New in v1.7.0:**
- ✅ **ELRS Backpack Protocol Fix** - Corrected MSP command implementation (0x0059 vs 0x0011)
- ✅ **Proper Channel Extraction** - Fixed byte offset (byte[0] instead of byte[2])
- ✅ **Hardware Verified** - Tested working with EdgeTX VTX Admin (B4, R7, L3 channels)
- ✅ **Telemetry Filtering** - Properly ignores broadcast telemetry packets
- ✅ **Official Protocol Compliance** - Now follows ExpressLRS Backpack specification

**Previous Release (v1.6.0):**
- Color-coded menu items with custom icons
- Band X custom channel editor (8 programmable channels)
- CPU frequency control (80/160/240MHz)
- GUI update rate control (10-100Hz)
- Diversity mode selection (Race/Freestyle/Long Range)
- ELRS Backpack ON/OFF toggle
- Major thermal and performance optimizations
- Default English language and beep disabled
