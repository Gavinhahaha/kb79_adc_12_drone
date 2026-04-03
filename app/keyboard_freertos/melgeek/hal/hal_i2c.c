/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include "hal_i2c.h"
#include "hpm_dmamux_drv.h"
#include "hpm_l1c_drv.h"
#include "hpm_dmav2_drv.h"
#include "app_debug.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "hal_iic"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#define I2C_DMA_TRANSFER_TIMEOUT_MS    (10UL)

static I2C_Type *i2c_base_ptr = NULL;
static uint32_t i2c_irq;
static i2c_complete_handler m_complete_handler = NULL;

#if BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_AW20XXX
#include "drv_aw20xxx.h"
#undef   BOARD_APP_I2C_IRQ
#define  BOARD_APP_I2C_IRQ  AW20XXX_I2C_IRQ
#elif  BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_IS37XX
#include "drv_is37xx.h"
#undef   BOARD_APP_I2C_IRQ
#define  BOARD_APP_I2C_IRQ  IS37XX_I2C_IRQ
#endif

static void i2c_isr(void)
{
    volatile uint32_t status;
    status = i2c_get_status(i2c_base_ptr);

    if (status & I2C_EVENT_TRANSACTION_COMPLETE) 
    {
        if (I2C_DIR_MASTER_WRITE == i2c_get_direction(i2c_base_ptr))
        {
            if (m_complete_handler)
            {
                i2c_disable_irq(i2c_base_ptr, I2C_EVENT_TRANSACTION_COMPLETE);
                i2c_clear_status(i2c_base_ptr, I2C_EVENT_TRANSACTION_COMPLETE);
                i2c_dma_disable(i2c_base_ptr); 
                m_complete_handler();
            }
        }
    }
}
SDK_DECLARE_EXT_ISR_M(BOARD_APP_I2C_IRQ, i2c_isr)

static hpm_stat_t i2c_tx_trigger_dma(DMA_Type *dma_ptr, uint8_t ch_num, I2C_Type *i2c_ptr, uint32_t src, uint32_t size)
{
    dma_handshake_config_t config;

    dma_default_handshake_config(dma_ptr, &config);
    config.ch_index = ch_num;
    config.dst = (uint32_t)&i2c_ptr->DATA;
    config.dst_fixed = true;
    config.src = src;
    config.src_fixed = false;
    config.data_width = DMA_TRANSFER_WIDTH_BYTE;
    config.size_in_byte = size;
    config.interrupt_mask = DMA_INTERRUPT_MASK_ALL;

    return dma_setup_handshake(dma_ptr, &config, true);
}

static hpm_stat_t i2c_rx_trigger_dma(DMA_Type *dma_ptr, uint8_t ch_num, I2C_Type *i2c_ptr, uint32_t dst, uint32_t size)
{
    dma_handshake_config_t config;

    dma_default_handshake_config(dma_ptr, &config);
    config.ch_index = ch_num;
    config.dst = dst;
    config.dst_fixed = false;
    config.src = (uint32_t)&i2c_ptr->DATA;
    config.src_fixed = true;
    config.data_width = DMA_TRANSFER_WIDTH_BYTE;
    config.size_in_byte = size;
    config.interrupt_mask = DMA_INTERRUPT_MASK_ALL;

    return dma_setup_handshake(dma_ptr, &config, true);
}

hpm_stat_t i2c_handle_dma_transfer_complete(I2C_Type *ptr)
{
    uint64_t ticks_per_ms = clock_get_core_clock_ticks_per_ms();
    uint64_t timeout_ticks = hpm_csr_get_core_cycle() + I2C_DMA_TRANSFER_TIMEOUT_MS * ticks_per_ms;

    volatile uint32_t status;
    do 
    {
        status = i2c_get_status(ptr);
        if (status & I2C_STATUS_CMPL_MASK) 
        {
            break;
        }
        if (hpm_csr_get_core_cycle() >= timeout_ticks) 
        {
            return status_timeout;
        }
    } while (1);

    i2c_clear_status(ptr, status);
    i2c_dma_disable(ptr);

    return status_success;
}

