# RX5808 RSSI Calibration & Performance Optimization Guide

## 📊 Part 1: RSSI Calibration Complete Guide

### **What is RSSI Calibration?**

RSSI (Received Signal Strength Indicator) calibration ensures accurate signal strength readings from both RX5808 receiver modules. Proper calibration gives you:
- Accurate signal strength bars
- Better diversity switching
- Reliable "best antenna" selection

### **When to Calibrate:**

- ✅ First time setup
- ✅ After replacing antennas
- ✅ After hardware modifications
- ✅ If diversity switching seems wrong
- ✅ If RSSI bars are always full or empty

---

## 🔧 RSSI Calibration Procedure

### **Requirements:**
- RX5808 powered on and working
- VTX transmitting (your drone or test transmitter)
- Access to RX5808 menu (navigation buttons working)
- VTX should be 3-5 meters away

### **Step-by-Step Calibration:**

#### **Step 1: Prepare Environment**

1. **Power on your VTX** (drone or bench transmitter)
2. Set VTX to known frequency (e.g., 5805 MHz, Band A Ch4)
3. Place VTX **3-5 meters** away from RX5808
4. VTX should transmit at **medium power** (25-200mW)
5. Use omnidirectional antennas (patch/directional antennas are not ideal)

#### **Step 2: Navigate to Calibration**

On RX5808 display:
1. **Short press** ENTER button → Main menu opens
2. Use **UP/DOWN** to navigate to **"Scan"** menu
3. Press **ENTER** to open Scan submenu
4. Use **UP/DOWN** to select **"RSSI Calibration"**
5. Press **ENTER** to start calibration

#### **Step 3: Min Value Calibration (No Signal)**

The display will show: **"CALIBRATING MIN..."**

1. **Turn OFF your VTX** (or move it very far away)
2. Wait for RX5808 to detect no signal
3. System will record minimum RSSI values (noise floor)
4. Wait for ~5 seconds
5. Display shows: **"MIN: [value0] [value1]"**

**Expected MIN values:** 100-400 (low = no signal detected)

#### **Step 4: Max Value Calibration (Strong Signal)**

The display will show: **"CALIBRATING MAX..."**

1. **Turn ON your VTX** (medium power, 3-5m away)
2. Ensure RX5808 is tuned to correct frequency
3. Wait for signal lock (video should appear)
4. System will record maximum RSSI values
5. Wait for ~5 seconds
6. Display shows: **"MAX: [value0] [value1]"**

**Expected MAX values:** 2500-4000 (high = strong signal)

#### **Step 5: Verify Calibration**

After calibration:
1. System returns to main screen
2. Check RSSI bars (should show reasonable signal)
3. Move VTX closer/farther → RSSI bars should change
4. Cover antenna → RSSI should drop
5. If calibration is good, **values are saved automatically**

#### **Step 6: Calibration Failed?**

If you see **"CALIBRATION FAILED"**:

**Reasons:**
- ❌ No VTX signal detected
- ❌ VTX on wrong frequency
- ❌ Signal too weak (VTX too far or low power)
- ❌ Signal too strong (VTX too close)
- ❌ Range too small (MIN and MAX values too similar)

**Fix:**
1. Verify VTX is transmitting
2. Check frequency match
3. Adjust distance (3-5m is optimal)
4. Try again

---

## 🎛️ Manual RSSI Adjustment (Advanced)

If calibration keeps failing, you can manually set values via serial monitor:

### **Find Current Values:**

```powershell
idf.py -p COM4 monitor
```

The firmware logs RSSI values. Look for:
```
RSSI0: 234 (12%)
RSSI1: 456 (23%)
```

### **Set Manual Values:**

You can set via code in [rx5808_config.c](main/hardware/rx5808_config.c) or add settings UI:

**Typical good values:**
```c
Min0: 200-400   // Noise floor receiver 0
Max0: 2800-3800 // Strong signal receiver 0
Min1: 200-400   // Noise floor receiver 1
Max1: 2800-3800 // Strong signal receiver 1
```

