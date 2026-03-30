/*
 * Copyright (c) 2021 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "hal_spi.h"
#include "board.h"
#include "hpm_gpio_drv.h"
#include "hpm_dma_mgr.h"
#include "hpm_crc_drv.h"
#include "app_debug.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #undef  LOG_TAG
    #define LOG_TAG          "hal_spi"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)     ((void)0)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)     ((void)0)
#endif

#define SPI_DMA_ISR_EN      (0)


static spi_device_config_t g_spi_cfg[SM_BUS_MAX] = {
    {
        .base = BOARD_SPI1_BASE,
        .dma_res = NULL,
    },
    {
        .base = BOARD_SPI2_BASE,
        .dma_res = NULL,
    }
};

static uint32_t spi_cs_pins[SM_DEV_NUM_MAX] = {
    BOARD_SPI_CS3_PIN,
    BOARD_SPI_CS0_PIN,
    BOARD_SPI_CS2_PIN,
    BOARD_SPI_CS1_PIN,
    BOARD_SPI_CS4_PIN,
};

static spi_rx_cb spi1_rx_cb = NULL;
static spi_rx_cb spi2_rx_cb = NULL;
ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(ADC_SOC_DMA_ADDR_ALIGNMENT)
static uint16_t spi1_rx_buf[SCAN_CH_EN_MAX + SPI_CRC_LEN] = {0}; //+1 for crc
ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(ADC_SOC_DMA_ADDR_ALIGNMENT)
static uint16_t spi2_rx_buf[SCAN_CH_EN_MAX + SPI_CRC_LEN] = {0};
uint8_t spi1_rx_busy = 0;
uint8_t spi2_rx_busy = 0;

uint32_t test_spi1_busy_cnt = 0;
uint32_t test_spi2_busy_cnt = 0;

uint32_t test_spi1_crc_err = 0;
uint32_t test_spi2_crc_err = 0;

static spi_msg_t spi1_msg = 
{
    .bus_num = SM_BUS_0,
    .p_rx_buf = spi1_rx_buf,
    .rx_len = 0,
    .remain_num = 0,
    .is_crc_err = false,
};

static spi_msg_t spi2_msg = 
{
    .bus_num = SM_BUS_1,
    .p_rx_buf = spi2_rx_buf,
    .rx_len = 0,
    .remain_num = 0,
    .is_crc_err = false,
};

static crc_channel_config_t spi1_crc_cfg = {
    .preset = crc_preset_none,
    .in_byte_order = crc_in_byte_order_lsb,
    .init = 0x0000,
    .poly = 0x8005,
    .poly_width = CRC_POLY_WIDTH_16,
    .refout = crc_refout_false,
    .refin = crc_refin_false,
    .xorout = 0x0000,
};

static crc_channel_config_t spi2_crc_cfg = {
    .preset = crc_preset_none,
    .in_byte_order = crc_in_byte_order_lsb,
    .init = 0x0000,
    .poly = 0x8005,  // 确保与从设备一致
    .poly_width = CRC_POLY_WIDTH_16,
    .refout = crc_refout_false,
    .refin = crc_refin_false,
    .xorout = 0x0000,
};


#if (SPI_DMA_ISR_EN == 0)
SDK_DECLARE_EXT_ISR_M(BOARD_SPI1_IRQ, spi1_isr)
void spi1_isr(void)
{
    volatile uint32_t irq_status = spi_get_interrupt_status(BOARD_SPI1_BASE); /* get interrupt stat */

    if (irq_status & spi_end_int) 
    {
        spi_clear_interrupt_status(BOARD_SPI1_BASE, spi_end_int);

        board_delay_us(1);
        spi1_rx_busy = 0;
        spi_msg_t *p_msg = &spi1_msg;
        hal_spi_write_cs(p_msg->cur_cs_num, !BOARD_SPI_CS_ACTIVE_LEVEL);
    
        if (spi1_rx_cb != NULL)
        {
            #if SPI_CRC_EN
            uint8_t data_len = p_msg->rx_len - SPI_CRC_LEN;
            uint16_t crc_value = p_msg->p_rx_buf[p_msg->rx_len - SPI_CRC_LEN];

            crc_setup_channel_config(HPM_CRC, CRC_CHN_0, &spi1_crc_cfg);
            crc_calc_block_half_words(HPM_CRC, CRC_CHN_0, (uint16_t *)p_msg->p_rx_buf, data_len);
            uint16_t crc_result = crc_get_result(HPM_CRC, CRC_CHN_0);

            if (crc_result != crc_value)
            {
                test_spi1_crc_err++;
                p_msg->is_crc_err = true;
            }
            p_msg->rx_len -= SPI_CRC_LEN; //去掉crc
            #endif
            spi1_rx_cb(p_msg);
        }
    }
}

