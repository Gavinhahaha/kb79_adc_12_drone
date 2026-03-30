/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
/* FreeRTOS kernel includes. */

#include "mg_indev.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_debug.h"

#if 0
#include "drv_button.h"
#include "drv_encoder.h"
#else
#include "drv_dpm.h"
#endif

#include "app_config.h"

#include "db.h"
#include "mg_matrix.h"
#include "layout_fn.h"
#include "power_save.h"



#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "mg_indev"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#define KEY_CODE_RELEASE_TIMEOUT           pdMS_TO_TICKS(20)

static TaskHandle_t mg_indev_handle = NULL; 
static TimerHandle_t key_code_release_timer = NULL;
static uint8_t index_key_code = 0;
static uint8_t last_bklb = 0;
static light_tape_config_t last_amblb_config = {0};
static kc_t custom_key = {0};
static bool is_custom_key_rpt = false;

static const kc_t key_code_table[KEY_CODE_MAX] = 
{
    [KEY_CODE_HOME] = _B(KC_HOME),
    [KEY_CODE_PGUP] = _B(KC_PGUP),
    [KEY_CODE_END]  = _B(KC_END),
    [KEY_CODE_PGDN] = _B(KC_PGDOWN),
    [KEY_CODE_VOLD] = _S(KC_VOLD),
    [KEY_CODE_MUTE] = _S(KC_MUTE),
    [KEY_CODE_VOLU] = _S(KC_VOLU),
    [KEY_CODE_MPRV] = _S(KC_MPRV),
    [KEY_CODE_MNXT] = _S(KC_MNXT),
    [KEY_CODE_MPLY] = _S(KC_MPLY),
};

static void key_code_release_timer_cb(TimerHandle_t xTimer)
{
    (void)xTimer;

    if (is_custom_key_rpt)
    {
        is_custom_key_rpt = false;
        layout_fn_active_key_code_rpt(custom_key, KEY_STATUS_U2U);
    }
    else
    {
        layout_fn_active_key_code_rpt(key_code_table[index_key_code], KEY_STATUS_U2U);
    }
}

