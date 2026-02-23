# Build Warning Investigation Report

This document records the investigation and resolution of all build warnings in the ESP32 RX5808 firmware.

---

## Fixed Warnings

### 1. `driver/periph_ctrl.h` deprecated include
**File:** `components/esp32-video/include/video_out.h` line 47  
**Warning:** `This file is deprecated, please use esp_private/periph_ctrl.h instead`  
**Root Cause:** ESP-IDF 5.x moved `periph_module_enable/disable` to a private header. The old path emits a `#warning` directive.  
**Fix Applied:** Changed `#include "driver/periph_ctrl.h"` → `#include "esp_private/periph_ctrl.h"`.  
All users of the header (`periph_module_enable`, `periph_module_disable`) are unaffected — the functions are identical, only the include path changed.

---

### 2. `lldesc_t _dma_desc[4] = {0}` missing field initializers
**File:** `components/esp32-video/include/video_out.h` line 62  
**Warning:** `missing field initializer` (C++ aggregate init with `{0}`)  
**Root Cause:** In C++, `= {0}` initialises only the first member explicitly; GCC warns the remaining fields are implicitly zeroed. The intent is to zero the whole struct.  
**Fix Applied:** Changed `= {0}` to `= {}`. In C++ `= {}` performs value-initialization on all array elements (zero-ini for POD types) without ambiguity.

---

### 3. IRAM_ATTR on `video_isr` forward declaration
**File:** `components/esp32-video/include/video_out.h` line 65  
**Warning:** `section of 'video_isr' changed to '.iram1.N'` conflicting with earlier `.iram1.M`  
**Root Cause:** The C++ compiler assigns a unique linker section suffix counter each time `IRAM_ATTR` appears. The forward declaration gets `.iram1.2` and the definition later in the same TU gets `.iram1.16`, triggering a conflict warning.  
**Fix Applied:** Removed `IRAM_ATTR` from the forward declaration only. The definition retains `IRAM_ATTR`, which is what matters for placement. The extern "C" linkage is preserved.

---

### 4. `DAC_CHANNEL_1` deprecated
**File:** `components/esp32-video/include/video_out.h` lines 156 and 305  
**Warning:** `'DAC_CHANNEL_1' is deprecated`  
**Root Cause:** ESP-IDF 5.x renamed the DAC channel enum. `DAC_CHANNEL_1` (GPIO25, first DAC channel) was renamed to `DAC_CHAN_0`. Both have the integer value `0`.  
**Fix Applied:** Replaced `DAC_CHANNEL_1` with `DAC_CHAN_0` in both call sites (`dac_output_enable` and `dac_output_disable`). Behaviour is identical.

---

### 5. `dst[2^1]` / `dst[10^1]` — misleading use of XOR operator
**File:** `components/esp32-video/include/video_out.h` lines 389, 417, 456, 467, 581, 592  
**Warning:** `result of '2^1' is 3; did you mean '1 << 1' (2)?` (`-Wxor-used-as-pow`)  
**Root Cause:** The entire `^1` XOR pattern across the blit functions is an intentional byte-swap trick for 16-bit DMA endianness: array index `i^1` swaps adjacent even/odd uint16 slots. GCC fires `-Wxor-used-as-pow` specifically when it sees a decimal integer constant combined with `^` that looks like "power" notation (e.g. `2^1`, `10^1`). The parenthesized form `(2^1)` still triggers the warning; only a hex literal suppresses it, because GCC's heuristic ignores hex.  
**Fix Applied:** Changed the decimal constants to hex: `dst[(2^1)]` → `dst[(0x2^1)]`, `dst[(10^1)]` → `dst[(0xa^1)]`. Result values are unchanged (`0x2^1 = 3`, `0xa^1 = 11`). This exactly follows the compiler's own suggestion in the diagnostic note.  
The byte-swap intent is preserved and now unambiguous.

---

### 6. Implicit fall-through in `blit()` switch
**File:** `components/esp32-video/include/video_out.h` (inside `blit()`)  
**Warning:** `this statement may fall through` (GCC -Wimplicit-fallthrough)  
**Root Cause:** `case EMU_NES:` sets `mask = 0x3F;` then falls through to `case EMU_SMS:` for the shared pixel-blitting loop. This is intentional — NES and SMS share the same output path with different masks.  
**Fix Applied:** Added `[[fallthrough]];` (C++17 attribute) between the two cases to explicitly annotate the intentional fall-through.

