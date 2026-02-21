/**
 * @file diversity.c
 * @brief Advanced diversity switching algorithm implementation
 * @version 1.7.1
 */

#include "diversity.h"
#include "rx5808.h"
#include "hwvers.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <string.h>
#include <math.h>

static const char* TAG = "diversity";

// Mode parameter definitions
const diversity_mode_params_t diversity_mode_params[DIVERSITY_MODE_COUNT] = {
    // RACE mode - aggressive, minimal lag
    {
        .dwell_ms = 80,
        .cooldown_ms = 150,
        .hysteresis_pct = 2,
        .weight_rssi = 0.85f,
        .weight_stability = 0.15f,
        .slope_threshold = -30,
        .name = "Race"
    },
    // FREESTYLE mode - balanced
    {
        .dwell_ms = 250,
        .cooldown_ms = 500,
        .hysteresis_pct = 4,
        .weight_rssi = 0.70f,
        .weight_stability = 0.30f,
        .slope_threshold = -20,
        .name = "Freestyle"
    },
    // LONG_RANGE mode - stable, minimize switching
    {
        .dwell_ms = 400,
        .cooldown_ms = 800,
        .hysteresis_pct = 6,
        .weight_rssi = 0.75f,
        .weight_stability = 0.25f,
        .slope_threshold = -15,
        .name = "Long Range"
    }
};

// Global diversity state
static diversity_state_t g_diversity_state = {0};
static bool g_diversity_initialized = false;

// NVS storage keys
#define NVS_NAMESPACE "diversity"
#define NVS_KEY_CAL_A_FLOOR "cal_a_floor"
#define NVS_KEY_CAL_A_PEAK "cal_a_peak"
#define NVS_KEY_CAL_B_FLOOR "cal_b_floor"
#define NVS_KEY_CAL_B_PEAK "cal_b_peak"
#define NVS_KEY_MODE "div_mode"

// Switch rate tracking (for switches per minute)
#define SWITCH_HISTORY_SIZE 60
static uint32_t g_switch_timestamps[SWITCH_HISTORY_SIZE] = {0};
static uint8_t g_switch_history_index = 0;

/**
 * @brief Initialize diversity system
 */
void diversity_init(void) {
    ESP_LOGI(TAG, "Initializing diversity system v1.7.1");
    
    memset(&g_diversity_state, 0, sizeof(diversity_state_t));
    
    // Set defaults
    g_diversity_state.mode = DIVERSITY_MODE_FREESTYLE;
    g_diversity_state.active_rx = DIVERSITY_RX_A;
    
    // Initialize calibration defaults (uncalibrated)
    g_diversity_state.cal_a.floor_raw = 0;
    g_diversity_state.cal_a.peak_raw = 4095;
    g_diversity_state.cal_a.calibrated = false;
    
    g_diversity_state.cal_b.floor_raw = 0;
    g_diversity_state.cal_b.peak_raw = 4095;
    g_diversity_state.cal_b.calibrated = false;
    
    // Load calibration from NVS
    diversity_calibrate_load();
    
    g_diversity_initialized = true;
    
    ESP_LOGI(TAG, "Diversity initialized - Mode: %s", 
             diversity_mode_params[g_diversity_state.mode].name);
}

/**
 * @brief Set diversity mode
 */
void diversity_set_mode(diversity_mode_t mode) {
    if (mode >= DIVERSITY_MODE_COUNT) {
        ESP_LOGW(TAG, "Invalid mode %d", mode);
        return;
    }
    
    g_diversity_state.mode = mode;
    
    // Save to NVS
    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_u8(nvs_handle, NVS_KEY_MODE, (uint8_t)mode);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }
    
    ESP_LOGI(TAG, "Mode changed to: %s", diversity_mode_params[mode].name);
}

/**
 * @brief Normalize raw RSSI to 0-100 scale using calibration
 */
