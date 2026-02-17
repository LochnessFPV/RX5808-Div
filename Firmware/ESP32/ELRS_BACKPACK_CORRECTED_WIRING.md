# ⚠️ CORRECTED ExpressLRS Backpack Wiring for RX5808 Diversity

## 🚨 CRITICAL CORRECTION - Previous Guide Was WRONG!

**The previous guide incorrectly showed UART wiring to ESP32. This is NOT how ExpressLRS VRX backpacks work!**

---

## ✅ Correct Architecture

ExpressLRS VRX backpacks use **SPI to control the RX5808 chip directly**, bypassing the ESP32 entirely.

```
┌──────────────────────────────────────────┐
│  ExpressLRS TX (in goggles/radio)       │
└──────────────┬───────────────────────────┘
               │
               │ 2.4GHz/900MHz Wireless
               │ (SPI commands over RF)
               ▼
┌──────────────────────────────────────────┐
│  HappyModel EP1 (VRX Backpack Mode)      │
│  • Acts as SPI master                    │
│  • Controls RX5808 directly              │
│  • BOOT pad → CLK (SPI Clock)            │
│  • RX pad → DATA (SPI MOSI)              │
│  • TX pad → CS (Chip Select)             │
└──────────────┬────────────────────── ────┘
               │
               │ SPI Bus (5 wires)
               ▼
┌──────────────────────────────────────────┐
│  RX5808 BOTTOM BOARD ← Connect Here!     │
│  (Dual Receiver Module)                  │
│  • Pin 1: CLK  ← EP1 BOOT                │
│  • Pin 2: DATA ← EP1 RX                  │
│  • Pin 3: CS   ← EP1 TX                  │
│  • Pin 7: GND  ← EP1 GND                 │
│  • Pin 9: 5V   → EP1 VCC                 │
└──────────────┬───────────────────────────┘
               │
               │ 40-pin connector
               ▼
┌──────────────────────────────────────────┐
│  ESP32 TOP BOARD (Firmware v1.5.1+)      │
│  • NOT connected to EP1                   │
│  • Controls RX5808 via its own SPI        │
│  • Handles UI/display/settings            │
│  • Passive backpack detection (monitors   │
│    RSSI patterns to detect SPI conflicts) │
│  • Auto-yields SPI when backpack active   │
│  • Shows "ELRS" indicator when detected   │
└───────────────────────────────────────────┘
```

---

## 🔌 Why SPI to RX5808, Not UART to ESP32?

| Approach | Why Wrong/Right |
|----------|-----------------|
| ❌ UART to ESP32 | ESP32 would need to parse CRSF/MSP commands and forward to RX5808. Complex, high latency, not standard ExpressLRS approach. |
| ✅ SPI to RX5808 | Backpack controls RX5808 directly via SPI, same as ESP32 does. Standard ExpressLRS approach. Works with ANY VRX module. ESP32 firmware includes optional passive detection to avoid conflicts. |

**ExpressLRS backpack is designed to control "dumb" RX5808 chips that only understand SPI!**

### 📡 Diversity Module Architecture

**Your module has 2 RX5808 chips, but they share ONE SPI bus:**

```
        ┌─────────────────────────────┐
        │   RX5808 BOTTOM BOARD       │
        │                             │
        │  ┌──────────┐  ┌──────────┐ │
        │  │ RX5808 A │  │ RX5808 B │ │  ← Two receivers
        │  │ (Left)   │  │ (Right)  │ │
        │  └────┬─────┘  └────┬─────┘ │
        │       │             │        │
        │       └──────┬──────┘        │
        │              │               │
        │      [Shared SPI Bus]       │  ← Both chips on same bus
        │       CLK, DATA, CS         │
        │              │               │
        │      Pins 1, 2, 3, 7, 9     │  ← Single connection point
        └──────────────┼───────────────┘
                       │
                   [EP1 wires]         ← One set of 5 wires controls BOTH
```

**How it works:**
- When backpack sends frequency command → **both receivers change** to same frequency
- Diversity switching happens at analog level (RF switches select active antenna)
- ESP32 controls diversity algorithm, backpack controls frequency
- Same as how ESP32 currently controls both receivers with one SPI bus

**You only need ONE set of 5 wires!**

---

## 🛠️ Correct Wiring Table

