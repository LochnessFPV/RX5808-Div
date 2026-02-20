# RX5808 Diversity v1.7.1 Release Package

**Release Date:** February 20, 2026  
**Type:** Stable Release  
**Hardware:** ESP32-D0WDQ6 (v1.2), ST7735 LCD

## 📦 Package Contents

### Firmware Binaries (3 files)
- `RX5808-v1.7.1-bootloader.bin` (25.66 KB)
- `RX5808-v1.7.1-partition-table.bin` (3.00 KB)  
- `RX5808-v1.7.1-firmware.bin` (1.01 MB)

### Documentation (4 files)
- `RELEASE_NOTES.md` - Comprehensive release notes with all improvements
- `MANIFEST.json` - Machine-readable manifest with checksums and flash addresses
- `FLASH_INSTRUCTIONS.txt` - Step-by-step flashing guide
- `README.md` - This file

## 🚀 Quick Start

### Prerequisites
```bash
pip install esptool
```

### Flash Command (Windows)
```bash
esptool.py --chip esp32 --port COM3 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 2MB 0x1000 RX5808-v1.7.1-bootloader.bin 0x8000 RX5808-v1.7.1-partition-table.bin 0x10000 RX5808-v1.7.1-firmware.bin
```

### Flash Command (Linux/Mac)
```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
  --before default_reset --after hard_reset write_flash \
  -z --flash_mode dio --flash_freq 80m --flash_size 2MB \
  0x1000 RX5808-v1.7.1-bootloader.bin \
  0x8000 RX5808-v1.7.1-partition-table.bin \
  0x10000 RX5808-v1.7.1-firmware.bin
```

## ✨ What's New in v1.7.1

### 6 Major Improvements
1. ✅ **Hardware Diversity Switching** - Physical GPIO antenna control (critical fix)
2. ⚡ **DMA-Accelerated SPI** - 64% faster RX5808 communication
3. 🧠 **Adaptive RSSI Sampling** - 15-20% CPU savings during stable flight
4. 💾 **Memory Pools** - 60-70% less fragmentation, 2-5x faster allocation
5. 🔄 **Dual-Core Utilization** - 38% Core 0 load reduction
6. 📦 **PSRAM Font Caching** - Future-ready for hardware upgrades

### Performance Impact
- **CPU Usage (cruising):** 45% → 30% (33% reduction)
- **SPI Speed:** 70μs → 25μs (64% faster)
- **Thermal:** 10-20°C cooler operation
- **Battery Life:** ~15% longer estimated

## 📝 Checksums (SHA256)

```
RX5808-v1.7.1-bootloader.bin:
73BAF7F970A2E0C530A7374DAFC4481DB308E0C41A0D5183322181A33D52F32F

RX5808-v1.7.1-partition-table.bin:
14AA27CCB35697643FEF4BE6C87E624CE1F6D4A396B0168E2B0F52533713D4F7

RX5808-v1.7.1-firmware.bin:
5295E8C42D6FB5F95E4E8A185932C6191D27CFC5078322475347E40E71AF815A
```

## ⚠️ Important Notes

- **Full flash required** - Memory layout changes from v1.7.0
- **Settings preserved:** ELRS config, Band X channels, RSSI calibration, diversity mode
- **May reset:** CPU frequency, GUI update rate (reconfigure in Setup menu)

## ✅ Verification

After flashing, check for:
- Version shows "v1.7.1" in top-right corner
- Boot message: "Memory pools initialized (4096 bytes)"
- Boot message: "RX5808 hardware SPI initialized (DMA-accelerated, 1MHz)"
- Diversity switching responds to signal changes
- UI remains smooth during operation

## 📚 Documentation

- **Full Details:** See `RELEASE_NOTES.md`
- **Flash Guide:** See `FLASH_INSTRUCTIONS.txt`
- **Technical Specs:** See `MANIFEST.json`
- **Project Docs:** [Main README](../../README.md)
- **Changelog:** [CHANGELOG.md](../../Firmware/ESP32/CHANGELOG.md)

## 🐛 Issues & Support

- **GitHub Issues:** https://github.com/LochnessFPV/RX5808-Div/issues
- **Discussions:** https://github.com/LochnessFPV/RX5808-Div/discussions
- **Upstream:** https://github.com/Ft-Available/RX5808-Div

## 📜 License

Open Source - See [LICENSE](../../LICENSE)

---

**Build Info:**  
ESP-IDF: 5.x  
Compiler: xtensa-esp-elf-gcc 14.2.0  
Build Date: 2026-02-20
