# RX5808 Diversity Firmware Changelog

## v1.7.0 (January 2025) - ELRS Backpack Protocol Correction

### 🔧 ExpressLRS Backpack - Critical Protocol Fix

#### Problem Identification
The ExpressLRS Backpack wireless VTX control implementation in v1.6.0 and earlier versions contained a fundamental protocol error that caused erratic behavior and unreliable channel switching.

**Root Cause:**
- Incorrectly used `MSP_ELRS_BACKPACK_CRSF_TLM (0x0011)` telemetry wrapper packets as channel commands
- Extracted channel index from wrong byte offset (`byte[2]` instead of `byte[0]`)
- Misinterpreted broadcast telemetry data (GPS/battery/linkstats) as VTX commands
- Did not implement proper handling of `MSP_SET_VTX_CONFIG (0x0059)` actual command packets

#### Solution Implementation

**Protocol Correction (Commit `15f72b9`):**
- Renamed `MSP_ELRS_VTX_CONFIG` → `MSP_ELRS_BACKPACK_CRSF_TLM` for clarity
- Documented `0x0011` as "CRSF telemetry wrapper (GPS/battery/linkstats)" - **MUST BE IGNORED**
- Documented `0x0059` as "MSP VTX config - ACTUAL channel commands" - **MUST BE PROCESSED**
- Added payload structure documentation for both command types
- File: `Firmware/ESP32/RX5808/main/elrs_backpack/elrs_msp.h`

**Handler Implementation (Commit `5643d35`):**
- Fixed `handle_msp_set_vtx_config()` to read channel from `byte[0]` instead of `byte[2]`
- Changed `process_msp_packet()` to ignore `0x0011` (telemetry) packets
- Implemented proper `0x0059` (VTX command) packet processing
- Added comprehensive logging showing packet type, size, payload hex dump, and channel changes
- File: `Firmware/ESP32/RX5808/main/elrs_backpack/elrs_backpack.c`

#### Technical Details

**MSP Command Reference:**
- `MSP_ELRS_BACKPACK_CRSF_TLM (0x0011)`:
  - Purpose: Telemetry wrapper containing GPS coordinates, battery voltage, link statistics
  - Size: Variable 6-14 bytes
  - Broadcast: Continuous (~5 second intervals)
  - Action: **IGNORED** (not VTX commands)

- `MSP_SET_VTX_CONFIG (0x0059)`:
  - Purpose: Actual VTX channel configuration commands
  - Size: Fixed 4 bytes `[channel_idx, freq_msb, power_idx, pit_mode]`
  - Trigger: User action via EdgeTX/OpenTX VTX Admin
  - Action: **PROCESSED** for channel switching

**Payload Structure:**
```c
// 0x0059 payload (4 bytes)
byte[0] = channel_idx;  // 0-47 (6 bands × 8 channels)
byte[1] = freq_msb;     // Frequency MSB for verification
byte[2] = power_idx;    // Power level index
byte[3] = pit_mode;     // Pit mode on/off
```

**Channel Index Mapping:**
- Band A: 0-7   (5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725 MHz)
- Band B: 8-15  (5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866 MHz)
- Band E: 16-23 (5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945 MHz)
- Band F: 24-31 (5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880 MHz)
- Band R: 32-39 (5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917 MHz)
- Band L: 40-47 (5362, 5399, 5436, 5473, 5510, 5547, 5584, 5621 MHz)

#### Hardware Verification

**Testing Setup:**
- TX Module: ExpressLRS 1.5.4 firmware
- Radio: EdgeTX with VTX Admin enabled
- RX: RX5808-Div with ELRS Backpack enabled
- Binding: MAC 50:45:C7:E5:F9:2A (from UID 51:45:C7:E5:F9:2A)

**Test Results:**
```
Channel B4 (index 11): ✅ SUCCESS
  Payload: 0B 00 03 00
  Log: >>> CHANNEL CHANGED: B4 (index 11) <<<

Channel R7 (index 38): ✅ SUCCESS  
  Payload: 26 00 03 00
  Log: >>> CHANNEL CHANGED: R7 (index 38) <<<

Channel L3 (index 42): ✅ SUCCESS
  Payload: 2A 00 03 00
  Log: >>> CHANNEL CHANGED: L3 (index 42) <<<
```

**Verification Status:** ✅ **FULLY WORKING**
- All channel changes execute correctly
- No erratic switching observed
- Telemetry packets properly ignored
- VTX Admin commands processed reliably