| EP1 Backpack Pad | Function | RX5808 Bottom Board | Pin Number |
|------------------|----------|---------------------|------------|
| BOOT             | SPI CLK  | Clock signal        | Pin 1      |
| RX               | SPI DATA | Data/MOSI signal    | Pin 2      |
| TX               | SPI CS   | Chip Select         | Pin 3      |
| GND              | Ground   | Ground              | Pin 7      |
| VCC              | Power    | 5V supply           | Pin 9      |

**Important Notes:**
- This is **NOT** UART - the RX/TX pads are repurposed for SPI
- Wire to **bottom board connector pins**, not ESP32
- **Both RX5808 chips share the same SPI bus** - one set of wires controls both receivers
- ESP32 firmware **includes passive backpack detection** (v1.5.1+)
- Backpack and ESP32 share SPI bus - detection system manages control automatically

---

## 📋 Step-by-Step Installation

### 1. Flash EP1 as VRX Backpack

**Download correct firmware:**
- Go to: https://github.com/ExpressLRS/Backpack/releases
- Download: `Generic_RX5808_backpack_via_WiFi.bin`

**Flash via WiFi:**
1. Power EP1 (enters WiFi mode with fast LED blink)
2. Connect to `ExpressLRS RX` WiFi network
3. Open browser to: `http://10.0.0.1/?force=true` ← **?force=true is critical!**
4. Upload the `.bin` file
5. Wait ~30 seconds for LED to stabilize

**Verify:**
- EP1 LED behavior changes (slower blink)
- WiFi network may become `ExpressLRS VRX Backpack`

---

### 2. Locate RX5808 Bottom Board Pads

**Disassemble your goggles:**
1. Remove screws securing top board
2. Carefully lift ESP32 board
3. Expose RX5808 bottom board

**Identify connector pins:**

```
RX5808 Bottom Board - 40-Pin Connector (looking from top):
┌──────────────────────────────────────┐
│  Pin 1  - CLK    ← Wire EP1 BOOT     │  ← Shared by BOTH RX5808 chips
│  Pin 2  - DATA   ← Wire EP1 RX       │  ← Shared by BOTH RX5808 chips
│  Pin 3  - CS     ← Wire EP1 TX       │  ← Shared by BOTH RX5808 chips
│  Pin 4-6         - Not used           │
│  Pin 7  - GND    ← Wire EP1 GND      │  ← Common ground
│  Pin 8           - Not used           │
│  Pin 9  - 5V     → Wire EP1 VCC      │  ← Powers EP1
│  ...                                  │
└──────────────────────────────────────┘

NOTE: These SPI pins control BOTH RX5808 chips simultaneously.
      When backpack changes frequency, both receivers tune together.
```

**Alternative Method:**
- Check hardware files in `Hardware/BUT/` folder for exact pinout
- Or solder directly to RX5808 chip SPI pads if accessible

---

### 3. Solder Wires to EP1

EP1 requires soldering to pads (no JST connector for backpack mode):

```
EP1 Board (component side):
┌────────────────────────┐
│   [WiFi Antenna]       │
│                        │
│  BOOT ●  ← Solder wire (CLK)
│   GND ●  ← Solder wire (GND)
│    RX ●  ← Solder wire (DATA)
│    TX ●  ← Solder wire (CS)
│   VCC ●  ← Solder wire (5V)
│    EN ●  (not used)
└────────────────────────┘
```

**Recommended:**
- Use 28-30 AWG wire (thin, flexible)
- Keep wires <10cm for signal integrity
- Color code: Blue=CLK, Green=DATA, Yellow=CS, Black=GND, Red=VCC

---

### 4. Connect to RX5808 Bottom Board

**Physical Wiring:**

```
EP1 BOOT (Blue)   → RX5808 Pin 1 (CLK)
EP1 RX (Green)    → RX5808 Pin 2 (DATA)
EP1 TX (Yellow)   → RX5808 Pin 3 (CS)
EP1 GND (Black)   → RX5808 Pin 7 (GND)
EP1 VCC (Red)     → RX5808 Pin 9 (5V)
```

**Soldering Tips:**
- Use flux for clean joints
- Test continuity with multimeter
- Insulate with heat shrink or kapton tape
- Strain relief important (wires will move during reassembly)

---

### 5. Mounting EP1

**Options:**

**A. External Mount (Easiest):**
- Route wires through cooling vents
- Mount EP1 on outside of goggles with double-sided tape
- Keeps antenna clear

**B. Internal Mount (Cleanest):**
- Find spare space in goggle housing
- Route wires between boards
- May require trimming foam/plastic
- Ensure antenna not blocked by metal

