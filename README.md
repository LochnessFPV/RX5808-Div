# RX5808-Div
**Diversity FPV Receiver Module with LVGL UI and ExpressLRS Backpack Support**

[![Firmware Version](https://img.shields.io/badge/firmware-v1.5.0-blue.svg)](Firmware/ESP32/CHANGELOG.md)
[![Platform](https://img.shields.io/badge/platform-ESP32-green.svg)](Firmware/ESP32/)
[![License](https://img.shields.io/badge/license-Open%20Source-orange.svg)](LICENSE)

## ✨ Features

- 🎨 **LVGL-based User Interface** - Modern, responsive GUI with touch navigation
- 📡 **Dual RX5808 Diversity** - Automatic antenna switching for optimal signal
- 📺 **OSD Support** - Real-time On-Screen Display synchronized with receiver UI
- 🔧 **Spectrum Analyzer** - Visual frequency scanning (5300-5900MHz)
- 🎯 **RSSI Calibration** - Professional signal strength calibration tools
- 🌐 **ExpressLRS Backpack** - Wireless VTX control via CRSF protocol (NEW in v1.5.0)
- ⚡ **Performance Optimized** - 3x faster channel switching, 5x smoother RSSI

## 🎬 Demo Videos

- **UI Demo:** https://www.bilibili.com/video/BV1yr4y1371b
- **OSD Demo:** https://www.bilibili.com/video/BV1ya411g78U

![ui](https://user-images.githubusercontent.com/66466560/218503938-571cd1fa-2c89-4279-a6aa-281c7fcf8234.jpeg)

## 📋 Table of Contents

- [User Interface](#-user-interface)
- [OSD Support](#-osd-support)
- [ExpressLRS Backpack Integration](#-expresslrs-backpack-integration-new)
- [Hardware Design](#-hardware-design)
- [Firmware Build & Flash](#-firmware-build--flash)
- [Documentation](#-documentation)
- [Contributing](#-contributing)

## 🎮 User Interface

### Main Screen
1. **Long press OK button**: Lock/unlock manual channel control
2. **Short press OK button**: Enter menu
3. **Unlocked mode**: Use arrow keys to adjust frequency

### Menu System
The menu interface has three main sections:

#### 📊 Scan Menu
- **Graph Scan**: Displays signal strength across 5300-5900MHz frequency range
- **Table Scan**: Shows VTX channel signal strength with color coding. After scanning, automatically switches to the channel with the best signal strength (displayed in top-right corner)
- **RSSI Calibration**: Calibrates RSSI readings (requires active VTX signal). Results are automatically saved on success

#### ⚙️ Settings
Configure display backlight intensity, fan speed, boot animation, buzzer, and OSD format. 
> **Note:** OSD format changes require saving and returning to main screen to take effect.

#### ℹ️ About
Displays system information and firmware version.

## 📺 OSD Support

OSD functionality added by ChisBread (林面包). Non-overlay mode - enable by unlocking on the main screen. The OSD is fully synchronized with the receiver UI.

**Demo:** https://www.bilibili.com/video/BV1ya411g78U

![osd](https://user-images.githubusercontent.com/66466560/218504602-102e7fe0-b935-48ca-be9e-f459200034c8.jpg)

## 🚀 ExpressLRS Backpack Integration (NEW!)

**Version 1.5.0** introduces full ExpressLRS VRX backpack support for wireless channel control from your radio transmitter.

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

### 📚 Complete Documentation

- 📖 **[Complete Setup Guide](Firmware/ESP32/COMPLETE_SETUP_GUIDE.md)** - End-to-end setup instructions
- 🔌 **[Backpack Flashing Guide](Firmware/ESP32/ELRS_EP1_BACKPACK_GUIDE.md)** - Flash EP1 as backpack
- 🔧 **[Wiring Diagram](Firmware/ESP32/WIRING_DIAGRAM.md)** - Detailed connection diagrams
- 📊 **[Calibration & Optimization](Firmware/ESP32/CALIBRATION_AND_OPTIMIZATION.md)** - RSSI calibration procedures
- 🐛 **[Troubleshooting Guide](Firmware/ESP32/TROUBLESHOOTING.md)** - Common issues and solutions
- 📋 **[Quick Reference Card](Firmware/ESP32/QUICK_REFERENCE_CARD.txt)** - Printable reference sheet
- 📝 **[Changelog](Firmware/ESP32/CHANGELOG.md)** - Version history and release notes

### Technical Details

- **Protocol:** CRSF (Crossfire) over UART
- **Baud Rate:** 420000 bps
- **Commands:** MSP VTX_CONFIG (0x58, 0x88)
- **Supported Bands:** A, B, E, F, R, L (5658-5917MHz)
- **Pin Configuration:** GPIO16 (RX), GPIO17 (TX)

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
- **ExpressLRS Backpack:** LochnessFPV

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

**Current Version:** v1.5.0 | [View Changelog](Firmware/ESP32/CHANGELOG.md)