uint8_t diversity_normalize_rssi(uint16_t raw, rssi_calibration_t* cal) {
    // Use uncalibrated mode if calibration not set or invalid (floor >= peak)
    if (!cal->calibrated || cal->floor_raw >= cal->peak_raw) {
        // Fallback to simple 12-bit to 0-100 mapping
        return (uint8_t)((raw * 100) / 4095);
    }
    
    // Clamp to calibrated range
    if (raw <= cal->floor_raw) return 0;
    if (raw >= cal->peak_raw) return 100;
    
    // Linear interpolation
    uint32_t range = cal->peak_raw - cal->floor_raw;
    uint32_t value = raw - cal->floor_raw;
    
    return (uint8_t)((value * 100) / range);
}

/**
 * @brief Calculate statistics (mean, variance, slope) over rolling window
 */
void diversity_calculate_statistics(diversity_rx_state_t* rx) {
    if (rx->sample_count == 0) {
        return;
    }
    
    // Calculate mean
    uint32_t sum = 0;
    for (uint8_t i = 0; i < rx->sample_count; i++) {
        sum += rx->rssi_samples[i];
    }
    rx->rssi_mean = sum / rx->sample_count;
    
    // Calculate variance (using integer math, scaled by 100)
    uint32_t variance_sum = 0;
    for (uint8_t i = 0; i < rx->sample_count; i++) {
        int32_t diff = (int32_t)rx->rssi_samples[i] - (int32_t)rx->rssi_mean;
        variance_sum += (diff * diff);
    }
    rx->rssi_variance = (uint16_t)(variance_sum / rx->sample_count);
    
    // Calculate slope (rate of change)
    // Compare current mean with mean from half window ago
    if (rx->sample_count >= 10) {
        uint8_t half_window = rx->sample_count / 2;
        uint8_t old_index = (rx->sample_index + DIVERSITY_MAX_SAMPLES - half_window) % DIVERSITY_MAX_SAMPLES;
        
        // Calculate old mean
        uint32_t old_sum = 0;
        for (uint8_t i = 0; i < half_window; i++) {
            uint8_t idx = (old_index + i) % DIVERSITY_MAX_SAMPLES;
            old_sum += rx->rssi_samples[idx];
        }
        uint16_t old_mean = old_sum / half_window;
        
        // Slope is delta/time (normalized)
        rx->rssi_slope = (int16_t)rx->rssi_mean - (int16_t)old_mean;
    } else {
        rx->rssi_slope = 0;
    }
}

/**
 * @brief Calculate stability score and combined score
 */
void diversity_calculate_scores(diversity_rx_state_t* rx, const diversity_mode_params_t* params) {
    // Stability score: 100 - penalties for variance and slope
    // Higher variance = less stable, higher slope magnitude = less stable
    
    float k_variance = 0.2f;  // Penalty coefficient for variance
    float k_slope = 1.5f;     // Penalty coefficient for slope magnitude
    
    float variance_penalty = k_variance * sqrtf((float)rx->rssi_variance);
    float slope_penalty = k_slope * fabsf((float)rx->rssi_slope);
    
    int16_t stability = 100 - (int16_t)(variance_penalty + slope_penalty);
    if (stability < 0) stability = 0;
    if (stability > 100) stability = 100;
    
    rx->stability_score = (uint8_t)stability;
    
    // Combined score: weighted sum of RSSI and stability
    float score = (params->weight_rssi * rx->rssi_norm) + 
                  (params->weight_stability * rx->stability_score);
    
    rx->combined_score = (uint8_t)score;
}

/**
 * @brief Check receiver health for stuck/failed conditions
 */
