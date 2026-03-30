/*
 * Copyright (c) 2024-2025 Melgeek, All Rights Reserved.
 *
 * Description:
 *
 */
/* FreeRTOS kernel includes. */
#include "dfu.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "app_debug.h"
#include "app_config.h"
#include "easy_crc.h"
#include "dfu_api.h"
#include "hal_flash.h"
#include "db.h"
#include "mg_uart.h"
#include "mg_sm.h"
#include "rgb_led_ctrl.h"
#include "rgb_light_tape.h"
#include "hal_i2c.h"

#if DEBUG_EN == 1
#define MODULE_LOG_ENABLED (1)
#if MODULE_LOG_ENABLED == 1
#define LOG_TAG "dfu_slave"
#define DEG_UART(fmt, ...) Mg_SEGGER_RTT_printf(LOG_TAG, fmt, ##__VA_ARGS__)
#else
#define DEG_UART(fmt, ...)
#endif
#else
#define DEG_UART(fmt, ...)
#endif


static TaskHandle_t m_slave_handle;
static QueueHandle_t xSlaveRetQueue;
static BaseType_t Suspended_boot = pdFALSE;
static uint8_t dfu_sm_bit = 0;
static dfu_sm_status_t sm_mode = SM_STATUS_IDLE;
static uint16_t upgrade_swv = 0xFFFF;
static uint8_t s_buff[16 + UART_SLAVE_CHUNK_SIZE + 4] = {0};
static uint32_t s_pCrcBuf[UART_SLAVE_CHUNK_SIZE / 4] = {0};


static bool receive_upgrade_response(uint8_t *frame, uint16_t frame_len);
static uint32_t parse_frame(uint8_t *frame, uint16_t frame_len, slave_rx_head_t *res_data);


uint8_t dfu_get_slave_status(void)
{
    return sm_mode;
}

uint8_t dfu_device_get_upgrade(void)
{
    return dfu_sm_bit;
}

void mg_uart_tran_start(uint16_t swv)
{
    if ((m_slave_handle != NULL) && (Suspended_boot == pdTRUE))
    {
        vTaskResume(m_slave_handle);
        Suspended_boot = pdFALSE;
    }

    hal_boot_data_cb(receive_upgrade_response);
    upgrade_swv = swv;
    sm_mode = SM_STATUS_ONLINE;
}

void mg_slave_boot_suspend(void)
{
    if ((m_slave_handle != NULL) && (Suspended_boot == pdFALSE))
    {
        Suspended_boot = pdTRUE;
        vTaskSuspend(m_slave_handle);
    }
}

uint8_t dfu_check_sm_current_app(void)
{
    uint32_t fw_version1 = 0;
    uint32_t fw_version2 = 0;

    uint16_t software_version = 0;
    uint16_t hardware_version = 0;
    mg_nation_hw_fw_ver_get(&hardware_version, &software_version);

    gm_fw_info_t fw_info = {0};
    db_read_gm_fw_info(FLASH_GM_FW_INFO_ADDR, &fw_info);
    if (fw_info.is_vaild == 1)
    {
        fw_version1 = fw_info.fw_ver;
    }

    db_read_gm_fw_info(FLASH_GM_FW2_INFO_ADDR, &fw_info);
    if (fw_info.is_vaild == 1)
    {
        fw_version2 = fw_info.fw_ver;
    }

    DEG_UART("ota1 version:%d, ota2 version:%d curr%d  ready %d\n", fw_version1, fw_version2, software_version, BOARD_SM_FW_VERSION);

    // flash compare
    if (fw_version1 == software_version)
    {
        return 1;
    }
    else if (fw_version2 == software_version)
    {
        return 2;
    }
    return 0;
}

static bool dfu_check_sm_sync_app(void)
{
    uint16_t software_version = 0;
    uint16_t hardware_version = 0;
    mg_nation_hw_fw_ver_get(&hardware_version, &software_version);

    DEG_UART("%d > to > %d \r\n", software_version, BOARD_SM_FW_VERSION);

    if (software_version == BOARD_SM_FW_VERSION)
    {
        return true;
    }
    else
    {
        //message hive
        return false;
    }
}

