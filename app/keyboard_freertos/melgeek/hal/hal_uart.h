/*
 * Copyright (c) 2024-2025 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef _HAL_UART_H_
#define _HAL_UART_H_

#include <stdio.h>
#include <stdarg.h>
#include "hpm_common.h"
#include "hpm_clock_drv.h"
#include "hpm_uart_drv.h"
#include "hpm_dmav2_drv.h"


#define UART_BAUDRATE_GROUP1               921600
#define UART_BAUDRATE_GROUP2               1000000



typedef void (*PFN_UART_CB)(uint8_t index, uint8_t len);
typedef void (*PFN_DMA_CB)(void);

typedef enum
{
    UART_DEVICE_0 = 0,
    UART_DEVICE_1,
    UART_DEVICE_2,
    UART_DEVICE_3,
    UART_DEVICE_4,
    UART_DEVICE_DPM,
    UART_DEVICE_MAX
} uart_device_index_t;

typedef enum
{
    UART_DMA_RX = 0,
    UART_DMA_TX
} uart_dma_direction_t;

typedef struct
{
    UART_Type *base;
    uint32_t irq;
    uint32_t baudrate;
    clock_name_t clock_name;
    uint8_t rx_dma_channel;
    uint8_t tx_dma_channel;
    uint32_t rx_dma_request;
    uint32_t tx_dma_request;
    PFN_UART_CB rx_cb;
    PFN_UART_CB tx_cb;
    PFN_DMA_CB tx_dma_cb;
} uart_device_config_t;



void hal_uart_register_callback(uart_device_index_t index, uart_dma_direction_t direction, void *callback, void *dma_callback);

hpm_stat_t hal_uart_start_dma_rx(uart_device_index_t index, uint8_t *buffer, uint32_t length);

hpm_stat_t hal_uart_start_dma_tx(uart_device_index_t index, uint8_t *buffer, uint32_t length);

void hal_init_boot_pin(void);

void hal_set_boot_inout(uint8_t id ,uint8_t inout);

hpm_stat_t uart_update_baudrate(uint8_t group, uint32_t baudrate);

int hal_uart_init(void);
#endif
