/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include "FreeRTOS.h"
#include "task.h"
#include "mg_matrix.h"
#include "hal_hid.h"
#include "hal_gpio.h"
#include "board.h"
#include "mg_matrix_type.h"
#include "app_debug.h"
#include "mg_hid.h"
#include "app_config.h"
#include "rgb_led_ctrl.h"
#include "hpm_trgm_drv.h"

#include "hpm_gptmr_drv.h"
#include "hpm_gpio_drv.h"



#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "mg_hid"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif


static PFN_SOF_SYNC_CALLBACK key_sync_callback = NULL;
static TaskHandle_t m_hid_thread;
QueueHandle_t hid_queue = NULL;
static bool is_hid_busy = false;
extern uint32_t cpu_time;
extern uint32_t get_end_time(void);


static void hid_thread(void *pvParameters)
{
    (void)pvParameters;
    rep_hid_t *p_report = NULL;
    
    xTaskNotifyGive(m_hid_thread);
    
    while (1)
    {
        //等待usb就绪
        if (!hal_hid_is_ready())
        {
            vTaskDelay(10);
            continue;
        }
    
        while (1)
        {
            if ( xQueueReceive( hid_queue, &(p_report), portMAX_DELAY ) == pdPASS )
            {
                if ((p_report == NULL) || (p_report->report_id == 0) || (p_report->report_id > REPORT_ID_MAX))
                {
                    DBG_PRINTF("invalid pointer: %p, clear queue\n", p_report);
                    xQueueReset(hid_queue); // 清空队列
                    continue;
                }

                if ( ulTaskNotifyTake(pdTRUE, 2) != pdFAIL )
                {
                    hal_hid_send_report(p_report);
                    is_hid_busy = true;
                }
                else
                {
                    DBG_PRINTF("hid_thread:time out=%d,busy=%d\n", p_report->report_id, is_hid_busy);
                    //break;
                    hal_hid_send_report(p_report);
                    //is_hid_busy = false;
                    /* TODO: hid_thread hid state err */
                }
            }
        }
    }
}

hid_ret mg_hid_send_queue(rep_hid_t *p_report)
{
    uint32_t msg = (uint32_t)p_report;
    
    if (!hal_hid_is_ready())
    {
        DBG_PRINTF("mg_hid_send_queue:hid busy\n");
        return HID_RET_W_BUSY;
    }
    
    if (xQueueSend(hid_queue, (const void * const )&msg, portMAX_DELAY) != pdPASS )
    {
        DBG_PRINTF("mg_hid_send_queue:full=%d\n", p_report->report_id);
        return HID_RET_W_QUEUE_FULL;
    }
    
    return HID_RET_SUCEESS;
}


void mg_hid_in_finish(void)
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    static uint8_t init_flg = 0;
    if (init_flg == 0)
    {
         //DBG_PRINTF("m");
         init_flg = 1;
        //hall_driver_auto_start_enable(true);
    }
    is_hid_busy = false;
    //cpu_time = get_end_time();
    ///DBG_PRINTF("run clk=%d,%d us\r\n", cpu_time,cpu_time/480);
    vTaskNotifyGiveFromISR(m_hid_thread, &higherPriorityTaskWoken);

    portYIELD_FROM_ISR( higherPriorityTaskWoken );
}

void mg_hid_led_report(uint8_t *pbuf, uint16_t size)
{
    uint8_t led;
    
    if (size > 1)
    {
        led = (*((uint16_t *)pbuf)) >> 8;
    }
    else
    {
        led = *(uint8_t *)pbuf;
    }
    led_set_sys_led_status(led);
    DBG_PRINTF("mg_hid_led_report:%d\n", led);
}

extern void hall_driver_auto_start_enable(bool enable);

#define SOF_FILTER_MS (8 * 1000) //2s, 8ms per tick, 2000 ticks = 16s

uint32_t counter_value[10];
uint32_t counter_idx = 0;
uint32_t tuning_cnt = 0;
uint32_t tuning_flg = 0;
static uint16_t sof_filter_tick = 0;

uint16_t hid_sof_sync = 2;

void mg_hid_sof_sync(void)
{
    if (key_sync_callback)
    {
        key_sync_callback();
    }
    //hall_driver_auto_start_enable(true);
/*
    counter_value[counter_idx] = gptmr_channel_get_counter(HPM_GPTMR1, 3, gptmr_counter_type_normal);
    if (counter_value[counter_idx] > 1700)
    {
        if (tuning_cnt == 0)
        {
            gptmr_channel_config_update_reload(HPM_GPTMR1, 3, 2082);
            //gptmr_channel_config_update_reload(HPM_GPTMR1, 2, 2082);
            tuning_cnt = 100;
            tuning_flg = 1;
            
        }
    }
    
    if (counter_value[counter_idx] < 1300)
    {
        if (tuning_cnt == 0)
        {
            gptmr_channel_config_update_reload(HPM_GPTMR1, 3, 2084);
            //gptmr_channel_config_update_reload(HPM_GPTMR1, 2, 2084);
            tuning_cnt = 100;
            tuning_flg = 1;
            
        }
    }
    
    if (tuning_cnt)
    {
        
        //gptmr_channel_config_update_reload(HPM_GPTMR1, 2, 2083);
        tuning_cnt--;
        if (tuning_cnt == 0)
        {
            if (tuning_flg ==1)
        {
        gptmr_channel_config_update_reload(HPM_GPTMR1, 3, 2083);
        tuning_flg = 0;
        }
            //gptmr_channel_config_update_reload(HPM_GPTMR1, 3, 2083);
        }
    }
    
    //counter_idx++;
    //counter_idx%=10;
    
    #if 0
    static uint8_t init_flg = 0;
    if (init_flg == 0)
    {
         //DBG_PRINTF("m");
         init_flg = 1;
        hall_driver_auto_start_enable(true);
    }
    #endif
    */
}

void hal_key_sync_callback(PFN_SOF_SYNC_CALLBACK pfn_cb)
{
    key_sync_callback = pfn_cb;
}

void mg_hid_sof_sync_filter(void)
{
    if ((++sof_filter_tick) > SOF_FILTER_MS)
    {
        sof_filter_tick = 0;
        hal_hid_set_sof_callback(mg_hid_sof_sync);
    }
}

void mg_hid_sof_sync_start(void)
{
    hal_hid_set_sof_callback(mg_hid_sof_sync_filter);
}

void mg_hid_sof_sync_stop(void)
{
    hal_hid_set_sof_callback(NULL);
    hal_notify_enable(false);
}

void mg_hid_init(void)
{
    hal_hid_set_cb_in(mg_hid_in_finish);
    hid_queue = xQueueCreate(10, sizeof(uint32_t));
    if (hid_queue == NULL)
    {
        DBG_PRINTF("mg_hid_init:hid_queue creat failed\n");
        while(1)
        {
            vTaskDelay(100);
        }
    }
    
    hal_hid_set_cb_st(mg_hid_led_report);
    mg_hid_sof_sync_start();
    hal_hid_init();
    if (xTaskCreate(hid_thread, "hid_thread", STACK_SIZE_HID, NULL, PRIORITY_HID, &m_hid_thread) != pdPASS)
    {
        DBG_PRINTF("hid_thread creation failed!.\n");
        for (;;)
        {
            ;
        }
    }
}

