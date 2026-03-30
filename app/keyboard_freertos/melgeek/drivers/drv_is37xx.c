/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include "drv_is37xx.h"
#include "mg_build_date.h"

static is37xx_wait_large_data_trans m_wait_trans_complete = NULL;

static i2c_config_t is37xx_i2c_config = 
{
    .i2c_mode            = i2c_mode_fast,
    .is_10bit_addressing = false,
};

static hpm_stat_t drv_is37xx_i2c_init(i2c_config_t config, i2c_complete_handler handler)
{
    hpm_stat_t stat = hal_i2c_init(IS37XX_I2C, IS37XX_I2C_CLOCK_NAME, config, 
                                  IS37XX_I2C_DMAMUX, IS37XX_I2C_DMAMUX_CH, IS37XX_I2C_DMAMUX_SRC);
    intc_m_enable_irq_with_priority(IS37XX_I2C_IRQ, 1);
    hal_i2c_irq_init(IS37XX_I2C, IS37XX_I2C_IRQ, handler);
    
    return stat;             
}

static hpm_stat_t drv_is37xx_i2c_reg_write(uint8_t slave_addr, uint16_t reg_addr, uint8_t reg_addr_len,
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

    stat = hal_i2c_write(IS37XX_I2C, IS37XX_I2C_DMA, IS37XX_I2C_DMA_CH, buffer, total_len, slave_addr, mode);

    if (mode == INTERRUPT_MODE)//((IS_I2C_DATA_LARGE(total_len)) && (mode == INTERRUPT_MODE))
    {
        if (m_wait_trans_complete)
        {
            m_wait_trans_complete();
        }
    }

    return stat;
}

static hpm_stat_t drv_is37xx_i2c_reg_read(uint8_t slave_addr, uint16_t reg_addr, uint8_t reg_addr_len,
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

    stat = hal_i2c_write(IS37XX_I2C, IS37XX_I2C_DMA, IS37XX_I2C_DMA_CH, reg_buffer, reg_addr_len, slave_addr, QUERY_MODE);
    if (stat != status_success)
    {
        return stat;
    }

    stat = hal_i2c_read(IS37XX_I2C, IS37XX_I2C_DMA, IS37XX_I2C_DMA_CH, pdata, data_len, slave_addr);
    
    return stat;
}

static bool is37xx_set_config_reg(uint8_t data)
{
#ifdef IS3741
    /* unlock FDh */
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFE, 1, (uint8_t []){0xC5}, 1, QUERY_MODE);
    /* write page four */
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFD, 1, (uint8_t []){0x04}, 1, QUERY_MODE);

    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, IS3741_REG_CONFIG, 1, &data, 1, QUERY_MODE);
#else
    drv_is37xx_i2c_reg_write(IS3729_ADDR0, IS3729_REG_CONFIG, 1, &data, 1, QUERY_MODE); //default:IS3729_REG_CONFIG = 0x09
    board_delay_ms(10);
#if (IS3729_USE_NUM == 2)
    drv_is37xx_i2c_reg_write(IS3729_ADDR1, IS3729_REG_CONFIG, 1, &data, 1, QUERY_MODE); //default:IS3729_REG_CONFIG = 0x09
    board_delay_ms(10);
#endif

#endif
    return true;
}

static bool is37xx_set_current_reg(uint8_t gcc)
{
#ifdef IS3741
    /* unlock FDh */
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFE, 1, (uint8_t []){0xC5}, 1, QUERY_MODE);
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFD, 1, (uint8_t []){0x04}, 1, QUERY_MODE);

    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, IS3741_REG_CURRENT, 1, &gcc, 1, QUERY_MODE);

    return true;
#else
    drv_is37xx_i2c_reg_write(IS3729_ADDR0, IS3729_REG_CURRENT, 1, &gcc, 1, QUERY_MODE); //default:IS3729_REG_CONFIG = 0x40