---

### 7. `volatile int` post-increment deprecated in C++
**File:** `components/esp32-video/include/video_out.h` inside `video_isr()`  
**Warning:** `increment of object of volatile-qualified type ... is deprecated` (C++20)  
**Root Cause:** In C++20, applying `++` to a `volatile`-qualified integer is deprecated because the semantics of the result value being discarded vs. used are ambiguous regarding volatile ordering.  
- `int i = _line_counter++;` — post-increment where old value is captured  
- `_frame_counter++;` — standalone increment  

**Fix Applied:**
```cpp
// Before:
int i = _line_counter++;
// After:
int i = _line_counter;
_line_counter = _line_counter + 1;

// Before:
_frame_counter++;
// After:
_frame_counter = _frame_counter + 1;
```
Both forms generate identical machine code on Xtensa; the explicit form unambiguously reads then writes the volatile variable.

---

### 8. `(long long)` cast in `bitmap.c` for pointer alignment
**File:** `components/esp32-video/bitmap.c` line 63  
**Warning:** `cast from pointer to integer of different size` (on 32-bit targets `long long` = 64-bit, pointer = 32-bit)  
**Root Cause:** The alignment trick `(((long long)ptr + 3) & ~3)` casts a pointer to `long long` to do integer arithmetic and round up to a 4-byte boundary. On a 32-bit platform `long long` (64-bit) is wider than a pointer (32-bit), causing a size-mismatch warning.  
**Fix Applied:** Replaced `(long long)` with `(uintptr_t)`. `uintptr_t` is defined by the C standard as an unsigned integer type guaranteed to be exactly the same width as a pointer, making this cast lossless and portable without warnings.

---

### 9. `esp_spi_flash.h` deprecated in `main.c`
**File:** `main/main.c` line 6  
**Warning:** `esp_spi_flash.h has been deprecated...`  
**Root Cause:** ESP-IDF 5.x renamed/reorganised the SPI flash map API. The legacy `esp_spi_flash.h` header emits a deprecation warning.  
**Fix Applied:** Changed `#include "esp_spi_flash.h"` → `#include "spi_flash_mmap.h"` (the new header). The firmware does not actually call any SPI flash functions directly from `main.c`, so this was purely an unnecessary leftover include.

---

### 10. `(void*)user_data` int-to-pointer cast in `lvgl_stl.c`
**File:** `main/gui/lvgl_app/lvgl_stl.c` lines ~49 and 191  
**Warning:** `cast to pointer from integer of different size` (line 49); `cast from pointer to integer of different size` (line 191)  
**Root Cause:** `lv_anim` stores opaque data in a `void*` field (`user_data`) but the code stores small integers in it and retrieves them. Casting an integer directly to/from `void*` triggers pointer-size warnings on 32-bit targets.  
**Fix Applied (both lines):** Used the two-step idiom `(void*)(uintptr_t)value` (storing) and `(uint8_t)(uintptr_t)a->user_data` (reading). First widening/narrowing through `uintptr_t` (exact pointer size), then casting to the target type. This is the standard-compliant way to store a small integer in a pointer-sized slot.

---

### 11. `instruction_label` unused variable in `page_drone_finder.c`
**File:** `main/gui/lvgl_app/page_drone_finder.c` line 17  
**Warning:** `'instruction_label' declared but unused`  
**Root Cause:** A `static lv_obj_t*` was declared for an instruction label widget but never assigned or referenced anywhere in the file. The widget was never created.  
**Fix Applied:** Removed the unused declaration. If an instruction label is needed in the future it can be added back when implemented.

---

### 12. `cast-function-type` on `lv_obj_set_x` in `page_menu.c`
**File:** `main/gui/lvgl_app/page_menu.c` line ~171  
**Warning:** `cast between incompatible function types from 'void (*)(lv_obj_t*, lv_coord_t)' to 'lv_anim_exec_xcb_t'`  
**Root Cause:** `lv_anim_exec_xcb_t` is `void (*)(void*, int32_t)`. `lv_obj_set_x` is `void (lv_obj_t*, lv_coord_t)`. Even though the ABI layouts are identical on ESP32 (both parameters are 32-bit), the C standard considers calling through an incompatible function pointer undefined behaviour, and GCC correctly warns.  
**Fix Applied:** Added a thin wrapper function:
```c
static void menu_obj_set_x(void *obj, int32_t x) {
    lv_obj_set_x((lv_obj_t *)obj, (lv_coord_t)x);
}
```
The wrapper has the exact signature expected by `lv_anim_exec_xcb_t` and contains explicit safe casts. The cast in the `lv_amin_start` call was removed.

