/*
 * Copyright (c) 2024-2025 Melgeek, All Rights Reserved.
 *
 * Description:
 *
 */

#include "hal_uart.h"
#include "board.h"
#include "hpm_dmamux_drv.h"
#include "hpm_gpio_drv.h"
#include "app_debug.h"

#if DEBUG_EN == 1
#define MODULE_LOG_ENABLED (0)
#if MODULE_LOG_ENABLED == 1
#define LOG_TAG "hal_uart"
#define DEG_UART(fmt, ...) DEBUG_RTT(LOG_TAG, fmt, ##__VA_ARGS__)
#else
#define DEG_UART(fmt, ...)
#endif
#else
#define DEG_UART(fmt, ...)
#endif

static const uint8_t reset_pin[UART_DEVICE_DPM][2] =
{
    {GPIO_DI_GPIOY, 0},
    {GPIO_DI_GPIOY, 1},
    {GPIO_DI_GPIOY, 2},
    {GPIO_DI_GPIOY, 3},
    {GPIO_DI_GPIOA, 10}
};

static const uart_device_config_t uart_group_configs[UART_DEVICE_MAX] = {
    /* UART0 配置 */
    {
        .base = HPM_UART0,
        .irq = IRQn_UART0,
        .baudrate = UART_BAUDRATE_GROUP1,
        .clock_name = clock_uart0,
        .rx_dma_channel = 20,
        .tx_dma_channel = 21,
        .rx_dma_request = HPM_DMA_SRC_UART0_RX,
        .tx_dma_request = HPM_DMA_SRC_UART0_TX,
        .tx_dma_cb = NULL,
        .tx_cb = NULL
    },
    /* UART1 配置 */
    {
        .base = HPM_UART1,
        .irq = IRQn_UART1,
        .baudrate = UART_BAUDRATE_GROUP1,
        .clock_name = clock_uart1,
        .rx_dma_channel = 22,
        .tx_dma_channel = 23,
        .rx_dma_request = HPM_DMA_SRC_UART1_RX,
        .tx_dma_request = HPM_DMA_SRC_UART1_TX,
        .tx_dma_cb = NULL,
        .tx_cb = NULL
    },
    /* UART2 配置 */
    {
        .base = HPM_UART2,
        .irq = IRQn_UART2,
        .baudrate = UART_BAUDRATE_GROUP1,
        .clock_name = clock_uart2,
        .rx_dma_channel = 24,
        .tx_dma_channel = 25,
        .rx_dma_request = HPM_DMA_SRC_UART2_RX,
        .tx_dma_request = HPM_DMA_SRC_UART2_TX,
        .tx_dma_cb = NULL,
        .tx_cb = NULL
    },
    /* UART3 配置 */
    {
        .base = HPM_UART3,
        .irq = IRQn_UART3,
        .baudrate = UART_BAUDRATE_GROUP1,
        .clock_name = clock_uart3,
        .rx_dma_channel = 26,
        .tx_dma_channel = 27,
        .rx_dma_request = HPM_DMA_SRC_UART3_RX,
        .tx_dma_request = HPM_DMA_SRC_UART3_TX,
        .tx_dma_cb = NULL,
        .tx_cb = NULL
    },
    /* UART4 配置 */
    {
        .base = HPM_UART4,
        .irq = IRQn_UART4,
        .baudrate = UART_BAUDRATE_GROUP1,
        .clock_name = clock_uart4,
        .rx_dma_channel = 28,
        .tx_dma_channel = 29,
        .rx_dma_request = HPM_DMA_SRC_UART4_RX,
        .tx_dma_request = HPM_DMA_SRC_UART4_TX,
        .tx_dma_cb = NULL,
        .tx_cb = NULL
    },
    /* UART5 配置 */
    {
        .base = HPM_UART5,
        .irq = IRQn_UART5,
        .baudrate = UART_BAUDRATE_GROUP2,
        .clock_name = clock_uart5,
        .rx_dma_channel = 30,
        .tx_dma_channel = 31,
        .rx_dma_request = HPM_DMA_SRC_UART5_RX,
        .tx_dma_request = HPM_DMA_SRC_UART5_TX,
        .tx_dma_cb = NULL,
        .tx_cb = NULL
    }
};


