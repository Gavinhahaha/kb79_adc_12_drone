/*
 * Copyright (c) 2021-2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __DRV_WS2812_H__
#define __DRV_WS2812_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct  {
    uint8_t g;
    uint8_t r;
    uint8_t b;
}rgb_t;

extern void drv_ws2812_init(void);
extern void drv_ws2812_power(bool enable);
extern void drv_ws2812_set_leds(rgb_t *rgb_array, uint32_t count);
extern void drv_ws2812_pin_init(void);
#endif