SDK_DECLARE_EXT_ISR_M(BOARD_SPI2_IRQ, spi2_isr)
void spi2_isr(void)
{
    volatile uint32_t irq_status = spi_get_interrupt_status(BOARD_SPI2_BASE); /* get interrupt stat */

    if (irq_status & spi_end_int) 
    {
        spi_clear_interrupt_status(BOARD_SPI2_BASE, spi_end_int);

        board_delay_us(1);
        spi2_rx_busy = 0;
        spi_msg_t *p_msg = &spi2_msg;
        hal_spi_write_cs(p_msg->cur_cs_num, !BOARD_SPI_CS_ACTIVE_LEVEL);
    
        if (spi2_rx_cb != NULL)
        {
            #if SPI_CRC_EN
            uint8_t data_len = p_msg->rx_len - SPI_CRC_LEN;
            uint16_t crc_value = p_msg->p_rx_buf[p_msg->rx_len - SPI_CRC_LEN];

            crc_setup_channel_config(HPM_CRC, CRC_CHN_1, &spi2_crc_cfg);
            crc_calc_block_half_words(HPM_CRC, CRC_CHN_1, (uint16_t *)p_msg->p_rx_buf, data_len);
            uint16_t crc_result = crc_get_result(HPM_CRC, CRC_CHN_1);

            if (crc_result != crc_value)
            {
                test_spi2_crc_err++;
                p_msg->is_crc_err = true;
            }
            p_msg->rx_len -= SPI_CRC_LEN; //去掉crc
            #endif
            spi2_rx_cb(p_msg);
        }
    }
}
#endif
void spi_clear(void)
{
    spi1_rx_busy = 0;
    spi1_msg.remain_num = 0;
    spi1_msg.cur_cs_num = 0;
    spi1_msg.is_crc_err = false;
    spi1_msg.rx_len = 0;
    memset(spi1_msg.p_rx_buf,0,sizeof(spi1_rx_buf));

    spi2_rx_busy = 0;
    spi2_msg.remain_num = 0;
    spi2_msg.cur_cs_num = 0;
    spi2_msg.is_crc_err = false;
    spi2_msg.rx_len = 0;
    memset(spi2_msg.p_rx_buf,0,sizeof(spi2_rx_buf));
}


static void spi1_rxdma_complete_cb(uint32_t channel)
{
    dma_clear_transfer_status(HPM_HDMA, channel);
    spi1_rx_busy = 0;
    spi_msg_t *p_msg = &spi1_msg;
    hal_spi_write_cs(p_msg->cur_cs_num, !BOARD_SPI_CS_ACTIVE_LEVEL);

    if (spi1_rx_cb != NULL)
    {
        spi1_rx_cb(p_msg);
    }
}

static void spi2_rxdma_complete_cb(uint32_t channel)
{
    dma_clear_transfer_status(HPM_HDMA, channel);
    spi2_rx_busy = 0;
    spi_msg_t *p_msg = &spi2_msg;
    hal_spi_write_cs(p_msg->cur_cs_num, !BOARD_SPI_CS_ACTIVE_LEVEL);

    if (spi2_rx_cb != NULL)
    {
        spi2_rx_cb(p_msg);
    }
}

