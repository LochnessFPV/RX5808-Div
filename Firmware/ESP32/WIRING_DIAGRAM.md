# Hardware Wiring Diagram - ExpressLRS Backpack

## ⚠️ CRITICAL: Connect Backpack to ESP32, NOT the RX5808 Receiver Chips!

The ExpressLRS backpack connects to the **ESP32 microcontroller** (the main brain), not to the RX5808 receiver modules themselves. The RX5808 chips are just analog video receivers with no UART interface - they are controlled by the ESP32 via SPI.

## Simple Connection Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                     RX5808 Diversity Module                     │
│                                                                 │
│  ┌────────────────────────────────────────────────────────┐   │
│  │              ESP32 (Main Controller) ← YOU WIRE HERE!  │   │
│  │                                                        │   │
│  │  GPIO16 (RX) ←───────┐  UART Interface for Backpack   │   │
│  │  GPIO17 (TX) ────────┼───┐                            │   │
│  │  GND ────────────────┼───┼───┐                        │   │
│  │  3.3V ───────────────┼───┼───┼───┐                    │   │
│  │                      │   │   │   │                    │   │
│  │  [Controls RX5808    │   │   │   │                    │   │
│  │   via SPI bus]       │   │   │   │                    │   │
│  │        ▼             │   │   │   │                    │   │
│  │  [RX5808 Module 0]   │   │   │   │  ← No UART here!  │   │
│  │  [RX5808 Module 1]   │   │   │   │  ← No UART here!  │   │
│  │  [LCD Display]       │   │   │   │                    │   │
│  │  [Buttons/Keys]      │   │   │   │                    │   │
│  └──────────────────────┼───┼───┼───┼────────────────────┘   │
│                         │   │   │   │                         │
└─────────────────────────┼───┼───┼───┼─────────────────────────┘
                          │   │   │   │
                          │   │   │   │  4-wire UART connection
            ┌─────────────┘   │   │   └─────────────┐
            │                 │   │                 │
            │  TX             │   │             VCC │
            │  (Transmit)     │   │           (3.3V)│
            │                 │   │                 │
            │              RX │   │ GND             │
            │          (Receive)  │ (Ground)        │
            │                 │   │                 │
      ┌─────┴─────────────────┴───┴─────────────────┴─────┐
      │                                                    │
      │          ExpressLRS Backpack Module                │
      │          (HappyModel EP1 or similar)               │
      │                                                    │
      │  ┌──────────────────────────────────────────┐     │
      │  │  ESP8285/ESP32 with ExpressLRS Firmware  │     │
      │  │  Configured as VRX Backpack              │     │
      │  │                                          │     │
      │  │  Baud: 420000 bps                        │     │
      │  │  Protocol: CRSF                          │     │
      │  └──────────────────────────────────────────┘     │
      │                                                    │
      │  [Antenna] ←→ Wireless Link to ExpressLRS TX      │
      │                                                    │
      └────────────────────────────────────────────────────┘
                              │
                              │ 2.4GHz/900MHz Wireless
                              │ CRSF Control Packets
                              ▼
      ┌────────────────────────────────────────────────────┐
      │         ExpressLRS TX (Radio Transmitter)          │
      │                                                    │
      │  - In FPV Goggles or Radio Controller              │
      │  - EdgeTX/OpenTX with VRX control enabled          │
      │  - Sends band/channel change commands              │
      │                                                    │
      └────────────────────────────────────────────────────┘
```

## Pin Assignment Table

| RX5808 ESP32 | ExpressLRS Backpack | Wire Color (Suggested) |
|--------------|---------------------|------------------------|
| GPIO16 (RX)  | TX (Transmit)       | 🟡 Yellow              |
| GPIO17 (TX)  | RX (Receive)        | 🟢 Green               |
| GND          | GND                 | ⚫ Black               |
| 3.3V         | VCC (3.3V)          | 🔴 Red                 |

⚠️ **Important**: TX connects to RX, RX connects to TX (crossover connection)

## Physical Layout Example

```
┌────────────────────────────────────────────────────────┐
│ FPV Goggles (e.g., Fatshark, Skyzone)                  │
│                                                        │
│  ┌─────────────┐ ┌─────────────────┐                  │
│  │   Battery   │ │  RX5808 Module  │                  │
│  │   Bay       │ │  (Main Board)   │                  │
│  └─────────────┘ └────────┬────────┘                  │
│                           │                            │
│                           │ (4-wire cable)             │
│                           │                            │
│                  ┌────────┴────────┐                   │
│                  │ ELRS Backpack   │                   │
│                  │ (Small PCB)     │                   │
│                  │  [Antenna]      │                   │
│                  └─────────────────┘                   │
│                                                        │
└────────────────────────────────────────────────────────┘
```

## Connector Options

### Option 1: Direct Solder
- Solder wires directly to ESP32 pins
- Most reliable, smallest footprint
- Requires soldering skill

### Option 2: 4-Pin JST Connector
- Use JST-SH or JST-PH connector
- Easy connection/disconnection
- Recommended for testing

```
JST Connector Pinout:
┌─────────────────────┐
│ 1  2  3  4         │  (Looking at socket)
└─────────────────────┘
  │  │  │  └─ VCC (3.3V)
  │  │  └─── GND
  │  └────── TX (GPIO17)
  └─────────RX (GPIO16)
