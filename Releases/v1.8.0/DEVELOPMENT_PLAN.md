# RX5808-Div v1.8.0 - UX/UI Overhaul Development Plan

**Branch:** `feature/v1.8.0-ux-overhaul`  
**Target Release:** March 2026  
**Type:** Minor version (new features, UX improvements)

---

## 🎯 Goals

Fix critical UX pain points identified in v1.7.1:
1. Inconsistent navigation across menus
2. Long workflow to access features (7+ steps for scan)
3. Poor calibration tool performance (high noise floor)
4. Clunky Band X editor
5. No standard CPU/GUI defaults
6. LED provides no user feedback

---

## 📋 Implementation Phases

### **Phase 1: Standardized Navigation System** ⭐ Priority: CRITICAL
**Status:** 🔴 Not Started  
**Target:** Week 1  
**Files:**
- `main/gui/navigation.h` (NEW)
- `main/gui/navigation.c` (NEW)
- All page files (`page_*.c`)

**Tasks:**
- [ ] Create universal key handler module
- [ ] Define standard controls (OK, arrows, long-hold)
- [ ] Update all pages to use unified system
- [ ] Add consistent beep feedback
- [ ] Test on all pages

**Acceptance Criteria:**
- ✅ All pages use same navigation logic
- ✅ Long-hold OK works for lock/unlock
- ✅ LEFT always goes back
- ✅ Consistent beep patterns

---

### **Phase 2: Quick Action Menu** ⭐ Priority: HIGH
**Status:** ✅ Complete  
**Target:** Week 2  
**Files:**
- `main/gui/lvgl_app/page_main.c` ✅
- `main/gui/quick_menu.c` (NEW) ✅
- `main/gui/quick_menu.h` (NEW) ✅

**Tasks:**
- [x] Implement long-hold RIGHT detection
- [x] Create quick menu popup UI
- [x] Add menu action handlers (6 items)
- [x] Visual feedback with animations
- [x] Test menu shortcuts

**Acceptance Criteria:**
- ✅ Long-hold RIGHT opens quick menu
- ✅ UP/DOWN navigate menu items
- ✅ OK selects action, LEFT cancels
- ✅ Quick access to scan, spectrum, calibration, Band X, settings, menu
- ✅ Workflow: 2 steps vs 7 steps (50% reduction)

**Implementation Details:**
- Module: `quick_menu.c/h` (440 lines)
- UI: 200x220px popup with semi-transparent modal background
- Animations: Slide-up + fade-in (200ms)
- Long-press threshold: 1000ms
- Menu items: 6 actions with icons
- Event intercept: Blocks main page when active
- Commits: 0573f2a (module), d9e3656 (integration)

---

### **Phase 3: Menu Reorganization** ⭐ Priority: HIGH
**Status:** 🔴 Not Started  
**Target:** Week 4  
**Files:**
- `main/gui/lvgl_app/page_menu.c`
- `main/gui/lvgl_app/page_menu.h`
- `main/gui/lvgl_app/page_scan.c`

**Tasks:**
- [ ] Remove "Receiver" submenu
- [ ] Flatten to 7 main menu items
- [ ] Update navigation paths
- [ ] Test all menu workflows

**Acceptance Criteria:**
- ✅ 7-item flat menu
- ✅ Quick Scan at top level
- ✅ Calibration at top level
- ✅ No nested submenus

---

### **Phase 4: Calibration Consolidation** ⭐ Priority: MEDIUM
**Status:** 🔴 Not Started  
**Target:** Week 6  
**Files:**
- `main/gui/lvgl_app/page_diversity_calib.c`
- `main/gui/lvgl_app/page_scan_calib.c`
- `main/hardware/diversity.c`

**Tasks:**
- [ ] Fix noise floor detection (median + MAD)
- [ ] Improve classic calibration (sustained signal)
- [ ] Create unified calibration UI with tabs
- [ ] Test with real hardware (VTX off/on)

**Acceptance Criteria:**
- ✅ Noise floor accuracy >95%
- ✅ Single-channel detection >90%
- ✅ No false positives
- ✅ Clear instructions

---

### **Phase 5: Band X Editor Improvements** ⭐ Priority: MEDIUM
**Status:** 🔴 Not Started  
**Target:** Week 7  
**Files:**
- `main/gui/lvgl_app/page_bandx_channel_select.c`
- `main/gui/lvgl_app/page_bandx_channel_select.h`

**Tasks:**
- [ ] Streamlined frequency adjustment
- [ ] Auto-repeat for fast changes
- [ ] Visual range validation (5645-5945 MHz)
- [ ] Real-time RSSI preview

**Acceptance Criteria:**
- ✅ Easy UP/DOWN channel selection
- ✅ LEFT/RIGHT frequency adjust
- ✅ Hold for 10 MHz/sec
- ✅ Green ✓ / Red ⚠️ indicators

---

### **Phase 6: Setup Page Defaults** ⭐ Priority: LOW
**Status:** 🔴 Not Started  
**Target:** Week 7  
**Files:**
- `main/gui/lvgl_app/page_setup.c`
- `main/rx5808_config.c`
- `main/rx5808_config.h`

