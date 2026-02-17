# HappyModel EP1 RX as ExpressLRS Backpack - Complete Guide

## 📡 What is VRX Backpack Mode?

ExpressLRS **VRX Backpack** mode allows you to control your video receiver wirelessly from your radio transmitter via CRSF protocol. The EP1 RX becomes a bridge between your ExpressLRS TX and the RX5808 diversity module.

---

## 🔧 Part 1: Flash EP1 RX as VRX Backpack

### **Requirements:**
- HappyModel EP1 RX (or any ExpressLRS receiver)
- USB-C cable
- Computer with ExpressLRS Configurator
- Your binding phrase (keep it same as your TX)

### **Step 1: Download ExpressLRS Configurator**

1. Go to: https://github.com/ExpressLRS/ExpressLRS-Configurator/releases
2. Download latest version for Windows: `ExpressLRS-Configurator-Setup-x.x.x.exe`
3. Install and run the configurator

### **Step 2: Connect EP1 RX in Bootloader Mode**

**Method A: Via STLink Pads (Recommended)**
1. Locate the **BOOT** and **GND** pads on EP1 RX
2. Short BOOT to GND with tweezers
3. While shorted, plug in USB-C cable
4. Release the short
5. EP1 should appear as DFU device

**Method B: Via Button (if available)**
1. Hold the **BIND** button on EP1
2. Plug in USB-C cable
3. Release button after 3 seconds
4. EP1 enters bootloader mode

**Verify:** Windows should detect "STM32 BOOTLOADER" or show new COM port

### **Step 3: Flash VRX Backpack Firmware via WiFi**

**Modern Method (ExpressLRS 3.x+):**

1. **Put EP1 in WiFi Mode:**
   - Power on EP1 (LED should blink fast = WiFi mode)
   - If not in WiFi mode, short BOOT to GND briefly while powered

2. **Connect to EP1 WiFi:**
   - Scan for WiFi networks: `ExpressLRS RX`
   - Connect (no password needed)

3. **Open Web Update Page:**
   - Open browser to: `http://10.0.0.1/?force=true`
   - The `?force=true` is critical for repurposing as backpack!

4. **Download Pre-Built Backpack Firmware:**
   - Go to: https://github.com/ExpressLRS/Backpack/releases
   - Download: `Generic_RX5808_backpack_via_WiFi.bin` (for RX5808 modules)
   - Or: Use specific binary for your VRX module

5. **Upload to EP1:**
   - On the WiFi update page, click "Choose File"
   - Select the downloaded `.bin` file
   - Click "Update"
   - Wait ~30 seconds for LED to turn solid

**Alternative: Use ExpressLRS Configurator (Advanced)**

If you want to customize (binding phrase, home WiFi):

1. Open **ExpressLRS Configurator v1.7.11+**
2. **Device Category**: `Backpack` (separate section in configurator)
3. **VRX Module**: `RX5808 Diversity Module`
4. **Backpack Device**: `ESP01F Module` ← **This is the ONLY option!**
   - ℹ️ The HappyModel EP1 uses an ESP8285 chip internally (same as ESP01F)
   - ℹ️ There is NO separate "HappyModel EP1" device option in the configurator
   - ℹ️ "ESP01F Module" is the correct generic ESP8285 firmware for EP1 hardware
5. **Firmware Version**: Latest stable
6. **Binding Phrase**: Enter your TX binding phrase (optional)
7. **Home WiFi**: SSID/Password (optional, for future updates)
8. Click **"Build"** (creates firmware file)
9. Flash via WiFi as described above (use `?force=true`)

**Important Notes:**
- ⚠️ There is NO checkbox for "ENABLE_VRX_BACKPACK" - it's a separate firmware type!
- ℹ️ The RX5808 Diversity Module has only ONE backpack device option: ESP01F Module
- ⚠️ You're flashing "Backpack" firmware, not receiver firmware
- ⚠️ Always use `?force=true` when repurposing an existing receiver

### **Step 4: Verify Backpack Firmware**

1. EP1 will reboot (~30 seconds)
2. LED behavior changes (slower blinking)
3. WiFi network may change to `ExpressLRS VRX Backpack`