/* 存储所有串口设备配置 */
static uart_device_config_t g_uart_devices[UART_DEVICE_MAX];

void uart_irq_handler(uart_device_index_t index)
{
    uart_device_config_t *config = &g_uart_devices[index];

    if (uart_is_rxline_idle(config->base))
    {
        dma_disable_channel(BOARD_APP_HDMA, config->rx_dma_channel);

        if (config->rx_cb)
        {
            uint32_t remaining = dma_get_remaining_transfer_size(BOARD_APP_HDMA, config->rx_dma_channel);
            config->rx_cb(index, remaining);
        }

        uart_clear_rxline_idle_flag(config->base);
        uart_flush(config->base);
    }
}

void uart_dpm_irq_handler(uart_device_index_t index)
{
    uart_device_config_t *config = &g_uart_devices[index];

    if (uart_is_rxline_idle(config->base))
    {
        dma_disable_channel(BOARD_APP_HDMA, config->rx_dma_channel);

        if (config->rx_cb)
        {
            uint32_t remaining = dma_get_remaining_transfer_size(BOARD_APP_HDMA, config->rx_dma_channel);
            config->rx_cb(index, remaining);
        }

        uart_clear_rxline_idle_flag(config->base);
        uart_flush(config->base);
    }
    if (uart_is_txline_idle(config->base))
    {
        dma_disable_channel(BOARD_APP_HDMA, config->tx_dma_channel);

        if (config->tx_cb)
        {
            config->tx_cb(index, 0);
        }

        uart_clear_txline_idle_flag(config->base);
        uart_flush(config->base);
    }
}

SDK_DECLARE_EXT_ISR_M(IRQn_UART0, uart0_isr)
void uart0_isr(void) { uart_irq_handler(UART_DEVICE_0); }

SDK_DECLARE_EXT_ISR_M(IRQn_UART1, uart1_isr)
void uart1_isr(void) { uart_irq_handler(UART_DEVICE_1); }

SDK_DECLARE_EXT_ISR_M(IRQn_UART2, uart2_isr)
void uart2_isr(void) { uart_irq_handler(UART_DEVICE_2); }

SDK_DECLARE_EXT_ISR_M(IRQn_UART3, uart3_isr)
void uart3_isr(void) { uart_irq_handler(UART_DEVICE_3); }

SDK_DECLARE_EXT_ISR_M(IRQn_UART4, uart4_isr)
void uart4_isr(void) { uart_irq_handler(UART_DEVICE_4); }

SDK_DECLARE_EXT_ISR_M(IRQn_UART5, uart5_isr)
void uart5_isr(void) { uart_dpm_irq_handler(UART_DEVICE_DPM); }


void dma_irq_handler(uart_device_index_t index, uart_dma_direction_t direction)
{
    uart_device_config_t *config = &g_uart_devices[index];

    if (direction == UART_DMA_TX && config->tx_dma_cb)
    {
        config->tx_dma_cb();
    }
    // else if (direction == UART_DMA_RX && config->rx_cb)
    // {
    //     // config->rx_cb(0); //  表示传输完成
    // }
}


SDK_DECLARE_EXT_ISR_M(BOARD_APP_HDMA_IRQ, dma_isr)
void dma_isr(void)
{
    for (int i = 0; i < UART_DEVICE_MAX; i++)
    {
        // /* 检查 RX DMA 通道状态 */
        // volatile hpm_stat_t rx_status = dma_check_transfer_status(
        //     BOARD_APP_HDMA, g_uart_devices[i].rx_dma_channel);

        // if (rx_status & DMA_CHANNEL_STATUS_TC)
        // {
        //     dma_irq_handler(i, UART_DMA_RX);
        // }

        /* 检查 TX DMA 通道状态 */
        volatile hpm_stat_t tx_status = dma_check_transfer_status(
            BOARD_APP_HDMA, g_uart_devices[i].tx_dma_channel);

        if (tx_status & DMA_CHANNEL_STATUS_TC)
        {
            dma_irq_handler(i, UART_DMA_TX);
        }
    }
}

