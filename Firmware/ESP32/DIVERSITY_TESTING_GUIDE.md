# RX5808 Diversity Algorithm Testing Guide v1.5.1

## Overview
This guide provides step-by-step testing procedures for the new diversity algorithm implementation in firmware v1.5.1. Test each component independently to verify correct operation.

---

## Prerequisites

### Hardware Setup
- **RX5808 Diversity Module** (v1.1 or v1.2 hardware)
- **Two antennas** connected to RX1 and RX2
- **VTX transmitter** (5.8GHz video transmitter for testing)
- **Power supply** (appropriate for your module)
- **Display/Goggles** connected to video out

### Software Requirements
- **Firmware v1.5.1** flashed (you should see "Diversity algorithm initialized!" in boot logs)
- **Serial monitor** (optional - for debug logs)
- **Basic understanding** of ESP-IDF monitor commands

---

## Test 1: Verify Diversity Initialization

### Purpose
Confirm the diversity algorithm starts correctly on boot.

### Steps
1. **Power on** the RX5808 module
2. **Watch the serial console** (if connected via USB) or check boot screen
3. **Look for** the initialization message:
   ```
   RX5808 init success!
   Diversity algorithm initialized!
   ``` 

### Expected Results
- ✅ "Diversity algorithm initialized!" message appears
- ✅ No error messages or crashes
- ✅ Module boots to main screen normally

### Troubleshooting
- ❌ If message missing: diversity_init() not called - check system.c
- ❌ If crash occurs: NVS partition may be corrupted - run `idf.py erase-flash`

---

## Test 2: Main Screen Telemetry Overlay

### Purpose
Verify the diversity telemetry displays correctly on the main screen.

### Prerequisites
- Set **Signal Source = Auto** in setup menu (diversity mode)
- Navigate to main screen

### Visual Elements to Check

#### 2.1 Active Receiver Indicator
- **Location**: Top right corner
- **Display**: Single letter "A" or "B" in green text
- **Background**: Black with rounded corners

**Test Procedure:**
1. Cover antenna on RX A → indicator should switch to "B"
2. Uncover RX A + cover RX B → indicator should switch back to "A"
3. Verify indicator matches the stronger signal

**Expected Results:**
- ✅ Indicator updates within 1-2 seconds of signal change
- ✅ Letter is clearly visible (green on black)
- ✅ Switches predictably based on RSSI

#### 2.2 Mode Indicator
- **Location**: Below channel label (left side)
- **Display**: "RACE", "FREE", or "L-R" in orange text
- **Updates**: When mode changed in setup menu

**Test Procedure:**
1. Note current mode displayed
2. Enter setup menu
3. Change "Div Mode" to different setting
4. Save & exit
5. Verify mode label updated

**Expected Results:**
- ✅ Mode label matches selected setting
- ✅ RACE = "RACE", Freestyle = "FREE", Long Range = "L-R"
- ✅ Persists across reboots

#### 2.3 Switch Counter
- **Location**: Bottom right, above delta
- **Display**: "SW:###" in yellow text
- **Range**: 0 to 999+ switches

**Test Procedure:**
1. Note initial switch count
2. Cover/uncover antennas alternately 10 times
3. Count actual switches that occur
4. Compare displayed count to actual

**Expected Results:**
- ✅ Counter increments on each switch event
- ✅ Roughly matches manual count (±1-2 due to hysteresis)
- ✅ Resets to 0 on reboot

#### 2.4 RSSI Delta
- **Location**: Bottom right, below switch counter  
- **Display**: "Δ:###" in light blue text
- **Range**: -100 to +100

**Test Procedure:**
1. With both antennas connected, note delta value
2. Delta should be near 0 for similar signals
3. Cover one antenna → delta should increase
4. Cover other antenna instead → delta should reverse sign

**Expected Results:**
- ✅ Delta shows difference between RX A and RX B
- ✅ Positive = RX A stronger, Negative = RX B stronger
- ✅ Updates in real-time (500ms refresh)

