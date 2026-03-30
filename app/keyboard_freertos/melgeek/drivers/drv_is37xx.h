/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef _DRV_IS37XX_H_
#define _DRV_IS37XX_H_

#include "hal_i2c.h"

#define IS37XX_I2C                  BOARD_APP_I2C3_BASE
#define IS37XX_I2C_IRQ              BOARD_APP_I2C3_IRQ
#define IS37XX_I2C_CLOCK_NAME       BOARD_APP_I2C3_CLK_NAME
#define IS37XX_I2C_DMA              BOARD_APP_I2C3_DMA
#define IS37XX_I2C_DMAMUX           BOARD_APP_I2C3_DMAMUX
#define IS37XX_I2C_DMAMUX_SRC       BOARD_APP_I2C3_DMA_SRC
#define IS37XX_I2C_DMA_CH           17
#define IS37XX_I2C_DMAMUX_CH        DMA_SOC_CHN_TO_DMAMUX_CHN(IS37XX_I2C_DMA, IS37XX_I2C_DMA_CH)

#ifdef IS3741
    #define WK_BOARD_ISSI_ADDR          (0x60)
    #define IS37XX_PWM_REG_NUM          351
#else
    #define IS3729_USE_NUM              2
    #define IS3729_ADDR0                (0x37)   //7位设备地址,底层接口会处理读写位
    #define IS3729_ADDR1                (0x34)   
    #define IS3729_PWM_REG_NUM          (uint16_t)144
    #define IS37XX_PWM_REG_NUM          (IS3729_PWM_REG_NUM * IS3729_USE_NUM)
#endif

#define IS3729_REG_CONFIG       0xA0
#define IS3729_REG_CURRENT      0xA1
#define IS3729_REG_PULL         0xB0

#define IS3741_REG_CONFIG       0x00
#define IS3741_REG_CURRENT      0x01
#define IS3741_REG_PULL         0x02


typedef void (*is37xx_wait_large_data_trans) (void);

void drv_is37xx_init(i2c_complete_handler handler);
void drv_is37xx_uninit(void);
void drv_is37xx_power_on(void);
void drv_is37xx_power_off(void);
bool drv_is37xx_fill_ctrl_led(uint8_t *pdata);
bool drv_is37xx_fill_pwm_reg(uint8_t *pdata, uint8_t wait_mode);
bool drv_is37xx_fill_pwm_reg_range(uint8_t id, uint8_t *pdata, uint16_t start_addr);
void drv_is37xx_register_wait_trans_complete(is37xx_wait_large_data_trans pfunc);
bool drv_is37xx_i2c_bus_test(void);

#endif