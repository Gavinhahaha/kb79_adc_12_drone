/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
/* FreeRTOS kernel includes. */
#include "hal_hid.h"
#include "hal_flash.h"
#include "FreeRTOS.h"
#include "task.h"

#include "usb_osal.h"
#include "usbd_core.h"
#include "usbd_hid.h"
#include "usb_config.h"
#include "board.h"
#include "hpm_gpio_drv.h"
#include "app_config.h"
#include "easy_fifo.h"
#include "mg_hive.h"
#include "mg_key_code.h"
#include "mg_matrix_ctrl.h"
#include "mg_matrix.h"
#include "mg_cali.h"
#include "dfu.h"
#include "dfu_api.h"
#include "mg_hid.h"
#include "interface.h"
#include "db.h"
#include "easy_crc.h"
#include "rgb_led_ctrl.h"
#include "rgb_led_color.h"
#include "app_debug.h"
#include "rgb_light_tape.h"
#include "rgb_led_action.h"
#include "rgb_lamp_sync.h"
#include "power_save.h"
#include "mg_uart.h"
#include "drv_dpm.h"
#include "mg_detection.h"
#include "mg_factory.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (DBG_EN_HIVE)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "mg_hive"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif


#define hive_PRIORITY (configMAX_PRIORITIES - 5U)
TaskHandle_t m_hive_thread;

#define HIVE_FIFO_SIZE                                 sizeof(hive_data_t)
#define HIV_FIFO_NUM                                   8
#define SEND_SIZE                                      10


static rep_hid_t hive_send_q[SEND_SIZE];
static fifo_t  hive_data_fifo;//rx fifo
static uint8_t hive_data_fifo_buf[HIVE_FIFO_SIZE * HIV_FIFO_NUM] __attribute__ ((aligned (4)));
uint8_t hive_data_buf[HIV_FIFO_NUM][64] __attribute__ ((aligned (4)));

static uint8_t hive_data_buf_cnt = 0;
static uint8_t muxt = 0;

typedef enum
{
    SOFTDEVICE_CRC = 0,
    FIRMWARE_CRC,
    BOOTLOADER_CRC,
    UICR_CRC,
} crc_type_t; //

static const uint8_t mouth_str[12][4] = {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec",
};

sys_run_info_s sys_run = {KEY_DATA_ZERO_FRAME,0,0,5000,0};//默认电压5v

extern kh_t kh;
extern tmx_t gmx;

static hive_state_t s_conn_state = OFFLINE;

hive_state_t get_hive_state(void)
{
    return s_conn_state;
}

void get_build_datetime(uint8_t *pdate)
{
    int y = 0, m = 0, d = 0;
    int h = 0, m1 = 0, s = 0;
    uint8_t buf[4] = {0};

    sscanf(__DATE__, "%s %d %d", buf, &d, &y);
    sscanf(__TIME__, "%d:%d:%d", &h, &m1, &s);

    for (uint8_t i = 0; i < 12; ++i)
    {
        if (0 == strcmp((const char *)mouth_str[i], (const char *)buf))
        {
            m = i + 1;
        }
    }

    sprintf((char *)pdate, "%04d%02d%02d%02d%02d%02d", y, m, d, h, m1, s);

    for (uint8_t i = 0; i < 14; i++)
    {
        pdate[i] -= 0x30;
    }
}

static uint32_t on_sys_handshake(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint32_t e = HIVE_SUCCESS;

    if (len != 1)
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }

    if (*pdata == 0)
    {
        ks_cmd_set_key_report(pdata, len, NULL, 0, NULL);
        s_conn_state = OFFLINE;
    }
    else if (*pdata == 1)
    {
        if (mg_factory_checkout() == true)
        {
            uint8_t buff[sizeof(mg_factory_t) + 4] = {1};
            memcpy(&buff[4], mg_factory_get_info(), sizeof(mg_factory_t));
            cmd_do_response(CMD_NOTIFY, NTF_SW_FAULT, sizeof(buff), 0, buff);
        }
        s_conn_state = ONLINE_CONSUMER;
    }

    cmd_do_response(CMD_SYS, SYS_HS, sizeof(e), 0, (uint8_t *)&e);

    return e;
}

static uint32_t on_sys_reset(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;

    db_reset();
    db_task_suspend();
    uart_notify_screen_cmd(CMD_UART_SYS, SCMD_RESET, NULL, 0);
    rgb_led_uninit();
    vTaskDelay(100);
    board_sw_reset();

    return HIVE_SUCCESS;
}

static uint32_t on_sys_reboot(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;

    drv_dpm_power_en(IO_LOW);
    board_sw_reset();

    return HIVE_SUCCESS;
}

static uint32_t on_sys_get_vendor_info(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    uint8_t buf[40] = {0};
	(void) len;
    (void) reserve;
    (void) pdata;
    user_product_header_t product_info;

    *(uint32_t *)&buf[0] = gbinfo.pi.vc;
    *(uint32_t *)&buf[4] = gbinfo.pi.pc;
    memcpy(&buf[8], gbinfo.pi.vn, 16);

    ota_board_flash_read(FLASH_BOARD_INFO_ADDR, &product_info, sizeof(user_product_header_t));
    if (product_info.magic == BOARD_SN_NAME_MAGIC)
    {
        memcpy(&buf[24], product_info.device_name, product_info.len);
    }
    else if (product_info.magic == BOARD_SN_NAME_MAGIC2)
    {
        for (uint8_t i = 0; i < 16; i++)
        {
            if (((uint8_t *)&product_info)[i] == 0x00)
            {
                break;
            }

            buf[24 + i] = ((uint8_t *)&product_info)[i];
        }

    }
    else
    {
        memcpy(&buf[24], BOARD_NAME, strlen(BOARD_NAME));
    }

    return cmd_do_response(CMD_SYS, SYS_GET_FA_INFO, sizeof(buf), 0, buf);
}

/// @brief 
/// @param len 
/// @param reserve 
/// @param pdata 
/// @return 
static uint32_t on_sys_get_fw_version(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    uint8_t buf[28] = {0};
    uint8_t uuid[8] = {0};
    hal_otp_get_custom_uuid(uuid);

    *(uint16_t *)&buf[0] = BOARD_FW_VERSION;
    *(uint16_t *)&buf[2] = BOARD_HW_VERSION;
    get_build_datetime(buf + 4);
    memcpy(&buf[20],uuid,8);

    return cmd_do_response(CMD_SYS, SYS_GET_VER_INFO, sizeof(buf), 0, buf);
}

static uint32_t on_sys_get_name(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
	(void) len;
    (void) reserve;
    (void) pdata;
    user_product_header_t product_info;
    uint8_t name[16] = {0};
    ota_board_flash_read(FLASH_BOARD_INFO_ADDR, &product_info, sizeof(user_product_header_t));
    if (product_info.magic == BOARD_SN_NAME_MAGIC)
    {
        memcpy(name, product_info.device_name, product_info.len);
    }
    else if (product_info.magic == BOARD_SN_NAME_MAGIC2)
    {
        for (uint8_t i = 0; i < 16; i++)
        {
            if (((uint8_t *)&product_info)[i] == 0x00)
            {
                break;
            }

            name[i] = ((uint8_t *)&product_info)[i];
        }
    }

    return cmd_do_response(CMD_SYS, SYS_GET_DEV_NAME, sizeof(name), 0, name);
}

static uint32_t on_sys_set_name(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) pdata;
    
    uint32_t e = HIVE_SUCCESS;

    user_product_header_t product_info = {0};
    if (len > sizeof(product_info.device_name))
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }

    mg_factory_t *factory = mg_factory_get_info();
    factory->magic = FACTORY_VENDORS_MAGIC;
    mg_factory_set_info(factory);

    // len =11;
    hal_flash_erase(FLASH_BOARD_INFO_ADDR, 1);
    product_info.magic = BOARD_SN_NAME_MAGIC;
    product_info.len = len;
    memcpy(product_info.device_name, pdata, len);

    ota_board_flash_write(FLASH_BOARD_INFO_ADDR, &product_info, sizeof(user_product_header_t));

    return cmd_do_response(CMD_SYS, SYS_SET_DEV_NAME, 4, 0, (uint8_t *)&e);
}


static uint32_t on_sys_get_status(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    
    uint8_t buf[20] = {0};

    buf[0] = 1;
    buf[1] = KC_USB;

    return cmd_do_response(CMD_SYS, SYS_GET_DEV_STATE, sizeof(buf), 0, buf);
}

static uint32_t on_sys_get_ps(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    
    ps_e_t e = PS_NOT_SUPPORT;
    return cmd_do_response(CMD_SYS, SYS_GET_PS_MODE, 1, 0, (uint8_t *)&e);
}

static uint32_t on_sys_set_ps(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    
    ps_e_t e = PS_NOT_SUPPORT;
    return cmd_do_response(CMD_SYS, SYS_SET_PS_MODE, 1, 0, (uint8_t *)&e);
}

static uint32_t on_sys_get_inactive(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;

    uint8_t buff[3] = {0};
    
    buff[0] = gbinfo.power_save.en == true ? 1 : 0;
    *(uint16_t *)&buff[1] = gbinfo.power_save.time;

    return cmd_do_response(CMD_SYS, SYS_GET_SLEEP_TIME, sizeof(buff), 0, buff);
}

static uint32_t on_sys_set_inactive(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    
    uint32_t e = HIVE_SUCCESS;

    bool     tmp_en     = pdata[0] == 1 ? true : false;
    uint16_t tmp_time   = *(uint16_t *)&pdata[1];

    if (ps_timer_update(tmp_en, tmp_time))
    {
        gbinfo.power_save.en   = tmp_en;
        gbinfo.power_save.time = tmp_time;

        db_update(DB_SYS, UPDATE_LATER);
    }
    else
    {
        e = HIVE_ERROR_INVALID_PARAM;
    }

    return cmd_do_response(CMD_SYS, SYS_SET_SLEEP_TIME, 4, 0, (uint8_t *)&e);
}

static uint32_t on_sys_get_sn_code(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    uint32_t buff[8] = {0};

    return cmd_do_response(CMD_SYS, SYS_GET_SN_CODE, sizeof(buff), 0, (uint8_t *)buff);
}

static uint32_t on_sys_set_sn_code(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    uint32_t e = HIVE_SUCCESS;
    //uint32_t *pbuf = (uint32_t *)pdata;
    //unused

    return cmd_do_response(CMD_SYS, SYS_SET_SN_CODE, 4, 0, (uint8_t *)&e);
}

static uint32_t on_sys_get_hive_ver(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    uint16_t ver;

#ifdef RESET_CHIP_ID
    {
        user_product_header_t product_info;
        uint8_t name[16] = {0};
        // uint8_t id_new[] = {0x4D, 0x41, 0x44, 0x45, 0x36, 0x38, 0x55, 0x4C, 0x54, 0x52, 0x41, 0x42, 0x4C}; // BL
        // uint8_t id_new[] = {0x4D, 0x41, 0x44, 0x45, 0x36, 0x38, 0x55, 0x4C, 0x54, 0x52, 0x41, 0x47, 0x4C}; // GL
        uint8_t id_new[] = {0x4D, 0x41, 0x44, 0x45, 0x36, 0x38, 0x55, 0x4C, 0x54, 0x52, 0x41, 0x52, 0x4C}; // RL
        ota_board_flash_read(FLASH_BOARD_INFO_ADDR, &product_info, sizeof(user_product_header_t));
        if (product_info.magic == BOARD_SN_NAME_MAGIC)
        {
            memcpy(name, product_info.device_name, product_info.len);
        }
        else if (product_info.magic == BOARD_SN_NAME_MAGIC2)
        {
            for (uint8_t i = 0; i < 16; i++)
            {
                if (((uint8_t *)&product_info)[i] == 0x00)
                {
                    break;
                }

                name[i] = ((uint8_t *)&product_info)[i];
            }
        }
        if (memcmp(name, id_new, sizeof(id_new)) != 0)
        {
            hal_flash_erase(FLASH_BOARD_INFO_ADDR, 1);
            product_info.magic = BOARD_SN_NAME_MAGIC;
            product_info.reserve = 1; //  over
            product_info.len = sizeof(id_new);
            memcpy(product_info.device_name, id_new, product_info.len);

            ota_board_flash_write(FLASH_BOARD_INFO_ADDR, &product_info, sizeof(user_product_header_t));
        }
    }
#endif

    ver = BOARD_HIVE_VER;

    return cmd_do_response(CMD_SYS, SYS_GET_HIVE_VER, sizeof(uint16_t), 0, (uint8_t *)&ver);
}

static uint32_t on_sys_get_cfg_idx(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    uint8_t idx = common_info.cfg_info.cfg_idx;
    
    return cmd_do_response(CMD_SYS, SYS_GET_CFG_IDX, sizeof(uint8_t), 0, &idx);
}

static uint32_t on_sys_set_cfg_idx(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    uint32_t e = 0;
    uint8_t idx = pdata[0];
    
    if (idx < KEYBOARD_CFG_MAX)
    {
        if (common_info.cfg_info.cfg_idx != idx)
        {
            common_info.cfg_info.cfg_idx = idx;
            db_send_toggle_cfg_signal();
        }
    }
    else
    {
        e = 1;
    }
    return cmd_do_response(CMD_SYS, SYS_SET_CFG_IDX, sizeof(uint32_t), 0, (uint8_t *)&e);
}

static uint32_t on_sys_get_cfg(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    uint8_t buff[CMD_DATA_MAX - 1] = {0};
    uint8_t idx = pdata[0];
    uint8_t block_id = pdata[1];
    uint16_t packet_id = *(uint16_t *)&pdata[2];
    uint16_t offset = 0;
    db_t block_db_map[] = {
        [REQ_BLOCK_0] = DB_SYS,
        [REQ_BLOCK_1] = DB_SK_LA_LM_KC,
        [REQ_BLOCK_2] = DB_KR,
        [REQ_BLOCK_3] = DB_MACRO,
    };
    
    if (idx < KEYBOARD_CFG_MAX)
    {
        if (0xffff == packet_id)
        {
            buff[0] = REQ_START;
            *(uint16_t *)(buff + 1) = sizeof(binfo_t);
            *(uint16_t *)(buff + 3) = sizeof(sk_la_lm_kc_info);
            *(uint16_t *)(buff + 5) = sizeof(key_led_color_info_t);
            *(uint16_t *)(buff + 7) = sizeof(macro_info);
            buff[9] = CMD_DATA_MAX - 1;
        }
        else
        {
            buff[0] = block_id;
            *(uint16_t *)(buff + 1) = packet_id;
            offset = packet_id * (CMD_DATA_MAX - 4);
            if (1)//(db_check_export_cfg_vaild(idx, block_db_map[block_id])) 
            {
                db_read_cfg_block(idx, block_db_map[block_id], offset, buff + 3, CMD_DATA_MAX - 4);
            }
            else
            {
                db_read_cfg_block(common_info.cfg_info.cfg_idx, block_db_map[block_id], offset, buff + 3, CMD_DATA_MAX - 4);
            }
        }
    }
    return cmd_do_response(CMD_SYS, SYS_GET_CFG, CMD_DATA_MAX - 1, 0, (uint8_t *)buff);
}

static uint32_t on_sys_set_cfg(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint32_t e         = 0;
    static uint8_t idx = 0;
    uint8_t block_id   = 0;
    uint16_t offset    = 0;
    uint16_t packet_id = 0;
    uint8_t buff[52]   = {0};

    uint16_t w_size[] = 
    {
        [DB_SYS]         = sizeof(binfo_t),
        [DB_SK_LA_LM_KC] = sizeof(sk_la_lm_kc_info),
        [DB_KR]          = sizeof(key_led_color_info_t),
        [DB_MACRO]       = sizeof(macro_info),
    };
        
    if (REQ_START == pdata[0])
    {
        if (pdata[6] < KEYBOARD_CFG_MAX)
        {
            idx = pdata[6];
        }
        else
        {
            e = 1;
        }
    }
    else if (REQ_FINISH == pdata[0])
    {
        uint8_t* buffer = (uint8_t *)dfu_get_bin_head_buf();

        for (db_t i = DB_SYS; i < DB_CALI; i++)
        {
            taskENTER_CRITICAL();

            db_read_cfg_cache(i, buffer, w_size[i]);
            switch (i)
            {
                case DB_SYS:          db_check_binfo((binfo_t *)buffer);                        break;
                case DB_SK_LA_LM_KC:  db_check_sk_la_lm_kc_para((sk_la_lm_kc_info_t *)buffer);  break;
                case DB_KR:           db_check_kr_para((key_led_color_info_t *)buffer);         break;
                case DB_MACRO:    break;
                default:          break;
            }
            db_write_cfg_block(idx, i, buffer, w_size[i]);

            taskEXIT_CRITICAL();
        }

        if (idx == common_info.cfg_info.cfg_idx)
        {
            db_send_toggle_cfg_signal();
        }
    }
    else
    {
        block_id  = pdata[0] - 1;
        packet_id = *(uint16_t *)&pdata[1];
        offset    = packet_id * (len - 3);
        memcpy(buff, pdata + 3, 52);
        DBG_PRINTF(" block_id = %d packet_id = %d offset = %d len = %d\r\n",block_id, packet_id, offset, len);
        if (0 == packet_id)
        {
            taskENTER_CRITICAL();
            db_erase_cfg_cache(block_id);
            db_write_cfg_cache(block_id, offset, buff, 52);
            taskEXIT_CRITICAL();
        }
        else
        {
            taskENTER_CRITICAL();
            if ((offset + 52) < w_size[block_id])
            {
                db_write_cfg_cache(block_id, offset, buff, 52);
            }
            else
            {
                db_write_cfg_cache(block_id, offset, buff, w_size[block_id] - offset);
            }
            taskEXIT_CRITICAL();
        }
    }
    return cmd_do_response(CMD_SYS, SYS_SET_CFG, sizeof(uint32_t), 0, (uint8_t *)&e);
}