static hpm_stat_t uart_tx_trigger_dma(UART_Type *uart_ptr, uint8_t ch_num,
                                      uint32_t src, uint32_t size)
{
    dma_handshake_config_t config;
    dma_default_handshake_config(BOARD_APP_HDMA, &config);

    config.ch_index = ch_num;
    config.dst = (uint32_t)&uart_ptr->THR;
    config.dst_fixed = true;
    config.src = src;
    config.src_fixed = false;
    config.data_width = DMA_TRANSFER_WIDTH_BYTE;
    config.size_in_byte = size;

    return dma_setup_handshake(BOARD_APP_HDMA, &config, true);
}

static hpm_stat_t uart_rx_trigger_dma(UART_Type *uart_ptr, uint8_t ch_num,
                                      uint32_t dst, uint32_t size)
{
    dma_handshake_config_t config;
    dma_default_handshake_config(BOARD_APP_HDMA, &config);

    config.ch_index = ch_num;
    config.dst = dst;
    config.dst_fixed = false;
    config.src = (uint32_t)&uart_ptr->RBR;
    config.src_fixed = true;
    config.data_width = DMA_TRANSFER_WIDTH_BYTE;
    config.size_in_byte = size;

    return dma_setup_handshake(BOARD_APP_HDMA, &config, true);
}


hpm_stat_t hal_uart_start_dma_rx(uart_device_index_t index, uint8_t *buffer, uint32_t length)
{
    if (index >= UART_DEVICE_MAX || !buffer || length == 0)
    {
        return status_invalid_argument;
    }

    uart_device_config_t *config = &g_uart_devices[index];

    dma_disable_channel(BOARD_APP_HDMA, config->rx_dma_channel);

    /* 配置并启动 DMA 接收 */
    return uart_rx_trigger_dma(
        config->base,
        config->rx_dma_channel,
        core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)buffer),
        length);
}


hpm_stat_t hal_uart_start_dma_tx(uart_device_index_t index, uint8_t *buffer, uint32_t length)
{
    if (index >= UART_DEVICE_MAX || !buffer || length == 0)
    {
        return status_invalid_argument;
    }

    uart_device_config_t *config = &g_uart_devices[index];

    dma_disable_channel(BOARD_APP_HDMA, config->tx_dma_channel);

    /* 配置并启动 DMA 发送 */
    return uart_tx_trigger_dma(
        config->base,
        config->tx_dma_channel,
        core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)buffer),
        length);
}

void hal_uart_register_callback(uart_device_index_t index, uart_dma_direction_t direction, void *callback, void *dma_callback)
{
    if (index >= UART_DEVICE_MAX || !callback || !dma_callback)
    {
        return;
    }

    uart_device_config_t *config = &g_uart_devices[index];

    if (direction == UART_DMA_RX)
    {
        config->rx_cb = (PFN_UART_CB)callback;
    }
    else
    {
        config->tx_cb = (PFN_UART_CB)callback;
        config->tx_dma_cb = (PFN_DMA_CB)dma_callback;
    }
}


hpm_stat_t hal_uart_config(uart_device_index_t index, const uart_device_config_t *config)
{
    if (index >= UART_DEVICE_MAX || !config)
    {
        return status_invalid_argument;
    }

    g_uart_devices[index] = *config;

    hpm_stat_t stat;
    uart_config_t uart_config = {0};

    board_init_uart(config->base);
    uart_default_config(config->base, &uart_config);

    uart_config.baudrate = config->baudrate;
    uart_config.fifo_enable = true;
    uart_config.dma_enable = true;
    uart_config.src_freq_in_hz = clock_get_frequency(config->clock_name);
    uart_config.rx_fifo_level = uart_rx_fifo_trg_not_empty;
    uart_config.rxidle_config.detect_enable = true;
    uart_config.rxidle_config.detect_irq_enable = true;
    uart_config.rxidle_config.idle_cond = uart_rxline_idle_cond_rxline_logic_one;
    uart_config.rxidle_config.threshold = 20U; /* 20bit */

    stat = uart_init(config->base, &uart_config);
    if (stat != status_success)
    {
        return stat;
    }

    if (UART_DEVICE_DPM == index)
    {
        uart_config.txidle_config.detect_enable = true;
        uart_config.txidle_config.detect_irq_enable = true;
        uart_config.txidle_config.idle_cond = uart_rxline_idle_cond_rxline_logic_one;
        uart_config.txidle_config.threshold = 10U;
        uart_init_txline_idle_detection(config->base, uart_config.txidle_config);
    }

    dmamux_config(BOARD_APP_DMAMUX, config->rx_dma_channel, config->rx_dma_request, true);
    dmamux_config(BOARD_APP_DMAMUX, config->tx_dma_channel, config->tx_dma_request, true);

    intc_m_enable_irq_with_priority(config->irq, 1);

    return status_success;
}