**Edit values** in EEPROM or via menu system.

---

## 📈 Part 2: Performance Optimization

### **1. Diversity Switching Optimization**

#### **Current Issue:**
Default firmware may switch antennas too aggressively or too slowly.

#### **Improvement: Add Hysteresis**

Edit [rx5808.c](main/hardware/rx5808.c) around the diversity switching logic:

```c
// Add hysteresis to prevent rapid switching
#define DIVERSITY_HYSTERESIS 5  // 5% hysteresis

float rssi0_percent = Rx5808_Get_Precentage0();
float rssi1_percent = Rx5808_Get_Precentage1();

// Only switch if difference is significant
if (rssi0_percent > rssi1_percent + DIVERSITY_HYSTERESIS) {
    // Switch to antenna 0
    gpio_set_level(RX5808_SWITCH0, 0);
    gpio_set_level(RX5808_SWITCH1, 1);
} else if (rssi1_percent > rssi0_percent + DIVERSITY_HYSTERESIS) {
    // Switch to antenna 1
    gpio_set_level(RX5808_SWITCH0, 1);
    gpio_set_level(RX5808_SWITCH1, 0);
}
// else: keep current antenna (no switching)
```

**Benefit:** Reduces annoying flickering between antennas on similar signals.

---

### **2. Faster Channel Switching**

#### **Current Issue:**
Channel changes take ~150ms due to RX5808 settling time.

#### **Improvement: Reduce Delays**

Edit [rx5808.c](main/hardware/rx5808.c) in `RX5808_Set_Freq()`:

```c
// Reduce delay after frequency change
vTaskDelay(50 / portTICK_PERIOD_MS);  // Was 150ms, now 50ms
```

**Benefit:** 3x faster channel switching! ⚡

**Warning:** Some RX5808 modules need full 150ms. Test thoroughly!

---

### **3. RSSI Filtering/Smoothing**

#### **Current Issue:**
Raw ADC readings can be noisy, causing wobbly RSSI bars.

#### **Improvement: Moving Average Filter**

Add to [rx5808.c](main/hardware/rx5808.c):

```c
#define RSSI_FILTER_SIZE 8

static uint16_t rssi0_buffer[RSSI_FILTER_SIZE] = {0};
static uint16_t rssi1_buffer[RSSI_FILTER_SIZE] = {0};
static uint8_t rssi_index = 0;

uint16_t filter_rssi(uint16_t new_value, uint16_t *buffer) {
    buffer[rssi_index] = new_value;
    rssi_index = (rssi_index + 1) % RSSI_FILTER_SIZE;
    
    uint32_t sum = 0;
    for(int i = 0; i < RSSI_FILTER_SIZE; i++) {
        sum += buffer[i];
    }
    return sum / RSSI_FILTER_SIZE;
}

// In RSSI reading function:
uint16_t raw_rssi0 = adc1_get_raw(RX5808_RSSI0_CHAN);
uint16_t filtered_rssi0 = filter_rssi(raw_rssi0, rssi0_buffer);
```

**Benefit:** Smooth, stable RSSI readings without jitter! 📊

---

### **4. Auto-Scan and Lock**

#### **Feature: Auto Find Best Channel**

Add a feature to automatically scan all channels and lock to the strongest:

```c
void rx5808_auto_scan(void) {
    uint16_t best_freq = 5800;
    float best_rssi = 0;
    
    // Scan all bands
    for(uint8_t band = 0; band < 6; band++) {
        for(uint8_t ch = 0; ch < 8; ch++) {
            uint16_t freq = Rx5808_Freq[band][ch];
            RX5808_Set_Freq(freq);
            vTaskDelay(100 / portTICK_PERIOD_MS);  // Wait for lock
            
            float rssi = (Rx5808_Get_Precentage0() + Rx5808_Get_Precentage1()) / 2;
            if(rssi > best_rssi) {
                best_rssi = rssi;
                best_freq = freq;
            }
        }
    }
    
    // Lock to best frequency
    RX5808_Set_Freq(best_freq);
    printf("Auto-scan: Best freq = %d MHz (RSSI: %.1f%%)\n", best_freq, best_rssi);
}
```

