/*
 * Copyright (c) 2024-2025 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#ifndef __MG_UART_H__
#define __MG_UART_H__


#include <stdint.h>
#include <stdbool.h>
#include "hal_uart.h"
#include "app_err_code.h"
#include "dfu_slave.h"



#define UART_BUFF_SIZE                       255
#define TIMEOUT_6S                          (6000)
#define TIMEOUT_1S                          (1000)
#define TIMEOUT_300MS                       (300)
#define TIMEOUT_100MS                       (100)
#define TIMEOUT_NULL                        (0)
#define UART_SM_CMD_BUF_SIZE                 16
#define SM_CMD_MIN_SIZE                      9
#define UART_SLAVE_CHUNK_SIZE                128
#define UART_SLAVE_BAUDRATE_9600             (0x80250000)
#define UART_SLAVE_BAUDRATE_115200           (0xC20100)
#define SM_CMD_SYS_CMD                       (0X01)
#define SM_CMD_CTRL_CMD                      (0X02)
#define SM_GET_VER_CMD                       ((SM_CMD_SYS_CMD << 8) | 0X01)
#define SM_SYS_RESET_CMD                     ((SM_CMD_SYS_CMD << 8) | 0X02)
#define SM_SYS_RESTART_CMD                   ((SM_CMD_SYS_CMD << 8) | 0X03)
#define SM_GET_THRESHOLD_CMD                 ((SM_CMD_CTRL_CMD << 8)| 0X01)
#define SM_SET_THRESHOLD_CMD                 ((SM_CMD_CTRL_CMD << 8)| 0X02)
#define SM_GET_KED_MODE_CMD                  ((SM_CMD_CTRL_CMD << 8)| 0X03)
#define SM_SET_KEY_MODE_CMD                  ((SM_CMD_CTRL_CMD << 8)| 0X04)
#define SM_GET_LOWPWR_TIME_CMD               ((SM_CMD_CTRL_CMD << 8)| 0X05)
#define SM_SET_LOWPWR_TIME_CMD               ((SM_CMD_CTRL_CMD << 8)| 0X06)
#define SM_GET_LOWPWR_FREQ_CMD               ((SM_CMD_CTRL_CMD << 8)| 0X07)
#define SM_SET_LOWPWR_FREQ_CMD               ((SM_CMD_CTRL_CMD << 8)| 0X08)
#define SM_GET_HALL_STABLE_TIME_CMD          ((SM_CMD_CTRL_CMD << 8)| 0X09)
#define SM_SET_HALL_STABLE_TIME_CMD          ((SM_CMD_CTRL_CMD << 8)| 0X0A)
#define SM_GET_ADC_CH_INFO_CMD               ((SM_CMD_CTRL_CMD << 8)| 0X0B)
#define SM_SET_ADC_CH_INFO_CMD               ((SM_CMD_CTRL_CMD << 8)| 0X0C)
#define SM_REPORT_ERR_INFO_CMD               ((SM_CMD_CTRL_CMD << 8)| 0X0D)
#define SM_REPORT_HEARTBEAT_INFO_CMD         ((SM_CMD_CTRL_CMD << 8)| 0X0E)
#define SM_SET_SYN_BOT_CAIL_CMD              ((SM_CMD_CTRL_CMD << 8)| 0X0F)
#define SM_SET_AUTO_CAIL_CTRL_CMD            ((SM_CMD_CTRL_CMD << 8)| 0X10)
#define SM_SET_CH_MAX_CMD                    ((SM_CMD_CTRL_CMD << 8)| 0X11)

#define SM_HALL_ERROR                        (0X01 << 0)
#define SM_SPI_ERROR                         (0X01 << 1)
#define SM_SCAN_SYN_ERROR                    (0X01 << 2)
#define SM_PROGRAM_ERROR                     (0X01 << 3)
#define SM_NOISE_ERROR                       (0X01 << 4)
#define SM_WATER_ERROR                       (0X01 << 5)
#define SM_VOLT_LOW_ERROR                    (0X01 << 6)
#define SM_ERROR_MASK                        (0x7F)
#define INVAIL_UINT16                        (0XFFFF)

#define UART_RETRY_TIMES                     2

#define UART_SUCCESS         0
#define MG_ERR_DEVICE_MASTER 0x00  // 主控错误
#define MG_ERR_DEVICE_SLAVE  0x80  // 从控错误
typedef enum
{
    UART_ERROR_INVALID_PARAM      = (MG_ERR_BASE_UART + MG_ERR_DEVICE_MASTER + 0x07),      ///< Invalid Parameter
    UART_ERROR_INVALID_LENGTH     = (MG_ERR_BASE_UART + MG_ERR_DEVICE_MASTER + 0x09),      ///< Invalid Length
    UART_ERROR_DATA_SIZE          = (MG_ERR_BASE_UART + MG_ERR_DEVICE_MASTER + 0x0C),      ///< Data    size exceeds limit
    UART_ERROR_CMD_TIMEOUT        = (MG_ERR_BASE_UART + MG_ERR_DEVICE_MASTER + 0x01),      ///< timeout
    UART_ERROR_CMD_CHECK          = (MG_ERR_BASE_UART + MG_ERR_DEVICE_MASTER + 0x02),      ///< check
    UART_S_ERROR_INVALID_PARAM    = (MG_ERR_BASE_UART + MG_ERR_DEVICE_SLAVE + 0x07),      ///< Invalid Parameter
    UART_S_ERROR_INVALID_LENGTH   = (MG_ERR_BASE_UART + MG_ERR_DEVICE_SLAVE + 0x09),      ///< Invalid Length
    UART_S_ERROR_DATA_SIZE        = (MG_ERR_BASE_UART + MG_ERR_DEVICE_SLAVE + 0x0C),      ///< Data    size exceeds limit
    UART_S_ERROR_CMD_TIMEOUT      = (MG_ERR_BASE_UART + MG_ERR_DEVICE_SLAVE + 0x01),      ///< timeout
    UART_S_ERROR_CMD_CHECK        = (MG_ERR_BASE_UART + MG_ERR_DEVICE_SLAVE + 0x02),      ///< check
    UART_RET_MAX
} mg_uart_ret;

typedef enum
{   // Successful
    ERR_SCR_ERROR = 1,
    ERR_SCR_TIMEOUT,
    ERR_SCR_FULL,
    ERR_SCR_EMPTY,
    ERR_SCR_NOMEM,
    ERR_SCR_NOSYS,
    ERR_SCR_BUSY,
    ERR_SCR_IO,
    ERR_SCR_INTR,
    ERR_SCR_INVAL,
    ERR_SCR_CRC,
    UART_SCR_MAX
} mg_uart_screen_ret;

typedef bool (*PFN_BOOT_CB)(uint8_t *frame, uint16_t frame_len);

typedef bool (*cmd_handler)(uint8_t *data, uint8_t len);

typedef enum
{
    UART_SM = 0x00,
    UART_DPM,
    UART_MAX
} uart_group_t;

typedef struct uart_fifo
{
    uint16_t id;
    uint16_t size;
    uint8_t *buf;
} fifo_node_t;

typedef struct
{
    uint8_t cmd;            
    uint8_t scmd;
    cmd_handler handler;
} command_entry;


typedef enum
{
    UART_SLAVE_SUCCESS = 0,
    UART_SLAVE_ERROR_TIMEOUT = 0x81,
    UART_SLAVE_ERROR_LEN = 0x82,
} uart_ret_t;

typedef enum
{
    CMD      = 0x00,
    SCMD     = 0x01,
    DIR      = 0x02,
    ERR      = 0x03,
    LEN      = 0x04,
    HEAD_MAX = 0x06

} cmd_head_cmd_t;

typedef struct
{
    uint8_t cmd;      /* Command */
    uint8_t scmd;     /* Sub Command */
    uint8_t dir;      /* pola */
    uint8_t errcode; /* Offset */
    uint16_t len;     /* Length */

} __attribute__((packed)) cmd_head_t;

