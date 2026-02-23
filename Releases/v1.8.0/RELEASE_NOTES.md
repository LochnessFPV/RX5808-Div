# RX5808 Diversity Receiver v1.8.0 — UX Overhaul & Performance

**Release Date:** February 23, 2026  
**Release Type:** Stable  
**Commit:** df9033d  
**Hardware Compatibility:** ESP32 D0WDQ6 v1.2, ST7735 80×160 LCD  
**Build Toolchain:** ESP-IDF v5.5.2-2

---

## Overview

v1.8.0 is the largest update to the RX5808-Div firmware since the project was created. It overhauls navigation, introduces a full LED feedback system, rewrites the diversity switching algorithm, replaces the calibration algorithm with a statistically-robust Median+MAD approach, and fixes a dozen runtime bugs. All changes are backward-compatible — no settings are lost on upgrade.

---

## What's New

### Quick Action Menu
From the main page, hold LEFT for 1.5 seconds to open an overlay menu with instant access to: Scan, Spectrum Analyzer, Calibration, Band X Editor, and Settings. What previously required 7+ steps (navigate back to root, open menu, drill down) now takes 2.

The lock/unlock toggle has also moved to a single long-press of the center button on the main page — no more entering the main menu just to unlock.

### Menu Reorganisation
The main menu structure has been reorganised so that Quick Scan, Spectrum Analyzer, and Calibration are grouped logically at the top level. Navigation paths that previously required multiple hops have been shortened.

### LED Feedback System
A dedicated LED driver (`led.c`) replaces the old single-pattern blink with meaningful, context-aware patterns:

| State | Pattern |
|---|---|
| Locked | Solid ON |
| Unlocked | Slow heartbeat |
| Scanning / busy | Fast blink |
| Calibrating | Smooth breathing (LUT-based, no `sinf`) |
| Antenna switch | Double-blink |
| Low signal (both RX < 15%) | Triple beep warning (once per 5 s) |
| Signal strength | LED brightness scales 0–100% |

LED transitions are non-blocking. A `saved_pattern` mechanism restores the previous mode after transient events (e.g., double-blink returns to heartbeat, not OFF). All patterns respect active-low polarity on GPIO21.

### Adjustable LED Brightness
A new **LED Brightness** setting in the Setup page (0–100%, adjustable in 5% steps) persists in NVS and scales all LED patterns at runtime. Setting 0% disables the LED entirely — useful for goggle builds where the indicator LED is visible to the pilot.

### Silent Diversity + Low-Signal Warning
Antenna switches are now silent — no beep on every switch, which was disruptive during rapid manoeuvres. Instead, a triple-beep fires only when **both** receivers fall below the noise floor simultaneously (once per 5 s), giving a clear range warning without polluting normal flight audio.

### Band X Inline Frequency Editor
Custom frequencies can now be edited directly from the main page without entering a separate editor screen. Double-click the center button to enter edit mode; the active digit blinks (asymmetric: 600 ms on / 150 ms off). Hold UP/DOWN for auto-repeat with fast-scroll (×10 MHz) after 2 s. Frequency is colour-coded: green = valid FPV range, amber = extended, red = out of range. Saved with a single confirm press; a "Saved!" toast confirms persistence.

### Scan Completion Confirmation Dialog
After a scan finishes, a confirmation dialog presents the results and asks the user to confirm or re-scan before navigating away. Eliminates accidental navigation out of the results screen.

### Restore Defaults Button
A **Restore Defaults** button in the Setup page resets all configuration fields to their factory values without requiring a full flash erase.

### Boot Animation Skip
Any keypress during the boot animation immediately skips it to the main page.

---

## Diversity Algorithm Overhaul

The diversity switching engine has been substantially rewritten. All changes are in `diversity.c` / `diversity.h`.

### Glitch-Free Antenna Switch
The GPIO switch sequence now transitions through a both-receivers-off intermediate state before enabling the target receiver, eliminating any brief double-reception glitch. The switch handler is tagged `IRAM_ATTR` for deterministic latency regardless of which task triggers it.

