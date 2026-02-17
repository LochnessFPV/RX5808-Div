# ExpressLRS Backpack - Important Information

## ⚠️ NO ESP32 FIRMWARE CODE NEEDED FOR BACKPACK!

The files `elrs_backpack.c.WRONG_IMPLEMENTATION` and `elrs_backpack.h.WRONG_IMPLEMENTATION` contain an **incorrect implementation** based on a misunderstanding of how ExpressLRS VRX backpacks work.

---

## ❌ What Was Wrong

The original implementation assumed:
- Backpack connects to ESP32 via UART (GPIO16/GPIO17)
- Backpack sends CRSF/MSP commands to ESP32
- ESP32 firmware parses commands and controls RX5808

**This is NOT how ExpressLRS VRX backpacks work!**

---

## ✅ How It Actually Works

ExpressLRS VRX backpacks:
- Connect **directly to RX5808 chip** via SPI (on bottom board)
- Act as **SPI master** controlling RX5808 independently
- **Bypass ESP32 entirely** - no communication with ESP32
- Use standard RX5808 SPI protocol (same as ESP32 uses)

### Correct Wiring

```
EP1 Backpack → RX5808 Bottom Board (SPI):
├─ BOOT pad → Pin 1 (CLK)
├─ RX pad   → Pin 2 (DATA) 
├─ TX pad   → Pin 3 (CS)
├─ GND      → Pin 7 (GND)
└─ VCC      → Pin 9 (5V)
```

**NOT connected to ESP32 at all!**

---

## 📄 Correct Documentation

See: `Firmware/ESP32/ELRS_BACKPACK_CORRECTED_WIRING.md`

This document contains:
- Correct SPI wiring to RX5808 bottom board
- Flash instructions for EP1 backpack firmware
- Physical installation guide
- Troubleshooting

---

## 🗂️ What To Do With This Code

**You can safely delete:**
- `elrs_backpack.c.WRONG_IMPLEMENTATION`
- `elrs_backpack.h.WRONG_IMPLEMENTATION`

These files are kept for reference only to show what NOT to do.

**No changes needed to ESP32 firmware!** The backpack works independently.

---

## 🔄 SPI Bus Sharing

Both ESP32 and backpack control RX5808 via the same SPI bus:

| Condition | Who Controls RX5808 |
|-----------|---------------------|
| Backpack OFF | ESP32 ✅ |
| Backpack ON, not connected | ESP32 ✅ |
| Backpack connected to TX | Backpack ✅ (ESP32 blocked) |

**Recommendation:** Add physical switch to power off backpack when not in use.

---

## 🎯 Summary

- ✅ Backpack = Independent SPI master for RX5808
- ✅ Wiring: EP1 → RX5808 bottom board (NOT ESP32)
- ✅ Protocol: SPI (NOT UART/CRSF)
- ✅ ESP32 firmware: NO CODE CHANGES NEEDED
- ❌ Original UART implementation: COMPLETELY WRONG

---

**For correct setup, see `ELRS_BACKPACK_CORRECTED_WIRING.md`**