static uint32_t on_sys_get_cfg_name(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    uint8_t buff[64] = {0};
    uint8_t idx = pdata[0];
    uint8_t length = 0;
    
    if (0xff == idx) 
    {
        
    }
    else
    {
        length = KEYBOARD_CFG_NAME_SIZE;
        memcpy(buff, common_info.cfg_info.cfg_name[idx], length);
    }
    return cmd_do_response(CMD_SYS, SYS_GET_CFG_NAME, length, 0, buff);
}

static uint32_t on_sys_set_cfg_name(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    uint32_t e = 0;
    uint8_t idx = pdata[0];
    
    if (idx < KEYBOARD_CFG_MAX)
    {
        memcpy(common_info.cfg_info.cfg_name[idx], pdata + 1, KEYBOARD_CFG_NAME_SIZE);
        db_update(DB_COMMON, UPDATE_NOW);
    }
    else
    {
        e = 1;
    }
    return cmd_do_response(CMD_SYS, SYS_SET_CFG_NAME, sizeof(uint32_t), 0, (uint8_t *)&e);
}

static uint32_t on_sys_get_cfg_size(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    uint8_t buff[11] = {0};

    *(uint16_t *)(buff + 0) = 0;
    *(uint16_t *)(buff + 2) = sizeof(binfo_t);
    *(uint16_t *)(buff + 4) = sizeof(sk_la_lm_kc_info);
    *(uint16_t *)(buff + 6) = sizeof(key_led_color_info_t);
    *(uint16_t *)(buff + 8) = sizeof(macro_info);
    buff[10] = CMD_DATA_MAX - 1;
    return cmd_do_response(CMD_SYS, SYS_GET_CFG_SIZE, sizeof(buff), 0, buff);
}

static uint32_t on_sys_set_player_number(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    uint32_t e = 0;

    gbinfo.player_number = *(uint32_t *)pdata;
    db_update(DB_SYS, UPDATE_LATER);
    
    return cmd_do_response(CMD_SYS, SYS_SET_PLAYER_NUMBER, sizeof(uint32_t), 0, (uint8_t *)&e);
}

static uint32_t on_sys_get_player_number(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;

    uint32_t player_number = gbinfo.player_number;

    return cmd_do_response(CMD_SYS, SYS_GET_PLAYER_NUMBER, sizeof(uint32_t), 0, (uint8_t *)&player_number);
}

static uint32_t on_sys_get_netbar_mode(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;

    uint8_t netbar_mode_en = common_info.netbar_info.mode_en;

    return cmd_do_response(CMD_SYS, SYS_GET_NETBAR_MODE, sizeof(uint8_t), 0, (uint8_t *)&netbar_mode_en);
}

static uint32_t on_sys_set_netbar_mode(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    uint32_t e = 0;

    if (pdata[0] > 1)
    {
        e = 1;
    }
    else
    {
        common_info.netbar_info.mode_en = pdata[0];
        if (NETBAR_MODE_EN == common_info.netbar_info.mode_en)
        {
            db_reset();
            common_info.netbar_info.mode_active = 1;
            rgb_led_set_prompt(PROMPT_NETBAR_MODE_ENABLE);
        }
        db_flash_region_mark_update(DB_COMMON);
        db_save_now();
    }

    return cmd_do_response(CMD_SYS, SYS_SET_NETBAR_MODE, sizeof(uint32_t), 0, (uint8_t *)&e);
}

static uint32_t on_sys_get_slave_fw_info(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void)reserve;

    if (len != 4 )
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }
    dfu_slave_fw_info_t buff = {0};
    uint32_t id = *(uint32_t *)pdata;

    switch (id)
    {

    case DFU_OBJ_DPM:
    {
        memcpy(&buff, dfu_get_slave_fw_show(), sizeof(dfu_slave_fw_info_t));
        if (buff.fwv == 0)
        {
            uint32_t ret = uart_get_screen_ver();
            if (ret != 0)
            {
                // return HIVE_ERROR_GENERIC;
                buff.fwv = 0;
                buff.hwv = 0;
            }
            else
            {
                memcpy(&buff, dfu_get_slave_fw_show(), sizeof(dfu_slave_fw_info_t));
            }
        }
    }
    break;

    case DFU_OBJ_GM:
    {
        uint32_t ret = mg_uart_send_to_all_nation_request_cmd(SM_GET_VER_CMD, 0, NULL);
        if (ret != 0)
        {
            // return HIVE_ERROR_GENERIC;
            buff.fwv = 0;
            buff.hwv = 0;
        }
        else
        {
            mg_nation_hw_fw_ver_get(&buff.hwv, &buff.fwv);
        }
        buff.device_type = id;
    }
    break;

    default:
        return HIVE_ERROR_INVALID_PARAM;
    }


    return cmd_do_response(CMD_SYS, SYS_GET_SLAVE_INFO, sizeof(dfu_slave_fw_info_t), 0, (uint8_t*)&buff);
}

static uint32_t on_sys_get_startup_eff_en(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    uint8_t data = gbinfo.startup_effect_en;
    
    return cmd_do_response(CMD_SYS, SYS_GET_STARTUP_EFF_EN, sizeof(uint8_t), 0, &data);
}

static uint32_t on_sys_set_startup_eff_en(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    uint32_t e = 0;
    uint8_t data = pdata[0];
    
    gbinfo.startup_effect_en = data;
    db_update(DB_SYS, UPDATE_LATER);
    return cmd_do_response(CMD_SYS, SYS_SET_STARTUP_EFF_EN, sizeof(uint32_t), 0, (uint8_t *)&e);
}

static uint32_t on_sys_set_volt_up_en(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    uint32_t e = 1;
    uint8_t data = pdata[0];
    if(data <= 1)
    {
        sys_run.volt_up_en = data;
        if(sys_run.volt_up_en == 1)
        {
            mg_detecte_task_notify();
        }
        DBG_PRINTF("hive set volt_up_en == %d.\n",sys_run.volt_up_en);
        e = 0;
    }
    return cmd_do_response(CMD_SYS, SYS_SET_VOLT_UP_EN, sizeof(uint32_t), 0, (uint8_t *)&e);
}

static uint32_t on_kc_get_kcm_max(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    uint8_t buf[2] = {0};

    buf[0] = gbinfo.kc.max_kcm;
    buf[1] = gbinfo.kc.max_kc;

    return cmd_do_response(CMD_KC, KC_GET_SIZE, sizeof(buf), 0, buf);
}

static uint32_t on_kc_get_kcm_index(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    uint8_t buf[1] = {0};

    buf[0] = 0;

    return cmd_do_response(CMD_KC, KC_GET_CUR_KCM, sizeof(buf), 0, buf);
}

static uint32_t on_kc_set_kcm_index(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint32_t e = HIVE_SUCCESS;
    uint8_t nkcm = 0;

    if (len != 1)
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }

    nkcm = pdata[0];
    

    if (nkcm >= gbinfo.kc.max_kcm)
    {
        return HIVE_ERROR_DATA_SIZE;
    }

    return cmd_do_response(CMD_KC, KC_SET_CUR_KCM, 4, 0, (uint8_t *)&e);
}

static uint32_t on_kc_read_kc(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t buf[56] = {0};
    uint8_t kcmid = 0, kcid0 = 0, kcid1 = 0, nbyte = 0;

    if (len != 4)
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }

    kcmid = pdata[0];
    kcid0 = pdata[1];
    kcid1 = pdata[2];

    if (kcmid >= gbinfo.kc.max_kcm ||
        kcid0 >= gbinfo.kc.max_kc ||
        kcid1 >= gbinfo.kc.max_kc ||
        kcid0 > kcid1 ||
        (kcid1 - kcid0) >= 13)
    {
        DBG_PRINTF("on_kc_read_kc:err=%d,%d,%d,%d,%d\n",pdata[0],pdata[1],pdata[2],gbinfo.kc.max_kcm,gbinfo.kc.max_kc);
        return HIVE_ERROR_INVALID_PARAM;
    }

    buf[0] = kcmid;
    buf[1] = kcid0;
    buf[2] = kcid1;
    buf[3] = 0;
    nbyte = 4;

    for (; kcid0 <= kcid1; ++kcid0)
    {
        memcpy(&buf[nbyte], &sk_la_lm_kc_info.kc_table[kcmid][kcid0], 4);
        nbyte += 4;
    }

    return cmd_do_response(CMD_KC, KC_GET_KC, nbyte, 0, buf);
}

static uint32_t on_kc_write_kc(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint32_t e = HIVE_SUCCESS;
    uint8_t kcmid = 0, kcid0 = 0, kcid1 = 0, nbyte = 0;
    uint8_t buf[8] = {0};

    if (len < 8 || len > 56)
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }

    kcmid = pdata[0];
    kcid0 = pdata[1];
    kcid1 = pdata[2];
    nbyte = 4;

    if (kcmid >= gbinfo.kc.max_kcm ||
        kcid0 >= gbinfo.kc.max_kc ||
        kcid1 >= gbinfo.kc.max_kc ||
        kcid0 > kcid1 ||
        (kcid1 - kcid0) >= 13)
    {
        DBG_PRINTF( "kcmid=%03d kcid0=%d kcid1=%d", kcmid, kcid0, kcid1);
        DBG_PRINTF( "max_kcm=%03d max_kc=%d", gbinfo.kc.max_kcm, gbinfo.kc.max_kc);
        return HIVE_ERROR_INVALID_PARAM;
    }

    for (; kcid0 <= kcid1; ++kcid0)
    {
        kc_t *pkc = (kc_t *)&pdata[nbyte];

        if (pkc->ty >= KCT_MAX)
        {
            return HIVE_ERROR_INVALID_PARAM;
        }
        nbyte += 4;
    }

    kcmid = pdata[0];
    kcid0 = pdata[1];
    kcid1 = pdata[2];
    nbyte = 4;

    for (; kcid0 <= kcid1; ++kcid0)
    {
        kc_t *pkc = (kc_t *)&pdata[nbyte];

        if (pkc->ty == KCT_FN && pkc->sty == 0 && pkc->co >= KC_FN0 && pkc->co <= KC_FN3)
        {
            for (int l = 0; l < BOARD_KCM_MAX; ++l)
            {
                memcpy(&sk_la_lm_kc_info.kc_table[l][kcid0], pkc, sizeof(kc_t));
            }
        }
        else
        {
            // uint8_t hidCode          = kc_table[kcmid][kcid0].co;

            uint8_t tid = pkc->co; // table id
            sk_la_lm_kc_info_t *pdb = &sk_la_lm_kc_info;
            // DBG_PRINTF("kcmid: %02d, kcid0:%02d, ty:%02d, sty:%02d, co:%02d, ex:%d\n", kcmid, kcid0, pkc->ty, pkc->sty, pkc->co, pdb->sk.adv_table[tid].adv_ex);

            if (pkc->ty == KCT_SK && pkc->sty == SK_DKS)
            {
                if (feature_dks_bind(kcmid, kcid0, tid) == false)
                {
                    e = HIVE_ERROR_INVALID_PARAM;
                }
            }
            else if (pkc->ty == KCT_SK && pkc->sty == SK_ADV && pdb->sk.adv_table[tid].adv_ex == ADV_MTP)
            {
                for (uint8_t i = 0; i < ADV_MTP_POINT_NUM; i++)
                {
                    uint16_t trg_dist = pdb->sk.adv_table[tid].data.mtp.trg_dist[i];
                    float distTrigger = HIVE_RXDA_001MM(trg_dist); /* get trigger stroke */
                    
                    Mg_SEGGER_RTT_printf("mtp","ki: %d, trg_dist: %d\r\n", kcid0, trg_dist);

                    uint16_t rls_lvl, trg_lvl = DIST_TO_LEVEL(distTrigger, KEY_GENERAL_TRG_ACC(kcid0));
                    if (KEY_SUPT_HACC(kcid0) && (distTrigger > KEY_NACC_TO_DIST(kcid0)))
                    {
                        trg_lvl = DIST_TO_LEVEL(distTrigger - KEY_NACC_TO_DIST(kcid0), KEY_HACC(kcid0)) + KEY_NACC_TO_LVL(kcid0);
                    }
                    if (trg_lvl > KEY_MAX_LEVEL(kcid0))
                    {
                        trg_lvl = KEY_MAX_LEVEL(kcid0);
                    }                    
                    trg_lvl -= 1;
                    kh.p_st[kcid0].advTrg[kcmid].mtp.point[i] = KEY_LVL_GET_VAL(kcid0, trg_lvl);
                    
                    rls_lvl = trg_lvl - 1;
                    kh.p_st[kcid0].advTrg[kcmid].mtp.point[ADV_MTP_POINT_NUM+i] = (trg_lvl == 0) ? kh.p_st[kcid0].topRaisePoint : KEY_LVL_GET_VAL(kcid0, rls_lvl);
                }
            }
            // else if (pkc->ty == KCT_SK && pkc->sty == SK_ADV && pdb->sk.adv_table[tid].adv_ex == ADV_MT)
            // {
            //     if (mt_bind(kcmid, kcid0, tid) == false)
            //     {
            //         e = HIVE_ERROR_INVALID_PARAM;
            //     }
            // }
            // else if (pkc->ty == KCT_SK && pkc->sty == SK_ADV && pdb->sk.adv_table[tid].adv_ex == ADV_TGL)
            // {
            //     if (tgl_bind(kcmid, kcid0, tid) == false)
            //     {
            //         e = HIVE_ERROR_INVALID_PARAM;
            //     }
            // }
            else
            {
                // if (pkc->ty == KCT_KB)
                // {
                //     keyCodeTokeySeq[hidCode] = 0xFF; //clear ki
                //     hidCode                  = kc_table[kcmid][kcid0].co;
                //     keyCodeTokeySeq[hidCode] = kcid0; //set ki
                // }

                if (kh.p_attr[kcid0].advTrg[kcmid].advType == KEY_ACT_DKS)
                {
                    feature_dks_unbind(kcmid, kcid0);
                }
                // else if (kh.p_attr[kcid0].advTrg[kcmid].advType == KEY_ACT_MT)
                // {
                //     mt_unbind(kcmid, kcid0);
                // }
                // else if (kh.p_attr[kcid0].advTrg[kcmid].advType == KEY_ACT_TGL)
                // {
                //     tgl_unbind(kcmid, kcid0);
                // }
            }
            if (e == HIVE_SUCCESS)
            {
                memcpy(&pdb->kc_table[kcmid][kcid0], pkc, sizeof(kc_t));
            }
        }
        nbyte += 4;
    }
    
    if (e == HIVE_SUCCESS)
    {
        db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
    }


    *(uint32_t *)&buf[0] = e;
    buf[4] = pdata[0];
    buf[5] = pdata[1];
    buf[6] = pdata[2];
    buf[7] = 0;

    return cmd_do_response(CMD_KC, KC_SET_KC, 8, 0, buf);
}

static uint32_t on_kc_set_default(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    uint32_t e = HIVE_SUCCESS;
    uint8_t key_layer = pdata[0];
    uint8_t key_index = pdata[1];

    if (key_layer >= BOARD_USED_KCM_MAX)
    {
        e = HIVE_ERROR_INVALID_PARAM;
    }
    else
    {
        if (key_index < BOARD_KEY_NUM)
        {
            if (sk_la_lm_kc_info.kc_table[key_layer][key_index].ty  == KCT_SK && 
                sk_la_lm_kc_info.kc_table[key_layer][key_index].sty == SK_DKS)
            {
                feature_dks_unbind(key_layer, key_index);
            }
            sk_la_lm_kc_info.kc_table[key_layer][key_index] = kc_table_default[key_layer][key_index];
        }
        else if (0xff == key_index)
        {
            for (int idx = 0; idx < BOARD_KEY_NUM; idx++)
            {
                if (sk_la_lm_kc_info.kc_table[key_layer][idx].ty  == KCT_SK && 
                    sk_la_lm_kc_info.kc_table[key_layer][idx].sty == SK_DKS)
                {
                    feature_dks_unbind(key_layer, idx);
                }
            }
            memcpy(sk_la_lm_kc_info.kc_table[key_layer], kc_table_default[key_layer], sizeof(kc_table_default[key_layer]));
        }

        db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
    }

    return cmd_do_response(CMD_KC, KC_SET_DEFAULT, 4, 0, (uint8_t *)&e);
}


#ifdef RGB_ENABLED

static uint32_t on_kr_get_krm_max(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    uint8_t buf[2] = {0};

    buf[0] = BOARD_LED_COLOR_LAYER_MAX;
    buf[1] = BOARD_KEY_LED_NUM - BOARD_SPACE_LED_NUM + 1;

    return cmd_do_response(CMD_KR, KR_GET_SIZE, sizeof(buf), 0, buf);
}

static uint32_t on_kr_get_krm_index(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    uint8_t buf[1] = {0};

    buf[0] = gbinfo.kr.cur_krm;

    return cmd_do_response(CMD_KR, KR_GET_CUR_KRM, sizeof(buf), 0, buf);
}

static uint32_t on_kr_set_krm_index(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint32_t e = HIVE_SUCCESS;
    uint8_t nkrm = 0;

    if (len != 1)
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }

    nkrm = pdata[0];

    if (nkrm >= gbinfo.kr.max_krm)
    {
        return HIVE_ERROR_DATA_SIZE;
    }

    gbinfo.kr.cur_krm = nkrm;


    for (int i = 0; i < BOARD_KEY_LED_NUM; ++i)
    {
        set_key_led_color(i, gmx.pkts[i].ltl, gmx.pkr[nkrm][i].r, gmx.pkr[nkrm][i].g, gmx.pkr[nkrm][i].b);
    }

    db_update(DB_SYS, UPDATE_LATER);


    return cmd_do_response(CMD_KR, KR_SET_CUR_KRM, 4, 0, (uint8_t *)&e);
}

