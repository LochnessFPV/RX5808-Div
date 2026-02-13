/**
 * @file diversity.h
 * @brief Advanced diversity switching algorithm based on Starfish specification  
 * @version 1.5.1
 * 
 * Implements intelligent diversity switching with:
 * - RSSI normalization and calibration
 * - Variance-based stability scoring
 * - Dwell time, hysteresis, and cooldown
 * - Mode profiles (Race, Freestyle, Long Range)
 * - Switch event counters and telemetry
 */

#ifndef __DIVERSITY_H
#define __DIVERSITY_H

#include <stdint.h>
#include <stdbool.h>

// Configuration
#define DIVERSITY_SAMPLE_WINDOW_MS 200 // Rolling window for variance calculation
#define DIVERSITY_SAMPLE_RATE_HZ 250   // RSSI sampling rate (4ms per sample)
#define DIVERSITY_MAX_SAMPLES 50       // Max samples in rolling window (200ms @ 250Hz)

/** @brief Diversity mode profiles */
typedef enum {
    DIVERSITY_MODE_RACE = 0,      // Aggressive switching, minimal lag
    DIVERSITY_MODE_FREESTYLE,      // Balanced switching, smooth video
    DIVERSITY_MODE_LONG_RANGE,     // Stable switching, range optimization
    DIVERSITY_MODE_COUNT
} diversity_mode_t;

/** @brief Active receiver selection */
typedef enum {
    DIVERSITY_RX_A = 0,
    DIVERSITY_RX_B,
} diversity_rx_t;

/** @brief Receiver health status flags */
typedef struct {
    bool stuck_high;               // RSSI stuck at max value
    bool stuck_low;                // RSSI stuck at min value
    bool no_variance;              // No RSSI changes detected
    bool disabled;                 // Receiver manually disabled
} diversity_health_t;

/** @brief Mode-specific parameters */
typedef struct {
    uint32_t dwell_ms;             // Minimum time before allowing next switch
    uint32_t cooldown_ms;          // Extended dwell after a switch
    uint8_t hysteresis_pct;        // Required score advantage to switch (0-100)
    float weight_rssi;             // RSSI weight in combined score (0.0-1.0)
    float weight_stability;        // Stability weight in combined score (0.0-1.0)
    int16_t slope_threshold;       // Preemptive switch slope threshold (negative)
    const char* name;              // Mode display name
} diversity_mode_params_t;

/** @brief RSSI calibration data per receiver */
typedef struct {
    uint16_t floor_raw;            // Noise floor (min RSSI)
    uint16_t peak_raw;             // Signal peak (max RSSI)
    bool calibrated;               // Calibration completed flag
} rssi_calibration_t;

/** @brief Per-receiver runtime state */
typedef struct {
    // Raw and normalized RSSI
    uint16_t rssi_raw;             // Current raw ADC value
    uint8_t rssi_norm;             // Normalized RSSI (0-100)
    
    // Rolling window for statistics
    uint16_t rssi_samples[DIVERSITY_MAX_SAMPLES];
    uint8_t sample_index;
    uint8_t sample_count;
    
    // Statistics
    uint16_t rssi_mean;            // Mean RSSI over window
    uint16_t rssi_variance;        // Variance over window
    int16_t rssi_slope;            // Rate of change (delta/time)
    
    // Scores
    uint8_t stability_score;       // Stability metric (0-100)
    uint8_t combined_score;        // Final weighted score (0-100)
    
    // Health
    diversity_health_t health;
    uint32_t health_check_ms;      // Time of last health check
} diversity_rx_state_t;

/** @brief Complete diversity system state */
typedef struct {
    // Active configuration
    diversity_mode_t mode;
    diversity_rx_t active_rx;
    
    // Per-receiver state
    diversity_rx_state_t rx_a;
    diversity_rx_state_t rx_b;
    
    // Calibration
    rssi_calibration_t cal_a;
    rssi_calibration_t cal_b;
    
    // Switching logic state
    uint32_t last_switch_ms;       // Time of last switch
    uint32_t switch_count;         // Total switches since boot
    uint32_t switch_count_minute;  // Switches in last 60 seconds
    uint32_t time_stable_ms;       // Time since last switch
    uint32_t longest_stable_ms;    // Longest stable period
    bool in_cooldown;              // Currently in cooldown period
    
    // Telemetry
    int8_t rssi_delta;             // RSSI_A - RSSI_B (signed)
    uint8_t switches_per_min;      // Recent switch rate
    
} diversity_state_t;

// Global mode parameters (defined in diversity.c)
extern const diversity_mode_params_t diversity_mode_params[DIVERSITY_MODE_COUNT];

// Core API functions
void diversity_init(void);
void diversity_set_mode(diversity_mode_t mode);
void diversity_update(void); // Call at DIVERSITY_SAMPLE_RATE_HZ
diversity_rx_t diversity_get_active_rx(void);
diversity_state_t* diversity_get_state(void);

// RSSI calibration
bool diversity_calibrate_start(diversity_rx_t rx);
bool diversity_calibrate_floor(diversity_rx_t rx, uint16_t* result);
bool diversity_calibrate_peak(diversity_rx_t rx, uint16_t* result);
bool diversity_calibrate_save(void);
void diversity_calibrate_load(void);

// Telemetry / stats
uint32_t diversity_get_switch_count(void);
uint32_t diversity_get_switches_per_minute(void);
uint32_t diversity_get_time_stable_ms(void);
void diversity_reset_stats(void);

// Internal functions (exposed for testing)
uint8_t diversity_normalize_rssi(uint16_t raw, rssi_calibration_t* cal);
void diversity_calculate_statistics(diversity_rx_state_t* rx);
void diversity_calculate_scores(diversity_rx_state_t* rx, const diversity_mode_params_t* params);
bool diversity_should_switch(diversity_state_t* state, const diversity_mode_params_t* params);
void diversity_check_receiver_health(diversity_rx_state_t* rx);

#endif // __DIVERSITY_H
