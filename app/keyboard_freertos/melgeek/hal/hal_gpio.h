/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#ifndef _HAL_GPIO_H_
#define _HAL_GPIO_H_

#include <stdio.h>
#include <stdarg.h>
#include "hpm_common.h"

typedef void (*isr_gpioa_callback_t)(void);


typedef enum
{
    SM_RESTART_APP = 0,
    SM_RESTART_BOOT, // Restart on boot
}sm_restart_state_t;

void hal_gpio_init(void);

void hal_sm_restart(sm_restart_state_t restart_sta);
void hal_sync_start(void);
void hal_notify_enable(uint8_t en);
uint8_t hal_notify_get_state(void);

void hal_gpioa_register_notify_callback(isr_gpioa_callback_t isr_cb);
void hal_gpioa_unregister_notify_callback(void);
void hal_gpioa_register_button_callback(isr_gpioa_callback_t isr_cb);
void hal_gpioa_unregister_button_callback(void);
void hal_gpioa_register_encoder_callback(isr_gpioa_callback_t isr_cb);
void hal_gpioa_unregister_encoder_callback(void);

#endif