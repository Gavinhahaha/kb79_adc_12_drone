/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef _HAL_I2C_H_
#define _HAL_I2C_H_

#include <stdint.h>
#include "board.h"
#include "hpm_clock_drv.h"
#include "hpm_i2c_drv.h"
#ifdef HPMSOC_HAS_HPMSDK_DMAV2
#include "hpm_dmav2_drv.h"
#else
#include "hpm_dma_drv.h"
#endif

#define  QUERY_MODE         0
#define  INTERRUPT_MODE     1
#define  IS_I2C_DATA_LARGE(val)    ((val) > 100 ? 1 : 0)


typedef void (*i2c_complete_handler) (void);


hpm_stat_t hal_i2c_init(I2C_Type *i2c_ptr, clock_name_t clock_name, i2c_config_t config,
                        DMAMUX_Type *dmamux_ptr, uint8_t dmamux_ch, uint8_t dmamux_src);
void hal_i2c_deinit(I2C_Type *i2c_ptr);
hpm_stat_t hal_i2c_write(I2C_Type *i2c_ptr, DMA_Type *dma_ptr, uint8_t dma_ch, uint8_t *data, uint32_t size, uint8_t slave_address, uint8_t wait_mode);
hpm_stat_t hal_i2c_read(I2C_Type *i2c_ptr, DMA_Type *dma_ptr, uint8_t dma_ch, uint8_t *data, uint32_t size, uint8_t slave_address);
void hal_i2c_irq_init(I2C_Type *i2c_ptr, uint32_t irq, i2c_complete_handler handler);

#endif