### Full Integration Test
**Method:** Fly or simulate flying conditions
1. Start with both antennas clear → note baseline
2. Create multipath (walk between TX and RX) → observe switches
3. Block one antenna partially → verify active RX stays on strong signal
4. Block both briefly → system should fallback gracefully

**Expected Results:**
- ✅ All four telemetry elements visible simultaneously
- ✅ No flickering or text corruption
- ✅ Updates synchronized (no stale data)
- ✅ Readable during flight (not too small/cluttered)

---

## Test 3: Mode Selection Menu

### Purpose
Verify users can change diversity modes via the setup menu.

### Steps
1. **Enter Setup Menu:**
   - From main screen, short press OK button
   - Navigate to "Setup" menu item
   - Press OK to enter

2. **Locate Diversity Mode Setting:**
   - Scroll down past BackLight, FanSpeed, Boot Logo, Beep, OSD Type, Language, Signal
   - Find "**Div Mode**" (English) or "**分集模式**" (Chinese)
   - Setting appears **before** "Save&Exit"

3. **Change Mode:**
   - Press LEFT/RIGHT arrows to cycle modes
   - Available modes:
     - **RACE** - Fast switching (80ms dwell, 2% hysteresis)
     - **FREE** - Balanced (250ms dwell, 4% hysteresis)  
     - **L-R** - Stable (400ms dwell, 6% hysteresis)
   
4. **Save Changes:**
   - Navigate to "Save&Exit"
   - Press OK
   - Mode should persist to NVS

5. **Verify Mode Active:**
   - Return to main screen
   - Check mode indicator matches selection
   - Test switching behavior aligns with mode characteristics

### Expected Results Per Mode

#### RACE Mode
- ✅ Very responsive switching (80ms+150ms cooldown)
- ✅ Low hysteresis (2%) - switches on small RSSI differences
- ✅ Good for racing with frequent rapid direction changes
- ✅ May switch more often (higher SW counter)

#### Freestyle Mode (Default)
- ✅ Moderate switching (250ms+500ms cooldown)
- ✅ Medium hysteresis (4%) - balanced stability
- ✅ Good general-purpose mode
- ✅ Should stay on strong receiver during stable flight

#### Long Range Mode
- ✅ Conservative switching (400ms+800ms cooldown)
- ✅ High hysteresis (6%) - very stable, avoids unnecessary switches
- ✅ Good for long range cruise / avoiding switch-induced dropouts
- ✅ Lowest switch count in normal conditions

### Troubleshooting
- ❌ Mode not in menu: Check page_setup.c modifications
- ❌ Mode doesn't save: diversity_calibrate_save() not working - check NVS partition
- ❌ Behavior identical across modes: diversity_set_mode() not called correctly

---

## Test 4: Backend Algorithm Validation

### Purpose
Verify the core diversity algorithm processes data correctly.

### 4.1 RSSI Normalization

**Test:** Check that raw ADC values convert to 0-100 scale

**Steps:**
1. Enable debug logging (optional):
   ```c
   // In diversity.c, set log level to DEBUG
   esp_log_level_set("DIVERSITY", ESP_LOG_DEBUG);
   ```

2. Monitor serial output during operation
3. Look for RSSI values printed

**Expected:**
- ✅ RSSI values range 0-100 (normalized)
- ✅ Strong signal = 80-100
- ✅ Weak signal = 0-30
- ✅ No values exceed 100 or go below 0

### 4.2 Statistical Analysis

**Test:** Verify variance and slope calculation

**Method:**
1. Create stable signal conditions
2. Note variance should be LOW (< 5)
3. Create unstable conditions (multipath, movement)
4. Note variance should INCREASE (> 10)

**Expected Results:**
- ✅ Stable signal = low variance, high stability score
- ✅ Unstable signal = high variance, low stability score
- ✅ Falling signal = negative slope → preemptive switch

### 4.3 Receiver Health Detection

**Test:** System detects stuck/dead receivers