static uint8_t dfu_check_sm_upgrade_app(void)
{
    uint32_t fw_version1 = 0;
    uint32_t fw_version2 = 0;

    gm_fw_info_t fw_info = {0};
    db_read_gm_fw_info(FLASH_GM_FW_INFO_ADDR, &fw_info);
    if (fw_info.is_vaild == 1)
    {
        fw_version1 = fw_info.fw_ver;
    }

    db_read_gm_fw_info(FLASH_GM_FW2_INFO_ADDR, &fw_info);
    if (fw_info.is_vaild == 1)
    {
        fw_version2 = fw_info.fw_ver;
    }

    DEG_UART("upgrade ota1 version:%d, ota2 version:%d\n", fw_version1, fw_version2);

    // flash compare
    if (sm_mode == SM_STATUS_CHECKOUT)
    {
        if (fw_version1 == BOARD_SM_FW_VERSION)
        {
            return 1;
        }
        else if (fw_version2 == BOARD_SM_FW_VERSION)
        {
            return 2;
        }
    }
    else
    {
        if (fw_version1 == upgrade_swv)
        {
            return 1;
        }
        else if (fw_version2 == upgrade_swv)
        {
            return 2;
        }
    }
    return 0;
}

static uint32_t dfu_slave_notify_version(uint8_t reload)
{
    dfu_slave_fw_info_t buff = {0};
    buff.device_type = DFU_OBJ_GM;
    if (reload)
    {
        mg_nation_hw_fw_ver_get(&buff.hwv, &buff.fwv);
    }

    return cmd_do_response(CMD_NOTIFY, NTF_SLAVE_INFO, sizeof(dfu_slave_fw_info_t), 0, (uint8_t *)&buff);
}

static uint32_t uart_nation_boot_cmd_response(uint8_t id, uint8_t dir, uint8_t *pdata, uint16_t len, uint32_t timeout)
{
    uint32_t e = 0;
    static uart_report_t send_q[5];
    static uint8_t send_cnt = 0;
    slave_ret_t scr_ret = {0};

    memset(&send_q[send_cnt], 0, sizeof(uart_report_t));
    send_q[send_cnt].nation_boot.report_id = id;
    send_q[send_cnt].nation_boot.report_size = len;

    if (pdata != NULL)
    {
        memcpy(send_q[send_cnt].nation_boot.data, pdata, len);
    }

    e = mg_uart_send_queue(&send_q[send_cnt]);
    if (e != 0)
    {
        return e;
    }
    send_cnt++;
    send_cnt %= 5;

    if (dir == DIR_ASK)
    {
        while (xQueueReceive(xSlaveRetQueue, &scr_ret, 0) == pdTRUE)
            ;

        if (xQueueReceive(xSlaveRetQueue, &scr_ret, timeout) == pdTRUE)
        {
            // DEG_UART(" ask  CMD_GET_INF  ret %04x\r\n", scr_ret.ret_code);
            e = scr_ret.ret_code;
        }
        else
        {
            DEG_UART(" UART_ERROR_CMD_TIMEOUT\r\n");
            e = UART_ERROR_CMD_TIMEOUT;
        }

    }

    return e;
}