---

### 6. Testing

**Power On Test:**
1. Reassemble goggles
2. Power on
3. EP1 LED should blink (searching for TX)
4. RX5808 should boot normally (ESP32 controls it initially)

**Backpack Connection Test:**
1. Enable TX backpack in your ExpressLRS radio module
2. Power on radio
3. EP1 LED should connect (fast blink → solid)
4. **ESP32 screen shows orange "ELRS" indicator** (bottom-right corner)
5. Backpack now has control of RX5808

**Frequency Control Test:**
1. On radio: ExpressLRS Lua Script → VRX Admin menu
2. Select Band R, Channel 1 (5658 MHz)
3. Goggles OSD should show frequency change
4. Video should appear (if VTX broadcasting)
5. Try changing frequency from ESP32 UI → should be blocked
6. Check serial logs show: "Backpack active - skipping frequency change"

**Auto-Recovery Test:**
1. Power off TX module
2. Wait ~5 seconds
3. "ELRS" indicator should disappear
4. ESP32 frequency controls should work again
5. Serial logs show: "Attempting to reclaim SPI bus from backpack"

---

## ⚠️ Important Behavior Notes

### SPI Bus Sharing Conflict - SOLVED ✅

**Problem:** ESP32 and EP1 backpack both control RX5808 via same SPI bus.

**Solution Implemented:** Firmware v1.5.1+ includes **passive backpack detection**

**Behavior:**
- When backpack powered OFF: ESP32 controls frequency ✅
- When backpack powered ON but not connected: ESP32 controls frequency ✅
- When backpack connected to TX: 
  - ESP32 detects backpack activity automatically (monitors RSSI patterns)
  - Orange "ELRS" indicator appears on screen
  - ESP32 pauses SPI writes to avoid conflicts
  - Backpack controls frequency ✅
- When TX disconnects:
  - After ~5 seconds, ESP32 auto-recovers
  - "ELRS" indicator disappears
  - ESP32 resumes normal control ✅

**Detection Details:**
- No extra hardware wire needed
- Software monitors RSSI stability every 70-500ms
- If no ESP32 frequency change for 2+ seconds AND signal present → backpack detected
- Auto-recovery after 5 seconds of backpack disconnection
- Can be disabled by setting `BACKPACK_DETECTION_ENABLED 0` in rx5808.c

**Manual Control:**
If you want full control, you can still add a physical switch to EP1 VCC line.

### Frequency Initialization

**Important:** Set RX5808 to **R8 (5917 MHz)** before first backpack use!

This ensures the backpack knows the current channel state. Do this once via ESP32 UI.

---

---

## 🔍 Troubleshooting

### Do I Need to Wire Both RX5808 Chips?

**NO! Only one set of 5 wires needed.**

**Why?** Both RX5808 chips (Receiver A and B) share a common SPI bus on the bottom board. The CLK, DATA, and CS pins on the connector are already wired internally to control BOTH chips simultaneously.

**Think of it like this:**
- ESP32 uses ONE SPI bus → controls BOTH receivers ✅
- Backpack uses SAME SPI bus → controls BOTH receivers ✅
- You connect to the shared bus pins (1, 2, 3) → controls BOTH receivers ✅

**Diversity switching** is handled separately by RF switches (not SPI). The backpack only changes frequency - ESP32 still manages which antenna is active.

---

### EP1 Powers On But No SPI Control

**Check:**
- ✅ Wired to BOTTOM board (RX5808), not TOP board (ESP32)
- ✅ BOOT→CLK, RX→DATA, TX→CS (correct pin mapping)
- ✅ Common ground connected (GND)
- ✅ 5V stable (measure at EP1 VCC pad - should be ~5V)
- ✅ All solder joints good (no cold joints or bridging)
- ✅ EP1 in VRX backpack mode (not standard RX firmware)

**Test:** Use logic analyzer on SPI lines when changing channel from radio

### EP1 Won't Connect to TX

**Check:**
- ✅ TX backpack enabled in radio ExpressLRS settings
- ✅ Binding phrase matches (if configured)
- ✅ TX backpack powered on and not in WiFi mode
- ✅ EP1 antenna connected and not damaged
- ✅ Distance <10m initially for testing

### Wrong Frequencies

**Check:**
- ✅ RX5808 set to R8 before first use
- ✅ VRX frequency table in Lua script matches RX5808 bands
- ✅ Using correct backpack firmware (Generic_RX5808, not RapidFire/SteadyView)