void hal_spi_write_cs(uint32_t cs_num, uint8_t state)
{
    uint32_t pin = spi_cs_pins[cs_num];
    gpio_write_pin(BOARD_SPI_CS_GPIO_CTRL, GPIO_GET_PORT_INDEX(pin), GPIO_GET_PIN_INDEX(pin), state);
}

static uint32_t bolock_err_cnt = 0;
uint8_t hal_spi_receive(spi_cs_num_t cs_num, uint32_t rx_len)
{
    spi_msg_t *p_msg = &spi1_msg;
    uint8_t p_busy  = spi1_rx_busy;
    spi_device_config_t *p_spi_cfg = &g_spi_cfg[SM_BUS_0];
    
    if (cs_num < SM_DEV_NUM_2) //spi2
    {
        p_busy = spi2_rx_busy;
        if (p_busy) 
        {
            test_spi2_busy_cnt++;
            goto END;
        }
        p_msg = &spi2_msg;
        p_spi_cfg = &g_spi_cfg[SM_BUS_1];
    }
    else if (cs_num < SM_DEV_NUM_MAX) //spi1
    {
        if (p_busy) 
        {
            test_spi1_busy_cnt++;
            goto END;
        }
    }
    else 
    {
        goto END;
    }

    uint8_t buf_size = SCAN_CH_EN_MAX + SPI_CRC_LEN;
    if (rx_len > buf_size)
    {
        rx_len = buf_size;
    }
    
    p_msg->rx_len     = rx_len;
    p_msg->cur_cs_num = cs_num;
    memset(p_msg->p_rx_buf,0,SPI_RX_KEY_MAX * 2);

    hal_spi_write_cs(cs_num, BOARD_SPI_CS_ACTIVE_LEVEL);
    board_delay_us(1); // 延迟1μs，确保从设备稳定选通
    spi_clear_interrupt_status(p_spi_cfg->base, spi_rx_fifo_overflow_int | spi_tx_fifo_underflow_int);
    spi_receive_fifo_reset(p_spi_cfg->base);
    uint32_t state = hpm_spi_receive_nonblocking(p_spi_cfg->base, (uint8_t *)p_msg->p_rx_buf, p_msg->rx_len * 2);
    if (state != status_success) 
    {
        p_busy = 0;
        bolock_err_cnt++;
        goto END;
    }
    p_busy = 1;
    return 0;

END:
    hal_spi_write_cs(cs_num, !BOARD_SPI_CS_ACTIVE_LEVEL);
    return 1;
}


void hal_spi_set_rx_callback(spi_rx_cb spi1_cb, spi_rx_cb spi2_cb)
{
    spi1_rx_cb = spi1_cb;
    spi2_rx_cb = spi2_cb;
}