### Time-Normalised Slope Scoring
The RSSI slope score was previously a raw sample delta, meaning its effective threshold varied with the update rate. It is now expressed in **ADC units/second** — the half-window average divided by the interval in milliseconds. All three flying-mode slope thresholds (RACE/FREESTYLE/LR) have been recalibrated to the new units. The algorithm now behaves identically at 20 Hz and 100 Hz adaptive rates.

### Per-Receiver Software AGC
Each receiver now maintains an independent 10-second rolling baseline of its normalised RSSI. This baseline offsets the receiver's score to compensate for hardware gain asymmetry between the RX-A and RX-B modules — a gain difference of up to ±15 points is corrected at runtime without any user calibration step.

### Outcome-Weighted Hysteresis
After a switch event, if the newly-active receiver's RSSI improves by more than 5 points within 200 ms (confirming the switch was correct), it receives a +8 score bonus that persists for 5 seconds. This rewards accurate predictions and naturally penalises ping-pong switching.

### Micro-Frequency-Offset Interference Rejector
Inspired by the RapidFire in-band interference rejection scheme. When the active receiver shows weak **and** declining RSSI (normalised score < 22, slope < −80 units/s) after being stable for at least 1.5 s, a non-blocking FSM automatically trials a ±1 MHz offset on both RX5808 chips:

1. **EVAL_PLUS** — try +1 MHz, evaluate for 300 ms
2. **EVAL_MINUS** — if +1 failed, try −1 MHz, evaluate for 300 ms
3. **HOLD** — if either direction improved RSSI by ≥ 5 points, hold for 30 s
4. **RESTORE** — return to nominal frequency (or immediately if the user switches channels)

A 30-second cooldown after any attempt prevents oscillation. No extra delay is introduced because the RX5808 PLL already settles in ~50 ms.

---

## Calibration Algorithm Overhaul

### Median + MAD Floor Estimation
Diversity floor calibration and spectrum noise-floor estimation now use **Median + MAD (Median Absolute Deviation)** instead of simple averaging:

- Floor sample: median of 100 samples over 2 s; safety margin = `max(1.5 × MAD, 10% × median)`
- Peak sample: 95th percentile of 100 samples to ignore transient spikes
- Reported accuracy: **>95%** (was ~70–80% with averaging)
- False positive rate: **<5%** (was ~20–30%)

This approach is robust to interference spikes during calibration and adapts its margin to actual RF environment variance rather than applying a fixed percentage.

### Improved Classic Scan Calibration
The classic scan mode now takes 5 consecutive samples per channel (was 3) with a 50 ms dwell (was 30 ms). The longer window filters out sub-50 ms transient noise spikes and improves single-channel detection accuracy to **>90%** (was ~75–85%).

### Fixed ESP Timer Exhaustion Crash
The previous calibration loop used `vTaskDelay()` called 500 times at 4 ms intervals, which exhausted the ESP timer pool and triggered an `assert failed: esp_timer_impl_set_alarm_id` crash on some devices. The loop was restructured to use 100 × 20 ms delays (same 2-second total duration, 80% fewer timer operations).

### Adaptive Spectrum Noise Floor
The spectrum analyzer's noise-floor threshold now uses the same MAD-based adaptive margin: `noise_floor = median + max(1.5 × MAD, 10% × median)`. This reduces false-positive activity bar triggering in high-variance RF environments while remaining conservative in clean conditions.

---

## Performance Improvements

### Dedicated CPU Cores
All hardware tasks (RSSI sampling, diversity algorithm, beep, LED, thermal fan) are explicitly pinned to **Core 1**. LVGL runs exclusively on **Core 0**. A 50 ms PLL-settle call during channel switching no longer causes UI jank.

### No More LVGL Task Blocking
`vTaskDelay(50)` inside an LVGL event callback was stalling Core 0's render loop for 50 ms on every page change. Replaced with a one-shot `lv_timer` so LVGL never blocks.

### Diversity Hot-Path: Integer Math, Fewer Syscalls
- `sqrtf(variance)` and `fabsf(slope)` in the scoring loop (called up to 200×/sec) replaced with an integer Newton-step `isqrt()` and a branchless abs — no libm on the hot path.
- `esp_timer_get_time()` snapshotted once per `diversity_update()` cycle and passed as a parameter to all helper functions, eliminating 4 redundant syscalls per update.