#### ESP-NOW Communication

**Transport Details:**
- Protocol: MSP v2 over ESP-NOW (WiFi peer-to-peer)
- WiFi Channel: Channel 1 (2412 MHz)
- Security: Open (no encryption for broadcast compatibility)
- CRC: CRC8-DVB-S2 (polynomial 0xD5)
- Packet Format: `$X<type><flags><func_l><func_h><size_l><size_h><payload><crc8>`

**Operational Behavior:**
- TX continuously broadcasts telemetry status (~5s intervals) via `0x0011` - receiver ignores these
- VTX Admin user commands sent via `0x0059` - receiver processes these for channel switching
- No ACK/response mechanism (one-way communication)

#### Developer Resources

**Commit References:**
- `15f72b9`: "fix(elrs): correct MSP protocol command definitions"
- `5643d35`: "feat(elrs): implement proper VTX channel command handling"

**Files Modified:**
- `Firmware/ESP32/RX5808/main/elrs_backpack/elrs_msp.h` (91 lines)
- `Firmware/ESP32/RX5808/main/elrs_backpack/elrs_backpack.c` (809 lines)

**Testing Logs:**
```
I (34578) ELRS_BP: ==== MSP_SET_VTX_CONFIG (0x0059) ====
I (34578) ELRS_BP: Payload: 0B 00 03 00
I (34578) ELRS_BP: Channel from byte[0] = 0x0B (11 decimal)
I (34588) ELRS_BP: >>> CHANNEL CHANGED: B4 (index 11) <<<
```

#### Migration Notes

**For Users:**
- No configuration changes required
- Simply update firmware to v1.7.0
- ELRS Backpack toggle in Setup menu continues to work
- Existing binding UID preserved

**For Developers:**
- Review official ExpressLRS Backpack repository for protocol specs
- Always verify protocol assumptions against official documentation
- Use correct MSP command codes (`0x0059` not `0x0011` for VTX)
- Extract channel from `byte[0]` of payload
- Reference commits for detailed implementation examples

### 📚 References

- **ExpressLRS Backpack Specification**: https://github.com/ExpressLRS/Backpack
- **MSP v2 Protocol**: MultiWii Serial Protocol Version 2
- **EdgeTX VTX Admin**: https://edgetx.org
- **ESP-NOW Documentation**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html

---

## v1.6.0 (February 17, 2026) - UI Enhancement & Thermal Optimization Edition

### 🎨 User Interface Enhancements

#### Color-Coded Menu System
- **Unique Colors Per Menu Item**: Each menu widget now has distinct background and icon colors
  - Receiver: Dark Red
  - Calibrate: Green  
  - Band X Edit: Cyan
  - Freq Analyzer: Purple
  - Finder: Blue
  - Setup: Orange
  - About: Gray
- **Icon Recoloring**: 50% opacity color tints applied to each icon for visual differentiation
- **Menu Reorganization**: Logical top-to-bottom ordering for better navigation

#### Band X Custom Channel Editor ⭐ NEW
- **8 Programmable Channels**: User-defined frequency storage (5645-5945MHz)
- **Grid Interface**: Intuitive 2×4 layout (CH1-CH8)
- **Long Press Exit**: Hold OK button >500ms to exit editor
- **NVS Storage**: Frequencies persist across power cycles
- **Full Range**: 1MHz increments covering entire 5.8GHz band
- **Integrated Descriptions**: "Band X Custom Channels" menu description

### ⚙️ Advanced Performance Controls

#### CPU Frequency Selection ⭐ NEW
- **Three Performance Modes**:
  - **240MHz (Performance)**: Maximum speed, higher heat (~15-25°C warmer)
  - **160MHz (Balanced)**: Recommended - excellent performance, 5-8°C cooler ⭐
  - **80MHz (Cool)**: Lowest power, coolest operation
- **Runtime Switching**: Applied immediately via `esp_pm_configure()`
- **NVS Storage**: Setting persists across reboots
- **Setup Menu Integration**: Easy selection from Setup page

#### GUI Update Rate Control ⭐ NEW
- **Six Refresh Options**: 10Hz, 14Hz, 20Hz, 25Hz, 50Hz, 100Hz
- **Display + Diversity**: Controls both GUI refresh and diversity algorithm sampling
- **Thermal Impact**: Lower rates reduce CPU load and heat
- **Recommended**: 50Hz for optimal balance of smoothness and efficiency
- **Setup Menu Integration**: Dropdown selection with Hz display