void diversity_check_receiver_health(diversity_rx_state_t* rx) {
    uint32_t now = esp_timer_get_time() / 1000; // ms
    
    // Check every 1 second
    if (now - rx->health_check_ms < 1000) {
        return;
    }
    rx->health_check_ms = now;
    
    // Reset flags initially
    rx->health.stuck_high = false;
    rx->health.stuck_low = false;
    rx->health.no_variance = false;
    
    // Stuck high: RSSI > 95 for >3s with zero variance
    if (rx->rssi_norm > 95 && rx->rssi_variance < 5) {
        rx->health.stuck_high = true;
        ESP_LOGW(TAG, "Receiver health: stuck high detected");
    }
    
    // Stuck low: RSSI < 5 for >3s
    if (rx->rssi_norm < 5) {
        rx->health.stuck_low = true;
        ESP_LOGW(TAG, "Receiver health: stuck low detected");
    }
    
    // No variance: variance near zero for >5s across multiple samples
    if (rx->sample_count > 20 && rx->rssi_variance < 2) {
        rx->health.no_variance = true;
        ESP_LOGW(TAG, "Receiver health: no variance detected");
    }
}

/**
 * @brief Determine if conditions are right to switch receivers
 */
bool diversity_should_switch(diversity_state_t* state, const diversity_mode_params_t* params) {
    uint32_t now = esp_timer_get_time() / 1000; // ms
    
    // Get pointers to active and other receiver
    diversity_rx_state_t* active = (state->active_rx == DIVERSITY_RX_A) ? &state->rx_a : &state->rx_b;
    diversity_rx_state_t* other = (state->active_rx == DIVERSITY_RX_A) ? &state->rx_b : &state->rx_a;
    
    // Don't switch if other receiver is unhealthy
    if (other->health.disabled || other->health.stuck_low || other->health.no_variance) {
        return false;
    }
    
    // If active receiver is unhealthy, switch immediately
    if (active->health.disabled || active->health.stuck_low) {
        ESP_LOGI(TAG, "Switching due to active receiver health issue");
        return true;
    }
    
    // Check dwell time
    uint32_t time_since_switch = now - state->last_switch_ms;
    uint32_t required_dwell = state->in_cooldown ? params->cooldown_ms : params->dwell_ms;
    
    if (time_since_switch < required_dwell) {
        return false; // Still in dwell/cooldown period
    }
    
    // Apply hysteresis
    uint8_t hysteresis = params->hysteresis_pct;
    if (state->in_cooldown) {
        hysteresis *= 2; // Double hysteresis during cooldown
    }
    
    // Switch only if other receiver is significantly better
    if (other->combined_score > (active->combined_score + hysteresis)) {
        ESP_LOGI(TAG, "Switch: other=%d active=%d (delta=%d, threshold=%d)",
                 other->combined_score, active->combined_score,
                 other->combined_score - active->combined_score, hysteresis);
        return true;
    }
    
    // Preemptive switching: if active receiver is dropping fast
    if (active->rssi_slope < params->slope_threshold && 
        other->combined_score > active->combined_score) {
        ESP_LOGI(TAG, "Preemptive switch: slope=%d (threshold=%d)",
                 active->rssi_slope, params->slope_threshold);
        return true;
    }
    
    return false;
}

/**
 * @brief Perform receiver switch
 */
static void diversity_perform_switch(diversity_state_t* state) {
    uint32_t now = esp_timer_get_time() / 1000; // ms
    
    // Toggle active receiver
    state->active_rx = (state->active_rx == DIVERSITY_RX_A) ? DIVERSITY_RX_B : DIVERSITY_RX_A;
    
    // Update switch tracking
    state->last_switch_ms = now;
    state->switch_count++;
    state->time_stable_ms = 0;
    state->in_cooldown = true;
    
    // Record switch timestamp for rate calculation
    g_switch_timestamps[g_switch_history_index] = now;
    g_switch_history_index = (g_switch_history_index + 1) % SWITCH_HISTORY_SIZE;
    
    // Hardware antenna switching - control physical RF path
    if (state->active_rx == DIVERSITY_RX_A) {
        // RX_A: Set SWITCH1=1, SWITCH0=0
        gpio_set_level(RX5808_SWITCH1, 1);
        gpio_set_level(RX5808_SWITCH0, 0);
    } else {
        // RX_B: Set SWITCH0=1, SWITCH1=0
        gpio_set_level(RX5808_SWITCH0, 1);
        gpio_set_level(RX5808_SWITCH1, 0);
    }
    
    ESP_LOGI(TAG, "Switched to RX_%c (switches=%lu)", 
             state->active_rx == DIVERSITY_RX_A ? 'A' : 'B',
             state->switch_count);
}