### ESP32 Controls Stop Working

**Expected behavior with firmware v1.5.1+:**

- **Orange "ELRS" indicator appears** when backpack detected
- ESP32 frequency changes are blocked automatically (no SPI conflicts)
- ESP32 UI still works (can navigate menus, change settings, etc.)
- RSSI display continues updating normally
- After ~5 seconds of backpack disconnection, ESP32 auto-recovers

**If "ELRS" indicator stays on permanently:**
- Backpack may be stuck in active state
- Try power cycling the backpack
- Check TX module is actually disconnected
- Manually force recovery: Navigate to Settings → Advanced → "Clear Backpack Detection"

**If detection doesn't work at all:**
- Check `BACKPACK_DETECTION_ENABLED` is set to `1` in rx5808.c
- Verify firmware version is v1.5.1 or newer
- Check serial logs for "ExpressLRS Backpack detected" messages

---

## 📖 Quick Reference Card

```
═══════════════════════════════════════════════════════════
  CORRECT EXPRESSLRS VRX BACKPACK WIRING
═══════════════════════════════════════════════════════════
CONNECT TO: RX5808 BOTTOM BOARD (NOT ESP32!)
PROTOCOL:   SPI (NOT UART!)
───────────────────────────────────────────────────────────
EP1 BOOT (Blue)   → RX5808 Pin 1 (CLK)
EP1 RX   (Green)  → RX5808 Pin 2 (DATA)
EP1 TX   (Yellow) → RX5808 Pin 3 (CS)
EP1 GND  (Black)  → RX5808 Pin 7 (GND)
EP1 VCC  (Red)    → RX5808 Pin 9 (5V)
───────────────────────────────────────────────────────────
✅  ESP32 FIRMWARE v1.5.1+ HAS BACKPACK DETECTION!
✅  BACKPACK CONTROLS RX5808 DIRECTLY VIA SPI!
✅  ORANGE "ELRS" INDICATOR SHOWS WHEN BACKPACK ACTIVE!
✅  AUTO-DETECTION & AUTO-RECOVERY - NO CONFLICTS!
═══════════════════════════════════════════════════════════
```

---

## 🎯 Summary

1. ✅ ExpressLRS backpack uses **SPI to control RX5808 directly**
2. ✅ Connect to **BOTTOM board** where RX5808 chips are located
3. ✅ EP1 pads are repurposed: BOOT→CLK, RX→DATA, TX→CS
4. ✅ **ESP32 firmware v1.5.1+ includes passive backpack detection**
5. ✅ Flash EP1 with `Generic_RX5808_backpack_via_WiFi.bin`
6. ✅ Orange "ELRS" indicator shows when backpack is active
7. ✅ Automatic SPI arbitration - no conflicts between ESP32 and backpack
8. ✅ Auto-recovery when backpack disconnects

**Optional:** Add physical switch to EP1 VCC line for manual power control

---

## 📚 Official References

- **ExpressLRS SteadyView Wiring:** https://www.expresslrs.org/hardware/backpack/backpack-vrx-setup/#steadyview-backpack-connection
- **ExpressLRS Generic RX5808:** https://www.expresslrs.org/hardware/backpack/backpack-vrx-setup/#generic-rx5808-connection  
- **Backpack Images:** https://github.com/ExpressLRS/Backpack/wiki/SkyZone-Wiring.jpg
- **RX5808 Datasheet:** https://github.com/sheaivey/rx5808-pro-diversity/blob/master/docs/rx5808-SMD-datasheet.pdf

---

## 🗑️ What Was Wrong

**The old guide and code were completely wrong!**

Deleted files (preserved as .WRONG_IMPLEMENTATION):
- `elrs_backpack.c` - UART/CRSF implementation not needed
- `elrs_backpack.h` - Headers not needed
- Old `ELRS_EP1_BACKPACK_GUIDE.md` sections about ESP32 UART wiring

**Why?**
- ExpressLRS backpack doesn't use UART to communicate with ESP32
- Backpack is a completely independent SPI master
- The backpack controls RX5808 the same way ESP32 does - via direct SPI commands

**What We Added Instead (v1.5.1+):**
- Passive backpack detection system in rx5808.c
- Orange "ELRS" status indicator in main UI
- Automatic SPI control yielding when backpack detected
- Auto-recovery when backpack disconnects

---

**Your backpack setup is now correct!** 📡✨