**Scenarios:**

**Stuck High (RX stuck at max RSSI):**
1. Disconnect antenna → RSSI should drop
2. If RSSI stays > 95 with zero variance → health flag set
3. System should prefer other receiver

**Stuck Low (RX stuck at 0):**
1. Connect strong signal → RSSI should rise
2. If RSSI stays < 5 constantly → health flag set
3. System ignores this receiver

**No Variance (RX not responding):**
1. Change signal strength → RSSI should change
2. If variance < 2 for extended time → health flag set
3. System detects frozen ADC

**Expected Results:**
- ✅ System detects and avoids unhealthy receivers
- ✅ Switches to good receiver even if nominally weaker
- ✅ Logs health issues (check serial console)

### 4.4 Switch Logic Validation

**Test:** Dwell time and hysteresis working correctly

**RACE Mode Test:**
```
Expected behavior:
- Switch threshold: 2% RSSI difference
- Dwell time: Must stay for 80ms before switch
- Cooldown: 150ms after switch (hysteresis doubled to 4%)
```

**Test Procedure:**
1. Set mode to RACE
2. Create rapid RSSI differences > 2%
3. Time how long before switch occurs
4. Create multiple switches in quick succession

**Expected Results:**
- ✅ No switch faster than 80ms dwell
- ✅ After switch, new receiver must be 4% better during cooldown
- ✅ System stabilizes on best receiver, not oscillating

---

## Test 5: Performance Characterization

### Purpose
Measure real-world diversity performance.

### 5.1 Switch Rate Test

**Scenario:** Normal flying conditions

**Steps:**
1. Reset switch counter (reboot or note starting value)
2. Fly for 5 minutes in typical environment
3. Record final switch count
4. Calculate switches/minute

**Expected Ranges:**
- **RACE mode:** 20-50 switches/min (aggressive)
- **Freestyle mode:** 5-15 switches/min (balanced)
- **Long Range mode:** 1-5 switches/min (conservative)

**Analysis:**
- Too many switches? Increase hysteresis or use more stable mode
- Too few switches? Decrease hysteresis or use more responsive mode

### 5.2 Stability Test

**Scenario:** Static position with stable signal

**Steps:**
1. Position TX and RX with clear line of sight
2. Keep both antennas exposed, equal signal
3. Watch for 60 seconds
4. Count unintended switches

**Expected Results:**
- ✅ Should remain on one receiver (0-1 switches/min)
- ✅ RSSI delta should fluctuate slightly but not trigger switches
- ✅ Stability score should be consistently high (> 80)

### 5.3 Response Time Test

**Scenario:** Sudden signal loss on active receiver

**Steps:**
1. Note which receiver is active (A or B indicator)
2. Quickly cover antenna on active receiver
3. Measure time until switch indicator changes
4. Compare to mode's dwell time + settling time

**Expected Results:**
- **RACE:** Switch within 130-230ms (80ms dwell + 50ms settling)
- **Freestyle:** Switch within 300-400ms (250ms dwell + 50ms settling)
- **Long Range:** Switch within 450-550ms (400ms dwell + 50ms settling)

### 5.4 Multipath Handling Test

**Scenario:** Difficult RF environment (indoors, metal structures)

**Steps:**
1. Set mode to freestyle (balanced)
2. Walk around while watching video feed
3. Note frequency of switches and video quality
4. Compare to pre-v1.5.1 behavior (if possible)

**Expected Results:**
- ✅ Switches occur but not excessively (< 30/min)
- ✅ System settles on best receiver quickly
- ✅ No "hunting" behavior (rapid back-and-forth switches)
- ✅ Video quality improved vs single receiver

---

## Test 6: Edge Cases & Stress Testing

### 6.1 Both Antennas Blocked
**Test:** What happens with dual signal loss

**Steps:**
1. Cover both antennas simultaneously
2. Observe behavior

**Expected:**
- ✅ System picks receiver with slightly better signal
- ✅ May switch occasionally but no crash
- ✅ Video shows noise/static but system remains responsive