static uint32_t on_kr_read_kr(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t buf[56] = {0};
    uint8_t krmid = 0, krid0 = 0, krid1 = 0, nbyte = 0;

    if (len != 4)
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }

    krmid = pdata[0];
    krid0 = pdata[1];
    krid1 = pdata[2];

    if (krmid >= gbinfo.kr.max_krm ||
        krid0 >= gbinfo.kr.max_kr ||
        krid1 >= gbinfo.kr.max_kr ||
        krid0 > krid1 ||
        (krid1 - krid0) >= 13)
    {
        return HIVE_ERROR_INVALID_PARAM;
    }

    buf[0] = krmid;
    buf[1] = krid0;
    buf[2] = krid1;
    buf[3] = 0;
    nbyte = 4;

    uint8_t current = krid0;

    for (; current <= krid1; ++current)
    {
        #if defined(BOARD_SPACE_LED_NUM) && (BOARD_SPACE_LED_NUM > 0)

        uint8_t index;
        if (current < BOARD_SPACE_KEY_INDEX)
        {
            index = current;
        }
        else if (current == BOARD_SPACE_KEY_INDEX)
        {
            index = BOARD_SPACE_KEY_INDEX;
        }
        else
        {
            index = current + (BOARD_SPACE_LED_NUM - 1);
        }

        if (index >= gbinfo.kr.max_kr)
        {
            return HIVE_ERROR_INVALID_PARAM;
        }
        memcpy(&buf[nbyte], &gmx.pkr[krmid][index], 4);

        #else
        memcpy(&buf[nbyte], &gmx.pkr[krmid][current], 4);
        #endif

        nbyte += 4;
    }

    return cmd_do_response(CMD_KR, KR_GET_KR, nbyte, 0, buf);
}

static uint32_t on_kr_write_kr(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t krmid = 0, krid0 = 0, krid1 = 0, nbyte = 0;
    uint8_t buf[8] = {0};

    if (len < 8 || len > 56)
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }

    krmid = pdata[0];
    krid0 = pdata[1];
    krid1 = pdata[2];
    nbyte = 4;

    DBG_PRINTF( "cur_krm=%d, krmid=%d, krid0=%d krid1=%d", gbinfo.kr.cur_krm, krmid, krid0, krid1);

    if (krmid >= gbinfo.kr.max_krm ||
        krid0 >= gbinfo.kr.max_kr ||
        krid1 >= gbinfo.kr.max_kr ||
        krid0 > krid1 ||
        (krid1 - krid0) >= 13)
    {
        return HIVE_ERROR_INVALID_PARAM;
    }

    uint8_t tmp = 0;
    for (; krid0 <= krid1; ++krid0)
    {
        tmp = krid0;
        if (tmp == BOARD_SPACE_KEY_INDEX)
        {
            for (; tmp < (krid0 + BOARD_SPACE_LED_NUM); ++tmp)
            {
                memcpy(get_key_led_color_ptr(krmid , tmp), &pdata[nbyte], 4);
                set_key_led_color(tmp, gmx.pkts[tmp].ltl, gmx.pkr[krmid][tmp].r, gmx.pkr[krmid][tmp].g, gmx.pkr[krmid][tmp].b);
            }
            nbyte += 4;
            continue;
        }
        else if (tmp > BOARD_SPACE_KEY_INDEX)
        {
            tmp += 2;
        }
        memcpy(get_key_led_color_ptr(krmid , tmp), &pdata[nbyte], 4);
        set_key_led_color(tmp, gmx.pkts[tmp].ltl, gmx.pkr[krmid][tmp].r, gmx.pkr[krmid][tmp].g, gmx.pkr[krmid][tmp].b);

        nbyte += 4;
    }

    db_update(DB_KR, UPDATE_LATER);

    rgb_led_set_all();

    *(uint32_t *)&buf[0] = HIVE_SUCCESS;
    buf[4] = pdata[0];
    buf[5] = pdata[1];
    buf[6] = pdata[2];
    buf[7] = 0;

    return cmd_do_response(CMD_KR, KR_SET_KR, 8, 0, buf);
}

static uint32_t on_kr_set_default(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    (void) pdata;
    uint32_t e = HIVE_SUCCESS;

    gbinfo.kr.cur_krm = 0;
    gbinfo.kr.max_krm = BOARD_LED_COLOR_LAYER_MAX;
    gbinfo.kr.max_kr  = BOARD_KEY_LED_NUM;

    gbinfo.lm.cur_lmm  = LED_ACTION_GRADUAL;
    gbinfo.lm.max_lmm  = LED_ACTION_MAX;
    gbinfo.lm.max_lm   = BOARD_KEY_LED_NUM;
    gbinfo.lm.last_lmm = LED_ACTION_MAX;
    
    db_reset_kr();
    db_reset_la();
    rgb_led_set_all();

    db_update(DB_SYS, UPDATE_LATER);
    db_update(DB_KR, UPDATE_LATER);
    db_update(DB_SK_LA_LM_KC, UPDATE_LATER);

    return cmd_do_response(CMD_KR, KR_SET_DEFAULT, 4, 0, (uint8_t *)&e);
}

#endif

static uint32_t on_sk_read_header(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) reserve;
    uint8_t buf[48] = {0};
    uint8_t size = 0;
    
    if (pdata[0] < SKT_MAX)
    {
        size = sizeof(skh_t);
        memcpy(buf, &sk_la_lm_kc_info.sk.superkey_table[pdata[0]], size);
    }
    else if (pdata[0] == 0xFF)
    {
        size = sizeof(skh_t) * 4; //读取DKS、ADV、MACRO、ST
        memcpy(buf, sk_la_lm_kc_info.sk.superkey_table, size);
    }

    return cmd_do_response(CMD_SK, SK_GET_HEADER, size, 0, buf);
}

static uint32_t on_sk_read_dks(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56]    = {0};
    uint16_t respBufferLenght = 0;
    
    ks_cmd_get_dks_para_table(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_SK, SK_GET_DKS, respBufferLenght, 0, respBuffer);
}

static uint32_t on_sk_write_dks(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56]    = {0};
    uint16_t respBufferLenght = 0;
    uint8_t opt_idx  = pdata[0];
    uint8_t opt_type = pdata[1];
     
    if (0 == opt_type)//删除
    {
        for (uint8_t layer = 0; layer < BOARD_USED_KCM_MAX; layer++)
        {
            for (uint8_t index = 0; index < BOARD_KEY_NUM; index++)
            {
                if ((KCT_SK  == sk_la_lm_kc_info.kc_table[layer][index].ty)  && 
                    (SK_DKS  == sk_la_lm_kc_info.kc_table[layer][index].sty) &&
                    (opt_idx == sk_la_lm_kc_info.kc_table[layer][index].co)) 
                {
                    sk_la_lm_kc_info.kc_table[layer][index] = kc_table_default[layer][index];
                    db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
                }
            }
        }
    }

    ks_cmd_set_dks_para_table(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);
    
    return cmd_do_response(CMD_SK, SK_SET_DKS, respBufferLenght, 0, respBuffer);
}

static uint32_t on_sk_read_dks_tick(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56]    = {0};
    uint16_t respBufferLenght = 0;
    
    ks_cmd_get_dks_tick(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_SK, SK_GET_DKS_TICK, respBufferLenght, 0, respBuffer);
}

static uint32_t on_sk_write_dks_tick(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56]    = {0};
    uint16_t respBufferLenght = 0;
    
    ks_cmd_set_dks_tick(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);
    
    return cmd_do_response(CMD_SK, SK_SET_DKS_TICK, respBufferLenght, 0, respBuffer);
}

static uint32_t on_sk_set_socd(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    if ((pdata[0] >= SK_SOCD_NUM) || (pdata[2] > KEYBOARD_NUMBER) || (pdata[3] > KEYBOARD_NUMBER))
    {
        return HIVE_ERROR_INVALID_PARAM;
    }
    else
    {

        ks_cmd_set_socd(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);
    }

    return cmd_do_response(CMD_SK, SK_SET_SOCD, respBufferLenght, 0, respBuffer);
}

static uint32_t on_sk_get_socd(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;

    ks_cmd_get_socd(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_SK, SK_GET_SNAP_TAP, respBufferLenght, 0, respBuffer);
}
static uint32_t on_sk_set_ht(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    
    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    if ((pdata[0] >= SK_HT_NUM) || (pdata[2] > KEYBOARD_NUMBER) || (pdata[3] > KEYBOARD_NUMBER))
    {
        return HIVE_ERROR_INVALID_PARAM;
    }
    else
    {
        ks_cmd_set_ht(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);
    }

    return cmd_do_response(CMD_SK, SK_SET_HT, respBufferLenght, 0, respBuffer);
}

static uint32_t on_sk_get_ht(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;

    ks_cmd_get_ht(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_SK, SK_GET_HT, respBufferLenght, 0, respBuffer);
}
static uint32_t on_sk_set_rs(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    
    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    if ((pdata[0] >= SK_RS_NUM) || (pdata[2] > KEYBOARD_NUMBER) || (pdata[3] > KEYBOARD_NUMBER))
    {
        return HIVE_ERROR_INVALID_PARAM;
    }
    else
    {
        ks_cmd_set_rs(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);
    }

    return cmd_do_response(CMD_SK, SK_SET_RS, respBufferLenght, 0, respBuffer);
}

static uint32_t on_sk_get_rs(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;

    ks_cmd_get_rs(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_SK, SK_GET_RS, respBufferLenght, 0, respBuffer);
}

static uint32_t on_sk_read_adv(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    static uint8_t adv_count = 0, rest_adv = 0;
    uint8_t buf[56] = {0};
    uint8_t id = 0;

    if (pdata[0] == 0)
    {
        id = pdata[1] & 0x7F;

        if (id >= SK_ADV_MAX)
        {
            buf[0] = 0xff;
        }
        else if (sk_la_lm_kc_info.sk.superkey_table[SKT_ADV].storage_mask & (1 << id))
        {
            buf[0] = 0xff;
        }
        else
        {
            buf[0] = 0;
            buf[1] = 1;
            buf[2] = id;
            memcpy(&buf[3], &sk_la_lm_kc_info.sk.adv_table[id], sizeof(adv_t));
        }
    }
    else if (pdata[0] == 1)
    {
        if ((pdata[1] & 0x80) == 0)
        {
            rest_adv = sk_la_lm_kc_info.sk.superkey_table[SKT_ADV].total_size - sk_la_lm_kc_info.sk.superkey_table[SKT_ADV].free_size;
            if (rest_adv == 0)
            {
                buf[0] = 0;
                buf[1] = 0;
                return cmd_do_response(CMD_SK, SK_GET_ADV, sizeof(buf), 0, buf);
            }
            adv_count = 0;
        }
        
        while ((sk_la_lm_kc_info.sk.superkey_table[SKT_ADV].storage_mask & (1 << adv_count)) == 0)
        {
            adv_count++;
        }

        buf[0] = --rest_adv;
        buf[1] = 1;
        buf[2] = adv_count;

        memcpy(&buf[3], &sk_la_lm_kc_info.sk.adv_table[adv_count], sizeof(adv_t));
        adv_count++;
        if (adv_count > 15)
        {
            rest_adv = sk_la_lm_kc_info.sk.superkey_table[SKT_ADV].total_size - sk_la_lm_kc_info.sk.superkey_table[SKT_ADV].free_size;
            adv_count = 0;
        }
    }
    else
    {
        buf[0] = 0xff;
    }
    DBG_PRINTF("read adv %d,%d,%d\n", buf[0], buf[1], buf[2]);
    // DBG_PRINTF("read adv(0x%02x): ", ARRAY_SIZE(buf));
    // for (uint16_t i = 0; i < ARRAY_SIZE(buf); i++)
    // {
    //     DBG_PRINTF("%02x ", buf[i]);
    // }
    //     DBG_PRINTF("\n");
    return cmd_do_response(CMD_SK, SK_GET_ADV, sizeof(buf), 0, buf);
}

static uint32_t on_sk_write_adv(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    uint32_t e = HIVE_SUCCESS;
    uint8_t id = 0;
    sk_la_lm_kc_info_t *pdb = &sk_la_lm_kc_info;

    id = pdata[0];

    if (id >= SK_ADV_MAX)
    {
        e = HIVE_ERROR_DATA_SIZE;
        return cmd_do_response(CMD_SK, SK_SET_ADV, 4, 0, (uint8_t *)&e);
    }

    adv_t tmpAdv;
    memcpy(&tmpAdv, &pdata[2], sizeof(adv_t));

    if (pdata[1] == ADV_OPT_DEL)
    {
        if (pdb->sk.superkey_table[SKT_ADV].storage_mask & (1<<id))
        {
            if (pdb->sk.adv_table[id].adv_ex == ADV_LOCK)
            {
                adv_enable_key(&pdb->sk.adv_table[id]);
            }
            
            pdb->sk.superkey_table[SKT_ADV].storage_mask &= ~(1<<id);  //clear bit
            pdb->sk.superkey_table[SKT_ADV].free_size++;
        }

        for (uint8_t layer = 0; layer < BOARD_USED_KCM_MAX; layer++)
        {
            for (uint8_t index = 0; index < BOARD_KEY_NUM; index++)
            {
                if ((KCT_SK  == pdb->kc_table[layer][index].ty)  && 
                    (SK_ADV  == pdb->kc_table[layer][index].sty) &&
                    (id      == pdb->kc_table[layer][index].co)) 
                {
                    pdb->kc_table[layer][index] = kc_table_default[layer][index];
                    db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
                }
            }
        }

    }
    else if (pdata[1] == ADV_OPT_NEW)
    {
        if ((pdb->sk.superkey_table[SKT_ADV].storage_mask & (1<<id)) == 0)
        {
            pdb->sk.superkey_table[SKT_ADV].storage_mask |= (1<<id);   //set bit
            pdb->sk.superkey_table[SKT_ADV].free_size--;

        }
        
        if (tmpAdv.adv_ex == ADV_SCENE)
        {
            int8_t isOk = adv_scene_is_pass(&tmpAdv);
            if (isOk < 0)
            {
                DBG_PRINTF("ADV_SCENE: ID: %d, isOk:%d\n", id, isOk);
                e = HIVE_ERROR_GENERIC;
                return cmd_do_response(CMD_SK, SK_SET_ADV, 4, 0, (uint8_t *)&e);
            }
        }
        else if (tmpAdv.adv_ex == ADV_MT)
        {
            #if defined(MODULE_LOG_ENABLED) && (MODULE_LOG_ENABLED == 1)
            uint16_t mtHd     = tmpAdv.data.mt.holdDuration;
            uint16_t mtHoldCo = tmpAdv.data.mt.kc[0].co; //hold
            uint16_t mtTapCo  = tmpAdv.data.mt.kc[1].co; //tap
            DBG_PRINTF("ADV NEW MT: ID: %d, hd:%d, hco:0x%x, tco:0x%x \n", id, mtHd, mtHoldCo, mtTapCo);
            #endif
        }
        else if (tmpAdv.adv_ex == ADV_TGL)
        {
            #if defined(MODULE_LOG_ENABLED) && (MODULE_LOG_ENABLED == 1)
            uint16_t tglHd = tmpAdv.data.tgl.holdDuration;
            uint16_t tglCo = tmpAdv.data.tgl.kc.co; //toggle
            DBG_PRINTF("ADV NEW TGL: ID: %d, hd:%d, tco:0x%x \n", id, tglHd, tglCo);
            #endif
        }
        else if (tmpAdv.adv_ex == ADV_MTP)
        {
            uint8_t kc_cnt  = 0;
            for (uint8_t i = 0; i < ADV_MTP_POINT_NUM; i++)
            {
                if (tmpAdv.data.mtp.trg_dist[i] == 0)
                {
                    DBG_PRINTF("cnt:%d, dist:%d\r\n", kc_cnt, tmpAdv.data.mtp.trg_dist[i]);
                    kc_cnt = 0;
                    e = HIVE_ERROR_GENERIC;
                    break;
                }
                if ((tmpAdv.data.mtp.kc[i].ty != KCT_NONE) && (tmpAdv.data.mtp.kc[i].ty < KCT_MAX))
                {
                    kc_cnt++;
                }
            }

            if (kc_cnt > 1)
            {
                for (uint8_t i = 1; i < kc_cnt; i++)
                {
                    if ((tmpAdv.data.mtp.trg_dist[i - 1] >= tmpAdv.data.mtp.trg_dist[i]))
                    {
                        DBG_PRINTF("cnt:%d, pre_dist:%d, dist:%d\r\n", kc_cnt, tmpAdv.data.mtp.trg_dist[i - 1], tmpAdv.data.mtp.trg_dist[i]);
                        e = HIVE_ERROR_GENERIC;
                        break;
                    }
                }
            }
        }
    }
    else if (pdata[1] == ADV_OPT_MDF)
    {
        if (pdb->sk.superkey_table[SKT_ADV].storage_mask & (1<<id))
        {
            if (pdb->sk.adv_table[id].adv_ex == ADV_LOCK)
            {
                adv_enable_key(&pdb->sk.adv_table[id]);
            }
            else if (tmpAdv.adv_ex == ADV_SCENE)
            {
                int8_t isOk = adv_scene_is_pass(&tmpAdv); 
                if (isOk < 0)
                {
                    DBG_PRINTF("ADV_SCENE: ID: %d, isOk:%d\n", id, isOk);
                    e = 1;
                    return cmd_do_response(CMD_SK, SK_SET_ADV, 4, 0, (uint8_t *)&e);
                }
            }
            else if (tmpAdv.adv_ex == ADV_MT)
            {
                #if defined(MODULE_LOG_ENABLED) && (MODULE_LOG_ENABLED == 1)
                uint16_t mtHd     = tmpAdv.data.mt.holdDuration;
                uint16_t mtHoldCo = tmpAdv.data.mt.kc[0].co; //hold
                uint16_t mtTapCo  = tmpAdv.data.mt.kc[1].co; //tap
                DBG_PRINTF("ADV MOD MT: ID: %d, hd:%d, hco:0x%x, tco:0x%x \n", id, mtHd, mtHoldCo, mtTapCo);
                #endif
            }
            else if (tmpAdv.adv_ex == ADV_TGL)
            {
                #if defined(MODULE_LOG_ENABLED) && (MODULE_LOG_ENABLED == 1)
                uint16_t tglHd = tmpAdv.data.tgl.holdDuration;
                uint16_t tglCo = tmpAdv.data.tgl.kc.co; //toggle
                DBG_PRINTF("ADV MOD TGL: ID: %d, hd:%d, tco:0x%x \n", id, tglHd, tglCo);
                #endif
            }
            else if (tmpAdv.adv_ex == ADV_MTP)
            {
                uint8_t kc_cnt  = 0;
                for (uint8_t i = 0; i < ADV_MTP_POINT_NUM; i++)
                {
                    if (tmpAdv.data.mtp.trg_dist[i] == 0)
                    {
                        DBG_PRINTF("cnt:%d, dist:%d\r\n", kc_cnt, tmpAdv.data.mtp.trg_dist[i]);
                        kc_cnt = 0;
                        e = HIVE_ERROR_GENERIC;
                        break;
                    }
                    if ((tmpAdv.data.mtp.kc[i].ty != KCT_NONE) && (tmpAdv.data.mtp.kc[i].ty < KCT_MAX))
                    {
                        kc_cnt++;
                    }
                }
    
                if (kc_cnt > 1)
                {
                    for (uint8_t i = 1; i < kc_cnt; i++)
                    {
                        if ((tmpAdv.data.mtp.trg_dist[i - 1] >= tmpAdv.data.mtp.trg_dist[i]))
                        {
                            DBG_PRINTF("cnt:%d, pre_dist:%d, dist:%d\r\n", kc_cnt, tmpAdv.data.mtp.trg_dist[i - 1], tmpAdv.data.mtp.trg_dist[i]);
                            e = HIVE_ERROR_GENERIC;
                            break;
                        }
                    }
                }
                adv_mtp_update_point(tmpAdv, id);
            }
        }
    }

    if (e == HIVE_SUCCESS)
    {
        memcpy(&pdb->sk.adv_table[id], &pdata[2], sizeof(adv_t));
        DBG_PRINTF("write adv %d,%d,%d\n", pdb->sk.adv_table[id].adv_co, pdb->sk.adv_table[id].adv_ex);
        db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
    }
    
    return cmd_do_response(CMD_SK, SK_SET_ADV, 4, 0, (uint8_t *)&e);
}

