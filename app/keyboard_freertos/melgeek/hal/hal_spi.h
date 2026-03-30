/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef _HAL_SPI_H_
#define _HAL_SPI_H_

#include <stdio.h>
#include <stdarg.h>
#include "hpm_common.h"
#include "hpm_spi.h"


#define SPI_RX_KEY_MAX (16)
#define SCAN_CH_EN_MAX (16)
#define SPI_CRC_EN      (1)
#if SPI_CRC_EN
    #define SPI_CRC_LEN     (1)
#else
    #define SPI_CRC_LEN     (0)
#endif

typedef struct
{
    SPI_Type *base;
    dma_resource_t *dma_res;
} spi_device_config_t;

typedef enum {
    //SM_DEV_NUM_NONE = -1,
    SM_DEV_NUM_0 = 0,
    SM_DEV_NUM_1,
    SM_DEV_NUM_2,
    SM_DEV_NUM_3,
    SM_DEV_NUM_4,
    SM_DEV_NUM_MAX,

    SM_SPI1_DEV_NUM = 3,
    SM_SPI2_DEV_NUM = 2,
} spi_cs_num_t;

typedef enum 
{
    SM_BUS_0 = 0,
    SM_BUS_1,
    SM_BUS_MAX
} bus_num_t;

typedef struct {
    const bus_num_t bus_num;
    spi_cs_num_t cur_cs_num;
    uint16_t *p_rx_buf;
    uint32_t rx_len;
    uint8_t remain_num;
    bool is_crc_err;
}spi_msg_t;

typedef void (*spi_rx_cb)(spi_msg_t *p_msg);

void hal_spi_init(void);
void hal_spi_set_rx_callback(spi_rx_cb spi1_cb, spi_rx_cb spi2_cb);
uint8_t hal_spi_receive(spi_cs_num_t cs_num, uint32_t rx_len);
void hal_spi_write_cs(uint32_t cs_num, uint8_t state);
void spi_clear(void);

#endif