static bool receive_upgrade_response(uint8_t *frame, uint16_t frame_len)
{
    uint32_t ret = 0;
    slave_rx_head_t resp_data;

    ret = parse_frame(frame, frame_len, &resp_data);
    if (ret != UART_SUCCESS)
    {
        // DEG_UART(" upgrade_response parse_frame  ret %04x\r\n", ret);
        if (xQueueSend(xSlaveRetQueue, &ret, portMAX_DELAY) != pdTRUE)
        {
            return false;
        }
    }
    else if (resp_data.cmd == CMD_SET_BR && resp_data.scmd == 0)
    {
        slave_ret_t ret = {0};
        ret.ret_code = resp_data.cr1 << 8 | resp_data.cr2;
        // DEG_UART(" upgrade_response CMD_SET_BR  ret %04x\r\n", ret.ret_code);
        if (xQueueSend(xSlaveRetQueue, &ret, portMAX_DELAY) != pdTRUE)
        {
            return false;
        }
    }
    else if (resp_data.cmd == CMD_GET_INF && resp_data.scmd == 0)
    {

        slave_ret_t ret = {0};
        ret.ret_code = resp_data.cr1 << 8 | resp_data.cr2;
        // DEG_UART(" upgrade_response CMD_GET_INF  ret %04x\r\n", ret.ret_code);
        if (xQueueSend(xSlaveRetQueue, &ret, portMAX_DELAY) != pdTRUE)
        {
            return false;
        }
    }
    else if (resp_data.cmd == CMD_USERX_OP && resp_data.scmd == 0)
    {
        slave_ret_t ret = {0};
        ret.ret_code = resp_data.cr1 << 8 | resp_data.cr2;
        // DEG_UART(" upgrade_response CMD_USERX_OP  ret %04x\r\n", ret.ret_code);
        if (xQueueSend(xSlaveRetQueue, &ret, portMAX_DELAY) != pdTRUE)
        {
            return false;
        }
    }
    else if (resp_data.cmd == CMD_OPT_RW && resp_data.scmd == 0)
    {
        slave_ret_t ret = {0};
        ret.ret_code = resp_data.cr1 << 8 | resp_data.cr2;
        // DEG_UART(" upgrade_response CMD_OPT_RW  ret %04x\r\n", ret.ret_code);
        if (xQueueSend(xSlaveRetQueue, &ret, portMAX_DELAY) != pdTRUE)
        {
            return false;
        }
    }
    else if (resp_data.cmd == CMD_FLASH_ERASE && resp_data.scmd == 0)
    {
        slave_ret_t ret = {0};
        ret.ret_code = resp_data.cr1 << 8 | resp_data.cr2;
        // DEG_UART(" upgrade_response CMD_FLASH_ERASE  ret %04x\r\n", ret.ret_code);
        if (xQueueSend(xSlaveRetQueue, &ret, portMAX_DELAY) != pdTRUE)
        {
            return false;
        }
    }
    else if (resp_data.cmd == CMD_FLASH_DWNLD && resp_data.scmd == 0)
    {
        slave_ret_t ret = {0};
        ret.ret_code = resp_data.cr1 << 8 | resp_data.cr2;
        // DEG_UART(" upgrade_response CMD_FLASH_DWNLD  ret %04x\r\n", ret.ret_code);
        if (xQueueSend(xSlaveRetQueue, &ret, portMAX_DELAY) != pdTRUE)
        {
            return false;
        }
    }
    else if (resp_data.cmd == CMD_DATA_CRC_CHECK && resp_data.scmd == 0)
    {
        slave_ret_t ret = {0};
        ret.ret_code = resp_data.cr1 << 8 | resp_data.cr2;
        // DEG_UART(" upgrade_response CMD_DATA_CRC_CHECK  ret %04x\r\n", ret.ret_code);
        if (xQueueSend(xSlaveRetQueue, &ret, portMAX_DELAY) != pdTRUE)
        {
            return false;
        }
    }
    else if (resp_data.cmd == CMD_SYS_RESET && resp_data.scmd == 0)
    {
        slave_ret_t ret = {0};
        ret.ret_code = resp_data.cr1 << 8 | resp_data.cr2;
        // DEG_UART(" upgrade_response CMD_SYS_RESET  ret %04x\r\n", ret.ret_code);
        if (xQueueSend(xSlaveRetQueue, &ret, portMAX_DELAY) != pdTRUE)
        {
            return false;
        }
    }
    else
    {
        DEG_UART(" find cmd fault ret \r\n");
    }
    return true;
}

uint8_t calculate_xor_checksum(uint8_t *data, uint16_t len)
{
    uint8_t xor = 0;
    for (uint16_t i = 0; i < len; i++)
    {
        xor ^= data[i];
    }
    return xor;
}

