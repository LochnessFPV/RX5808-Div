# v1.7.0 - ELRS Wireless VRX Backpack Support (2026-02-19)

## 🎉 Major New Features

### ELRS Wireless VRX Backpack Support
- **ESP-NOW Communication**: Receive channel commands from ELRS TX modules via WiFi
- **One-Click Binding**: Simple binding process through Setup menu
- **MSP v2 Protocol**: Full implementation of ELRS Backpack protocol (MSP_SET_VTX_CONFIG)
- **48-Channel Support**: All 6 bands (A, B, E, F, R, L) × 8 channels

### VTX Band Swap Feature
- **Configurable Band Mapping**: Optional R↔L band swap for non-standard VTX firmware
- **NVS Persistence**: Settings survive power cycles
- **Easy Toggle**: "Swap R/L" switch in Setup menu
- **Use Case**: Solves compatibility issues when VTX firmware has reversed R/L bands

## 🐛 Bug Fixes

### Hardware Tuning
- Fixed RX5808 not actually tuning when receiving remote channel commands
- Added proper frequency tuning call before channel switching
- Improved RSSI accuracy and video quality after remote changes
- Added 50ms PLL settling time for stable tuning

## 📚 Documentation

### New Documentation
- Complete ELRS setup guide
- Troubleshooting guide
- Protocol specification document
- Binding instructions with screenshots

### Updated Documentation
- README with ELRS feature overview
- CHANGELOG with detailed version history
- Setup instructions for wireless control

## 🔧 Technical Details

### Protocol Implementation
- ESP-NOW on WiFi channel 1
- MSP v2 with CRC8-DVB-S2 validation
- Processes MSP_SET_VTX_CONFIG (0x0059)
- Ignores telemetry wrapper (0x0011)
- Channel index mapping: 0-47

### Configuration Storage
- ELRS binding state persisted in NVS
- VTX band swap setting persisted in NVS
- Survives firmware updates (data partition preserved)

### GUI Integration
- ELRS bind button in Setup page
- VTX band swap toggle in Setup page
- Visual feedback for binding status
- Both English and Chinese language support

## 🚀 Getting Started

### First Time Setup
1. Flash firmware to RX5808-Div
2. Navigate to Setup menu
3. Press ELRS "Bind" button
4. Set TX to bind mode
5. Wait for successful binding (green checkmark)
6. Test channel switching from VTX Admin

### If R/L Bands Are Reversed
1. Navigate to Setup menu
2. Toggle "Swap R/L" to ON
3. Setting saves automatically
4. Test channels again

## 📦 Release Assets

This release includes:
- `rx5808-esp32-v1.7.0.bin` - Main application firmware (flash @ 0x10000)
- `rx5808-bootloader-v1.7.0.bin` - ESP32 bootloader (flash @ 0x1000)
- `rx5808-partition-table-v1.7.0.bin` - Partition table (flash @ 0x8000)
- `manifest.json` - Web configurator metadata

### Flashing Instructions

#### Via esptool (Command line)
```bash
esptool.py --port COM3 write_flash 0x1000 rx5808-bootloader-v1.7.0.bin 0x8000 rx5808-partition-table-v1.7.0.bin 0x10000 rx5808-esp32-v1.7.0.bin
```

#### Via ESP-IDF
```bash
cd Firmware/ESP32/RX5808
idf.py flash
```

## 🙏 Acknowledgments

Special thanks to:
- **@jp39** - Original ESP-NOW implementation and protocol research
- **ExpressLRS Team** - ELRS Backpack protocol specification
- **RX5808-Div Community** - Testing and feedback

## 🔗 Links

- [Project Repository](https://github.com/Ft-Available/RX5808-Div)
- [Full Changelog](https://github.com/Ft-Available/RX5808-Div/blob/main/CHANGELOG.md#v170---2026-02-19)
- [Documentation](https://github.com/Ft-Available/RX5808-Div#readme)
- [Issue Tracker](https://github.com/Ft-Available/RX5808-Div/issues)

---

**Note**: This release includes infrastructure for automated firmware builds and web-based updates. Future releases will automatically build and upload firmware files via GitHub Actions.
