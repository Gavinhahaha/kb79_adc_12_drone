/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include "mg_matrix.h"
#include "mg_key_code.h"
#include "board.h"
#include "db_flash_map.h"
#include "db.h"
#include "app_debug.h"
#include "queue.h"
#include "app_config.h"
#include "mg_build_date.h"
#include "hal_flash.h"
#include "easy_crc.h"
#include "mg_cali.h"
#include "mg_key_code.h"
#include "rgb_led_color.h"
#include "rgb_led_ctrl.h"
#include "layout_fn.h"
#include "mg_hive.h"
#include "hal_wdg.h"
#include "mg_uart.h"
#include "mg_indev.h"
#include "mg_factory.h"
#include "mg_hid.h"
#include "mg_detection.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "db"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif


#define SYS_STABLE_RUN_TIME   pdMS_TO_TICKS(7000)
#define SAVE_INTERVAL_TIME    pdMS_TO_TICKS(3000)
#define DB_NOTIFY_SAVE_NOW    (1UL << 0)
#define DB_NOTIFY_SAVE_LATER  (1UL << 1)
#define DB_NOTIFY_SWITCH_CFG  (1UL << 2)

#define POWER_SAVE_TIME_DEFAULT   (60 * 30) // 30分钟

#define CFG1_DEFAULT_NAME   "Default Profile"
#define CFG2_DEFAULT_NAME   "Typing Profile"
#define CFG3_DEFAULT_NAME   "Rapid Profile"
#define CFG4_DEFAULT_NAME   "E-sports Profile"
 
static void reload_binfo(void);
static void reload_sk_la_lm_kc(void);
static void reload_kr(void);
static void reload_macro(void);
static void db_reset_sk(void);
static void db_reset_mx(void);
static void db_reset_kc(void);
static void db_reset_binfo(void);
extern void sk_check_para(sk_la_lm_kc_info_t *info);

static TaskHandle_t db_handle = NULL;
static volatile db_item_t db_item[DB_MAX] = {0};
static kb_dpm_param_t kb_dpm_param = {0};
static bool is_swithcing_profile = false;
sk_la_lm_kc_info_t sk_la_lm_kc_info;
binfo_t gbinfo;
common_info_t common_info;

tmx_t gmx;

static const uint32_t flash_bs_addr[DB_MAX] =
{
    FLASH_SYS_ADDR,
    FLASH_SK_ADDR,//DB_LA_BASE,//DB_LM_BASE,//DB_KC_BASE,
    FLASH_KR_ADDR,
    FLASH_MACRO_ADDR,
    FLASH_CALIB_ADDR,
    FLASH_COMMON_INFO_ADDR,
};

static cfg_cb_t cfg_cb_group[DB_MAX] = 
{
    [DB_SYS]         = reload_binfo,
    [DB_SK_LA_LM_KC] = reload_sk_la_lm_kc,
    [DB_KR]          = reload_kr,
    [DB_MACRO]       = reload_macro,
    [DB_CALI]        = NULL,
    [DB_COMMON]      = NULL,
};

static bool db_item_is_ready(db_t idx)
{
    return (idx < DB_MAX) &&
           (db_item[idx].flash_addr != 0) &&
           (db_item[idx].src_addr != 0) &&
           (db_item[idx].size != 0);
}
    
uint32_t db_read(db_t idx)
{
    if (!db_item_is_ready(idx))
    {
        DBG_PRINTF("db_read err = %d,%x,%x,%d\n", idx, db_item[idx].flash_addr, db_item[idx].src_addr, db_item[idx].size);
        return DB_RET_ERR_ADDR; 
    }
    return (uint32_t)hal_flash_read(db_item[idx].flash_addr, db_item[idx].src_addr, db_item[idx].size);
}

