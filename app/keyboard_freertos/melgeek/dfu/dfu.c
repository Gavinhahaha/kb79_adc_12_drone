/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#include "dfu.h"
#include "dfu_api.h"
#include "hal_flash.h"
#include "hal_timer.h"
#include "db.h"
#include "mg_hive.h"
#include "mg_matrix.h"
#include "mg_uart.h"
#include "rgb_led_ctrl.h"
#include "app_debug.h"
#include "dfu_slave.h"
#include "easy_crc.h"
#include "mg_sm.h"
#include "mg_detection.h"
#include "hal_i2c.h"
#include "hal_wdg.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "dfu"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#define DFU_SCREEN_FRAME_SIMPLE           (4)
#define DFU_BIN_FRAME_SIZE                (48)
#define APP_TIMER_SHOT                    (500000)
#define APP_TIMER_ACK                     (1000000)
#define APP_TIMER_SCREEN                  (15000000)
#define APP_TIMER_GM                      (15000000)
#define GM_SUBPACKETS_PER_PACKET          (80)


uint8_t bin_head[FLASH_PAGE_SIZE + 1024] __attribute__((aligned(4)));
uint8_t bin_buffer[64] __attribute__((aligned(4)));

static dfu_timeout g_timeout;
static volatile uint32_t s_next_sequence_number = 0;
static volatile uint32_t s_current_sequence_number = 0;
static uint8_t dfu_step    = DFU_STEP_IDLE;
static bool b_dfu_start = 0;
static uint32_t total_number;
static uint32_t dfu_obj = DFU_OBJ_MAX;
static fw_trans_info_t gm_fw_trans_info = {0};
static fw_trans_info_t mcu_fota_info = {0};
static dfu_slave_fw_info_t dfu_slave_fw_show = {0};
static bool b_slave_scan = false;
static TaskHandle_t xTaskCHandle;

uint32_t app_boot_start = 0x00000000;
uint32_t app_boot_check = 0x00000000;
uint32_t app_boot_curr = 0x00000000;
uint32_t app_page_size = 0x00000000;
uint32_t sw_version_pack_mark = 0x00000000;
uint32_t sw_version_offset_mark = 0x00000000;
uint32_t sw_version_write_pos = 0x00000000;
uint8_t sw_version_write_smile[DFU_BIN_FRAME_SIZE] = {0};


static uint32_t dfu_finish(void);

uint8_t *dfu_get_bin_head_buf(void)
{
    return bin_head;
}

void dfu_set_slave_fw_show(dfu_slave_fw_info_t *info, uint8_t index)
{
    memcpy(&dfu_slave_fw_show,info,index);
}

dfu_slave_fw_info_t* dfu_get_slave_fw_show(void)
{    
    return (&dfu_slave_fw_show);
}

void dfu_slave_notify(void)
{
    if (xTaskCHandle != NULL)
    {
        xTaskNotifyGive(xTaskCHandle);
    }
    else if (dfu_step == DFU_STEP_START_GM)
    {
        dfu_finish();
    }
    b_slave_scan = false;
}

static flash_ret dfu_crc16_full(uint16_t *crc_out, uint32_t addr, uint32_t total_len)
{
    flash_ret ret = FLASH_RET_SUCCESS;
    uint16_t crc = 0xFFFF;
    uint32_t processed = 0;

    while (processed < total_len)
    {
        hal_wdg_manual_feed();
        uint32_t remaining = total_len - processed;
        uint32_t current_chunk = (remaining < FLASH_PAGE_SIZE) ? remaining : FLASH_PAGE_SIZE;
        memset(bin_head, 0, current_chunk);
        // uint8_t retry_count = 0;
        // do
        // {
        disable_global_irq(CSR_MSTATUS_MIE_MASK);
        ret = ota_board_flash_read(addr + processed, bin_head, current_chunk);
        enable_global_irq(CSR_MSTATUS_MIE_MASK);
        // if (ret == FLASH_RET_SUCCESS)
        // {
        //     break;
        // }
        //     retry_count++;
        // } while (retry_count < 3);

        if (ret != FLASH_RET_SUCCESS)
        {
            return ret;
        }

        crc = crc16_continue(crc, bin_head, current_chunk);
        processed += current_chunk;
    }

    *crc_out = crc;

    return ret;
}

