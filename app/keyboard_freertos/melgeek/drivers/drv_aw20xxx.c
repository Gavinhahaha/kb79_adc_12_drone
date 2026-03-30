/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#include <string.h>
#include "board.h"
#include "hpm_soc.h"
#include "hpm_gpio_drv.h"
#include "hpm_spi_drv.h"
#include "hpm_dmamux_drv.h"
#include "hpm_dmav2_drv.h"
#include "hpm_clock_drv.h"
#include "drv_aw20xxx.h"

static aw20xxx_wait_large_data_trans m_wait_trans_complete = NULL;

static i2c_config_t aw20xxx_i2c_config = 
{
    .i2c_mode            = i2c_mode_fast,
    .is_10bit_addressing = false,
};

static hpm_stat_t aw20xxx_i2c_init(i2c_config_t config, i2c_complete_handler handler)
{
    hpm_stat_t stat = hal_i2c_init(AW20XXX_I2C, AW20XXX_I2C_CLOCK_NAME, config, 
                                  AW20XXX_I2C_DMAMUX, AW20XXX_I2C_DMAMUX_CH, AW20XXX_I2C_DMAMUX_SRC);
    intc_m_enable_irq_with_priority(AW20XXX_I2C_IRQ, 1);
    hal_i2c_irq_init(AW20XXX_I2C, AW20XXX_I2C_IRQ, handler);
    
    return stat;             
}

static hpm_stat_t aw20xxx_i2c_reg_write(uint8_t slave_addr, uint16_t reg_addr, uint8_t reg_addr_len,
                                                         uint8_t *pdata, uint16_t data_len, uint8_t wait_mode)
{
    hpm_stat_t stat;
    uint16_t total_len = (reg_addr_len > 1) ? data_len + 2 : data_len + 1;
    uint8_t buffer[total_len];
    uint8_t mode = !!wait_mode;

    if (reg_addr_len > 1)
    {
        buffer[0] = (uint8_t)(reg_addr >> 8);  
        buffer[1] = (uint8_t)(reg_addr & 0xFF); 
        memcpy(&buffer[2], pdata, data_len); 
    }
    else
    {
        buffer[0] = reg_addr; 
        memcpy(&buffer[1], pdata, data_len);
    }

    stat = hal_i2c_write(AW20XXX_I2C, AW20XXX_I2C_DMA, AW20XXX_I2C_DMA_CH, buffer, total_len, slave_addr, mode);

    if (mode == INTERRUPT_MODE)//((IS_I2C_DATA_LARGE(total_len)) && (mode == INTERRUPT_MODE))
    {
        if (m_wait_trans_complete)
        {
            m_wait_trans_complete();
        }
    }

    return stat;
}

static hpm_stat_t aw20xxx_i2c_reg_read(uint8_t slave_addr, uint16_t reg_addr, uint8_t reg_addr_len,
                                                        uint8_t *pdata, uint16_t data_len)
{
    hpm_stat_t stat;
    uint8_t reg_buffer[reg_addr_len];

    if (reg_addr_len > 1)
    {
        reg_buffer[0] = (uint8_t)(reg_addr >> 8);  
        reg_buffer[1] = (uint8_t)(reg_addr & 0xFF); 
    }
    else
    {
        reg_buffer[0] = reg_addr; 
    }

    stat = hal_i2c_write(AW20XXX_I2C, AW20XXX_I2C_DMA, AW20XXX_I2C_DMA_CH, reg_buffer, reg_addr_len, slave_addr, QUERY_MODE);
    if (stat != status_success)
    {
        return stat;
    }

    stat = hal_i2c_read(AW20XXX_I2C, AW20XXX_I2C_DMA, AW20XXX_I2C_DMA_CH, pdata, data_len, slave_addr);
    
    return stat;
}

static void aw20xxx_soft_rst(void)
{
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE0}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_RSTN, 1, (uint8_t []){0xAE}, 1, QUERY_MODE);
    board_delay_ms(3);
}