/**
 * @brief Main diversity update function - call at DIVERSITY_SAMPLE_RATE_HZ
 * Implements adaptive sampling: 100Hz (10ms) during active switching, 20Hz (50ms) when stable
 */
void diversity_update(void) {
    if (!g_diversity_initialized) {
        return;
    }
    
    uint32_t now = esp_timer_get_time() / 1000; // ms
    diversity_state_t* state = &g_diversity_state;
    const diversity_mode_params_t* params = &diversity_mode_params[state->mode];
    
    // Calculate switches per second over last 5 seconds
    uint32_t window_start = now - 5000; // 5 second window
    uint32_t recent_switches = 0;
    for (uint8_t i = 0; i < SWITCH_HISTORY_SIZE; i++) {
        if (g_switch_timestamps[i] >= window_start && g_switch_timestamps[i] <= now) {
            recent_switches++;
        }
    }
    state->switches_per_second = recent_switches / 5.0f;
    
    // Adaptive sampling rate: switch between 20Hz (50ms) and 100Hz (10ms)
    // High rate (100Hz): switches_per_second >= 2 OR time_stable < 3s
    // Low rate (20Hz):  switches_per_second < 2 AND time_stable >= 3s
    uint32_t time_since_last_sample = now - state->last_sample_ms;
    
    if (state->switches_per_second >= 2.0f || state->time_stable_ms < 3000) {
        // High activity mode: 100Hz sampling (10ms intervals)
        state->adaptive_high_rate = true;
        if (time_since_last_sample < 10) {
            return; // Skip this call, not enough time elapsed
        }
    } else {
        // Low activity mode: 20Hz sampling (50ms intervals)
        state->adaptive_high_rate = false;
        if (time_since_last_sample < 50) {
            return; // Skip this call, not enough time elapsed
        }
    }
    
    // Update last sample timestamp
    state->last_sample_ms = now;
    
    // Sample raw RSSI from both receivers via ADC
    // adc_converted_value[] is populated by DMA2_Stream0_IRQHandler in rx5808.c
    state->rx_a.rssi_raw = adc_converted_value[0];  // RSSI A from RX5808
    state->rx_b.rssi_raw = adc_converted_value[1];  // RSSI B from RX5808
    
    // Normalize RSSI
    state->rx_a.rssi_norm = diversity_normalize_rssi(state->rx_a.rssi_raw, &state->cal_a);
    state->rx_b.rssi_norm = diversity_normalize_rssi(state->rx_b.rssi_raw, &state->cal_b);
    
    // Store samples in rolling windows
    state->rx_a.rssi_samples[state->rx_a.sample_index] = state->rx_a.rssi_raw;
    state->rx_a.sample_index = (state->rx_a.sample_index + 1) % DIVERSITY_MAX_SAMPLES;
    if (state->rx_a.sample_count < DIVERSITY_MAX_SAMPLES) state->rx_a.sample_count++;
    
    state->rx_b.rssi_samples[state->rx_b.sample_index] = state->rx_b.rssi_raw;
    state->rx_b.sample_index = (state->rx_b.sample_index + 1) % DIVERSITY_MAX_SAMPLES;
    if (state->rx_b.sample_count < DIVERSITY_MAX_SAMPLES) state->rx_b.sample_count++;
    
    // Calculate statistics
    diversity_calculate_statistics(&state->rx_a);
    diversity_calculate_statistics(&state->rx_b);
    
    // Calculate scores
    diversity_calculate_scores(&state->rx_a, params);
    diversity_calculate_scores(&state->rx_b, params);
    
    // Check receiver health
    diversity_check_receiver_health(&state->rx_a);
    diversity_check_receiver_health(&state->rx_b);
    
    // Update telemetry
    state->rssi_delta = (int8_t)state->rx_a.rssi_norm - (int8_t)state->rx_b.rssi_norm;
    state->time_stable_ms = now - state->last_switch_ms;
    
    // Update longest stable time
    if (state->time_stable_ms > state->longest_stable_ms) {
        state->longest_stable_ms = state->time_stable_ms;
    }
    
    // Clear cooldown flag after cooldown period
    if (state->in_cooldown && (now - state->last_switch_ms) > params->cooldown_ms) {
        state->in_cooldown = false;
    }
    
    // Decide whether to switch
    if (diversity_should_switch(state, params)) {
        diversity_perform_switch(state);
    }
}

