# ESP32 Physical Pinout Reference - RX5808 Diversity Module

## 📍 ESP32 DevKit Physical Pinout

```
                           ESP32 DEVKIT V1 (38 PIN)
                    ┌─────────────────────────────┐
                    │                             │
                    │         USB PORT            │
                    │                             │
                    └─────────────────────────────┘
    
    LEFT SIDE (Top to Bottom)          RIGHT SIDE (Top to Bottom)
    ═══════════════════════════        ═══════════════════════════
    
    3V3  ● Power 3.3V                  GND  ● Ground
    GND  ● Ground                      GPIO23 ● RX5808 MOSI (SPI)
    GPIO15 ● SPI CS (LCD)              GPIO22 ● Beep Pin
    GPIO2 ● LCD DC (Display)           TXD0 ●
    GPIO4 ● RX5808 Switch 0            RXD0 ●
    GPIO16 ● UART RX (ELRS Backpack)   GPIO21 ● LED0
    GPIO17 ● UART TX (ELRS Backpack)   GPIO19 ● Beep RMT
    GPIO5 ● RX5808 CS                  GPIO18 ● RX5808 SCLK (SPI)
    GPIO18 ● (shared with SPI)         GPIO5 ● (shared)
    GPIO19 ● (shared with Beep)        GPIO17 ● (shared with UART)
    GPIO21 ● (shared with LED)         GPIO16 ● (shared with UART)
    RXD2 ●                             GPIO4 ● (shared)
    TXD2 ●                             GPIO0 ● Fan PWM / Boot
    GPIO22 ● (shared with Beep)        GPIO2 ● (shared)
    GPIO23 ● (shared with SPI)         GPIO15 ● (shared)
    3V3  ● Power 3.3V                  GND  ● Ground
    EN   ● Enable (Reset)              GPIO12 ● RX5808 Switch 1
    SVP  ● GPIO36 (ADC0/RSSI0)         GPIO14 ● SPI CLK
    SVN  ● GPIO39                      GPIO27 ● LCD Reset
    GPIO34 ● ADC6 (Key - D0WDQ6)       GPIO26 ● LCD Backlight
    GPIO35 ● ADC7 (VBAT - D0WDQ6)      GPIO25 ● (unused)
    GPIO32 ● I2C SDA (EEPROM)          GPIO33 ● I2C SCL (EEPROM)
    GPIO33 ● (shared with I2C)         GPIO32 ● (shared with I2C)
    GPIO25 ● (unused)                  GND  ● Ground
    GPIO26 ● (shared LCD Backlight)    GPIO13 ● SPI MOSI
    GPIO27 ● (shared LCD Reset)        GPIO9 ● Flash (internal)
    GPIO14 ● (shared SPI CLK)          GPIO10 ● Flash (internal)
    GPIO12 ● (shared RX5808 SW1)       GPIO11 ● Flash (internal)
    GND  ● Ground                      VIN  ● Power Input (5V)
    GPIO13 ● (shared SPI MOSI)         
    SD2  ● GPIO9 (Flash - internal)    
    SD3  ● GPIO10 (Flash - internal)   
    CMD  ● GPIO11 (Flash - internal)   
    VIN  ● Power Input 5V              

```

---

## 🎯 RX5808 Project Pin Assignments

### **SPI Interface (LCD Display - ST7789)**
| Pin | GPIO | Function | Connection |
|-----|------|----------|------------|
| MISO | GPIO12 | SPI MISO | LCD/RX5808 (shared bus) |
| MOSI | GPIO13 | SPI MOSI | LCD/RX5808 (shared bus) |
| CLK | GPIO14 | SPI Clock | LCD/RX5808 (shared bus) |
| CS | GPIO15 | Chip Select | LCD only |
| DC | GPIO2 | Data/Command | LCD control |
| RST | GPIO27 | Reset | LCD reset |
| BL | GPIO26 | Backlight | LCD backlight PWM |

### **RX5808 Receiver Module (SPI)**
| Pin | GPIO | Function | Connection |
|-----|------|----------|------------|
| SCLK | GPIO18 | SPI Clock | RX5808 clock |
| MOSI | GPIO23 | SPI Data | RX5808 data |
| CS | GPIO5 | Chip Select | RX5808 select |
| SWITCH0 | GPIO4 | Diversity SW | Antenna switch 0 |
| SWITCH1 | GPIO12 | Diversity SW | Antenna switch 1 |