static void aw20xxx_chip_swen(void)
{
    uint8_t val = 0;

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE0}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_read(AW20XXX_DEVICE_ADDR, REG_GCR, 1, &val, sizeof(val));
    
    val &= BIT_CHIPEN_DIS;
    val |= BIT_CHIPEN_EN;
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_GCR, 1, (uint8_t []){val}, 1, QUERY_MODE);
}

static bool aw20xxx_chip_id(void)
{
    uint8_t val = 0;
    bool id_sta = false;

    aw20xxx_i2c_reg_read(AW20XXX_DEVICE_ADDR, REG_RSTN, 1, &val, sizeof(val));
    if(AW20XXX_CHIPID == val) 
    {
      id_sta = true;
    }
    return id_sta;
}

static void aw20xxx_set_global_current(uint8_t current)
{
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE0}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_GCCR, 1, (uint8_t []){current}, 1, QUERY_MODE);
}

static void aw20xxx_set_pwm_freq(uint8_t freq)
{
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE0}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PCCR, 1, (uint8_t []){freq}, 1, QUERY_MODE);
}

static void aw20xxx_set_breath_pwm(uint8_t pwm_reg, uint8_t pwm)
{
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE0}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, pwm_reg, 1, (uint8_t []){pwm}, 1, QUERY_MODE);
}

static void aw20xxx_set_constant_current_by_idx(uint8_t idx, uint8_t constant_current)
{
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, idx, 1, (uint8_t []){constant_current}, 1, QUERY_MODE);
}

static void aw20xxx_set_pwm_by_idx(int idx, unsigned char pwm)
{
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, idx, 1, (uint8_t []){pwm}, 1, QUERY_MODE);
}

static void aw20xxx_fast_clear_display(void)
{
    int i = 0;

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE1}, 1, QUERY_MODE);
    for(i = 0;i < AW20XXX_REG_NUM_PAG1; i++)
    {
        aw20xxx_set_pwm_by_idx(i, 0x00);
    }

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE2}, 1, QUERY_MODE);

    for(i = 0;i < AW20XXX_REG_NUM_PAG2; i++)
    {
        aw20xxx_set_constant_current_by_idx(i, 0x00);
    }
}

static void aw20xxx_RGB_brightness_or_colour(void)
{
    uint16_t i = 0;

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE2}, 1, QUERY_MODE);
    for (i = 0; i < AW20XXX_REG_NUM_PAG1 / 3; i++) 
    {
      /* R colour brightness set*/
      aw20xxx_set_constant_current_by_idx(i * 3, AW20XXX_RSL_SET);
      /* G colour brightness set*/
      aw20xxx_set_constant_current_by_idx(i * 3 + 1, AW20XXX_GSL_SET);
      /* B colour brightness set*/
      aw20xxx_set_constant_current_by_idx(i * 3 + 2, AW20XXX_BSL_SET);
    }
}

static void aw20xxx_fast_breath_PWM_display(void)
{
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE0}, 1, QUERY_MODE);
    /* 2. set pattern0_breath max/min level */
    aw20xxx_set_breath_pwm(REG_PWMH0, AW20XXX_MAX_BREATH_PWM);
    aw20xxx_set_breath_pwm(REG_PWML0, AW20XXX_MIN_BREATH_PWM);
    aw20xxx_set_breath_pwm(REG_PWMH1, AW20XXX_MAX_BREATH_PWM);
    aw20xxx_set_breath_pwm(REG_PWML1, AW20XXX_MIN_BREATH_PWM);
    aw20xxx_set_breath_pwm(REG_PWMH2, AW20XXX_MAX_BREATH_PWM);
    aw20xxx_set_breath_pwm(REG_PWML2, AW20XXX_MIN_BREATH_PWM);
}