// 数据格式
typedef enum
{
    CMD_UART_SYS         = 0x01,
    CMD_UART_PICTURE     = 0x02,
    CMD_UART_PC_PARAM    = 0x03,
    CMD_UART_DPM_MODE    = 0x04,
    CMD_UART_KB_PARAM    = 0x05,
    CMD_UART_KEY_TRIGGER = 0x06,
    CMD_UART_KEY_REPORT  = 0x07,
    CMD_UART_KB_WARNING  = 0x08,
    CMD_UART_GENERAL_DATA= 0x09,
    CMD_UART_FACTORY     = 0xFA,
    CMD_UART_ERR         = 0xEE,

} cmd_t;
typedef enum
{
    SCMD_VER     = 0x01,
    SCMD_RESET   = 0x02,
    SCMD_RESTART = 0x03,
    SCMD_DFU     = 0x04,

} scmd_sys_t;

typedef enum
{
    SCMD_DOWNLOAD = 0x01,
    SCMD_UPLOAD   = 0x02,
    SCMD_DELETE   = 0x03,

} scmd_picture_t;

typedef enum
{
    SCMD_GET_PC_INFO = 0x01,
    SCMD_SET_PC_INFO = 0x02,
}scmd_pc_param_t;
typedef enum
{
    SCMD_SET_DPM_INFO = 0x01,
    SCMD_GET_DPM_INFO = 0x02,
} scmd_dpm_mode_t;
typedef enum
{
    SCMD_GET_KB_INFO = 0x01,
    SCMD_SET_KB_INFO = 0x02,
} scmd_kb_param_t;