static uint32_t on_sk_read_macro(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    static uint8_t macro_id = 0xff;
    static uint8_t total_pkt = 0, pkt_id = 0;
    uint8_t last_pkt_size = 0;
    uint8_t buf[56] = {0};
    
    if (pdata[0] == M_R_NEW)
    {
        macro_id = pdata[1];
        if (macro_id >= SK_MACRO_MAX)
        {
            macro_id = 0xff;
            buf[0] = M_R_ERR_END;
        }
        else
        {
            if ((sk_la_lm_kc_info.sk.superkey_table[SKT_MACRO].storage_mask & (1<<macro_id)) == 0)
            {
                macro_id = 0xff;
                buf[0] = M_R_ERR_END;
            }
            else
            {
                pkt_id = 0;

                buf[0] = M_R_NEW;
                buf[1] = macro_id;
                buf[2] = macro_info.macro_table[macro_id].mh.total_frames;
                memcpy(&buf[3], (uint8_t *)&macro_info.macro_table[macro_id], 53);

                total_pkt = (sizeof(mh_t) + sizeof(mframe_t) * macro_info.macro_table[macro_id].mh.total_frames + 1) / 54;
                last_pkt_size = (sizeof(mh_t) + sizeof(mframe_t) * macro_info.macro_table[macro_id].mh.total_frames + 1) % 54;
                if (last_pkt_size)
                {
                    if (total_pkt == 0)
                    {
                        macro_id = 0xff;
                    }
                    total_pkt += 1;
                }
                DBG_PRINTF("total_pkt  =  %d\n", total_pkt);
            }
        }
    }
    else if (pdata[0] == M_R_TRX)
    {
        if (macro_id == 0xff)
        {
            buf[0] = M_R_ERR_END;
        }
        else
        {
            pkt_id++;
            DBG_PRINTF("MACRO M_R_TRX pkt_id = %d\n", pkt_id);

            buf[0] = M_R_TRX;
            buf[1] = pkt_id;
            memcpy(&buf[2], (uint8_t *)&macro_info.macro_table[macro_id]+(pkt_id * 54 - 1), 54);

            if (pkt_id >= (total_pkt-1))
            {
                macro_id = 0xff;
            }
        }
    }

    return cmd_do_response(CMD_SK, SK_GET_MACRO, sizeof(buf), 0, (uint8_t *)buf);
}

static uint32_t on_sk_write_macro(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    static uint8_t macro_id = 0xff;
    static uint8_t total_pkt = 0, pkt_id = 0;
    uint8_t last_pkt_size = 0;
    uint8_t buf[2] = {0};

    if (pdata[0] == M_W_DEL) //del
    {
        if (macro_id != 0xff)
        {
            buf[0] = M_CMD_ERROR;
        }
        else
        {
            macro_id = pdata[1];
            if (macro_id >= SK_MACRO_MAX)
            {
                buf[0] = M_NUMBER_ERROR;
            }
            else
            {
                if (sk_la_lm_kc_info.sk.superkey_table[SKT_MACRO].storage_mask & (1<<macro_id))
                {
                    sk_la_lm_kc_info.sk.superkey_table[SKT_MACRO].storage_mask &= ~(1<<macro_id);  //clear bit
                    sk_la_lm_kc_info.sk.superkey_table[SKT_MACRO].free_size++;
                    memset((uint8_t *)&macro_info.macro_table[macro_id], 0, sizeof(macro_t));
                    db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
                    DBG_PRINTF("MACRO DEL SUCCESS!\n");
                }
                	
                for (uint8_t layer = 0; layer < BOARD_USED_KCM_MAX; layer++)
                {
                    for (uint8_t index = 0; index < BOARD_KEY_NUM; index++)
                    {
                        if ((KCT_SK    == sk_la_lm_kc_info.kc_table[layer][index].ty)  && 
                            (SK_MACRO  == sk_la_lm_kc_info.kc_table[layer][index].sty) &&
                            (macro_id  == sk_la_lm_kc_info.kc_table[layer][index].co)) 
                        {
                            sk_la_lm_kc_info.kc_table[layer][index] = kc_table_default[layer][index];
                            db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
                        }
                    }
                }
            }
        }
        macro_id = 0xff;
    }
    else if ((pdata[0] == M_W_NEW) || (pdata[0] == M_W_MDF)) //new
    {
        if (macro_id != 0xff)
        {
            macro_id = 0xff;
            buf[0] = M_CMD_ERROR;
        }
        else
        {
            macro_id = pdata[1];
            if (macro_id >= SK_MACRO_MAX)
            {
                macro_id = 0xff;
                buf[0] = M_NUMBER_ERROR;
            }
            else if ((pdata[2] == 0) || pdata[2] >= 81)
            {
                macro_id = 0xff;
                buf[0] = M_TOTAL_FRAMES_ERROR;
            }
            else
            {
                if ((sk_la_lm_kc_info.sk.superkey_table[SKT_MACRO].storage_mask & (1<<macro_id)) == 0)
                {
                    sk_la_lm_kc_info.sk.superkey_table[SKT_MACRO].storage_mask |= (1<<macro_id);   //set bit
                    sk_la_lm_kc_info.sk.superkey_table[SKT_MACRO].free_size--;
                    db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
                }
                else
                {
                    memset((uint8_t *)&macro_info.macro_table[macro_id], 0, sizeof(macro_t));
                }
 
                memcpy((uint8_t *)&macro_info.macro_table[macro_id], &pdata[3], 53);

                pkt_id = 0;

                buf[0] = M_TRANS_SUCCESS;
                buf[1] = pkt_id;

                total_pkt = (sizeof(mh_t) + sizeof(mframe_t) * pdata[2] + 1) / 54;
                last_pkt_size = (sizeof(mh_t) + sizeof(mframe_t) * pdata[2] + 1) % 54;
                if (last_pkt_size)
                {
                    if (total_pkt == 0)
                    {
                        db_update(DB_MACRO, UPDATE_LATER);
                        macro_id = 0xff;
                    }
                    total_pkt += 1;
                }
                DBG_PRINTF("mframe_t  =  %d\n", sizeof(mframe_t));
                DBG_PRINTF("mh_t  =  %d\n", sizeof(mh_t));
                DBG_PRINTF("total_pkt  =  %d\n", total_pkt);
            }
        }
    }
    else if (pdata[0] == M_W_TRX)
    {
        if (macro_id == 0xff)
        {
            buf[0] = M_CMD_ERROR;
        }
        else
        {
            pkt_id++;
            DBG_PRINTF("MACRO M_W_TRX pkt_id = %d\n", pkt_id);
            if (pkt_id != pdata[1])
            {
                macro_id = 0xff;
                buf[0] = M_PID_ERROR;
            }
            else
            {
                memcpy((uint8_t *)&macro_info.macro_table[macro_id]+(pkt_id * 54 - 1), &pdata[2], 54);

                if (pdata[1] >= (total_pkt-1))
                {
                    macro_id = 0xff;
                    db_update(DB_MACRO, UPDATE_LATER);
                }

                buf[0] = M_TRANS_SUCCESS;
                buf[1] = pkt_id;
            }
        }
    }
    else
    {
        buf[0] = M_CMD_ERROR;
    }

    return cmd_do_response(CMD_SK, SK_SET_MACRO, sizeof(buf), 0, (uint8_t *)buf);
}

#ifdef RGB_ENABLED
static uint32_t on_lm_get_lm_info(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    (void) pdata;
    uint8_t buf[2] = {0};

    buf[0] = gbinfo.lm.max_lmm;
    buf[1] = gbinfo.lm.max_lm;

    return cmd_do_response(CMD_LA, KM_GET_SIZE, sizeof(buf), 0, buf);
}

static uint32_t on_lm_get_cur_lm(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    (void) pdata;
    uint8_t buf[1] = {0};

    buf[0] = gbinfo.lm.cur_lmm;

    return cmd_do_response(CMD_LA, KM_GET_KMM, sizeof(buf), 0, buf);
}

static uint32_t on_lm_set_cur_lm(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint32_t e = HIVE_SUCCESS;
    uint8_t nlm = 0;

    if (len != 1)
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }

    nlm = pdata[0];

    if (nlm >= gbinfo.lm.max_lmm)
    {
        return HIVE_ERROR_DATA_SIZE;
    }

    {
        if (!nlm)
        {
            gbinfo.lm.last_lmm = gbinfo.lm.cur_lmm;
            gbinfo.lm.cur_lmm = LED_ACTION_NONE;
        }
        else
        {
            gbinfo.lm.cur_lmm = gbinfo.lm.last_lmm;
            gbinfo.lm.last_lmm = LED_ACTION_MAX;
        }
    }


    gbinfo.lm.cur_lmm = nlm;
    db_update(DB_SYS, UPDATE_LATER);

    rgb_led_set_all();

    return cmd_do_response(CMD_LA, KM_SET_KMM, 4, 0, (uint8_t *)&e);
}

static uint32_t on_lm_toggle_lm(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    uint32_t e = HIVE_SUCCESS;
    uint8_t status = *pdata;

    if (!status)
    {
        if (gbinfo.lm.cur_lmm != LED_ACTION_NONE)
        {
            gbinfo.lm.last_lmm = gbinfo.lm.cur_lmm;
            gbinfo.lm.cur_lmm  = LED_ACTION_NONE;
        }
    }
    else
    {
        if (gbinfo.lm.cur_lmm == LED_ACTION_NONE)
        {
            gbinfo.lm.cur_lmm  = gbinfo.lm.last_lmm;
            gbinfo.lm.last_lmm = LED_ACTION_MAX;
        }
    }

    DBG_PRINTF("on_lm_toggle_lm=%d,%d\n", gbinfo.lm.cur_lmm, gbinfo.lm.last_lmm);
    db_update(DB_SYS, UPDATE_LATER);
    rgb_led_set_all();

    return cmd_do_response(CMD_LA, KM_TOG_KMM, 4, 0, (uint8_t *)&e);
}

static uint32_t on_la_read_lm(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t buf[56] = {0};
    uint8_t lmid = 0, lmid0 = 0, lmid1 = 0, nbyte = 0;

    if (len != 4)
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }

    lmid = pdata[0];
    lmid0 = pdata[1];
    lmid1 = pdata[2];

    if (lmid >= gbinfo.lm.max_lmm ||
        lmid0 >= gbinfo.lm.max_lm ||
        lmid1 >= gbinfo.lm.max_lm ||
        lmid0 > lmid1 ||
        (lmid1 - lmid0) >= 48)
    {
        return HIVE_ERROR_INVALID_PARAM;
    }

    buf[0] = lmid;
    buf[1] = lmid0;
    buf[2] = lmid1;
    buf[3] = 0;
    nbyte = 4;

    for (; lmid0 <= lmid1; ++lmid0)
    {
        // memcpy(&buf[nbyte], &sk_la_lm_kc_info.lm_table[lmid][lmid0], 1);
        nbyte += 1;
    }

    return cmd_do_response(CMD_LA, KM_GET_KM, nbyte, 0, buf);
}

static uint32_t on_la_write_lm(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t buf[8] = {0};
    uint8_t lmid = 0, lmid0 = 0, lmid1 = 0;// nbyte = 0;

    if (len < 5 || len > 56)
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }

    lmid = pdata[0];
    lmid0 = pdata[1];
    lmid1 = pdata[2];
    //nbyte = 4;

    /// DBG_PRINTF(HIVE_DEBUG_LEVEL_DEBUG, "cur_lm=%d, lmid=%d, lmid0=%d lmid1=%d", gbinfo.lm.cur_lmm, lmid, lmid0, lmid1);

    if (lmid >= gbinfo.lm.max_lmm ||
        lmid0 >= gbinfo.lm.max_lm ||
        lmid1 >= gbinfo.lm.max_lm ||
        lmid1 > lmid0 ||
        (lmid1 - lmid0) >= 48)
    {
        return HIVE_ERROR_INVALID_PARAM;
    }

    // memcpy(&sk_la_lm_kc_info.lm_table[lmid][lmid0], &pdata[nbyte], lmid1 - lmid0 + 1);

    db_update(DB_SK_LA_LM_KC, UPDATE_LATER);

    *(uint32_t *)&buf[0] = HIVE_SUCCESS;
    buf[4] = pdata[0];
    buf[5] = pdata[1];
    buf[6] = pdata[2];
    buf[7] = 0;

    for (int i = lmid0; i <= lmid1; ++i)
    {
        rgb_led_set_one(i);
    }

    return cmd_do_response(CMD_LA, KM_SET_KM, 8, 0, buf);
}

static uint32_t on_la_get_la_info(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    (void) pdata;
    uint8_t buf[1] = {0};

    buf[0] = LED_ACTION_MAX;

    return cmd_do_response(CMD_LA, KM_GET_LM_SIZE, sizeof(buf), 0, buf);
}

static uint32_t on_la_read_la(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t buf[56] = {0};

    if (len != 1)
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }

    if (pdata[0] >= LED_ACTION_MAX)
    {
        return HIVE_ERROR_INVALID_PARAM;
    }

    led_action_t tmp = sk_la_lm_kc_info.la_table[pdata[0]];
    tmp.hl = VALUE_TO_PERCENT(tmp.hl, LED_BRIGHTNESS_MAX) / 2;

    memcpy(&buf, &tmp, sizeof(led_action_t));

    return cmd_do_response(CMD_LA, KM_GET_LM, sizeof(led_action_t), 0, buf);
}

static uint32_t on_la_write_la(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) pdata;
    uint32_t e = HIVE_SUCCESS;
    led_action_t *pla = (led_action_t *)pdata;

    if (len != sizeof(led_action_t))
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }

    if (pla->id >= LED_ACTION_MAX)
    {
        return HIVE_ERROR_INVALID_PARAM;
    }

    DBG_PRINTF("write_la: id=%d at=%d cy=%d rr=%d pr=%d bd=%d ls=%d hl=%d ll=%d tpt=%d hzc=%d pdy=%d hdy=%d",
            pla->id, pla->at, pla->cy, pla->rr, pla->pr, pla->bd, pla->ls, pla->hl, pla->ll, pla->tpt, pla->hzc, pla->pdy, pla->hdy);

    pla->hl = PERCENT_TO_VALUE(pla->hl * 2, LED_BRIGHTNESS_MAX);

    memcpy(&sk_la_lm_kc_info.la_table[pla->id], pla, sizeof(led_action_t));
    db_update(DB_SK_LA_LM_KC, UPDATE_LATER);

    rgb_led_set_all();

    return cmd_do_response(CMD_LA, KM_SET_LM, sizeof(e), 0, (uint8_t *)&e);
}

#endif

#ifdef LB_ENABLED
static uint32_t on_lb_back_get_state(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void)len;
    (void)pdata;

    uint8_t state = gbinfo.rl[LIGHT_TAPE_PART_BACK].state;

    cmd_do_response(CMD_LB_BACK, LB_BACK_GET_SATE, sizeof(state), reserve, &state);
    return HIVE_SUCCESS;
}

static uint32_t on_lb_back_set_state(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void)len;

    uint32_t e = HIVE_SUCCESS;

    gbinfo.rl[LIGHT_TAPE_PART_BACK].state = pdata[0];
    db_update(DB_SYS, UPDATE_LATER);

    cmd_do_response(CMD_LB_BACK, LB_BACK_SET_SATE, 4, reserve, (uint8_t *)&e);

    return HIVE_SUCCESS;
}

