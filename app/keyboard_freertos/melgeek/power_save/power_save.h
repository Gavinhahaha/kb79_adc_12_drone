/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef  _POWER_SAVE_H_
#define  _POWER_SAVE_H_

#include <stdint.h>
#include <stdbool.h>
enum
{
    NORMAL_MODE,
    LOW_PWR_MODE,
};

void ps_timer_reset_later(void);
void ps_timer_stop(void);
void ps_timer_init(bool en, uint16_t period);
void ps_timer_reset(void);
uint8_t ps_timer_update(bool en, uint16_t period);
uint8_t ps_get_kb_working_mode(void);
#endif