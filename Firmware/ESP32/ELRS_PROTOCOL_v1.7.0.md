# ExpressLRS Backpack Protocol Documentation (v1.7.0)

## Overview

This document details the **corrected MSP v2 protocol implementation** for ExpressLRS Backpack wireless VTX control in RX5808-Div firmware v1.7.0 and later.

**Status:** ✅ **VERIFIED WORKING** - Hardware tested with EdgeTX VTX Admin (B4, R7, L3 channels confirmed)

---

## Protocol Correction Summary

### What Was Wrong (v1.6.0 and earlier)

❌ **Incorrect Implementation:**
- Used `MSP_ELRS_VTX_CONFIG (0x0011)` as channel command packets
- Actually `0x0011` = telemetry wrapper containing GPS/battery/linkstats
- Extracted channel from `byte[2]` instead of `byte[0]`
- Resulted in erratic switching from misinterpreted telemetry data

### What's Fixed (v1.7.0)

✅ **Correct Implementation:**
- Uses `MSP_SET_VTX_CONFIG (0x0059)` for actual VTX channel commands
- Properly ignores `MSP_ELRS_BACKPACK_CRSF_TLM (0x0011)` telemetry packets
- Extracts channel from `byte[0]` as per official specification
- Reliable channel switching verified with hardware testing

---

## MSP Command Reference

### MSP_ELRS_BACKPACK_CRSF_TLM (0x0011)

**Purpose:** Telemetry wrapper containing CRSF telemetry data

**Payload Structure:**
```
Variable 6-14 bytes:
[GPS lat/lon, battery voltage, RSSI, LQ, SNR, antenna, etc.]
```

**Transmission Pattern:**
- Broadcast by TX continuously (~5 second intervals)
- NOT triggered by user action
- Contains flight telemetry, NOT VTX commands

**Action:** **IGNORE** - Do not process as channel commands

**Code Implementation:**
```c
case MSP_ELRS_BACKPACK_CRSF_TLM:
    ESP_LOGD(TAG, "Ignoring MSP_ELRS_BACKPACK_CRSF_TLM (0x0011) - telemetry wrapper");
    return; // Do not process
```

---

### MSP_SET_VTX_CONFIG (0x0059)

**Purpose:** Actual VTX channel configuration commands from VTX Admin

**Payload Structure:**
```
Fixed 4 bytes:
byte[0] = channel_idx  (0-47: 6 bands × 8 channels)
byte[1] = freq_msb     (Frequency MSB for verification)
byte[2] = power_idx    (Power level index)
byte[3] = pit_mode     (Pit mode on/off)
```

**Transmission Pattern:**
- Sent by TX on user request via VTX Admin menu
- Triggered when user changes channel/band in radio
- Arrives as immediate response to user action

**Action:** **PROCESS** - This is the actual VTX control command

**Code Implementation:**
```c
case MSP_SET_VTX_CONFIG:
    if (size == 4) {
        uint8_t channel_idx = payload[0]; // ← CORRECT: byte[0]
        ESP_LOGI(TAG, "==== MSP_SET_VTX_CONFIG (0x0059) ====");
        ESP_LOGI(TAG, "Channel from byte[0] = 0x%02X (%d decimal)", 
                 channel_idx, channel_idx);
        
        // Process channel change
        handle_msp_set_vtx_config(payload, size);
    }
    break;
```

---

## Channel Index Mapping

### Band Layout (0-47)

| Band | Index Range | Channels | Example |
|------|-------------|----------|---------|
| **A** | 0-7 | 5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725 | A1 = 0 |
| **B** | 8-15 | 5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866 | B4 = 11 |
| **E** | 16-23 | 5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945 | E2 = 17 |
| **F** | 24-31 | 5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880 | F7 = 30 |
| **R** | 32-39 | 5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917 | R7 = 38 |
| **L** | 40-47 | 5362, 5399, 5436, 5473, 5510, 5547, 5584, 5621 | L3 = 42 |

### Calculation Formula

```c
// Band and channel to index
uint8_t band_idx = 0;  // A=0, B=1, E=2, F=3, R=4, L=5
uint8_t channel = 4;   // 1-8 (subtract 1 for 0-based)
uint8_t index = (band_idx * 8) + (channel - 1);

// Example: B4 → (1 * 8) + (4 - 1) = 11

// Index to band and channel
uint8_t band_idx = index / 8;
uint8_t channel = (index % 8) + 1;

// Example: 38 → band = 38/8 = 4 (R), channel = 38%8 + 1 = 7 (R7)
```

---

## MSP v2 Packet Format

### Structure

```
MSP v2 over ESP-NOW:
$X<type><flags><func_l><func_h><size_l><size_h><payload><crc8>

Bytes:
[0-1]  : "$X" - Header (0x24, 0x58)
[2]    : Type ('<' = 0x3C request, '>' = 0x3E response)
[3]    : Flags (typically 0x00)
[4-5]  : Function code (little-endian, e.g., 0x59 0x00 = 0x0059)
[6-7]  : Payload size (little-endian)
[8-n]  : Payload data
[n+1]  : CRC8-DVB-S2 checksum
```