static uint32_t on_lb_back_get_para(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void)reserve;

    uint8_t buf[4 + RGBLIGHT_EFFECT_MAX*4] = {0};
    uint8_t buf_len = 0;

    buf[buf_len++] = RGBLIGHT_EFFECT_MAX;
    if (!gbinfo.rl[LIGHT_TAPE_PART_BACK].state)
    {
        buf[buf_len++] = 0;
    }
    else
    {                                                                                                                                                
        buf[buf_len++] = gbinfo.rl[LIGHT_TAPE_PART_BACK].md;
    }

    uint8_t pack_max = (64 - 8 - 4) / 4 ;
    buf[buf_len++] = (RGBLIGHT_EFFECT_MAX + pack_max - 1) / pack_max;
    if (pdata[0] >= buf[2] || len > 1)
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }

    uint8_t current_pid = pdata[0];

    if (buf[2] > 1)
    {
        buf[buf_len++] = current_pid;

        for (uint8_t i = current_pid * pack_max; i < (current_pid + 1) * pack_max; i++)
        {
            if (i < RGBLIGHT_EFFECT_MAX)
            {
                buf[buf_len++] = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[i].hsv.h;
                buf[buf_len++] = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[i].hsv.s;
                buf[buf_len++] = VALUE_TO_PERCENT(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[i].hsv.v, RGBLIGHT_LIMIT_VAL);
                buf[buf_len++] = 9 - gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[i].speed;
            }
        }
    }
    else
    {
        buf[buf_len++] = current_pid;
        for (uint8_t i = 0; i < RGBLIGHT_EFFECT_MAX; i++)
        {
            buf[buf_len++] = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[i].hsv.h;
            buf[buf_len++] = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[i].hsv.s;
            buf[buf_len++] = VALUE_TO_PERCENT(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[i].hsv.v, RGBLIGHT_LIMIT_VAL);
            buf[buf_len++] = 9 - gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[i].speed;
        }
         
    }

    return cmd_do_response(CMD_LB_BACK, LB_BACK_GET_PARA, buf_len, 0, buf);
}

static uint32_t on_lb_back_set_para(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    uint32_t e = HIVE_SUCCESS;
    // DBG_PRINTF(" on_lb_set_para = %d,%d,%d,%d\r\n", pdata[0],pdata[1],pdata[2],pdata[3]);
    if (pdata[0] >= RGBLIGHT_EFFECT_MAX)
    {
        return HIVE_ERROR_INVALID_PARAM;
    }

    if (pdata[0] == 0)
    {
        gbinfo.rl[LIGHT_TAPE_PART_BACK].state = 0;
    }
    else
    {
        gbinfo.rl[LIGHT_TAPE_PART_BACK].state = 1;
        gbinfo.rl[LIGHT_TAPE_PART_BACK].md = pdata[0];
#if 1
        gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h = pdata[1];
        gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s = pdata[2];
        gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v = PERCENT_TO_VALUE(pdata[3], RGBLIGHT_LIMIT_VAL);
#else
        gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h = 0;
        gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s = 0;
        gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v = 148;
#endif
        if ((pdata[4] > 8) || (pdata[4] < 1))
        {
            // data error
        }
        else
        {
            gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].speed = 9 - pdata[4];
        }
    }
    db_update(DB_SYS, UPDATE_LATER);

    return cmd_do_response(CMD_LB_BACK, LB_BACK_SET_PARA, 4, 0, (uint8_t *)&e);
}

static uint32_t on_lb_back_get_list_id(uint16_t len, uint16_t reserve, uint8_t *pdata)
{

    uint8_t buf[RGBLIGHT_EFFECT_MAX] = {0};

    const rgblight_mode *list = rgblight_get_mode_list();
    memcpy(buf, list, sizeof(buf));

    return cmd_do_response(CMD_LB_BACK, LB_BACK_SET_LIST_ID, sizeof(buf), 0, buf);
}

static uint32_t on_lb_back_get_shape(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    uint8_t shape = gbinfo.rl_shape;

    return cmd_do_response(CMD_LB_BACK, LB_BACK_GET_SHAPE, sizeof(shape), 0, (uint8_t *)&shape);
}

static uint32_t on_lb_back_set_shape(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    uint32_t e = HIVE_SUCCESS;
    uint8_t shape = pdata[0];
    if (shape > LIGHT_TAPE_WHOLE)
    {
        e = HIVE_ERROR_INVALID_PARAM;
    }
    else
    {
        prompt_type_e type;
        rgb_sm_state_e state;
        rgb_led_get_prompt(&state, &type);
        if (state == STATE_SYNC)
        {
            if (shape == 0)
            {
                gbinfo.rl_shape = shape;
                light_tape_init_ranges();
                db_update(DB_SYS, UPDATE_LATER);
            }
            else
            {
                e = HIVE_ERROR_GENERIC;
            }
        }
        else
        {
            gbinfo.rl_shape = shape;
            light_tape_init_ranges();
            db_update(DB_SYS, UPDATE_LATER);
        }
    }

    return cmd_do_response(CMD_LB_BACK, LB_BACK_SET_SHAPE, 4, 0, (uint8_t *)&e);
}

static uint32_t on_lb_side_get_state(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    uint8_t state = gbinfo.rl[LIGHT_TAPE_PART_SIDE].state;

    cmd_do_response(CMD_LB_SIDE, LB_SIDE_GET_SATE, sizeof(state), reserve, &state);
    return HIVE_SUCCESS;
}

static uint32_t on_lb_side_set_state(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    uint32_t e = HIVE_SUCCESS;

    gbinfo.rl[LIGHT_TAPE_PART_SIDE].state = pdata[0];
    db_update(DB_SYS, UPDATE_LATER);

    cmd_do_response(CMD_LB_SIDE, LB_SIDE_SET_SATE, 4, reserve, (uint8_t *)&e);

    return HIVE_SUCCESS;
}

static uint32_t on_lb_side_get_para(uint16_t len, uint16_t reserve, uint8_t *pdata)
{

    uint8_t buf[4 + RGBLIGHT_EFFECT_MAX*4] = {0};
    uint8_t buf_len = 0;

    buf[buf_len++] = RGBLIGHT_EFFECT_MAX;
    if (!gbinfo.rl[LIGHT_TAPE_PART_SIDE].state)
    {
        buf[buf_len++] = 0;
    }
    else
    {
        buf[buf_len++] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].md;
    }

    uint8_t pack_max = (64 - 8 - 4) / 4 ;
    buf[buf_len++] = (RGBLIGHT_EFFECT_MAX + pack_max - 1) / pack_max;
    if (pdata[0] >= buf[2] || len > 1)
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }

    uint8_t current_pid = pdata[0];

    if (buf[2] > 1)
    {
        buf[buf_len++] = current_pid;

        for (uint8_t i = current_pid * pack_max; i < (current_pid + 1) * pack_max; i++)
        {
            if (i < RGBLIGHT_EFFECT_MAX)
            {
                buf[buf_len++] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[i].hsv.h;
                buf[buf_len++] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[i].hsv.s;
                buf[buf_len++] = VALUE_TO_PERCENT(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[i].hsv.v, RGBLIGHT_LIMIT_VAL);
                buf[buf_len++] = 9 - gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[i].speed;
            }
        }
    }
    else
    {
        buf[buf_len++] = current_pid;
        for (uint8_t i = 0; i < RGBLIGHT_EFFECT_MAX; i++)
        {
            buf[buf_len++] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[i].hsv.h;
            buf[buf_len++] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[i].hsv.s;
            buf[buf_len++] = VALUE_TO_PERCENT(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[i].hsv.v, RGBLIGHT_LIMIT_VAL);
            buf[buf_len++] = 9 - gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[i].speed;
        }
         
    }

    return cmd_do_response(CMD_LB_SIDE, LB_SIDE_GET_PARA, buf_len, 0, buf);
}

static uint32_t on_lb_side_set_para(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    uint32_t e = HIVE_SUCCESS;
    // DBG_PRINTF(" on_lb_set_para = %d,%d,%d,%d\r\n", pdata[0],pdata[1],pdata[2],pdata[3]);
    if (pdata[0] >= RGBLIGHT_EFFECT_MAX)
    {
        return HIVE_ERROR_INVALID_PARAM;
    }

    if (pdata[0] == 0)
    {
        gbinfo.rl[LIGHT_TAPE_PART_SIDE].state = 0;
    }
    else
    {
        gbinfo.rl[LIGHT_TAPE_PART_SIDE].state = 1;
        gbinfo.rl[LIGHT_TAPE_PART_SIDE].md = pdata[0];
#if 1
        gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h = pdata[1];
        gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s = pdata[2];
        gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v = PERCENT_TO_VALUE(pdata[3], RGBLIGHT_LIMIT_VAL);
#else
        gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h = 0;
        gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s = 0;
        gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v = 148;
#endif
        if ((pdata[4] > 8) || (pdata[4] < 1))
        {
            // data error
        }
        else
        {
            gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].speed = 9 - pdata[4];
        }
    }
    db_update(DB_SYS, UPDATE_LATER);

    return cmd_do_response(CMD_LB_SIDE, LB_SIDE_SET_PARA, 4, 0, (uint8_t *)&e);
}

static uint32_t on_lb_side_get_list_id(uint16_t len, uint16_t reserve, uint8_t *pdata)
{

    uint8_t buf[RGBLIGHT_EFFECT_MAX] = {0};

    const rgblight_mode *list = rgblight_get_mode_list();
    memcpy(buf, list, sizeof(buf));

    return cmd_do_response(CMD_LB_SIDE, LB_SIDE_SET_LIST_ID, sizeof(buf), 0, buf);
}
#endif

static uint32_t on_ks_set_act_point(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) pdata;
    (void) len;
    uint8_t respBuffer[8] = {0};
    uint16_t respBufferLenght = 0;

    if (mg_detecte_get_volt_low_state())
    {
        return HIVE_ERROR_GENERIC;
    }
    ks_cmd_set_general_trigger_stroke(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_KS, KS_SET_ACT_POINT, respBufferLenght, 0, respBuffer);
}

static uint32_t on_ks_get_act_point(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    ks_cmd_get_general_trigger_stroke(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_KS, KS_GET_ACT_POINT, respBufferLenght, 0, respBuffer);
}

static uint32_t on_ks_set_rt_para(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    // uint32_t e = HIVE_SUCCESS;

    DBG_PRINTF("%s\n", __FUNCTION__);

    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    if (mg_detecte_get_volt_low_state())
    {
        return HIVE_ERROR_GENERIC;
    }
    ks_cmd_set_rapid_trigger(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_KS, KS_SET_RT_PARA, respBufferLenght, 0, respBuffer);
}

static uint32_t on_ks_get_rt_para(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    // uint32_t e = HIVE_SUCCESS;
    (void) reserve;

    DBG_PRINTF("%s\n",__FUNCTION__);

    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    ks_cmd_get_rapid_trigger(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_KS, KS_GET_RT_PARA, respBufferLenght, 0, respBuffer);
}

static uint32_t on_ks_set_low_latency(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) pdata;
    (void) len;
    uint32_t e = HIVE_SUCCESS;

    // cmd_mcu_send(CMD_KEY_PARA, KS_CMD_SET_LOW_LANCYTE, pdata, len);
    // return cmd_do_response(CMD_KS, KS_CMD_SET_LOW_LATENCY, 1, 0, (uint8_t *)&e);
    return e;
}

static uint32_t on_ks_get_low_latency(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) pdata;
    (void) len;
    uint32_t e = HIVE_SUCCESS;

    // return cmd_do_response(CMD_KS, KS_CMD_SET_LOW_LATENCY, 1, 0, (uint8_t *)&e);
    return e;
}

static uint32_t on_ks_set_tickrate(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) pdata;
    (void) len;
    uint32_t e = HIVE_SUCCESS;

    // cmd_mcu_send(CMD_KEY_PARA, KS_CMD_SET_TICK_RATE, pdata, len);
    // return cmd_do_response(CMD_KS, KS_CMD_SET_GAME_TICKRATE, 1, 0, (uint8_t *)&e);
    return e;
}

static uint32_t on_ks_get_tickrate(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) pdata;
    (void) len;
    uint32_t e = HIVE_SUCCESS;

    // return cmd_do_response(CMD_KS, KS_CMD_SET_GAME_TICKRATE | 0x10, 1, 0, (uint8_t *)&e);
    return e;
}

// static uint32_t on_ks_set_dks(uint16_t len, uint16_t reserve, uint8_t *pdata)
//{
// uint8_t respBuffer[56]    = {0};
// uint16_t respBufferLenght = 0;
// ks_cmd_set_dks_para_table(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

// return cmd_do_response(CMD_SK, SK_SET_DKS, respBufferLenght, 0, respBuffer);
//}

// static uint32_t on_ks_get_dks(uint16_t len, uint16_t reserve, uint8_t *pdata)
//{
// uint8_t respBuffer[56]    = {0};
// uint16_t respBufferLenght = 0;
// ks_cmd_get_dks_para_table(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

// return cmd_do_response(CMD_SK, SK_GET_DKS, respBufferLenght, 0, respBuffer);
//}

static uint32_t on_ks_set_dks_act_point(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) pdata;
    (void) len;
    uint32_t e = HIVE_SUCCESS;
    DBG_PRINTF("on_ks_get_dks idx=%d,len=%d\n", pdata[0], len);
    // cmd_mcu_send(CMD_KEY_PARA, KS_CMD_GET_DKS, pdata, len);

    return e;
}

static uint32_t on_ks_get_dks_act_point(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) pdata;
    (void) len;
    uint32_t e = HIVE_SUCCESS;
    DBG_PRINTF("on_ks_get_dks idx=%d,len=%d\n", pdata[0], len);
    // cmd_mcu_send(CMD_KEY_PARA, KS_CMD_GET_DKS, pdata, len);

    return e;
}

// static uint32_t on_ks_get_dks_size(uint16_t len, uint16_t reserve, uint8_t *pdata)
//{
// uint32_t e = HIVE_SUCCESS;
// DBG_PRINTF("on_ks_get_dks idx=%d,len=%d\n", pdata[0], len);
// cmd_mcu_send(CMD_KEY_PARA, KS_CMD_GET_DKS, pdata, len);

// return e;
//}

static uint32_t on_ks_set_mt(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) pdata;
    (void) len;
    uint32_t e = HIVE_SUCCESS;

    // cmd_mcu_send(CMD_KEY_PARA, KS_CMD_SET_MT, pdata, len);
    return e;
}

static uint32_t on_ks_get_mt(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) pdata;
    (void) len;
    uint32_t e = HIVE_SUCCESS;
    return e;

    // return cmd_do_response(CMD_KS, KS_CMD_SET_MT, 1, 0, (uint8_t *)&e);
}

static uint32_t on_ks_set_tgl(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) pdata;
    (void) len;
    uint32_t e = HIVE_SUCCESS;
    return e;

    // cmd_mcu_send(CMD_KEY_PARA, KS_CMD_SET_TGL, pdata, len);
    // return cmd_do_response(CMD_KS, SUB_CMD_SET_TGL, 1, 0, (uint8_t *)&e);
}

static uint32_t on_ks_get_tgl(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) pdata;
    (void) len;
    uint32_t e = HIVE_SUCCESS;
    return e;
    // return cmd_do_response(CMD_KS, SUB_CMD_SET_TGL | 0x10, 1, 0, (uint8_t *)&e);
}

static uint32_t on_ks_set_ps(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) pdata;
    (void) len;
    uint32_t e = HIVE_SUCCESS;
    return e;

    // cmd_mcu_send(CMD_KEY_PARA, KS_CMD_SET_POWER_SAVE, pdata, len);
    // return cmd_do_response(CMD_KS, KS_CMD_SET_PS, 1, 0, (uint8_t *)&e);
}

static uint32_t on_ks_get_ps(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) pdata;
    (void) len;
    uint32_t e = HIVE_SUCCESS;
    return e;

    // return cmd_do_response(CMD_KS, KS_CMD_SET_PS, 1, 0, (uint8_t *)&e);
}

static uint32_t on_ks_set_key_rpt(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    ks_cmd_set_key_report(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_KS, KS_SET_KEY_REPORT, respBufferLenght, 0, respBuffer);
}

static uint32_t on_ks_get_key_rpt(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    ks_cmd_get_key_report(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_KS, KS_GET_KEY_REPORT, respBufferLenght, 0, respBuffer);
}

static uint32_t on_ks_set_cali(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    uint8_t ret;
    (void) reserve;
    (void) len;

    char tmpBuff[10] = {'\0'};
    sprintf(tmpBuff, "%s", pdata[0] == KEY_CALIB_ABORT ? "ABORT" : pdata[0] == KEY_CALIB_START ? "START"
                                                               : pdata[0] == KEY_CALIB_DONE    ? "DONE"
                                                                                               : "UNKONW");
    DBG_PRINTF("step  calib:%s\n", tmpBuff);

    ret = (uint8_t)cali_cmd((cail_step_t)pdata[0]);

    return cmd_do_response(CMD_KS, KS_SET_CALI, 1, 0, &ret);
}

static uint32_t on_ks_get_cali(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    ks_cmd_get_cali(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_KS, KS_GET_CALI, respBufferLenght, 0, respBuffer);
}

// act point resolution
static uint32_t on_ks_get_ap_res(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    // uint8_t res = 20;
    // DBG_PRINTF("on_ks_get_ap_res\n");
    // cmd_mcu_send(CMD_KEY_PARA, KS_CMD_GET_ACTP_RES, pdata, len);
    (void) reserve;

    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    ks_cmd_get_key_para(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_KS, KS_GET_AP_RES, respBufferLenght, 0, respBuffer);
}

// static uint32_t on_ks_get_dks_list(uint16_t len, uint16_t reserve, uint8_t *pdata)
//{
// uint32_t e = HIVE_SUCCESS;
// DBG_PRINTF("on_ks_get_dks_list idx=%d,len=%d\n", pdata[0], len);
// cmd_mcu_send(CMD_KEY_PARA, KS_CMD_GET_DKS, pdata, len);

// return e;
//}

static uint32_t on_ks_set_dz(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    // uint32_t e = HIVE_SUCCESS;
    // DBG_PRINTF("on_ks_set_dz idx=%d,len=%d\n", pdata[0], len);
    // cmd_mcu_send(CMD_KEY_PARA, KS_CMD_GET_DKS, pdata, len);
    (void) reserve;

    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    if (mg_detecte_get_volt_low_state())
    {
        return HIVE_ERROR_GENERIC;
    }
    ks_cmd_set_dead_band(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_KS, KS_SET_DZ, respBufferLenght, 0, respBuffer);
}