### 6.2 Rapid Signal Fluctuations
**Test:** Simulate extreme multipath

**Steps:**
1. Wave hand between TX and RX rapidly
2. Create 10+ RSSI changes per second
3. Watch for crashes or freezes

**Expected:**
- ✅ System handles rapid changes gracefully
- ✅ Dwell time prevents excessive switching
- ✅ No crashes or resets
- ✅ Switch counter increases but not wildly

### 6.3 Extended Operation
**Test:** Long-duration stability

**Steps:**
1. Run for 30+ minutes continuously
2. Monitor switch counter and RSSI delta
3. Check for memory leaks or slowdowns

**Expected:**
- ✅ No performance degradation over time
- ✅ Switch counter can exceed 999 without issues
- ✅ System remains responsive
- ✅ No memory leaks (check with `esp_get_free_heap_size()`)

### 6.4 Mode Switching While Flying
**Test:** Change modes without reboot

**Steps:**
1. Start in RACE mode, note behavior
2. Pause, enter setup, switch to L-R mode
3. Resume, verify behavior changed

**Expected:**
- ✅ New mode takes effect immediately (no reboot needed)
- ✅ Switch counter resets (optional - current implementation keeps count)
- ✅ Switching characteristics match new mode

---

## Test 7: Integration with Existing Features

### 7.1 ExpressLRS Backpack Integration
**Test:** Diversity works with ELRS channel control

**Steps:**
1. Configure ELRS backpack (if using)
2. Change channels via radio
3. Verify diversity continues working

**Expected:**
- ✅ Diversity algorithm unaffected by ELRS commands
- ✅ Active RX indicator still updates correctly
- ✅ No conflicts or crashes

### 7.2 OSD Compatibility
**Test:** Diversity telemetry visible on OSD

**Steps:**
1. Enable composite video sync in setup
2. View output through goggles/monitor
3. Lock main screen (long press OK)

**Expected:**
- ✅ Active RX indicator visible on OSD
- ✅ Mode, SW count, delta readable
- ✅ No flickering or interference with video
- ✅ Binary OSD optimization working

### 7.3 Manual Channel Override
**Test:** Lock mode disables diversity

**Steps:**
1. On main screen, long press OK to lock
2. Observe active RX indicator
3. Cover active antenna

**Expected:**
- ✅ Lock mode may fix to one receiver (implementation dependent)
- ✅ OR diversity continues working even when locked
- ✅ Manual channel control still functions
- ✅ Unlock restores expected behavior

---

## Debug Logging

### Enable Detailed Logs

Add to `diversity.c` (top):
```c
static const char* TAG = "DIVERSITY";

void diversity_enable_debug_logging(void) {
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
}
```

### Key Log Messages

**Initialization:**
```
I (1234) DIVERSITY: Diversity algorithm v1.5.1 initialized
I (1235) DIVERSITY: Mode: FREESTYLE, RX A: calib loaded, RX B: calib loaded
```

**Switch Events:**
```
D (5678) DIVERSITY: SWITCH A->B (RSSI: 45 vs 67, delta: -22, score: 85 vs 92)
```

**Health Issues:**
```
W (9012) DIVERSITY: RX A health: stuck HIGH (rssi=99, variance=0.2)
W (9013) DIVERSITY: Preferring RX B despite lower RSSI
```

### Monitor Command

```bash
idf.py monitor --port COM3
# Or:
idf.py monitor --port /dev/ttyUSB0 (Linux)
```

---

## Performance Benchmarks

### Target Metrics (v1.5.1)

| Metric | RACE Mode | Freestyle Mode | Long Range Mode |
|--------|-----------|----------------|-----------------|
| **Dwell Time** | 80ms | 250ms | 400ms |
| **Cooldown** | 150ms | 500ms | 800ms |
| **Hysteresis** | 2% / 4% | 4% / 8% | 6% / 12% |
| **Switches/Min (normal)** | 20-50 | 5-15 | 1-5 |
| **Switch Latency** | 130-230ms | 300-400ms | 450-550ms |
| **Stability Score (clear)** | 70-90 | 80-95 | 85-100 |
| **CPU Load** | < 2% | < 2% | < 2% |