static void aw20xxx_all_on(void)
{
    int i = 0;

    /* 1.set constant current for broghtness */
    aw20xxx_RGB_brightness_or_colour();

    /* 2.set PWM for brigstness */
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE1}, 1, QUERY_MODE);
    for (i = 0; i < AW20XXX_REG_NUM_PAG2; i++) 
    {
        aw20xxx_set_pwm_by_idx(i, AW20XXX_MAX_PWM);
    }
}

static void aw20xxx_all_breath_forever(void)
{
    int i = 0;

    /* 1. set breath pwm mode source pattern controller 0*/

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE3}, 1, QUERY_MODE);
    for (i = 0; i < AW20XXX_REG_NUM_PAG3; i++) 
    {
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, i, 1, (uint8_t []){0x15}, 1, QUERY_MODE);
    }

    /* 2. set RGB current for brightness Equal to set color */
    aw20xxx_RGB_brightness_or_colour();

    /* 3. pattern controller init */
    aw20xxx_fast_breath_PWM_display();

    /* 4. set pattern 0-2 breath T0, T1, T2, T3 */
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT0T0, 1, (uint8_t []){T0T23_10S | T1T30_38S}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT0T1, 1, (uint8_t []){T0T23_10S | T1T30_04S}, 1, QUERY_MODE);

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT1T0, 1, (uint8_t []){T0T23_10S | T1T30_38S}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT1T1, 1, (uint8_t []){T0T23_10S | T1T30_04S}, 1, QUERY_MODE);

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT2T0, 1, (uint8_t []){T0T23_10S | T1T30_38S}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT2T1, 1, (uint8_t []){T0T23_10S | T1T30_04S}, 1, QUERY_MODE);

    /* 5. set pattern 0-2 breath start form xx state */

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT0T2, 1, (uint8_t []){0x00}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT1T2, 1, (uint8_t []){0x00}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT2T2, 1, (uint8_t []){0x00}, 1, QUERY_MODE);

    /* 6. set pattern 0-2 breath loop times -> forever */

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT0T3, 1, (uint8_t []){0x00}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT1T3, 1, (uint8_t []){0x00}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT2T3, 1, (uint8_t []){0x00}, 1, QUERY_MODE);

    /* 7. set pattern 0-2 breath mode */

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT0CFG, 1, (uint8_t []){0x03}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT1CFG, 1, (uint8_t []){0x03}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT2CFG, 1, (uint8_t []){0x03}, 1, QUERY_MODE);

    /* 8. start breath */
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PATGO, 1, (uint8_t []){0x07}, 1, QUERY_MODE);

}

static void aw20xxx_all_manual(void)
{
    int i = 0;

    /* 1. set breath pwm mode source pattern controller 0*/
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE3}, 1, QUERY_MODE);
    for(i = 0; i < AW20XXX_REG_NUM_PAG3; i++)
    {
          aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, i, 1, (uint8_t []){0x15}, 1, QUERY_MODE);
    }

    /* 2. set RGB current for brightness Equal to set color */
    aw20xxx_RGB_brightness_or_colour();

    /* 3. pattern controller init */
    aw20xxx_fast_breath_PWM_display();

    /* 4. set pattern 0-2 breath T0, T1, T2, T3 */
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT0T0, 1, (uint8_t []){T0T23_10S | T1T30_38S}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT0T1, 1, (uint8_t []){T0T23_10S | T1T30_04S}, 1, QUERY_MODE);

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT1T0, 1, (uint8_t []){T0T23_10S | T1T30_38S}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT1T1, 1, (uint8_t []){T0T23_10S | T1T30_04S}, 1, QUERY_MODE);

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT2T0, 1, (uint8_t []){T0T23_10S | T1T30_38S}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT2T1, 1, (uint8_t []){T0T23_10S | T1T30_04S}, 1, QUERY_MODE);

    /* 5. set pattern 0-2 breath start form xx state */

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT0T2, 1, (uint8_t []){0x00}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT1T2, 1, (uint8_t []){0x00}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT2T2, 1, (uint8_t []){0x00}, 1, QUERY_MODE);

    /* 6. set pattern 0-2 breath loop times -> forever */

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT0T3, 1, (uint8_t []){0x00}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT1T3, 1, (uint8_t []){0x00}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT2T3, 1, (uint8_t []){0x00}, 1, QUERY_MODE);

    /* 7. set auto_breath mode */
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT0CFG, 1, (uint8_t []){0x0d}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT1CFG, 1, (uint8_t []){0x0d}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_PAT2CFG, 1, (uint8_t []){0x0d}, 1, QUERY_MODE);

}