void mg_indev_handler(uint32_t evt, uint8_t *param)
{
    if ((INEDV_EVT_KEY_CODE_RPT == evt))
    {
        if (*param >= KEY_CODE_MAX)
        {
            return;
        }
        index_key_code = *param;
    }
    else if ((INEDV_EVT_CUSTOM_KEY_RPT == evt))
    {
        kc_t tmp_kc = *(kc_t *)param;
        if ((tmp_kc.en != KCE_ENABLE) ||
            (tmp_kc.ex != 0)          ||
            (tmp_kc.ty != KCT_KB)     ||
            (tmp_kc.sty != 0)         ||
            (tmp_kc.co > KC_RGUI)     ||
            (tmp_kc.co < KC_A))
        {
            return;
        }
        custom_key = tmp_kc;
    }

    if (xPortIsInsideInterrupt() != pdFALSE)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xTaskNotifyFromISR(mg_indev_handle, evt, eSetBits, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    else
    {
        xTaskNotify(mg_indev_handle, evt, eSetBits);
    }
}


static void mg_indev_task(void *p_context)
{
    (void)p_context;

    uint32_t notified_events;

    while (1)
    {
        if (xTaskNotifyWait(0x00,        // 不清除进入前的位
                            0xFFFFFFFF,  // 清除退出时所有位
                            &notified_events,
                            portMAX_DELAY) == pdTRUE)
        {
            ps_timer_reset_later();
            if (notified_events & INEDV_EVT_KEY_CODE_RPT)
            {
                if (!xTimerIsTimerActive(key_code_release_timer))
                {   
                    layout_fn_active_key_code_rpt(key_code_table[index_key_code], KEY_STATUS_U2D);
                    xTimerReset(key_code_release_timer, 0);
                }
                DBG_PRINTF("indev_evt: INEDV_EVT_KEY_CODE_RPT\n");
            }

            if (notified_events & INEDV_EVT_CUSTOM_KEY_RPT)
            {
                if (!xTimerIsTimerActive(key_code_release_timer))
                {   
                    is_custom_key_rpt = true;
                    layout_fn_active_key_code_rpt(custom_key, KEY_STATUS_U2D);
                    xTimerReset(key_code_release_timer, 0);
                }
                DBG_PRINTF("indev_evt: INEDV_EVT_CUSTOM_KEY_RPT\n");
            }

            if (notified_events & INEDV_EVT_BKL_BRI_TOG)
            {
                if (sk_la_lm_kc_info.la_table[gbinfo.lm.cur_lmm].hl != 0)
                {
                    last_bklb = sk_la_lm_kc_info.la_table[gbinfo.lm.cur_lmm].hl;
                    sk_la_lm_kc_info.la_table[gbinfo.lm.cur_lmm].hl = 0;
                }
                else
                {
                    sk_la_lm_kc_info.la_table[gbinfo.lm.cur_lmm].hl = last_bklb;
                    if (sk_la_lm_kc_info.la_table[gbinfo.lm.cur_lmm].hl == 0)
                    {
                        sk_la_lm_kc_info.la_table[gbinfo.lm.cur_lmm].hl = 21;//默认亮度
                    }
                }

                db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
                rgb_led_set_all();
                DBG_PRINTF("indev_evt: INEDV_EVT_BKL_BRI_TRG\n");
            }

            if (notified_events & INEDV_EVT_AMB_BACK_BRI_TOG)
            {
                if ((gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h != 0) || 
                    (gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s != 0) || 
                    (gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v != 0))
                {
                    last_amblb_config = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md];
                    gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h = 0;
                    gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s = 0;
                    gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v = 0;
                }
                else
                {
                    gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md] = last_amblb_config;

                    if ((gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h == 0) && 
                    (gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s == 0) && 
                    (gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v == 0))
                    {
                        switch (gbinfo.rl[LIGHT_TAPE_PART_BACK].md)
                        {
                            case RGBLIGHT_EFFECT_RAINBOW_MOOD:
                            case RGBLIGHT_EFFECT_RAINBOW_SWIRL:
                            case RGBLIGHT_EFFECT_CHRISTMAS:
                            case RGBLIGHT_EFFECT_ROTATE:
                                gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h = RGBLIGHT_DEFAULT_HUE;
                                gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s = 255;
                                gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v = RGBLIGHT_DEFAULT_VAL;
                                gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].speed = RGBLIGHT_DEFAULT_SPD;
                                break;

                            default:
                                gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h = RGBLIGHT_DEFAULT_HUE;
                                gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s = RGBLIGHT_DEFAULT_SAT;
                                gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v = RGBLIGHT_DEFAULT_VAL;
                                gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].speed = RGBLIGHT_DEFAULT_SPD;
                                break;
                        }
                    }
                }
                db_update(DB_SYS, UPDATE_LATER);
                DBG_PRINTF("indev_evt: INEDV_EVT_AMB_BACK_BRI_TRG\n");
            }

            if (notified_events & INEDV_EVT_AMB_SIDE_BRI_TOG)
            {
                if ((gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h != 0) || 
                    (gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s != 0) || 
                    (gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v != 0))
                {
                    last_amblb_config = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md];
                    gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h = 0;
                    gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s = 0;
                    gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v = 0;
                }
                else
                {
                    gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md] = last_amblb_config;

                    if ((gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h == 0) && 
                    (gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s == 0) && 
                    (gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v == 0))
                    {
                        switch (gbinfo.rl[LIGHT_TAPE_PART_SIDE].md)
                        {
                            case RGBLIGHT_EFFECT_RAINBOW_MOOD:
                            case RGBLIGHT_EFFECT_RAINBOW_SWIRL:
                            case RGBLIGHT_EFFECT_CHRISTMAS:
                            case RGBLIGHT_EFFECT_ROTATE:
                                gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h = RGBLIGHT_DEFAULT_HUE;
                                gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s = 255;
                                gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v = RGBLIGHT_DEFAULT_VAL;
                                gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].speed = RGBLIGHT_DEFAULT_SPD;
                                break;

                            default:
                                gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h = RGBLIGHT_DEFAULT_HUE;
                                gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s = RGBLIGHT_DEFAULT_SAT;
                                gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v = RGBLIGHT_DEFAULT_VAL;
                                gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].speed = RGBLIGHT_DEFAULT_SPD;
                                break;
                        }
                    }
                }
                db_update(DB_SYS, UPDATE_LATER);
                DBG_PRINTF("indev_evt: INEDV_EVT_AMB_SIDE_BRI_TRG\n");
            }

            if (notified_events & INEDV_EVT_SWITCH_CFG)
            {
                db_send_toggle_cfg_signal();
                DBG_PRINTF("indev_evt: INEDV_EVT_SWITCH_CFG\n");
            }

            if (notified_events & INEDV_EVT_OPEN_CN_WEBSITE)
            {
                macro_open_website(CN_WEBSITE);
                DBG_PRINTF("indev_evt: INEDV_EVT_OPEN_CN_WEBSITE\n");
            }

            if (notified_events & INEDV_EVT_OPEN_EN_WEBSITE)
            {
                macro_open_website(EN_WEBSITE);
                DBG_PRINTF("indev_evt: INEDV_EVT_OPEN_EN_WEBSITE\n");
            }
        }
    }
}

void mg_indev_init(void)
{
    #if 0
    drv_encoder_init(mg_indev_handler);
    drv_button_init(mg_indev_handler);
    #else
    drv_dpm_gpio_init();
    #endif

    key_code_release_timer = xTimerCreate("enc_rel",
                                         KEY_CODE_RELEASE_TIMEOUT, 
                                         pdFALSE,             
                                         NULL,
                                         key_code_release_timer_cb);

    if (key_code_release_timer == NULL)
    {
        DBG_PRINTF("key code release timer creation failed!\r\n");
    }

    if (pdPASS != xTaskCreate(mg_indev_task, "mg_indev", STACK_SIZE_INDEV, NULL, PRIORITY_INDEV, &mg_indev_handle))
    {
        DBG_PRINTF("mg_indev task creation failure\r\n");
    }
}