static uint32_t db_write(db_t idx)
{
    uint32_t ret;
    uint32_t page_num;
    uint32_t written_size = 0;

    page_num = (db_item[idx].size < FLASH_PAGE_SIZE) ? 1 : (db_item[idx].size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;

    uint32_t addr = db_item[idx].flash_addr;
    while (page_num > 0)
    {
        uint32_t pages_to_erase = (page_num > FLASH_ERASE_ONCE_MAX_PAGES) ? FLASH_ERASE_ONCE_MAX_PAGES : page_num;

        hal_wdg_manual_feed();
        ret = hal_flash_erase(addr, pages_to_erase);
        if (ret != 0)
        {
            return ret;
        }
        addr += pages_to_erase * FLASH_PAGE_SIZE;
        page_num -= pages_to_erase;
    }

    while (written_size < db_item[idx].size)
    {
        uint32_t current_chunk_size = (db_item[idx].size - written_size > FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : (db_item[idx].size - written_size);

        hal_wdg_manual_feed();
        ret = hal_flash_write(db_item[idx].flash_addr + written_size, db_item[idx].src_addr + written_size, current_chunk_size);
        if (ret != 0)
        {
            return ret;
        }

        written_size += current_chunk_size;
    }

    return 0; 
}

void db_read_cfg_block(uint8_t cfg_id, db_t db_id, uint16_t offset, uint8_t *buff, uint16_t size)
{
    hal_flash_read(flash_bs_addr[db_id] + cfg_id * FLASH_ONE_CFG_TOTAL_SIZE + offset, buff, size);
}

void db_write_cfg_block(uint8_t cfg_id, db_t db_id, uint8_t *buff, uint16_t size)
{
    uint32_t ret;
    uint32_t written_size = 0;
    uint32_t page_num = (size < FLASH_PAGE_SIZE) ? 1 : (size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;

    uint32_t addr = flash_bs_addr[db_id] + cfg_id * FLASH_ONE_CFG_TOTAL_SIZE;
    while (page_num > 0)
    {
        uint32_t pages_to_erase = (page_num > FLASH_ERASE_ONCE_MAX_PAGES) ? FLASH_ERASE_ONCE_MAX_PAGES : page_num;
        hal_wdg_manual_feed();
        hal_flash_erase(addr, pages_to_erase);
        addr += pages_to_erase * FLASH_PAGE_SIZE;
        page_num -= pages_to_erase;
    }

    while (written_size < size)
    {
        uint32_t current_chunk_size = (size - written_size > FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : (size - written_size);
        hal_wdg_manual_feed();
        ret = hal_flash_write(flash_bs_addr[db_id] + cfg_id * FLASH_ONE_CFG_TOTAL_SIZE + written_size, (const uint8_t *)buff + written_size, current_chunk_size);
        if (ret != 0)
        {
            return;
        }

        written_size += current_chunk_size;
    }

    return; 
}

void db_read_cfg_cache(uint8_t block_id, uint8_t *buff, uint16_t size)
{
    if (DB_SYS == block_id)
    {
        hal_flash_read(FLASH_CFG_CACHE_ADDR, buff, size);
    }
    else
    {
        hal_flash_read(FLASH_CFG_CACHE_ADDR + FLASH_SYS_SIZE + (block_id - 1) * 4 * 1024, buff, size);
    }
}

void db_write_cfg_cache(uint8_t block_id, uint16_t offset, uint8_t *buff, uint16_t size)
{
	hal_wdg_manual_feed();
    if (DB_SYS == block_id)
    {
        hal_flash_write(FLASH_CFG_CACHE_ADDR + offset, (const uint8_t *)buff, size);
    }
    else
    {
        hal_flash_write(FLASH_CFG_CACHE_ADDR + FLASH_SYS_SIZE + (block_id - 1) * 4 * 1024 + offset, (const uint8_t *)buff, size);
    }
}

void db_erase_cfg_cache(uint8_t block_id)
{
	hal_wdg_manual_feed();
    if (DB_SYS == block_id)
    {
        hal_flash_erase(FLASH_CFG_CACHE_ADDR, 2);
    }
    else
    {
        hal_flash_erase(FLASH_CFG_CACHE_ADDR + FLASH_SYS_SIZE + (block_id - 1) * 4 * 1024, 1);
    }
}

void db_erase_gm_fw_info(uint32_t addr)
{
	hal_wdg_manual_feed();
    uint32_t page_num = (sizeof(gm_fw_info_t) < FLASH_PAGE_SIZE) ? 1 : (sizeof(gm_fw_info_t) + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;
    hal_flash_erase(addr, page_num);
}

void db_write_gm_fw_info(uint32_t addr, gm_fw_info_t *fw_info)
{
    if (fw_info)
    {
		hal_wdg_manual_feed();
        hal_flash_write(addr, (const uint8_t *)fw_info, sizeof(gm_fw_info_t));
    }
}

void db_read_gm_fw_info(uint32_t addr, gm_fw_info_t *fw_info)
{
    if (fw_info)
    {
        hal_flash_read(addr, (uint8_t *)fw_info, sizeof(gm_fw_info_t));
    }
}

void db_erase_gm_fw(uint32_t addr1)
{
    uint32_t total_pages = (FLASH_GM_FW_SIZE + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;
    uint32_t addr = addr1;

    while (total_pages > 0)
    {
        uint32_t pages_to_erase = (total_pages > FLASH_ERASE_ONCE_MAX_PAGES) ? FLASH_ERASE_ONCE_MAX_PAGES : total_pages;
		hal_wdg_manual_feed();
        hal_flash_erase(addr, pages_to_erase);
        addr += pages_to_erase * FLASH_PAGE_SIZE;
        total_pages -= pages_to_erase;
    }
}

void db_write_gm_fw(uint32_t offset, uint8_t *buff, uint16_t size)
{
	hal_wdg_manual_feed();
    hal_flash_write(offset, (const uint8_t *)buff, size);
}

void db_read_gm_fw(uint32_t offset, uint8_t *buff, uint16_t size)
{
    hal_flash_read(offset, buff, size);
}

void db_register(db_t idx, uint32_t *p_buf, int size)
{
    if (size > (FLASH_PAGE_SIZE * 2))
    {
        DBG_PRINTF("db_register:err db[%d] size= %d\n", idx, size);
        return;
    }
    
    db_item[idx].flash_addr = flash_bs_addr[idx];
    db_item[idx].src_addr   = (uint8_t *)p_buf;
    db_item[idx].size       = size;
    db_item[idx].cfg_cb     = cfg_cb_group[idx];
    DBG_PRINTF("db[%d] size= %d\n", idx, size);
}

uint32_t db_get_itf(db_t idx)
{
    uint32_t init = 0;

    if (idx < DB_MAX)
    {
        init = *(uint32_t *)db_item[idx].src_addr;
    }
    return init;
}

static void db_set_itf(db_t idx, uint32_t itf)
{
    if (idx < DB_MAX)
    {
        *(uint32_t *)db_item[idx].src_addr = itf;
    }
}

static void db_notify_task(uint32_t notify_bits)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (db_handle == NULL)
    {
        return;
    }

    if (xPortIsInsideInterrupt() != pdFALSE)
    {
        if (xTaskNotifyFromISR(db_handle, notify_bits, eSetBits, &xHigherPriorityTaskWoken) != pdPASS)
        {
            DBG_PRINTF("failed to notify db task in interrupt\r\n");
        }
    }
    else
    {
        if (xTaskNotify(db_handle, notify_bits, eSetBits) != pdPASS)
        {
            DBG_PRINTF("failed to notify db task in thread\r\n");
        }
    }
}

static void db_flush_dirty_data(void)
{
    for (db_t i = 0; i < DB_MAX; i++)
    {
        bool should_write = false;

        taskENTER_CRITICAL();
        if ((db_item[i].need_update == true) && db_item_is_ready(i))
        {
            db_item[i].need_update = false;
            should_write = true;
        }
        taskEXIT_CRITICAL();

        if (should_write)
        {
            hal_wdg_manual_feed();
            db_set_itf(i, DB_INIT_MAGIC);
            if (db_write(i) != 0)
            {
                taskENTER_CRITICAL();
                db_item[i].need_update = true;
                taskEXIT_CRITICAL();
            }
        }
    }
}

void db_set_with_dpm_refresh(void)
{
    kb_dpm_param = KB_DPM_PARAM_GET();
}

static void db_dpm_param_handle_change(void)
{
    if (true == is_swithcing_profile)
    {
        return;
    }

    kb_dpm_param_t curr_param = KB_DPM_PARAM_GET();
    bool has_changed = false;
    uint8_t buf[2] = {0};

    for (uint8_t i = 1; i < sizeof(kb_dpm_param_t); i++)
    {
        if (*((uint8_t *)&curr_param + i) != *((uint8_t *)&kb_dpm_param + i))
        {
            buf[0] = i;
            buf[1] = *((uint8_t *)&curr_param + i);
            uart_notify_screen_cmd(CMD_UART_KB_PARAM, SCMD_SET_KB_INFO, buf, sizeof(buf));
            vTaskDelay(pdMS_TO_TICKS(25));
            has_changed = true;
        }
    }

    if (has_changed)
    {
        kb_dpm_param = curr_param;
    }
}

void db_update(db_t idx, db_act_t act)
{
    bool already_pending = false;

    if (mg_detecte_get_volt_low_state())
    {
        if ((idx == DB_SYS) || (idx == DB_CALI))
        {
            return;
        }
    }

    db_dpm_param_handle_change();
    if ((idx >= DB_MAX) || (act > UPDATE_LATER))
    {
        return;
    }

    already_pending = db_item[idx].need_update;
    db_item[idx].need_update = true;

    if (UPDATE_NOW == act)
    {
        if ((xPortIsInsideInterrupt() == pdFALSE) &&
            ((db_handle == NULL) ||
             (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) ||
             (eTaskGetState(db_handle) == eSuspended)))
        {
            db_flush_dirty_data();
            return;
        }
        db_notify_task(DB_NOTIFY_SAVE_NOW);
    }
    else
    {
        if (already_pending)
        {
            return;
        }
        db_notify_task(DB_NOTIFY_SAVE_LATER);
    }
}

void db_check_binfo(binfo_t * binfo)
{
    DBG_PRINTF("db_binfo_init\n");

    if (binfo->nkro > BOARD_NKRO_ENABLE)
    {
        binfo->nkro = BOARD_NKRO_ENABLE;
    }
    binfo->pi.inat = BOARD_INACTIVE_TIME;
    binfo->pi.vc   = BOARD_VC;
    binfo->pi.pc   = BOARD_PC;

    memcpy(binfo->pi.vn, BOARD_VN, strlen(BOARD_VN));

    binfo->pi.fv = BOARD_FW_VERSION;
    binfo->pi.hv = BOARD_HW_VERSION;

    mg_get_yyyymmddhhmmss(binfo->pi.cd);

    memcpy(binfo->pi.ui, BOARD_UI, strlen(BOARD_UI));

    binfo->sms = 0;
    
    if ((binfo->kc.cur_kcm >= BOARD_KCM_MAX)||
        (binfo->kc.max_kcm != BOARD_KCM_MAX)||
        (binfo->kc.cur_kcm >= binfo->kc.max_kcm)||
        (binfo->kc.max_kc > BOARD_KEY_NUM))
    {
        binfo->kc.cur_kcm = 0;
        binfo->kc.max_kcm = BOARD_KCM_MAX;
        binfo->kc.max_kc  = BOARD_KEY_NUM;
    }
        
    if (binfo->layout > 3) 
    {
        binfo->layout = 0;
    }

    if ((binfo->kr.cur_krm >= BOARD_LED_COLOR_LAYER_MAX)||
        (binfo->kr.max_krm != BOARD_LED_COLOR_LAYER_MAX)||
        (binfo->kr.cur_krm >= binfo->kr.max_krm )||
        (binfo->kr.max_kr  > BOARD_KEY_LED_NUM))
    {
        binfo->kr.cur_krm = 0;
        binfo->kr.max_krm = BOARD_LED_COLOR_LAYER_MAX;
        binfo->kr.max_kr  = BOARD_KEY_LED_NUM;
    }
    
    if ((binfo->lm.cur_lmm >= LED_ACTION_MAX) ||
        (binfo->lm.max_lmm != LED_ACTION_MAX)  ||
        (binfo->lm.cur_lmm >= binfo->lm.max_lmm)||
        (binfo->lm.max_lm  > BOARD_KEY_LED_NUM))
    {
        binfo->lm.cur_lmm  = LED_ACTION_GRADUAL;
        binfo->lm.max_lmm  = LED_ACTION_MAX;
        binfo->lm.max_lm   = BOARD_KEY_LED_NUM;
        binfo->lm.last_lmm = LED_ACTION_MAX;
        binfo->lm.tmp_lmm  = LED_ACTION_GRADUAL;
    }
    
    for (uint8_t i = 0; i < LIGHT_TAPE_PART_MAX; i++)
    {
        if (binfo->rl[i].md >= RGBLIGHT_EFFECT_MAX)
        {
            binfo->rl[i].state = 1;
            binfo->rl[i].md    = 1;
        }
    }

    if (binfo->startup_effect_en > 1)
    {
        binfo->startup_effect_en = 1;
    }

    if (binfo->rl_shape > LIGHT_TAPE_WHOLE)
    {
        binfo->rl_shape = LIGHT_TAPE_SPLIT;
    }


    if ((binfo->power_save.time > 60 * 60) || (binfo->power_save.time < 60))
    {
       binfo->power_save.en    = true;
       binfo->power_save.time  = POWER_SAVE_TIME_DEFAULT;
    }
    
    if (binfo->kh_qty.rt_separately_en > 1)
    {
       binfo->kh_qty.rt_separately_en    = 0;
    }
    light_tape_check_para(binfo->rl);
    matrix_para_check(binfo->key_attr);
    
}

static void db_check_kc_para(kc_t (*kc_table)[BOARD_KEY_NUM])
{
    if (kc_table)
    {
        for (uint8_t i = 0; i < BOARD_KCM_MAX; i++)
        {
            for (uint8_t j = 0; j < BOARD_KEY_NUM; j++)
            {
                if (kc_table[i][j].ty  >= KCT_MAX ||
                    kc_table[i][j].sty >= 128     ||
                    kc_table[i][j].co  >= 0x3000)
                {
                    kc_table[i][j] = kc_get_default_layer_ptr(i)[j];
                }
            }
        }
    }
}

static void db_check_la_para(led_action_t *la_table)
{
    const led_action_t *la_table_default = get_led_action_default_table();

    if (la_table)
    {
        for (uint8_t i = 0; i < LED_ACTION_MAX; i++)
        {
            if (la_table[i].at >= LED_ACTION_SYS_MAX ||
                la_table[i].id >= LED_ACTION_MAX)
            {
                la_table[i]    = la_table_default[i];
                la_table[i].id = la_table_default[i].id;
                la_table[i].at = la_table_default[i].at;
            }
        }
    }
}

void db_check_sk_la_lm_kc_para(sk_la_lm_kc_info_t *para)
{
    if (para)
    { 
        sk_check_para(para);
        db_check_kc_para(para->kc_table);
        db_check_la_para(para->la_table);
    }
}

void db_check_kr_para(key_led_color_info_t *para)
{
    if (para)
    {
        rgb_led_check_para(para);
    }
}

static void db_check_common_info(common_info_t *common_info)
{
    if (common_info)
    {
        bool need_update = false;

        const char cfg_default_name[KEYBOARD_CFG_MAX][KEYBOARD_CFG_NAME_SIZE] = {
            CFG1_DEFAULT_NAME,
            CFG2_DEFAULT_NAME,
            CFG3_DEFAULT_NAME,
            CFG4_DEFAULT_NAME
        };

        if (NETBAR_MODE_EN == common_info->netbar_info.mode_en)
        {
            db_reset_binfo();
            db_reset_mx();

            for (uint8_t cfg_id = 0; cfg_id < KEYBOARD_CFG_MAX; cfg_id++)
            {
                for (db_t i = 0; i < DB_CALI; i++)
                {
                    if (db_item_is_ready(i))
                    {
                        hal_wdg_manual_feed();
                        db_item[i].flash_addr = flash_bs_addr[i] + cfg_id * FLASH_ONE_CFG_TOTAL_SIZE;
                        db_item[i].need_update = false;
                        db_set_itf(i, DB_INIT_MAGIC);
                        db_write(i);   
                    }
                }
            }

            common_info->netbar_info.mode_active = 1;
            common_info->cfg_info.cfg_idx = 0;
            for (int i = 0; i < KEYBOARD_CFG_MAX; i++)
            {
                memset(common_info->cfg_info.cfg_name[i], '\0', KEYBOARD_CFG_NAME_SIZE);
                memcpy(common_info->cfg_info.cfg_name[i], cfg_default_name[i], KEYBOARD_CFG_NAME_SIZE);
            }
        }

        if (common_info->itf != DB_INIT_MAGIC)
        {
            memset(common_info, 0, sizeof(common_info_t));
            common_info->itf = DB_INIT_MAGIC;
            common_info->cfg_info.cfg_idx = 0;
            for (int i = 0; i < KEYBOARD_CFG_MAX; i++)
            {
                memset(common_info->cfg_info.cfg_name[i], '\0', KEYBOARD_CFG_NAME_SIZE);
                memcpy(common_info->cfg_info.cfg_name[i], cfg_default_name[i], KEYBOARD_CFG_NAME_SIZE);
            }

            common_info->netbar_info.mode_en     = 0;
            common_info->netbar_info.mode_active = 0;
            
            need_update = true;
        }
        else
        {
            if (common_info->cfg_info.cfg_idx >= KEYBOARD_CFG_MAX)
            {
                common_info->cfg_info.cfg_idx = 0;
                need_update = true;
            }

            for (int i = 0; i < KEYBOARD_CFG_MAX; i++)
            {
                const char *str = (const char *)common_info->cfg_info.cfg_name[i];
                uint16_t len = strnlen(str, KEYBOARD_CFG_NAME_SIZE);
                if ((len == 0) || (len >= KEYBOARD_CFG_NAME_SIZE))
                {
                    memset(common_info->cfg_info.cfg_name[i], '\0', KEYBOARD_CFG_NAME_SIZE);
                    memcpy(common_info->cfg_info.cfg_name[i], cfg_default_name[i], KEYBOARD_CFG_NAME_SIZE);
                    need_update = true;
                }
            }

            if (common_info->netbar_info.mode_en > 1)
            {
                common_info->netbar_info.mode_en     = 0;
                common_info->netbar_info.mode_active = 0;
                need_update = true;
            }
        }

        if (need_update)
        {
            db_flash_region_mark_update(DB_COMMON);
        }
    }
}

static void common_db_register(void)
{
    db_register(DB_COMMON, (uint32_t *)&common_info, sizeof(common_info));
    db_read(DB_COMMON);
    db_check_common_info(&common_info);
}

static void reload_binfo(void)
{
    db_read(DB_SYS);
    DBG_PRINTF("reload binfo=%x\n", gbinfo.itf);
    
    if (gbinfo.itf != DB_INIT_MAGIC)
    {
        db_reset_binfo();
        db_reset_mx();
    }
    else
    {
        db_check_binfo(&gbinfo);
    }

    if (layout_fn_is_in())
    {
        gbinfo.kc.cur_kcm = 1; //fn layer
        gbinfo.layout = 0;
    }
    else
    {
        gbinfo.kc.cur_kcm = gbinfo.layout = 0;
    }
}

static void reload_sk_la_lm_kc(void)
{
    db_read(DB_SK_LA_LM_KC);
    uint32_t itf = db_get_itf(DB_SK_LA_LM_KC);

    DBG_PRINTF("reload_sk_la_lm_kc\n");
    
    if (itf != DB_INIT_MAGIC)
    {
        DBG_PRINTF("DB_SK_LA_LM_KC invalid data=%x,%x\r\n",itf,DB_INIT_MAGIC);
        db_reset_kc();
        db_reset_la();
        db_reset_sk();
    }
    else
    {
        db_check_sk_la_lm_kc_para(&sk_la_lm_kc_info);
    }
}

static void reload_kr(void)
{
    db_read(DB_KR);
    uint32_t itf = db_get_itf(DB_KR);

    DBG_PRINTF("reload_kr\n");
    
    if (itf != DB_INIT_MAGIC)
    {
        DBG_PRINTF("DB_KR invalid data=%x,%d\r\n",itf,DB_INIT_MAGIC);
        db_reset_kr();
        db_flash_region_mark_update(DB_KR); 
    }
    else
    {
        key_led_color_info_t * para = get_key_led_color_table_info();
        db_check_kr_para(para);
    }
}

static void reload_macro(void)
{
    db_read(DB_MACRO);
    uint32_t itf = db_get_itf(DB_MACRO);
    
    DBG_PRINTF("reload_macro\n");
    
    if (itf != DB_INIT_MAGIC)
    {
        sk_set_default(SKT_MACRO);
        db_flash_region_mark_update(DB_MACRO);
    }

    rgb_led_set_all();
}

void db_send_toggle_cfg_signal(void)
{
    is_swithcing_profile = true;
    db_flash_region_mark_update(DB_COMMON);
    db_notify_task(DB_NOTIFY_SWITCH_CFG);
}

static void db_set_cfg(uint8_t cfg_id)
{
    DBG_PRINTF("db_set_cfg=%d\n",cfg_id);
    for (db_t i = 0; i < DB_CALI; i++)
    {
        db_item[i].flash_addr = flash_bs_addr[i] + cfg_id * FLASH_ONE_CFG_TOTAL_SIZE;
    }
    
    for (db_t i = 0; i < DB_CALI; i++)//轴体类型和校准数据不随配置改变
    {
        //重新初始化相关模块
        if (db_item[i].cfg_cb)
        {
            db_item[i].cfg_cb();
        }
    }
}

void db_sys_set_cfg(uint8_t cfg_id)
{
    DBG_PRINTF("db_set_cfg=%d\n",cfg_id);
    db_item[DB_SYS].flash_addr = flash_bs_addr[DB_SYS] + cfg_id * FLASH_ONE_CFG_TOTAL_SIZE;
    //重新初始化相关模块
    if (db_item[DB_SYS].cfg_cb)
    {
        db_item[DB_SYS].cfg_cb();
    }
}

static void db_reset_binfo(void)
{
    DBG_PRINTF("db_reset_binfo\n");
    memset(&gbinfo, 0, sizeof(gbinfo));

    gbinfo.itf  = DB_INIT_MAGIC;
    gbinfo.nkro = BOARD_NKRO_ENABLE;

    gbinfo.pi.inat = BOARD_INACTIVE_TIME;
    gbinfo.pi.vc   = BOARD_VC;
    gbinfo.pi.pc   = BOARD_PC;

    memcpy(gbinfo.pi.vn, BOARD_VN, strlen(BOARD_VN));

    gbinfo.pi.fv = BOARD_FW_VERSION;
    gbinfo.pi.hv = BOARD_HW_VERSION;

    mg_get_yyyymmddhhmmss(gbinfo.pi.cd);

    memcpy(gbinfo.pi.ui, BOARD_UI, strlen(BOARD_UI));

    gbinfo.sms = 0;

    gbinfo.kc.cur_kcm   = 0;
    gbinfo.kc.max_kcm   = BOARD_KCM_MAX;
    gbinfo.kc.max_kc    = BOARD_KEY_NUM;
    gbinfo.layout       = 0;
    gbinfo.startup_effect_en = 1;

    gbinfo.kr.cur_krm = 0;
    gbinfo.kr.max_krm = BOARD_LED_COLOR_LAYER_MAX;
    gbinfo.kr.max_kr  = BOARD_KEY_LED_NUM;

    gbinfo.lm.cur_lmm  = LED_ACTION_GRADUAL;
    gbinfo.lm.max_lmm  = LED_ACTION_MAX;
    gbinfo.lm.max_lm   = BOARD_KEY_LED_NUM;
    gbinfo.lm.last_lmm = LED_ACTION_MAX;

    gbinfo.rl_shape = LIGHT_TAPE_SPLIT;

    gbinfo.rl[LIGHT_TAPE_PART_BACK].state = 1;
    gbinfo.rl[LIGHT_TAPE_PART_BACK].md    = 1;

    gbinfo.rl[LIGHT_TAPE_PART_SIDE].state = 1;
    gbinfo.rl[LIGHT_TAPE_PART_SIDE].md    = 1;

    gbinfo.power_save.en           = true;
    gbinfo.power_save.time         = POWER_SAVE_TIME_DEFAULT;
    gbinfo.kh_qty.rt_separately_en = false;
    matrix_para_reset();
    light_tape_set_default();
    kb_dpm_param = KB_DPM_PARAM_GET();
    db_flash_region_mark_update(DB_SYS);
}

static void db_reset_kc(void)
{
    DBG_PRINTF("db_reset_kc\n");
    memcpy(sk_la_lm_kc_info.kc_table, kc_get_default_layer_ptr(0), sizeof(sk_la_lm_kc_info.kc_table));
    db_flash_region_mark_update(DB_SK_LA_LM_KC);
}

void db_reset_kr(void)
{
    const uint32_t *color_ptr = get_rgb_layer_color();
    for (int i = 0; i < BOARD_LED_COLOR_LAYER_MAX; i++)
    {
        for (int j = 0; j < BOARD_KEY_LED_NUM; j++)
        {
            set_rgb_from_hex(color_ptr[i], get_key_led_color_ptr(i, j));
        }
    }
}

void db_reset_la(void)
{
    const led_action_t *la_table_default = get_led_action_default_table();

    memcpy(sk_la_lm_kc_info.la_table, la_table_default, sizeof(sk_la_lm_kc_info.la_table));
    for (unsigned int i = 0; i < LED_ACTION_MAX; ++i)
    {
        gmx.pla[i].id = sk_la_lm_kc_info.la_table[i].id;
        gmx.pla[i].at = sk_la_lm_kc_info.la_table[i].at;
    }
}

static void db_reset_sk(void)
{
    sk_set_default(SKT_MAX);
}

static void db_reset_mx(void)
{
    DBG_PRINTF("db_reset_mx\n");
    db_reset_kc();
    db_reset_sk();
    db_reset_kr();
    db_reset_la();
    db_flash_region_mark_update(DB_KR);
    db_flash_region_mark_update(DB_SK_LA_LM_KC);
}

void db_task(void *pvParameters)
{
    (void)pvParameters;
    uint32_t notify_bits = 0;
    uint32_t pending_bits = 0;
    
    while(1)
    {
        if ((pending_bits == 0) &&
            (xTaskNotifyWait(0, 0xFFFFFFFF, &notify_bits, portMAX_DELAY) == pdPASS))
        {
            pending_bits |= notify_bits;
        }

        if ((pending_bits & DB_NOTIFY_SWITCH_CFG) != 0)
        {
            pending_bits = 0;
            sm_unsync_callback();
            db_flush_dirty_data();
            taskENTER_CRITICAL();
            db_set_cfg(common_info.cfg_info.cfg_idx);
            taskEXIT_CRITICAL();
            db_flush_dirty_data();
            is_swithcing_profile = false;
            uint8_t buf[2] = {PROFILE_INDEX, common_info.cfg_info.cfg_idx};
            uart_notify_screen_cmd(CMD_UART_KB_PARAM, SCMD_SET_KB_INFO, buf, sizeof(buf));
            if (mg_detecte_get_volt_low_state())
            {
                matrix_para_reset();
                interface_release_report();
            }
            matrix_set_nkro(gbinfo.nkro);
            matrix_para_reload();
            cmd_do_response(CMD_NOTIFY, NTF_ADV_SCENE, 0, 0, NULL);
            hal_key_sync_callback(sm_sync_callback);
            light_tape_init_ranges();
            rgb_led_toggle_cfg_complete_prompt(common_info.cfg_info.cfg_idx);
            continue;
        }

        if ((xTaskGetTickCount() < SYS_STABLE_RUN_TIME) &&
            ((pending_bits & (DB_NOTIFY_SAVE_NOW | DB_NOTIFY_SAVE_LATER)) != 0))
        {
            TickType_t wait_ticks = SYS_STABLE_RUN_TIME - xTaskGetTickCount();

            if (xTaskNotifyWait(0, 0xFFFFFFFF, &notify_bits, wait_ticks) == pdPASS)
            {
                pending_bits |= notify_bits;
                continue;
            }
        }

        if ((pending_bits & DB_NOTIFY_SAVE_NOW) != 0)
        {
            pending_bits &= ~(DB_NOTIFY_SAVE_NOW | DB_NOTIFY_SAVE_LATER);
            sm_unsync_callback();
            db_flush_dirty_data();
            hal_key_sync_callback(sm_sync_callback);
            continue;
        }

        if ((pending_bits & DB_NOTIFY_SAVE_LATER) != 0)
        {
            if (xTaskNotifyWait(0, 0xFFFFFFFF, &notify_bits, SAVE_INTERVAL_TIME) == pdPASS)
            {
                pending_bits |= notify_bits;
                continue;
            }

            pending_bits &= ~DB_NOTIFY_SAVE_LATER;
            sm_unsync_callback();
            db_flush_dirty_data();
            hal_key_sync_callback(sm_sync_callback);
        }
    }
}

void mx_db_register(void)
{
    db_register(DB_SK_LA_LM_KC, (uint32_t *)&sk_la_lm_kc_info, sizeof(sk_la_lm_kc_info));
    key_code_init(sk_la_lm_kc_info.kc_table);
    gmx.pkc  = sk_la_lm_kc_info.kc_table;
    table_info_t table_info = get_key_tmp_sta_table_info();
    gmx.pkts = (key_tmp_sta_t *)table_info.table;

    key_led_color_info_t * led_color_info = get_key_led_color_table_info();
    db_register(DB_KR, (uint32_t *)led_color_info, sizeof(key_led_color_info_t));
    
    gmx.pkr  = led_color_info->table;
    gmx.pla  = sk_la_lm_kc_info.la_table;
    gmx.laht = get_led_action_handle_table();
}

void db_reset(void)
{
    const char cfg_default_name[KEYBOARD_CFG_MAX][KEYBOARD_CFG_NAME_SIZE] = {
        CFG1_DEFAULT_NAME,
        CFG2_DEFAULT_NAME,
        CFG3_DEFAULT_NAME,
        CFG4_DEFAULT_NAME
    };

    db_reset_binfo();
    db_reset_mx();
    
    db_task_suspend();
    for (uint8_t cfg_id = 0; cfg_id < KEYBOARD_CFG_MAX; cfg_id++)
    {
        for (db_t i = 0; i < DB_CALI; i++)
        {
            db_set_itf(i, DB_INIT_MAGIC);
            taskENTER_CRITICAL();
            db_item[i].flash_addr = flash_bs_addr[i] + cfg_id * FLASH_ONE_CFG_TOTAL_SIZE;
            db_item[i].need_update = false;
            taskEXIT_CRITICAL();
            if (db_write(i) != 0)
            {
                db_task_resume();
                return;
            }
        }
    }

    taskENTER_CRITICAL();
    common_info.cfg_info.cfg_idx = 0;
    for (int i = 0; i < KEYBOARD_CFG_MAX; i++)
    {
        memset(common_info.cfg_info.cfg_name[i], '\0', KEYBOARD_CFG_NAME_SIZE);
        memcpy(common_info.cfg_info.cfg_name[i], cfg_default_name[i], KEYBOARD_CFG_NAME_SIZE);
    }
    taskEXIT_CRITICAL();

    db_item[DB_COMMON].need_update = false;
    db_set_itf(DB_COMMON, DB_INIT_MAGIC);
    db_write(DB_COMMON);
    db_task_resume();
}

void db_flash_region_mark_update(db_t region)
{
    if (true != db_item[region].need_update)
    {
        db_item[region].need_update = true;
    }
}

void db_save_now(void)
{
    db_flush_dirty_data();
}

void db_task_suspend(void)
{
    if (db_handle)
    {
        vTaskSuspend(db_handle);
    }
}

void db_task_resume(void)
{
    if (db_handle)
    {
        vTaskResume(db_handle);
    }
}

void db_init(void)
{
    hal_flash_init();
    board_delay_ms(10);
    DBG_PRINTF("db_init\n");
    DBG_PRINTF("%s--%s\n",__DATE__,__TIME__);

    mg_factory_init();
    
    if (xTaskCreate(db_task, "db", STACK_SIZE_DB, NULL, PRIORITY_DB, &db_handle) != pdPASS)
    {
        DBG_PRINTF("db_init:db_task creat failed\n");
    }
    matrix_para_register();
    cali_db_register();
    db_register(DB_SYS, (uint32_t *)&gbinfo, sizeof(gbinfo));
    mx_db_register();
    sk_init();
    common_db_register();
    db_set_cfg(common_info.cfg_info.cfg_idx);
    kb_dpm_param = KB_DPM_PARAM_GET();
}

