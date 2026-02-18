# ExpressLRS Backpack Troubleshooting Guide

> **📌 v1.7.0 Protocol Update:**  
> This guide reflects the **corrected MSP protocol implementation** in v1.7.0. If you're experiencing issues with earlier versions showing `0x58` or `0x88` commands, update to v1.7.0 for proper `MSP_SET_VTX_CONFIG (0x0059)` handling.
> 
> **Key Changes:** Channel commands now use `0x0059` (not `0x0011`), channel extracted from `byte[0]` (not `byte[2]`), and telemetry packets are properly filtered.

## Quick Diagnostics

### Check 1: Monitor Output
```bash
idf.py -p COM3 monitor
```

Expected output on boot:
```
I (1234) ELRS_BACKPACK: Initializing ExpressLRS Backpack
I (1235) ELRS_BACKPACK: ExpressLRS Backpack initialized (TX=17, RX=16, Baud=420000)
I (1236) ELRS_BACKPACK: ExpressLRS Backpack task started
```

### Check 2: UART Test
Add test code to send a message:
```c
uart_write_bytes(UART_NUM_1, "TEST\n", 5);
```

Should see on backpack (if backpack has debug output).

---

## Problem: No Initialization Message

### Symptoms
- No "Initializing ExpressLRS Backpack" in monitor
- Backpack code not running

### Solutions

#### Solution 1: Check Configuration
```bash
idf.py menuconfig
# RX5808 Configuration → Enable ExpressLRS Backpack = [*]
```

#### Solution 2: Remove ifdef
Edit [system.c](main/hardware/system.c):
```c
// Remove this:
#ifdef CONFIG_ELRS_BACKPACK_ENABLE
elrs_backpack_init();
#endif

// Use this:
elrs_backpack_init();
```

#### Solution 3: Check Build Output
```bash
idf.py build 2>&1 | grep elrs_backpack
```
Should show compilation of elrs_backpack.c

---

## Problem: Build Errors

### Error: `uart.h: No such file or directory`

**Cause**: ESP-IDF UART driver not enabled

**Solution**:
```bash
idf.py menuconfig
# Component config → Driver configurations → UART → Enable UART = [*]
```

Or add to sdkconfig:
```
CONFIG_UART_ENABLE=y
```

### Error: `undefined reference to elrs_backpack_init`

**Cause**: elrs_backpack.c not compiled

**Solution 1**: Check CMakeLists.txt includes driver folder
```cmake
file(GLOB_RECURSE SOURCES 
    "./driver/*.c"
    # ... other patterns
)
```

**Solution 2**: Force rebuild
```bash
idf.py fullclean
idf.py build
```

### Error: `Multiple definition of elrs_backpack_init`

**Cause**: elrs_backpack_example.c being compiled (only for reference)

**Solution**: Rename to .txt or move outside main/:
```bash
mv main/driver/elrs_backpack_example.c main/driver/elrs_backpack_example.txt
```

---

## Problem: No CRSF Communication

### Symptoms
- Backpack initializes
- No `MSP_SET_VTX_CONFIG (0x0059)` messages when changing channel on TX via VTX Admin

### What You Should See (v1.7.0+)

**Normal Operation:**
```
I (12345) ELRS_BP: Ignoring MSP_ELRS_BACKPACK_CRSF_TLM (0x0011) - telemetry wrapper
I (17890) ELRS_BP: ==== MSP_SET_VTX_CONFIG (0x0059) ====
I (17890) ELRS_BP: Payload: 0B 00 03 00
I (17890) ELRS_BP: Channel from byte[0] = 0x0B (11 decimal)
I (17900) ELRS_BP: >>> CHANNEL CHANGED: B4 (index 11) <<<
```

**Expected Behavior:**
- `0x0011` packets every ~5 seconds → **Ignored** (telemetry broadcasts)
- `0x0059` packets on user action → **Processed** (VTX Admin channel commands)

### Diagnostic Steps

#### Step 1: Check Wiring
```
┌──────────┐     ┌──────────┐
│  ESP32   │     │ Backpack │
├──────────┤     ├──────────┤
│ GPIO16 RX│ ←─  │ TX       │  Check: TX → RX
│ GPIO17 TX│  ─→ │ RX       │  Check: RX → TX
│ GND      │ ─── │ GND      │  Check: Common ground
│ 3.3V     │ ──→ │ VCC      │  Check: 3.3V not 5V
└──────────┘     └──────────┘
```

Use multimeter:
- Continuity test between pins
- Voltage test: Should read 3.3V on VCC

#### Step 2: Check Baud Rate
Edit [elrs_backpack.h](main/driver/elrs_backpack.h) to match backpack:
```c
#define ELRS_UART_BAUD_RATE    420000  // Standard CRSF
// or try:
#define ELRS_UART_BAUD_RATE    115200  // Slower for testing
```