---

### 13. `IRAM_ATTR` on `esp32_video_set_color` declaration in `capi_video.h`
**File:** `components/esp32-video/include/capi_video.h` line 7  
**Warning:** `ignoring attribute 'section (.iram1.16)' because it conflicts with previous 'section (.iram1.0)'`  
**Root Cause:** `IRAM_ATTR` on both the `extern "C"` declaration in the header and the definition in `capi_video.cpp` causes the compiler to assign two different `.iram1.N` section numbers within the same translation unit that includes both.  
**Fix Applied:** Removed `IRAM_ATTR` from the declaration in `capi_video.h`. The definition in `capi_video.cpp` retains `IRAM_ATTR`, which is what the linker uses for placement.

---

### 14. `IRAM_ATTR` section conflicts in `lv_port_disp.c`
**File:** `main/gui/lvgl_driver/lv_port_disp.c` lines 47–48  
**Warning:** `ignoring attribute 'section (.iram1.5/.iram1.6)' because it conflicts with previous 'section (.iram1.2/.iram1.1)'` on `composite_monitor_cb` and `composite_rounder_cb`  
**Root Cause:** Same as warnings #3 and #13 — `IRAM_ATTR` on both forward declaration and definition causes duplicate section-number assignment within the same translation unit.  
**Fix Applied:** Removed `IRAM_ATTR` from both forward declarations (lines 47–48). Definitions at lines 192 and 203 retain `IRAM_ATTR`.

---

### 15. `cast-function-type` on LVGL animation callbacks (7 files)
**Files:** `page_scan_chart.c`, `page_about.c`, `page_scan.c`, `page_scan_table.c`, `page_start.c`, `page_scan_calib.c`, `page_setup.c`  
**Warning:** `cast between incompatible function types from 'void (*)(lv_obj_t*, lv_coord_t)' to 'void (*)(void*, int32_t)'`  
**Root Cause:** `lv_anim_exec_xcb_t` is `void (*)(void*, int32_t)`. LVGL geometry helpers (`lv_obj_set_x/y/height`, `lv_obj_opa_cb`, `lv_bar_set_value`) have slightly different argument types (`lv_coord_t` = `short int`; `uint8_t`; or an extra argument). Casting through `(lv_anim_exec_xcb_t)` is technically UB per C strict aliasing rules, even though the ABI representation happens to work on ESP32.  
**Fix Applied:** Created `main/gui/lvgl_app/lv_anim_helpers.h` with `static inline` wrapper functions:
```c
static inline void anim_set_x_cb(void *obj, int32_t v)      { lv_obj_set_x(...); }
static inline void anim_set_y_cb(void *obj, int32_t v)      { lv_obj_set_y(...); }
static inline void anim_set_height_cb(void *obj, int32_t v) { lv_obj_set_height(...); }
static inline void anim_opa_cb(void *obj, int32_t v)        { lv_obj_opa_cb(...); }
static inline void anim_bar_value_cb(void *obj, int32_t v)  { lv_bar_set_value(..., LV_ANIM_OFF); }
```
Each wrapper has the exact `lv_anim_exec_xcb_t` signature and contains the safe casts internally. All 7 files now `#include "lv_anim_helpers.h"` and use the wrappers instead of the cast expressions.  
Note: `page_main.c`, `page_spectrum.c`, and `page_bandx_channel_select.c` already suppressed this warning with `#pragma GCC diagnostic ignored "-Wcast-function-type"` — those are left in place as retroactively equivalent.

---

### 16. Unused `ret` variables in `24cxx.c`
**File:** `main/hardware/24cxx.c` lines 29, 45, 65, 89  
**Warning:** `unused variable 'ret'`  
**Root Cause:** Each I2C transaction function stored the return value of `i2c_master_cmd_begin()` in a local `esp_err_t ret`, but the subsequent `ESP_ERROR_CHECK(ret)` was commented out, leaving `ret` set but never read.  
**Fix Applied:** Removed the `esp_err_t ret =` assignment and replaced with `(void)i2c_master_cmd_begin(...)`. The `//ESP_ERROR_CHECK(ret)` comments are also removed. I2C errors here are non-fatal (the EEPROM is optional), consistent with the original intent of disabling the check.