#if (IS3729_USE_NUM == 2)
    drv_is37xx_i2c_reg_write(IS3729_ADDR1, IS3729_REG_CURRENT, 1, &gcc, 1, QUERY_MODE); //default:IS3729_REG_CONFIG = 0x40
#endif
    return true;
#endif
}

static bool is37xx_set_pull_reg(uint8_t data)
{
#ifdef IS3741
    /* unlock FDh */
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFE, 1, (uint8_t []){0xC5}, 1, QUERY_MODE);
    /* write page four */
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFD, 1, (uint8_t []){0x04}, 1, QUERY_MODE);

    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, IS3741_REG_PULL, 1, data, QUERY_MODE);
#else
    drv_is37xx_i2c_reg_write(IS3729_ADDR0, IS3729_REG_PULL, 1, &data, 1, QUERY_MODE); //default:IS3729_REG_CONFIG = 0x09
#if (IS3729_USE_NUM == 2)
    drv_is37xx_i2c_reg_write(IS3729_ADDR1, IS3729_REG_PULL, 1, &data, 1, QUERY_MODE); //default:IS3729_REG_CONFIG = 0x09
#endif

#endif
    return true;
}

bool drv_is37xx_fill_ctrl_led(uint8_t *pdata)
{
    uint8_t val = 0;
#ifdef IS3741
    /* unlock FDh */
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFE, 1, (uint8_t []){0xC5}, 1, QUERY_MODE);
    /* write page one */
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFD, 1, (uint8_t []){0x02}, 1, QUERY_MODE);
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0x00, 1, pdata, 180, QUERY_MODE);

    /* unlock FDh */
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFE, 1, (uint8_t []){0xC5}, 1, QUERY_MODE);
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFD, 1, (uint8_t []){0x03}, 1, QUERY_MODE);

    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0x00, 1, pdata + 180, 171, QUERY_MODE);

    board_delay_ms(1);
    return true;
#else
    val = 95;
    for (int i = 0x90; i <= 0x9f; i++)
    {
        drv_is37xx_i2c_reg_write(IS3729_ADDR0, i, 1, &val, 1, QUERY_MODE);//SL
#if (IS3729_USE_NUM == 2)
        drv_is37xx_i2c_reg_write(IS3729_ADDR1, i, 1, &val, 1, QUERY_MODE);//SL
#endif
    }
    return true;
#endif
}

bool drv_is37xx_fill_pwm_reg_range(uint8_t id, uint8_t *pdata, uint16_t start_addr)
{
    //判断写入的范围属于IS3729_ADDR0(小于0x8F)还是IS3729_ADDR1(大于等于0x90),还是两者都有,分别处理,直接写入,不用等待模式
#ifdef IS3741
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFE, 1, (uint8_t []){0xC5}, 1, QUERY_MODE);

    if (start_addr < 180)
    {
        /* write page zero */
        drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFD, 1, (uint8_t []){0x0}, 1, QUERY_MODE);
    }
    else
    {
        /* write page one */
        start_addr -= 180;
        drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFD, (uint8_t []){0x1}, 1, QUERY_MODE);
    }
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, start_addr, 1, pdata, 3, QUERY_MODE);
#else
    if (!id)
    {
        drv_is37xx_i2c_reg_write(IS3729_ADDR0, start_addr, 1, pdata, 3, QUERY_MODE);
    }
    else
    {
        drv_is37xx_i2c_reg_write(IS3729_ADDR1, start_addr, 1, pdata, 3, QUERY_MODE);
    }
#endif

    return true;
}