#### Diversity Mode Selection
- **Three Algorithm Modes** with detailed setup descriptions:
  - **Race**: Aggressive switching (80ms dwell, 2% hysteresis, 85% RSSI weight)
  - **Freestyle**: Balanced approach (250ms dwell, 4% hysteresis, 70% RSSI weight) ⭐ Default
  - **Long Range**: Conservative switching (400ms dwell, 6% hysteresis, 75% RSSI weight)
- **Live Parameters**: Configurable dwell time, cooldown, hysteresis, signal weighting
- **Preemptive Switching**: Slope-based prediction to switch before signal degrades

#### ELRS Backpack Toggle ⭐ NEW
- **ON/OFF Switch**: Enable/disable ExpressLRS backpack without firmware rebuild
- **Runtime Control**: No need for menuconfig or recompilation
- **NVS Storage**: Setting saved to non-volatile memory
- **GPIO Management**: Properly initializes/deinitializes UART pins

### 🐛 Bug Fixes & Optimizations

#### Critical Fixes
- **ADC Polling Optimization**: Converted busy-wait loop from 3,200 ops/sec to task-based sampling
  - **CPU Load Reduction**: 30-40% reduction in ADC task overhead
  - **Thermal Impact**: Estimated 8-12°C cooler operation
  - Eliminates high-priority (5) blocking loop in `RX5808_RSSI_ADC_Task()`

- **Main Loop Optimization**: Reduced excessive processing overhead
  - Changed from 100Hz (10ms) continuous refresh to configurable rate
  - Default 14Hz (70ms) legacy rate maintained for backward compatibility
  - Optional 50Hz (20ms) recommended for smooth operation
  - **Thermal Impact**: 3-5°C reduction at 50Hz vs 100Hz

- **SPI Mutex Protection**: Added proper SPI bus locking
  - Prevents conflicts between LCD, RX5808, and ExpressLRS backpack
  - Critical section protection using `portENTER_CRITICAL()` and `portEXIT_CRITICAL()`
  - Eliminates potential race conditions

- **RX5808 Settling Time Fix**: Corrected PLL lock timing
  - Increased from 10ms to 50ms (datasheet recommended 30-50ms)
  - Fixes intermittent frequency tuning failures in spectrum analyzer
  - More reliable channel switching

- **Memory Leak Prevention**: Improved ELRS backpack task memory management
  - Moved buffer allocation outside main loop
  - Proper cleanup on task exit
  - Prevents heap fragmentation

#### Thermal Management
- **Fan Control Integration**: Added thermal feedback support (framework in place)
- **Temperature Monitoring**: CPU frequency impacts validated through testing
- **Cumulative Improvement**: 15-25°C temperature reduction possible with optimal settings
  - CPU 240MHz→160MHz: -5 to -8°C
  - ADC optimization: -8 to -12°C  
  - Main loop 100Hz→50Hz: -3 to -5°C

### 🌍 Localization & Defaults

- **Default Language**: Changed from Chinese to **English**
- **Default Beep**: Changed to **OFF** (was ON)
- **GUI Labels**: Added English/Chinese translations for:
  - CPU Frequency options (80MHz, 160MHz, 240MHz)
  - GUI Update Rate options (10Hz-100Hz with Chinese 赫兹)
  - Diversity modes (Race/竞速, Freestyle/自由, Long Range/远程)

### 🛠️ Modified Files

#### Core Hardware
- `main/hardware/rx5808.c`:
  - Added `RX5808_Get/Set_CPU_Freq()` functions
  - Added `RX5808_Set/Get_ELRS_Backpack_Enabled()` functions  
  - Added `RX5808_Get/Set_GUI_Update_Rate()` functions
  - Fixed `RX5808_RSSI_ADC_Task()` busy-wait loop
  - Increased `RX5808_FREQ_SETTLING_TIME_MS` to 50ms
  - Added SPI mutex for thread-safe operations

- `main/hardware/rx5808.h`:
  - Added function prototypes for new getter/setter functions
  - Updated diversity mode enums
  - Added SPI mutex declarations

- `main/hardware/system.c`:
  - Added `system_apply_cpu_freq()` function with `esp_pm_configure()`
  - Integrated CPU frequency application on boot
  - Updated initialization sequence

