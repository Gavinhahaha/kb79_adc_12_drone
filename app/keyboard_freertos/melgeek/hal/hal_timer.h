/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef _HAL_TIMER_H_
#define _HAL_TIMER_H_
#include <stdio.h>
#include <stdarg.h>
#include "hpm_common.h"

// #define ADC_SOC_NO_HW_TRIG_SRC

extern __IO uint8_t g_tmr_flag;
extern __IO uint8_t g_sof_flag;

typedef void (*PFN_HAL_TIMER_CALLBACK)(void);

void hal_timer_sw_init(uint32_t time_us, PFN_HAL_TIMER_CALLBACK pfn_callback);
void hal_timer_hw_init(uint32_t time_us);
void hal_timer_sw_stop(void);

void hal_timer_config_start(uint32_t time_us, PFN_HAL_TIMER_CALLBACK pfn_cb);
void hal_timer_sw_start(void);
void hal_timer_sw_stop(void);
void hal_timer_start(void);
void hal_timer_stop(void);
void hal_timer_set_timeout_callback(PFN_HAL_TIMER_CALLBACK pfn_callback);
#endif