static uint32_t parse_frame(uint8_t *frame, uint16_t frame_len, slave_rx_head_t *res_data)
{
    uint32_t ret = UART_SUCCESS;
    uint8_t index = 0;

    if (frame_len < 6)
    {
        return UART_ERROR_INVALID_LENGTH;
    }

    if (frame[index++] != FRAME_HEADER1 || frame[index++] != FRAME_HEADER2)
    {
        return UART_ERROR_INVALID_PARAM;
    }

    res_data->cmd = frame[index++];
    res_data->scmd = frame[index++];
    uint8_t len = frame[index++];
    res_data->len = len | (frame[index++] << 8);
    if (res_data->len > 0)
    {
        for (uint16_t i = 0; i < res_data->len; i++)
        {
            res_data->data[i] = frame[6 + i];
        }
    }
    res_data->cr1 = frame[index + res_data->len];
    index++;
    res_data->cr2 = frame[index + res_data->len];
    index++;
    // DEG_UART(" UART_ERROR_CMD_CHECK   %x  %x   %x\r\n", frame[8 + res_data->len], res_data->cr1, res_data->cr2);

    uint8_t calc_xor = calculate_xor_checksum(frame, frame_len - 1);
    if (calc_xor != frame[frame_len - 1])
    {
        return UART_ERROR_CMD_CHECK;
    }

    return ret;
}

static bool build_frame(uint8_t cmd, uint8_t scmd, uint32_t param, uint8_t *data, uint16_t data_len, uint8_t *frame, uint16_t *frame_len)
{

    if (data_len > MAX_DATA_LENGTH)
    {
        return false;
    }

    slave_tx_head_t *p_head = (slave_tx_head_t *)&frame[0];
    uint16_t len = sizeof(slave_tx_head_t);
    p_head->magich = FRAME_HEADER1;
    p_head->magicl = FRAME_HEADER2;
    p_head->cmd = cmd;
    p_head->scmd = scmd;
    p_head->len = data_len;
    p_head->param = param;

    if (data_len > 0 && data != NULL)
    {
        for (uint16_t i = 0; i < data_len; i++)
        {
            frame[len + i] = data[i];
        }
    }

    frame[len + data_len] = calculate_xor_checksum(frame, len + data_len);

    *frame_len = len + data_len + 1;
    return true;
}

static uint32_t send_upgrade_command(uint8_t id, uint8_t cmd, uint8_t scmd, uint32_t param, uint8_t *data, uint16_t data_len)
{
    uint8_t frame[MAX_DATA_LENGTH + 6];
    uint16_t frame_len;

    if (!build_frame(cmd, scmd, param, data, data_len, frame, &frame_len))
    {
        return false;
    }

    return uart_nation_boot_cmd_response(id, DIR_ASK, frame, frame_len, TIMEOUT_1S);
}