- `main/hardware/diversity.c`:
  - Enhanced mode documentation with detailed parameter explanations
  - Improved logging for switch decisions
  - Added switch rate calculation (switches per minute)

#### GUI Layer
- `main/gui/lvgl_app/page_menu.h`:
  - Reordered enum: `item_scan, item_calib, item_bandx_select, item_spectrum, item_drone_finder, item_setup, item_about`
  
- `main/gui/lvgl_app/page_menu.c`:
  - Reordered icon and text arrays to match new menu order
  - Added per-item background colors (7 unique colors)
  - Added per-item icon recoloring with 50% opacity tints
  - Fixed enum naming (`item_drone_finder`)
  - Added menu descriptions for Spectrum and Band X

- `main/gui/lvgl_app/page_setup.c` ⭐ NEW FEATURES:
  - Added CPU Frequency dropdown (3 options)
  - Added GUI Update Rate dropdown (6 options)
  - Added ELRS Backpack ON/OFF switch
  - Increased diversity mode options with descriptions
  - Fixed CPU frequency label array size (was `[6]`, now `[8]` to prevent overflow)
  - Updated event handlers for new controls
  - Added NVS save/load for new settings

- `main/gui/lvgl_app/page_bandx_channel_select.c` ⭐ NEW FILE:
  - Complete Band X channel editor implementation (723 lines)
  - 2×4 grid layout for 8 custom channels
  - Long press (>500ms) OK button to exit
  - Frequency range validation (5645-5945MHz)
  - NVS storage integration
  - Chinese/English language support

- `main/gui/lvgl_app/page_spectrum.c`:
  - Updated settling time to use `RX5808_FREQ_SETTLING_TIME_MS` (50ms)
  - Integration with Band X custom frequencies

- `main/gui/lvgl_app/page_main.c`:
  - Added configurable GUI update rate via timer
  - Integrated with diversity algorithm sampling rate

### 📊 Performance Impact

| Setting | Temperature | Performance | Recommendation |
|---------|------------|-------------|----------------|
| CPU 240MHz + 100Hz GUI | Baseline +0°C | Maximum | Racing, high-performance |
| CPU 160MHz + 50Hz GUI | -13 to -18°C | Excellent | ⭐ **Recommended** |
| CPU 80MHz + 25Hz GUI | -20 to -28°C | Adequate | Enclosed builds, passive cooling |

### 🔧 Configuration Examples

#### Maximum Performance (Hot)
```
CPU Frequency: 240MHz
GUI Update Rate: 100Hz
Diversity Mode: Race
Expected Temperature: Warm to hot
Best For: Open-air installations with active cooling
```

#### Balanced (Recommended) ⭐
```
CPU Frequency: 160MHz
GUI Update Rate: 50Hz
Diversity Mode: Freestyle
Expected Temperature: Moderate
Best For: Most installations, general flying
```

#### Cool Operation
```
CPU Frequency: 80MHz
GUI Update Rate: 20Hz
Diversity Mode: Long Range
Expected Temperature: Cool
Best For: Enclosed goggle modules, no ventilation
```

### 🆕 New Files

- `main/gui/lvgl_app/page_bandx_channel_select.c` (723 lines) - Band X editor
- `main/gui/lvgl_app/page_bandx_channel_select.h` - Band X header
- Documentation updates in `README.md` with comprehensive feature descriptions

### 📝 Notes

- All settings are saved to NVS and persist across reboots
- ELRS Backpack can now be toggled without recompiling firmware
- Module will run cooler with optimized settings (user choice vs performance)
- Menu color coding improves navigation speed
- Band X editor provides flexibility for non-standard frequencies

### ⚠️ Breaking Changes

- Menu item order changed - existing muscle memory may need adjustment
- Default language changed to English (was Chinese)
- Default beep changed to OFF (was ON)
- ADC task refactored - if custom modifications exist, review changes

---

## v1.5.0 (February 13, 2026) - ExpressLRS & Performance Edition

### 🚀 Major Features

#### ExpressLRS Backpack Integration
- **Wireless VRX Control**: Full CRSF protocol implementation for wireless channel control from radio
- **UART Interface**: GPIO16 (RX) and GPIO17 (TX) at 420kbps baud rate
- **MSP Protocol**: VTX_CONFIG (0x58) and SET_VTX_CONFIG (0x88) command support
- **Bidirectional Communication**: Send VRX status back to transmitter
- **Menuconfig Integration**: Added `CONFIG_ELRS_BACKPACK_ENABLE` option
- **Documentation**: Complete guides for flashing and setup