**Benefit:** One-button "find my quad" feature! 🔍

---

### **5. Optimize ADC Sampling**

#### **Current:** Single ADC read per RSSI check

#### **Improvement: Multi-sample and average**

```c
uint16_t read_rssi_multi_sample(adc1_channel_t channel) {
    uint32_t sum = 0;
    for(int i = 0; i < 4; i++) {
        sum += adc1_get_raw(channel);
    }
    return sum / 4;
}
```

**Benefit:** More accurate RSSI readings, less ADC noise! 📡

---

### **6. ExpressLRS Backpack Optimization**

#### **Reduce Latency:**

Edit [elrs_backpack.c](main/driver/elrs_backpack.c):

```c
// Increase task priority for faster response
xTaskCreate(elrs_backpack_task, 
            "elrs_backpack", 
            4096, 
            NULL, 
            15,  // Higher priority (was 10)
            NULL);
```

**Benefit:** Faster channel switching response! ⚡

---

### **7. Display Optimization**

#### **Reduce LVGL Update Rate:**

Edit [main.c](main/main.c):

```c
while(1) {
    lv_task_handler();
    vTaskDelay(20 / portTICK_PERIOD_MS);  // Was 10ms, now 20ms
}
```

**Benefit:** Lower CPU usage, more resources for diversity switching! 💪

---

### **8. Battery Monitoring Enhancement**

#### **Add Low Battery Warning:**

```c
#define LOW_BATTERY_THRESHOLD 3.2  // Volts

void check_battery(void) {
    float voltage = Get_Battery_Voltage();
    if(voltage < LOW_BATTERY_THRESHOLD && voltage > 2.0) {
        // Flash LED or beep
        Beep_On();
        vTaskDelay(100 / portTICK_PERIOD_MS);
        Beep_Off();
    }
}
```

**Benefit:** Never run out of power during a race! 🔋

---

## 🚀 Quick Performance Summary

| Optimization | Benefit | Difficulty | Impact |
|--------------|---------|------------|--------|
| **Diversity Hysteresis** | Stable switching | Easy ⭐ | High 🔥 |
| **Faster Channel Switch** | 3x speed | Easy ⭐ | Medium 💪 |
| **RSSI Filtering** | Smooth bars | Medium ⭐⭐ | High 🔥 |
| **Auto-Scan** | Find best channel | Medium ⭐⭐ | High 🔥 |
| **Multi-Sample ADC** | Accurate RSSI | Easy ⭐ | Medium 💪 |
| **Backpack Priority** | Faster response | Easy ⭐ | Low ⚡ |
| **Display Optimization** | Lower CPU usage | Easy ⭐ | Medium 💪 |
| **Battery Warning** | Safety feature | Easy ⭐ | Medium 💪 |

---

## 📝 Calibration Checklist

Print and follow:

```
□ VTX transmitting on known frequency
□ VTX 3-5 meters away, medium power
□ Navigate to Scan → RSSI Calibration
□ Turn OFF VTX for MIN calibration
□ Turn ON VTX for MAX calibration
□ Verify RSSI bars respond to signal changes
□ Calibration saved automatically
□ Test diversity switching (cover antennas)
```

---

## 🎯 Expected Results After Optimization

- ✅ Smooth RSSI bars (no jitter)
- ✅ Fast channel switching (<100ms)
- ✅ Intelligent diversity switching
- ✅ Accurate signal readings
- ✅ Better video quality
- ✅ Lower CPU usage
- ✅ Wireless control working

---

**Your RX5808 is now fully calibrated and optimized!** 🏆📡