static uint32_t uart_perform_firmware_upgrade(void)
{
    uint32_t ret = RESP_SUCCESS;
    uint8_t data[16] = {0};

    for (uint8_t device = UART_DEVICE_0; device < UART_DEVICE_DPM; device++)
    {
        uart_update_baudrate(device, 9600);
    }

    for (uint8_t device = UART_DEVICE_0; device < UART_DEVICE_DPM; device++)
    {
        hal_set_boot_inout(device, 1);
    }

    for (uint8_t device = UART_DEVICE_0; device < UART_DEVICE_DPM; device++)
    {
        ret = send_upgrade_command(device, CMD_SET_BR, 0, UART_SLAVE_BAUDRATE_115200, NULL, 0);
        if (ret != RESP_SUCCESS)
        {
            if (ret != RESP_ERROR)
            {
                ret = APP_ERROR_BR_TIMEOUT;
            }
            DEG_UART(" CMD_SET_BR %04x\r\n", ret);
            return ret;
        }
        else
        {
            uart_update_baudrate(device, 115200);
        }
    }

    for (uint8_t device = UART_DEVICE_0; device < UART_DEVICE_DPM; device++)
    {
        ret = send_upgrade_command(device, CMD_GET_INF, 0, 0, NULL, 0);
        if (ret != RESP_SUCCESS)
        {
            DEG_UART(" cmd RESP_ERROR %04x\r\n", ret);
            return ret;
        }
        DEG_UART(" CMD_GET_INF   ok\r\n");
    }
    for (uint8_t device = UART_DEVICE_0; device < UART_DEVICE_DPM; device++)
    {
        ret = send_upgrade_command(device, CMD_USERX_OP, 0, 0xFF0000, NULL, 0);
        if (ret != RESP_SUCCESS)
        {
            return ret;
        }
        DEG_UART(" CMD_USERX_OP   ok\r\n");
    }
    for (uint8_t device = UART_DEVICE_0; device < UART_DEVICE_DPM; device++)
    {
        ret = send_upgrade_command(device, CMD_USERX_OP, 0, 0xFF0002, NULL, 0);
        if (ret != RESP_SUCCESS)
        {
            return ret;
        }
        DEG_UART(" CMD_USERX_OP2   ok\r\n");
    }
    for (uint8_t device = UART_DEVICE_0; device < UART_DEVICE_DPM; device++)
    {
        ret = send_upgrade_command(device, CMD_OPT_RW, 0, 0, data, sizeof(data));
        if (ret != RESP_SUCCESS)
        {
            return ret;
        }
        DEG_UART(" CMD_OPT_RW   ok\r\n");
    }

    uint32_t addr_fw = 0;
    gm_fw_info_t fw_info = {0};
    uint8_t index = dfu_check_sm_upgrade_app();
    if (index == 1)
    {
        addr_fw = FLASH_GM_FW_ADDR;
        db_read_gm_fw_info(FLASH_GM_FW_INFO_ADDR, &fw_info);
        DEG_UART("APP index %d LOADING!!!\r\n", index);
    }
    else if (index == 2)
    {
        addr_fw = FLASH_GM_FW2_ADDR;
        db_read_gm_fw_info(FLASH_GM_FW2_INFO_ADDR, &fw_info);
        DEG_UART("APP index %d LOADING!!!\r\n", index);
    }
    else
    {
        DEG_UART(" index not find\r\n");
        ret = APP_ERROR_FW_INVALID;
    }

    if (fw_info.is_vaild == 1)
    {
        uint32_t offset = 0;
        const uint32_t addr_start = 0x8000000;
        uint16_t page_addr = (addr_start - 0x8000000) / 0x800;
        uint16_t page_count = fw_info.fw_size / 0x800 + 1;
        uint32_t param = (page_count << 16) | page_addr;

        rgb_led_set_prompt(PROMPT_DFU_SLAVE_FLASH_DOWNLOAD);

        // 准备固件bin
        for (uint8_t device = UART_DEVICE_0; device < UART_DEVICE_DPM; device++)
        {
            ret = send_upgrade_command(device, CMD_FLASH_ERASE, 0, param, data, sizeof(data));
            if (ret != RESP_SUCCESS)
            {
                return ret;
            }
            DEG_UART(" CMD_FLASH_ERASE   ok\r\n");
        }
        DEG_UART(" fw_info.fw_ver   %d  size %d  crc %x\r\n", fw_info.fw_ver, fw_info.fw_size, fw_info.check);

        // uint32_t remainder = fw_info.fw_size % UART_SLAVE_CHUNK_SIZE;
        // uint32_t total_chunks = (fw_info.fw_size + UART_SLAVE_CHUNK_SIZE - 1) / UART_SLAVE_CHUNK_SIZE;
        // uint32_t current_chunk = 0;

        // uint8_t buff[16 + UART_SLAVE_CHUNK_SIZE + 4] = {0};
        // uint32_t bytes_to_read = UART_SLAVE_CHUNK_SIZE;
        // uint32_t pCrcBuf[UART_SLAVE_CHUNK_SIZE / 4] = {0};
        // while (offset < fw_info.fw_size)
        // {

        //     if (offset + bytes_to_read > fw_info.fw_size)
        //     {
        //         bytes_to_read = fw_info.fw_size - offset;
        //     }

        //     db_read_gm_fw(offset, &buff[16], bytes_to_read);

        //     if (bytes_to_read < UART_SLAVE_CHUNK_SIZE)
        //     {
        //         memset(&buff[16 + bytes_to_read], 0, UART_SLAVE_CHUNK_SIZE - bytes_to_read);
        //     }

        //     char_to_uint32(&buff[16], UART_SLAVE_CHUNK_SIZE, pCrcBuf);
        //     uint32_t crc32 = boot_crc32(pCrcBuf, UART_SLAVE_CHUNK_SIZE / 4);
        //     *(uint32_t *)&buff[16 + UART_SLAVE_CHUNK_SIZE] = crc32; // crc

        //     ret = send_upgrade_command(device, CMD_FLASH_DWNLD, 0, addr_start + offset, buff, sizeof(buff));
        //     if (ret != RESP_SUCCESS)
        //     {
        //         DEG_UART("CMD_FLASH_DWNLD error: %x\r\n", ret);
        //         return ret;
        //     }
        //     DEG_UART("CMD_FLASH_DWNLD ok\r\n");

        //     offset += bytes_to_read;
        //     // current_chunk++;
        // }

        uint32_t bytes_to_read = 0;
        memset(s_buff, 0, sizeof(s_buff));
        memset(s_pCrcBuf, 0, (4 * sizeof(s_pCrcBuf)));

        while (offset < fw_info.fw_size)
        {
            uint32_t remaining = fw_info.fw_size - offset;

            bytes_to_read = (remaining < UART_SLAVE_CHUNK_SIZE) ? remaining : UART_SLAVE_CHUNK_SIZE;

            taskENTER_CRITICAL();
            db_read_gm_fw(addr_fw + offset, &s_buff[16], bytes_to_read);
            char_to_uint32(&s_buff[16], bytes_to_read, s_pCrcBuf);
            uint32_t crc32 = boot_crc32(s_pCrcBuf, bytes_to_read / 4);
            *(uint32_t *)&s_buff[16 + bytes_to_read] = crc32; // CRC
            taskEXIT_CRITICAL();
            for (uint8_t device = UART_DEVICE_0; device < UART_DEVICE_DPM; device++)
            {
                ret = send_upgrade_command(device, CMD_FLASH_DWNLD, 0, addr_start + offset, s_buff, 20 + bytes_to_read);
                if (ret != RESP_SUCCESS)
                {
                    rgb_led_set_prompt(PROMPT_DFU_SLAVE_FLASH_ABORT);
                    DEG_UART("id %d CMD_FLASH_DWNLD error: %x\r\n", device, ret);
                    return ret;
                }
            }

            offset += bytes_to_read;
        }

        uint8_t data_check[0x18] = {0};
        *(uint32_t *)&data_check[0x10] = addr_start;
        *(uint32_t *)&data_check[0x14] = offset;
        for (uint8_t device = UART_DEVICE_0; device < UART_DEVICE_DPM; device++)
        {
            ret = send_upgrade_command(device, CMD_DATA_CRC_CHECK, 0, fw_info.check, data_check, sizeof(data_check));
            if (ret != RESP_SUCCESS)
            {
                DEG_UART(" CMD_DATA_CRC_CHECK   error\r\n", ret);
                return ret;
            }
            DEG_UART(" CMD_DATA_CRC_CHECK   ok\r\n");
        }

        rgb_led_set_prompt(PROMPT_DFU_SLAVE_FLASH_COMPLETE);
    }
    else
    {
        DEG_UART(" fw_info.is_vaild  %d\r\n", fw_info.is_vaild);
    }

    // ret = send_upgrade_command(device, CMD_SYS_RESET, 0, 0, NULL, 0);
    // if (ret != RESP_SUCCESS)
    // {
    //     DEG_UART(" CMD_SYS_RESET   error\r\n", ret);
    //     return ret;
    // }
    // DEG_UART(" CMD_SYS_RESET   ok\r\n");

    for (uint8_t device = UART_DEVICE_0; device < UART_DEVICE_DPM; device++)
    {
        hal_set_boot_inout(device, 0);
    }

    return ret;
}


