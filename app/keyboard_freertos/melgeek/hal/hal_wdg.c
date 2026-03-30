/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */


#include "hal_wdg.h"
#include "hal_timer.h"
#include "board.h"
#include "hpm_ewdg_drv.h"
#include "app_debug.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "hal_wdg"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif



#define EWDG_CNT_CLK_FREQ            32768UL
#define HAL_WDG_PERIOD_IN_US         2000000UL
#define HAL_APP_WDG                  HPM_EWDG0
static uint8_t wdg_init_flag = 0;
/**
 * @description: 
 * @return {*}
 */
void hal_wdg_feed(void)
{
    if (wdg_init_flag)
    {
        ewdg_refresh(HAL_APP_WDG);
    }
}

/**
 * @description: 
 * @return {*}
 */
void hal_wdg_init(void)
{
    ewdg_config_t config;
    ewdg_get_default_config(HAL_APP_WDG, &config);

    /* Enable EWDG */
    config.enable_watchdog = true;
    /* Enable EWDG Timeout Reset */
    config.int_rst_config.enable_timeout_reset = true;
    config.int_rst_config.enable_refresh_violation_reset = true;
    config.int_rst_config.enable_refresh_unlock_fail_reset = true;
    config.ctrl_config.use_lowlevel_timeout = false;
    /* Set EWDG Count clock source to OSC32 */
    config.ctrl_config.cnt_clk_sel = ewdg_cnt_clk_src_ext_osc_clk;

    config.ctrl_config.enable_window_mode = false;
    config.ctrl_config.window_lower_limit = ewdg_window_lower_timeout_period_8_div_16;
    config.ctrl_config.window_upper_limit = ewdg_window_upper_timeout_period_8_div_16;

    /* Set the EWDG reset timeout to 101ms */
    uint32_t ewdg_src_clk_freq = EWDG_CNT_CLK_FREQ;

    config.cnt_src_freq = ewdg_src_clk_freq;
    config.ctrl_config.timeout_reset_us = 8000UL * 1000UL;

    /* Initialize the WDG */
    hpm_stat_t status = ewdg_init(HAL_APP_WDG, &config);
    if (status != status_success)
    {
        DBG_PRINTF(" EWDG initialization failed, error_code=%d\n", status);
    }

    board_timer_create(HAL_WDG_PERIOD_IN_US, hal_wdg_feed);
    wdg_init_flag = 1;
}

/**
 * @description: 
 * @return {*}
 */
void hal_wdg_manual_feed(void)
{
    if (wdg_init_flag)
    {
	    ewdg_refresh(HAL_APP_WDG);
    }
}