### CRC8-DVB-S2

**Polynomial:** `0xD5`

**Calculation:**
```c
uint8_t crc8_dvb_s2(uint8_t crc, uint8_t a) {
    crc ^= a;
    for (int i = 0; i < 8; i++) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ 0xD5;
        } else {
            crc = crc << 1;
        }
    }
    return crc;
}

// Usage: CRC over type, flags, function, size, payload
uint8_t crc = 0;
for (int i = 2; i < packet_len - 1; i++) {
    crc = crc8_dvb_s2(crc, packet[i]);
}
```

---

## ESP-NOW Transport

### Configuration

**WiFi Channel:** Channel 1 (2412 MHz)

**Security:** Open (no encryption for broadcast compatibility)

**MAC Addressing:**
- TX broadcasts to all receivers
- Binding via UID → MAC address sanitization
- Clear bit 0 (multicast) for proper peer MAC

**Example:**
```c
// UID from binding: 51:45:C7:E5:F9:2A
// Sanitized MAC:    50:45:C7:E5:F9:2A (bit 0 cleared)
```

### Initialization

```c
// 1. Initialize WiFi in STA mode
esp_wifi_set_mode(WIFI_MODE_STA);

// 2. Set WiFi channel to 1
esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

// 3. Initialize ESP-NOW
esp_now_init();

// 4. Register receive callback
esp_now_register_recv_cb(on_esp_now_recv);

// 5. Add peer (unencrypted broadcast)
esp_now_peer_info_t peer = {
    .channel = 1,
    .encrypt = false,
};
memcpy(peer.peer_addr, tx_mac, 6);
esp_now_add_peer(&peer);
```

---

## Hardware Testing Results

### Test Setup

- **TX Module:** ExpressLRS 1.5.4 firmware
- **Radio:** EdgeTX with VTX Admin enabled
- **RX:** RX5808-Div v1.7.0 with ELRS Backpack enabled
- **Binding:** MAC 50:45:C7:E5:F9:2A

### Test Channels

| Test | Channel | Index | Payload | Result |
|------|---------|-------|---------|--------|
| 1 | **B4** | 11 | `0B 00 03 00` | ✅ SUCCESS |
| 2 | **R7** | 38 | `26 00 03 00` | ✅ SUCCESS |
| 3 | **L3** | 42 | `2A 00 03 00` | ✅ SUCCESS |

### Serial Monitor Output

```
I (34578) ELRS_BP: ==== MSP_SET_VTX_CONFIG (0x0059) ====
I (34578) ELRS_BP: Payload: 0B 00 03 00
I (34578) ELRS_BP: Channel from byte[0] = 0x0B (11 decimal)
I (34588) ELRS_BP: >>> CHANNEL CHANGED: B4 (index 11) <<<

I (45123) ELRS_BP: ==== MSP_SET_VTX_CONFIG (0x0059) ====
I (45123) ELRS_BP: Payload: 26 00 03 00
I (45123) ELRS_BP: Channel from byte[0] = 0x26 (38 decimal)
I (45133) ELRS_BP: >>> CHANNEL CHANGED: R7 (index 38) <<<

I (56789) ELRS_BP: ==== MSP_SET_VTX_CONFIG (0x0059) ====
I (56789) ELRS_BP: Payload: 2A 00 03 00
I (56789) ELRS_BP: Channel from byte[0] = 0x2A (42 decimal)
I (56799) ELRS_BP: >>> CHANNEL CHANGED: L3 (index 42) <<<
```

**Verification:** All channel changes executed correctly, no errors, telemetry packets properly ignored.

---

## Code References

### Files Modified

1. **`elrs_msp.h`** (Commit `15f72b9`)
   - Renamed `MSP_ELRS_VTX_CONFIG` → `MSP_ELRS_BACKPACK_CRSF_TLM`
   - Documented command purposes and payload structures
   - Added developer notes about byte offsets

2. **`elrs_backpack.c`** (Commit `5643d35`)
   - Fixed `handle_msp_set_vtx_config()` to read from `byte[0]`
   - Updated `process_msp_packet()` to ignore `0x0011`, process `0x0059`
   - Enhanced logging with payload hex dumps and channel change confirmations

### Key Functions

**MSP Packet Processing:**
```c
void process_msp_packet(uint8_t *data, size_t len) {
    uint16_t function = (data[5] << 8) | data[4];
    uint16_t size = (data[7] << 8) | data[6];
    uint8_t *payload = &data[8];
    
    switch (function) {
        case MSP_ELRS_BACKPACK_CRSF_TLM:
            // Ignore telemetry wrapper
            return;
            
        case MSP_SET_VTX_CONFIG:
            if (size == 4) {
                handle_msp_set_vtx_config(payload, size);
            }
            break;
    }
}
```

