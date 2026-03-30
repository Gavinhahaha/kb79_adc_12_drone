/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef _DRV_AW20XXX_H_
#define _DRV_AW20XXX_H_

#include "hal_i2c.h"
#include <stdbool.h>
#include <stdint.h>


#define AW20XXX_I2C                  BOARD_APP_I2C3_BASE
#define AW20XXX_I2C_IRQ              BOARD_APP_I2C3_IRQ
#define AW20XXX_I2C_CLOCK_NAME       BOARD_APP_I2C3_CLK_NAME
#define AW20XXX_I2C_DMA              BOARD_APP_I2C3_DMA
#define AW20XXX_I2C_DMAMUX           BOARD_APP_I2C3_DMAMUX
#define AW20XXX_I2C_DMAMUX_SRC       BOARD_APP_I2C3_DMA_SRC
#define AW20XXX_I2C_DMA_CH           15
#define AW20XXX_I2C_DMAMUX_CH        DMA_SOC_CHN_TO_DMAMUX_CHN(AW20XXX_I2C_DMA, AW20XXX_I2C_DMA_CH)

#define AW20198_CHIP_ENABLE		(0)
#define AW20216_CHIP_ENABLE		(1)


#define AW20216_CS_SW(cs, sw)   (((sw) - 1) * 18 + ((cs) - 1))

#define AW20XXX_MAX_CURRENT		(0x9f)
#define AW20XXX_RSL_SET			(0xff) /* R colour brightness set */
#define AW20XXX_GSL_SET			(0xb9) /* G colour brightness set  */
#define AW20XXX_BSL_SET			(0xef) /* B colour brightness set*/

#define AW20XXX_MAX_PWM			(0xff)
#define AW20XXX_MAX_BREATH_PWM	(0xff) /* auto breath pwm */
#define AW20XXX_MIN_BREATH_PWM	(0x00) /* R colour brightness set */

/*********************************************************
 *
 * pag num
 *
 ********************************************************/
#define AW20198_REG_NUM_PAG1	(198)
#define AW20198_REG_NUM_PAG2	(198)
#define AW20198_REG_NUM_PAG3	(66)
#define AW20198_REG_NUM_PAG4	(198)

#define AW20216_REG_NUM_PAG1	(216)
#define AW20216_REG_NUM_PAG2	(216)
#define AW20216_REG_NUM_PAG3	(72)
#define AW20216_REG_NUM_PAG4	(216)

#if AW20198_CHIP_ENABLE
#define AW20XXX_REG_NUM_PAG1	AW20198_REG_NUM_PAG1
#define AW20XXX_REG_NUM_PAG2	AW20198_REG_NUM_PAG2
#define AW20XXX_REG_NUM_PAG3	AW20198_REG_NUM_PAG3
#define AW20XXX_REG_NUM_PAG4	AW20198_REG_NUM_PAG4
#elif AW20216_CHIP_ENABLE
#define AW20XXX_REG_NUM_PAG1	AW20216_REG_NUM_PAG1
#define AW20XXX_REG_NUM_PAG2	AW20216_REG_NUM_PAG2
#define AW20XXX_REG_NUM_PAG3	AW20216_REG_NUM_PAG3
#define AW20XXX_REG_NUM_PAG4	AW20216_REG_NUM_PAG4
#endif

/*********************************************************
 *
 * chip info
 *
 ********************************************************/
#define AW20XXX_DEVICE_ADDR		(0X20) /* AD0 and AD1 both LOW */

#define AW20198_CHIPID			(0x71)
#define AW20216_CHIPID			(0x70)

#if AW20198_CHIP_ENABLE
#define AW20XXX_CHIPID			AW20198_CHIPID
#elif AW20216_CHIP_ENABLE
#define AW20XXX_CHIPID			AW20216_CHIPID
#endif

#define AW20XXX_PAGE_ADDR		(0xF0)
#define AW20XXX_CMD_PAGE0		(0xC0)
#define AW20XXX_CMD_PAGE1		(0xC1)
#define AW20XXX_CMD_PAGE2		(0xC2)
#define AW20XXX_CMD_PAGE3		(0xC3)
#define AW20XXX_CMD_PAGE4		(0xC4)

#if AW20198_CHIP_ENABLE
#define AW20XXX_SUPPORT_LED_NUM_MAX					(198)
#elif AW20216_CHIP_ENABLE
#define AW20XXX_SUPPORT_LED_NUM_MAX					(216)
#endif

#define BIT_CHIPEN_DIS			(~(1<<0))
#define BIT_CHIPEN_EN			(0x1)
#define BIT_SHORT_EN			(0x5)
#define BIT_OPEN_EN				(0x7)