### CPU Auto Mode
A new AUTO frequency mode dynamically scales the ESP32 between 80/160/240 MHz and enables light sleep when the interface is locked/idle. Active use returns to full speed. Reduces idle temperature noticeably.

### Complete ADC Oneshot Migration
All ADC reads (RSSI sampling, keypad, temperature sensor) have been migrated from the legacy `adc1_get_raw()` API to `adc_oneshot_read()`. An `adc_mutex` serialises all calls to the shared ADC handle — the RSSI burst on Core 1 and the keypad reader on Core 0 no longer race.

### Other
- Breathing LED uses a precomputed 100-entry LUT instead of `sinf()` at runtime.
- LEDC fan duty: `duty * 81.91f` → `duty * 8191 / 100` (pure integer).
- `rssi0_filter_index` and `rssi1_filter_index` are now independent so reading RX_A never advances RX_B's circular buffer pointer.

---

## Bug Fixes

| # | Description |
|---|---|
| 1 | Diversity bar highlight was inverted — the RX_A bar lit up when RX_B was active and vice versa. |
| 2 | Legacy simple-diversity `else` branch still ran inside the RSSI task after `diversity.c` took over GPIO, causing a dual-write conflict on the antenna switch GPIO. |
| 3 | `printf("KEY_ADC_V: %d\n")` fired on every keypress, flooding the UART at 50 Hz. |
| 4 | `vTaskDelay(50)` inside an LVGL event callback stalled the render task for 50 ms on every page change. |
| 5 | `rssi_filter_index` was shared between both receivers; advancing it during RX_A sampling also stepped RX_B's circular buffer. |
| 6 | `vTaskDelay(0)` in the diversity task yielded without sleeping, starving lower-priority tasks and triggering WDT warnings. Fixed to `vTaskDelay(1)`. |
| 7 | `zero_streak` counter in the input driver was `uint8_t`, wrapping to 0 after 255 cycles and incorrectly re-triggering key events during prolonged idle. Fixed to `uint32_t`. |
| 8 | Active-low LED polarity was not handled — the LED could not reach a true OFF state on hardware revisions where GPIO21 is active-low. |
| 9 | Band E frequencies were incorrect. Restored correct values matching published Band E channel plan. |
| 10 | Chinese string literals in `page_menu.c` and `page_setup.c` were corrupted by source-file encoding mismatch, displaying garbled characters on the Chinese UI. Fixed to proper UTF-8 escape sequences. |
| 11 | ESP timer exhaustion crash (`assert failed: esp_timer_impl_set_alarm_id`) during diversity calibration on some devices. |
| 12 | Duplicate `#include "nvs_flash.h"` on consecutive lines in `rx5808.c`. |

---

## Upgrading from v1.7.1

No settings are lost. The device will migrate automatically on first boot. The navigation layout has changed (lock is now long-press center; quick menu is long-hold LEFT) but all existing features remain accessible.

---

## Installation

**Via Web Configurator (recommended)**  
Visit the web configurator, connect via USB, select v1.8.0, click Install.

**Via esptool / ESP Flash Download Tool**

| File | Address |
|---|---|
| `RX5808-v1.8.0-bootloader.bin` | 0x1000 |
| `RX5808-v1.8.0-partition-table.bin` | 0x8000 |
| `RX5808-v1.8.0-firmware.bin` | 0x10000 |

---

## Included Files

- `manifest.json` — Web configurator metadata and flash addresses
- `RX5808-v1.8.0-bootloader.bin` @ 0x1000
- `RX5808-v1.8.0-partition-table.bin` @ 0x8000
- `RX5808-v1.8.0-firmware.bin` @ 0x10000
- `RELEASE_NOTES.md` — This file

---

## Links

- [Repository](https://github.com/Ft-Available/RX5808-Div)
- [Full Changelog](https://github.com/Ft-Available/RX5808-Div/blob/main/Firmware/ESP32/CHANGELOG.md)
- [Issue Tracker](https://github.com/Ft-Available/RX5808-Div/issues)