static bool uart_slave_version_sync(void)
{
    uint32_t ret = mg_uart_send_to_all_nation_request_cmd(SM_GET_VER_CMD, 0, NULL);
    dfu_check_sm_current_app();
    if (ret == UART_SUCCESS)
    {
        return dfu_check_sm_sync_app();
    }

    hal_boot_data_cb(receive_upgrade_response);

    for (uint8_t i = UART_DEVICE_0; i < UART_DEVICE_DPM; i++)
    {
        uart_update_baudrate(i, 9600);

        ret = send_upgrade_command(i, CMD_GET_INF, 0, 0, NULL, 0);
        if (ret == RESP_SUCCESS)
        {
            DEG_UART("id %d err is boot mode %04x\r\n", i, ret);
            return false;
        }
        else if (ret == UART_ERROR_CMD_TIMEOUT)
        {
            DEG_UART("id %d err is boot timeout%04x\r\n", i, ret);
            return false;
        }
        else
        {
            DEG_UART("id %d other err %04x\r\n", i, ret);
        }
    }
    return false;
}

static void mg_uart_tran_close(uint32_t result)
{
    if (sm_mode == SM_STATUS_CHECKOUT)
    {
        if ((dfu_sm_bit == 0x1F) && (result == RESP_SUCCESS)) // 1f
        {
            rgb_led_set_normal();
            mg_scan_task_notify();
            dfu_slave_notify_version(1);
        }
        else if (dfu_sm_bit == 0x1F)
        {
            dfu_slave_notify_version(1);
        }
        else
        {
            dfu_slave_notify_version(0);
        }
        uint8_t pack_info = FW_TRANS_IDLE;
        uart_set_screen_dfu(&pack_info, sizeof(pack_info));
    }
    else if (sm_mode == SM_STATUS_ONLINE)
    {
        upgrade_swv = 0xFFFF;
        dfu_slave_notify();
    }

    sm_mode = SM_STATUS_IDLE;
    mg_slave_boot_suspend();
}