static uint32_t on_ks_get_dz(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    // uint32_t e = HIVE_SUCCESS;
    // DBG_PRINTF("on_ks_get_dz idx=%d,len=%d\n", pdata[0], len);
    // cmd_mcu_send(CMD_KEY_PARA, KS_CMD_GET_DKS, pdata, len);
    (void) reserve;

    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    ks_cmd_get_dead_band(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_KS, KS_GET_DZ, respBufferLenght, 0, respBuffer);
}

static uint32_t on_ks_get_pr(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) len;
    (void) pdata;
    uint16_t scan_ms = 0;
    (void) reserve;

    scan_ms = gbinfo.sms;

    return cmd_do_response(CMD_KS, KS_GET_PR, 2, 0, (uint8_t *)&scan_ms);
}

static uint32_t on_ks_set_pr(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint32_t e = HIVE_SUCCESS;
    uint16_t scan_ms = 0;

    if (len != 0x0002)
    {
        e = HIVE_ERROR_INVALID_LENGTH;

        return cmd_do_response(CMD_KS, KS_SET_PR, 4, 0, (uint8_t *)&e);
    }

    scan_ms = *(uint16_t *)pdata;

    if (scan_ms >= 1 && scan_ms <= 10)
    {
        gbinfo.sms = scan_ms;
        db_update(DB_SYS, UPDATE_LATER);
        // board_reset_scan_timer(gbinfo.sms);
    }
    else
    {
        e = HIVE_ERROR_DATA_SIZE;
    }

    return cmd_do_response(CMD_KS, KS_SET_PR, 4, 0, (uint8_t *)&e);
}

static uint32_t on_ks_get_nkro(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    (void) pdata;
    uint8_t buf[1] = {0};

    buf[0] = gbinfo.nkro;

    return cmd_do_response(CMD_KS, KS_GET_NKRO, sizeof(buf), 0, buf);
}

static uint32_t on_ks_set_nkro(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    uint32_t e = HIVE_SUCCESS;
    (void) reserve;

    if (len != 1)
    {
        return HIVE_ERROR_INVALID_LENGTH;
    }

    if (*pdata != BOARD_NKRO_DISABLE && *pdata != BOARD_NKRO_ENABLE)
    {
        return HIVE_ERROR_INVALID_PARAM;
    }
    
    if (gbinfo.nkro != *pdata)
    {
        gbinfo.nkro = *pdata;
        db_update(DB_SYS, UPDATE_LATER);
        matrix_set_nkro(gbinfo.nkro);
    }

    return cmd_do_response(CMD_KS, KS_SET_NKRO, sizeof(e), 0, (uint8_t *)&e);
}

static uint32_t on_ks_get_turing(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    key_cmd_get_tuning(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_KS, KS_GET_TUNING, respBufferLenght, 0, respBuffer);
}

static uint32_t on_ks_set_turing(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    key_cmd_set_tuning(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_KS, KS_SET_TUNING, respBufferLenght, 0, respBuffer);
}

static uint32_t on_ks_get_shaft_type(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    ks_cmd_get_default_shaft_type(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_KS, KS_GET_SHAFT_TYPE, respBufferLenght, 0, respBuffer);
}

static uint32_t on_ks_set_shaft_type(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    ks_cmd_set_shaft_type(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_KS, KS_SET_SHAFT_TYPE, respBufferLenght, 0, respBuffer);
}

static uint32_t on_ks_get_wave_max(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    uint8_t respBuffer[56] = {0};
    uint16_t respBufferLenght = 0;
    ks_cmd_get_wave_max(pdata, len, respBuffer, ARRAY_SIZE(respBuffer), &respBufferLenght);

    return cmd_do_response(CMD_KS, KS_GET_WAVE_MAX, respBufferLenght, 0, respBuffer);
}

static uint32_t on_ks_get_rt_sep(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    (void) pdata;
    uint8_t buf[1] = {0};

    buf[0] = gbinfo.kh_qty.rt_separately_en;

    return cmd_do_response(CMD_KS, KS_GET_RT_SEP, sizeof(buf), 0, buf);
}

static uint32_t on_ks_set_rt_sep(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    uint32_t e = HIVE_SUCCESS;
    (void) reserve;

    if (len != 1)
    {
        e = HIVE_ERROR_INVALID_LENGTH;
    }
    else
    {
        if (gbinfo.kh_qty.rt_separately_en != *pdata)
        {
            gbinfo.kh_qty.rt_separately_en = (*pdata != 0) ? true : false;
            db_update(DB_SYS, UPDATE_LATER);
        }
    }

    return cmd_do_response(CMD_KS, KS_SET_RT_SEP, sizeof(e), 0, (uint8_t *)&e);
}


static uint32_t on_ks_set_test(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    (void) pdata;
    uint32_t e = HIVE_SUCCESS;

    // if (pdata[0])   cmd_mcu_start_ks();
    // else            cmd_mcu_stop_ks();

    // kh.p_kh_gst->d2u_time_ms_min = *(uint16_t *)(pdata);
    // kh.p_kh_gst->d2u_time_ms_max = *(uint16_t *)(pdata+2);
    // kh.p_kh_gst->shape_over_vale = *(pdata+4);

    // DBG_PRINTF("set min:%dms, max:%dms, shapeV:%d\r\n", kh.p_kh_gst->d2u_time_ms_min, kh.p_kh_gst->d2u_time_ms_max, kh.p_kh_gst->shape_over_vale);

    return e;
}
static uint32_t on_ks_get_test(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    (void) pdata;
    uint32_t e = HIVE_SUCCESS;
    // DBG_PRINTF("min:%04d, max:%04d\r\n", kh.p_kh_gst->d2u_time_ms_min, kh.p_kh_gst->d2u_time_ms_max);
    /*
    DBG_PRINTF(" cmd_hive = %d \r\n",(int32_t)uxTaskGetStackHighWaterMark(m_cmd_hive_thread));
    DBG_PRINTF(" log      = %d \r\n",(int32_t)uxTaskGetStackHighWaterMark(m_logger_thread));
    DBG_PRINTF(" kh       = %d \r\n",(int32_t)uxTaskGetStackHighWaterMark(m_kh_thread));
    DBG_PRINTF(" ka       = %d \r\n",(int32_t)uxTaskGetStackHighWaterMark(m_ka_thread));
    DBG_PRINTF(" tm       = %d \r\n",(int32_t)uxTaskGetStackHighWaterMark(xTimerGetTimerDaemonTaskHandle()));
    DBG_PRINTF(" usb      = %d \r\n",(int32_t)uxTaskGetStackHighWaterMark(m_usbd_thread));
    */
    return e;
}

static uint32_t on_dpm_custom_pic_dl(uint16_t len, uint16_t reserve, uint8_t *pdata)
{

    #define PIC_SUBPACKETS_PER_PACKET (4)
    #define PIC_BUF_SIZE              (256)
    #define PIC_BUF_USED_MALLOC       (0)

    (void) reserve;
    (void) len;
    uint32_t e = HIVE_SUCCESS;
    static pic_trans_info_t trans_info = {0};

    if (PIC_TRANS_START == pdata[0])
    {
        if (trans_info.start_flag == 0)
        {
            #if 1 == PIC_BUF_USED_MALLOC
            if (trans_info.p_buf != NULL)
            {
                vPortFree((void *)trans_info.p_buf);
                trans_info.p_buf = NULL;
            }
            #endif
            memset(&trans_info, 0, sizeof(trans_info));

            trans_info.start_flag                   = 1;
            trans_info.pic_id                       = pdata[1];
            trans_info.type                         = pdata[2];
            trans_info.total_size                   = *(uint32_t *)&pdata[3];
            trans_info.sub_info.total_pkg_num       = *(uint32_t *)&pdata[7];
            trans_info.sub_info.payload_max_len     = *(uint16_t *)&pdata[11];
            trans_info.sub_info.last_payload_len    = trans_info.total_size % trans_info.sub_info.payload_max_len;

            if ((trans_info.type >= 3)  ||
                (trans_info.sub_info.payload_max_len > 51)  ||
                (trans_info.sub_info.payload_max_len < 1)   ||
                (trans_info.sub_info.total_pkg_num < 1)     ||
                (trans_info.total_size < 1))
            {
                e = HIVE_ERROR_INVALID_PARAM;
                DBG_PRINTF("Invalid pic trans info");
                goto __fail;
            }

            #if 1 == PIC_BUF_USED_MALLOC
            if (NULL == trans_info.p_buf)
            {
                trans_info.p_buf = (uint8_t *)pvPortMalloc(PIC_BUF_SIZE);
                if (trans_info.p_buf != NULL)
                {
                    memset(trans_info.p_buf, 0, PIC_BUF_SIZE);
                }
                else
                {
                    e = HIVE_ERROR_INVALID_PARAM;
                    DBG_PRINTF("malloc p_buf failed");
                    goto __fail;
                }   
            }
            #else
            trans_info.p_buf = (uint8_t *)dfu_get_bin_head_buf();
            #endif

            trans_info.group_info.payload_max_len  = PIC_SUBPACKETS_PER_PACKET * trans_info.sub_info.payload_max_len;
            trans_info.group_info.total_pkg_num    = (trans_info.total_size + trans_info.group_info.payload_max_len - 1) /
                                                      trans_info.group_info.payload_max_len;
            trans_info.group_info.last_payload_len = trans_info.total_size % trans_info.group_info.payload_max_len;

            uint32_t info_len = sizeof(trans_info.pic_id) + 
                                sizeof(trans_info.type) + 
                                sizeof(trans_info.total_size) + 
                                sizeof(trans_info.group_info.total_pkg_num) +
                                sizeof(trans_info.group_info.payload_max_len);

            trans_info.p_buf[0] = PIC_TRANS_START;
            memcpy(trans_info.p_buf + 1, &trans_info.pic_id, info_len);

            if (HIVE_SUCCESS != uart_set_screen_picture(trans_info.p_buf, info_len + 1))
            {
                e = HIVE_ERROR_GENERIC;
                goto __fail;
            }
        }
        else
        {
            e = HIVE_ERROR_GENERIC;
            goto __fail;
        }
    }
    else if (PIC_TRANS_CANCEL == pdata[0])
    {
        if (trans_info.start_flag != 1)
        {
            e = HIVE_ERROR_GENERIC;
            goto __fail;
        }

        trans_info.p_buf[0] = PIC_TRANS_CANCEL;
        if (HIVE_SUCCESS != uart_set_screen_picture(trans_info.p_buf, 1))
        {
            e = HIVE_ERROR_GENERIC;
            goto __fail;
        }

        e = HIVE_ERROR_GENERIC;
        DBG_PRINTF("Cancel the download of picture");
        goto __fail;
    }
    else if (PIC_TRANS_PROG == pdata[0])
    {
        if (trans_info.start_flag != 1)
        {
            e = HIVE_ERROR_GENERIC;
            goto __fail;
        }
        else
        {
            DBG_PRINTF("pic_prog: pkg_id = %d" , *(uint32_t *)&pdata[1]);
            if (*(uint32_t *)&pdata[1] > 0)
            {
                if (*(uint32_t *)&pdata[1] != trans_info.sub_info.curr_pkg_index + 1)
                {
                    e = HIVE_ERROR_GENERIC;
                    DBG_PRINTF("pic pkg index error");
                    goto __fail;
                }
            }
        }

        trans_info.sub_info.curr_pkg_index = *(uint32_t *)&pdata[1];

        int subpacket_index = trans_info.sub_info.curr_pkg_index % PIC_SUBPACKETS_PER_PACKET;
        int offset = subpacket_index * trans_info.sub_info.payload_max_len;

        memcpy(trans_info.p_buf + 5 + offset, pdata + 5, trans_info.sub_info.payload_max_len);
        if (subpacket_index == (PIC_SUBPACKETS_PER_PACKET - 1))
        {
            trans_info.p_buf[0] = PIC_TRANS_PROG;
            *(uint32_t *)&trans_info.p_buf[1] = trans_info.group_info.curr_pkg_index;
            if (HIVE_SUCCESS != uart_set_screen_picture(trans_info.p_buf, trans_info.group_info.payload_max_len + 5))
            {
                e = HIVE_ERROR_GENERIC;
                goto __fail;
            }
            trans_info.group_info.curr_pkg_index++;
        }
    }
    else if (PIC_TRANS_FINISH == pdata[0])
    {
        if ((trans_info.start_flag != 1) || 
            (*(uint32_t *)&pdata[1] != trans_info.sub_info.total_pkg_num - 1))
        {
            e = HIVE_ERROR_GENERIC;
            goto __fail;
        }

        trans_info.sub_info.curr_pkg_index = *(uint32_t *)&pdata[1];

        int subpacket_index = trans_info.sub_info.curr_pkg_index % PIC_SUBPACKETS_PER_PACKET;
        int offset = subpacket_index * trans_info.sub_info.payload_max_len;

        memcpy(trans_info.p_buf + 5 + offset, pdata + 5, trans_info.sub_info.last_payload_len);

        if ((trans_info.group_info.last_payload_len != (offset + trans_info.sub_info.last_payload_len)) ||
            (trans_info.group_info.curr_pkg_index   != trans_info.group_info.total_pkg_num - 1))
        {
            e = HIVE_ERROR_GENERIC;
            goto __fail;
        }
        trans_info.p_buf[0] = PIC_TRANS_FINISH;
        *(uint32_t *)&trans_info.p_buf[1] = trans_info.group_info.curr_pkg_index;

        if (HIVE_SUCCESS != uart_set_screen_picture(trans_info.p_buf, trans_info.group_info.last_payload_len + 5))
        {
            e = HIVE_ERROR_GENERIC;
            goto __fail;
        }

        #if 1 == PIC_BUF_USED_MALLOC
        if (trans_info.p_buf != NULL)
        {
            vPortFree((void *)trans_info.p_buf);
            trans_info.p_buf = NULL;
        }
        #endif

        memset(&trans_info, 0, sizeof(trans_info));
    }
    else
    {
        e = HIVE_ERROR_GENERIC;
        goto __fail;
    }

    __fail:

    if (HIVE_SUCCESS != e)
    {
        #if 1 == PIC_BUF_USED_MALLOC
        if (trans_info.p_buf != NULL)
        {
            vPortFree((void *)trans_info.p_buf);
            trans_info.p_buf = NULL;
        }
        #endif
        memset(&trans_info, 0, sizeof(trans_info));
    }
    return cmd_do_response(CMD_DPM, DPM_CUSTOM_PIC_DL, sizeof(e), 0, (uint8_t *)&e);
}

static uint32_t on_dpm_general_data_sync(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void)reserve;
    (void)len;
    (void)pdata;
    uint32_t e = HIVE_SUCCESS;

    e = uart_set_general_data(pdata, len);

    uint8_t *p_data = uart_get_general_buff();
    if ((*p_data > 0) && (*p_data < 56) && (e == HIVE_SUCCESS))
    {
        return cmd_do_response(CMD_DPM, DPM_DATA_SYNC, *p_data, 0, p_data + 1);
    }
    else
    {
        return cmd_do_response(CMD_DPM, DPM_DATA_SYNC, sizeof(e), 0, (uint8_t *)&e);
    }
}

static uint32_t on_lamp_get_config(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void)reserve;
    (void)len;
    (void)pdata;
    lamp_keyboard_attr_t buff = {0};
    uint16_t length = get_lamp_keyboard_attr((uint8_t *)&buff);

    return cmd_do_response(CMD_LS, SYNC_GET_CONFIG, length, 0, (uint8_t *)&buff);
}

static uint32_t on_lamp_get_array(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void)reserve;
    (void)len;
    (void)pdata;

    lamp_sync_attr_t buff = {0};
    uint16_t id = *(uint16_t *)pdata;

    uint16_t length = get_lamp_sync_attr(id, (uint8_t *)&buff);

    return cmd_do_response(CMD_LS, SYNC_GET_ARRAY, length, 0, (uint8_t *)&buff);
}

static uint32_t on_lamp_set_data(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void)reserve;
    (void)len;
    (void)pdata;
    uint32_t e = HIVE_SUCCESS;

    uint8_t mode = *pdata;
    set_lamp_logic_attr(mode);

    return cmd_do_response(CMD_LS, SYNC_SET_DATA, sizeof(e), 0, (uint8_t *)&e);
}

static uint32_t on_lamp_get_data(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void)reserve;
    (void)len;
    (void)pdata;

    uint8_t mode = get_lamp_logic_attr();

    return cmd_do_response(CMD_LS, SYNC_GET_DATA, sizeof(mode), 0, (uint8_t *)&mode);
}

static uint32_t on_lamp_set_mode(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void)reserve;
    (void)len;
    (void)pdata;
    uint32_t e = HIVE_SUCCESS;

    set_autonomous_sync_mode(pdata);

    return cmd_do_response(CMD_LS, SYNC_SET_MODE, sizeof(e), 0, (uint8_t *)&e);
}

static uint32_t on_lamp_set_part(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void)reserve;
    (void)len;
    (void)pdata;
    uint32_t e = HIVE_SUCCESS;

    set_lamp_part(pdata);

    return cmd_do_response(CMD_LS, SYNC_SET_PART, sizeof(e), 0, (uint8_t *)&e);
}

static uint32_t on_lamp_set_multi(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void)reserve;
    (void)len;
    (void)pdata;
    uint32_t e = HIVE_SUCCESS;

    if (set_lamp_multiple(pdata))
    {
        return cmd_do_response(CMD_LS, SYNC_SET_MULTI, sizeof(e), 0, (uint8_t *)&e);
    }
    return e;
}