**Tasks:**
- [ ] Implement AUTO CPU mode
- [ ] Set default values (160 MHz, 14 Hz)
- [ ] Add restore defaults function
- [ ] Update UI layout

**Acceptance Criteria:**
- ✅ AUTO CPU mode working
- ✅ Defaults: 160 MHz CPU, 14 Hz GUI
- ✅ Restore defaults confirmation

---

### **Phase 7: LED Status Feedback** ⭐ Priority: MEDIUM
**Status:** 🔴 Not Started  
**Target:** Week 5  
**Files:**
- `main/hardware/led.c` (NEW)
- `main/hardware/led.h` (NEW)
- `main/gui/lvgl_app/page_main.c`

**Tasks:**
- [ ] Create LED task module
- [ ] Implement patterns (heartbeat, scanning, etc.)
- [ ] Integrate with lock state
- [ ] Signal strength indicators

**Acceptance Criteria:**
- ✅ Locked: Solid LED
- ✅ Unlocked: Heartbeat (1/2s)
- ✅ Scanning: Fast blink (5 Hz)
- ✅ Calibrating: Breathing pulse
- ✅ Signal strength indicators

---

### **Phase 8: Auto-Switch After Scan** ⭐ Priority: HIGH
**Status:** 🔴 Not Started  
**Target:** Week 3  
**Files:**
- `main/gui/lvgl_app/page_scan_table.c`
- `main/gui/lvgl_app/page_scan_chart.c`

**Tasks:**
- [ ] Find highest RSSI channel
- [ ] Show confirmation dialog
- [ ] Auto-highlight strongest
- [ ] LED activity feedback

**Acceptance Criteria:**
- ✅ Detects strongest signal
- ✅ Confirmation prompt
- ✅ Auto-highlight in table
- ✅ LED double-blink on switch

---

## 📅 Timeline

### **Week 1** (Feb 24-28)
- ✅ Create feature branch
- ✅ Setup v1.8.0 infrastructure
- 🔴 Phase 1: Navigation system

### **Week 2** (Mar 3-7)
- 🔴 Phase 2: Quick action menu
- 🔴 Testing & bug fixes

### **Week 3** (Mar 10-14)
- 🔴 Phase 8: Auto-switch after scan
- 🔴 Alpha testing

### **Week 4** (Mar 17-21)
- 🔴 Phase 3: Menu reorganization
- 🔴 Integration testing

### **Week 5** (Mar 24-28)
- 🔴 Phase 7: LED feedback
- 🔴 Beta testing

### **Week 6** (Mar 31-Apr 4)
- 🔴 Phase 4: Calibration fixes
- 🔴 Hardware testing

### **Week 7** (Apr 7-11)
- 🔴 Phase 5: Band X improvements
- 🔴 Phase 6: Setup defaults
- 🔴 Final testing & documentation

### **Week 8** (Apr 14-18)
- 🔴 Release candidate builds
- 🔴 User acceptance testing
- 🔴 Release v1.8.0

---

## 🧪 Testing Checklist

### **Per Phase:**
- [ ] Compiles without errors
- [ ] Flashes successfully
- [ ] All buttons work as expected
- [ ] No crashes or hangs
- [ ] LED behaves correctly
- [ ] Settings persist after reboot
- [ ] Existing features still work

### **Integration Testing:**
- [ ] All navigation flows
- [ ] Lock/unlock works everywhere
- [ ] Quick menu from main page
- [ ] Scan workflows
- [ ] Calibration workflows
- [ ] Band X editor
- [ ] LED patterns
- [ ] Settings persistence

### **Hardware Testing:**
- [ ] Test with real VTX
- [ ] Calibration accuracy
- [ ] Diversity switching
- [ ] RSSI readings
- [ ] Channel switching
- [ ] Band X frequencies
- [ ] LED patterns
- [ ] Beep feedback

---

## 📊 Success Metrics

**Navigation Efficiency:**
```
Action               | v1.7.1 | v1.8.0 | Improvement
---------------------|--------|--------|------------
Channel Scan         | 7 steps| 2 steps| -71%
Band Switch          | N/A    | 1 press| NEW
Channel Switch       | N/A    | 1 press| NEW
Access Spectrum      | 3 steps| 2 steps| -33%
```

**Calibration:**
- Noise floor accuracy: >95%
- Single-channel detection: >90%
- False positives: <5%

**User Feedback:**
- LED status indicators: All states
- Visual feedback: Lock icon (existing)
- Audio feedback: Consistent beeps
- Instructions: Always visible when unlocked

---

## 🚀 Release Criteria

Before merging to main:
- [ ] All phases complete
- [ ] All tests passing
- [ ] Documentation updated
- [ ] Changelog complete
- [ ] Release notes written
- [ ] Binaries built
- [ ] Web configurator updated
- [ ] User testing complete

---

## 📝 Notes

### **Breaking Changes:**
- None expected (backward compatible)

### **Migration:**
- Settings will migrate automatically
- New features opt-in (AUTO CPU mode)
- Existing workflows still work

### **Known Issues:**
- (To be filled during development)

---

**Last Updated:** February 20, 2026  
**Status:** Planning Complete, Ready for Implementation