static void aw20xxx_open_detect(uint8_t *data)
{
    uint8_t val = 0;
    uint8_t i,j = 0;

    /* 1. enable chipen and set SW number and enable open detect */
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE0}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_read(AW20XXX_DEVICE_ADDR, REG_GCR, 1, &val, sizeof(val));

    val &= BIT_CHIPEN_DIS;
    val |= BIT_OPEN_EN;
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_GCR, 1, (uint8_t []){val}, 1, QUERY_MODE);

    for (i = 0; i < AW20XXX_REG_NUM_PAG3 / 2; i++) 
    {
      aw20xxx_i2c_reg_read(AW20XXX_DEVICE_ADDR, REG_OSR0 + i, 1, &val, sizeof(val));
      for (j = 0; j < 6; j++) 
      {
        *(data + 6*i + j) = val & 0x01;
        val  = val >> 1;
      }
    }
}

static void aw20xxx_short_detect(uint8_t *data)
{
    uint8_t val = 0;
    uint8_t i,j = 0;

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE0}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_read(AW20XXX_DEVICE_ADDR, REG_GCR, 1, &val, sizeof(val));

    val &= BIT_CHIPEN_DIS;
    val |= BIT_SHORT_EN;
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_GCR, 1, (uint8_t []){val}, 1, QUERY_MODE);

    for (i = 0; i < AW20XXX_REG_NUM_PAG3 / 2; i++) 
    {
        aw20xxx_i2c_reg_read(AW20XXX_DEVICE_ADDR, REG_OSR0 + i, 1, &val, sizeof(val));
        for (j = 0;j < 6; j++) 
        {
            *(data + 6*i + j) = val & 0x01;
            val  = val >> 1;
        }
    }
}

static void aw20xxx_white_light_sw_pin_init(void)
{
    HPM_IOC->PAD[IOC_PAD_PA18].FUNC_CTL  = IOC_PA18_FUNC_CTL_GPIO_A_18;
    gpio_set_pin_output(HPM_GPIO0, GPIO_DI_GPIOA, 18);
    gpio_write_pin(HPM_GPIO0, GPIO_DI_GPIOA, 18 , 0);
}

void drv_aw20xxx_white_ligth_switch(bool on)
{
    if (on)
    {
        gpio_write_pin(HPM_GPIO0, GPIO_DI_GPIOA, 18 , 1);
    }
    else
    {
        gpio_write_pin(HPM_GPIO0, GPIO_DI_GPIOA, 18 , 0);
    }
}

static void aw20xxx_set_all_sl(uint8_t r_sl, uint8_t g_sl, uint8_t b_sl)
{
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE2}, 1, QUERY_MODE);
    for (int i = 0; i < AW20XXX_REG_NUM_PAG2 / 3; i++) 
    {
        aw20xxx_set_constant_current_by_idx(i * 3, b_sl);
        aw20xxx_set_constant_current_by_idx(i * 3 + 1, g_sl);
        aw20xxx_set_constant_current_by_idx(i * 3 + 2, r_sl);
    }
}

static void aw20xxx_set_all_pwm(uint8_t pwm_val)
{
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE1}, 1, QUERY_MODE);
    for (int i = 0; i < AW20XXX_CMD_PAGE1; i++) 
    {
        aw20xxx_set_pwm_by_idx(i, pwm_val);
    }
}