static uint32_t on_factory(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    uint8_t buf[8] = {0};
    (void) len;
    (void) pdata;

    buf[0] = (gbinfo.itf == DB_INIT_MAGIC) ? true : false;
    buf[1] = 1; // bt_is_alive
    buf[2] = 0; //
    //board_kb_send_release();

    prompt_type_e type;
    rgb_sm_state_e state;
    rgb_led_get_prompt(&state, &type);

    fa_t fa;
    fa.fa_idx  = pdata[0];
    fa.fa_step = pdata[1];
    
    if (fa.fa_idx != FA_IDX_EXIT)
    {
        if (fa.fa_step == 0)
        {
        }       
        else if ((fa.fa_idx == FA_IDX_JIANHANG) || (fa.fa_idx == FA_IDX_NARUI))
        {
            uint8_t idx  = fa.fa_idx - 1;
            uint8_t step = fa.fa_step - 1;
            mg_factory_t *factory = mg_factory_get_info();
            memcpy(&factory->fa[idx][step], &fa, sizeof(fa));
            mg_factory_set_info(factory);
            db_save_now();
            db_task_suspend();
        }
        if (((state == STATE_PROMPT) && (type >= PROMPT_DFU_SLAVE_FLASH_DOWNLOAD))
        && ((state == STATE_PROMPT) && (type <= PROMPT_DFU_SLAVE_ABORT)))
        {
        }
        else
        {
            rgb_led_set_factory(FACTORY_RGB_ENTER);
        }
        s_conn_state = ONLINE_FACTORY;
    }
    else
    {

#ifdef RGB_ENABLED
        db_reset_kr();
        db_reset_la();


        gbinfo.kr.cur_krm = 0;
        gbinfo.kr.max_krm = BOARD_LED_COLOR_LAYER_MAX;
        gbinfo.kr.max_kr  = BOARD_KEY_LED_NUM;

        gbinfo.lm.cur_lmm  = LED_ACTION_GRADUAL;
        gbinfo.lm.max_lmm  = LED_ACTION_MAX;
        gbinfo.lm.max_lm   = BOARD_KEY_LED_NUM;
        gbinfo.lm.last_lmm = LED_ACTION_MAX;
        light_tape_set_default();
        db_flash_region_mark_update(DB_SYS);
        db_save_now();
        db_task_suspend();
        if (((state == STATE_PROMPT) && (type >= PROMPT_DFU_SLAVE_FLASH_DOWNLOAD))
        && ((state == STATE_PROMPT) && (type <= PROMPT_DFU_SLAVE_ABORT)))
        {
        }
        else
        {
            rgb_led_set_factory(FACTORY_RGB_EXIT);
        }

        s_conn_state = OFFLINE;

#endif

    }
    uint8_t mode = fa.fa_idx ? 0 : 1;
    uart_ask_screen_cmd(CMD_UART_FACTORY, SCMD_FAC_MODE, &mode, sizeof(mode));

    return cmd_do_response(CMD_FAC, 0x00, sizeof(buf), 0, buf);
}

static uint32_t on_get_factory(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    uint32_t e = HIVE_SUCCESS;
    (void) reserve;
    
    fa_t fa[FA_STEP_MAX];
    uint8_t fa_step_max[VENDORS_MAX] = {FA_STEP_MAX_JIANHANG, FA_STEP_MAX_NARUI};
    uint16_t fa_step_len = 0;

    if (len != 1)
    {
        e = HIVE_ERROR_INVALID_LENGTH;
    }
    else
    {
        uint8_t fa_idx = *pdata;
        if (fa_idx != FA_IDX_EXIT)
        {
            if ((fa_idx == FA_IDX_JIANHANG) || (fa_idx == FA_IDX_NARUI))
            {
                fa_idx -= 1;
                fa_step_len = fa_step_max[fa_idx];
                mg_factory_t *factory = mg_factory_get_info();
                memcpy(fa, factory->fa[fa_idx], fa_step_len * sizeof(fa_t));
            }
        }
    }
    return cmd_do_response(CMD_FAC, FACTORY_GET_MODE, fa_step_len * sizeof(fa_t), 0, (uint8_t *)&fa[0]);
}
static uint32_t on_factory_color(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    uint32_t e = HIVE_SUCCESS;

    led_set_all_color(LED_BRIGHTNESS_MID, pdata[0], pdata[1], pdata[2]);

    if ((pdata[0] == 255) && (pdata[1] == 255) && (pdata[2] == 255))
    {
        for (uint8_t i = 0; i < LIGHT_TAPE_PART_MAX; i++)
        {
            gbinfo.rl[i].ms[gbinfo.rl[i].md].hsv.h = 255;
            gbinfo.rl[i].ms[gbinfo.rl[i].md].hsv.s = 0;
            gbinfo.rl[i].ms[gbinfo.rl[i].md].hsv.v = 200;
        }
    }
    else if (pdata[0] == 255)
    {
        for (uint8_t i = 0; i < LIGHT_TAPE_PART_MAX; i++)
        {
            gbinfo.rl[i].ms[gbinfo.rl[i].md].hsv.h = 0;
            gbinfo.rl[i].ms[gbinfo.rl[i].md].hsv.s = 255;
            gbinfo.rl[i].ms[gbinfo.rl[i].md].hsv.v = 200;
        }
    }
    else if (pdata[1] == 255)
    {
        for (uint8_t i = 0; i < LIGHT_TAPE_PART_MAX; i++)
        {
            gbinfo.rl[i].ms[gbinfo.rl[i].md].hsv.h = 85;
            gbinfo.rl[i].ms[gbinfo.rl[i].md].hsv.s = 255;
            gbinfo.rl[i].ms[gbinfo.rl[i].md].hsv.v = 200;
        }
    }
    else if (pdata[2] == 255)
    {
        for (uint8_t i = 0; i < LIGHT_TAPE_PART_MAX; i++)
        {
            gbinfo.rl[i].ms[gbinfo.rl[i].md].hsv.h = 170;
            gbinfo.rl[i].ms[gbinfo.rl[i].md].hsv.s = 255;
            gbinfo.rl[i].ms[gbinfo.rl[i].md].hsv.v = 200;
        }
    }

    prompt_type_e type;
    rgb_sm_state_e state;
    rgb_led_get_prompt(&state, &type);
    if (((state == STATE_PROMPT) && (type >= PROMPT_DFU_SLAVE_FLASH_DOWNLOAD))
    && ((state == STATE_PROMPT) && (type <= PROMPT_DFU_SLAVE_ABORT)))
    {
        e = HIVE_ERROR_GENERIC;
    }
    else
    {
        rgb_led_set_factory(FACTORY_RGB_UPDATE);
    }

    uart_ask_screen_cmd(CMD_UART_FACTORY, SCMD_FAC_COLOR, pdata, len);

    return cmd_do_response(CMD_FAC, 0x10, 4, 0, (uint8_t *)&e);
}

static uint32_t on_factory_rssi(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    (void) pdata;
    uint8_t rssi_tmp = 0xFF;

    return cmd_do_response(CMD_FAC, 0x20, 1, 0, &rssi_tmp);
}

static uint32_t on_factory_touch(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void)reserve;
    (void)len;
    (void)pdata;
    uint32_t ret = uart_ask_screen_cmd(CMD_UART_FACTORY, SCMD_FAC_TOUCH, pdata, len);

    return cmd_do_response(CMD_FAC, FACTORY_GET_TOUCH, sizeof(ret), 0, (uint8_t *)&ret);
}

static uint32_t on_key_dbg_sw(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
	(void) reserve;
    (void) len;

    uint32_t e = HIVE_SUCCESS;
    kh.p_gst->key_dbg_sw = pdata[0];
    return cmd_do_response(CMD_DEBUG, KEY_DBG_SW, sizeof(e), 0, (uint8_t *)&e);
}
static uint32_t on_key_dbg_id(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
	(void) reserve;
    (void) len;
    
    uint32_t e = HIVE_SUCCESS;
    uint8_t size = sizeof(key_dbg) / sizeof(key_dbg[0]);

    if (len > size)
    {
        e = HIVE_ERROR_INVALID_LENGTH;
    }
    else
    {
        for (int i = 0; i < len; i++)
        {

            if (pdata[i] >= BOARD_KEY_NUM)
            {
                key_dbg[i].ki = 0xff;
            }
            else
            {
                key_dbg[i].ki = pdata[i];
            }
        }
    }
    return cmd_do_response(CMD_DEBUG, KEY_DBG_ID, sizeof(e), 0, (uint8_t *)&e);
}
static uint32_t on_key_dbg_rate(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
	(void) reserve;
    (void) len;
    (void) pdata;
    uint32_t e = HIVE_SUCCESS;
    return cmd_do_response(CMD_DEBUG, KEY_DBG_RATE, sizeof(e), 0, (uint8_t *)&e);
}

static uint32_t on_log_cail_dbg_set(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    uint32_t e = HIVE_SUCCESS;
	
    if (pdata[0] > 1)
    {
        e = HIVE_ERROR_INVALID_PARAM;
    }
    cali_rpt_dbg_set_state(pdata[0]);
    return cmd_do_response(CMD_DEBUG, KEY_LOG_CAIL_RPT, sizeof(e), 0, (uint8_t *)&e);
}

static uint32_t on_firmware_crc(uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    (void) reserve;
    (void) len;
    // uint32_t e = HIVE_SUCCESS;
    uint32_t addr = 0;
    uint32_t size = 0;
    uint8_t buf[6] = {0};

    if (pdata[0] == SOFTDEVICE_CRC)
    {
        addr = 0;
        size = 0x1C000;
    }
    else if (pdata[0] == FIRMWARE_CRC)
    {
        addr = 0x1C000;
        size = 0x2D000;
    }
    else if (pdata[0] == BOOTLOADER_CRC)
    {
        addr = 0x7B000;
        size = 0x5000;
    }
    else if (pdata[0] == UICR_CRC)
    {
        addr = 0x10001000;
        size = 0x1000;
    }
    else
    {
        return 0;
    }

    *(uint16_t *)&buf[0] = crc16((uint8_t *)addr, size);
    // *(uint32_t *)&buf[2] = crc32((uint32_t *)addr, size);

    return cmd_do_response(CMD_FAC, 0x30, sizeof(buf), 0, buf);
}

uint32_t cmd_hive_send_cali_rpt(uint8_t *pdata, uint8_t len)
{
    DBG_PRINTF("ntf_send_key_rpt,size=%d\n", len);
    if (muxt)
    {
        DBG_PRINTF("cali muxt=%d\n",muxt);
        return HIVE_SUCCESS;
    }
    return cmd_do_response(CMD_NOTIFY, NTF_CALI2, len, 0, pdata);
}

uint32_t cmd_hive_send_adc_rpt(uint8_t *pdata, uint8_t len)
{
    if (muxt)
    {
        //DBG_PRINTF("kh muxt=%d\n",muxt);
        return HIVE_SUCCESS;
    }
    return cmd_do_response(CMD_NOTIFY, NTF_LVL_ADC, len, 0, pdata);
}

uint32_t cmd_hive_send_adc_cail_rpt(uint8_t *pdata, uint8_t len)
{
    if (muxt)
    {
        //DBG_PRINTF("kh muxt=%d\n",muxt);
        return HIVE_SUCCESS;
    }
    return cmd_do_response(CMD_NOTIFY, NTF_RPT_CAIL, len, 0, pdata);
}

uint32_t cmd_hive_send_key_dbg(uint8_t *pdata, uint8_t len)
{
    return cmd_do_response(CMD_NOTIFY, NTF_KEY_DBG, len, 0, pdata);
}
/// @brief 
/// @param busid 
/// @param ep 
/// @param nbytes 
static void hid_recv_handler(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void) busid;
    (void) ep;
    BaseType_t result = pdFAIL;
    hive_data_t event;
    uint32_t size;
    
    size = nbytes;
    
    
    if (size > 64)
    {
        DBG_PRINTF("hid sz err=%d\n", size);
        return;
    }
    //
    if (hal_hid_read_data(hive_data_buf[hive_data_buf_cnt % HIV_FIFO_NUM], nbytes) < 0)
    {
        return;
    }
    //DBG_PRINTF(" hid_recv_handler nbytes=%d,%d,%d,%d\n", nbytes, busid, ep, hive_data_buf_cnt);
    // for (uint8_t i = 0;i < size; i++)
    // {
    //     DBG_PRINTF("%x,", hive_data_buf[hive_data_buf_cnt][i]);
    // }
    
    
    // DBG_PRINTF("\r\n");
    event.buf  = hive_data_buf[hive_data_buf_cnt % HIV_FIFO_NUM];
    event.size = size;
    
    hive_data_buf_cnt++;
    hive_data_buf_cnt %= HIV_FIFO_NUM;
    
    if (!fifo_is_full(&hive_data_fifo))
    {
        muxt++;
        fifo_in(&hive_data_fifo, (uint8_t *)&event, 1);
    }
    else
    {
        DBG_PRINTF("hive_data_fifo is full \r\n");
    }
    
    if (xPortIsInsideInterrupt())
    {
        
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        result = xTaskNotifyFromISR( m_hive_thread, 0, eSetValueWithoutOverwrite, &higherPriorityTaskWoken );
        //DBG_PRINTF("int=%d\n",result);

        if ( pdFAIL != result )
        {
            portYIELD_FROM_ISR( higherPriorityTaskWoken );
        }
        
    }
    else
    {
        //DBG_PRINTF("app\n");
        result = xTaskNotify( m_hive_thread, 0, eSetValueWithoutOverwrite );
    }
}

uint32_t cmd_do_response(uint8_t cmd, uint8_t subcmd, uint16_t len, uint16_t reserve, uint8_t *pdata)
{
    uint32_t e = 0;
    static uint8_t send_cnt = 0;
    //bool bb = false;
    // if ((cmd == CMD_KS)&&(subcmd == KS_GET_RT_PARA))
    //DBG_PRINTF("response %x  %x len %d\r\n", pdata[0], pdata[1], len);

    if ((cmd == CMD_NOTIFY) && (subcmd == NTF_HS))
    {
        goto SEND_MSG;
    }
    
    SEND_MSG:
    memset(&hive_send_q[send_cnt], 0, sizeof(rep_hid_t));
    hive_send_q[send_cnt].hive.payload.cmd      = cmd;
    hive_send_q[send_cnt].hive.payload.scmd     = subcmd;
    hive_send_q[send_cnt].hive.payload.len      = len;
    hive_send_q[send_cnt].hive.payload.reserved = reserve;
    hive_send_q[send_cnt].hive.payload.crc      = crc16(pdata, len);
    hive_send_q[send_cnt].report_id             = REPORT_ID_CONSUMER;

    if (pdata != NULL)
    {
        memcpy( hive_send_q[send_cnt].hive.payload.data, pdata, len);
    }
    
    e = mg_hid_send_queue(&hive_send_q[send_cnt]);
    send_cnt++;
    send_cnt %= SEND_SIZE;

    //e = hal_hid_write_data(send_buf, sizeof(send_buf));
    //if (e == HIVE_ERROR_BUSY)
    {
        //bb = true;
        //e = HIVE_ERROR_BUSY_SENDING_CMD;
        //DBG_PRINTF("Cmd Send Busy!!!\n");
    }
    /* send one more time */

    return e;
}

uint32_t do_error_report(uint8_t cmd, uint8_t scmd, uint16_t len, uint16_t reserve, uint16_t cs, uint32_t error_num)
{
    uint8_t buf[12] = {0};

    *(uint8_t  *)&buf[0] = cmd;
    *(uint8_t  *)&buf[1] = scmd;
    *(uint16_t *)&buf[2] = len;
    *(uint16_t *)&buf[4] = reserve;
    *(uint16_t *)&buf[6] = cs;
    *(uint32_t *)&buf[8] = error_num;

    return cmd_do_response(CMD_ERROR, 0x00, 12, 0, buf);
}

