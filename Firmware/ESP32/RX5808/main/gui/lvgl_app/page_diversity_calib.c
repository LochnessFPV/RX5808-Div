#include "page_diversity_calib.h"
#include "page_menu.h"
#include "page_main.h"
#include "diversity.h"
#include "rx5808.h"
#include "lvgl_stl.h"
#include "beep.h"
#include "led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

LV_FONT_DECLARE(lv_font_chinese_12);

#define page_diversity_calib_anim_enter  lv_anim_path_bounce
#define page_diversity_calib_anim_leave  lv_anim_path_bounce

// Calibration states - Phase 3A: 8-screen flow
typedef enum {
    CALIB_STATE_WELCOME,          // Screen 1: Introduction
    CALIB_STATE_FLOOR_SETUP,      // Screen 2: Remove antennas instruction
    CALIB_STATE_FLOOR_SAMPLING,   // Screen 3: Sampling with progress
    CALIB_STATE_FLOOR_RESULTS,    // Screen 4: Show results, retry/continue
    CALIB_STATE_PEAK_SETUP,       // Screen 5: Connect antennas & VTX instruction
    CALIB_STATE_PEAK_SAMPLING,    // Screen 6: Sampling with progress
    CALIB_STATE_PEAK_RESULTS,     // Screen 7: Show results, retry/continue
    CALIB_STATE_SUMMARY           // Screen 8: Final summary with save
} calib_state_t;

static lv_obj_t* calib_contain;
static lv_obj_t* title_label;
static lv_obj_t* instruction_label;
static lv_obj_t* rssi_a_label;
static lv_obj_t* rssi_b_label;
static lv_obj_t* warning_label;
static lv_obj_t* progress_bar;
static lv_obj_t* action_btn_label;
static lv_obj_t* retry_btn_label;
static lv_timer_t* calib_timer;
static lv_group_t* calib_group;

static calib_state_t current_state = CALIB_STATE_WELCOME;
static uint16_t floor_a_result = 0;
static uint16_t floor_b_result = 0;
static uint16_t peak_a_result = 0;
static uint16_t peak_b_result = 0;
static uint16_t old_floor_a = 0;
static uint16_t old_floor_b = 0;
static uint16_t old_peak_a = 0;
static uint16_t old_peak_b = 0;
static int sampling_progress = 0;
static bool calibrating = false;
static bool initializing = false;  // Prevent key events during initialization

// Animation callback wrapper for lv_obj_set_y
static void anim_y_cb(void* obj, int32_t value)
{
    lv_obj_set_y((lv_obj_t*)obj, (lv_coord_t)value);
}

static void page_diversity_calib_exit(void);
static void page_diversity_calib_event(lv_event_t* event);
static void page_diversity_calib_timer_event(lv_timer_t* tmr);

