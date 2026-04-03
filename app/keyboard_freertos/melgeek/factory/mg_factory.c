/*
 * Copyright (c) 2024-2025 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#include "mg_factory.h"
#include "app_debug.h"
#include "hal_flash.h"
#include "hal_wdg.h"

#if DEBUG_EN == 1
#define MODULE_LOG_ENABLED (1)
#if MODULE_LOG_ENABLED == 1
#define LOG_TAG "mg_factory"
#define DBG_PRINTF(fmt, ...) DEBUG_RTT(LOG_TAG, fmt, ##__VA_ARGS__)
#else
#define DBG_PRINTF(fmt, ...)
#endif
#else
#define DBG_PRINTF(fmt, ...)
#endif


mg_factory_t g_fact_info = {0};

mg_factory_t* mg_factory_get_info(void)
{
    return &g_fact_info;
}

void mg_factory_set_info(mg_factory_t * update)
{
    memcpy(&g_fact_info, update, sizeof(mg_factory_t));
    hal_wdg_manual_feed();
    hal_flash_erase(FLASH_FACTORY_ADDR, 1);

    hal_flash_write(FLASH_FACTORY_ADDR, (const uint8_t *)&g_fact_info, sizeof(mg_factory_t));
}

bool mg_factory_checkout(void)
{
    bool change = false;
    fa_cnt_t fa_max[VENDORS_MAX] = {FA_STEP_MAX_JIANHANG, FA_STEP_MAX_NARUI};

    for (uint8_t i = 0; i < VENDORS_MAX; i++)
    {
        for (uint8_t j = 0; j < FA_STEP_MAX; j++)
        {
            uint16_t *tmp_fa = (uint16_t *)&g_fact_info.fa[i][j];

            if (j < fa_max[i])
            {
                uint16_t fa_code = ((j + 1) << 8) | (i + 1);
                if (*tmp_fa != fa_code)
                {
                    change = true;
                }
            }
        }
    }

    return change;
}

void mg_factory_set_default_data(void)
{
    bool change = false;
    fa_cnt_t fa_max[VENDORS_MAX] = {FA_STEP_MAX_JIANHANG, FA_STEP_MAX_NARUI};

    for (uint8_t i = 0; i < VENDORS_MAX; i++)
    {
        for (uint8_t j = 0; j < FA_STEP_MAX; j++)
        {
            uint16_t *tmp_fa = (uint16_t *)&g_fact_info.fa[i][j];

            if (j < fa_max[i])
            {
                uint16_t fa_code = ((j + 1) << 8) | (i + 1);
                if (*tmp_fa != fa_code)
                {
                    *tmp_fa = fa_code;
                    change = true;
                }
            }
            else
            {
                if (*tmp_fa != 0xFFFF)
                {
                    *tmp_fa = 0xFFFF;
                    change = true;
                }
            }
        }
    }

    if (change)
    {
        hal_wdg_manual_feed();
        hal_flash_erase(FLASH_FACTORY_ADDR, 1);
        hal_flash_write(FLASH_FACTORY_ADDR, (const uint8_t *)&g_fact_info, sizeof(mg_factory_t));
    }
}

void mg_factory_init(void)
{
    hal_flash_read(FLASH_FACTORY_ADDR, &g_fact_info, sizeof(mg_factory_t));
}