static void __attribute__((unused)) i2c_bus_clear(I2C_Type *ptr)
{
    if (i2c_get_line_scl_status(ptr) == false) {
        DBG_PRINTF("CLK is low, please power cycle the board\n");
        while (1) {
        }
    }
    if (i2c_get_line_sda_status(ptr) == false) {
        DBG_PRINTF("SDA is low, try to issue I2C bus clear\n");
    } else {
        DBG_PRINTF("I2C bus is ready\n");
        return;
    }
    i2c_gen_reset_signal(ptr, 9);
    board_delay_ms(100);
    DBG_PRINTF("I2C bus is cleared\n");
}

hpm_stat_t hal_i2c_init(I2C_Type *i2c_ptr, clock_name_t clock_name, i2c_config_t config,
                        DMAMUX_Type *dmamux_ptr, uint8_t dmamux_ch, uint8_t dmamux_src)
{
    hpm_stat_t stat;
    uint32_t freq;

    init_i2c_pins(i2c_ptr);
    //i2c_bus_clear(i2c_ptr);

    freq = clock_get_frequency(clock_name);
    stat = i2c_init_master(i2c_ptr, freq, &config);

    dmamux_config(dmamux_ptr, dmamux_ch, dmamux_src, true);

    return stat;
}

void hal_i2c_deinit(I2C_Type *i2c_ptr)
{
    (void)(i2c_ptr);
}

hpm_stat_t hal_i2c_write(I2C_Type *i2c_ptr, DMA_Type *dma_ptr, uint8_t dma_ch, uint8_t *data, uint32_t size, uint8_t slave_address, uint8_t wait_mode)
{
    hpm_stat_t stat;

    if (wait_mode == INTERRUPT_MODE)//((IS_I2C_DATA_LARGE(size)) && (wait_mode == INTERRUPT_MODE))
    {
        i2c_enable_irq(i2c_ptr, I2C_EVENT_TRANSACTION_COMPLETE);
    }

    stat = i2c_tx_trigger_dma(dma_ptr, dma_ch, i2c_ptr, core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)data), size);
    if (stat != status_success) {
        return stat;
    }
    stat = i2c_master_start_dma_write(i2c_ptr, slave_address, size);
    if (stat != status_success) {
        return stat;
    }
    
    if (wait_mode == QUERY_MODE)//((!IS_I2C_DATA_LARGE(size)) || (wait_mode == QUERY_MODE))
    {
        if (i2c_handle_dma_transfer_complete(i2c_ptr) != status_success)
        {
            return status_timeout;
        }
    }

    return status_success;
}

hpm_stat_t hal_i2c_read(I2C_Type *i2c_ptr, DMA_Type *dma_ptr, uint8_t dma_ch, uint8_t *data, uint32_t size, uint8_t slave_address)
{
    hpm_stat_t stat;

    stat = i2c_rx_trigger_dma(dma_ptr, dma_ch, i2c_ptr, core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)data), size);
    if (stat != status_success) {
        return stat;
    }
    stat = i2c_master_start_dma_read(i2c_ptr, slave_address, size);
    if (stat != status_success) {
        return stat;
    }

    if (i2c_handle_dma_transfer_complete(i2c_ptr) != status_success)
    {
        return status_timeout;
    }

    return status_success;
}

void hal_i2c_irq_init(I2C_Type *i2c_ptr, uint32_t irq, i2c_complete_handler handler)
{
    i2c_base_ptr       = i2c_ptr;
    i2c_irq            = irq;
    m_complete_handler = handler;

    
    i2c_disable_irq(i2c_base_ptr, I2C_EVENT_TRANSACTION_COMPLETE);

    intc_m_enable_irq_with_priority(i2c_irq, 1);
}