**If flash fails:**
- Ensure you used `?force=true` in URL
- Try different browser (Chrome/Edge recommended)
- Re-enter WiFi mode (power cycle EP1)
- Check downloaded file is not corrupted

### **Step 5: Verify VRX Backpack Mode**

After flashing:
1. Disconnect and reconnect EP1
2. Power on your ExpressLRS TX
3. EP1 LED should:
   - Blink slowly = Searching for TX
   - Solid/fast blink = Connected to TX
4. Open ExpressLRS Web UI (http://10.0.0.1 when TX in WiFi mode)
5. You should see EP1 listed as "VRX Backpack"

---

## 🔌 Part 2: Wiring - ESP32 vs RX5808 Clarification

### **CRITICAL: Connect Backpack to ESP32, NOT RX5808!**

```
                    CORRECT WIRING ✅
                    
    ┌─────────────────────────────────────────┐
    │  ExpressLRS TX (in goggles or radio)    │
    └──────────────┬──────────────────────────┘
                   │
                   │ 2.4GHz/900MHz Wireless
                   │ CRSF Protocol
                   ▼
    ┌──────────────────────────────────────────┐
    │  HappyModel EP1 (VRX Backpack Mode)      │
    │  ┌────────────────────────────────────┐  │
    │  │ UART Pins:                         │  │
    │  │ • TX  (pin 1)                      │  │
    │  │ • RX  (pin 2)                      │  │
    │  │ • GND (pin 3)                      │  │
    │  │ • VCC (pin 4) - 3.3V or 5V         │  │
    │  └─────┬──────────────────────────────┘  │
    └────────┼────────────────────────────────┘
             │
             │ UART Cable (4 wires)
             │ Crossover: TX→RX, RX→TX
             ▼
    ┌──────────────────────────────────────────┐
    │  ESP32 (Main Controller)                 │
    │  ┌────────────────────────────────────┐  │
    │  │ GPIO16 (RX) ← EP1 TX               │  │
    │  │ GPIO17 (TX) → EP1 RX               │  │
    │  │ GND         ← EP1 GND              │  │
    │  │ 3.3V        → EP1 VCC              │  │
    │  └─────┬──────────────────────────────┘  │
    │        │                                  │
    │        │ Controls RX5808 via SPI         │
    │        ▼                                  │
    │  ┌────────────────────────────────────┐  │
    │  │ RX5808 Module 0 (Antenna 1)        │  │
    │  │ RX5808 Module 1 (Antenna 2)        │  │
    │  └────────────────────────────────────┘  │
    └──────────────────────────────────────────┘
```

### **Why Connect to ESP32, Not RX5808?**

| Component | Role | Why |
|-----------|------|-----|
| **ESP32** | Main controller | Has UART hardware, runs firmware, controls everything |
| **RX5808** | Radio receiver chips | Only receive video, controlled via SPI by ESP32 |
| **EP1 Backpack** | Wireless control link | Sends CRSF commands to ESP32 via UART |

**The RX5808 modules don't have UART or intelligence - they're just analog radio receivers!**

### **Physical Wiring Table**

| EP1 Backpack Pin | Wire Color | ESP32 RX5808 Pin | ESP32 GPIO |
|------------------|------------|------------------|------------|
| TX (Transmit)    | 🟡 Yellow  | RX (Receive)     | GPIO16     |
| RX (Receive)     | 🟢 Green   | TX (Transmit)    | GPIO17     |
| GND              | ⚫ Black   | GND              | GND        |
| VCC (3.3-5V)     | 🔴 Red     | 3.3V or 5V       | 3V3 or VIN |

**⚠️ CRITICAL NOTES:**
- This is **SPI wiring**, NOT UART! BOOT/RX/TX pads are repurposed for SPI signals
- Wire directly to **RX5808 bottom board pads**, not ESP32 top board
- ESP32 firmware requires **NO modifications** - backpack controls RX5808 independently
- When backpack is active, ESP32 cannot control frequency (conflict on SPI bus)
- Use SteadyView wiring diagram as reference: https://github.com/ExpressLRS/Backpack/wiki/SkyZone-Wiring.jpg

### **Locating RX5808 Bottom Board Pads**

The RX5808 bottom board ("BUT" folder in hardware files) has exposed pads or pins for SPI:

```
RX5808 Bottom Board - Connector Pinout (to TOP board):
┌─────────────────────────────────────────┐
│ Pin  Function    EP1 Connection         │
├─────────────────────────────────────────┤
│  1   CLK         ← BOOT pad (Blue)      │
│  2   DATA        ← RX pad (Green)       │
│  3   CS          ← TX pad (Yellow)      │
│  4   ?           Not used               │
│  5   ?           Not used               │
│  6   ?           Not used               │
│  7   GND         ← GND (Black)          │
│  8   ?           Not used               │
│  9   5V          ← VCC (Red)            │
│ ...  ...         ...                    │
└─────────────────────────────────────────┘
```

**Alternative:** Solder directly to RX5808 chip SPI pads if pins not accessible.

### **EP1 Pad Locations (Repurposed for SPI)**

```
EP1 Board (Component Side):
┌───────────────────────────┐
│    [WiFi Antenna]         │
│                           │
│  BOOT ●  ← CLK signal     │
│   GND ●                   │
│    RX ●  ← DATA signal    │
│    TX ●  ← CS signal      │
│   VCC ●  ← 5V power       │
│    EN ●  (not used)       │
└───────────────────────────┘
```

**Note:** You'll need to solder wires to BOOT, RX, TX, GND, and VCC pads on EP1.
Pin 5: (optional telemetry)
Pin 6: (optional)
```

**Verify your EP1 pinout - some models differ!**

---

## 🛠️ Part 3: Complete Connection Guide

### **Materials Needed:**
- 4x Dupont wires or JST connector
- Soldering iron (if permanent install)
- Multimeter (to check voltages)

### **Step-by-Step Wiring:**

1. **Disassemble goggles** - remove TOP board to access BOTTOM board
2. **Identify RX5808 bottom board SPI pads/pins** (CLK, DATA, CS, GND, 5V)
3. **Prepare EP1:**
   - Solder 5 wires to: BOOT, RX, TX, GND, VCC pads
   - Use thin wire (~28-30 AWG)
   - Keep wires short (<10cm) for signal integrity
4. **Connect wires:**
   ```
   EP1 BOOT → RX5808 CLK  (Pin 1)
   EP1 RX   → RX5808 DATA (Pin 2)
   EP1 TX   → RX5808 CS   (Pin 3)
   EP1 GND  → RX5808 GND  (Pin 7)
   EP1 VCC  → RX5808 5V   (Pin 9)
   ```
5. **Test continuity** with multimeter
6. **Insulate connections** (heat shrink, kapton tape)
7. **Mount EP1** externally or in spare space
8. **Reassemble goggles**

### **Voltage Compatibility:**

EP1 backpack mode uses **5V directly from RX5808 board**:
- **Power source**: RX5808 Pin 9 (5V supply)
- **Current draw**: ~80-150mA typical
- **SPI signals**: 3.3V logic (RX5808 is 3.3V, EP1 is 5V tolerant)

**Important:** The RX5808 SPI signals are 3.3V. EP1 must be 3.3V/5V input tolerant (most ESP8285 modules are).

---

## ✅ Part 4: Testing

### **Test 1: Power On**
1. Power on goggles with EP1 connected to RX5808 bottom board
2. EP1 LED should blink (searching for TX backpack)
3. RX5808 should initialize normally

**Note:** ESP32 firmware requires NO changes - backpack controls RX5808 independently!

### **Test 2: Wireless Connection**
1. Power on ExpressLRS TX with backpack enabled
2. EP1 LED should connect (fast blink or solid)
3. Backpack now has SPI control of RX5808

### **Test 3: VRX Control from Radio**
1. On radio: ExpressLRS Lua Script → VRX Admin
2. Select Band A, Channel 1 (5865 MHz)
3. RX5808 should change frequency (OSD will show new frequency)
4. Video should display on goggles

### **⚠️ Important Behavior:**
- When backpack is powered and connected, **ESP32 cannot control frequency**
- Both backpack and ESP32 share the SPI bus to RX5808
- To use ESP32 controls: Power off backpack or disconnect it
- Recommended: Add physical switch to disconnect EP1 VCC when not using backpack

---

## 🔍 Troubleshooting

### No SPI Communication / Backpack Not Controlling RX5808

**Check:**
- ✅ Wired to BOTTOM board (RX5808), not TOP board (ESP32)
- ✅ BOOT→CLK, RX→DATA, TX→CS (correct SPI mapping)
- ✅ Common ground connected
- ✅ 5V power stable (measure at EP1 VCC pad)
- ✅ EP1 is in VRX backpack mode (blue LED behavior)
- ✅ Solder joints good (no cold joints)

**Test:** Power on - EP1 LED should show connection status

### EP1 Won't Connect to TX Backpack

**Fix:**
1. TX backpack enabled in ExpressLRS TX module settings
2. Binding phrase matches (if used)
3. TX backpack powered on
4. EP1 antenna connected (critical for 2.4GHz)

### ESP32 Controls Don't Work When Backpack Powered

**Normal behavior!** Backpack and ESP32 share SPI bus.

**Solutions:**
- Power off backpack when using ESP32 controls
- Add physical switch to EP1 VCC line
- Use backpack OR ESP32 controls, not both simultaneously

### Wrong Frequencies Selected

**Fix:**
- Verify VRX band/channel table in ExpressLRS TX Lua script
- Ensure RX5808 was set to R8 (5917 MHz) before first backpack use
- Check RX5808 SPI frequency compatibility

---

## 📖 Quick Reference Card

**Print and keep near workbench:**

```
═════════════════════════════════════════════════════
  ELRS BACKPACK → RX5808 BOTTOM BOARD (SPI WIRING)
═════════════════════════════════════════════════════
EP1 BOOT (Blue)  → RX5808 CLK  (Pin 1)
EP1 RX   (Green) → RX5808 DATA (Pin 2)
EP1 TX   (Yellow)→ RX5808 CS   (Pin 3)
EP1 GND  (Black) → RX5808 GND  (Pin 7)
EP1 VCC  (Red)   → RX5808 5V   (Pin 9)
═════════════════════════════════════════════════════
⚠️ CRITICAL: WIRE TO BOTTOM BOARD, NOT ESP32!
⚠️ THIS IS SPI, NOT UART!
⚠️ NO ESP32 FIRMWARE CHANGES NEEDED!
═════════════════════════════════════════════════════
```

---

## 🎯 Summary

1. ✅ Flash EP1 with **RX5808 VRX Backpack** firmware (Generic_RX5808_backpack)
2. ✅ Wire EP1 to **RX5808 BOTTOM board** using SPI (BOOT→CLK, RX→DATA, TX→CS)
3. ✅ Power EP1 from RX5808 5V (Pin 9)
4. ✅ **NO ESP32 firmware changes needed** - backpack controls RX5808 independently via SPI
5. ✅ Enable TX backpack in ExpressLRS radio
6. ✅ Test wireless control from radio VRX Admin menu
7. ✅ Optional: Add switch to power off backpack when using ESP32 controls

**Key Points:**
- Backpack connects to **BOTTOM board** (RX5808 chips), not TOP board (ESP32)
- Uses **SPI protocol**, not UART
- ESP32 and backpack share SPI bus - only one can control at a time
- Backpack acts as SPI master - no ESP32 code needed!

**Your backpack is now ready for wireless VRX control!** 📡✨

---

## 📚 Additional References

- ExpressLRS Official SteadyView Wiring: https://www.expresslrs.org/hardware/backpack/backpack-vrx-setup/#steadyview-backpack-connection
- ExpressLRS Generic RX5808 Guide: https://www.expresslrs.org/hardware/backpack/backpack-vrx-setup/#generic-rx5808-connection
- RX5808 Datasheet: https://github.com/sheaivey/rx5808-pro-diversity/blob/master/docs/rx5808-SMD-datasheet.pdf