#### Step 3: Enable UART Debug
Add to [elrs_backpack.c](main/driver/elrs_backpack.c) `elrs_backpack_task()`:
```c
case UART_DATA:
    if (event.size > 0) {
        int len = uart_read_bytes(ELRS_UART_NUM, data, 
                                 event.size, pdMS_TO_TICKS(100));
        if (len > 0) {
            // ADD THIS DEBUG:
            ESP_LOGI(TAG, "Received %d bytes", len);
            ESP_LOG_BUFFER_HEX(TAG, data, len);
            
            elrs_parse_crsf_frame(data, len);
        }
    }
    break;
```

Rebuild and flash. Should show hex dump of all UART data.

#### Step 4: Test with Manual TX
Send test MSP v2 frame (v1.7.0+ format):
```c
// Add to system_init() after elrs_backpack_init()
vTaskDelay(pdMS_TO_TICKS(2000));

// MSP v2 SET_VTX_CONFIG (0x0059) for channel B4 (index 11)
uint8_t test_frame[] = {
    0x24, 0x58,  // "$X" - MSP v2 header
    0x3C,        // '<' - request
    0x00,        // flags
    0x59, 0x00,  // function = 0x0059 (MSP_SET_VTX_CONFIG)
    0x04, 0x00,  // payload size = 4 bytes
    0x0B,        // byte[0]: channel_idx = 11 (B4)
    0x00,        // byte[1]: freq_msb
    0x03,        // byte[2]: power_idx
    0x00,        // byte[3]: pit_mode
    0xXX         // CRC8-DVB-S2 (calculate properly)
};
// Note: Calculate CRC properly or use existing MSP library function
uart_write_bytes(ELRS_UART_NUM, test_frame, sizeof(test_frame));
```

**Important:** This is ESP-NOW transport in v1.7.0, not UART! The above is for testing UART fallback mode only.

---

## Problem: Wrong Channels Selected

### Symptoms
- Backpack receives commands
- Wrong frequency displayed on RX5808

### Solution 1: Check Band Mapping

ExpressLRS TX bands → RX5808 bands:
```
ExpressLRS   │ RX5808   │ Example Freq
─────────────┼──────────┼──────────────
Band A = 0   │ A        │ 5865
Band B = 1   │ B        │ 5733
Band E = 2   │ E        │ 5705
Band F = 3   │ F        │ 5740
Band R = 4   │ R        │ 5658
Band L = 5   │ L        │ 5362
```

### Solution 2: Use Frequency Mode

Instead of band/channel, use direct frequency:
```c
// In elrs_process_msp_command(), prioritize frequency:
if (new_freq > 5000 && new_freq < 6000) {
    RX5808_Set_Freq(new_freq);
    // Don't use band/channel
}
```

### Solution 3: Add Debug Logging
In `elrs_process_msp_command()`:
```c
ESP_LOGI(TAG, "Received: Band=%d, Ch=%d, Freq=%d", 
         new_band, new_channel, new_freq);
ESP_LOGI(TAG, "Expected RX5808 freq: %d", 
         Rx5808_Freq[new_band][new_channel]);
```

---

## Problem: CRC Errors

### Symptoms
```
W (1234) ELRS_BACKPACK: CRC mismatch: calc=0xAB, recv=0xCD
```

### Causes
1. Wrong baud rate
2. Electrical noise on UART lines
3. Voltage mismatch (5V ↔ 3.3V)
4. Bad connection

### Solutions

#### Solution 1: Lower Baud Rate
```c
#define ELRS_UART_BAUD_RATE    115200  // More reliable
```

#### Solution 2: Add Pull-up Resistors
```
ESP32 RX (GPIO16) ─┬── [4.7kΩ] ── 3.3V
                   │
                   └── Backpack TX
```

#### Solution 3: Shorten Wires
- Use <10cm wires for UART
- Twist TX/RX wires together
- Keep away from power lines

#### Solution 4: Add Capacitor
```
Backpack VCC ─┬── [100nF] ── GND
              │
              └── ESP32 3.3V
```

---

## Problem: Backpack Not Responding

### Symptoms
- ESP32 sends CRSF frames
- No response from backpack
- No channel changes on TX screen

### Check 1: Backpack Mode
ExpressLRS backpack must be in **VRX Backpack** mode, not TX mode.

Flash backpack with correct firmware:
```bash
# ExpressLRS Configurator:
# Device: [Your Backpack]
# Firmware: [Version]
# Options: VRX_BACKPACK = ON
```

### Check 2: Backpack Binding
Backpack must be bound to ExpressLRS TX:
1. Put TX in binding mode
2. Power on backpack with button held
3. Wait for bind confirmation

### Check 3: Backpack Address
CRSF address for VTX/VRX: `0x03`

Check in [elrs_backpack.h](main/driver/elrs_backpack.h):
```c
#define CRSF_ADDRESS_VTX    0x03  // Correct
```

---

## Problem: System Hangs/Crashes

### Symptoms
- ESP32 boots, then crashes
- Watchdog timer resets
- Guru meditation errors