### **ADC Channels (Analog Inputs)**
| Pin | GPIO | ADC Chan | Function | Version |
|-----|------|----------|----------|---------|
| SVP | GPIO36 | ADC1_CH0 | RSSI 0 | All |
| GPIO39 | GPIO39 | ADC1_CH3 | RSSI 1 | All |
| GPIO34 | GPIO34 | ADC1_CH6 | Keypad | D0WDQ6 module |
| GPIO35 | GPIO35 | ADC1_CH7 | Battery voltage | D0WDQ6 module |
| GPIO37 | GPIO37 | ADC1_CH1 | Battery voltage | Chip version |
| GPIO38 | GPIO38 | ADC1_CH2 | Keypad | Chip version |

**Note:** GPIO37/38 are not available on standard 38-pin DevKit!

### **I2C Interface (EEPROM)**
| Pin | GPIO | Function | Connection |
|-----|------|----------|------------|
| SDA | GPIO32 | I2C Data | 24Cxx EEPROM |
| SCL | GPIO33 | I2C Clock | 24Cxx EEPROM |

### **ExpressLRS Backpack (UART)**
| Pin | GPIO | Function | Connection |
|-----|------|----------|------------|
| TX | GPIO17 | UART TX | Backpack RX ← |
| RX | GPIO16 | UART RX | Backpack TX → |

### **Other Peripherals**
| Pin | GPIO | Function | Connection |
|-----|------|----------|------------|
| LED0 | GPIO21 | Status LED | Onboard LED |
| BEEP | GPIO19 | Buzzer | Beeper (RMT) |
| BEEP | GPIO22 | Buzzer Alt | Alternative beeper pin |
| FAN | GPIO0 | Fan PWM | Cooling fan control |

---

## ⚠️ Important Notes

### **Boot Mode Pins (Don't Use for Other Functions)**
- **GPIO0** - Pulled LOW on boot = Download mode. Used for fan (safe)
- **GPIO2** - Must be LOW or floating during boot (used for LCD DC - OK)
- **GPIO12** - Flash voltage selector (used for Switch1 - should be OK)
- **GPIO15** - Must be HIGH during boot (used for LCD CS - OK with pull-up)

### **Input-Only Pins (No Pull-up/Pull-down)**
- **GPIO34, GPIO35, GPIO36, GPIO39** - Input only, used for ADC (perfect for RSSI/Keys)

### **Reserved Pins (Don't Use)**
- **GPIO6-11** - Connected to internal flash (SPI flash)
- Never use these for external connections!

### **Safe to Use (Currently Unused)**
- **GPIO25** - Completely unused, available for expansion
- **GPIO1 (TXD0)** - Debug UART TX (can repurpose if careful)
- **GPIO3 (RXD0)** - Debug UART RX (can repurpose if careful)

---

## 🔌 ExpressLRS Backpack Wiring

```
ESP32 RX5808 Module              ExpressLRS Backpack
┌──────────────────┐              ┌─────────────────┐
│                  │              │                 │
│ GPIO17 (TX) ────────────────────→ RX (Receive)   │
│                  │              │                 │
│ GPIO16 (RX) ←────────────────────── TX (Transmit)│
│                  │              │                 │
│ GND ─────────────────────────────── GND           │
│                  │              │                 │
│ 3.3V ────────────────────────────→ VCC (3.3V)    │
│                  │              │                 │
└──────────────────┘              └─────────────────┘

⚠️ CRITICAL: TX connects to RX, RX connects to TX (crossover!)
```

---

## 🔍 Hardware Version Detection

### **Software vs Hardware Version:**
- **Software version**: In code (hwvers.h) - Currently 1.4.0
- **Hardware version**: Printed on PCB - Your board might be v1.2, v1.3, v1.4, etc.

### **Common Issue: v1.2 Hardware + v1.4 Software**
If your PCB says **"v1.2"** but you're running v1.4 software:
- The keypad pins/ADC channels might be different
- You may need to adjust ADC channel configuration
- Check if D0WDQ6_VER needs to be enabled/disabled

### **How to Identify Your ESP32 Variant:**