static void aw20xxx_set_ssr(uint8_t ssr_val, uint8_t clt_val)
{
    uint8_t val = 0;

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE0}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_read(AW20XXX_DEVICE_ADDR, REG_SSCR, 1, &val, sizeof(val));

    // bit7     CLKOE 
    // bit6     CLKSEL 
    // bit5     RESERVED 
    // bit4     SSE 
    // bit3:2   SSR 
    // bit1:0   CLT
    val &= ~(0x1F); // 清除 SSR（bit3:2）、SSE（bit4）和 CLT（bit1:0）位，保留 bit5-7
    
    val |= (ssr_val & 0x03) << 2; // 设置 SSR 值（0~3 对应 ±5%, ±15%, ±25%, ±35%）
    
    val |= (1 << 4); // 设置 SSE 位为 1，启用频谱扩展
    
    val |= (clt_val & 0x03); // 设置 CLT 值（0~3 对应 1980μs, 1200μs, 820μs, 660μs）

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_SSCR, 1, (uint8_t []){val}, 1, QUERY_MODE);
}

void drv_aw20xxx_update_frame(uint8_t *data, uint8_t wait_mode)
{
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE1}, 1, wait_mode);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, 0, 1, data, AW20XXX_REG_NUM_PAG1, wait_mode);
}

void drv_aw20xxx_update_frame_by_idx(uint8_t idx, uint8_t *data)
{
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE1}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, idx, 1, data, 3, QUERY_MODE);
}

void drv_aw20xxx_read_frame_by_idx(uint8_t idx, uint8_t *data)
{
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE1}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_read(AW20XXX_DEVICE_ADDR, idx, 1, data, 3);
}

int drv_aw20xxx_init(i2c_complete_handler handler)
{
    int ret = 0;

    aw20xxx_i2c_init(aw20xxx_i2c_config, handler);
    aw20xxx_white_light_sw_pin_init();
    drv_aw20xxx_white_ligth_switch(false);

    board_delay_ms(3);
    
    aw20xxx_soft_rst();
    
    aw20xxx_set_ssr(0, 0);
    ret = aw20xxx_chip_id();

    aw20xxx_set_global_current(0xff);
    aw20xxx_set_all_pwm(0);        

    uint8_t sl_val = 0x45;
    aw20xxx_set_all_sl(sl_val * 0.8, sl_val, sl_val * 1.1);

    aw20xxx_chip_swen();

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_MIXCR, 1, (uint8_t []){0x04}, 1, QUERY_MODE);
    
    return ret;
}

void drv_aw20xxx_uninit(void)
{
    int i = 0;
    
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE1}, 1, QUERY_MODE);
    for (i = 0; i < AW20XXX_REG_NUM_PAG1; i++) 
    {
        aw20xxx_set_pwm_by_idx(i, 0x00);
    }

    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE2}, 1, QUERY_MODE);
    for (i = 0; i < AW20XXX_REG_NUM_PAG2; i++) 
    {
        aw20xxx_set_constant_current_by_idx(i, 0x00);
    }

    uint8_t val = 0;
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, AW20XXX_PAGE_ADDR, 1, (uint8_t []){AW20XXX_CMD_PAGE0}, 1, QUERY_MODE);
    aw20xxx_i2c_reg_read(AW20XXX_DEVICE_ADDR, REG_GCR, 1, &val, sizeof(val));
    val &= BIT_CHIPEN_DIS;  // 清除 CHIPEN 位
    aw20xxx_i2c_reg_write(AW20XXX_DEVICE_ADDR, REG_GCR, 1, (uint8_t []){val}, 1, QUERY_MODE);

    board_delay_ms(10);

}

void drv_aw20xxx_register_wait_trans_complete(aw20xxx_wait_large_data_trans pfunc)
{
    m_wait_trans_complete = pfunc;
}