enum AW20XXX_T0_T2_TIME {
	T0T20_00S = 0x00,
	T0T20_13S = 0X10,
	T0T20_26S = 0X20,
	T0T20_38S = 0x30,
	T0T20_51S = 0X40,
	T0T20_77S = 0X50,
	T0T21_04S = 0x60,
	T0T21_60S = 0X70,
	T0T22_10S = 0X80,
	T0T22_60S = 0X90,
	T0T23_10S = 0XA0,
	T0T24_20S = 0XB0,
	T0T25_20S = 0XC0,
	T0T26_20S = 0XD0,
	T0T27_30S = 0XE0,
	T0T28_30S = 0XF0
};

enum AW20XXX_T1_T3_TIME {
	T1T30_04S = 0x00,
	T1T30_13S = 0X01,
	T1T30_26S = 0X02,
	T1T30_38S = 0x03,
	T1T30_51S = 0X04,
	T1T30_77S = 0X05,
	T1T31_04S = 0x06,
	T1T31_60S = 0X07,
	T1T32_10S = 0X08,
	T1T32_60S = 0X09,
	T1T33_10S = 0X0A,
	T1T34_20S = 0X0B,
	T1T35_20S = 0X0C,
	T1T36_20S = 0X0D,
	T1T37_30S = 0X0E,
	T1T38_30S = 0X0F
};

#define REG_GCR					(0x00)
#define REG_GCCR				(0x01)
#define REG_DGCR				(0x02)
#define REG_OSR0				(0x03)
#define REG_OSR1				(0x04)
#define REG_OSR2				(0x05)
#define REG_OSR3				(0x06)
#define REG_OSR4				(0x07)
#define REG_OSR5				(0x08)
#define REG_OSR6				(0x09)
#define REG_OSR7				(0x0A)
#define REG_OSR8				(0x0B)
#define REG_OSR9				(0x0C)
#define REG_OSR10				(0x0D)
#define REG_OSR11				(0x0E)
#define REG_OSR12				(0x0F)
#define REG_OSR13				(0x10)
#define REG_OSR14				(0x11)
#define REG_OSR15				(0x12)
#define REG_OSR16				(0x13)
#define REG_OSR17				(0x14)
#define REG_OSR18				(0x15)
#define REG_OSR19				(0x16)
#define REG_OSR20				(0x17)
#define REG_OSR21				(0x18)
#define REG_OSR22				(0x19)
#define REG_OSR23				(0x1A)
#define REG_OSR24				(0x1B)
#define REG_OSR25				(0x1C)
#define REG_OSR26				(0x1D)
#define REG_OSR27				(0x1E)
#define REG_OSR28				(0x1F)
#define REG_OSR29				(0x20)
#define REG_OSR30				(0x21)
#define REG_OSR31				(0x22)
#define REG_OSR32				(0x23)
#define REG_OTCR				(0x27)
#define REG_SSCR				(0x28)
#define REG_PCCR				(0x29)
#define REG_UVCR				(0x2A)
#define REG_SRCR				(0x2B)
#define REG_RSTN				(0x2F)
#define REG_PWMH0				(0x30)
#define REG_PWMH1				(0x31)
#define REG_PWMH2				(0x32)
#define REG_PWML0				(0x33)
#define REG_PWML1				(0x34)
#define REG_PWML2				(0x35)
#define REG_PAT0T0				(0x36)
#define REG_PAT0T1				(0x37)
#define REG_PAT0T2				(0x38)
#define REG_PAT0T3				(0x39)
#define REG_PAT1T0				(0x3A)
#define REG_PAT1T1				(0x3B)
#define REG_PAT1T2				(0x3C)
#define REG_PAT1T3				(0x3D)
#define REG_PAT2T0				(0x3E)
#define REG_PAT2T1				(0x3F)
#define REG_PAT2T2				(0x40)
#define REG_PAT2T3				(0x41)
#define REG_PAT0CFG				(0x42)
#define REG_PAT1CFG				(0x43)
#define REG_PAT2CFG				(0x44)
#define REG_PATGO				(0x45)
#define REG_MIXCR				(0x46)
#define REG_PAGE				(0xF0)

typedef void (*aw20xxx_wait_large_data_trans) (void);

int drv_aw20xxx_init(i2c_complete_handler handler);
void drv_aw20xxx_register_wait_trans_complete(aw20xxx_wait_large_data_trans pfunc);
void drv_aw20xxx_white_ligth_switch(bool on);
void drv_aw20xxx_update_frame(uint8_t *data, uint8_t wait_mode);
void drv_aw20xxx_update_frame_by_idx(uint8_t idx, uint8_t *data);
void drv_aw20xxx_uninit(void);

#endif