static void update_ui_for_state(void)
{
    const char* title = "";
    const char* instruction = "";
    bool show_rssi = false;
    bool show_action_btn = true;
    bool show_retry_btn = false;
    bool show_warning = false;
    const char* action_text = "Next";
    const char* warning_text = "";
    int progress = 0;
    bool is_english = (RX5808_Get_Language() == 0);

    if (current_state == CALIB_STATE_FLOOR_SAMPLING || current_state == CALIB_STATE_PEAK_SAMPLING) {
        led_set_pattern(LED_PATTERN_BREATHING);
    } else {
        led_set_pattern(lock_flag ? LED_PATTERN_SOLID : LED_PATTERN_HEARTBEAT);
    }

    switch(current_state) {
        case CALIB_STATE_WELCOME:
            title = "Calibration";
            instruction = is_english ? 
                "Calibrate RSSI for\nboth receivers.\n\n✓ Median+MAD algorithm\nEstimate: 30 sec" :
                "校准两个接收器\n的RSSI。\n\n✓ 中位数+MAD算法\n预计: 30秒";
            action_text = is_english ? "Start" : "开始";
            progress = 0;
            break;

        case CALIB_STATE_FLOOR_SETUP:
            title = is_english ? "Floor Calibration" : "底噪校准";
            instruction = is_english ?
                "Remove BOTH antennas\n(top A & bottom B)\n\nPress OK when ready" :
                "移除两个天线\n(顶部A和底部B)\n\n准备好后按OK";
            show_rssi = true;
            action_text = is_english ? "Start" : "开始";
            progress = 10;
            break;

        case CALIB_STATE_FLOOR_SAMPLING:
            title = is_english ? "Sampling Floor..." : "采样底噪...";
            instruction = is_english ?
                "Median+MAD: 100 samples\nKeep antennas removed\n\n>95% accuracy" :
                "中位数+MAD: 100样本\n保持天线移除\n\n>95%准确率";
            show_rssi = true;
            show_action_btn = false;
            progress = 10 + (sampling_progress * 20 / 100); // 10->30%
            break;

        case CALIB_STATE_FLOOR_RESULTS:
            title = is_english ? "Floor Results" : "底噪结果";
            instruction = is_english ?
                "RX A Floor: %4d\nRX B Floor: %4d" :
                "接收器A底噪: %4d\n接收器B底噪: %4d";
            show_action_btn = true;
            show_retry_btn = true;
            show_warning = true;
            action_text = is_english ? "Continue" : "继续";
            progress = 30;
            
            // Validation warnings
            if (floor_a_result > 500 || floor_b_result > 500) {
                warning_text = is_english ? "Warning: High floor!" : "警告:底噪过高!";
            } else if (abs((int)floor_a_result - (int)floor_b_result) > 100) {
                warning_text = is_english ? "Warning: Asymmetric!" : "警告:不对称!";
            } else {
                warning_text = is_english ? "Good!" : "良好!";
                show_warning = false; // Don't show if good
            }
            break;

        case CALIB_STATE_PEAK_SETUP:
            title = is_english ? "Peak Calibration" : "峰值校准";
            instruction = is_english ?
                "Connect BOTH antennas\nPlace VTX 10cm away\n\nPress OK when ready" :
                "连接两个天线\n将VTX放在10cm处\n\n准备好后按OK";
            show_rssi = true;
            action_text = is_english ? "Start" : "开始";
            progress = 50;
            break;

        case CALIB_STATE_PEAK_SAMPLING:
            title = is_english ? "Sampling Peak..." : "采样峰值...";
            instruction = is_english ?
                "95th percentile: 100\nKeep VTX in place\n\nRobust estimation" :
                "95百分位: 100样本\n保持VTX位置\n\n稳健估计";
            show_rssi = true;
            show_action_btn = false;
            progress = 50 + (sampling_progress * 20 / 100); // 50->70%
            break;

        case CALIB_STATE_PEAK_RESULTS:
            title = is_english ? "Peak Results" : "峰值结果";
            instruction = is_english ?
                "RX A Peak: %4d\nRX B Peak: %4d" :
                "接收器A峰值: %4d\n接收器B峰值: %4d";
            show_action_btn = true;
            show_retry_btn = true;
            show_warning = true;
            action_text = is_english ? "Continue" : "继续";
            progress = 70;
            
            // Validation warnings
            int range_a = peak_a_result - floor_a_result;
            int range_b = peak_b_result - floor_b_result;
            
            if (peak_a_result <= floor_a_result || peak_b_result <= floor_b_result) {
                warning_text = is_english ? "Error: Peak < Floor!" : "错误:峰值<底噪!";
            } else if (range_a < 500 || range_b < 500) {
                warning_text = is_english ? "Warning: Range too small!" : "警告:范围太小!";
            } else if (abs(range_a - range_b) > range_a / 5) {  // >20% difference
                warning_text = is_english ? "Warning: Asymmetric!" : "警告:不对称!";
            } else {
                warning_text = is_english ? "Good!" : "良好!";
                show_warning = false;
            }
            break;

        case CALIB_STATE_SUMMARY:
            title = is_english ? "Summary" : "总结";
            instruction = is_english ?
                "A: %4d->%4d (R:%4d)\nB: %4d->%4d (R:%4d)\n\nSave calibration?" :
                "A: %4d->%4d (范围:%4d)\nB: %4d->%4d (范围:%4d)\n\n保存校准?";
            show_action_btn = true;
            show_retry_btn = true;  // Show discard button
            action_text = is_english ? "Save" : "保存";
            progress = 100;
            break;
    }

    // Update UI elements
    lv_label_set_text(title_label, title);
    
    // Update instruction with formatting if needed
    if (current_state == CALIB_STATE_FLOOR_RESULTS) {
        lv_label_set_text_fmt(instruction_label, instruction, floor_a_result, floor_b_result);
    } else if (current_state == CALIB_STATE_PEAK_RESULTS) {
        lv_label_set_text_fmt(instruction_label, instruction, peak_a_result, peak_b_result);
    } else if (current_state == CALIB_STATE_SUMMARY) {
        int range_a = peak_a_result - floor_a_result;
        int range_b = peak_b_result - floor_b_result;
        lv_label_set_text_fmt(instruction_label, instruction,
            floor_a_result, peak_a_result, range_a,
            floor_b_result, peak_b_result, range_b);
    } else {
        lv_label_set_text(instruction_label, instruction);
    }
    
    // Show/hide RSSI labels
    if (show_rssi) {
        lv_obj_clear_flag(rssi_a_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(rssi_b_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(rssi_a_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(rssi_b_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    // Show/hide buttons
    if (show_action_btn) {
        lv_obj_clear_flag(action_btn_label, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(action_btn_label, action_text);
    } else {
        lv_obj_add_flag(action_btn_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    if (show_retry_btn) {
        lv_obj_clear_flag(retry_btn_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(retry_btn_label, LV_STATE_DISABLED);  // Enable focus
        // Set button text based on state
        if (current_state == CALIB_STATE_SUMMARY) {
            lv_label_set_text(retry_btn_label, is_english ? "Discard" : "放弃");
        } else {
            lv_label_set_text(retry_btn_label, is_english ? "Retry" : "重试");
        }
        // Focus action button by default when both buttons are shown
        lv_group_focus_obj(action_btn_label);
    } else {
        lv_obj_add_flag(retry_btn_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(retry_btn_label, LV_STATE_DISABLED);  // Disable focus
        // Focus action button when only it is shown
        if (show_action_btn) {
            lv_group_focus_obj(action_btn_label);
        }
    }
    
    // Show/hide warning
    if (show_warning && warning_text[0] != '\0') {
        lv_obj_clear_flag(warning_label, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(warning_label, warning_text);
        // Color based on content
        if (strstr(warning_text, "Error") || strstr(warning_text, "错误")) {
            lv_obj_set_style_text_color(warning_label, lv_color_make(255, 0, 0), LV_STATE_DEFAULT);
        } else if (strstr(warning_text, "Warning") || strstr(warning_text, "警告")) {
            lv_obj_set_style_text_color(warning_label, lv_color_make(255, 200, 0), LV_STATE_DEFAULT);
        } else {
            lv_obj_set_style_text_color(warning_label, lv_color_make(0, 255, 0), LV_STATE_DEFAULT);
        }
    } else {
        lv_obj_add_flag(warning_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    lv_bar_set_value(progress_bar, progress, LV_ANIM_ON);
}

static void perform_calibration_step(void)
{
    bool success_a = false;
    bool success_b = false;
    
    if (current_state == CALIB_STATE_FLOOR_SAMPLING) {
        // Calibrate floor for both receivers
        sampling_progress = 0;
        calibrating = true;
        success_a = diversity_calibrate_floor(DIVERSITY_RX_A, &floor_a_result);
        success_b = diversity_calibrate_floor(DIVERSITY_RX_B, &floor_b_result);
        calibrating = false;
        
        if (success_a && success_b) {
            current_state = CALIB_STATE_FLOOR_RESULTS;
        } else {
            // Show error and stay in sampling state
            lv_label_set_text(warning_label, 
                RX5808_Get_Language() == 0 ? "Error! Try again" : "错误!重试");
            lv_obj_clear_flag(warning_label, LV_OBJ_FLAG_HIDDEN);
            current_state = CALIB_STATE_FLOOR_SETUP; // Go back to setup
        }
        update_ui_for_state();
    }
    else if (current_state == CALIB_STATE_PEAK_SAMPLING) {
        // Calibrate peak for both receivers
        sampling_progress = 0;
        calibrating = true;
        success_a = diversity_calibrate_peak(DIVERSITY_RX_A, &peak_a_result);
        success_b = diversity_calibrate_peak(DIVERSITY_RX_B, &peak_b_result);
        calibrating = false;
        
        if (success_a && success_b) {
            current_state = CALIB_STATE_PEAK_RESULTS;
        } else {
            // Show error and go back
            lv_label_set_text(warning_label,
                RX5808_Get_Language() == 0 ? "Error! Try again" : "错误!重试");
            lv_obj_clear_flag(warning_label, LV_OBJ_FLAG_HIDDEN);
            current_state = CALIB_STATE_PEAK_SETUP;
        }
        update_ui_for_state();
    }
}

static void page_diversity_calib_event(lv_event_t* event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t* obj = lv_event_get_target(event);
    
    // Ignore events during initialization to prevent immediate closure
    if (initializing) {
        return;
    }
    
    if (code == LV_EVENT_KEY) {
        beep_turn_on();
        lv_key_t key_status = lv_indev_get_key(lv_indev_get_act());
        
        // Handle directional keys for button navigation on screens with two buttons
        if (key_status == LV_KEY_LEFT || key_status == LV_KEY_UP) {
            // Switch to retry/discard button if both buttons are shown
            if ((current_state == CALIB_STATE_FLOOR_RESULTS || 
                 current_state == CALIB_STATE_PEAK_RESULTS ||
                 current_state == CALIB_STATE_SUMMARY) &&
                !lv_obj_has_flag(retry_btn_label, LV_OBJ_FLAG_HIDDEN)) {
                lv_group_focus_obj(retry_btn_label);
            }
            return;
        }
        else if (key_status == LV_KEY_RIGHT || key_status == LV_KEY_DOWN) {
            // Switch to action button if both buttons are shown
            if ((current_state == CALIB_STATE_FLOOR_RESULTS || 
                 current_state == CALIB_STATE_PEAK_RESULTS ||
                 current_state == CALIB_STATE_SUMMARY) &&
                !lv_obj_has_flag(retry_btn_label, LV_OBJ_FLAG_HIDDEN)) {
                lv_group_focus_obj(action_btn_label);
            }
            return;
        }
        
        if (key_status == LV_KEY_ENTER) {
            // Handle Retry/Discard button
            if (obj == retry_btn_label) {
                if (current_state == CALIB_STATE_FLOOR_RESULTS) {
                    current_state = CALIB_STATE_FLOOR_SETUP;
                    update_ui_for_state();
                } else if (current_state == CALIB_STATE_PEAK_RESULTS) {
                    current_state = CALIB_STATE_PEAK_SETUP;
                    update_ui_for_state();
                } else if (current_state == CALIB_STATE_SUMMARY) {
                    // Discard calibration and exit without saving
                    page_diversity_calib_exit();
                    lv_fun_param_delayed(page_menu_create, 500, 0);
                }
                return;
            }
            
            // Handle Action button
            if (obj == action_btn_label) {
                switch (current_state) {
                    case CALIB_STATE_WELCOME:
                        current_state = CALIB_STATE_FLOOR_SETUP;
                        update_ui_for_state();
                        break;
                        
                    case CALIB_STATE_FLOOR_SETUP:
                        current_state = CALIB_STATE_FLOOR_SAMPLING;
                        update_ui_for_state();
                        // Start calibration in background (timer will trigger it)
                        lv_timer_set_period(calib_timer, 100);
                        perform_calibration_step();
                        break;
                        
                    case CALIB_STATE_FLOOR_RESULTS:
                        current_state = CALIB_STATE_PEAK_SETUP;
                        update_ui_for_state();
                        break;
                        
                    case CALIB_STATE_PEAK_SETUP:
                        current_state = CALIB_STATE_PEAK_SAMPLING;
                        update_ui_for_state();
                        // Start calibration in background
                        lv_timer_set_period(calib_timer, 100);
                        perform_calibration_step();
                        break;
                        
                    case CALIB_STATE_PEAK_RESULTS:
                        current_state = CALIB_STATE_SUMMARY;
                        update_ui_for_state();
                        break;
                        
                    case CALIB_STATE_SUMMARY:
                        // Save calibration and exit
                        diversity_calibrate_save();
                        page_diversity_calib_exit();
                        lv_fun_param_delayed(page_menu_create, 500, 0);
                        break;
                        
                    default:
                        break;
                }
            }
        }
    }
    else if (code == LV_EVENT_LONG_PRESSED) {
        // Long press to cancel
        beep_turn_on();
        page_diversity_calib_exit();
        lv_fun_param_delayed(page_menu_create, 500, 0);
    }
}

static void page_diversity_calib_timer_event(lv_timer_t* tmr)
{
    // Clear initialization flag after first few ticks (longer delay)
    static int init_ticks = 0;
    if (initializing) {
        init_ticks++;
        if (init_ticks >= 3) {  // Wait for 3 ticks (1.5 seconds)
            initializing = false;
            init_ticks = 0;
        }
        return;  // Don't process anything during initialization
    }
    
    // Update real-time RSSI values
    if (current_state == CALIB_STATE_FLOOR_SETUP || 
        current_state == CALIB_STATE_FLOOR_SAMPLING ||
        current_state == CALIB_STATE_PEAK_SETUP ||
        current_state == CALIB_STATE_PEAK_SAMPLING) {
        
        // Get current RSSI values from hardware
        extern uint16_t adc_converted_value[3];
        lv_label_set_text_fmt(rssi_a_label, "RX A: %4d", adc_converted_value[0]);
        lv_label_set_text_fmt(rssi_b_label, "RX B: %4d", adc_converted_value[1]);
    }
    
    // Update progress during sampling (estimate)
    if (calibrating) {
        sampling_progress += 50; // Increment progress estimate
        if (sampling_progress > 500) sampling_progress = 500;
        
        // Update progress bar
        if (current_state == CALIB_STATE_FLOOR_SAMPLING) {
            int progress = 10 + (sampling_progress * 20 / 500);
            lv_bar_set_value(progress_bar, progress, LV_ANIM_OFF);
        } else if (current_state == CALIB_STATE_PEAK_SAMPLING) {
            int progress = 50 + (sampling_progress * 20 / 500);
            lv_bar_set_value(progress_bar, progress, LV_ANIM_OFF);
        }
    }
}

static void page_diversity_calib_exit(void)
{
    if (calib_timer) {
        lv_timer_del(calib_timer);
        calib_timer = NULL;
    }
    
    lv_obj_remove_event_cb(action_btn_label, page_diversity_calib_event);
    lv_obj_remove_event_cb(retry_btn_label, page_diversity_calib_event);
    
    if (calib_group) {
        lv_group_del(calib_group);
        calib_group = NULL;
    }
    lv_obj_del_delayed(calib_contain, 500);
}

void page_diversity_calib_create(void)
{
    // Reset state
    current_state = CALIB_STATE_WELCOME;
    floor_a_result = 0;
    floor_b_result = 0;
    peak_a_result = 0;
    peak_b_result = 0;
    sampling_progress = 0;
    calibrating = false;
    initializing = true;  // Set initialization flag
    
    // Get current calibration values as reference
    diversity_state_t* state = diversity_get_state();
    old_floor_a = state->cal_a.floor_raw;
    old_floor_b = state->cal_b.floor_raw;
    old_peak_a = state->cal_a.peak_raw;
    old_peak_b = state->cal_b.peak_raw;
    
    // Create container
    calib_contain = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(calib_contain);
    lv_obj_set_style_bg_color(calib_contain, lv_color_make(0, 0, 0), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(calib_contain, (lv_opa_t)LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_set_size(calib_contain, 160, 80);
    lv_obj_set_pos(calib_contain, 0, 0);
    
    // Title label
    title_label = lv_label_create(calib_contain);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(title_label, lv_color_make(255, 200, 0), LV_STATE_DEFAULT);
    lv_obj_set_pos(title_label, 5, 2);
    lv_obj_set_size(title_label, 150, 12);
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_CLIP);
    
    // Instruction label -  compact for 160x80 display
    instruction_label = lv_label_create(calib_contain);
    lv_obj_set_style_text_font(instruction_label, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(instruction_label, lv_color_make(255, 255, 255), LV_STATE_DEFAULT);
    lv_obj_set_pos(instruction_label, 3, 14);
    lv_obj_set_size(instruction_label, 154, 28);
    lv_label_set_long_mode(instruction_label, LV_LABEL_LONG_WRAP);
    
    // RSSI A label - real-time values
    rssi_a_label = lv_label_create(calib_contain);
    lv_obj_set_style_text_font(rssi_a_label, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(rssi_a_label, lv_color_make(0, 255, 255), LV_STATE_DEFAULT);
    lv_obj_set_pos(rssi_a_label, 5, 43);
    lv_obj_set_size(rssi_a_label, 75, 12);
    lv_label_set_text(rssi_a_label, "RX A: 0");
    lv_obj_add_flag(rssi_a_label, LV_OBJ_FLAG_HIDDEN);
    
    // RSSI B label
    rssi_b_label = lv_label_create(calib_contain);
    lv_obj_set_style_text_font(rssi_b_label, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(rssi_b_label, lv_color_make(255, 0, 255), LV_STATE_DEFAULT);
    lv_obj_set_pos(rssi_b_label, 80, 43);
    lv_obj_set_size(rssi_b_label, 75, 12);
    lv_label_set_text(rssi_b_label, "RX B: 0");
    lv_obj_add_flag(rssi_b_label, LV_OBJ_FLAG_HIDDEN);
    
    // Warning label
    warning_label = lv_label_create(calib_contain);
    lv_obj_set_style_text_font(warning_label, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(warning_label, lv_color_make(255, 200, 0), LV_STATE_DEFAULT);
    lv_obj_set_pos(warning_label, 5, 43);
    lv_obj_set_size(warning_label, 150, 12);
    lv_label_set_text(warning_label, "");
    lv_obj_add_flag(warning_label, LV_OBJ_FLAG_HIDDEN);
    
    // Progress bar
    progress_bar = lv_bar_create(calib_contain);
    lv_obj_remove_style(progress_bar, NULL, LV_PART_KNOB);
    lv_obj_set_size(progress_bar, 150, 6);
    lv_obj_set_pos(progress_bar, 5, 70);
    lv_obj_set_style_bg_color(progress_bar, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress_bar, lv_color_make(0, 200, 255), LV_PART_INDICATOR);
    lv_obj_set_style_radius(progress_bar, 3, LV_PART_MAIN);
    lv_bar_set_range(progress_bar, 0, 100);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    
    // Action button (primary - Continue/Start/Save)
    action_btn_label = lv_label_create(calib_contain);
    lv_obj_set_style_text_font(action_btn_label, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(action_btn_label, lv_color_make(0, 0, 0), LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(action_btn_label, lv_color_make(255, 255, 255), LV_STATE_FOCUSED);
    lv_obj_set_style_bg_opa(action_btn_label, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(action_btn_label, lv_color_make(0, 180, 0), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(action_btn_label, lv_color_make(0, 255, 0), LV_STATE_FOCUSED);
    lv_obj_set_style_radius(action_btn_label, 3, LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(action_btn_label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(action_btn_label, 0, LV_STATE_DEFAULT);
    lv_obj_set_pos(action_btn_label, 85, 56);
    lv_obj_set_size(action_btn_label, 70, 13);
    lv_label_set_text(action_btn_label, "Start");
    
    // Retry button (secondary - shown only on results screens)
    retry_btn_label = lv_label_create(calib_contain);
    lv_obj_set_style_text_font(retry_btn_label, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(retry_btn_label, lv_color_make(0, 0, 0), LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(retry_btn_label, lv_color_make(255, 255, 255), LV_STATE_FOCUSED);
    lv_obj_set_style_bg_opa(retry_btn_label, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(retry_btn_label, lv_color_make(100, 100, 100), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(retry_btn_label, lv_color_make(180, 180, 180), LV_STATE_FOCUSED);
    lv_obj_set_style_radius(retry_btn_label, 3, LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(retry_btn_label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(retry_btn_label, 0, LV_STATE_DEFAULT);
    lv_obj_set_pos(retry_btn_label, 5, 56);
    lv_obj_set_size(retry_btn_label, 50, 13);
    lv_label_set_text(retry_btn_label, "Retry");
    lv_obj_add_flag(retry_btn_label, LV_OBJ_FLAG_HIDDEN); // Hidden by default
    
    // Setup input group
    calib_group = lv_group_create();
    lv_group_set_wrap(calib_group, true);  // Enable cycling between buttons
    lv_indev_set_group(indev_keypad, calib_group);
    lv_group_add_obj(calib_group, retry_btn_label);  // Add retry first (left button)
    lv_group_add_obj(calib_group, action_btn_label); // Add action second (right button)
    lv_group_set_editing(calib_group, false);
    lv_group_focus_obj(action_btn_label);  // Focus action button by default
    
    // Add event callbacks AFTER setting up the group
    lv_obj_add_event_cb(action_btn_label, page_diversity_calib_event, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(retry_btn_label, page_diversity_calib_event, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(action_btn_label, page_diversity_calib_event, LV_EVENT_LONG_PRESSED, NULL);
    
    // Create timer for real-time updates (500ms interval)
    calib_timer = lv_timer_create(page_diversity_calib_timer_event, 500, NULL);
    
    // Initialize UI for welcome state
    update_ui_for_state();
    
    // Entrance animations
    lv_amin_start(title_label, -50, 2, 1, 600, 0, (lv_anim_exec_xcb_t)anim_y_cb, page_diversity_calib_anim_enter);
    lv_amin_start(instruction_label, -50, 14, 1, 600, 100, (lv_anim_exec_xcb_t)anim_y_cb, page_diversity_calib_anim_enter);
    lv_amin_start(progress_bar, 100, 70, 1, 600, 200, (lv_anim_exec_xcb_t)anim_y_cb, page_diversity_calib_anim_enter);
    lv_amin_start(action_btn_label, 100, 56, 1, 600, 300, (lv_anim_exec_xcb_t)anim_y_cb, page_diversity_calib_anim_enter);
}
