#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "spi_flash_mmap.h"
#include "system.h"
#include "lvgl_init.h"

void app_main(void)
{

    system_init();
    lvgl_init();
    while(1){
    lv_task_handler();
    vTaskDelay(system_lvgl_sleep_ms / portTICK_PERIOD_MS);
    }
}