static const cmd_pro cmd_table[] =
{
    { .cmd = CMD_SYS,   .scmd = SYS_HS,                 .func = on_sys_handshake,         },
    { .cmd = CMD_SYS,   .scmd = SYS_RESET,              .func = on_sys_reset,             },
    { .cmd = CMD_SYS,   .scmd = SYS_REBOOT,             .func = on_sys_reboot,            },
    { .cmd = CMD_SYS,   .scmd = SYS_DFU_REQ,            .func = on_sys_dfu_req,           },
    { .cmd = CMD_SYS,   .scmd = SYS_DFU_TRANS,          .func = dfu_trans_process,        },
    { .cmd = CMD_SYS,   .scmd = SYS_GET_FA_INFO,        .func = on_sys_get_vendor_info,   },
    { .cmd = CMD_SYS,   .scmd = SYS_GET_VER_INFO,       .func = on_sys_get_fw_version,    },
    { .cmd = CMD_SYS,   .scmd = SYS_GET_DEV_NAME,       .func = on_sys_get_name,          },
    { .cmd = CMD_SYS,   .scmd = SYS_SET_DEV_NAME,       .func = on_sys_set_name,          },
    { .cmd = CMD_SYS,   .scmd = SYS_GET_DEV_STATE,      .func = on_sys_get_status,        },
    { .cmd = CMD_SYS,   .scmd = SYS_GET_PS_MODE,        .func = on_sys_get_ps,            },
    { .cmd = CMD_SYS,   .scmd = SYS_SET_PS_MODE,        .func = on_sys_set_ps,            },
    { .cmd = CMD_SYS,   .scmd = SYS_GET_SLEEP_TIME,     .func = on_sys_get_inactive,      },
    { .cmd = CMD_SYS,   .scmd = SYS_SET_SLEEP_TIME,     .func = on_sys_set_inactive,      },
    { .cmd = CMD_SYS,   .scmd = SYS_GET_SN_CODE,        .func = on_sys_get_sn_code,       },
    { .cmd = CMD_SYS,   .scmd = SYS_SET_SN_CODE,        .func = on_sys_set_sn_code,       },
    { .cmd = CMD_SYS,   .scmd = SYS_GET_HIVE_VER,       .func = on_sys_get_hive_ver,      },
    { .cmd = CMD_SYS,   .scmd = SYS_GET_CFG_IDX,        .func = on_sys_get_cfg_idx,       },
    { .cmd = CMD_SYS,   .scmd = SYS_SET_CFG_IDX,        .func = on_sys_set_cfg_idx,       },
    { .cmd = CMD_SYS,   .scmd = SYS_GET_CFG,            .func = on_sys_get_cfg,           },
    { .cmd = CMD_SYS,   .scmd = SYS_SET_CFG,            .func = on_sys_set_cfg,           },
    { .cmd = CMD_SYS,   .scmd = SYS_GET_CFG_NAME,       .func = on_sys_get_cfg_name,      },
    { .cmd = CMD_SYS,   .scmd = SYS_SET_CFG_NAME,       .func = on_sys_set_cfg_name,      },
    { .cmd = CMD_SYS,   .scmd = SYS_GET_CFG_SIZE,       .func = on_sys_get_cfg_size,      },
    { .cmd = CMD_SYS,   .scmd = SYS_SET_PLAYER_NUMBER,  .func = on_sys_set_player_number, },
    { .cmd = CMD_SYS,   .scmd = SYS_GET_PLAYER_NUMBER,  .func = on_sys_get_player_number, },
    { .cmd = CMD_SYS,   .scmd = SYS_GET_SLAVE_INFO,     .func = on_sys_get_slave_fw_info, },
    { .cmd = CMD_SYS,   .scmd = SYS_GET_STARTUP_EFF_EN,   .func = on_sys_get_startup_eff_en, },     
    { .cmd = CMD_SYS,   .scmd = SYS_SET_STARTUP_EFF_EN,   .func = on_sys_set_startup_eff_en, },
    { .cmd = CMD_SYS,   .scmd = SYS_SET_VOLT_UP_EN,     .func = on_sys_set_volt_up_en, },
    { .cmd = CMD_SYS,   .scmd = SYS_GET_NETBAR_MODE,    .func = on_sys_get_netbar_mode,},
    { .cmd = CMD_SYS,   .scmd = SYS_SET_NETBAR_MODE,    .func = on_sys_set_netbar_mode,},
    


    { .cmd = CMD_KC,    .scmd = KC_GET_SIZE,        .func = on_kc_get_kcm_max,        },
    { .cmd = CMD_KC,    .scmd = KC_GET_CUR_KCM,     .func = on_kc_get_kcm_index,      },
    { .cmd = CMD_KC,    .scmd = KC_SET_CUR_KCM,     .func = on_kc_set_kcm_index,      },
    { .cmd = CMD_KC,    .scmd = KC_GET_KC,          .func = on_kc_read_kc,            },
    { .cmd = CMD_KC,    .scmd = KC_SET_KC,          .func = on_kc_write_kc,           },
    { .cmd = CMD_KC,    .scmd = KC_SET_DEFAULT,     .func = on_kc_set_default,        },



#ifdef RGB_ENABLED
    { .cmd = CMD_KR,    .scmd = KR_GET_SIZE,        .func = on_kr_get_krm_max,        },
    { .cmd = CMD_KR,    .scmd = KR_GET_CUR_KRM,     .func = on_kr_get_krm_index,      },
    { .cmd = CMD_KR,    .scmd = KR_SET_CUR_KRM,     .func = on_kr_set_krm_index,      },
    { .cmd = CMD_KR,    .scmd = KR_GET_KR,          .func = on_kr_read_kr,            },
    { .cmd = CMD_KR,    .scmd = KR_SET_KR,          .func = on_kr_write_kr,           },
    { .cmd = CMD_KR,    .scmd = KR_SET_DEFAULT,     .func = on_kr_set_default,        },
#endif

    { .cmd = CMD_SK,    .scmd = SK_GET_HEADER,      .func = on_sk_read_header,        },
    { .cmd = CMD_SK,    .scmd = SK_GET_DKS,         .func = on_sk_read_dks,           },
    { .cmd = CMD_SK,    .scmd = SK_SET_DKS,         .func = on_sk_write_dks,          },
    { .cmd = CMD_SK,    .scmd = SK_GET_ADV,         .func = on_sk_read_adv,           },
    { .cmd = CMD_SK,    .scmd = SK_SET_ADV,         .func = on_sk_write_adv,          },
    { .cmd = CMD_SK,    .scmd = SK_GET_MACRO,       .func = on_sk_read_macro,         },
    { .cmd = CMD_SK,    .scmd = SK_SET_MACRO,       .func = on_sk_write_macro,        },
    { .cmd = CMD_SK,    .scmd = SK_GET_DKS_TICK,    .func = on_sk_read_dks_tick,      },
    { .cmd = CMD_SK,    .scmd = SK_SET_DKS_TICK,    .func = on_sk_write_dks_tick,     },
    { .cmd = CMD_SK,    .scmd = SK_SET_SOCD,        .func = on_sk_set_socd,           },
    { .cmd = CMD_SK,    .scmd = SK_GET_SNAP_TAP,    .func = on_sk_get_socd,           },
    { .cmd = CMD_SK,    .scmd = SK_SET_HT,          .func = on_sk_set_ht,             },
    { .cmd = CMD_SK,    .scmd = SK_GET_HT,          .func = on_sk_get_ht,             },
    { .cmd = CMD_SK,    .scmd = SK_SET_RS,          .func = on_sk_set_rs,             },
    { .cmd = CMD_SK,    .scmd = SK_GET_RS,          .func = on_sk_get_rs,             },

#ifdef RGB_ENABLED
    { .cmd = CMD_LA,    .scmd = KM_GET_SIZE,        .func = on_lm_get_lm_info,        },
    { .cmd = CMD_LA,    .scmd = KM_GET_KMM,         .func = on_lm_get_cur_lm,         },
    { .cmd = CMD_LA,    .scmd = KM_SET_KMM,         .func = on_lm_set_cur_lm,         },
    { .cmd = CMD_LA,    .scmd = KM_TOG_KMM,         .func = on_lm_toggle_lm,          },
    { .cmd = CMD_LA,    .scmd = KM_GET_KM,          .func = on_la_read_lm,            },
    { .cmd = CMD_LA,    .scmd = KM_SET_KM,          .func = on_la_write_lm,           },
    { .cmd = CMD_LA,    .scmd = KM_GET_LM_SIZE,     .func = on_la_get_la_info,        },
    { .cmd = CMD_LA,    .scmd = KM_GET_LM,          .func = on_la_read_la,            },
    { .cmd = CMD_LA,    .scmd = KM_SET_LM,          .func = on_la_write_la,           },
#endif

#ifdef LB_ENABLED
    { .cmd = CMD_LB_BACK,   .scmd = LB_BACK_GET_SATE,      .func = on_lb_back_get_state,          },
    { .cmd = CMD_LB_BACK,   .scmd = LB_BACK_SET_SATE,      .func = on_lb_back_set_state,          },
    { .cmd = CMD_LB_BACK,   .scmd = LB_BACK_GET_PARA,      .func = on_lb_back_get_para,           },
    { .cmd = CMD_LB_BACK,   .scmd = LB_BACK_SET_PARA,      .func = on_lb_back_set_para,           },
    { .cmd = CMD_LB_BACK,   .scmd = LB_BACK_SET_LIST_ID,   .func = on_lb_back_get_list_id,        },
    { .cmd = CMD_LB_BACK,   .scmd = LB_BACK_GET_SHAPE,     .func = on_lb_back_get_shape,          },
    { .cmd = CMD_LB_BACK,   .scmd = LB_BACK_SET_SHAPE,     .func = on_lb_back_set_shape,          },

    { .cmd = CMD_LB_SIDE,   .scmd = LB_SIDE_GET_SATE,      .func = on_lb_side_get_state,          },
    { .cmd = CMD_LB_SIDE,   .scmd = LB_SIDE_SET_SATE,      .func = on_lb_side_set_state,          },
    { .cmd = CMD_LB_SIDE,   .scmd = LB_SIDE_GET_PARA,      .func = on_lb_side_get_para,           },
    { .cmd = CMD_LB_SIDE,   .scmd = LB_SIDE_SET_PARA,      .func = on_lb_side_set_para,           },
    { .cmd = CMD_LB_SIDE,   .scmd = LB_SIDE_SET_LIST_ID,   .func = on_lb_side_get_list_id,        },
#endif

    { .cmd = CMD_KS,   .scmd = KS_SET_ACT_POINT,    .func = on_ks_set_act_point,      },
    { .cmd = CMD_KS,   .scmd = KS_GET_ACT_POINT,    .func = on_ks_get_act_point,      },
    { .cmd = CMD_KS,   .scmd = KS_SET_RT_PARA,      .func = on_ks_set_rt_para,        },
    { .cmd = CMD_KS,   .scmd = KS_GET_RT_PARA,      .func = on_ks_get_rt_para,        },
    { .cmd = CMD_KS,   .scmd = KS_SET_LOW_LATENCY,  .func = on_ks_set_low_latency,    },
    { .cmd = CMD_KS,   .scmd = KS_GET_LOW_LATENCY,  .func = on_ks_get_low_latency,    },
    { .cmd = CMD_KS,   .scmd = KS_SET_GAME_TICKRATE,.func = on_ks_set_tickrate,       },
    { .cmd = CMD_KS,   .scmd = KS_GET_GAME_TICKRATE,.func = on_ks_get_tickrate,       },
    { .cmd = CMD_KS,   .scmd = KS_SET_DKS_ACT_POINT,.func = on_ks_set_dks_act_point,  },
    { .cmd = CMD_KS,   .scmd = KS_GET_DKS_ACT_POINT,.func = on_ks_get_dks_act_point,  },
    { .cmd = CMD_KS,   .scmd = KS_SET_MT,           .func = on_ks_set_mt,             },
    { .cmd = CMD_KS,   .scmd = KS_GET_MT,           .func = on_ks_get_mt,             },
    { .cmd = CMD_KS,   .scmd = KS_SET_TGL,          .func = on_ks_set_tgl,            },
    { .cmd = CMD_KS,   .scmd = KS_GET_TGL,          .func = on_ks_get_tgl,            },
    { .cmd = CMD_KS,   .scmd = KS_SET_PS,           .func = on_ks_set_ps,             },
    { .cmd = CMD_KS,   .scmd = KS_GET_PS,           .func = on_ks_get_ps,             },
    { .cmd = CMD_KS,   .scmd = KS_SET_KEY_REPORT,   .func = on_ks_set_key_rpt,        },
    { .cmd = CMD_KS,   .scmd = KS_GET_KEY_REPORT,   .func = on_ks_get_key_rpt,        },
    { .cmd = CMD_KS,   .scmd = KS_SET_CALI,         .func = on_ks_set_cali,           },
    { .cmd = CMD_KS,   .scmd = KS_GET_CALI,         .func = on_ks_get_cali,           },
    { .cmd = CMD_KS,   .scmd = KS_GET_AP_RES,       .func = on_ks_get_ap_res,         },
    { .cmd = CMD_KS,   .scmd = KS_SET_DZ,           .func = on_ks_set_dz,             },
    { .cmd = CMD_KS,   .scmd = KS_GET_DZ,           .func = on_ks_get_dz,             },
    { .cmd = CMD_KS,   .scmd = KS_SET_PR,           .func = on_ks_set_pr,             },
    { .cmd = CMD_KS,   .scmd = KS_GET_PR,           .func = on_ks_get_pr,             },
    { .cmd = CMD_KS,   .scmd = KS_SET_NKRO,         .func = on_ks_set_nkro,           },
    { .cmd = CMD_KS,   .scmd = KS_GET_NKRO,         .func = on_ks_get_nkro,           },
    { .cmd = CMD_KS,   .scmd = KS_SET_TUNING,       .func = on_ks_set_turing,         },
    { .cmd = CMD_KS,   .scmd = KS_GET_TUNING,       .func = on_ks_get_turing,         },
    { .cmd = CMD_KS,   .scmd = KS_SET_SHAFT_TYPE,   .func = on_ks_set_shaft_type,     },
    { .cmd = CMD_KS,   .scmd = KS_GET_SHAFT_TYPE,   .func = on_ks_get_shaft_type,     },
    { .cmd = CMD_KS,   .scmd = KS_GET_WAVE_MAX,     .func = on_ks_get_wave_max,       },
    { .cmd = CMD_KS,   .scmd = KS_GET_RT_SEP,       .func = on_ks_get_rt_sep,         },
    { .cmd = CMD_KS,   .scmd = KS_SET_RT_SEP,       .func = on_ks_set_rt_sep,         },
    { .cmd = CMD_KS,   .scmd = KS_SET_TEST,         .func = on_ks_set_test,           },
    { .cmd = CMD_KS,   .scmd = KS_GET_TEST,         .func = on_ks_get_test,           },

    { .cmd = CMD_DPM,  .scmd = DPM_CUSTOM_PIC_DL,   .func = on_dpm_custom_pic_dl,     },
    { .cmd = CMD_DPM,  .scmd = DPM_DATA_SYNC,       .func = on_dpm_general_data_sync, },

    { .cmd = CMD_LS,    .scmd = SYNC_GET_CONFIG,    .func = on_lamp_get_config,         },
    { .cmd = CMD_LS,    .scmd = SYNC_GET_ARRAY,     .func = on_lamp_get_array,          },
    { .cmd = CMD_LS,    .scmd = SYNC_SET_DATA,      .func = on_lamp_set_data,           },
    { .cmd = CMD_LS,    .scmd = SYNC_GET_DATA,      .func = on_lamp_get_data,           },
    { .cmd = CMD_LS,    .scmd = SYNC_SET_PART,      .func = on_lamp_set_part,           },
    { .cmd = CMD_LS,    .scmd = SYNC_SET_MULTI,     .func = on_lamp_set_multi,          },
    { .cmd = CMD_LS,    .scmd = SYNC_SET_MODE,      .func = on_lamp_set_mode,          },

    { .cmd = CMD_DEBUG,.scmd = KEY_DBG_SW,          .func = on_key_dbg_sw,            },
    { .cmd = CMD_DEBUG,.scmd = KEY_DBG_ID,          .func = on_key_dbg_id,            },
    { .cmd = CMD_DEBUG,.scmd = KEY_DBG_RATE,        .func = on_key_dbg_rate,          },
    { .cmd = CMD_DEBUG,.scmd = KEY_LOG_CAIL_RPT,    .func = on_log_cail_dbg_set,      },

    { .cmd = CMD_FAC,  .scmd = FACTORY_SET_MODE,    .func = on_factory,               },
    { .cmd = CMD_FAC,  .scmd = FACTORY_GET_MODE,    .func = on_get_factory,           },
    { .cmd = CMD_FAC,  .scmd = FACTORY_SET_COLOR,   .func = on_factory_color,         },
    // { .cmd = CMD_FAC,  .scmd = FACTORY_GET_RSSI,    .func = on_factory_rssi,          },
    { .cmd = CMD_FAC,  .scmd = FACTORY_GET_VERIFY,  .func = on_firmware_crc,          },
    { .cmd = CMD_FAC,  .scmd = FACTORY_GET_TOUCH,  .func = on_factory_touch,          },
};


void hive_thread(void * arg)
{
    (void) arg;
    uint32_t e = 0;
    uint8_t *  recv_buf;
    uint16_t len = 0, reserve = 0, cs = 0;
    uint8_t  cmd = 0, scmd = 0, *pdata = NULL;
    bool b_found = false;
    hive_data_t event;
    DBG_PRINTF("hive_thread...\n");
    
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        while (!fifo_is_empty(&hive_data_fifo))
        {
            fifo_out(&hive_data_fifo, (uint8_t *)&event, 1);
            recv_buf = event.buf;

            cmd     = recv_buf[CMD_OFFSET_CMD];
            scmd    = recv_buf[CMD_OFFSET_SCMD]&0x7f;
            len     = *(uint16_t *)&recv_buf[CMD_OFFSET_LEN];
            reserve = *(uint16_t *)&recv_buf[CMD_OFFSET_RESERVE];
            cs      = *(uint16_t *)&recv_buf[CMD_OFFSET_CS];
            pdata   = &recv_buf[CMD_OFFSET_DATA];

#if 1
            {
                DBG_PRINTF("-->cmd=%02x scmd=%02x len=%02x reserve=%02x cs=%02x data=\"", cmd, scmd, len, reserve, cs);
                for (int i = 0; i < len && len <= 56; ++i)
                {
                    if (i != 0 && ((i % 8) == 0))
                    {
                        DBG_PRINTF(" | ");
                    }
                    DBG_PRINTF("%02x ", pdata[i]);
                }
                DBG_PRINTF("\"\n");
            }
#else
            DBG_PRINTF("-->cmd=%02x scmd=%02x len=%02x\n", cmd, scmd, len);

#endif
            if (len > 56)
            {
                DBG_PRINTF("cmd len err=%d,%d\n",len,len&0xff);
                goto END_LABEL;
                continue;
            }
            else if ((len > 0) && (cs != 0x00))
            {

                uint16_t crc_cal = crc16(pdata, len);
                if (cs != crc_cal)
                {
                    e = 1;
                    DBG_PRINTF("cmd crc err %x %x %d\n", cs, crc_cal, len);

                    goto END_LABEL;
                }

            }
            b_found = false;
            for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(cmd_table[0]); ++i)
            {
                if ((cmd_table[i].cmd == cmd) && (cmd_table[i].scmd == scmd))
                {
                    if(cmd_table[i].func != NULL)
                    {
                        e = cmd_table[i].func(len, reserve, pdata);
                        b_found = true;
                        ps_timer_reset_later();
                    }
                    else
                    {
                        DBG_PRINTF("cmd_table[%d] not found\r\n",i);
                        e = 2;
                    }
                    break;
                }
            }

        END_LABEL:
            if ( (0 != e) || (b_found == false))
            {
                //debugn(HIVE_DEBUG_LEVEL_ERROR, "e=0x%X", e);
                
                if (b_found == 0)
                {
                    e = 2;
                }
                DBG_PRINTF("cmd err=0x%x,%d\n", e,b_found);
                do_error_report(cmd, scmd, len, reserve, cs, e);
            } 
            
            if (muxt)
            {
                muxt--;
            }
        }
        
        if (muxt)
        {
            DBG_PRINTF("cmd muxt berr=%d\n", muxt);
            muxt = 0;
        }
    }
}


void hive_init(void)
{
    DBG_PRINTF("hive_init\n");
    hal_hid_set_out_callback(hid_recv_handler);
    
    fifo_init(&hive_data_fifo, hive_data_fifo_buf, HIV_FIFO_NUM * HIVE_FIFO_SIZE, HIVE_FIFO_SIZE);
    if (pdPASS != xTaskCreate(hive_thread, "hiv", STACK_SIZE_HIVE, NULL, hive_PRIORITY, &m_hive_thread))
    {
        DBG_PRINTF("hive_thread creat fali\n");
    }
}