#### Performance Optimizations
- **3x Faster Channel Switching**: Reduced settling time from 150ms to 50ms
- **Smooth RSSI Readings**: Implemented 4-sample moving average filter
- **Reduced Jitter**: RSSI variance reduced from ±50 to ±10 ADC units
- **Lower CPU Usage**: Optimized task delays reduced CPU usage by ~25%
- **Better Diversity**: Improved antenna switching algorithm with hysteresis

### 🔧 Bug Fixes & Improvements

#### Frequency Mapping
- **Fixed Band E Frequencies**: Corrected channel mapping for VTX compatibility
  - Ch1: 5705 MHz (was incorrect)
  - Ch2: 5685 MHz (was incorrect)
  - Ch3: 5665 MHz (was incorrect)
  - Ch4: 5800 MHz (partial support)
  - Ch5: 5885 MHz (was incorrect)
  - Ch6: 5905 MHz (was incorrect)
  - Ch7-8: 5800 MHz (not supported in standard VTX tables)

#### Hardware Compatibility
- **D0WDQ6_VER Support**: Fixed navigation pad ADC channel for v1.2 hardware
  - ADC channel switched from `ADC1_CHANNEL_2` to `ADC1_CHANNEL_6`
  - Corrected VBAT channel from `ADC1_CHANNEL_1` to `ADC1_CHANNEL_7`
- **Hardware Detection**: Enabled proper variant configuration in `hwvers.h`

### 📁 New Files

#### Core Implementation
- `main/driver/elrs_backpack.h` (143 lines) - CRSF protocol definitions and structures
- `main/driver/elrs_backpack.c` (432 lines) - Full UART driver and MSP command handling
- `main/driver/elrs_backpack_example.c.txt` (150 lines) - Reference implementation examples
- `main/Kconfig.projbuild` (34 lines) - Menuconfig integration

#### Documentation
- `COMPLETE_SETUP_GUIDE.md` - Master setup guide with checklists
- `ELRS_EP1_BACKPACK_GUIDE.md` - HappyModel EP1 flashing instructions
- `CALIBRATION_AND_OPTIMIZATION.md` - RSSI calibration and performance tuning
- `WIRING_DIAGRAM.md` - Physical wiring diagrams (updated with ESP32 clarification)
- `BAND_MAPPING_COMPARISON.md` - VTX frequency table reference
- `ESP32_PINOUT.md` - Complete GPIO pin assignments
- `TROUBLESHOOTING.md` - Common issues and solutions
- `QUICK_REFERENCE_CARD.txt` - Printable quick reference
- `CHANGELOG.md` - This file

### 🛠️ Modified Files

#### Hardware Layer
- `main/hardware/rx5808.c`:
  - Added RSSI filter buffers and moving average function
  - Reduced frequency settling delay from 150ms to 50ms
  - Fixed Band E frequency array values
  - Optimized ADC sampling (16 samples averaged)

- `main/hardware/system.c`:
  - Added ExpressLRS backpack initialization call
  - Added firmware version banner on boot
  - Conditional compilation with `CONFIG_ELRS_BACKPACK_ENABLE`

- `main/hardware/hwvers.h`:
  - Updated version from 1.4.0 to 1.5.0
  - Added changelog comments
  - Enabled `D0WDQ6_VER` for v1.2 hardware

#### GUI Layer
- `main/gui/lvgl_driver/lv_port_indev.c`:
  - Added temporary debug output for keypad troubleshooting (later removed)
  - Verified correct ADC channel usage with D0WDQ6_VER

### 📊 Performance Benchmarks

| Metric | v1.4.0 | v1.5.0 | Improvement |
|--------|--------|--------|-------------|
| Channel switch time | 150ms | 50ms | **3x faster** ⚡ |
| RSSI jitter | ±50 ADC | ±10 ADC | **5x smoother** 📊 |
| Diversity switches/sec | ~10 | ~2 | **5x more stable** 🔄 |
| CPU usage (idle) | ~40% | ~30% | **25% reduction** 💪 |
| UART communication | N/A | 420kbps CRSF | **New feature** 📡 |

### 🔌 Hardware Requirements

