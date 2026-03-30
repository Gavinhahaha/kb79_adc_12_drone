/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#ifndef __DRV_BUTTON_H__
#define __DRV_BUTTON_H__

#include <stdbool.h>
#include <stdint.h>


#define BUTTON_EVT_BTN_1_DOWN    (1 << 2)
#define BUTTON_EVT_BTN_1_UP      (1 << 3)
#define BUTTON_EVT_BTN_2_DOWN    (1 << 4)
#define BUTTON_EVT_BTN_2_UP      (1 << 5)


typedef void (*drv_button_evt)(uint32_t evt);

void drv_button_init(void (*callback)(uint32_t evt));










#endif