---

### 17. Unused variables in `system.c`
**File:** `main/hardware/system.c` lines 66, 208  

**`freq_mhz` (line 66):** Set in the `switch` block and used inside `#ifdef CONFIG_PM_ENABLE`, but in the `#else` compile path it was never referenced, causing "set but not used". **Fix:** Added `freq_mhz` to the `#else` log message: `"PM not enabled. Current: %lu MHz (requested %lu MHz)"`. This makes the warning disappear and improves diagnostic output.

**`CPU_STACK_RunInfo` (line 208):** Declared as a 400-byte buffer for `vTaskList()` output, but all code using it is commented out. **Fix:** Added `(void)CPU_STACK_RunInfo;` immediately after the declaration to document that the buffer is intentionally reserved for when the debug logging is re-enabled.

---

### 18. `ADC_ATTEN_DB_11` deprecated in `rx5808.c`
**File:** `main/hardware/rx5808.c` lines 127, 133–136  
**Warning:** `'ADC_ATTEN_DB_11' is deprecated`  
**Root Cause:** ESP-IDF 5.x deprecated `ADC_ATTEN_DB_11`. Per the HAL source, `ADC_ATTEN_DB_11` is now an alias for `ADC_ATTEN_DB_12` with the same integer value (both select the ~2.5V full-scale attenuation on ESP32). The deprecation note says they behave identically.  
**Fix Applied:** Replaced all 5 occurrences of `ADC_ATTEN_DB_11` with `ADC_ATTEN_DB_12`. Measurement range is unchanged.

---

### 19. Unused variable `rates_ms[]` in `rx5808.c`
**File:** `main/hardware/rx5808.c` line 441  
**Warning:** `unused variable 'rates_ms'`  
**Root Cause:** The array `const uint16_t rates_ms[]` was declared alongside `rates_str[]` but only `rates_str` was actually used in the log message.  
**Fix Applied:** Removed the `rates_ms` array declaration. The numeric values were never needed at runtime; `rates_str` already provides the human-readable string.

---

### 20. `initializer-string for array of 'char' is too long` in `page_menu.c` and `page_setup.c`
**Files:** `main/gui/lvgl_app/page_menu.c` line 52; `main/gui/lvgl_app/page_setup.c` lines 64, 67, 69, 72  
**Warning:** `initializer-string for array of 'char' is too long` / `(near initialization for '...[N]')`  
**Root Cause:** Chinese UI labels are stored as UTF-8 encoded C string literals in fixed-size `char[N][K]` 2D arrays. Each Chinese character occupies 3 bytes in UTF-8, so strings like "快速扫描" (4 chars) require 12 bytes of payload plus 1 null terminator = 13 bytes total. The declared element size `[12]` leaves no room for the null terminator. Other arrays (e.g., `[][9]`, `[][10]`) have similar tight budgets where some element strings slightly exceed the declared size.  
**Fix Applied:** Increased all affected array element sizes to `[24]`, which safely accommodates any 7-character Chinese string (7×3+1 = 22 bytes) plus several bytes of margin:
- `icon_txt_array_chinese[menu_item_count][12]` → `[24]` in `page_menu.c`
- `language_label_text[][10]` → `[][24]` in `page_setup.c`
- `signal_source_label_chinese_text[][12]` → `[][24]` in `page_setup.c` and its matching `extern` in `page_menu.c`
- `diversity_mode_label_chinese_text[][9]` → `[][24]` in `page_setup.c`
- `gui_update_rate_label_chinese_text[][10]` → `[][24]` in `page_setup.c`

All usage sites use `(const char*)(&array[i])` which takes the address of the `char[K]` sub-array — this pattern works correctly regardless of element size `K` as long as the string is null-terminated within the element.

---

## Unfixable Warnings — Explanation

### A. `driver/dac.h` deprecated (legacy DAC driver)
**File:** `components/esp32-video/include/video_out.h` line 48  
**Warning:** `driver/dac.h: This header is deprecated, please use driver/dac_oneshot.h, driver/dac_continuous.h or driver/dac_cosine.h instead`  
**Status: CANNOT BE FIXED without a major rewrite**