/**
 * @brief Get current active receiver
 */
diversity_rx_t diversity_get_active_rx(void) {
    return g_diversity_state.active_rx;
}

/**
 * @brief Get pointer to diversity state (for UI/telemetry)
 */
diversity_state_t* diversity_get_state(void) {
    return &g_diversity_state;
}

/**
 * @brief Get total switch count
 */
uint32_t diversity_get_switch_count(void) {
    return g_diversity_state.switch_count;
}

/**
 * @brief Calculate switches per minute over last 60 seconds
 */
uint32_t diversity_get_switches_per_minute(void) {
    uint32_t now = esp_timer_get_time() / 1000; // ms
    uint32_t count = 0;
    
    // Count switches in last 60 seconds
    for (int i = 0; i < SWITCH_HISTORY_SIZE; i++) {
        if (g_switch_timestamps[i] > 0 && (now - g_switch_timestamps[i]) < 60000) {
            count++;
        }
    }
    
    return count;
}

/**
 * @brief Get time stable in milliseconds
 */
uint32_t diversity_get_time_stable_ms(void) {
    return g_diversity_state.time_stable_ms;
}

/**
 * @brief Reset statistics
 */
void diversity_reset_stats(void) {
    g_diversity_state.switch_count = 0;
    g_diversity_state.longest_stable_ms = 0;
    memset(g_switch_timestamps, 0, sizeof(g_switch_timestamps));
    g_switch_history_index = 0;
    ESP_LOGI(TAG, "Statistics reset");
}

// ============================================================================
// RSSI Calibration Functions
// ============================================================================

/**
 * @brief Load calibration from NVS
 */
void diversity_calibrate_load(void) {
    nvs_handle_t nvs_handle;
    
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle) != ESP_OK) {
        ESP_LOGW(TAG, "No calibration found in NVS");
        return;
    }
    
    uint16_t value;
    
    // Load RX A calibration
    if (nvs_get_u16(nvs_handle, NVS_KEY_CAL_A_FLOOR, &value) == ESP_OK) {
        g_diversity_state.cal_a.floor_raw = value;
    }
    if (nvs_get_u16(nvs_handle, NVS_KEY_CAL_A_PEAK, &value) == ESP_OK) {
        g_diversity_state.cal_a.peak_raw = value;
    }
    
    // Load RX B calibration
    if (nvs_get_u16(nvs_handle, NVS_KEY_CAL_B_FLOOR, &value) == ESP_OK) {
        g_diversity_state.cal_b.floor_raw = value;
    }
    if (nvs_get_u16(nvs_handle, NVS_KEY_CAL_B_PEAK, &value) == ESP_OK) {
        g_diversity_state.cal_b.peak_raw = value;
    }
    
    // Validate calibration
    if (g_diversity_state.cal_a.peak_raw > g_diversity_state.cal_a.floor_raw + 50) {
        g_diversity_state.cal_a.calibrated = true;
        ESP_LOGI(TAG, "RX A calibration loaded: floor=%d peak=%d",
                 g_diversity_state.cal_a.floor_raw, g_diversity_state.cal_a.peak_raw);
    }
    
    if (g_diversity_state.cal_b.peak_raw > g_diversity_state.cal_b.floor_raw + 50) {
        g_diversity_state.cal_b.calibrated = true;
        ESP_LOGI(TAG, "RX B calibration loaded: floor=%d peak=%d",
                 g_diversity_state.cal_b.floor_raw, g_diversity_state.cal_b.peak_raw);
    }
    
    nvs_close(nvs_handle);
}

