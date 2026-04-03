/*
 * Copyright (c) 2024-2025 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "mg_uart.h"
#include "board.h"
#include "easy_crc.h"
#include "easy_fifo.h"
#include "dfu.h"
#include "app_debug.h"
#include "app_config.h"
#include "db.h"
#include "dfu_slave.h"
#include "rgb_led_ctrl.h"
#include "hpm_gpio_drv.h"
#include "mg_indev.h"
#include "power_save.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "mg_uart"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif


#define RESPONSE

#define DPM_RETRIES_0               (0)
#define DPM_RETRIES_1               (1)
#define UART_FIFO_NUM               (2)

extern key_cali_t key_cali;
extern key_st_t key_st[];
extern kh_t kh;

static PFN_BOOT_CB g_boot_cb = NULL;

ATTR_PLACE_AT_NONCACHEABLE uint8_t uart_fifo_buf[UART_MAX][UART_FIFO_NUM][UART_BUFF_SIZE];

static TaskHandle_t m_rx_handle;
static TaskHandle_t m_tx_handle;
SemaphoreHandle_t xBinarySemaphore;
SemaphoreHandle_t xTxSemaphore;
QueueHandle_t xCommandQueue; // 指令队列
QueueHandle_t xResponseQueue[UART_DEVICE_DPM]; // 应答队列
QueueHandle_t xReplyQueue;
static TimerHandle_t timer_handle = NULL;
static TaskHandle_t m_sync_handle;

static fifo_t uart_fifo;
static uint8_t uart_node[UART_MAX][sizeof(fifo_node_t) * UART_FIFO_NUM] __attribute__((aligned(4)));
static uint8_t uart_fifo_group2_num = 0;
static uint8_t uart_fifo_group1_num[UART_DEVICE_DPM] = {0};
static uint8_t apex = 0;
static uint8_t last_lvl[BOARD_KEY_NUM] = {0};
static uint8_t data_buff[UART_BUFF_SIZE] = {0};
static volatile uint16_t hw_ver = INVAIL_UINT16,fw_ver = INVAIL_UINT16;

typedef struct __attribute__((packed))
{
    uint8_t key_index;
    uint16_t curr_stroke;
    uint16_t total_stroke;
    uint16_t trg_point;
    uint8_t rt_mode;
    uint8_t rt_ada_sens_extreme_mode;
    uint8_t rt_separately_en;
    uint8_t rt_press_sensitive;
    uint8_t rt_raise_sensitive;
    uint8_t trg_state;
} key_trg_info_t;

uint8_t *uart_get_general_buff(void)
{
    return data_buff;
}

uint32_t mg_uart_send_queue(uart_report_t *p_report)
{
    uint32_t ret = UART_SUCCESS;
    uintptr_t addr = (uintptr_t)p_report;

    if (xQueueSend(xCommandQueue, &addr, portMAX_DELAY) != pdTRUE)
    {
        return 0xff;
    }
    return ret;
}

static uint32_t uart_cmd_response(uint8_t cmd, uint8_t subcmd, uint8_t dir, uint8_t err, uint16_t len, uint8_t *pdata, uint8_t id, uint32_t timeout)
{
    uint32_t e = 0;
    static uart_report_t send_q[5];
    static uint8_t send_cnt = 0;
    screen_ret_t scr_ret = {0};

    memset(&send_q[send_cnt], 0, sizeof(uart_report_t));
    send_q[send_cnt].display.head.cmd = cmd;
    send_q[send_cnt].display.head.scmd = subcmd;
    send_q[send_cnt].display.head.dir = dir;
    send_q[send_cnt].display.head.len = len;
    send_q[send_cnt].display.head.errcode = err;
    send_q[send_cnt].display.report_id = id;
    send_q[send_cnt].display.report_size = HEAD_MAX + len + 2;

    if (pdata != NULL)
    {
        memcpy(send_q[send_cnt].display.data, pdata, len);
    }

    uint16_t crc = crc16((uint8_t *)&send_q[send_cnt].display.head, HEAD_MAX + len);
    *(uint16_t *)&send_q[send_cnt].display.data[len] = crc;

    if (xCommandQueue != NULL)
    {
        if (xTxSemaphore != NULL)
        {
            if (xSemaphoreTake(xTxSemaphore, pdMS_TO_TICKS(10)) != pdTRUE)
            {
                DBG_PRINTF("xTxSemaphore warning %d\r\n");
            }
        }
        uintptr_t addr = (uintptr_t)&send_q[send_cnt];
        if (xQueueSend(xCommandQueue, &addr, pdMS_TO_TICKS(100)) != pdTRUE)
        {
            // if (cmd.sem)
            // {
            //     vSemaphoreDelete(cmd.sem);
            // }
            return UART_ERROR_INVALID_PARAM;
        }
        send_cnt++;
        send_cnt %= 5;
    }
    else
    {
        return UART_ERROR_INVALID_PARAM;
    }

    if ((dir == DIR_ASK) && (timeout > 0))
    {
        while (xQueueReceive(xReplyQueue, &scr_ret, 0) == pdTRUE)
            ;
#ifdef RESPONSE
        if (xQueueReceive(xReplyQueue, &scr_ret, timeout) == pdTRUE)
        {
            e = scr_ret.ret_code;
        }
        else
        {
            e = UART_ERROR_CMD_TIMEOUT;
        }
#endif
    }

    return e;
}

static bool uart_response_queue(uint32_t ret_code)
{

    screen_ret_t ret = {0};
    ret.ret_code = ret_code;

#ifdef RESPONSE
    if (xQueueSend(xReplyQueue, &ret, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        return false;
    }
#endif
    return true;
}

static uint32_t uart_cmd_with_retry(uint8_t cmd, uint8_t scmd, uint8_t dir, uint8_t err, uint16_t len, uint8_t *buf, uint8_t device, uint32_t timeout, uint8_t retry_count)
{
    uint32_t result;

    do
    {
        result = uart_cmd_response(cmd, scmd, dir, err, len, buf, device, timeout);
        if (result != UART_ERROR_CMD_TIMEOUT)
        {
            break;
        }

    } while (retry_count--);

    return result;
}

static void uart_cond_cmd_do_response(uint8_t cmd, uint8_t scmd, uint8_t *pdata, uint16_t len)
{
    if (get_hive_state() != OFFLINE)
    {
        cmd_do_response(cmd, scmd, len, 0, pdata);
    }
}

/// @brief ask
uint32_t uart_set_general_data(uint8_t *buf, uint8_t len)
{
    return uart_cmd_with_retry(CMD_UART_GENERAL_DATA, SCMD_DATA_SYNC, DIR_ASK, 0, len, buf, UART_DEVICE_DPM, TIMEOUT_300MS, DPM_RETRIES_0);
}

uint32_t uart_get_screen_ver(void)
{
    return uart_cmd_with_retry(CMD_UART_SYS, SCMD_VER, DIR_ASK, 0, 0, NULL, UART_DEVICE_DPM, TIMEOUT_100MS, DPM_RETRIES_0);
}

uint32_t uart_set_screen_dfu(uint8_t *buf, uint8_t len)
{
    return uart_cmd_with_retry(CMD_UART_SYS, SCMD_DFU, DIR_ASK, 0, len, buf, UART_DEVICE_DPM, TIMEOUT_6S, DPM_RETRIES_0);
}

uint32_t uart_set_screen_picture(uint8_t *buf, uint8_t len)
{
    return uart_cmd_with_retry(CMD_UART_PICTURE, SCMD_DOWNLOAD, DIR_ASK, 0, len, buf, UART_DEVICE_DPM, TIMEOUT_1S, DPM_RETRIES_0);
}

uint32_t uart_ask_screen_cmd(uint8_t cmd, uint8_t scmd, uint8_t *buf, uint8_t len)
{
    return uart_cmd_with_retry(cmd, scmd, DIR_ASK, 0, len, buf, UART_DEVICE_DPM, TIMEOUT_100MS, DPM_RETRIES_0);
}

/// @brief response
static uint32_t uart_reply_screen_cmd(uint8_t cmd, uint8_t scmd, uint8_t errcode, uint8_t *buf, uint8_t len)
{
    return uart_cmd_with_retry(cmd, scmd, DIR_REPLY, errcode, len, buf, UART_DEVICE_DPM, TIMEOUT_100MS, DPM_RETRIES_0);
}

/// @brief notify
uint32_t uart_notify_screen_cmd(uint8_t cmd, uint8_t scmd, uint8_t *buf, uint8_t len)
{
    return uart_cmd_with_retry(cmd, scmd, DIR_NOTIFY, 0, len, buf, UART_DEVICE_DPM, TIMEOUT_NULL, DPM_RETRIES_0);
}

static void mg_set_param_with_dpm(uint8_t id, uint8_t data)
{

    uint8_t buf[6] = {0};
    uint8_t index = 0;

    kb_dpm_param_t set_info = KB_DPM_PARAM_GET();
    if (id > OPEN_EN_WEBSITE)
    {
        return;
    }

    if (id < BKL_BRI_TOG)
    {
        ((uint8_t *)&set_info)[id] = data;
    }

    switch (id)
    {
    case PROFILE_INDEX:
        if (set_info.profile_index > 3)
        {
            return;
        }
        common_info.cfg_info.cfg_idx = set_info.profile_index;
        db_set_with_dpm_refresh();
        db_send_toggle_cfg_signal();
        break;

    case BKL_MODE:
        if (set_info.bkl_mode == LED_ACTION_MAX - 1)
        {
            gbinfo.lm.cur_lmm = 0;
        }
        else
        {
            gbinfo.lm.cur_lmm = set_info.bkl_mode + 1;
        }
        db_set_with_dpm_refresh();
        db_update(DB_SYS, UPDATE_LATER);
        rgb_led_set_normal();

        buf[index++] = 0;
        buf[index++] = gbinfo.lm.cur_lmm;
        buf[index++] = sk_la_lm_kc_info.la_table[gbinfo.lm.cur_lmm].hl;
        uart_cond_cmd_do_response(CMD_NOTIFY, NTF_KM, buf, index);
        break;

    case BKL_BRI:
        sk_la_lm_kc_info.la_table[gbinfo.lm.cur_lmm].hl = set_info.bkl_bri * LED_BRIGHTNESS_MAX / 100;
        db_set_with_dpm_refresh();
        db_update(DB_SYS, UPDATE_LATER);
        rgb_led_set_normal();

        buf[index++] = 0;
        buf[index++] = gbinfo.lm.cur_lmm;
        buf[index++] = set_info.bkl_bri / 2;
        uart_cond_cmd_do_response(CMD_NOTIFY, NTF_KM, buf, index);
        break;

    case AMBL_BACK_MODE:
        if (set_info.ambl_back_mode == (RGBLIGHT_EFFECT_MAX - 1))
        {
            gbinfo.rl[LIGHT_TAPE_PART_BACK].state = 0;
        }
        else
        {
            gbinfo.rl[LIGHT_TAPE_PART_BACK].md = set_info.ambl_back_mode + 1;
            gbinfo.rl[LIGHT_TAPE_PART_BACK].state = 1;
        }
        db_set_with_dpm_refresh();
        db_update(DB_SYS, UPDATE_LATER);

        buf[index++] = gbinfo.rl[LIGHT_TAPE_PART_BACK].md;
        buf[index++] = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h;
        buf[index++] = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s;
        buf[index++] = VALUE_TO_PERCENT(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v, RGBLIGHT_LIMIT_VAL);
        buf[index++] = 9 - gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].speed;
        buf[index++] = gbinfo.rl[LIGHT_TAPE_PART_BACK].state;
        uart_cond_cmd_do_response(CMD_NOTIFY, NTF_LB, buf, index);
        break;

    case AMBL_BACK_BRI:
        gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v =
            set_info.ambl_back_bri * RGBLIGHT_LIMIT_VAL / 100;
        db_set_with_dpm_refresh();
        db_update(DB_SYS, UPDATE_LATER);

        buf[index++] = gbinfo.rl[LIGHT_TAPE_PART_BACK].md;
        buf[index++] = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h;
        buf[index++] = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s;
        buf[index++] = set_info.ambl_back_bri;
        buf[index++] = 9 - gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].speed;
        buf[index++] = gbinfo.rl[LIGHT_TAPE_PART_BACK].state;
        uart_cond_cmd_do_response(CMD_NOTIFY, NTF_LB, buf, index);
        break;
    case AMBL_SIDE_MODE:
        if (set_info.ambl_side_mode == (RGBLIGHT_EFFECT_MAX - 1))
        {
            gbinfo.rl[LIGHT_TAPE_PART_SIDE].state = 0;
        }
        else
        {
            gbinfo.rl[LIGHT_TAPE_PART_SIDE].md = set_info.ambl_side_mode + 1;
            gbinfo.rl[LIGHT_TAPE_PART_SIDE].state = 1;
        }
        db_set_with_dpm_refresh();
        db_update(DB_SYS, UPDATE_LATER);

        buf[index++] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].md;
        buf[index++] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h;
        buf[index++] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s;
        buf[index++] = VALUE_TO_PERCENT(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v, RGBLIGHT_LIMIT_VAL);
        buf[index++] = 9 - gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].speed;
        buf[index++] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].state;
        uart_cond_cmd_do_response(CMD_NOTIFY, NTF_LB_SIDE, buf, index);
        break;

    case AMBL_SIDE_BRI:

        gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v =
            set_info.ambl_side_bri * RGBLIGHT_LIMIT_VAL / 100;
        db_set_with_dpm_refresh();
        db_update(DB_SYS, UPDATE_LATER);

        buf[index++] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].md;
        buf[index++] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h;
        buf[index++] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s;
        buf[index++] = set_info.ambl_side_bri;
        buf[index++] = 9 - gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].speed;
        buf[index++] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].state;
        uart_cond_cmd_do_response(CMD_NOTIFY, NTF_LB_SIDE, buf, index);
        break;

    case BKL_BRI_TOG:
        mg_indev_handler(INEDV_EVT_BKL_BRI_TOG, NULL);
        break;

    case AMBL_BACK_BRI_TOG:
        mg_indev_handler(INEDV_EVT_AMB_BACK_BRI_TOG, NULL);
        break;

    case AMBL_SIDE_BRI_TOG:
        mg_indev_handler(INEDV_EVT_AMB_SIDE_BRI_TOG, NULL);
        break;

    case OPEN_CN_WEBSITE:
        mg_indev_handler(INEDV_EVT_OPEN_CN_WEBSITE, NULL);
        break;

     case OPEN_EN_WEBSITE:
        mg_indev_handler(INEDV_EVT_OPEN_EN_WEBSITE, NULL);
        break;

    default:
        break;
    }
}

static void uart_trig_process(bool enable)
{
    if (enable)
    {
        if (m_sync_handle)
        {
            vTaskResume(m_sync_handle);
        }
    }
    else
    {
        if (m_sync_handle)
        {
            vTaskSuspend(m_sync_handle);
        }
    }
}


static bool handle_version(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;

    if ((head.dir == DIR_REPLY) || (head.dir == DIR_NOTIFY))
    {
        if (head.errcode == UART_SLAVE_SUCCESS)
        {
            dfu_slave_fw_info_t info = {0};
            info.device_type = DFU_OBJ_DPM;
            info.fwv = *(uint16_t *)&data[HEAD_MAX];
            dfu_set_slave_fw_show(&info, sizeof(dfu_slave_fw_info_t));
            DBG_PRINTF("DFU_OBJ_DPM swv %d\r\n", info.fwv);
        }

        if (head.dir == DIR_REPLY)
        {
            return uart_response_queue(head.errcode);
        }
    }
    else if (head.dir == DIR_ASK)
    {
        dfu_slave_fw_info_t fw = {0};
        mg_nation_hw_fw_ver_get(&fw.hwv, &fw.fwv);
        uint8_t buf[8] = {0};
        *(uint16_t *)&buf[0] = BOARD_FW_VERSION;
        *(uint16_t *)&buf[2] = BOARD_HW_VERSION;
        *(uint16_t *)&buf[4] = fw.fwv;
        *(uint16_t *)&buf[6] = fw.hwv;
        uart_reply_screen_cmd(head.cmd, 0x7f & head.scmd, UART_SUCCESS, buf, sizeof(buf));
        uart_get_screen_ver();
    }
    return true;
}

static bool handle_reset(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;

    if (head.dir == DIR_REPLY)
    {
        if (head.errcode != UART_SUCCESS)
        {
            // DBG_PRINTF(" DIR_REPLY  ret %d\r\n", head.errcode);
        }

        return uart_response_queue(head.errcode);
    }
    return true;
}

static bool handle_restart(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;

    if (head.dir == DIR_REPLY)
    {
        if (head.errcode != UART_SUCCESS)
        {
            // DBG_PRINTF(" DIR_REPLY  ret %d\r\n", head.errcode);
        }

        return uart_response_queue(head.errcode);
    }
    return true;
}

static bool handle_dfu(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;

    if (head.dir == DIR_REPLY)
    {
        if (head.errcode != UART_SUCCESS)
        {
            head.errcode = (uint8_t)(head.errcode + MG_ERR_DEVICE_SLAVE);
            DBG_PRINTF(" receive dfu DIR_REPLY  ret %d\r\n", head.errcode);
        }

        return uart_response_queue(head.errcode);
    }
    return true;
}

static bool handle_picture_download(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;

    if (head.dir == DIR_REPLY)
    {
        return uart_response_queue(head.errcode);
    }
    return true;
}

static bool handle_picture_upload(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;

    if (head.dir == DIR_REPLY)
    {
        return uart_response_queue(head.errcode);
    }
    return true;
}

static bool handle_picture_delete(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;

    if (head.dir == DIR_REPLY)
    {
        return uart_response_queue(head.errcode);
    }
    return true;
}

static bool handle_set_dpm_mode(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;
    if (head.dir == DIR_REPLY)
    {
        return uart_response_queue(head.errcode);
    }

    return true;
}

static bool handle_get_dpm_mode(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;
    if ((head.dir == DIR_ASK) || (head.dir == DIR_NOTIFY))
    {
        if (head.errcode == UART_SLAVE_SUCCESS)
        {
            ps_timer_reset_later();
        }
        if (head.dir == DIR_ASK)
        {
            uart_reply_screen_cmd(head.cmd, 0x7f & head.scmd, head.errcode, NULL, 0);
        }
    }

    return true;
}

static bool handle_kb_param(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;

    if (head.dir == DIR_REPLY)
    {
    }
    else if ((head.dir == DIR_ASK) || (head.dir == DIR_NOTIFY))
    {
        kb_dpm_param_t curr_param = {0};
        if (head.errcode == UART_SLAVE_SUCCESS)
        {
            curr_param = KB_DPM_PARAM_GET();
        }

        if (head.dir == DIR_ASK )
        {
            uart_reply_screen_cmd(head.cmd, 0x7f & head.scmd, head.errcode, (uint8_t *)&curr_param, sizeof(kb_dpm_param_t));
        }
        
    }
    return true;
}
static bool  handle_set_kb_param(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;

    if (head.dir == DIR_REPLY)
    {
    }
    else if ((head.dir == DIR_ASK) || (head.dir == DIR_NOTIFY))
    {
        if (head.errcode == UART_SLAVE_SUCCESS)
        {
            prompt_type_e type;
            rgb_sm_state_e state;
            rgb_led_get_prompt(&state, &type);
            if (((state == STATE_PROMPT) && (type >= PROMPT_DFU_SLAVE_FLASH_DOWNLOAD))
             && ((state == STATE_PROMPT) && (type <= PROMPT_DFU_SLAVE_ABORT)))
            {
                head.errcode = ERR_SCR_INVAL;
                uart_reply_screen_cmd(head.cmd, 0x7f & head.scmd, head.errcode, NULL, 0);
            }
            else if (head.len >= 2)
            {
                for (uint8_t i = 0; 2 * i < head.len; i++)
                {
                    mg_set_param_with_dpm(data[HEAD_MAX + 2 * i], data[HEAD_MAX + 2 * i + 1]);
                }
            }
        }
        if (head.dir == DIR_ASK)
        {
            uart_reply_screen_cmd(head.cmd, 0x7f & head.scmd, head.errcode, NULL, 0);
        }
    }
    return true;
}

static bool handle_set_trigger_mode(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;

    if ((head.dir == DIR_ASK) || (head.dir == DIR_NOTIFY))
    {
        if (head.errcode == UART_SLAVE_SUCCESS)
        {
            if (data[HEAD_MAX])
            {
                uart_trig_process(1);
            }
            else
            {
                uart_trig_process(0);
            }
        }
        if (head.dir == DIR_ASK)
        {
            uart_reply_screen_cmd(head.cmd, 0x7f & head.scmd, head.errcode, NULL, 0);
        }
    }
    return true;
}

static bool handle_set_key_trigger(uint8_t *data, uint8_t len)
{
    (void)len;
    (void)data;

    return true;
}

static bool handle_custom_key_report(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;

    if (head.dir == DIR_REPLY)
    {
    }
    else if ((head.dir == DIR_ASK) || (head.dir == DIR_NOTIFY))
    {
        if (head.errcode == UART_SLAVE_SUCCESS)
        {
            mg_indev_handler(INEDV_EVT_CUSTOM_KEY_RPT, &data[HEAD_MAX]);
        }
        if (head.dir == DIR_ASK)
        {
            uart_reply_screen_cmd(head.cmd, 0x7f & head.scmd, head.errcode, NULL, 0);
        }
    }
    return true;
}

static bool handle_set_key_report(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;

    if (head.dir == DIR_REPLY)
    {
        
    }
    else if ((head.dir == DIR_ASK) || (head.dir == DIR_NOTIFY))
    {
        if (head.errcode == UART_SLAVE_SUCCESS)
        {
            mg_indev_handler(INEDV_EVT_KEY_CODE_RPT, &data[HEAD_MAX]);
        }
        if (head.dir == DIR_ASK)
        {
            uart_reply_screen_cmd(head.cmd, 0x7f & head.scmd, head.errcode, NULL, 0);
        }
    }
    return true;
}

static bool handle_general_data_sync(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;

    if (head.dir == DIR_REPLY)
    {
        if (head.len > 0)
        {
            data_buff[0] = head.len;
            memcpy(&data_buff[1], &data[HEAD_MAX], head.len);
        }
        else
        {
            memset(data_buff, 0, sizeof(data_buff));
        }
        
        return uart_response_queue(head.errcode);
    }
    else if ((head.dir == DIR_ASK) || (head.dir == DIR_NOTIFY))
    {
        if (head.errcode == UART_SLAVE_SUCCESS)
        {
            uart_cond_cmd_do_response(CMD_NOTIFY, NTF_DPM_SYNC, &data[HEAD_MAX], head.len);
        }
        if (head.dir == DIR_ASK)
        {
            uart_reply_screen_cmd(head.cmd, 0x7f & head.scmd, head.errcode, NULL, 0);
        }
    }
    return true;
}

static bool handle_factory_mode(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;

    if (head.dir == DIR_REPLY)
    {
        return uart_response_queue(head.errcode);
    }
    return true;
}

static bool handle_factory_color(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;

    if (head.dir == DIR_REPLY)
    {
        return uart_response_queue(head.errcode);
    }
    return true;
}

static bool handle_factory_touch(uint8_t *data, uint8_t len)
{
    (void)len;
    cmd_head_t head = *(cmd_head_t *)data;

    if (head.dir == DIR_REPLY)
    {
        if (head.errcode == UART_SLAVE_SUCCESS)
        {
            head.errcode = data[HEAD_MAX];
        }

        return uart_response_queue(head.errcode);
    }
    return true;
}
static bool handle_error(uint8_t *data, uint8_t len)
{
    (void)len;
    (void)data;

    DBG_PRINTF("crc err\r\n");

    return true;
}

static const command_entry command_table[] = {
    {CMD_UART_SYS,          SCMD_VER,              handle_version},
    {CMD_UART_SYS,          SCMD_RESET,            handle_reset},
    {CMD_UART_SYS,          SCMD_RESTART,          handle_restart},
    {CMD_UART_SYS,          SCMD_DFU,              handle_dfu},
    {CMD_UART_PICTURE,      SCMD_DOWNLOAD,         handle_picture_download},
    {CMD_UART_PICTURE,      SCMD_UPLOAD,           handle_picture_upload},
    {CMD_UART_PICTURE,      SCMD_DELETE,           handle_picture_delete},
    {CMD_UART_DPM_MODE,     SCMD_SET_DPM_INFO,     handle_set_dpm_mode},
    {CMD_UART_DPM_MODE,     SCMD_GET_DPM_INFO,     handle_get_dpm_mode},
    {CMD_UART_KB_PARAM,     SCMD_GET_KB_INFO,      handle_kb_param},
    {CMD_UART_KB_PARAM,     SCMD_SET_KB_INFO,      handle_set_kb_param},
    {CMD_UART_KEY_TRIGGER,  SCMD_SET_TRIGGER_MODE, handle_set_trigger_mode},
    {CMD_UART_KEY_TRIGGER,  SCMD_SET_KEY_TRIGGER,  handle_set_key_trigger},
    {CMD_UART_KEY_REPORT,   SCMD_GET_CUSTOM_KEY_REPORT,    handle_custom_key_report},
    {CMD_UART_KEY_REPORT,   SCMD_GET_KB_REPORT,    handle_set_key_report},
    {CMD_UART_GENERAL_DATA, SCMD_DATA_SYNC,        handle_general_data_sync},

    {CMD_UART_FACTORY,      SCMD_FAC_MODE,         handle_factory_mode},
    {CMD_UART_FACTORY,      SCMD_FAC_COLOR,        handle_factory_color},
    {CMD_UART_FACTORY,      SCMD_FAC_TOUCH,        handle_factory_touch},
    {CMD_UART_ERR,          0,                     handle_error},
};

static const size_t command_table_size = sizeof(command_table) / sizeof(command_table[0]);

bool receive_dpm_analyze(uint8_t *data, uint16_t len)
{
    bool b_seach = 0;

    if ((len < (sizeof(cmd_head_t) + 2)) || (len > UART_BUFF_SIZE))
    {
        DBG_PRINTF(" len err\r\n");
        return false;
    }

    uint8_t receive_data[UART_BUFF_SIZE] = {0};
    memcpy(receive_data, data, len);

    cmd_head_t head = {0};
    memcpy(&head, receive_data, sizeof(cmd_head_t));

    uint16_t receive_crc = *(uint16_t *)&receive_data[len - 2];
    if (receive_crc != 0)
    {
        uint16_t crc = crc16(data, len - 2);
        if (crc != receive_crc)
        {
            DBG_PRINTF(" crc err %x   %x\r\n", crc, receive_crc);
            uart_reply_screen_cmd(CMD_UART_ERR, 0, ERR_SCR_CRC, (uint8_t *)&receive_data, len);
            return false;
        }
    }

    for (size_t i = 0; i < command_table_size; i++)
    {
        const command_entry *entry = &command_table[i];

        if (entry->cmd == head.cmd && (entry->scmd == (head.scmd & 0x7f)))
        {
            entry->handler(receive_data, len);
            b_seach = 1;
        }
    }

    if (b_seach == 0)
    {
        uart_reply_screen_cmd(head.cmd, head.scmd & 0x7f, ERR_SCR_ERROR, (uint8_t *)&head, HEAD_MAX);
        return false;
    }

    return true;
}

static void uart_sync_handle(void *pvParameters)
{
    (void)pvParameters;
    while (1)
    {
        {
            key_st_t *pkst = NULL;
            uint8_t id = 0;
            bool notify = false;
            for (uint8_t ki = 0; ki < BOARD_KEY_NUM; ki++)
            {
                pkst = &key_st[ki];
                if (pkst->cur_lvl != last_lvl[ki])
                {
                    id = ki;
                    last_lvl[ki] = pkst->cur_lvl;
                    notify = true;
                    // DBG_PRINTF(" id %d   %d\r\n",id,notify);
                }
            }
            if (notify)
            {
                key_trg_info_t key_trg = {0};
                key_attr_t *pkattr = kh.p_attr + id;
                pkst = &key_st[id];
                key_trg.key_index = id;
                key_trg.curr_stroke = (pkst->cur_lvl > sw_config_table[key_cali.sw[id]].nacc2lvl)
                                          ? (sw_config_table[key_cali.sw[id]].nacc2lvl * 10 + (pkst->cur_lvl - sw_config_table[key_cali.sw[id]].nacc2lvl))
                                          : (pkst->cur_lvl * 10);
                key_trg.total_stroke = (uint16_t)((sw_config_table[key_cali.sw[id]].nacc2dist + sw_config_table[key_cali.sw[id]].hacc2dist) * 100);
                key_trg.trg_point = (pkattr->triggerlevel > sw_config_table[key_cali.sw[id]].nacc2lvl)
                                        ? (sw_config_table[key_cali.sw[id]].nacc2lvl * 10 + (pkattr->triggerlevel - sw_config_table[key_cali.sw[id]].nacc2lvl))
                                        : (pkattr->triggerlevel * 10);
                key_trg.rt_press_sensitive = pkattr->rtPressSensitive;
                key_trg.rt_raise_sensitive = pkattr->rtRaiseSensitive;
                key_trg.rt_mode = pkattr->rt_en ? pkattr->rt_mode : 0xFF;
                key_trg.rt_separately_en = kh.p_qty->rt_separately_en;
                key_trg.rt_ada_sens_extreme_mode = pkattr->rt_ada_extreme ? 0x80 : pkattr->rt_ada_sens;
                key_trg.trg_state = (pkst->trg_logic == KEY_STATUS_U2U) ? 0 : 1;

                notify = false;

                uart_cond_cmd_do_response(CMD_NOTIFY, NTF_KEY_TRIGGER_SHOW, (uint8_t *)&key_trg, sizeof(key_trg));
                uart_notify_screen_cmd(CMD_UART_KEY_TRIGGER, SCMD_SET_KEY_TRIGGER, (uint8_t *)&key_trg, sizeof(key_trg));
            }
        }
        vTaskDelay(30);
    }
}

static void mg_nation_hw_fw_ver_update(uint16_t hw_version, uint16_t fw_version)
{
    hw_ver = hw_version;
    fw_ver = fw_version;
}

bool mg_nation_hw_fw_ver_get(uint16_t *hw_version, uint16_t *fw_version)
{
    if (hw_version == NULL || fw_version == NULL)
    {
        return false;
    }

    *hw_version = hw_ver;
    *fw_version = fw_ver;

    return true;
}

// 发送接口
uint32_t uart_send_to_nation_cmd(uint8_t id, uint16_t cmd, uint16_t data_len, uint8_t dir, uint8_t *pdata, uint16_t frame_len, uint32_t timeout)
{
    uint32_t e = 0;
    static uart_report_t send_q[5];
    static uint8_t send_cnt = 0;
    slave_ret_t scr_ret = {0};

    memset(&send_q[send_cnt], 0, sizeof(uart_report_t));
    send_q[send_cnt].sm_ctl.head.cmd = (uint8_t)(cmd >> 8);
    send_q[send_cnt].sm_ctl.head.scmd = (uint8_t)cmd;
    send_q[send_cnt].sm_ctl.head.dir = dir;
    send_q[send_cnt].sm_ctl.head.reserve = 0;
    send_q[send_cnt].sm_ctl.head.len = data_len;
    send_q[send_cnt].sm_ctl.head.crc = crc16(pdata, data_len);
    send_q[send_cnt].sm_ctl.report_id = id;
    send_q[send_cnt].sm_ctl.report_size = frame_len;

    if (pdata != NULL)
    {
        memcpy(send_q[send_cnt].sm_ctl.data, pdata, data_len);
    }
    uintptr_t addr = (uintptr_t)&send_q[send_cnt];
    if (xQueueSend(xCommandQueue, &addr, portMAX_DELAY) != pdTRUE)
    {
        DBG_PRINTF(" send api xQueueSend error\r\n");
        return 0xff;
    }
    send_cnt++;
    send_cnt %= 5;

    if (dir == DIR_ASK)
    {
#ifdef RESPONSE

        if (xQueueReceive(xResponseQueue[id], &scr_ret, timeout) == pdTRUE)
        {
            // DBG_PRINTF(" ask  ret %04x\r\n", scr_ret.ret_code);
            e = scr_ret.ret_code;
        }
        else
        {
            DBG_PRINTF(" UART_ERROR_CMD_TIMEOUT\r\n");
            e = UART_ERROR_CMD_TIMEOUT;
        }
#endif
    }

    return e;
}



// 发送接口
uint32_t mg_uart_send_to_single_nation_request_cmd(uint8_t id, uint16_t cmd, uint16_t data_len, uint8_t *pdata, uint8_t retry_times)
{
    if (g_boot_cb != NULL)
    {
        return UART_ERROR_INVALID_PARAM;
    }
    
    uint32_t ret = CMD_FAIL;
    uint16_t frame_len = SM_CMD_MIN_SIZE + data_len;
    for(uint8_t i = 0; i< retry_times; i++)
    {
        if (CMD_SUCCESS == uart_send_to_nation_cmd(id, cmd, data_len, DIR_ASK, pdata, frame_len, TIMEOUT_300MS))
        {
            ret = CMD_SUCCESS;
            break;
        }
    }
    return ret;
}

uint32_t mg_uart_send_to_all_nation_request_cmd(uint16_t cmd, uint16_t data_len, uint8_t *pdata)
{
    uint32_t ret = CMD_SUCCESS;

    for (uint8_t i = 0; i < UART_DEVICE_DPM; i++)
    {
        ret = mg_uart_send_to_single_nation_request_cmd(i, cmd, data_len,pdata,UART_RETRY_TIMES);

        if (ret != CMD_SUCCESS)
        {
            DBG_PRINTF("uart i :%d \r\n", i);
            return ret;
        }
    }
    return ret;
}

// 接收接口
bool receive_nation_cmd_handle(uint8_t id, uint8_t *frame, uint16_t frame_len)
{
    uint32_t ret = true;
    slave_ret_t ack = {0};
    uint16_t indx = 0;
    uint8_t cmd = 0, scmd = 0;
    uint16_t data_len = 0;
    uint16_t rev_crc = 0;
    uint8_t pdata[UART_SM_CMD_BUF_SIZE] = {0};
    uint16_t curr_hw_ver = INVAIL_UINT16,curr_fw_ver = INVAIL_UINT16;

    if ((NULL == frame) || (frame_len < SM_CMD_MIN_SIZE))
    {
        DBG_PRINTF(" frame is null or frame_len error\r\n");
        return false;
    }

    memset(pdata, 0, UART_SM_CMD_BUF_SIZE);
    cmd = frame[indx++];
    scmd = frame[indx++];
    indx += 1; //dir
    data_len = frame[indx++];
    data_len |= (uint16_t)(frame[indx++] << 8);
    indx += 2; //reserve
    rev_crc = frame[indx++]; 
    rev_crc |= (uint16_t)(frame[indx++] << 8);

    do
    {
        if (data_len > UART_SM_CMD_BUF_SIZE)
        {
            DBG_PRINTF(" frame payload len error len :%d cmd :%d,scmd : %d \r\n",data_len,cmd,scmd);
            ret = false;
            break;
        }

        for (uint8_t i = 0; i < data_len; i++)
        {
            pdata[i] = frame[SM_CMD_MIN_SIZE + i];
        }

        if(data_len > 0)
        {
            uint16_t crc = crc16(pdata, data_len);
            if (rev_crc != crc)
            {
                DBG_PRINTF(" cmd :%d ,scmd : %d frame crc error\r\n",cmd,scmd);
                ret = false;
                break;
            }
        }

        uint16_t all_cmd = (uint16_t)(cmd << 8) | scmd;
        // DBG_PRINTF("cmd:%02x,scmd:%02x\r\n", cmd, scmd);

        switch (all_cmd)
        {
        case SM_GET_VER_CMD:
            
            curr_fw_ver = *(uint16_t*)&pdata[0];
            curr_hw_ver = *(uint16_t*)&pdata[2];
            DBG_PRINTF("sm_fw_version : %d,%d\r\n", curr_fw_ver,curr_hw_ver);

            if((curr_fw_ver != 0) && (curr_fw_ver != INVAIL_UINT16)\
             && (curr_hw_ver != 0) && (curr_hw_ver != INVAIL_UINT16))
            {
                mg_nation_hw_fw_ver_update(curr_hw_ver,curr_fw_ver);
            }
            else
            {
                DBG_PRINTF("result : SM_GET_VER_CMD error\r\n");
                mg_nation_hw_fw_ver_update(INVAIL_UINT16,INVAIL_UINT16);
                ret = false;
            }
            break;
        case SM_SYS_RESET_CMD:
            break;
        case SM_SYS_RESTART_CMD:
            break;
        case SM_GET_THRESHOLD_CMD:
            break;
        case SM_SET_THRESHOLD_CMD:
            if (pdata[0] == CMD_FAIL)
            {
                DBG_PRINTF("result : THRESHOLD CMD set fail\r\n");
                ret = false;
            }
            break;
        case SM_GET_KED_MODE_CMD:
            DBG_PRINTF("\r\n");
            break;
        case SM_SET_KEY_MODE_CMD:
            if (pdata[0] == CMD_FAIL)
            {
                DBG_PRINTF("result : SET KEY MODE fail\r\n");
                ret = false;
            }
            break;
        case SM_GET_LOWPWR_TIME_CMD:
            DBG_PRINTF("\r\n");
            break;
        case SM_SET_LOWPWR_TIME_CMD:
            if (pdata[0] == CMD_FAIL)
            {
                DBG_PRINTF("result : SET_LOWPWR_TIME fail\r\n");
                ret = false;
            }
            break;
        case SM_GET_LOWPWR_FREQ_CMD:
            DBG_PRINTF("\r\n");
            break;
        case SM_SET_LOWPWR_FREQ_CMD:
            if (pdata[0] == CMD_FAIL)
            {
                DBG_PRINTF("result : SET_LOWPWR_FREQ fail\r\n");
                ret = false;
            }
            break;
        case SM_GET_HALL_STABLE_TIME_CMD:
            DBG_PRINTF("\r\n");
            break;
        case SM_SET_HALL_STABLE_TIME_CMD:
            if (pdata[0] == CMD_FAIL)
            {
                DBG_PRINTF("result : SET_HALL_STABLE_TIME fail\r\n");
                ret = false;
            }
            break;
        case SM_GET_ADC_CH_INFO_CMD:
            DBG_PRINTF("\r\n");
            break;
        case SM_SET_ADC_CH_INFO_CMD:
            if (pdata[0] == CMD_FAIL)
            {
                DBG_PRINTF("result : SET_HALL_STABLE_TIME fail\r\n");
                ret = false;
            }
            break;
        case SM_REPORT_ERR_INFO_CMD:
            if((pdata[0] & SM_ERROR_MASK) || (pdata[0] == 0))
            {
                sys_run.sys_err_status = pdata[0];
                DBG_PRINTF(" sm error num : %d\r\n",pdata[0]);
            }
            break;
        case SM_REPORT_HEARTBEAT_INFO_CMD:
            break;
        case SM_SET_SYN_BOT_CAIL_CMD:
            if (pdata[0] == CMD_FAIL)
            {
                DBG_PRINTF("result : SM_SET_SYN_BOT_CAIL_CMD fail\r\n");
                ret = false;
            }
            break;
        case SM_SET_AUTO_CAIL_CTRL_CMD:
            if (pdata[0] == CMD_FAIL)
            {
                DBG_PRINTF("result : SM_SET_AUTO_CAIL_CTRL_CMD fail\r\n");
                ret = false;
            }
            break;
        case SM_SET_CH_MAX_CMD:
            if (pdata[0] == CMD_FAIL)
            {
                DBG_PRINTF("result : SM_SET_CH_MAX_CMD fail\r\n");
                ret = false;
            }
            break;
        default:
            ret = false;
            DBG_PRINTF(" find cmd fault ret \r\n");
            break;
        }

        if ((all_cmd != SM_REPORT_HEARTBEAT_INFO_CMD) && (all_cmd != SM_REPORT_ERR_INFO_CMD))
        {
            ack.ret_code = (ret == true) ? CMD_SUCCESS : CMD_FAIL;
            if (xQueueSend(xResponseQueue[id], &ack, portMAX_DELAY) != pdTRUE)
            {
                DBG_PRINTF("xQueueSend error\r\n");
                return false;
            }
            else
            {
                // DBG_PRINTF("xQueueSend send ok\r\n");
            }
        }

    } while (0);

    return ret;
}

/////////////////////////////////////////

static void uart_rx_handle(void *pvParameters)
{
    (void)pvParameters;
    fifo_node_t event;
    uint8_t *recv_buf = NULL;

    while (1)
    {
        uint32_t buff = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint32_t index = (buff >> 8) & 0xff;
        // uint32_t data_count = buff & 0xff;
        // uint32_t rx_len = UART_BUFF_SIZE - data_count;


        while (!fifo_is_empty(&uart_fifo))
        {
            fifo_out(&uart_fifo, (uint8_t *)&event, 1);
            recv_buf = event.buf;
            index = event.id;

            if (index == UART_DEVICE_DPM)
            {
                receive_dpm_analyze(recv_buf, event.size);
            }
            else
            {
                if (g_boot_cb != NULL)
                {
                    g_boot_cb(recv_buf, event.size);
                }
                else
                {
                    receive_nation_cmd_handle(index, recv_buf, event.size);
                }
            }
#ifdef DATA_DETAIL
            DBG_PRINTF("id %d rx %d bytes  ", index, event.size);
            for (uint32_t i = 0; i < event.size; i++)
            {
                DBG_PRINTF("%x ", recv_buf[i]);
            }
            DBG_PRINTF("\n");
#endif

            if (apex)
            {
                apex--;
            }
        }

        if (apex)
        {
            DBG_PRINTF("cmd apex berr=%d\n", apex);
            apex = 0;
        }
        //  vTaskDelay(1000);
    }
}

static void uart_tx_handle(void *pvParameters)
{
    (void)pvParameters;

    uart_report_t* report = NULL;
    while (1)
    {
        if (xQueueReceive(xCommandQueue, &report, portMAX_DELAY) == pdTRUE)
        {

            // xSemaphoreTake(xUartMutex, portMAX_DELAY);
#ifdef DATA_DETAIL
            DBG_PRINTF("id %d tx  len %d bytes  data :", report->comm.report_id, report->comm.report_size);
            for (uint32_t i = 0; i < report->comm.report_size; i++)
            {
                DBG_PRINTF("%02x ", report->comm.data[i]);
            }
            DBG_PRINTF("\n");
#endif

            hal_uart_start_dma_tx(report->comm.report_id, report->comm.data, report->comm.report_size);

            if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE)
            {
                // DBG_PRINTF("id %d tx: dma finished...\n", report->comm.report_id);
            }
            else
            {
                DBG_PRINTF( "tx: dma take semp...\n");
            }

            // xSemaphoreGive(xUartMutex);

        }

        // vTaskDelay(5000);
    }
}

static void tx_hal_idle(uint8_t index, uint8_t len)
{
    (void)index;
    (void)len;
    // BOARD_TEST_PIN1_TOGGLE();
    if (xTxSemaphore != NULL)
    {
        if (xPortIsInsideInterrupt() == pdTRUE)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(xTxSemaphore, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        else
        {
            xSemaphoreGive(xTxSemaphore);
        }
    }
}

static void dma_tx_idle(void)
{
    // BOARD_TEST_PIN0_TOGGLE();
    if (xBinarySemaphore != NULL)
    {
        if (xPortIsInsideInterrupt() == pdTRUE) 
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        else
        {
            xSemaphoreGive(xBinarySemaphore);
        }
    }
}

static void rx_hal_idle(uint8_t index, uint8_t len)
{
    // BOARD_TEST_PIN1_TOGGLE();
    uint32_t ulValue = 0;// ((uint32_t)index << 8) | len;

    if (index > UART_DEVICE_MAX)
    {
        DBG_PRINTF("index err=%d\n", index);
    }
    else if (len < 2)
    {
        DBG_PRINTF("len err=%d\n", len);
    }

    fifo_node_t event;
    /* configure new dma transmission */
    if (index == UART_DEVICE_DPM)
    {
        event.buf = uart_fifo_buf[UART_DPM][uart_fifo_group2_num];
        uart_fifo_group2_num++;
        uart_fifo_group2_num %= UART_FIFO_NUM;
        hal_uart_start_dma_rx(index, uart_fifo_buf[UART_DPM][uart_fifo_group2_num], UART_BUFF_SIZE);
    }
    else
    {
        event.buf = uart_fifo_buf[UART_SM][uart_fifo_group1_num[index]];
        uart_fifo_group1_num[index]++;
        uart_fifo_group1_num[index] %= UART_FIFO_NUM;
        hal_uart_start_dma_rx(index, uart_fifo_buf[UART_SM][uart_fifo_group1_num[index]], UART_BUFF_SIZE);
    }
    event.size = UART_BUFF_SIZE - len;
    event.id = index;

    if (!fifo_is_full(&uart_fifo))
    {
        apex++;
        fifo_in(&uart_fifo, (uint8_t *)&event, 1);
    }
    else
    {
        DBG_PRINTF("uart_fifo is full \r\n");
    }

    if (m_rx_handle != NULL)
    {
        if (xPortIsInsideInterrupt() == pdTRUE)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xTaskNotifyFromISR(
                m_rx_handle,
                ulValue,
                eSetValueWithoutOverwrite,
                &xHigherPriorityTaskWoken);

            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        else
        {
            xTaskNotify(
                m_rx_handle,
                ulValue,
                eSetValueWithoutOverwrite);
        }
    }

}