**Investigation:**  
The video driver uses the following legacy DAC functions:
- `dac_output_enable(DAC_CHAN_0)` / `dac_output_disable(DAC_CHAN_0)` — enable/disable the DAC channel
- `dac_i2s_enable()` / `dac_i2s_disable()` — couple the DAC output to the I2S peripheral's DMA stream

The last two functions (`dac_i2s_enable`/`dac_i2s_disable`) **have no equivalent in the new ESP-IDF 5.x DAC API**. The new API (`driver/dac_continuous.h`) provides a software-facing continuous DMA interface, but the video driver bypasses the new abstraction entirely and programs the I2S peripheral and DAC registers directly for precise NTSC/PAL timing. The `dac_i2s_enable()` call is what enables the hardware path that routes the I2S DMA output to DAC1 (GPIO25), which is fundamental to how composite video is generated.

Migrating to the new API would require:
1. Rewriting the entire low-level `start_dma()` / `video_init_hw()` pipeline (~100 lines) to use `dac_continuous_config_t` and `dac_continuous_handle_t`
2. Verifying that the new API still permits the precise APLL timing control required for NTSC/PAL colour burst accuracy
3. Confirming that the new driver's DMA callback mechanism can meet the per-scanline ISR latency requirement

This is a significant effort with a risk of breaking the video signal timing. The warning is benign — `driver/dac.h` still works correctly in the current ESP-IDF version. The warning is documented here for future maintainers.

**Workaround if the warning noise is unacceptable:**  
Add to the component's `CMakeLists.txt`:
```cmake
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-deprecated-declarations)
```
This scopes the suppression to the `esp32-video` component only and does not hide deprecation warnings anywhere else in the project.

---

### B. `driver/adc.h` and `esp_adc_cal.h` deprecated (legacy ADC driver)
**Files:** `main/hardware/rx5808.c` lines 3, 5; `main/gui/lvgl_driver/lv_port_indev.c` lines 282, 283  
**Warning:** `legacy adc driver is deprecated...` / `legacy adc calibration driver is deprecated...`  
**Status: CANNOT BE FIXED without ADC driver rewrite**

**Investigation:**  
`rx5808.c` uses the legacy one-shot ADC API (`adc1_config_channel_atten`, `esp_adc_cal_characterize`, `adc1_get_raw`) to continuously read RSSI from two channels plus battery voltage and button ADC. `lv_port_indev.c` uses the same legacy API for button detection via ADC.

The new API (`esp_adc/adc_oneshot.h` + `esp_adc/adc_cali.h`) requires a refactor of the initialization and read paths to use handle-based objects. This is a non-trivial change touching RSSI measurement (critical to scanning accuracy) and input detection. Both APIs produce the same ADC samples; migrating is low risk but medium effort.

The warnings are informational only — the legacy API is fully functional in the current ESP-IDF version.

---

### C. `driver/mcpwm.h` deprecated (legacy MCPWM driver)
**File:** `main/hardware/lcd.c` line 5  
**Warning:** `legacy MCPWM driver is deprecated, please migrate to the new driver (include driver/mcpwm_prelude.h)`  
**Status: CANNOT BE FIXED without MCPWM rewrite**

**Investigation:**  
`lcd.c` uses the legacy MCPWM API (`mcpwm_config_t`, `mcpwm_init`) to generate the backlight PWM signal. The new API (`driver/mcpwm_prelude.h`) uses a completely different handle-based model. The current code is minimal and works correctly; migration is medium effort for no functional gain.

---

### D. `driver/rmt.h` deprecated (legacy RMT driver)
**File:** `main/hardware/ws2812.c` line 14  
**Warning:** `The legacy RMT driver is deprecated, please use driver/rmt_tx.h and/or driver/rmt_rx.h`  
**Status: CANNOT BE FIXED without WS2812 RMT rewrite**

**Investigation:**  
`ws2812.c` uses the legacy RMT API (`rmt_config_t`, `rmt_install_tx_end_callback`, `rmt_write_items`) to drive the WS2812/WS2812B LED. The new TX-only API (`driver/rmt_tx.h`) requires a handle-based encoder pattern. Migration effort is medium; the LED driver is non-critical functionality.