### Solution 1: Increase Stack Size
Edit [elrs_backpack.c](main/driver/elrs_backpack.c):
```c
xTaskCreate(elrs_backpack_task, 
            "elrs_backpack", 
            8192,              // Increase from 4096
            NULL, 
            10, 
            NULL);
```

### Solution 2: Lower Task Priority
```c
xTaskCreate(elrs_backpack_task, 
            "elrs_backpack", 
            4096, 
            NULL, 
            5,                 // Decrease from 10
            NULL);
```

### Solution 3: Check for Buffer Overflows
In monitor, look for:
```
W (1234) ELRS_BACKPACK: UART FIFO overflow
W (1234) ELRS_BACKPACK: UART ring buffer full
```

If present, increase buffer size:
```c
#define ELRS_RX_BUFFER_SIZE    512  // Increase from 256
```

---

## Problem: GPIO Pin Conflicts

### Symptoms
```
E (1234) gpio: gpio_set_direction(123): GPIO number error
```

### Solution: Choose Available Pins

Check your hardware schematic. Avoid:
- GPIO 6-11 (Flash)
- GPIO 0 (Boot mode)
- Pins used by LCD, SPI, I2C, etc.

Safe pins for ESP32:
- **RX**: GPIO 16, 22, 26, 32
- **TX**: GPIO 17, 23, 27, 33

Update in [elrs_backpack.h](main/driver/elrs_backpack.h):
```c
#define ELRS_UART_TX_PIN    GPIO_NUM_23  // Change here
#define ELRS_UART_RX_PIN    GPIO_NUM_22  // Change here
```

---

## Problem: Intermittent Connection

### Symptoms
- Sometimes works, sometimes doesn't
- CRC errors intermittent
- Lost packets

### Solution 1: Check Power Supply
Measure voltage under load:
```
Multimeter → VCC pin → Should be 3.25-3.35V

If drops to <3.0V:
- Add larger capacitor (470µF)
- Use separate power supply
- Check wire gauge (use thicker wires)
```

### Solution 2: Check Antenna
- Ensure backpack antenna is not blocked
- Position away from metal
- Check antenna connection (not broken)

### Solution 3: Reduce Range
Move TX closer to backpack for testing.

### Solution 4: Check RF Interference
- Move away from WiFi routers
- Disable WiFi on ESP32 if enabled
- Test in different location

---

## Debug Checklist

Run through this checklist:

- [ ] Backpack powered on (LED blinking)
- [ ] Backpack in VRX mode (not TX mode)
- [ ] Backpack bound to ExpressLRS TX
- [ ] Correct wiring (TX→RX crossover)
- [ ] 3.3V voltage (not 5V)
- [ ] GPIO pins don't conflict with other peripherals
- [ ] Baud rate = 420000 (or 115200 for testing)
- [ ] UART driver enabled in ESP-IDF
- [ ] Firmware compiled without errors
- [ ] Monitor shows initialization message
- [ ] ExpressLRS TX has VRX control enabled
- [ ] Correct frequency band table in TX

---

## Advanced Debugging

### UART Loopback Test

Temporarily short TX to RX on ESP32:
```c
// Should receive own transmissions
uart_write_bytes(ELRS_UART_NUM, "TEST", 4);
// Should appear in elrs_backpack_task()
```

### Logic Analyzer

Use logic analyzer to capture UART:
- Sample rate: 4.2 MHz (10x baud rate)
- Decode: UART, 420000 baud, 8N1
- Look for: SYNC byte (0xC8), valid frames

### Oscilloscope

Check signal integrity:
- Signal should be 0V (low) or 3.3V (high)
- Clean transitions (not rounded)
- No ringing or overshoot

---

## Getting Help

### Information to Provide

When asking for help, include:

1. **Monitor output** (full boot log)
2. **Hardware**: ESP32 version, backpack model
3. **Wiring**: Photo or detailed description
4. **Firmware**: ESP-IDF version, build output
5. **Configuration**: sdkconfig settings
6. **ExpressLRS**: TX firmware version, settings screenshot

### Useful Commands

```bash
# Full build log
idf.py build > build.log 2>&1

# Monitor with timestamps
idf.py -p COM3 monitor --print_filter "*:V"

# Check GPIO config
idf.py menuconfig
# Component config → ESP32-specific → GPIO

# UART config
idf.py menuconfig
# Component config → Driver configurations → UART
```

---

## Still Not Working?

If you've tried everything:

1. **Test basic RX5808 functionality** (without backpack)
   - Can you change channels manually?
   - Does RSSI work?

2. **Test UART with loopback** (TX→RX shorted)
   - Can ESP32 receive its own transmissions?

3. **Test different UART pins**
4. **Test with different backpack** (if available)
5. **Test with USB-UART adapter** (receive on PC)

Open an issue with complete debug information!

---

**Most common fix: Double-check TX→RX, RX→TX crossover wiring! 🔧**