### Comparison to v1.5.0

| Feature | v1.5.0 | v1.5.1 |
|---------|--------|--------|
| **Algorithm** | Simple RSSI threshold | Variance-based stability scoring |
| **Modes** | None (fixed) | 3 selectable profiles |
| **Preemptive Switching** | No | Yes (slope detection) |
| **Health Detection** | No | Yes (stuck/dead RX detection) |
| **Calibration** | Manual ADC min/max | Per-receiver floor/peak calibration |
| **Telemetry** | Basic RSSI bars | Active RX, mode, SW count, delta |
| **Persistence** | None | NVS storage for mode & calibration |

---

## Troubleshooting Guide

### Issue: No Diversity Telemetry on Main Screen
**Symptoms:** Main screen looks like v1.5.0, no A/B indicator

**Possible Causes:**
1. Signal Source not set to "Auto" (diversity mode disabled)
   - **Fix:** Setup menu → Signal → Auto

2. UI elements not created (if Signal Source = Recv1/Recv2)
   - **Fix:** This is expected - telemetry only shows in Auto mode

3. Display buffer too small
   - **Fix:** Check LVGL_MEM_SIZE in sdkconfig

### Issue: Mode Selection Not in Setup Menu
**Symptoms:** "Div Mode" missing from setup page

**Possible Causes:**
1. page_setup.c not compiled with diversity.h
   - **Fix:** Clean build: `idf.py fullclean && idf.py build`

2. CMakeLists.txt needs refresh
   - **Fix:** Delete build folder, rebuild

### Issue: Excessive Switching
**Symptoms:** Switch counter increments rapidly, video unstable

**Possible Causes:**
1. Hysteresis too low for environment
   - **Fix:** Use Long Range mode (6% hysteresis)

2. Antennas mismatched (different gains)
   - **Fix:** Use matched antenna pair

3. Antennas too close together (cross-coupling)
   - **Fix:** Space antennas 50-100mm apart

4. Multipath-heavy environment
   - **Fix:** Increase hysteresis, use directional antennas

### Issue: No Switching Occurs
**Symptoms:** Active RX never changes, even with signal loss

**Possible Causes:**
1. Dwell time too long + weak signal differences
   - **Fix:** Use RACE mode or amplify antenna separation

2. One receiver not working (stuck)
   - **Fix:** Check health flags in logs, inspect hardware

3. Calibration incorrect (both RX normalized to similar values)
   - **Fix:** Re-run calibration wizard (not yet implemented - use defaults)

### Issue: Module Crashes on Boot
**Symptoms:** Reboot loop, never reaches main screen

**Possible Causes:**
1. NVS partition corrupted
   - **Fix:** `idf.py erase-flash` then reflash firmware

2. Stack overflow in diversity_update()
   - **Fix:** Increase task stack size in FreeRTOS config

3. Divide-by-zero in statistics (unlikely with guards)
   - **Fix:** Add assertions, check logs before crash

### Issue: Performance Degradation
**Symptoms:** Slower UI, delayed switch responses

**Possible Causes:**
1. Memory leak in diversity code
   - **Fix:** Check with `esp_get_free_heap_size()`, inspect allocations

2. Timer conflict (250Hz sampling vs UI timer)
   - **Fix:** Verify diversity_update() using esp_timer_get_time() checks

3. Too many log statements (if debug enabled)
   - **Fix:** Reduce log verbosity or increase UART baud rate

---

## Next Steps After Testing

### If Tests Pass ✅
1. **Build final binaries:**
   ```bash
   idf.py build
   ```

2. **Create release documentation** (RELEASE_v1.5.1.md)

3. **Update main README** with v1.5.1 features

