# RX5808 Diversity Receiver v1.8.0 — UX Overhaul & Performance

**Release Date:** February 23, 2026  
**Release Type:** Stable  
**Commit:** fd438a8  
**Hardware Compatibility:** ESP32 D0WDQ6 v1.2, ST7735 80×160 LCD  
**Build Toolchain:** ESP-IDF v5.5.2-2

---

## Overview

v1.8.0 is a major UX and performance release. It replaces the old deep-nested menu flow with a quick-access overlay, introduces a full LED feedback system, silences distracting antenna-switch clicks, and fixes several runtime bugs that affected rendering smoothness and RSSI accuracy. All changes are backward-compatible — no settings are lost on upgrade.

---

## What's New

### Quick Action Menu
The most impactful UX change. From the main page, hold LEFT for 1 second to open an overlay menu with instant access to: Scan, Spectrum Analyzer, Calibration, Band X Editor, and Settings. What previously required 7 steps (navigate back to root, open menu, drill down, ...) now takes 2.

The lock/unlock toggle has also moved to a single long-press of the center button on the main page — no more entering the main menu just to unlock.

### LED Feedback System (new)
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

LED transitions are non-blocking. A `saved_pattern` mechanism restores the previous mode after transient events (e.g., double-blink returns to heartbeat, not OFF).

### Silent Diversity + Low-Signal Warning
Antenna switches are now silent — no beep on every switch, which was disruptive during rapid manoeuvres. Instead, a triple-beep fires only when **both** receivers fall below the noise floor simultaneously, giving a clear range warning without polluting normal flight.

### Band X Inline Frequency Editor
Custom frequencies can now be edited directly from the main page without entering a separate editor screen. Double-click the center button to enter edit mode; the active digit blinks (asymmetric: 600 ms on / 150 ms off). Hold UP/DOWN for auto-repeat with fast-scroll (×10 MHz) after 2 s. Frequency is colour-coded: green = valid FPV range, amber = extended, red = out of range. Saved with a single confirm press; a "Saved!" toast confirms persistence.

### Boot Animation Skip
Any keypress during the boot animation immediately skips it to the main page.

---

## Performance Improvements

### Dedicated CPU Cores
All hardware tasks (RSSI sampling, diversity algorithm, beep, LED, thermal fan) are now explicitly pinned to **Core 1**. LVGL runs exclusively on **Core 0**. This eliminates contention: a 50 ms PLL-settle call during channel switching no longer causes UI jank.

### No More LVGL Task Blocking
The page-switch delay on the main page used `vTaskDelay(50)` inside an LVGL event callback, stalling Core 0's rendering loop for 50 ms every single navigation. Replaced with a one-shot `lv_timer` so LVGL never blocks.

### Diversity Hot-Path: Integer Math, Fewer Syscalls
- `sqrtf(variance)` and `fabsf(slope)` in the scoring loop (called up to **200×/sec**) replaced with an integer Newton-step `isqrt()` and a branchless abs. No libm on the hot path.
- `esp_timer_get_time()` was called 4–5 times per `diversity_update()` cycle. Now snapshotted once at the top of `diversity_update()` and passed as a parameter to all helper functions.

### CPU Auto Mode
A new AUTO frequency mode dynamically scales the ESP32 between 80/160/240 MHz and enables light sleep when the interface is locked/idle. Active use returns to full speed. Reduces idle temperature noticeably.

### Other Performance Fixes
- Breathing LED pattern uses a precomputed LUT (100 entries) instead of `sinf()` at runtime.
- LEDC fan duty: `duty * 81.91f` → `duty * 8191 / 100` (pure integer).
- ADC mutex (`adc_mutex`) serialises all `adc_oneshot_read()` calls — the RSSI burst on Core 1 and the keypad reader on Core 0 no longer race the shared ADC handle.
- RSSI filter indices split: `rssi0_filter_index` and `rssi1_filter_index` are now independent so reading RX_A never advances RX_B's buffer pointer.

---

## Bug Fixes

| # | Description |
|---|---|
| 1 | Diversity bar highlight was inverted — the RX_A bar lit up when RX_B was active and vice versa. |
| 2 | Legacy simple-diversity `else` branch still ran inside the RSSI task after `diversity.c` took over GPIO, causing a dual-write conflict. |
| 3 | `printf("KEY_ADC_V: %d\n")` fired on every keypress, flooding the UART at 50 Hz. |
| 4 | `vTaskDelay(50)` inside an LVGL event callback stalled the LVGL render task for 50 ms on every page change. |
| 5 | `rssi_filter_index` was shared between both receivers; advancing it during RX_A sampling also stepped RX_B's circular buffer. |
| 6 | `vTaskDelay(0)` in the diversity task yielded without actually sleeping, repeatedly starving lower-priority tasks and causing WDT warnings. Changed to `vTaskDelay(1)`. |
| 7 | Duplicate `#include "nvs_flash.h"` on consecutive lines in `rx5808.c`. |

---

## Upgrading from v1.7.1

No settings are lost. The device will migrate automatically on first boot. The navigation layout has changed slightly (lock is now long-press center; quick menu is long-hold LEFT) but all existing features remain accessible.

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