1. **Count the ADC pins on your ESP32 chip:**
   - **38-pin DevKit**: Standard version (no GPIO37/38)
   - **30-pin Module**: D0WDQ6 version (has GPIO34-39 accessible)

2. **Check the ESP32 chip marking:**
   - **ESP32-D0WDQ6**: Module version
   - **ESP32-WROOM-32**: Chip version (more common)

### **Configure in Code:**

Edit [hwvers.h](main/hardware/hwvers.h):

```c
// For standard 38-pin DevKit / Hardware v1.4 (most common):
//#define D0WDQ6_VER

// For D0WDQ6 module board / Some v1.2-v1.3 hardware:
#define D0WDQ6_VER
```

### **Hardware v1.2 Specific:**
If you have hardware v1.2 and keypad doesn't work:
1. Try enabling `D0WDQ6_VER` (uncomment the line)
2. Rebuild and test
3. If still not working, use the debug output to find correct ADC values

---

## 📊 Visual Pin Conflict Check

```
GPIO0  : Fan PWM                        ✅ OK (boot pin, but works for fan)
GPIO2  : LCD DC                         ✅ OK (must be low/float on boot)
GPIO4  : RX5808 Switch 0                ✅ OK
GPIO5  : RX5808 CS                      ✅ OK
GPIO12 : RX5808 Switch 1                ⚠️ OK (flash voltage pin, but safe)
GPIO13 : SPI MOSI                       ✅ OK
GPIO14 : SPI CLK                        ✅ OK
GPIO15 : LCD CS                         ✅ OK (must be high on boot)
GPIO16 : UART RX (ELRS Backpack)        ✅ OK - NEW
GPIO17 : UART TX (ELRS Backpack)        ✅ OK - NEW
GPIO18 : RX5808 SCLK                    ✅ OK
GPIO19 : Beep RMT                       ✅ OK
GPIO21 : LED0                           ✅ OK
GPIO22 : Beep                           ✅ OK
GPIO23 : RX5808 MOSI                    ✅ OK
GPIO25 : (Unused)                       ✅ Available for expansion
GPIO26 : LCD Backlight                  ✅ OK
GPIO27 : LCD Reset                      ✅ OK
GPIO32 : I2C SDA                        ✅ OK
GPIO33 : I2C SCL                        ✅ OK
GPIO34 : ADC (Keypad D0WDQ6)            ✅ OK (input only)
GPIO35 : ADC (VBAT D0WDQ6)              ✅ OK (input only)
GPIO36 : ADC (RSSI 0)                   ✅ OK (input only)
GPIO39 : ADC (RSSI 1)                   ✅ OK (input only)
```

**✅ No conflicts! GPIO16/17 (UART for backpack) are free!**

---

## 🛠️ Testing Pins

To test if a pin is working:

```c
// Set as output
gpio_set_direction(GPIO_NUM_XX, GPIO_MODE_OUTPUT);

// Toggle it
gpio_set_level(GPIO_NUM_XX, 1);  // HIGH
vTaskDelay(1000 / portTICK_PERIOD_MS);
gpio_set_level(GPIO_NUM_XX, 0);  // LOW
```

To read ADC:
```c
adc1_config_width(ADC_WIDTH_BIT_12);
adc1_config_channel_atten(ADC1_CHANNEL_X, ADC_ATTEN_DB_11);
int value = adc1_get_raw(ADC1_CHANNEL_X);
printf("ADC value: %d\n", value);
```

---

## 📖 Quick Reference Card

**Print this out and keep near your workbench:**

| Function | Pin(s) | Notes |
|----------|--------|-------|
| **ELRS Backpack** | GPIO16(RX), GPIO17(TX) | UART1, 420kbps |
| **RX5808 Module** | GPIO18,23,5,4,12 | SPI + switches |
| **LCD Display** | GPIO13,14,15,2,27,26 | SPI + control |
| **Navigation Keys** | GPIO38 or GPIO34 | ADC, depends on version |
| **EEPROM** | GPIO32,33 | I2C |
| **Status LED** | GPIO21 | Active low |
| **Beeper** | GPIO19,22 | RMT channel |
| **Available** | GPIO25 | Unused, expansion |

---

**Now you have the complete ESP32 pinout for the RX5808 project!** 📌