4. **Tag release in git:**
   ```bash
   git tag -a v1.5.1 -m "Diversity algorithm with mode profiles"
   git push origin v1.5.1
   ```

5. **Test calibration wizard** (Phase 3 - see CALIBRATION_WIZARD_PLAN.md)

### If Tests Fail ❌
1. **Document failure details** (mode, conditions, logs)
2. **Check relevant troubleshooting section above**
3. **Enable debug logging** for detailed diagnostics
4. **File issue** with reproduction steps
5. **Revert to v1.5.0** if critical

---

## Test Report Template

Use this template to document your test results:

```markdown
## RX5808 Diversity v1.5.1 Test Report

**Tester:** [Your Name]
**Date:** [YYYY-MM-DD]
**Hardware:** v1.1 / v1.2 (circle one)
**Firmware:** v1.5.1 (commit: a1dcb50)

### Test 1: Initialization
- [ ] PASS  [ ] FAIL  [ ] N/A
- Notes:

### Test 2: Main Screen Telemetry
- [ ] PASS  [ ] FAIL  [ ] N/A
- Active RX indicator: [ ] PASS [ ] FAIL
- Mode indicator: [ ] PASS [ ] FAIL
- Switch counter: [ ] PASS [ ] FAIL
- RSSI delta: [ ] PASS [ ] FAIL
- Notes:

### Test 3: Mode Selection Menu
- [ ] PASS  [ ] FAIL  [ ] N/A
- RACE mode: [ ] PASS [ ] FAIL
- Freestyle mode: [ ] PASS [ ] FAIL
- Long Range mode: [ ] PASS [ ] FAIL
- Persistence: [ ] PASS [ ] FAIL
- Notes:

### Test 4: Backend Algorithm
- [ ] PASS  [ ] FAIL  [ ] N/A
- Normalization: [ ] PASS [ ] FAIL
- Statistics: [ ] PASS [ ] FAIL
- Health detection: [ ] PASS [ ] FAIL
- Switch logic: [ ] PASS [ ] FAIL
- Notes:

### Test 5: Performance
- [ ] PASS  [ ] FAIL  [ ] N/A
- Switch rate: _____ switches/min in [MODE]
- Stability: _____ switches in 60sec (target: 0-1)
- Response time: _____ ms (mode: [RACE/FREE/L-R])
- Notes:

### Test 6: Edge Cases
- [ ] PASS  [ ] FAIL  [ ] N/A
- Dual antenna block: [ ] PASS [ ] FAIL
- Rapid fluctuations: [ ] PASS [ ] FAIL
- Extended operation: [ ] PASS [ ] FAIL
- Runtime mode switching: [ ] PASS [ ] FAIL
- Notes:

### Test 7: Integration
- [ ] PASS  [ ] FAIL  [ ] N/A
- ELRS backpack: [ ] PASS [ ] FAIL [ ] N/A
- OSD compatibility: [ ] PASS [ ] FAIL
- Manual override: [ ] PASS [ ] FAIL
- Notes:

### Overall Assessment
- [ ] READY FOR RELEASE
- [ ] NEEDS MINOR FIXES
- [ ] NEEDS MAJOR REWORK
- [ ] BLOCKED BY: ________________

### Additional Comments:
[Your detailed feedback here]
```

---

## Contact & Support

**Issues:** https://github.com/LochnessFPV/RX5808-Div/issues
**Discussions:** https://github.com/LochnessFPV/RX5808-Div/discussions

**Key Files for Reference:**
- `diversity.h` - API and data structures
- `diversity.c` - Algorithm implementation (592 lines)
- `page_main.c` - Telemetry overlay UI
- `page_setup.c` - Mode selection menu
- `system.c` - Initialization sequence

**Version Info:**
- Firmware: v1.5.1
- Phase 1 commit: a064ccc
- Phase 2 commit: a1dcb50
- ESP-IDF: v5.5.2-2

---

**Happy Testing! 🚁📡**

*For questions about specific test procedures, refer to inline comments in diversity.c or open a discussion on GitHub.*
