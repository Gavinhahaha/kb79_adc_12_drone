/*
 * Copyright (c) 2021-2022 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#define DBG_TAG "drv ws2812"

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "board.h"
#include "hpm_soc.h"
#include "hpm_gpio_drv.h"
#include "hpm_spi_drv.h"
#include "hpm_dmamux_drv.h"
#include "hpm_dmav2_drv.h"
#include "hpm_clock_drv.h"

#include "drv_ws2812.h"

#define APP_SPI_BASE                 HPM_SPI3
#define APP_SPI_CLK_NAME             clock_spi3
#define APP_SPI_IRQ                  IRQn_SPI3
#define APP_SPI_SCLK_FREQ            (6000000UL) // 6MHZ 1tick=160ns, 0:160*2=320ns, 11000000= 0xC0; 1:160*4= 640ns, 11110000=0xF0
#define APP_SPI_ADDR_LEN_IN_BYTES    (1U)
#define APP_SPI_DATA_LEN_IN_BITS     (8U)
#define APP_SPI_DATA_LEN_IN_BYTE     (1U)
#define APP_SPI_RX_DMA               HPM_DMA_SRC_SPI3_RX
#define APP_SPI_TX_DMA               HPM_DMA_SRC_SPI3_TX

#define SPI_DMA                      HPM_HDMA
#define SPI_DMAMUX                   HPM_DMAMUX
#define SPI_TX_DMA_REQ               HPM_DMA_SRC_SPI3_TX
#define SPI_TX_DMA_CH                16
#define SPI_TX_DMAMUX_CH             DMA_SOC_CHN_TO_DMAMUX_CHN(HPM_HDMA, SPI_TX_DMA_CH)

#define USE_WS2812_PWR_CTRL          0

#define WS2812_0            0xC0
#define WS2812_1            0xF0
#define WS2812_RST          0x00
#define RGB_BIT             24

ATTR_PLACE_AT_NONCACHEABLE static uint8_t send_buff[(RGB_BIT * BOARD_LIGHT_TAPE_LED_NUM) + 1];

/**
create 24 byte sent by SPI using RGB values.
*/
static void ws2812_convert_data(uint8_t R, uint8_t G, uint8_t B, uint8_t *covert_buff)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        covert_buff[7 - i] = (G & 0x01) ? WS2812_1 : WS2812_0;
        G = G >> 1;
    }
    for (uint8_t i = 0; i < 8; i++)
    {
        covert_buff[15 - i] = (R & 0x01) ? WS2812_1 : WS2812_0;
        R = R >> 1;
    }
    for (uint8_t i = 0; i < 8; i++)
    {
        covert_buff[23 - i] = (B & 0x01) ? WS2812_1 : WS2812_0;
        B = B >> 1;
    }
}

static hpm_stat_t drv_ws2812_trigger_dma(DMA_Type *dma_ptr, uint8_t ch_num, SPI_Type *spi_ptr, uint32_t src, uint8_t data_width, uint32_t size)
{
    dma_handshake_config_t config;

    dma_default_handshake_config(dma_ptr, &config);
    config.ch_index = ch_num;
    config.dst = (uint32_t)&spi_ptr->DATA;
    config.dst_fixed = true;
    config.src = src;
    config.src_fixed = false;
    config.data_width = data_width;
    config.size_in_byte = size;
    config.interrupt_mask = DMA_INTERRUPT_MASK_ALL;

    return dma_setup_handshake(dma_ptr, &config, true);
}

void drv_ws2812_pin_init(void)
{
    #if USE_WS2812_PWR_CTRL
    
    // RGB EN PY05
    HPM_PIOC->PAD[IOC_PAD_PY05].FUNC_CTL = IOC_PY05_FUNC_CTL_SOC_GPIO_Y_05;
    HPM_IOC->PAD[IOC_PAD_PY05].FUNC_CTL  = IOC_PY05_FUNC_CTL_GPIO_Y_05;

    gpio_set_pin_output(HPM_GPIO0, GPIO_DO_GPIOY, 5);
    gpio_write_pin(HPM_GPIO0, GPIO_DO_GPIOY, 5, 0);

    #endif
    

    // RGB PIN PY07
    //HPM_PIOC->PAD[IOC_PAD_PY07].FUNC_CTL = IOC_PY07_FUNC_CTL_SOC_GPIO_Y_07;
    //HPM_IOC->PAD[IOC_PAD_PY07].FUNC_CTL = IOC_PY07_FUNC_CTL_SPI2_MOSI;

    HPM_IOC->PAD[IOC_PAD_PA13].FUNC_CTL = IOC_PA13_FUNC_CTL_SPI3_MOSI;


}

