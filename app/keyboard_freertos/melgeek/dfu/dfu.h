/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#ifndef _DFU_H_
#define _DFU_H_

#include <stdint.h>
#include <stdbool.h>
#include "mg_uart.h"

#define GET_8BIT(x) ((uint8_t)((x) & 0xFF))

typedef enum e_dfu_err
{
    DFU_ACK_SUCCESS = 0,
    DFU_ACK_PID_ERR,
    DFU_ACK_SIZE_ERR,
    DFU_ACK_PKT_ERR,
    DFU_ACK_END_ERR,
    DFU_ACK_ERASE_ERR,
    DFU_ACK_STATE_ERR,
    DFU_ACK_TIMEOUT_ERR,
    DFU_ACK_SLAVE_ERR,
    DFU_ACK_SLAVE_SW_BUSY,
    DFU_ACK_CHECK_ERR,
    DFU_ACK_FLASH_ERR,
    DFU_ACK_PKT_CHECK_ERR,
    DFU_SLAVE_NULL           = 0x20,
    DFU_M_CMD_TIMEOUT    = GET_8BIT(DFU_SLAVE_NULL + UART_ERROR_CMD_TIMEOUT),
    DFU_M_CMD_CHECK      = GET_8BIT(DFU_SLAVE_NULL + UART_ERROR_CMD_CHECK),
    DFU_M_INVALID_PARAM  = GET_8BIT(DFU_SLAVE_NULL + UART_ERROR_INVALID_PARAM),
    DFU_M_INVALID_LENGTH = GET_8BIT(DFU_SLAVE_NULL + UART_ERROR_INVALID_LENGTH),
    DFU_M_DATA_SIZE      = GET_8BIT(DFU_SLAVE_NULL + UART_ERROR_DATA_SIZE),
    DFU_S_CMD_TIMEOUT    = GET_8BIT(DFU_SLAVE_NULL + UART_S_ERROR_CMD_TIMEOUT),
    DFU_S_CMD_CHECK      = GET_8BIT(DFU_SLAVE_NULL + UART_S_ERROR_CMD_CHECK),
    DFU_S_INVALID_PARAM  = GET_8BIT(DFU_SLAVE_NULL + UART_S_ERROR_INVALID_PARAM),
    DFU_S_INVALID_LENGTH = GET_8BIT(DFU_SLAVE_NULL + UART_S_ERROR_INVALID_LENGTH),
    DFU_S_DATA_SIZE      = GET_8BIT(DFU_SLAVE_NULL + UART_S_ERROR_DATA_SIZE),

} dfu_err_t;

enum
{
    DFU_STEP_IDLE,
    DFU_STEP_START_NRF,
    DFU_STEP_RECV_BIN,
    DFU_STEP_RECV_FINISH,
    DFU_STEP_START_GD,
    DFU_STEP_START_MCU,
    DFU_STEP_START_DMP,
    DFU_STEP_START_GM,
    DFU_STEP_MAX
};

enum
{
    DFU_OBJ_MCU,
    DFU_OBJ_GD,
    DFU_OBJ_DPM,
    DFU_OBJ_GM,
    DFU_OBJ_MAX,
};

enum e_dfu_cmd
{
    DFU_REQ = 0x80,
    DFU_RECV_BIN = 0x81,
    DFU_CMD_MAX,
};


typedef enum
{
    DFU_TIMEOUT_NONE = 0,
    DFU_TIMEOUT_ONCE,
    DFU_TIMEOUT_TWICE,
    DFU_TIMEOUT_MAX,
}dfu_timeout;

typedef struct
{
    uint8_t  cmd;
    uint8_t  subcmd;
    uint16_t data_length;
    uint16_t reserve;
    uint16_t checksum;
    uint32_t sequence_number;

}dfu_head_info_t;

typedef enum
{
    FW_TRANS_IDLE = 0,
    FW_TRANS_BUSY = 1,
    FW_TRANS_START = 2,  // 固件传输请求开始
    FW_TRANS_PROG = 3,   // 固件传输进行中
    FW_TRANS_FINISH = 4, // 固件传输结束
} fw_trans_state_e;

typedef struct
{
    uint32_t fw_ver;
    uint32_t total_size;
    uint32_t total_pkg_num;
    uint32_t curr_pkg_index;
    uint16_t payload_max_len;
    uint16_t last_payload_len;
    // uint8_t sub_payload_index;
    // uint8_t fw_buf[256];
}dpm_fw_trans_info_t;

typedef enum
{
    GM_BOOT_CMD_SET_BR          = 0X01,
    GM_BOOT_CMD_GET_INF         = 0x10,
    GM_BOOT_CMD_GET_RNG         = 0x20,
    GM_BOOT_CMD_KEY_UPDATE      = 0x21,
    GM_BOOT_CMD_FLASH_ERASE     = 0x30,
    GM_BOOT_CMD_FLASH_DWNLD     = 0x31,
    GM_BOOT_CMD_DATA_CRC_CHECK  = 0x32,
    GM_BOOT_CMD_OPT_RW          = 0x40,
    GM_BOOT_CMD_USERX_OP        = 0x41,
    GM_BOOT_CMD_SYS_RESET       = 0x50,
}gm_boot_cmd_e;

typedef struct
{
    uint32_t fw_ver;
    uint32_t total_size;
    uint32_t check;
    uint32_t sub_pkg_idx;
}fw_trans_info_t;

typedef struct
{
    uint32_t device_type;
    uint16_t fwv;
    uint16_t hwv;
    uint8_t compile[16];
    uint8_t chip_id[16];
} dfu_slave_fw_info_t;

uint32_t on_sys_dfu_req(uint16_t len, uint16_t reserve, uint8_t *pdata);
uint32_t dfu_trans_process(uint16_t len, uint16_t reserve, uint8_t *pdata);
void dfu_set_slave_fw_show(dfu_slave_fw_info_t *info, uint8_t index);
dfu_slave_fw_info_t *dfu_get_slave_fw_show(void);
void dfu_slave_notify(void);
uint8_t* dfu_get_bin_head_buf(void);
#endif
