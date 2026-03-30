/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef _HAL_ADC_H_
#define _HAL_ADC_H_

#include <stdio.h>
#include <stdarg.h>
#include "board.h"
#include "hpm_common.h"

#ifdef __HAL_ADC_INC__
    #define HAL_ADC_EXTERN 
#else
    #define HAL_ADC_EXTERN extern
#endif

// #define ADC_SOC_NO_HW_TRIG_SRC

typedef void (*PFN_HAL_CAPTURE_CALLBACK)(void);


HAL_ADC_EXTERN void hal_adc_init(void);

HAL_ADC_EXTERN uint16_t hal_adc_volt_value_get(void);
#endif
