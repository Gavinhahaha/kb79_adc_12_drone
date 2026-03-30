/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include "hal_timer.h"
#include "hal_gpio.h"
#include "board.h"
#include "hpm_gptmr_drv.h"
#ifndef ADC_SOC_NO_HW_TRIG_SRC
#include "hpm_trgm_drv.h"
#include "hpm_trgmmux_src.h"
#endif
#include "app_debug.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "hal_timer"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

/* CALLBACK TIMER section */
#define APP_CALLBACK_TIMER            (HPM_GPTMR1)
#define APP_CALLBACK_TIMER_CH         1
#define APP_CALLBACK_TIMER_IRQ        IRQn_GPTMR1
#define APP_CALLBACK_TIMER_CLK_NAME   (clock_gptmr1)

#define APP_BOARD_GPTMR               BOARD_GPTMR
#define APP_BOARD_GPTMR_CH            BOARD_GPTMR_CHANNEL
#define APP_BOARD_GPTMR_IRQ           BOARD_GPTMR_IRQ
#define APP_BOARD_GPTMR_CLOCK         BOARD_GPTMR_CLK_NAME

#ifndef ADC_SOC_NO_HW_TRIG_SRC
#define APP_BOARD_CMP_MS               (5)
#define APP_BOARD_GPTMR_CMP            (0)
#define APP_BOARD_GPTMR_CMP1           (1)
#define APP_BOARD_GPTMR_CMPINIT        (0) /*it's mean compare output start state, now is low*/
#define APP_TICK_MS                    (15)
#define APP_TICK_UNIT                  (1000000)
#else
#define APP_BOARD_CMP_MS            (15)
#define APP_TICK_TIMEOUT            APP_BOARD_CMP_MS
#define APP_TICK_UNIT               (1000000)

#endif

static PFN_HAL_TIMER_CALLBACK g_hal_timer_timeout_callback = NULL;
static PFN_HAL_TIMER_CALLBACK g_hal_tmr_timeout_callback = NULL;

/**
 * @description: 
 * @return {*}
 */
void hal_timer_set_timeout_callback(PFN_HAL_TIMER_CALLBACK pfn_callback)
{
    g_hal_timer_timeout_callback = pfn_callback;
}

void hal_timer_isr(void)
{
    if (gptmr_check_status(APP_CALLBACK_TIMER, GPTMR_CH_RLD_STAT_MASK(APP_CALLBACK_TIMER_CH)))
    {
        gptmr_clear_status(APP_CALLBACK_TIMER, GPTMR_CH_RLD_STAT_MASK(APP_CALLBACK_TIMER_CH));
        if (g_hal_tmr_timeout_callback)
        {
            g_hal_tmr_timeout_callback();
        }
    }
}
SDK_DECLARE_EXT_ISR_M(APP_CALLBACK_TIMER_IRQ, hal_timer_isr);

/**
 * @description:
 * @return {*}
 */
void hal_timer_sw_init(uint32_t time_us, PFN_HAL_TIMER_CALLBACK pfn_callback)
{
    uint32_t gptmr_freq;
    gptmr_channel_config_t config;

    g_hal_tmr_timeout_callback = pfn_callback;
    gptmr_channel_get_default_config(APP_CALLBACK_TIMER, &config);

    clock_add_to_group(APP_CALLBACK_TIMER_CLK_NAME, 0);
    gptmr_freq = clock_get_frequency(APP_CALLBACK_TIMER_CLK_NAME);

    config.reload = gptmr_freq / 1000 / 1000 * time_us;
    gptmr_channel_config(APP_CALLBACK_TIMER, APP_CALLBACK_TIMER_CH, &config, false);
    gptmr_enable_irq(APP_CALLBACK_TIMER, GPTMR_CH_RLD_IRQ_MASK(APP_CALLBACK_TIMER_CH));
    intc_m_enable_irq_with_priority(APP_CALLBACK_TIMER_IRQ, 1);

}

/**
 * @description:
 * @return {*}
 */
void hal_timer_sw_stop(void)
{
    gptmr_disable_irq(APP_CALLBACK_TIMER, GPTMR_CH_RLD_IRQ_MASK(APP_CALLBACK_TIMER_CH));
    gptmr_stop_counter(APP_CALLBACK_TIMER, APP_CALLBACK_TIMER_CH);
    gptmr_channel_reset_count(APP_CALLBACK_TIMER, APP_CALLBACK_TIMER_CH);
}

/**
 * @description:
 * @return {*}
 */
void hal_timer_sw_start(void)
{
    gptmr_channel_reset_count(APP_CALLBACK_TIMER, APP_CALLBACK_TIMER_CH);
    gptmr_start_counter(APP_CALLBACK_TIMER, APP_CALLBACK_TIMER_CH);
    gptmr_enable_irq(APP_CALLBACK_TIMER, GPTMR_CH_RLD_IRQ_MASK(APP_CALLBACK_TIMER_CH));
}

/**
 * @description: 
 * @return {*}
 */
