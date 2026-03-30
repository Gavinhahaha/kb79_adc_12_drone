/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#ifndef __DRV_DPM_H__
#define __DRV_DPM_H__

#include <stdbool.h>
#include <stdint.h>


#define  IO_LOW               0
#define  IO_HIGH              1

#define  DPM_PIN_CTRL        (HPM_GPIO0)
#define  DPM_PIN_PORT        (GPIO_DI_GPIOA)
#define  DPM_PIN_IRQ         IRQn_GPIO0_A
#define  DPM_PWR_PIN         (IOC_PAD_PA20)
#define  DPM_RESET_PIN       (IOC_PAD_PA21)


void drv_dpm_gpio_init(void);
void drv_dpm_power_en(bool en);
void drv_dpm_reset(void);






#endif