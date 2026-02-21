#include "page_start.h"
#include "page_main.h"
#include "lvgl_stl.h"
#include "hwvers.h"
#include "lv_anim_helpers.h"

//ä¿¡å·è´¨é‡èƒŒå…‰äº®åº¦å¼€æœºåŠ¨ç”»èœ‚é¸£å™¨æ‰“å…³é—­è¯­è¨€ä¸­æ–‡è¾“å‡ºæºä¿å­˜å¹¶é€€å‡ºä¾›ç”µåŽ‹ç•Œé¢ç‰ˆæœ¬æºåè®®å›ºä»¶æ‰«æä¸­å›¾ä¼ å§‹æ ¡å‡†æˆåŠŸå¤±è´¥å¯è®¾ç½®æœ€å¤§åŠŸçŽ‡è¿‡å°è¯·é‡è¯•å±å¹•ç³»ç»Ÿè‡ªæŽ¥æ”¶é¢‘è°±é“ç»“æžœå·²å®‰è£…å¤©çº¿é£Žæ‰‡è½¬é€Ÿåˆ¶å¼ï¼Œ
//æŽ¥æ”¶æœºè®¾ç½®å…³äºŽåˆ†é›†æ‰«æä¸­ç»“æŸæ¨¡æ‹Ÿ 0x21,0x2d,0x2e

LV_FONT_DECLARE(lv_font_start);

lv_obj_t* page_start_contain;
volatile uint8_t start_animation = true;

static void page_start_exit(void);

void page_start_create()
{
    if (start_animation == true)
    {
        page_start_contain = lv_obj_create(lv_scr_act());
        lv_obj_remove_style_all(page_start_contain);
        lv_obj_set_style_bg_color(page_start_contain, lv_color_make(0, 0, 0), LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(page_start_contain, (lv_opa_t)LV_OPA_COVER, LV_STATE_DEFAULT);
        lv_obj_set_size(page_start_contain, 160, 80);
        lv_obj_set_pos(page_start_contain, 0, 0);


        lv_obj_t* start_info_label = lv_label_create(page_start_contain);
        lv_label_set_long_mode(start_info_label, LV_LABEL_LONG_WRAP);

        lv_obj_align(start_info_label, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_text_font(start_info_label, &lv_font_start, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(start_info_label, lv_color_make(255, 255, 255), LV_STATE_DEFAULT);
        lv_label_set_recolor(start_info_label, true);
        lv_label_set_text(start_info_label, "RX5808");

        lv_obj_t* start_info_bar = lv_bar_create(page_start_contain);
        lv_obj_remove_style(start_info_bar, NULL, LV_PART_KNOB);
        lv_obj_set_size(start_info_bar, 100, 5);
        lv_obj_set_style_bg_color(start_info_bar, lv_color_make(255, 140, 0), LV_PART_INDICATOR);
        lv_obj_align(start_info_bar, LV_ALIGN_TOP_MID, 0, 53);
        lv_bar_set_value(start_info_bar, 0, LV_ANIM_OFF);

        // Add version label below the loading bar
        lv_obj_t* version_label = lv_label_create(page_start_contain);
        lv_obj_align(version_label, LV_ALIGN_TOP_MID, 0, 63);
        lv_obj_set_style_text_color(version_label, lv_color_make(128, 128, 128), LV_STATE_DEFAULT);
        lv_label_set_text_fmt(version_label, "v%d.%d.%d", 
            RX5808_VERSION_MAJOR, RX5808_VERSION_MINOR, RX5808_VERSION_PATCH);

        lv_amin_start(start_info_label, -50, 20, 1, 800, 0, anim_set_y_cb, lv_anim_path_linear);
        lv_amin_start(start_info_bar, 0, 100, 1, 500, 800, anim_opa_cb, lv_anim_path_linear);
        lv_amin_start(start_info_bar, 0, 100, 1, 1500, 1000, anim_bar_value_cb, lv_anim_path_linear);

        lv_fun_delayed(page_start_exit, 3000);
    }
    else
        page_main_create();

}

static void page_start_exit()
{
    lv_obj_del(page_start_contain);
    page_main_create();
}


void page_set_animation_en(uint8_t en)
{
    if (en)
        start_animation = true;
    else
        start_animation = false;
}

uint16_t page_get_animation_en()
{
    return start_animation;
}