static void dfu_reboot(void)
{
    hal_timer_sw_stop();
    rgb_led_set_uninit();
    //ota_board_complete_reset();
}
/**
 * @description:
 * @return {*}
 */
static void timer_timeout_callback(void)
{
    switch (g_timeout)
    {
    case DFU_TIMEOUT_NONE:
        hal_timer_sw_start();
        uint8_t send_buffer[5];
        send_buffer[0] = s_current_sequence_number & 0xff;
        send_buffer[1] = (s_current_sequence_number >> 8) & 0xff;
        send_buffer[2] = (s_current_sequence_number >> 16) & 0xff;
        send_buffer[3] = (s_current_sequence_number >> 24) & 0xff;
        send_buffer[4] = DFU_ACK_TIMEOUT_ERR;
        cmd_do_response(CMD_SYS, SYS_DFU_TRANS, sizeof(send_buffer), 0, send_buffer);
        g_timeout = DFU_TIMEOUT_ONCE;
        break;

    case DFU_TIMEOUT_ONCE:
        dfu_reboot();
        g_timeout = DFU_TIMEOUT_TWICE;
        break;
    
    default:
        break;
    }
}
static uint32_t dfu_finish(void)
{
    hal_timer_sw_init(APP_TIMER_SHOT, dfu_reboot);
    hal_timer_sw_start();

    return DFU_ACK_SUCCESS;
}