**Channel Extraction:**
```c
void handle_msp_set_vtx_config(uint8_t *payload, size_t size) {
    if (size != 4) return;
    
    uint8_t channel_idx = payload[0]; // ← CRITICAL: byte[0] not byte[2]
    
    // Calculate band and channel
    uint8_t band_idx = channel_idx / 8;
    uint8_t channel = (channel_idx % 8) + 1;
    
    // Map to RX5808 frequency
    uint16_t frequency = get_frequency_for_channel(band_idx, channel);
    
    // Tune receiver
    rx5808_set_frequency(frequency);
    
    ESP_LOGI(TAG, ">>> CHANNEL CHANGED: %c%d (index %d) <<<",
             band_names[band_idx], channel, channel_idx);
}
```

---

## Migration Guide

### From v1.6.0 to v1.7.0

**For Users:**
1. Update firmware: `idf.py build flash`
2. No configuration changes required
3. ELRS Backpack toggle continues working in Setup menu
4. Existing UID binding preserved

**For Developers:**
1. Review official ExpressLRS Backpack repository for protocol specs
2. **Always verify protocol assumptions** against official documentation
3. Use `0x0059` for VTX commands, ignore `0x0011` telemetry
4. Extract channel from `byte[0]`, not `byte[2]`
5. Implement proper CRC8-DVB-S2 validation
6. Test with real hardware, not just simulated data

### Common Mistakes to Avoid

❌ **Don't:**
- Use `0x0011` for channel commands (it's telemetry!)
- Extract channel from `byte[2]` (it's power index!)
- Assume UART transport (it's ESP-NOW!)
- Skip CRC validation (corruption causes issues!)

✅ **Do:**
- Use `0x0059` for channel commands
- Extract channel from `byte[0]`
- Implement ESP-NOW on WiFi channel 1
- Validate CRC8-DVB-S2 on all packets
- Filter and ignore telemetry packets
- Log packet contents during development

---

## Debugging Tips

### Enable Detailed Logging

```c
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

// In process_msp_packet():
ESP_LOGI(TAG, "MSP Function: 0x%04X, Size: %d", function, size);
ESP_LOG_BUFFER_HEX(TAG, payload, size);
```

### Monitor ESP-NOW Reception

```c
void on_esp_now_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    ESP_LOGI(TAG, "ESP-NOW from MAC: %02X:%02X:%02X:%02X:%02X:%02X, len=%d",
             info->src_addr[0], info->src_addr[1], info->src_addr[2],
             info->src_addr[3], info->src_addr[4], info->src_addr[5], len);
    ESP_LOG_BUFFER_HEX(TAG, data, len);
}
```

### Verify Channel Mapping

```c
// Test all 48 channels
for (uint8_t i = 0; i < 48; i++) {
    uint8_t band = i / 8;
    uint8_t chan = (i % 8) + 1;
    uint16_t freq = get_frequency_for_channel(band, chan);
    ESP_LOGI(TAG, "Index %2d → %c%d = %d MHz", i, band_names[band], chan, freq);
}
```

---

## References

### Official Documentation

- **ExpressLRS Backpack:** https://github.com/ExpressLRS/Backpack
- **MSP v2 Protocol:** https://github.com/iNavFlight/inav/wiki/MSP-V2
- **ESP-NOW:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html
- **EdgeTX VTX Admin:** https://edgetx.org

### Related Files

- [CHANGELOG.md](CHANGELOG.md) - Full version history with v1.7.0 details
- [COMPLETE_SETUP_GUIDE.md](COMPLETE_SETUP_GUIDE.md) - End-to-end setup instructions
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - Common issues and solutions
- [README.md](../../README.md) - Main project documentation

### Commit History

- **Commit `15f72b9`**: Protocol definition corrections
- **Commit `5643d35`**: Proper VTX channel command handling
- **Testing Logs**: Hardware verification with B4, R7, L3 channels

---

## FAQ

**Q: Why ESP-NOW instead of UART?**  
A: ESP-NOW provides wireless peer-to-peer communication without requiring physical wiring between TX backpack and VRX. More flexible for mounting configurations.

**Q: Why ignore 0x0011 packets?**  
A: They contain telemetry data (GPS, battery, etc.), not VTX commands. Processing them as channel commands causes erratic switching.

**Q: Can I use band E?**  
A: Partial support. Channels E1, E2, E3, E5, E6 work. Channels E4, E7, E8 may be restricted depending on region.

**Q: Do I need to rebind after updating to v1.7.0?**  
A: No, existing UID binding is preserved. Just flash new firmware.

**Q: What if I see 0x58 or 0x88 in logs?**  
A: Update to v1.7.0. Those are incorrect command codes from earlier versions.

**Q: How do I verify it's working?**  
A: Enable serial monitor, change channels via VTX Admin on radio, look for `>>> CHANNEL CHANGED:` messages with correct band/channel.

---

**Document Version:** 1.0  
**Firmware Version:** v1.7.0  
**Last Updated:** January 2025  
**Status:** ✅ Production Ready
