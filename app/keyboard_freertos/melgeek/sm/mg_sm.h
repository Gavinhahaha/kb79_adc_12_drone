/*
 * Copyright (c) 2024-2025 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#ifndef __SCAN_MODULE_H__
#define __SCAN_MODULE_H__


#include <stdint.h>
#include <stdbool.h>
#include "app_err_code.h"
#include "hal_gpio.h"
#include "hal_spi.h"
#include "mg_matrix_type.h"


#define RX_FIRST_KEY_NUM    5
#define RX_FIRST_PACK_LEN   6
#define ADC_VAL_MIN         (1000)
#define ADC_VAL_MAX         (4095)

#define MCU_NUM_MAX_SIZE                     (5)

typedef struct
{
    uint8_t uart_id;
    uint32_t channel;
}dev_channel_map_s;

extern dev_channel_map_s dev_channel_map[SM_DEV_NUM_MAX];

#pragma pack(1)
typedef union 
{
    uint16_t val;
    //uint16_t num;
    struct 
    {
        uint16_t id : 4;
        uint16_t adc : 12;
    }bit;
    struct
    {
        uint16_t mode : 11;
        uint16_t num : 5;
    }bit_property;
} key_packet_t;
#pragma pack()


typedef enum
{
    DATA_NONE_FRAME_TYPE,
    DATA_KEY_FRAME_TYPE,
    DATA_CALI_TOP_FRAME_TYPE,
    DATA_CALI_BOT_FRAME_TYPE,
    DATA_CALI_ZERO_FRAME_TYPE,
    DATA_CALI_TATOL_FRAME_TYPE,
    DATA_OTHER_TYPE,
    DATA_FRAME_MAX,
}data_type_e;

typedef enum
{
    SPI_REV_NONE,
    SPI_REV_FIRST_OK,
    SPI_REV_FIRST_LEN_ERROR,
    SPI_REV_FIRST_MODE_ERROR,
    SPI_REV_SECOND_OK,
    SPI_REV_SECOND_ERROR,
    SPI_REV_OTHER_ERROR,
}spi_rev_result_e;

typedef struct
{
    uint8_t remain_all_num;
    uint8_t ntf_rdy_num[SM_BUS_MAX];
    uint8_t ntf_not_rdy_num[SM_BUS_MAX];
}spi_remain_state_s;

typedef struct
{
    spi_rev_result_e is_complete;//完成结果
    uint8_t remain_size;
    bool dev_err;
    uint8_t rev_ready;//0:ntf = 0, 1:ntf = 1, 2:完成
    uint16_t sm_raw_data[SPI_RX_KEY_MAX];
}sm_dev_data_s;

typedef struct 
{
    bool rev_all_ok;
    bool timeout_flag;
    uint8_t dev_cur_num[SM_BUS_MAX];
    uint8_t bus_act[SM_BUS_MAX]; //spi正在传输状态
    uint8_t test_flag[SM_BUS_MAX];
    sm_dev_data_s dev_data[SM_DEV_NUM_MAX];
    uint64_t dev_time;
}sm_dev_state_t;
extern sm_dev_state_t sm_dev_sta;

typedef enum
{
    PKT_TYPE_NONE        = 0, /*数据没有更新*/
    PKT_TYPE_CHANGE      = 1,/*数据发生变化,需要处理按键逻辑*/
    PKT_TYPE_ZERO_CALI   = 2,/*初始位置的校准值发生变化*/
    PKT_TYPE_TOP_CALI    = 3,/*顶部校准值发生变化*/
    PKT_TYPE_BOT_CALI    = 4,/*底部初始位置的校准值发生变化*/
    PKT_TYPE_TATOL_CALI  = 5,/*全键上报帧*/
    PKT_TYPE_MAX
}pkt_t;

typedef struct
{
    struct 
    {
        uint16_t adc : 12;
        uint16_t ty  : 4; //pkt_t
    } pack[SM_DEV_NUM_MAX*SPI_RX_KEY_MAX];
    uint8_t data_type[SM_DEV_NUM_MAX];//0:按键帧；1:校准帧；2:其他帧；
    uint8_t dev_data_num[SM_DEV_NUM_MAX];
    uint8_t all_key_num;
    }sm_data_t;

extern sm_data_t sm_data;


void mg_sm_init(void);
void mg_sm_uninit(void);
void mg_scan_task_notify(void);
#endif