bool drv_is37xx_fill_pwm_reg(uint8_t *pdata, uint8_t wait_mode)
{
  hpm_stat_t stat;
  uint8_t mode = !!wait_mode;

#ifdef IS3741
    /* unlock FDh */
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFE, 1, (uint8_t []){0xC5}, 1, mode);
    /* write page zero */
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFD, 1, (uint8_t []){0x0}, 1, mode);
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0x00, 1, pdata, 180, mode);

    /* unlock FDh */
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFE, 1, (uint8_t []){0xC5}, 1, mode);
    /* write page one */
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFD, 1, (uint8_t []){0x01}, 1, mode);
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0x00, 1, pdata + 180, 171, mode);
#else
    #if (IS3729_USE_NUM == 2)
        stat = drv_is37xx_i2c_reg_write(IS3729_ADDR1, 0x01, 1, pdata  + 1 + 0x90, 0x8F, mode);
        if (stat != status_success)
        {
            return false;
        }
    #endif
        stat = drv_is37xx_i2c_reg_write(IS3729_ADDR0, 0x01, 1, pdata + 1, 0x8F, mode);
        if (stat != status_success)
        {
            return false;
        }
#endif

    return true;
}

#ifdef IS3741
static bool is37xx_select_page(uint8_t pn)
{
    /* unlock FDh */
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFE, 1, (uint8_t []){0xC5}, 1, QUERY_MODE);
    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0xFD, 1, (uint8_t []){pn}, 1, QUERY_MODE);    /* write page one */

    return true;
}
#endif

static void is37xx_sw_shutdown(uint8_t addr, uint8_t data)
{
    drv_is37xx_i2c_reg_write(addr, 0xA0, 1, &data, 1, QUERY_MODE);
}

static void is37xx_set_pwm_freq(uint8_t addr, uint8_t freq)
{
    drv_is37xx_i2c_reg_write(addr, 0xB2, 1, &freq, 1, QUERY_MODE);
}


//软件打开灯控
void drv_is37xx_power_on(void)
{
    is37xx_set_config_reg(0x01);
}

//软件关闭灯控
void drv_is37xx_power_off(void)
{
    is37xx_set_config_reg(0x00);
}

void drv_is37xx_init(i2c_complete_handler handler)
{
     drv_is37xx_i2c_init(is37xx_i2c_config, handler);

     drv_is37xx_power_on();
     board_delay_ms(4);

 #ifdef IS3741
     is37xx_set_config_reg(0x01);
     is37xx_set_pull_reg(0x11);
     is37xx_set_current_reg(0xFF);
 #else
     is37xx_set_config_reg(0x09);
     is37xx_set_pull_reg(0x11);
     is37xx_set_current_reg(0x14);
     is37xx_set_pwm_freq(IS3729_ADDR0, 0x2);
     is37xx_set_pwm_freq(IS3729_ADDR1, 0x2);
#endif
}

void drv_is37xx_uninit(void)
{
    drv_is37xx_power_off();
}

void drv_is37xx_register_wait_trans_complete(is37xx_wait_large_data_trans pfunc)
{
    m_wait_trans_complete = pfunc;
}

bool drv_is37xx_i2c_bus_test(void)
{
#ifdef IS3741
    uint8_t write_data = 0xAA;
    uint8_t read_data  = 0;

    drv_is37xx_i2c_reg_write(WK_BOARD_ISSI_ADDR, 0x01, 1, &write_data, 1, QUERY_MODE);
    drv_is37xx_i2c_reg_read(WK_BOARD_ISSI_ADDR, 0x01, 1, &read_data, 1, QUERY_MODE);
    if (read_data != test_val)
    {
        return false;
    }
#else
    uint8_t write_data0 = 0x12;
    uint8_t write_data1 = 0x34;
    uint8_t read_data0 = 0;
    uint8_t read_data1 = 0;
    
    drv_is37xx_i2c_reg_write(IS3729_ADDR0, 0x01, 1, &write_data0, 1, QUERY_MODE);
    drv_is37xx_i2c_reg_write(IS3729_ADDR1, 0x01, 1, &write_data1, 1, QUERY_MODE);

    drv_is37xx_i2c_reg_read(IS3729_ADDR0, 0x01, 1, &read_data0, 1);
    drv_is37xx_i2c_reg_read(IS3729_ADDR1, 0x01, 1, &read_data1, 1);

    if ((read_data0 != write_data0) || (read_data1 != write_data1))
    {
        return false;
    }
#endif
    return true;
}