uint32_t dfu_trans_process(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    uint8_t i;
    uint8_t verify_bin[64];
    uint32_t sequence_number;
    uint32_t write_len;
    uint32_t write_pos;
    // static uint32_t offset;
    uint8_t *p_data_addr;
    uint8_t send_buffer[5] = {0};
    DBG_PRINTF("%s\n", __FUNCTION__);

    if (len > 56)
    {
        send_buffer[4] = DFU_ACK_SIZE_ERR;
        return cmd_do_response(CMD_SYS, SYS_DFU_TRANS, sizeof(send_buffer), 0, send_buffer);
    }

    if ((dfu_step != DFU_STEP_START_MCU) && (dfu_step != DFU_STEP_START_DMP) && (dfu_step != DFU_STEP_START_GM))
    {
        send_buffer[4] = DFU_ACK_STATE_ERR;
        return cmd_do_response(CMD_SYS, SYS_DFU_TRANS, sizeof(send_buffer), 0, send_buffer);
    }
    else
    {
        hal_timer_sw_start();
        g_timeout = DFU_TIMEOUT_NONE;
    }

    if (!b_dfu_start)
    {
        DBG_PRINTF("on_recv_bin err! dfu not start\n");
        send_buffer[4] = DFU_ACK_STATE_ERR;
        return cmd_do_response(CMD_SYS, SYS_DFU_TRANS, sizeof(send_buffer), 0, send_buffer);
    }

    memcpy(bin_buffer, pdata, len);

    uint32_t *p_sn = (uint32_t *)bin_buffer;

    if ((s_current_sequence_number) != *p_sn)
    {
        DBG_PRINTF("pkterr %d,%d\n", s_current_sequence_number, *p_sn);
        send_buffer[4] = DFU_ACK_PKT_ERR;
        b_dfu_start = 0;
        return cmd_do_response(CMD_SYS, SYS_DFU_TRANS, sizeof(send_buffer), 0, send_buffer);
    }

    if (DFU_OBJ_MCU == dfu_obj)
    {
        write_pos = app_boot_start + DFU_BIN_FRAME_SIZE * s_current_sequence_number;
        write_len = len - 4;
        p_data_addr = bin_buffer + 4;


        if (write_pos < (app_boot_start + app_page_size))
        {
            if ((app_boot_start + app_page_size) - write_pos >= DFU_BIN_FRAME_SIZE)
            {

                memcpy(&bin_head[write_pos - app_boot_start], p_data_addr, write_len);
                // offset = 0;
            }
            else
            {

                memcpy(&bin_head[write_pos - app_boot_start], p_data_addr, (app_boot_start + app_page_size) - write_pos);

                write_len = DFU_BIN_FRAME_SIZE - ((app_boot_start + app_page_size) - write_pos);
                // offset = write_len;
                taskENTER_CRITICAL();
                ota_board_flash_write(app_boot_start + app_page_size, p_data_addr + ((app_boot_start + app_page_size) - write_pos), write_len);
                taskEXIT_CRITICAL();
                for (i = 0; i < write_len; i++)
                {
                    if (verify_bin[i] != bin_buffer[i + 4 + ((app_boot_start + app_page_size) - write_pos)])
                    {
                        // DBG_PRINTF("2verify fail at address 0x%08x,f = 0x%02x r= 0x%02x\r\n",write_pos+i,verify_bin[i],bin_buffer[i+4]);
                        send_buffer[4] = DFU_ACK_PKT_CHECK_ERR;
                        b_dfu_start = 0;
                        return cmd_do_response(CMD_SYS, SYS_DFU_TRANS, sizeof(send_buffer), 0, send_buffer);
                    }
                }

            }
        }
        else
        {
            if (s_current_sequence_number == sw_version_pack_mark)
            {
                sw_version_write_pos = write_pos;

                memcpy(sw_version_write_smile, p_data_addr, write_len);

            }
            else
            {
                taskENTER_CRITICAL();
                ota_board_flash_write(write_pos, p_data_addr, write_len);

                ota_board_flash_read(write_pos, verify_bin, write_len);
                taskEXIT_CRITICAL();
                for (i = 0; i < write_len; i++)
                {
                    if (verify_bin[i] != bin_buffer[i + 4])
                    {
                        DBG_PRINTF("verify fail at address 0x%08x,f = 0x%02x r= 0x%02x\r\n", write_pos + i, verify_bin[i], bin_buffer[i + 4]);
                        send_buffer[4] = DFU_ACK_PKT_CHECK_ERR;
                        b_dfu_start = 0;
                        return cmd_do_response(CMD_SYS, SYS_DFU_TRANS, sizeof(send_buffer), 0, send_buffer);
                    }
                }
            }
        }
    }
    else if (DFU_OBJ_DPM == dfu_obj)
    {
        write_len = len - 4;
        p_data_addr = bin_buffer + 4;

        memcpy(&bin_head[5] + DFU_BIN_FRAME_SIZE * (s_current_sequence_number % DFU_SCREEN_FRAME_SIMPLE), p_data_addr, write_len);

        if (s_current_sequence_number == (total_number - 1))
        {

            bin_head[0] = FW_TRANS_FINISH;
            *(uint32_t *)&bin_head[1] = s_current_sequence_number / DFU_SCREEN_FRAME_SIMPLE;
            uint16_t len = (s_current_sequence_number % DFU_SCREEN_FRAME_SIMPLE) * DFU_BIN_FRAME_SIZE + write_len + 5;
            uint32_t ret = uart_set_screen_dfu(bin_head, len);
            if (ret != DFU_ACK_SUCCESS)
            {
                send_buffer[4] = GET_8BIT(DFU_SLAVE_NULL + ret);
                return cmd_do_response(CMD_SYS, SYS_DFU_TRANS, sizeof(send_buffer), 0, send_buffer);
            }
        }
        else if ((DFU_SCREEN_FRAME_SIMPLE-1) == (s_current_sequence_number % DFU_SCREEN_FRAME_SIMPLE))
        {
            bin_head[0] = FW_TRANS_PROG;
            *(uint32_t *)&bin_head[1] = s_current_sequence_number / DFU_SCREEN_FRAME_SIMPLE;
            uint16_t len = DFU_SCREEN_FRAME_SIMPLE * DFU_BIN_FRAME_SIZE + 5;
            uint32_t ret = uart_set_screen_dfu(bin_head, len);
            if (ret != DFU_ACK_SUCCESS)
            {
                send_buffer[4] = GET_8BIT(DFU_SLAVE_NULL + ret);
                return cmd_do_response(CMD_SYS, SYS_DFU_TRANS, sizeof(send_buffer), 0, send_buffer);
            }
        }
    }
    else if (DFU_OBJ_GM == dfu_obj)
    {
        static bool is_first_write = true;
        write_len = len - 4;
        p_data_addr = bin_buffer + 4;

        memcpy(&bin_head[5] + DFU_BIN_FRAME_SIZE * (s_current_sequence_number % GM_SUBPACKETS_PER_PACKET), p_data_addr, write_len);

        if (s_current_sequence_number == (total_number - 1))
        {
            taskENTER_CRITICAL();

            //uint32_t subpacket_in_current_packet = s_current_sequence_number % GM_SUBPACKETS_PER_PACKET;
            //uint32_t write_subpacket_count = subpacket_in_current_packet + 1;
            uint32_t offset = (s_current_sequence_number / GM_SUBPACKETS_PER_PACKET) * DFU_BIN_FRAME_SIZE * GM_SUBPACKETS_PER_PACKET;

            //db_write_gm_fw(offset, &bin_head[5], write_subpacket_count * DFU_BIN_FRAME_SIZE);
            db_write_gm_fw(app_boot_curr + offset, &bin_head[5], gm_fw_trans_info.total_size - offset);
            gm_fw_info_t gm_fw_info = {0};
            gm_fw_info.is_vaild = 1;  
            gm_fw_info.fw_ver  = gm_fw_trans_info.fw_ver;
            gm_fw_info.fw_size = gm_fw_trans_info.total_size;
            gm_fw_info.check   = gm_fw_trans_info.check;

            db_write_gm_fw_info(app_boot_start, &gm_fw_info);
            is_first_write = true;

            taskEXIT_CRITICAL();

        }
        else if ((GM_SUBPACKETS_PER_PACKET - 1) == (s_current_sequence_number % GM_SUBPACKETS_PER_PACKET))
        {
            taskENTER_CRITICAL();
            if (is_first_write)
            {
                is_first_write = false;
                db_erase_gm_fw_info(app_boot_start);
                db_erase_gm_fw(app_boot_curr);
            }

            uint32_t offset = (s_current_sequence_number / GM_SUBPACKETS_PER_PACKET) * DFU_BIN_FRAME_SIZE * GM_SUBPACKETS_PER_PACKET;
            db_write_gm_fw(app_boot_curr + offset, &bin_head[5], GM_SUBPACKETS_PER_PACKET * DFU_BIN_FRAME_SIZE);
            taskEXIT_CRITICAL();
        }
    }

    if (s_current_sequence_number == (total_number -1))
    {
        if (DFU_OBJ_MCU == dfu_obj)
        {
            ota_board_flash_write(sw_version_write_pos , sw_version_write_smile, DFU_BIN_FRAME_SIZE);

            uint16_t new_version = 0;
            ota_board_flash_read(sw_version_write_pos + sw_version_offset_mark, &new_version, 2);
            uint16_t curr_version = 0;
            ota_board_flash_read(app_boot_curr + FW_HEADER_SW_VERSION_OFFSET, &curr_version, 2);
            DBG_PRINTF("compare  new %d now %d\n", new_version, curr_version);
            if (new_version < curr_version )
            {
                memset(&mcu_fota_info, 0, sizeof(fw_trans_info_t));
                ota_board_flash_read(app_boot_check, &mcu_fota_info, sizeof(fw_trans_info_t));
                uint16_t check = 0;
                if (dfu_crc16_full(&check, app_boot_start, mcu_fota_info.total_size) != FLASH_RET_SUCCESS)
                {
                    send_buffer[4] = DFU_ACK_FLASH_ERR;
                    return cmd_do_response(CMD_SYS, SYS_DFU_TRANS, sizeof(send_buffer), 0, send_buffer);
                }
                else
                {
                    if ((mcu_fota_info.check == (uint32_t)check))
                    {
                        taskENTER_CRITICAL();
                        ota_board_flash_read(app_boot_curr, bin_head, FLASH_PAGE_SIZE);
                        bin_head[FW_HEADER_SW_VERSION_OFFSET] = 0x0;
                        bin_head[FW_HEADER_SW_VERSION_OFFSET + 1] = 0x0;
                        // bin_head[FW_HEADER_HASH_OFFSET] &= 0xF0;
                        if (hal_flash_erase(app_boot_curr, 1) == FLASH_RET_SUCCESS)
                        {
                            ota_board_flash_write(app_boot_curr, bin_head, FLASH_PAGE_SIZE);
                            taskEXIT_CRITICAL();
                        }
                        else
                        {
                            taskEXIT_CRITICAL();
                            send_buffer[4] = DFU_ACK_ERASE_ERR;
                            return cmd_do_response(CMD_SYS, SYS_DFU_TRANS, sizeof(send_buffer), 0, send_buffer);
                        }
                    }
                    else
                    {
                        send_buffer[4] = DFU_ACK_CHECK_ERR;
                        return cmd_do_response(CMD_SYS, SYS_DFU_TRANS, sizeof(send_buffer), 0, send_buffer);
                    }
                }
            }
            rgb_led_set_prompt(PROMPT_DFU_FLASH_COMPLETE);

            hal_timer_sw_stop();
            xTaskCHandle = xTaskGetCurrentTaskHandle();
            if (b_slave_scan == true)
            {
                uint32_t ulReturn = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(30000));
                if (ulReturn != 0)
                {
                    DBG_PRINTF("start checkout\n");
                    uint8_t device_bit = dfu_device_get_upgrade();
                    if (device_bit != 0x1F)
                    {
                        /* code */
                    }
                }
                else
                {
                    DBG_PRINTF("slave wait timout...\n");
                }
            }
            b_slave_scan = false;
            dfu_step = DFU_STEP_IDLE;
            xTaskCHandle = NULL;

            dfu_finish();
            DBG_PRINTF("dfu finish %d, now jump boot!\n", total_number);
        }
        else if (DFU_OBJ_DPM == dfu_obj)
        {
            hal_timer_sw_stop();
            rgb_led_set_prompt(PROMPT_DFU_FLASH_COMPLETE);
            xTaskCHandle = xTaskGetCurrentTaskHandle();
            if (b_slave_scan == true)
            {
                uint32_t ulReturn = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(30000));
                if (ulReturn != 0)
                {
                    DBG_PRINTF("start checkout\n");
                    uint8_t device_bit = dfu_device_get_upgrade();
                    if (device_bit != 0x1F)
                    {
                        /* code */
                    }
                }
                else
                {
                    DBG_PRINTF("slave wait timout...\n");
                }
            }
            b_slave_scan = false;
            xTaskCHandle = NULL;
        }
        else if (DFU_OBJ_GM == dfu_obj)
        {
            hal_timer_sw_stop();
            mg_uart_tran_start(gm_fw_trans_info.fw_ver);
            rgb_led_set_prompt(PROMPT_DFU_FLASH_COMPLETE);
            b_slave_scan = true;
        }

        b_dfu_start = 0;
    }

    sequence_number = s_current_sequence_number++;
    send_buffer[0] = sequence_number & 0xff;
    send_buffer[1] = (sequence_number >> 8) & 0xff;
    send_buffer[2] = (sequence_number >> 16) & 0xff;
    send_buffer[3] = (sequence_number >> 24) & 0xff;
    return cmd_do_response(CMD_SYS, SYS_DFU_TRANS, sizeof(send_buffer), 0, send_buffer);

}

