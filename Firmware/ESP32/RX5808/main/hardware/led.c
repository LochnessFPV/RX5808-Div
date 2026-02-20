#include "led.h"
#include "hwvers.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "LED";

// LED state
static led_pattern_t current_pattern = LED_PATTERN_OFF;
static led_pattern_t saved_pattern = LED_PATTERN_OFF;
static uint8_t signal_strength_rssi = 0;
static bool double_blink_triggered = false;
static SemaphoreHandle_t led_mutex = NULL;

// FreeRTOS task handle
static TaskHandle_t led_task_handle = NULL;

/**
 * @brief Calculate LED brightness for breathing pattern
 * 
 * @param tick Current tick value
 * @return Brightness (0-100)
 */
static uint8_t calculate_breathing_brightness(uint32_t tick)
{
    // Breathing cycle: 2 seconds (2000ms)
    // Use sine wave for smooth breathing effect
    float phase = (tick % 2000) / 2000.0f * 2.0f * M_PI;
    float brightness = (sinf(phase) + 1.0f) / 2.0f; // 0.0 to 1.0
    return (uint8_t)(brightness * 100);
}

/**
 * @brief Calculate LED brightness for heartbeat pattern
 * 
 * @param tick Current tick value
 * @return Brightness (0-100), or 0 for off periods
 */
static uint8_t calculate_heartbeat_brightness(uint32_t tick)
{
    // Heartbeat cycle: 2 seconds
    // Two quick pulses, then pause
    uint32_t cycle_pos = tick % 2000;
    
    if (cycle_pos < 100) {
        // First pulse (0-100ms)
        return 100;
    } else if (cycle_pos < 200) {
        // First fade (100-200ms)
        return 100 - ((cycle_pos - 100) * 100 / 100);
    } else if (cycle_pos < 350) {
        // Short pause (200-350ms)
        return 0;
    } else if (cycle_pos < 450) {
        // Second pulse (350-450ms)
        return 100;
    } else if (cycle_pos < 550) {
        // Second fade (450-550ms)
        return 100 - ((cycle_pos - 450) * 100 / 100);
    } else {
        // Long pause (550-2000ms)
        return 0;
    }
}

/**
 * @brief LED control task
 * 
 * Runs continuously, updates LED based on current pattern.
 * 
 * @param param Unused
 */
static void led_task(void *param)
{
    uint32_t tick = 0;
    uint8_t brightness = 0;
    bool led_state = false;
    uint8_t double_blink_count = 0;
    
    while (1) {
        // Check for double blink trigger
        if (xSemaphoreTake(led_mutex, 0) == pdTRUE) {
            if (double_blink_triggered) {
                double_blink_count = 4; // 2 blinks = 4 transitions (on-off-on-off)
                double_blink_triggered = false;
            }
            xSemaphoreGive(led_mutex);
        }
        
        // Handle double blink (overrides other patterns)
        if (double_blink_count > 0) {
            led_state = !led_state;
            gpio_set_level(LED0_PIN, led_state ? 1 : 0);
            double_blink_count--;
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }
        
        // Handle normal patterns
        switch (current_pattern) {
            case LED_PATTERN_OFF:
                gpio_set_level(LED0_PIN, 0);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                break;
                
            case LED_PATTERN_SOLID:
                gpio_set_level(LED0_PIN, 1);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                break;
                
            case LED_PATTERN_HEARTBEAT:
                brightness = calculate_heartbeat_brightness(tick);
                gpio_set_level(LED0_PIN, brightness > 50 ? 1 : 0);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                tick += 10;
                break;
                
            case LED_PATTERN_FAST_BLINK:
                // 5 Hz = 200ms period = 100ms on, 100ms off
                led_state = (tick % 200) < 100;
                gpio_set_level(LED0_PIN, led_state ? 1 : 0);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                tick += 10;
                break;
                
            case LED_PATTERN_BREATHING:
                brightness = calculate_breathing_brightness(tick);
                // Simple on/off approximation (no PWM)
                // LED is on for brightness% of each 20ms period
                if (brightness > ((tick % 20) * 5)) {
                    gpio_set_level(LED0_PIN, 1);
                } else {
                    gpio_set_level(LED0_PIN, 0);
                }
                vTaskDelay(2 / portTICK_PERIOD_MS);
                tick += 2;
                break;
                
            case LED_PATTERN_SIGNAL_STRENGTH:
                // LED brightness proportional to signal strength
                // Simple on/off approximation
                if (signal_strength_rssi > ((tick % 20) * 5)) {
                    gpio_set_level(LED0_PIN, 1);
                } else {
                    gpio_set_level(LED0_PIN, 0);
                }
                vTaskDelay(2 / portTICK_PERIOD_MS);
                tick += 2;
                break;
                
            case LED_PATTERN_DOUBLE_BLINK:
                // Handled above
                vTaskDelay(10 / portTICK_PERIOD_MS);
                break;
                
            default:
                gpio_set_level(LED0_PIN, 0);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                break;
        }
    }
}

void led_init(void)
{
    ESP_LOGI(TAG, "Initializing LED module on GPIO %d", LED0_PIN);
    
    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED0_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Start with LED off
    gpio_set_level(LED0_PIN, 0);
    
    // Create mutex
    led_mutex = xSemaphoreCreateMutex();
    if (led_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create LED mutex");
        return;
    }
    
    // Create LED task
    xTaskCreatePinnedToCore(
        led_task,
        "led_task",
        2048,
        NULL,
        5,  // Priority 5 (below GUI, above idle)
        &led_task_handle,
        1   // Core 1 (RF tasks)
    );
    
    if (led_task_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create LED task");
        return;
    }
    
    ESP_LOGI(TAG, "LED module initialized successfully");
}

void led_set_pattern(led_pattern_t pattern)
{
    if (led_mutex != NULL) {
        xSemaphoreTake(led_mutex, portMAX_DELAY);
        current_pattern = pattern;
        ESP_LOGD(TAG, "LED pattern changed to: %d", pattern);
        xSemaphoreGive(led_mutex);
    } else {
        current_pattern = pattern;
    }
}

led_pattern_t led_get_pattern(void)
{
    led_pattern_t pattern;
    if (led_mutex != NULL) {
        xSemaphoreTake(led_mutex, portMAX_DELAY);
        pattern = current_pattern;
        xSemaphoreGive(led_mutex);
    } else {
        pattern = current_pattern;
    }
    return pattern;
}

void led_set_signal_strength(uint8_t rssi_percent)
{
    if (rssi_percent > 100) {
        rssi_percent = 100;
    }
    
    if (led_mutex != NULL) {
        xSemaphoreTake(led_mutex, portMAX_DELAY);
        signal_strength_rssi = rssi_percent;
        xSemaphoreGive(led_mutex);
    } else {
        signal_strength_rssi = rssi_percent;
    }
}

void led_trigger_double_blink(void)
{
    if (led_mutex != NULL) {
        xSemaphoreTake(led_mutex, portMAX_DELAY);
        saved_pattern = current_pattern;
        double_blink_triggered = true;
        ESP_LOGD(TAG, "Double blink triggered");
        xSemaphoreGive(led_mutex);
    }
}