hpm_stat_t uart_update_baudrate(uint8_t group, uint32_t baudrate)
{
    const uart_device_config_t *default_config = &uart_group_configs[group];
    return uart_set_baudrate(default_config->base, baudrate, clock_get_frequency(default_config->clock_name));
}

int hal_uart_init(void)
{
    int success = status_success;

    for (int i = 0; i < UART_DEVICE_MAX; i++)
    {
        if (hal_uart_config((uart_device_index_t)i, &uart_group_configs[i]) != status_success)
        {
            success = status_fail;
            DEG_UART("failed to initialize uart\n");
        }
    }

    intc_m_enable_irq_with_priority(BOARD_APP_HDMA_IRQ, 1);
    return success;
}

void hal_init_boot_pin(void)
{
    // boot 高电平有效
    HPM_IOC->PAD[IOC_PAD_PA01].FUNC_CTL = IOC_PA01_FUNC_CTL_GPIO_A_01;
    gpio_set_pin_output_with_initial(HPM_GPIO0, GPIO_DI_GPIOA, 1, 1);

    // reset 低电平有效
    HPM_PIOC->PAD[IOC_PAD_PY00].FUNC_CTL = IOC_PY00_FUNC_CTL_SOC_GPIO_Y_00;
    HPM_IOC->PAD[IOC_PAD_PY00].PAD_CTL = IOC_PY00_FUNC_CTL_PGPIO_Y_00;
    gpio_set_pin_output_with_initial(HPM_GPIO0, reset_pin[0][0], reset_pin[0][1], 1);

    HPM_PIOC->PAD[IOC_PAD_PY01].FUNC_CTL = IOC_PY01_FUNC_CTL_SOC_GPIO_Y_01;
    HPM_IOC->PAD[IOC_PAD_PY01].PAD_CTL = IOC_PY01_FUNC_CTL_PGPIO_Y_01;
    gpio_set_pin_output_with_initial(HPM_GPIO0, reset_pin[1][0], reset_pin[1][1], 1);

    HPM_PIOC->PAD[IOC_PAD_PY02].FUNC_CTL = IOC_PY02_FUNC_CTL_SOC_GPIO_Y_02;
    HPM_IOC->PAD[IOC_PAD_PY02].PAD_CTL = IOC_PY02_FUNC_CTL_PGPIO_Y_02;
    gpio_set_pin_output_with_initial(HPM_GPIO0, reset_pin[2][0], reset_pin[2][1], 1);

    HPM_PIOC->PAD[IOC_PAD_PY03].FUNC_CTL = IOC_PY03_FUNC_CTL_SOC_GPIO_Y_03;
    HPM_IOC->PAD[IOC_PAD_PY03].PAD_CTL = IOC_PY03_FUNC_CTL_GPIO_Y_03;
    gpio_set_pin_output_with_initial(HPM_GPIO0, reset_pin[3][0], reset_pin[3][1], 1);

    HPM_IOC->PAD[IOC_PAD_PA10].PAD_CTL = IOC_PA10_FUNC_CTL_GPIO_A_10;
    gpio_set_pin_output_with_initial(HPM_GPIO0, reset_pin[4][0], reset_pin[4][1], 1);
}

void hal_set_boot_inout(uint8_t id ,uint8_t inout)
{
    if ((id > UART_DEVICE_4) || (inout > 1))
    {
        return;
    }

    gpio_write_pin(HPM_GPIO0, GPIO_DI_GPIOA, 1, inout);

    board_delay_ms(10);
    gpio_set_pin_output_with_initial(HPM_GPIO0, reset_pin[id][0], reset_pin[id][1], 0);
    board_delay_ms(10);
    gpio_write_pin(HPM_GPIO0, reset_pin[id][0], reset_pin[id][1], 1);
}