uint32_t on_sys_dfu_req(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    DBG_PRINTF("%s\n",__FUNCTION__);
    
    uint8_t buf[3] = {0};
    uint32_t firmwarm_size = pdata[4] | (pdata[5] << 8) | (pdata[6] << 16) | (pdata[7] << 24);
    dfu_obj = *(uint32_t *)pdata;
    b_dfu_start = 0;

    if ((dfu_step != DFU_STEP_IDLE) && (dfu_step != DFU_STEP_START_DMP) && (dfu_step != DFU_STEP_START_GM))
    {
        buf[0] = DFU_ACK_STATE_ERR;
        // DBG_PRINTF("dfu_step ERR=%d\n",dfu_step);
        return cmd_do_response(CMD_SYS, SYS_DFU_REQ, 3, 0, buf);
    }
    else 
    {
        if (DFU_OBJ_DPM == dfu_obj)
        {
            if ((firmwarm_size < 0xff) || (firmwarm_size > 3 * 1024 * 1024))
            {
                buf[0] = DFU_ACK_SIZE_ERR;
                // DBG_PRINTF("DFU_ACK_SIZE_ERR=%d\n",firmwarm_size);
                return cmd_do_response(CMD_SYS, SYS_DFU_REQ, 3, 0, buf);
            }
        }
        else
        {
            if ((firmwarm_size < 0xff) || (firmwarm_size > 240 * 1024))
            {
                buf[0] = DFU_ACK_SIZE_ERR;
                // DBG_PRINTF("DFU_ACK_SIZE_ERR=%d\n",firmwarm_size);
                return cmd_do_response(CMD_SYS, SYS_DFU_REQ, 3, 0, buf);
            }
        }
    }

    s_current_sequence_number = 0;
    total_number = firmwarm_size / DFU_BIN_FRAME_SIZE;
    if (firmwarm_size % DFU_BIN_FRAME_SIZE)
    {
        total_number++;
        DBG_PRINTF("total_number %d\r\n", total_number);
    }

    if (DFU_OBJ_MCU == dfu_obj)
    {
        matrix_uninit();
        mg_sm_uninit();
        mg_detected_suspend();
        db_save_now();
        db_task_suspend();
        if (dfu_step == DFU_STEP_IDLE)
        {
            rgb_led_set_prompt(PROMPT_DFU_FLASH_START);
        }
        else
        {
            rgb_led_set_prompt(PROMPT_DFU_FLASH_PROGRESS);
        }
        if (pdata[16] == 0x01)
        {
            gbinfo.itf = 0;
            db_update(DB_SYS, UPDATE_NOW);
            vTaskDelay(20);
        }

        uint8_t ota_index = ota_check_current_otaindex();
        uint8_t block = ota_index ? 0 : 1;

        bool exip = 0;
        hal_otp_get_exip(&exip);
        if (exip)
        {
            app_boot_start = ota_index ? FLASH_APP1_EXIP_INFO_ADDR : FLASH_APP2_EXIP_INFO_ADDR;
            sw_version_pack_mark = (FLASH_APP1_EXIP_INFO_SIZE + FLASH_APP1_NOR_CFG_SIZE + FW_HEADER_SW_VERSION_OFFSET) / DFU_BIN_FRAME_SIZE;
            sw_version_offset_mark = (FLASH_APP1_EXIP_INFO_SIZE + FLASH_APP1_NOR_CFG_SIZE + FW_HEADER_SW_VERSION_OFFSET) % DFU_BIN_FRAME_SIZE;
        }
        else
        {
            app_boot_start = ota_index ? FLASH_APP1_NOR_CFG_ADDR : FLASH_APP2_NOR_CFG_ADDR;
            sw_version_pack_mark = (FLASH_APP1_NOR_CFG_SIZE + FW_HEADER_SW_VERSION_OFFSET) / DFU_BIN_FRAME_SIZE;
            sw_version_offset_mark = (FLASH_APP1_NOR_CFG_SIZE + FW_HEADER_SW_VERSION_OFFSET) % DFU_BIN_FRAME_SIZE;
        }
        app_boot_curr = ota_index ? FLASH_APP2_BOOT_HEAD_ADDR : FLASH_APP1_BOOT_HEAD_ADDR;
        DBG_PRINTF("exip %d now runing OTA%d...\n", exip, ota_index + 1);
        DBG_PRINTF("block %d  app_boot_start %x end...\n", block, app_boot_start);
        DBG_PRINTF("sw_version_pack_mark %d  sw_version_offset_mark %x ...\n", sw_version_pack_mark, sw_version_offset_mark);
        // erase ready
        if (DFU_ACK_SUCCESS != ota_board_flash_erase(block))
        {
            buf[0] = DFU_ACK_ERASE_ERR;
            return cmd_do_response(CMD_SYS, SYS_DFU_REQ, 3, 0, buf);
        }
        else
        {
            memset(&mcu_fota_info, 0, sizeof(fw_trans_info_t));

            mcu_fota_info.total_size = *(uint32_t *)(pdata + 4);
            mcu_fota_info.fw_ver = *(uint32_t *)(pdata + 8);
            mcu_fota_info.check = *(uint32_t *)(pdata + 12);

            app_boot_check = ota_index ? FLASH_APP1_IMGINFO_ADDR : FLASH_APP2_IMGINFO_ADDR;
            if (hal_flash_erase(app_boot_check, 1) == FLASH_RET_SUCCESS)
            {
                ota_board_flash_write(app_boot_check, (uint8_t *)&mcu_fota_info, sizeof(fw_trans_info_t));
            }
        }

        uint8_t pack_info = FW_TRANS_BUSY;
        uart_set_screen_dfu(&pack_info, sizeof(pack_info));
        hal_timer_sw_init(APP_TIMER_ACK, timer_timeout_callback);
        dfu_step = DFU_STEP_START_MCU;
    }
    else if (DFU_OBJ_DPM == dfu_obj)
    {
        matrix_uninit();
        mg_sm_uninit();
        mg_detected_suspend();
        db_save_now();
        db_task_suspend();
        if (dfu_step == DFU_STEP_IDLE)
        {
            rgb_led_set_prompt(PROMPT_DFU_FLASH_START);
        }
        else
        {
            rgb_led_set_prompt(PROMPT_DFU_FLASH_PROGRESS);
        }

        dpm_fw_trans_info_t dpm_fw_trans_info = {0};
        dpm_fw_trans_info.total_size = firmwarm_size;
        dpm_fw_trans_info.fw_ver = *(uint32_t *)(pdata + 8);
        dpm_fw_trans_info.payload_max_len = DFU_BIN_FRAME_SIZE * 4;
        dpm_fw_trans_info.total_pkg_num = (dpm_fw_trans_info.total_size + dpm_fw_trans_info.payload_max_len - 1) / dpm_fw_trans_info.payload_max_len;

        uint8_t pack_info[15] = {0};
        pack_info[0] = FW_TRANS_START;
        *(uint32_t *)&pack_info[1] = dpm_fw_trans_info.fw_ver;
        *(uint32_t *)&pack_info[5] = dpm_fw_trans_info.total_size;
        *(uint32_t *)&pack_info[9] = dpm_fw_trans_info.total_pkg_num;
        *(uint16_t *)&pack_info[13] = dpm_fw_trans_info.payload_max_len;
        uint32_t ret = uart_set_screen_dfu(pack_info, sizeof(pack_info));
        if (ret != DFU_ACK_SUCCESS)
        {
            buf[0] = GET_8BIT(DFU_SLAVE_NULL + ret);
            return cmd_do_response(CMD_SYS, SYS_DFU_REQ, 3, 0, buf);
        }

        hal_timer_sw_init(APP_TIMER_SCREEN, timer_timeout_callback);
        dfu_step = DFU_STEP_START_DMP;
    }
    else if (DFU_OBJ_GM == dfu_obj)
    {
        if (dfu_get_slave_status() == SM_STATUS_IDLE)
        {
            matrix_uninit();
            mg_sm_uninit();
            mg_detected_suspend();
            db_save_now();
            db_task_suspend();
            if (dfu_step == DFU_STEP_IDLE)
            {
                rgb_led_set_prompt(PROMPT_DFU_FLASH_START);
            }
            else
            {
                rgb_led_set_prompt(PROMPT_DFU_FLASH_PROGRESS);
            }

            uint8_t ota_index = dfu_check_sm_current_app();
            if (ota_index != 1)
            {
                app_boot_start = FLASH_GM_FW_INFO_ADDR;
                app_boot_curr = FLASH_GM_FW_ADDR;
            }
            else
            {
                app_boot_start = FLASH_GM_FW2_INFO_ADDR;
                app_boot_curr = FLASH_GM_FW2_ADDR;
            }

            memset(&gm_fw_trans_info, 0, sizeof(fw_trans_info_t));

            gm_fw_trans_info.total_size = *(uint32_t *)(pdata + 4);
            gm_fw_trans_info.fw_ver = *(uint32_t *)(pdata + 8);
            gm_fw_trans_info.check = *(uint32_t *)(pdata + 12);

            uint8_t pack_info = FW_TRANS_BUSY;
            uart_set_screen_dfu(&pack_info, sizeof(pack_info));
            hal_timer_sw_init(APP_TIMER_GM, timer_timeout_callback);
            dfu_step = DFU_STEP_START_GM;
        }
        else
        {
            buf[0] = DFU_ACK_SLAVE_SW_BUSY;
            return cmd_do_response(CMD_SYS, SYS_DFU_REQ, 3, 0, buf);
        }
    }

    buf[2] = DFU_BIN_FRAME_SIZE >> 8;
    buf[1] = DFU_BIN_FRAME_SIZE & 0xff;
    
    b_dfu_start = 1;

    return cmd_do_response(CMD_SYS, SYS_DFU_REQ, 3, 0, buf);
}
