/**
 * @file quick_menu.c
 * @brief Quick Action Menu Implementation
 * 
 * Popup menu for fast access to common features from main page.
 * Activated by long-hold RIGHT button (1 second).
 * 
 * Design:
 * - Centered popup with semi-transparent background
 * - 6 menu items with icons
 * - Smooth animation (slide up 200ms)
 * - Consistent navigation pattern
 * - Audio feedback on selection
 * 
 * @version 1.8.0
 * @date 2026-02-20
 */

#include "quick_menu.h"
#include "beep.h"
#include <string.h>
#include <stdlib.h>

// Menu styling constants
#define MENU_WIDTH          200
#define MENU_HEIGHT         220
#define MENU_RADIUS         10
#define MENU_PADDING        10
#define ITEM_HEIGHT         32
#define ANIM_TIME           200  // ms

// Static menu items definition
static const quick_menu_item_t menu_items[QUICK_ACTION_COUNT] = {
    {NULL, NULL, QUICK_ACTION_NONE, false},  // Placeholder
    {"Quick Scan", LV_SYMBOL_REFRESH, QUICK_ACTION_SCAN, true},
    {"Spectrum", LV_SYMBOL_IMAGE, QUICK_ACTION_SPECTRUM, true},
    {"Calibration", LV_SYMBOL_SETTINGS, QUICK_ACTION_CALIBRATION, true},
    {"Band X", LV_SYMBOL_EDIT, QUICK_ACTION_BAND_X, true},
    {"Settings", LV_SYMBOL_LIST, QUICK_ACTION_SETTINGS, true},
    {"Main Menu", LV_SYMBOL_HOME, QUICK_ACTION_MAIN_MENU, true}
};

// Static menu context (single instance)
static quick_menu_t g_menu = {0};
static bool g_menu_initialized = false;

/**
 * @brief Event handler for menu list
 */
static void menu_event_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    quick_menu_t* menu = (quick_menu_t*)lv_event_get_user_data(e);
    
    if (!menu || !menu->active) return;
    
    if (code == LV_EVENT_CLICKED) {
        // Get selected button index
        lv_obj_t* btn = lv_event_get_target(e);
        uint32_t index = (uint32_t)(uintptr_t)lv_obj_get_user_data(btn);
        
        if (index > 0 && index < QUICK_ACTION_COUNT) {
            quick_action_t action = menu_items[index].action;
            
            // Audio feedback
            beep_turn_on(50);
            
            // Hide menu
            quick_menu_hide(menu);
            
            // Trigger callback
            if (menu->on_action) {
                menu->on_action(action);
            }
        }
    }
}

/**
 * @brief Create menu popup UI
 */
