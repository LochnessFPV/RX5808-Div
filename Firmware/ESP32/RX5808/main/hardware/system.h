#ifndef __MODULE_H
#define __MODULE_H

#include <stdint.h>



void LED_Init(void);
void USART_init(uint32_t bound);
void system_init(void);
void system_apply_cpu_freq(uint16_t freq_setting);

#endif