void drv_ws2812_init(void)
{
    spi_timing_config_t timing_config = {0};
    spi_format_config_t format_config = {0};
    spi_control_config_t control_config = {0};
    uint32_t spi_clcok;
    uint32_t spi_tx_trans_count;

    // ws2812_pin_init();
    // // GRB EN on
    // gpio_set_pin_output(HPM_GPIO0, GPIO_DO_GPIOY, 5);
    // //gpio_write_pin_extra(GPIO_DO_GPIOY, 5, 0);
    // gpio_write_pin(HPM_GPIO0, GPIO_DO_GPIOY, 5, 0);

    // GRB DATA, spi
    clock_add_to_group(APP_SPI_CLK_NAME, 0);
    clock_set_source_divider(APP_SPI_CLK_NAME, clk_src_pll1_clk1, 111U); /* 666/111 = 6MHz */
    spi_clcok = clock_get_frequency(APP_SPI_CLK_NAME);

    /* set SPI sclk frequency for master */
    spi_master_get_default_timing_config(&timing_config);
    timing_config.master_config.clk_src_freq_in_hz = spi_clcok;
    timing_config.master_config.sclk_freq_in_hz = (6006006UL);

    spi_master_timing_init(APP_SPI_BASE, &timing_config);

    /* set SPI format config for master */
    spi_master_get_default_format_config(&format_config);
    format_config.master_config.addr_len_in_bytes = APP_SPI_ADDR_LEN_IN_BYTES;
    format_config.common_config.data_len_in_bits = APP_SPI_DATA_LEN_IN_BITS;
    format_config.common_config.data_merge = false;
    format_config.common_config.mosi_bidir = false;
    format_config.common_config.lsb = false;
    format_config.common_config.mode = spi_master_mode;
    format_config.common_config.cpol = spi_sclk_low_idle;
    format_config.common_config.cpha = spi_sclk_sampling_even_clk_edges;
    spi_format_init(APP_SPI_BASE, &format_config);

    for (unsigned i = 0; i < (unsigned)BOARD_LIGHT_TAPE_LED_NUM; i++)
    {
        ws2812_convert_data(0, 0, 0, &send_buff[i * RGB_BIT]);
    }
    send_buff[(RGB_BIT * BOARD_LIGHT_TAPE_LED_NUM)] = WS2812_RST;

    /* set SPI control config for master */
    spi_master_get_default_control_config(&control_config);
    control_config.master_config.cmd_enable = false;
    control_config.master_config.addr_enable = false;
    control_config.master_config.addr_phase_fmt = spi_address_phase_format_single_io_mode;
    control_config.common_config.tx_dma_enable = true;
    control_config.common_config.rx_dma_enable = false;
    control_config.common_config.trans_mode = spi_trans_write_only;
    control_config.common_config.data_phase_fmt = spi_single_io_mode;
    control_config.common_config.dummy_cnt = spi_dummy_count_1;

    spi_tx_trans_count = sizeof(send_buff) / APP_SPI_DATA_LEN_IN_BYTE;

    spi_setup_dma_transfer(APP_SPI_BASE,
                           &control_config,
                           NULL, 
                           NULL,
                           spi_tx_trans_count, 1);

    dmamux_config(SPI_DMAMUX, SPI_TX_DMAMUX_CH, SPI_TX_DMA_REQ, true);

    //memset(send_buff, 0x00, sizeof(send_buff));

    drv_ws2812_trigger_dma(SPI_DMA,
                           SPI_TX_DMA_CH,
                           APP_SPI_BASE,
                           core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)send_buff),
                           DMA_TRANSFER_WIDTH_BYTE,
                           sizeof(send_buff));
}

void drv_ws2812_power(bool enable)
{
    #if USE_WS2812_PWR_CTRL
    if (enable)
    {
        gpio_write_pin_extra(GPIO_DO_GPIOY, 5, 1);
        gpio_write_pin(HPM_GPIO0, GPIO_DO_GPIOY, 5, 1);
    }
    else
    {
        gpio_write_pin_extra(GPIO_DO_GPIOY, 5, 0);
        gpio_write_pin(HPM_GPIO0, GPIO_DO_GPIOY, 5, 0);
    }
    #endif
}

void drv_ws2812_set_leds(rgb_t *rgb_array, uint32_t count)
{
    uint32_t spi_tx_trans_count;
    uint32_t i;
    spi_control_config_t control_config = {0};

    if (dma_check_transfer_status(SPI_DMA, SPI_TX_DMA_CH) == DMA_CHANNEL_STATUS_ONGOING)
    {
        dma_abort_channel(SPI_DMA, SPI_TX_DMA_CH);
    }

    for (i = 0; i < count; i++)
    {
        ws2812_convert_data(rgb_array[i].r, rgb_array[i].g, rgb_array[i].b, &send_buff[i * RGB_BIT]);
    }
    send_buff[(RGB_BIT * BOARD_LIGHT_TAPE_LED_NUM)] = WS2812_RST;

    spi_master_get_default_control_config(&control_config);
    control_config.master_config.cmd_enable = false;
    control_config.master_config.addr_enable = false;
    control_config.master_config.addr_phase_fmt = spi_address_phase_format_single_io_mode;
    control_config.common_config.tx_dma_enable = true;
    control_config.common_config.rx_dma_enable = false;
    control_config.common_config.trans_mode = spi_trans_write_only;
    control_config.common_config.data_phase_fmt = spi_single_io_mode;
    control_config.common_config.dummy_cnt = spi_dummy_count_1;

    spi_tx_trans_count = sizeof(send_buff) / APP_SPI_DATA_LEN_IN_BYTE;

    spi_setup_dma_transfer(APP_SPI_BASE,
                           &control_config,
                           NULL, NULL,
                           spi_tx_trans_count, 1);

    drv_ws2812_trigger_dma(SPI_DMA,
                           SPI_TX_DMA_CH,
                           APP_SPI_BASE,
                           core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)send_buff),
                           DMA_TRANSFER_WIDTH_BYTE,
                           sizeof(send_buff));
    #if 1
    while (spi_is_active(APP_SPI_BASE)) {
    }
    spi_disable_tx_dma(APP_SPI_BASE);
    #endif
}