static bool create_menu_ui(quick_menu_t* menu, lv_obj_t* parent) {
    if (!menu || !parent) return false;
    
    // Create modal background
    lv_obj_t* bg = lv_obj_create(parent);
    lv_obj_set_size(bg, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(bg, 0, 0);
    lv_obj_set_style_bg_color(bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(bg, LV_OPA_50, 0);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create menu container
    lv_obj_t* menu_obj = lv_obj_create(bg);
    lv_obj_set_size(menu_obj, MENU_WIDTH, MENU_HEIGHT);
    lv_obj_align(menu_obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(menu_obj, MENU_RADIUS, 0);
    lv_obj_set_style_pad_all(menu_obj, MENU_PADDING, 0);
    lv_obj_clear_flag(menu_obj, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create title label
    lv_obj_t* title = lv_label_create(menu_obj);
    lv_label_set_text(title, "Quick Actions");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    
    // Create list container for menu items
    lv_obj_t* list = lv_obj_create(menu_obj);
    lv_obj_set_size(list, MENU_WIDTH - MENU_PADDING * 2, MENU_HEIGHT - 40);
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_pad_all(list, 2, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_clear_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create menu item buttons
    for (uint8_t i = 1; i < QUICK_ACTION_COUNT; i++) {
        if (!menu_items[i].enabled) continue;
        
        lv_obj_t* btn = lv_btn_create(list);
        lv_obj_set_size(btn, LV_PCT(100), ITEM_HEIGHT);
        lv_obj_add_event_cb(btn, menu_event_handler, LV_EVENT_CLICKED, menu);
        lv_obj_set_user_data(btn, (void*)(uintptr_t)i);
        
        // Create label with icon
        lv_obj_t* label = lv_label_create(btn);
        char text[64];
        snprintf(text, sizeof(text), "%s %s", 
                 menu_items[i].icon ? menu_items[i].icon : "",
                 menu_items[i].label);
        lv_label_set_text(label, text);
        lv_obj_center(label);
    }
    
    // Slide-up animation
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, menu_obj);
    lv_anim_set_time(&a, ANIM_TIME);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_values(&a, lv_obj_get_y(menu_obj) + 50, lv_obj_get_y(menu_obj));
    lv_anim_start(&a);
    
    // Fade-in animation
    lv_anim_t a2;
    lv_anim_init(&a2);
    lv_anim_set_var(&a2, menu_obj);
    lv_anim_set_time(&a2, ANIM_TIME);
    lv_anim_set_exec_cb(&a2, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_values(&a2, LV_OPA_0, LV_OPA_COVER);
    lv_anim_start(&a2);
    
    menu->menu_obj = bg;
    menu->list = list;
    menu->parent = parent;
    menu->active = true;
    menu->selected_index = 1;  // First item
    
    return true;
}

// Public API implementation

quick_menu_t* quick_menu_init(void) {
    if (g_menu_initialized) {
        return &g_menu;
    }
    
    memset(&g_menu, 0, sizeof(quick_menu_t));
    g_menu_initialized = true;
    
    return &g_menu;
}

bool quick_menu_show(quick_menu_t* menu, lv_obj_t* parent, void (*on_action)(quick_action_t)) {
    if (!menu || !parent) return false;
    
    // Don't show if already active
    if (menu->active) return false;
    
    menu->on_action = on_action;
    
    // Create UI
    if (!create_menu_ui(menu, parent)) {
        return false;
    }
    
    // Audio feedback
    beep_turn_on(50);
    
    return true;
}

void quick_menu_hide(quick_menu_t* menu) {
    if (!menu || !menu->active) return;
    
    // Delete menu object (includes all children)
    if (menu->menu_obj) {
        lv_obj_del(menu->menu_obj);
        menu->menu_obj = NULL;
    }
    
    menu->list = NULL;
    menu->parent = NULL;
    menu->active = false;
    menu->selected_index = 0;
    menu->on_action = NULL;
}

bool quick_menu_is_active(quick_menu_t* menu) {
    return menu ? menu->active : false;
}

bool quick_menu_handle_key(quick_menu_t* menu, uint32_t key) {
    if (!menu || !menu->active) return false;
    
    switch (key) {
        case LV_KEY_UP:
            if (menu->selected_index > 1) {
                menu->selected_index--;
                beep_turn_on(30);
            }
            return true;
            
        case LV_KEY_DOWN:
            if (menu->selected_index < QUICK_ACTION_COUNT - 1) {
                menu->selected_index++;
                beep_turn_on(30);
            }
            return true;
            
        case LV_KEY_ENTER:
            // Select current item
            if (menu->selected_index > 0 && menu->selected_index < QUICK_ACTION_COUNT) {
                quick_action_t action = menu_items[menu->selected_index].action;
                beep_turn_on(50);
                quick_menu_hide(menu);
                
                if (menu->on_action) {
                    menu->on_action(action);
                }
            }
            return true;
            
        case LV_KEY_LEFT:
        case LV_KEY_ESC:
            // Cancel/close menu
            beep_turn_on(30);
            quick_menu_hide(menu);
            return true;
            
        default:
            return false;
    }
}

quick_action_t quick_menu_get_selected(quick_menu_t* menu) {
    if (!menu || menu->selected_index >= QUICK_ACTION_COUNT) {
        return QUICK_ACTION_NONE;
    }
    return menu_items[menu->selected_index].action;
}

void quick_menu_set_item_enabled(quick_menu_t* menu, quick_action_t action, bool enabled) {
    // Note: This modifies static data - not thread-safe
    // For runtime enable/disable, would need dynamic array
    if (action > 0 && action < QUICK_ACTION_COUNT) {
        // Cannot modify const data directly
        // In production, would need mutable item array
    }
}

void quick_menu_cleanup(quick_menu_t* menu) {
    if (!menu) return;
    
    quick_menu_hide(menu);
    memset(menu, 0, sizeof(quick_menu_t));
    
    if (menu == &g_menu) {
        g_menu_initialized = false;
    }
}

const char* quick_menu_action_name(quick_action_t action) {
    if (action <= 0 || action >= QUICK_ACTION_COUNT) {
        return "NONE";
    }
    return menu_items[action].label;
}