static void uart_slave_handle(void *pvParameters)
{
    (void)pvParameters;
    Suspended_boot = pdFALSE;

    DEG_UART("\r\nscan ver sync launch\r\n");
    bool sync = uart_slave_version_sync();
    if (sync == true)
    {
        mg_scan_task_notify();
        sm_mode = SM_STATUS_IDLE;
        mg_slave_boot_suspend();
        vTaskDelay(1000);
    }
    else
    {
        dfu_sm_bit = 0;
        rgb_led_set_prompt(PROMPT_DFU_SLAVE_CHECKOUT);
        sm_mode = SM_STATUS_CHECKOUT;
        vTaskDelay(1000);

        uint8_t pack_info = FW_TRANS_BUSY;
        uart_set_screen_dfu(&pack_info, sizeof(pack_info));
    }

    while (1)
    {

        hal_init_boot_pin();
        hal_boot_data_cb(receive_upgrade_response);
        uint32_t ret = uart_perform_firmware_upgrade();
        if (ret != RESP_SUCCESS)
        {
            DEG_UART(" uart_device_index  %x continue\r\n", ret);
            if (ret == APP_ERROR_FW_INVALID)
            {
                rgb_led_set_prompt(PROMPT_DFU_SLAVE_INVALID);
            }
            else if (ret == RESP_ERROR)
            {
                rgb_led_set_prompt(PROMPT_DFU_SLAVE_ABORT);
            }
            else if (ret == APP_ERROR_BR_TIMEOUT)
            {
                rgb_led_set_prompt(PROMPT_DFU_SLAVE_DISCONN);
            }
        }

        hal_boot_data_cb(NULL);

        for (uint8_t i = UART_DEVICE_0; i < UART_DEVICE_DPM; i++)
        {
            dfu_sm_bit &= ~(1 << i);
            uart_update_baudrate(i, UART_BAUDRATE_GROUP1);
            if (ret != APP_ERROR_BR_TIMEOUT)
            {
                uint32_t ret_app = mg_uart_send_to_single_nation_request_cmd(i, SM_GET_VER_CMD, 0, NULL,UART_RETRY_TIMES);
                if (ret_app == UART_SUCCESS)
                {
                    dfu_sm_bit |= (1 << i);
                    DEG_UART("id %d communication ok%04x\r\n", i, ret_app);
                }
            }
        }
        vTaskDelay(500);

        mg_uart_tran_close(ret);
    }
}

void mg_uart_tran_init(void)
{
    if (xSlaveRetQueue == NULL)
    {
        xSlaveRetQueue = xQueueCreate(5, sizeof(slave_ret_t));
    }

    if (m_slave_handle == NULL)
    {
        if (xTaskCreate(uart_slave_handle, "uart_slave_handle", STACK_SIZE_UART, NULL, PRIORITY_UART, &m_slave_handle) != pdPASS)
        {
            DEG_UART("uart_slave_handle create error\n");
        }
    }
}