static void dma_rx_idle(void)
{
    
}

static void timer_callback(TimerHandle_t xTimer)
{
    (void)xTimer;
    DBG_PRINTF( "start checkout\n");

}

void mg_uart_firmware_dispatch_start(uint32_t timeout)
{
    if (timer_handle == NULL)
    {
        timer_handle = xTimerCreate("Timer", pdMS_TO_TICKS(timeout), pdFALSE, 0, timer_callback);
        if (timer_handle == NULL)
        {
            DBG_PRINTF("RGB timer creation failure\r\n");
        }
    }

    xTimerStart(timer_handle, 0);
}

uint32_t uart_master_set_slave_dfu(void)
{
    mg_uart_firmware_dispatch_start(1000);
    return 0;
}

void hal_boot_data_cb(PFN_BOOT_CB cb)
{
    g_boot_cb = cb;
}

void mg_uart_init(void)
{
    hal_uart_init();

    mg_uart_tran_init();

    for (uart_device_index_t i = 0; i < UART_DEVICE_MAX; i++)
    {
        hal_uart_register_callback(i, UART_DMA_TX, tx_hal_idle, dma_tx_idle);
        hal_uart_register_callback(i, UART_DMA_RX, rx_hal_idle, dma_rx_idle);
        if (UART_DEVICE_DPM == i)
        {
            hal_uart_start_dma_rx(i, uart_fifo_buf[UART_DPM][uart_fifo_group2_num], UART_BUFF_SIZE);
        }
        else
        {
            hal_uart_start_dma_rx(i, uart_fifo_buf[UART_SM][uart_fifo_group1_num[i]], UART_BUFF_SIZE);
        }
    }

    fifo_init(&uart_fifo, (uint8_t *)uart_node, sizeof(uart_node), sizeof(fifo_node_t));

    xCommandQueue = xQueueCreate(10, sizeof(uintptr_t));
    xReplyQueue = xQueueCreate(5, sizeof(screen_ret_t));
    for (int i = 0; i < UART_DEVICE_DPM; i++)
    {
        xResponseQueue[i] = xQueueCreate(2, sizeof(slave_ret_t));
    }

    xBinarySemaphore = xSemaphoreCreateBinary();
    if (xBinarySemaphore == NULL)
    {
        DBG_PRINTF("semap create error\n");
    }
    // else
    // {
    //     xSemaphoreGive(xBinarySemaphore);
    // }
    xTxSemaphore = xSemaphoreCreateBinary();
    if (xTxSemaphore == NULL)
    {
        DBG_PRINTF("semapTx create error\n");
    }
    else
    {
        xSemaphoreGive(xTxSemaphore);
    }

    // RX
    if (xTaskCreate(uart_rx_handle, "rx_handle", STACK_SIZE_UART, NULL, PRIORITY_UART, &m_rx_handle) != pdPASS)
    {
        DBG_PRINTF("uart create error\n");
    }

    // TX
    if (xTaskCreate(uart_tx_handle, "tx_handle", STACK_SIZE_UART, NULL, PRIORITY_UART, &m_tx_handle) != pdPASS)
    {
        DBG_PRINTF("tx create error\n");
    }

    if (xTaskCreate(uart_sync_handle, "uart_sync_handle", STACK_SIZE_SYNC, NULL, PRIORITY_SYNC, &m_sync_handle) != pdPASS)
    {
        DBG_PRINTF(" create error\n");
    }
    uart_trig_process(0);
}