/**
 * @brief Add one floor sample (call once per timer tick, non-blocking)
 *
 * Collects ADC samples incrementally so the LVGL task never blocks.
 * Call diversity_calibrate_floor_finish() after DIVERSITY_CALIB_SAMPLES ticks.
 */
void diversity_calibrate_floor_sample(diversity_rx_t rx, uint16_t* buf, int index) {
    buf[index] = (rx == DIVERSITY_RX_A) ? adc_converted_value[0] : adc_converted_value[1];
}

/**
 * @brief Compute floor result from filled sample buffer
 *
 * Algorithm: median + 8% fixed margin (robust, no MAD complexity needed
 * given ESP32 ADC noise floor is already larger than MAD precision).
 */
bool diversity_calibrate_floor_finish(uint16_t* buf, int count, uint16_t* result) {
    if (count <= 0) return false;

    // Insertion sort (fine for <=50 samples)
    for (int i = 1; i < count; i++) {
        uint16_t key = buf[i];
        int j = i - 1;
        while (j >= 0 && buf[j] > key) { buf[j + 1] = buf[j]; j--; }
        buf[j + 1] = key;
    }

    uint16_t median = buf[count / 2];
    uint16_t margin = (median * 8) / 100;
    *result = median + margin;

    ESP_LOGI(TAG, "Floor finish: median=%d margin=%d result=%d", median, margin, *result);
    return true;
}

/**
 * @brief Add one peak sample (call once per timer tick, non-blocking)
 */
void diversity_calibrate_peak_sample(diversity_rx_t rx, uint16_t* buf, int index) {
    buf[index] = (rx == DIVERSITY_RX_A) ? adc_converted_value[0] : adc_converted_value[1];
}

/**
 * @brief Compute peak result from filled sample buffer
 *
 * Uses 90th percentile — less sensitive to VTX distance variation than 95th.
 */
bool diversity_calibrate_peak_finish(uint16_t* buf, int count, uint16_t* result) {
    if (count <= 0) return false;

    // Insertion sort
    for (int i = 1; i < count; i++) {
        uint16_t key = buf[i];
        int j = i - 1;
        while (j >= 0 && buf[j] > key) { buf[j + 1] = buf[j]; j--; }
        buf[j + 1] = key;
    }

    *result = buf[(count * 90) / 100];

    ESP_LOGI(TAG, "Peak finish: 90th-pct=%d", *result);
    return true;
}

/**
 * @brief Save calibration to NVS
 */
bool diversity_calibrate_save(void) {
    nvs_handle_t nvs_handle;
    
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for calibration save");
        return false;
    }
    
    // Save RX A
    nvs_set_u16(nvs_handle, NVS_KEY_CAL_A_FLOOR, g_diversity_state.cal_a.floor_raw);
    nvs_set_u16(nvs_handle, NVS_KEY_CAL_A_PEAK, g_diversity_state.cal_a.peak_raw);
    
    // Save RX B
    nvs_set_u16(nvs_handle, NVS_KEY_CAL_B_FLOOR, g_diversity_state.cal_b.floor_raw);
    nvs_set_u16(nvs_handle, NVS_KEY_CAL_B_PEAK, g_diversity_state.cal_b.peak_raw);
    
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "Calibration saved to NVS");
    return true;
}
