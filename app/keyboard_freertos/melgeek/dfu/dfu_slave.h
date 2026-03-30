/*
 * Copyright (c) 2024-2025 Melgeek, All Rights Reserved.
 *
 * Description:
 *
 */
#ifndef _DFU_SLAVE_H_
#define _DFU_SLAVE_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_err_code.h"


#define APP_ERROR_FW_INVALID       0xC000
#define APP_ERROR_BR_TIMEOUT       0xC100

// 最大数据长度
#define MAX_DATA_LENGTH                      256
#define UART_SLAVE_CHUNK_SIZE                128
#define UART_SLAVE_BAUDRATE_9600             (0x80250000)
#define UART_SLAVE_BAUDRATE_115200           (0xC20100)


// 应答
#define RESP_SUCCESS     0xA000
#define RESP_ERROR       0xB000

// 帧头定义
#define FRAME_HEADER1      0xAA
#define FRAME_HEADER2      0x55
// 命令定义
#define CMD_SET_BR         0x01  //设置串口波特率（仅使用串口时有效）
#define CMD_GET_INF        0x10  //读取芯片型号索引、BOOT版本号、芯片ID
                                 /*AA 55 10 00 00 00 00 00 00 00 EF*/
#define CMD_GET_RNG        0x20  //获取随机数
#define CMD_KEY_UPDATE     0x21  //更新加密下载密钥或者分区认证密钥
#define CMD_FLASH_ERASE    0x30  //擦除FLASH
#define CMD_FLASH_DWNLD    0x31  //下载用户程序到FLASH
#define CMD_DATA_CRC_CHECK 0x32  //CRC校验下载用户程序
#define CMD_OPT_RW         0x40  //读取/配置选项字节（包含了读保护等级、 FLASH页写保护、 Data0/1配置、 USER配置）
                                 /*AA 55 40 00 10 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 AF*/
#define CMD_USERX_OP       0x41  //获取分区USERX大小，配置分区USERX大小
                                 /*AA 55 41 00 00 00 00 00 FF 00 41 */
#define CMD_SYS_RESET      0x50  //系统复位


/// @brief    slave

// 头部字段枚举
typedef enum
{
    TX_FIELD_MAGICH = 0x0,
    TX_FIELD_MAGICL = 0x1,
    TX_FIELD_CMD    = 0x2,
    TX_FIELD_SCMD   = 0x3,
    TX_FIELD_LEN    = 0x4,
    TX_FIELD_PARAM  = 0x6,
    TX_FIELD_MAX    = 0xA
} slave_send_field_t;

typedef struct
{
    uint8_t magich;
    uint8_t magicl;
    uint8_t cmd;
    uint8_t scmd;
    uint16_t len;
    uint32_t param;

} __attribute__((packed)) slave_tx_head_t;

typedef struct
{
    uint8_t magich;
    uint8_t magicl;
    uint8_t cmd;
    uint8_t scmd;
    uint16_t len;
    uint8_t data[MAX_DATA_LENGTH];
    uint8_t cr1;
    uint8_t cr2;

} __attribute__((packed)) slave_rx_head_t;

typedef struct
{
    uint32_t ret_code;
} slave_ret_t;


typedef enum
{
    SM_STATUS_IDLE = 0x0,
    SM_STATUS_ONLINE = 0x1,
    SM_STATUS_CHECKOUT = 0x2
}dfu_sm_status_t;

uint8_t dfu_device_get_upgrade(void);
uint8_t dfu_get_slave_status(void);
uint8_t dfu_check_sm_current_app(void);
void mg_uart_tran_start(uint16_t swv);
void mg_uart_tran_init(void);

#endif