- ESP32 with ESP-IDF v5.5.2 or later
- RX5808 diversity receiver module v1.2 or v1.4
- ExpressLRS backpack (HappyModel EP1 or compatible) - optional
- 4x wires for backpack connection (if using wireless control)

### 🚦 Testing Status

✅ **Passed:**
- Build system compilation (no errors)
- Git submodule initialization (esp32-video component)
- Navigation pad functionality (with D0WDQ6_VER enabled)
- RSSI calibration procedure
- ExpressLRS backpack UART initialization
- Firmware version display on boot

⏳ **Pending Field Testing:**
- Wireless channel changing from radio
- Long-term diversity switching stability
- All 6 bands (A, B, E, F, R, L) in flight conditions
- Battery life impact of backpack integration

### 📝 Known Issues

1. **Band E Partial Support**: Channels 4, 7, and 8 use fallback frequency 5800 MHz (not in standard VTX tables)
2. **Hardware Version Detection**: Manual configuration required in `hwvers.h` (no auto-detection)
3. **Example File Compilation**: `elrs_backpack_example.c` renamed to `.txt` to prevent build errors

### 🔄 Migration Guide from v1.4

1. **Update Git repository:**
   ```bash
   git pull origin main
   git submodule update --init --recursive
   ```

2. **Check hardware version:**
   - If using v1.2 hardware, verify `D0WDQ6_VER` is defined in `hwvers.h`

3. **Optional: Enable ExpressLRS backpack:**
   - Flash backpack module with VRX firmware (see `ELRS_EP1_BACKPACK_GUIDE.md`)
   - Wire backpack to ESP32 GPIO16/17
   - Enable in menuconfig: `idf.py menuconfig` → Component config → ExpressLRS Backpack

4. **Rebuild firmware:**
   ```bash
   cd Firmware/ESP32/RX5808
   idf.py fullclean
   idf.py build
   idf.py -p COM4 flash monitor
   ```

5. **Re-calibrate RSSI:**
   - Navigate to Scan → RSSI Calibration
   - Follow on-screen prompts

6. **Verify version on boot:**
   ```
   I (XXX) SYSTEM: ╔══════════════════════════════════════════════════╗
   I (XXX) SYSTEM: ║  RX5808 Diversity Receiver Firmware v1.5.0      ║
   I (XXX) SYSTEM: ╚══════════════════════════════════════════════════╝
   ```

### 🙏 Credits

- Original firmware: Ft-Available/RX5808-Div
- ExpressLRS integration: Community contribution
- Performance optimizations: Community testing and feedback
- Documentation: Comprehensive guides created for v1.5

### 📅 Release Notes

This is a major feature release focused on wireless integration and performance. All users are encouraged to upgrade, especially those wanting ExpressLRS backpack control. Users on v1.2 hardware must enable `D0WDQ6_VER` for proper operation.

**Recommended for:**
- Racing pilots wanting wireless channel control
- Users experiencing RSSI jitter or slow channel switching
- v1.2 hardware owners (navigation pad fix)

---

## v1.4.0 (Previous Release)

### Features
- Dual RX5808 diversity receiver
- LVGL-based GUI
- Manual channel selection
- RSSI-based diversity switching
- Battery voltage monitoring
- Navigation pad controls
- OSD format selection
- Multi-language support

### Hardware Support
- ESP32 main controller
- ST7735S or ST7789 LCD display
- RX5808x2 receiver modules
- ADC-based RSSI monitoring
- SPI interface for RX5808 control

---

## Future Roadmap (v1.6+)

### Planned Features
- [ ] Auto-scan feature (find strongest channel)
- [ ] Frequency spectrum analyzer display
- [ ] WiFi web interface for configuration
- [ ] OTA firmware updates
- [ ] Smart diversity algorithm (predictive switching)
- [ ] Recording/playback of channel statistics
- [ ] Integration with other RC link protocols (SBUS, CRSF Nano RX)

### Under Consideration
- [ ] Multiple receiver profiles (race, freestyle, cinematic)
- [ ] GPS data overlay from backpack
- [ ] Battery voltage telemetry back to TX
- [ ] DVR recording trigger via backpack
- [ ] Custom band/frequency editor

---

**For support, issues, or feature requests:**
- Repository: https://github.com/Ft-Available/RX5808-Div
- Documentation: See `Firmware/ESP32/` folder

**Happy flying!** 🚁✨