void hal_spi_init(void)
{
    uint8_t rx_fifo_max = 0;
    spi_initialize_config_t init_config;
    // spi1
    hpm_spi_get_default_init_config(&init_config);
    board_init_spi_clock(BOARD_SPI1_BASE);
    board_init_spi_pins_with_gpio_as_cs(BOARD_SPI1_BASE);
    init_config.mode = spi_master_mode;
    init_config.clk_phase = spi_sclk_sampling_even_clk_edges;
    init_config.clk_polarity = spi_sclk_low_idle;
    init_config.data_len = BOARD_SPI1_DATA_LEN_IN_BITS;
    /* step.1  initialize spi */
    if (hpm_spi_initialize(BOARD_SPI1_BASE, &init_config) != status_success) {
        DBG_PRINTF("hpm_spi_initialize fail\n");
        while (1) {
        }
    }
    /* step.2  set spi sclk frequency for master */
    if (hpm_spi_set_sclk_frequency(BOARD_SPI1_BASE, BOARD_SPI1_SCLK_FREQ) != status_success) {
        DBG_PRINTF("hpm_spi_set_sclk_frequency fail\n");
        while (1) {
        }
    }
    /* step.3 install dma callback if want use dma */
    if (hpm_spi_dma_mgr_install_callback(BOARD_SPI1_BASE, NULL, spi1_rxdma_complete_cb) != status_success) {
        DBG_PRINTF("hpm_spi_dma_install_callback fail\n");
        while (1) {
        }
    }

    rx_fifo_max = spi_get_rx_fifo_size(BOARD_SPI1_BASE);
    spi_set_rx_fifo_threshold(BOARD_SPI1_BASE, rx_fifo_max * 2 / 3);

    // spi2
    hpm_spi_get_default_init_config(&init_config);
    board_init_spi_clock(BOARD_SPI2_BASE);
    board_init_spi_pins_with_gpio_as_cs(BOARD_SPI2_BASE);
    init_config.mode = spi_master_mode;
    init_config.clk_phase = spi_sclk_sampling_even_clk_edges;
    init_config.clk_polarity = spi_sclk_low_idle;
    init_config.data_len = BOARD_SPI2_DATA_LEN_IN_BITS;
    /* step.1  initialize spi */
    if (hpm_spi_initialize(BOARD_SPI2_BASE, &init_config) != status_success) {
        DBG_PRINTF("hpm_spi_initialize fail\n");
        while (1) {
        }
    }
    /* step.2  set spi sclk frequency for master */
    if (hpm_spi_set_sclk_frequency(BOARD_SPI2_BASE, BOARD_SPI2_SCLK_FREQ) != status_success) {
        DBG_PRINTF("hpm_spi_set_sclk_frequency fail\n");
        while (1) {
        }
    }
    /* step.3 install dma callback if want use dma */
    if (hpm_spi_dma_mgr_install_callback(BOARD_SPI2_BASE, NULL, spi2_rxdma_complete_cb) != status_success) {
        DBG_PRINTF("hpm_spi_dma_install_callback fail\n");
        while (1) {
        }
    }

    rx_fifo_max = spi_get_rx_fifo_size(BOARD_SPI2_BASE);
    spi_set_rx_fifo_threshold(BOARD_SPI2_BASE, rx_fifo_max * 2 / 3);

    g_spi_cfg[SM_BUS_0].base = BOARD_SPI1_BASE;
    g_spi_cfg[SM_BUS_0].dma_res = hpm_spi_get_rx_dma_resource(BOARD_SPI1_BASE);

    g_spi_cfg[SM_BUS_1].base = BOARD_SPI2_BASE;
    g_spi_cfg[SM_BUS_1].dma_res = hpm_spi_get_rx_dma_resource(BOARD_SPI2_BASE);

#if (SPI_DMA_ISR_EN == 0)
    dma_disable_channel_interrupt(g_spi_cfg[SM_BUS_0].dma_res->base, g_spi_cfg[SM_BUS_0].dma_res->channel, DMA_MGR_INTERRUPT_MASK_ALL);
    dma_disable_channel_interrupt(g_spi_cfg[SM_BUS_1].dma_res->base, g_spi_cfg[SM_BUS_1].dma_res->channel, DMA_MGR_INTERRUPT_MASK_ALL);

    intc_m_disable_irq(g_spi_cfg[SM_BUS_0].dma_res->irq_num);
    intc_m_disable_irq(g_spi_cfg[SM_BUS_1].dma_res->irq_num);

    spi_enable_interrupt(BOARD_SPI1_BASE, spi_end_int);
    spi_enable_interrupt(BOARD_SPI2_BASE, spi_end_int);
    
    intc_m_enable_irq_with_priority(BOARD_SPI1_IRQ, 2);
    intc_m_enable_irq_with_priority(BOARD_SPI2_IRQ, 3);

#endif

}