```

### Option 3: Dupont Headers
- 4x 2.54mm headers
- Easy prototyping
- Less secure for flying

## Complete System Diagram

```
┌──────────────────────────────────────────────────────────────┐
│                    Complete FPV System                       │
└──────────────────────────────────────────────────────────────┘

    Pilot's Radio                  In Goggles             Drone
┌─────────────────┐          ┌──────────────────┐    ┌──────────┐
│                 │          │                  │    │          │
│   EdgeTX/OpenTX │          │  RX5808          │    │  Camera  │
│                 │          │  + ELRS Backpack │    │    +     │
│  [VRX Control]  │          │                  │    │   VTX    │
│   A/B/E/F/R/L   │          │  ┌─────────────┐ │    │  5.8GHz  │
│   Channels 1-8  │          │  │ ELRS Module │ │    │          │
│                 │          │  │  (Backpack) │ │    │          │
│  ┌───────────┐  │          │  └──────┬──────┘ │    │  ┌──────┐│
│  │ELRS Module│  │          │         │        │    │  │Antenna││
│  │  (TX)     │  │          │    ┌────┴─────┐  │    │  └───┬──┘│
│  └─────┬─────┘  │          │    │ ESP32    │  │    └──────┼───┘
│        │        │          │    │ RX5808   │  │           │
└────────┼────────┘          │    │ Receiver │  │           │
         │                   │    └────┬─────┘  │           │
         │                   │         │        │           │
         │ 2.4G/900MHz       │  ┌──────┴──────┐ │           │
         │ Control Link      │  │  Video IN   │ │  5.8GHz   │
         └───────────────────┴──┤  (from VTX) │◄├───────────┘
                             │  └─────────────┘ │  Video Link
                             └──────────────────┘

Control Flow:
1. Pilot selects channel on radio (e.g., "B3")
2. ExpressLRS TX sends MSP command via 2.4G/900MHz
3. Backpack in goggles receives command via UART
4. ESP32 RX5808 changes to Band B, Channel 3 (5771 MHz)
5. Video from drone VTX is now on correct frequency
6. RX5808 sends confirmation back to pilot's radio
```

## Voltage Considerations

| Backpack Type | Voltage | ESP32 Pin | Notes |
|---------------|---------|-----------|-------|
| ESP8285 | 3.3V | 3V3 pin | ✅ Safe direct connection |
| ESP32 (Backpack) | 3.3V | 3V3 pin | ✅ Safe direct connection |
| 5V Backpack | 5.0V | ⚠️ Requires level shifter | Use voltage divider or level shifter |

⚠️ **Warning**: Most ExpressLRS backpacks use 3.3V logic. If yours uses 5V, add a voltage divider:

```
5V TX → [1kΩ] → [2kΩ] → GND
                  │
                  └─→ ESP32 RX (GPIO16)
```

## Testing Checklist

- [ ] Connections checked: TX→RX, RX→TX (crossover)
- [ ] GND connected
- [ ] Power voltage verified (3.3V)
- [ ] No shorts between pins
- [ ] Backpack powered on and blinking
- [ ] Firmware compiled and flashed
- [ ] Monitor shows initialization message
- [ ] ExpressLRS TX configured for VRX control
- [ ] Test channel change from radio

## Common Mistakes

❌ **Wrong**: TX→TX, RX→RX (straight through)
✅ **Right**: TX→RX, RX→TX (crossover)

❌ **Wrong**: 5V power to ESP32 GPIO
✅ **Right**: 3.3V power or level shifter

❌ **Wrong**: Incorrect baud rate (115200)
✅ **Right**: CRSF baud rate (420000)

❌ **Wrong**: Backpack in TX mode
✅ **Right**: Backpack in VRX mode

## Mounting Ideas

1. **Inside goggles module bay** - Clean, protected
2. **External with double-sided tape** - Easy access
3. **3D printed bracket** - Professional look
4. **Velcro strap** - Removable, flexible

---

**Once connected, build and flash the firmware following [INTEGRATION_SUMMARY.md](INTEGRATION_SUMMARY.md)!**
