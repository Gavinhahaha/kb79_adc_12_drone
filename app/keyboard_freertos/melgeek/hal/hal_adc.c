/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include "FreeRTOS.h"
#include "task.h"
#include "hal_adc.h"
#include "hal_gpio.h"
#include "board.h"
#include "hpm_adc16_drv.h"
#ifndef ADC_SOC_NO_HW_TRIG_SRC
#include "hpm_trgm_drv.h"
#include "hpm_trgmmux_src.h"
#endif
#include "app_debug.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "hal_adc"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#define DOUBLE_ADC
#define BATCH_DMA
#define STABILITY_TEST

#define APP_ADC16_CH_SAMPLE_CYCLE             (20U)//1920K
#define APP_ADC16_CH_WDOG_EVENT               (1 << BOARD_APP_ADC0_CH_1)
#define APP_ADC16_SEQ_IRQ_EVENT               adc16_event_seq_single_complete
#define APP1_ADC16_CH_WDOG_EVENT              (1 << BOARD_APP_ADC1_CH_1)


#ifndef APP_ADC16_CORE
#define APP_ADC16_CORE                        BOARD_RUNNING_CORE
#endif

static PFN_HAL_CAPTURE_CALLBACK g_hal_adc0_capture_callback = NULL;
static PFN_HAL_CAPTURE_CALLBACK g_hal_adc1_capture_callback = NULL;

static __IO uint8_t  s_trig_complete_flag;
static __IO uint32_t s_res_out_of_adc0_thr_flag;
static __IO uint32_t s_res_out_of_adc1_thr_flag;


void hal_adc0_set_capture_callback(PFN_HAL_CAPTURE_CALLBACK pfn_callback)
{
    g_hal_adc0_capture_callback = pfn_callback;
}
void hal_adc1_set_capture_callback(PFN_HAL_CAPTURE_CALLBACK pfn_callback)
{
    g_hal_adc1_capture_callback = pfn_callback;
}

SDK_DECLARE_EXT_ISR_M(BOARD_APP_ADC0_IRQn, isr_adc0)
void isr_adc0(void)
{
    uint32_t status;

    status = adc16_get_status_flags(BOARD_APP_ADC0_BASE);

    /* Clear status */
    adc16_clear_status_flags(BOARD_APP_ADC0_BASE, status);

    if (ADC16_INT_STS_SEQ_CVC_GET(status))
    {
        /* Set flag to read memory data */
        if (g_hal_adc0_capture_callback)
        {
            g_hal_adc0_capture_callback();
        }
    }

    if (ADC16_INT_STS_TRIG_CMPT_GET(status))
    {
        /* Set flag to read memory data */
        s_trig_complete_flag = 1;
    }

    if (ADC16_INT_STS_WDOG_GET(status) & APP_ADC16_CH_WDOG_EVENT)
    {
        adc16_disable_interrupts(BOARD_APP_ADC0_BASE, APP_ADC16_CH_WDOG_EVENT);
        s_res_out_of_adc0_thr_flag = ADC16_INT_STS_WDOG_GET(status) & APP_ADC16_CH_WDOG_EVENT;
    }
}


/**
 * @description: 
 * @param {adc16_conversion_mode_t} conv_mode
 * @return {*}
 */
static hpm_stat_t init_adc0_common_config(adc16_conversion_mode_t conv_mode)
{
    adc16_config_t cfg;

    /* initialize an ADC instance */
    adc16_get_default_config(&cfg);

    cfg.res = adc16_res_16_bits;
    cfg.conv_mode = conv_mode;
    cfg.adc_clk_div = adc16_clock_divider_4;
    cfg.sel_sync_ahb = (clk_adc_src_ahb0 == clock_get_source(BOARD_APP_ADC0_CLK_NAME)) ? true : false;

    if (cfg.conv_mode == adc16_conv_mode_sequence ||
        cfg.conv_mode == adc16_conv_mode_preemption)
    {
        cfg.adc_ahb_en = true;
    }

    /* adc16 initialization */
    if (adc16_init(BOARD_APP_ADC0_BASE, &cfg) == status_success)
    {
        /* enable irq */
        intc_m_enable_irq_with_priority(BOARD_APP_ADC0_IRQn, 1);
        return status_success;
    }
    else
    {
        //DBG_PRINTF("%s initialization failed!\n", BOARD_APP_ADC16_NAME);
        return status_fail;
    }
}

void channel_result_out_of_threshold_handler(void)
{
    adc16_channel_threshold_t threshold;
    uint32_t i = 31;

    if (s_res_out_of_adc0_thr_flag)
    {
        while (i--)
        {
            if ((s_res_out_of_adc0_thr_flag >> i) & 0x01)
            {
                adc16_get_channel_threshold(BOARD_APP_ADC0_BASE, i, &threshold);
            }
        }

        s_res_out_of_adc0_thr_flag = 0;
        adc16_enable_interrupts(BOARD_APP_ADC0_BASE, APP_ADC16_CH_WDOG_EVENT);
    }
}

void init_period_config(void)
{
    adc16_channel_config_t ch_cfg;
    adc16_prd_config_t prd_cfg;

    /* get a default channel config */
    adc16_get_channel_default_config(&ch_cfg);

    /* initialize an ADC channel */
    ch_cfg.ch           = BOARD_APP_ADC0_CH_1;
    ch_cfg.sample_cycle = APP_ADC16_CH_SAMPLE_CYCLE;

    adc16_init_channel(BOARD_APP_ADC0_BASE, &ch_cfg);

    prd_cfg.ch           = BOARD_APP_ADC0_CH_1;
    prd_cfg.prescale     = 22;    /* Set divider: 2^22 clocks */
    prd_cfg.period_count = 5;     /* 104.86ms when AHB clock at 200MHz is ADC clock source */

    adc16_set_prd_config(BOARD_APP_ADC0_BASE, &prd_cfg);
}

uint16_t hal_adc_volt_value_get(void)
{
    uint16_t result;
    channel_result_out_of_threshold_handler();
    adc16_get_prd_result(BOARD_APP_ADC0_BASE, BOARD_APP_ADC0_CH_1, &result);
    //DBG_PRINTF("Period Mode - %s [channel %02d] - Result: 0x%04x\n", BOARD_APP_ADC0_NAME, BOARD_APP_ADC0_CH_1, result);
    return result;
}

void hal_adc_init(void)
{

    /* ADC pin initialization */
    board_init_adc16_pins();

    /* ADC clock initialization */
    board_init_adc16_clock(BOARD_APP_ADC0_BASE, true);

    /* ADC16 common initialization */
    init_adc0_common_config(adc16_conv_mode_sequence);

    init_period_config();

}
