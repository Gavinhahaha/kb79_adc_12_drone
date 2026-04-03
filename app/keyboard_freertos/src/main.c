/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "board.h"
#include <stdio.h>

#include "hal_adc.h"     
#include "hal_flash.h"

#include "hal_timer.h"
#include "hal_gpio.h"
#include "hal_spi.h"
#include "hal_wdg.h"
#include "hal_flash.h"
#include "mg_matrix.h"
#include "mg_hive.h"
#include "mg_uart.h"
#include "rgb_led_action.h"
#include "db.h"
#include "mg_hid.h"
#include "dfu_api.h"
#include "layout_fn.h"
#include "app_debug.h"
#include "mg_matrix.h"
#include "usb_osal.h"
#include "usb_config.h"
#include "app_config.h"
#include "hpm_dma_mgr.h"
#include "mg_indev.h"
#include "mg_sm.h"
#include "mg_detection.h"


#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #undef  LOG_TAG
    #define LOG_TAG          "main"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

int main(void)
{
    board_init();
    db_init();
    rgb_led_init();
    dma_mgr_init();
    hal_gpio_init();
    hal_spi_init();
    mg_uart_init();
    hive_init();
    mg_sm_init();
    layout_fn_init();
    matrix_init();
    mg_indev_init();
    mg_detectiopn_init();
    mg_hid_init();
    hal_wdg_init();
    DBG_PRINTF("sys init.\n");

    vTaskStartScheduler();
    return 0;
}


