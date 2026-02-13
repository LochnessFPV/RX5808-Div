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

**⚠️ CRITICAL:**
- TX connects to RX (crossover!)
- RX connects to TX (crossover!)
- GND to GND (common ground)
- Check EP1 voltage: 3.3V or 5V compatible

### **EP1 Pinout (Standard ExpressLRS RX)**

```
EP1 Top View (looking at pins):
┌─────────────────┐
│  [Antenna]      │
│                 │
│  ● ● ● ● ● ●   │
│  1 2 3 4 5 6   │
└─────────────────┘

Pin 1: VCC (3.3-5V)
Pin 2: GND
Pin 3: RX  → Connect to ESP32 GPIO17 (TX)
Pin 4: TX  → Connect to ESP32 GPIO16 (RX)
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

1. **Power off everything**
2. **Identify EP1 pins** (use multimeter to verify)
3. **Connect wires:**
   ```
   EP1 TX → ESP32 GPIO16 (RX)
   EP1 RX → ESP32 GPIO17 (TX)
   EP1 GND → ESP32 GND
   EP1 VCC → ESP32 3.3V (if EP1 is 3.3V) or 5V (if EP1 is 5V)
   ```
4. **Double-check connections** (TX→RX, RX→TX!)
5. **Secure wires** (twist ties, heat shrink, or hot glue)

### **Voltage Compatibility:**

Most EP1 receivers work with **3.3V or 5V**. Check yours:
- **3.3V only**: Connect to ESP32 3V3 pin
- **5V tolerant**: Connect to ESP32 5V (VIN) pin
- **Unknown**: Use 3.3V to be safe

**ESP32 GPIO16/17 are 3.3V - DO NOT connect 5V to them directly!**

If EP1 outputs 5V on TX pin, use voltage divider:
```
EP1 TX (5V) → [1kΩ] → [2kΩ] → GND
                         ↓
                    ESP32 GPIO16 (3.3V max)
```

---

## ✅ Part 4: Testing

### **Test 1: Power On**
```powershell
idf.py -p COM4 monitor
```

Expected output:
```
I (477) ELRS_BACKPACK: Initializing ExpressLRS Backpack
I (477) ELRS_BACKPACK: ExpressLRS Backpack initialized (TX=17, RX=16, Baud=420000)
I (477) ELRS_BACKPACK: ExpressLRS Backpack task started
```

### **Test 2: Wireless Connection**
1. Power on ExpressLRS TX (in WiFi mode or normal mode)
2. EP1 LED should connect (fast blink or solid)
3. If not connecting, check binding phrase

### **Test 3: VRX Control**
1. On radio: Model Settings → VTX
2. Select Band A, Channel 1
3. Watch monitor output:
   ```
   I (5678) ELRS_BACKPACK: MSP Command: 0x58 from 0xEE
   I (5679) ELRS_BACKPACK: Setting VRX: Band=0, Ch=0, Freq=5865
   ```
4. RX5808 display should change to 5865 MHz

---

## 🔍 Troubleshooting

### No UART Communication

**Check:**
- ✅ TX→RX, RX→TX (crossover)
- ✅ Common ground (GND connected)
- ✅ Correct baud rate (420000)
- ✅ EP1 is in VRX backpack mode (not standard RX mode)

**Test:** Disconnect TX/RX wires and short them together (loopback test)

### EP1 Won't Connect to TX

**Fix:**
1. Binding phrase EXACTLY matches TX
2. Same regulatory domain
3. TX is powered on and not in WiFi mode
4. EP1 antenna connected

### Wrong Channels Selected

**Fix:**
- Verify band mapping in radio VTX table
- Check frequency mode vs band/channel mode

---

## 📖 Quick Reference Card

**Print and keep near workbench:**

```
═════════════════════════════════════════
  ELRS BACKPACK → ESP32 WIRING
═════════════════════════════════════════
EP1 TX  (Yellow) → ESP32 GPIO16 (RX)
EP1 RX  (Green)  → ESP32 GPIO17 (TX)  
EP1 GND (Black)  → ESP32 GND
EP1 VCC (Red)    → ESP32 3.3V
═════════════════════════════════════════
⚠️ CRITICAL: TX→RX, RX→TX (CROSSOVER!)
═════════════════════════════════════════
```

---

## 🎯 Summary

1. ✅ Flash EP1 with **ENABLE_VRX_BACKPACK** in Configurator
2. ✅ Connect EP1 to **ESP32** (not RX5808 chips!)
3. ✅ Use GPIO16/17 (UART) with crossover wiring
4. ✅ Test wireless control from radio
5. ✅ Enjoy wireless channel switching! 🚁

**Your backpack is now ready for wireless VRX control!** 📡✨