typedef enum
{
    SCMD_GET_KB_REPORT         = 0x01,
    SCMD_GET_CUSTOM_KEY_REPORT = 0x02,
} scmd_kb_report_t;

typedef enum
{
    SCMD_SET_TRIGGER_MODE = 0x01,
    SCMD_SET_KEY_TRIGGER  = 0x02,
} scmd_key_trigger_t;

typedef enum
{
    SCMD_SET_KB_WARNING = 0x01,
} scmd_kb_warning_t;
typedef enum
{
    SCMD_DATA_SYNC = 0x01,
} scmd_data_sync_t;

typedef enum
{
    SCMD_FAC_MODE = 0x01,
    SCMD_FAC_COLOR= 0x02,
    SCMD_FAC_TOUCH= 0x03
} scmd_factory_t;
typedef enum
{
    DIR_ASK=0,
    DIR_REPLY,
    DIR_NOTIFY

} dir_t;

typedef struct
{
    uint32_t ret_code;
} screen_ret_t;


/**************驱动器********************/
typedef enum
{
    CMD_SUCCESS=0,
    CMD_FAIL,

} cmd_e;

typedef enum
{
    NATION_CMD      = 0x00,
    NATION_SCMD     = 0x01,
    NATION_DIR      = 0x02,
    NATION_LEN      = 0x03,
    NATION_RESERVE  = 0x05,
    NATION_CRC      = 0x07,
    NATION_HEAD_MAX = 0x09

} nation_cmd_head_cmd_t;

    typedef struct
{
    uint8_t cmd;      /* Command */
    uint8_t scmd;     /* Sub Command */
    uint8_t dir;      /* pola */
    uint16_t len;     /* Length */
    uint16_t reserve; /* Offset */
    uint16_t crc;     /* Checksum */

} __attribute__((packed)) nation_cmd_head_t;


typedef union
{
    uint8_t raw[UART_BUFF_SIZE];
    struct
    {
        uint8_t report_id;
        uint16_t report_size;
        uint8_t data[UART_BUFF_SIZE - 3];

    } __attribute__((packed)) comm;

    struct
    {
        uint8_t report_id;
        uint16_t report_size;
        cmd_head_t head;
        uint8_t data[UART_BUFF_SIZE - 3 - sizeof(cmd_head_t)];

    } __attribute__((packed)) display;

    struct
    {
        uint8_t report_id;
        uint16_t report_size;
        nation_cmd_head_t head;
        uint8_t data[UART_BUFF_SIZE - 3 - sizeof(nation_cmd_head_t)];

    } __attribute__((packed)) sm_ctl;

    struct
    {
        uint8_t report_id;
        uint16_t report_size;
        uint8_t data[UART_BUFF_SIZE - 3 - TX_FIELD_MAX];

    } __attribute__((packed)) nation_boot;
} uart_report_t;


void mg_uart_init(void);
/// @brief 
/// @param cb 
void hal_boot_data_cb(PFN_BOOT_CB cb);

uint32_t mg_uart_send_to_all_nation_request_cmd(uint16_t cmd, uint16_t data_len, uint8_t *pdata);
uint32_t mg_uart_send_to_single_nation_request_cmd(uint8_t id, uint16_t cmd, uint16_t data_len, uint8_t *pdata, uint8_t retry_times);
uint32_t mg_uart_send_queue(uart_report_t *p_report);
bool mg_nation_hw_fw_ver_get(uint16_t *hw_version,uint16_t *fw_version);

uint32_t uart_get_screen_ver(void);
uint32_t uart_set_screen_dfu(uint8_t *buf, uint8_t len);
uint32_t uart_set_screen_picture(uint8_t *buf, uint8_t len);
uint32_t uart_set_general_data(uint8_t *buf, uint8_t len);
uint8_t *uart_get_general_buff(void);
uint32_t uart_notify_screen_cmd(uint8_t cmd, uint8_t scmd, uint8_t *buf, uint8_t len);
uint32_t uart_ask_screen_cmd(uint8_t cmd, uint8_t scmd, uint8_t *buf, uint8_t len);
#endif

