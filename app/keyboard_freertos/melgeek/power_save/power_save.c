#include "power_save.h"
#include "mg_hall.h"
#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "app_debug.h"
#include "hpm_clock_drv.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "power_save"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#define LOW_PWR_ENABLED  0

static uint8_t working_mode = NORMAL_MODE;
static TimerHandle_t timer_handle = NULL;
static uint8_t timer_reset_flag = 0;


static void timer_callback(TimerHandle_t xTimer)
{
    (void)(xTimer);

    if (LOW_PWR_MODE != working_mode)
    {
        working_mode = LOW_PWR_MODE;
        #if LOW_PWR_ENABLED == 1
        hall_driver_switch_scan_mode(working_mode);
        clock_set_source_divider(clock_cpu0, clk_src_pll0_clk0, 6U);
        clock_update_core_clock();
        DBG_PRINTF("cpu0:\t\t %luHz\n", clock_get_frequency(clock_cpu0));
        #endif
    }
}

void ps_timer_reset(void)
{
    if ((1 == timer_reset_flag) && (NULL != timer_handle))
    {
        if (xTimerIsTimerActive(timer_handle) == pdTRUE)
        {
            xTimerReset(timer_handle, 0);
        }
        
        if (NORMAL_MODE != working_mode)
        {
            working_mode = NORMAL_MODE;
            #if LOW_PWR_ENABLED == 1
            hall_driver_switch_scan_mode(working_mode);
            clock_set_source_divider(clock_cpu0, clk_src_pll0_clk0, 2U);
            clock_update_core_clock();
            DBG_PRINTF("cpu0:\t\t %luHz\n", clock_get_frequency(clock_cpu0));
            #endif
        }
        timer_reset_flag = 0;
    }
}

void ps_timer_stop(void)
{
    if (NULL != timer_handle)
    {
        xTimerStop(timer_handle, 0);
    }
}

void ps_timer_init(bool en, uint16_t period)
{
    uint32_t period_ms = (period < 60 ? 60 : period) * 1000;

    timer_handle = xTimerCreate("CheckModeTimer", pdMS_TO_TICKS(period_ms), pdTRUE, 0, timer_callback);
    working_mode = NORMAL_MODE;

    if (NULL == timer_handle)
    {
        DBG_PRINTF("check mode timer creation failure\r\n");
        return;                            
    }

    if (true == en)
    {
        if (xTimerStart(timer_handle, 0) != pdPASS)
        {
            DBG_PRINTF("timer start failure!\r\n");
        }
    }
}

uint8_t ps_timer_update(bool en, uint16_t period)
{
    uint8_t ret = 1;

    if (NULL != timer_handle)
    {
        uint32_t period_ms = (period < 60 ? 60 : period) * 1000;

        if (true == en)
        {
            if (xTimerChangePeriod(timer_handle, pdMS_TO_TICKS(period_ms), 0) != pdPASS)
            {
                return 0;
            }

            if (xTimerIsTimerActive(timer_handle) != pdTRUE)
            {
                if (xTimerStart(timer_handle, 0) != pdPASS)
                {
                    return 0;
                }
            }
        }
        else
        {
            if (xTimerIsTimerActive(timer_handle) == pdTRUE)
            {
                if (xTimerStop(timer_handle, 0) != pdPASS)
                {
                    return 0;
                }
            }
        }
    }
    else
    {
        return 0;
    }
    
    return ret;
}

void ps_timer_reset_later(void)
{
    if (timer_reset_flag != 1)
    {
        timer_reset_flag = 1;
    }
}

uint8_t ps_get_kb_working_mode(void)
{
    return working_mode;
}