void hal_tick_isr(void)
{
    if (gptmr_check_status(APP_BOARD_GPTMR, GPTMR_CH_RLD_STAT_MASK(APP_BOARD_GPTMR_CH)))
    {
        gptmr_clear_status(APP_BOARD_GPTMR, GPTMR_CH_RLD_STAT_MASK(APP_BOARD_GPTMR_CH));

        if (g_hal_timer_timeout_callback)
        {
            g_hal_timer_timeout_callback();
        }
    }
}
SDK_DECLARE_EXT_ISR_M(APP_BOARD_GPTMR_IRQ, hal_tick_isr);

/**
 * @description: 
 * @return {*}
 */

#ifndef ADC_SOC_NO_HW_TRIG_SRC
void hal_timer_hw_init(uint32_t time_us)
{

    init_gptmr_pins(APP_BOARD_GPTMR);

    gptmr_channel_config_t config;
    uint32_t gptmr_freq;
    gptmr_channel_get_default_config(APP_BOARD_GPTMR, &config);
    gptmr_freq = clock_get_frequency(APP_BOARD_GPTMR_CLOCK);
    config.reload = gptmr_freq / APP_TICK_UNIT * time_us;
    config.cmp[APP_BOARD_GPTMR_CMP] = gptmr_freq / APP_TICK_UNIT * APP_BOARD_CMP_MS;
    config.cmp[APP_BOARD_GPTMR_CMP1] = gptmr_freq / APP_TICK_UNIT * (time_us + 1);
    config.cmp_initial_polarity_high = APP_BOARD_GPTMR_CMPINIT;

    // gptmr_enable_irq(APP_BOARD_GPTMR, GPTMR_CH_CMP_IRQ_MASK(APP_BOARD_GPTMR_CH, APP_BOARD_GPTMR_CMP));
    gptmr_channel_config(APP_BOARD_GPTMR, APP_BOARD_GPTMR_CH, &config, false);
    gptmr_channel_reset_count(APP_BOARD_GPTMR, APP_BOARD_GPTMR_CH);
}
#else
void hal_timer_hw_init(uint32_t time_us)
{

    init_gptmr_pins(APP_BOARD_GPTMR);

    uint32_t gptmr_freq;
    gptmr_channel_config_t config;

    gptmr_channel_get_default_config(APP_BOARD_GPTMR, &config);

    gptmr_freq = clock_get_frequency(APP_BOARD_GPTMR_CLOCK);
    config.reload = gptmr_freq / APP_TICK_UNIT * time_us;
    gptmr_channel_config(APP_BOARD_GPTMR, APP_BOARD_GPTMR_CH, &config, false);
    intc_m_enable_irq_with_priority(APP_BOARD_GPTMR_IRQ, 1);
}

#endif
/**
 * @description: 
 * @return {*}
 */
void hal_timer_config_start(uint32_t time_us, PFN_HAL_TIMER_CALLBACK pfn_cb)
{
    hal_timer_set_timeout_callback(pfn_cb);

    uint32_t gptmr_freq;
    gptmr_channel_config_t config;

    gptmr_channel_get_default_config(APP_BOARD_GPTMR, &config);

    gptmr_freq = clock_get_frequency(APP_BOARD_GPTMR_CLOCK);
    config.reload = gptmr_freq / APP_TICK_UNIT * time_us;
    gptmr_channel_config(APP_BOARD_GPTMR, APP_BOARD_GPTMR_CH, &config, false);
    //gptmr_channel_reset_count(APP_BOARD_GPTMR, APP_BOARD_GPTMR_CH);

    gptmr_start_counter(APP_BOARD_GPTMR, APP_BOARD_GPTMR_CH);

#ifdef ADC_SOC_NO_HW_TRIG_SRC
    gptmr_enable_irq(APP_BOARD_GPTMR, GPTMR_CH_RLD_IRQ_MASK(APP_BOARD_GPTMR_CH));
#endif

}

/**
 * @description:
 * @return {*}
 */
void hal_timer_start(void)
{
    gptmr_channel_reset_count(APP_BOARD_GPTMR, APP_BOARD_GPTMR_CH);
    gptmr_start_counter(APP_BOARD_GPTMR, APP_BOARD_GPTMR_CH);
#ifdef ADC_SOC_NO_HW_TRIG_SRC
    gptmr_enable_irq(APP_BOARD_GPTMR, GPTMR_CH_RLD_IRQ_MASK(APP_BOARD_GPTMR_CH));
#endif
}
/**
 * @description: 
 * @return {*}
 */
void hal_timer_stop(void)
{

    gptmr_channel_reset_count(APP_BOARD_GPTMR, APP_BOARD_GPTMR_CH);
#ifdef ADC_SOC_NO_HW_TRIG_SRC
    gptmr_disable_irq(APP_BOARD_GPTMR, GPTMR_CH_RLD_IRQ_MASK(APP_BOARD_GPTMR_CH));
#endif
    gptmr_stop_counter(APP_BOARD_GPTMR, APP_BOARD_GPTMR_CH);
}

