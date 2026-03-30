/*
 * Copyright (c) 2021-2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __HALL_DRIVER_H__
#define __HALL_DRIVER_H__

#include <stdint.h>
#include <stdbool.h>

typedef void (*p_hall_isr)(uint32_t *pbuff);

extern void hall_driver_init(p_hall_isr pfn);
extern void hall_driver_power(uint8_t index, bool enable);
extern void hall_driver_mux_enable(bool enable);
extern void gpio_write_pin_extra(uint32_t port, uint8_t pin, uint8_t high);

#endif
