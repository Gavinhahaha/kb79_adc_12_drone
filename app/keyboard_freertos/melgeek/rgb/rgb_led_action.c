/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include "FreeRTOS.h"
#include "task.h"
#include "rgb_led_action.h"
#include "rgb_led_ctrl.h"
#include "rgb_led_color.h"
#include "app_config.h"
#include "app_debug.h"
#include "db.h"
#include "hal_hid.h"
#include "hal_i2c.h"
#include "rgb_light_tape.h"
#include "hpm_gpio_drv.h"
#include "power_save.h"
#include "rgb_lamp_array.h"
#include "rgb_lamp_sync.h"
#include "mg_build_date.h"
#include "mg_uart.h"
#include "mg_detection.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "rgb_led_action"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#define RGB_TASK_PERIOD_MS      (10)
#define STARTUP_TICKS_PER_STEP  (2)
#define INDICATOR_KEEP_MS       (5000)
#define DELAY_CNT_THRESHOLD     (800)
#define RGB_STATE_ON            (1)
#define RGB_STATE_OFF           (0)

#define LED_VALID_ACTIONS_MASK ((1 << LED_ACTION_RIPPLE) |   \
                                (1 << LED_ACTION_REACTION) | \
                                (1 << LED_ACTION_RAY) |      \
                                (1 << LED_ACTION_RAIN))
#define IS_VALID_LED_ACTION(action)     ((1 << (action)) & LED_VALID_ACTIONS_MASK)


typedef struct 
{
    rgb_sm_state_e state;
    uint8_t        startup_step;
    uint8_t        startup_cnt;
    uint8_t        startup_end;
    bool           indicator_comp;
    TickType_t     indicator_tick;
    bool           need_refresh;
    uint8_t        frame_toogle;
    prompt_type_e  prompt_type;
    uint8_t        prompt_repeat;
    uint8_t        key_idx;
    uint8_t        key_evt;
    uint16_t       delay_10ms_cnt;
    factory_rgb_e  factory_rgb;
    uint8_t        unit;
} rgb_sm_ctx_t;

static rgb_sm_ctx_t g_sm_ctx = 
{
    .state        = STATE_STARTUP,
    .need_refresh = true,
    .startup_end  = 0,
    .prompt_type  = PROMPT_MAX,
};

static TaskHandle_t m_rgb_thread;

static void rgb_led_lamp_set_all(void);
static void rgb_led_sync_set_all(void);

static void rgb_led_calc_action_set(uint8_t li, const led_action_t *pla, key_tmp_sta_t *pkts)
{
    if (pkts->ltdy > 0)
    {
        --pkts->ltdy;
        return;
    }

    switch (pkts->ltst)
    {
        case LED_STA_START:
        {
            pkts->ltl  = pla->bd ? pkts->lhl : pkts->lll;
            pkts->lltl = pkts->ltl;
            pkts->ltts = (pkts->lhl - pkts->lll) << 1;

            if (LED_CYC_ENABLE != pla->cy && pkts->lttpt == 0)
            {
                pkts->ltst = LED_STA_STOP;
            }
            else
            {
                pkts->ltst = pla->bd ? LED_STA_H2L : LED_STA_L2H;
            }
            break;
        }
        case LED_STA_L2H:
        {
            if (pkts->ltts > 0)
            {
                pkts->ltts -= (pkts->ltts >= pkts->lls) ? pkts->lls : pkts->ltts;
                if ((pkts->ltl + pkts->lls) <= pkts->lhl)
                {
                    pkts->ltl += pkts->lls;
                    pkts->ltdy = pkts->lhzc;
                }
                else
                {
                    pkts->ltl  = pkts->lhl;
                    pkts->ltdy = pkts->lhdy;
                    pkts->ltst = LED_STA_H2L;
                }
            }
            else
            {
                pkts->ltst = LED_STA_STOP;
            }
            break;
        }
        case LED_STA_H2L:
        {
            if (pkts->ltts > 0)
            {
                pkts->ltts -= (pkts->ltts >= pkts->lls) ? pkts->lls : pkts->ltts;

                if (pkts->ltl >= pkts->lls && (pkts->ltl - pkts->lls) >= pkts->lll)
                {
                    pkts->ltl -= pkts->lls;
                    pkts->ltdy = pkts->lhzc;
                }
                else
                {
                    pkts->ltl  = pkts->lll;
                    pkts->ltst = LED_STA_L2H;
                }
            }
            else
            {
                pkts->ltst = LED_STA_STOP;
            }
            break;
        }
        case LED_STA_PAUSE:
        {
            break;
        }
        case LED_STA_STOP:
        {
            if (LED_CYC_ENABLE == pla->cy)
            {
                pkts->ltl  = pkts->lltl;
                pkts->ltdy = pkts->lpdy;
                pkts->ltst = LED_STA_START;
            }
            else
            {
                if (pkts->lttpt > 0)
                {
                    --pkts->lttpt;
                    pkts->ltl  = pkts->lltl;
                    pkts->ltdy = pkts->lpdy;
                    pkts->ltts = (pkts->lhl - pkts->lll) << 1;
                    pkts->ltst = LED_STA_START;
                }
                else
                {
                    pkts->ltl   = pkts->lltl;
                    pkts->lttpt = pkts->ltpt;
                    pkts->ltdy  = 0;
                    pkts->ltst  = LED_STA_NONE;
                }
            }
            break;
        }
        default:
        {
            pkts->ltst = LED_STA_NONE;
            break;
        }
    }

    led_set_color(li, pkts->ltl, pkts->ltkrr, pkts->ltkrg, pkts->ltkrb);
}

static void axis_startup_effect(uint8_t step)
{
    const uint8_t num = BOARD_KEY_LED_NUM;
    const uint8_t r = 120, g = 120, b = 120;
    const uint8_t center_left = (num - 1) / 2;
    const uint8_t center_right = num / 2;
    const uint8_t expand_steps = center_right + 1;

    if (step == 0)
    {
        for (uint8_t i = 0; i < num; ++i)
        {
            led_set_color_dircet(i, 0, 0, 0, 0);
        }
    }

    if (step < expand_steps)
    {
        uint8_t offset = step;
        int16_t left_idx = center_left - offset;
        int16_t right_idx = center_right + offset;

        if (left_idx >= 0 && left_idx < num)
        {
            led_set_color_dircet(left_idx, 60, r, g, b);
        }

        if (right_idx < num && right_idx != left_idx)
        {
            led_set_color_dircet(right_idx, 60, r, g, b);
        }
    }
    else
    {
        uint8_t fade_offset = step - expand_steps;
        int16_t left_idx = fade_offset;
        int16_t right_idx = num - 1 - fade_offset;

        if (left_idx >= 0 && left_idx < num)
        {
            led_set_color_dircet(left_idx, 60, 0, 0, 0);
        }

        if (right_idx < num && right_idx != left_idx && right_idx >= 0)
        {
            led_set_color_dircet(right_idx, 60, 0, 0, 0);
        }
    }
}

static void state_startup_proc(void) 
{
    if (!gbinfo.startup_effect_en) 
    {
        g_sm_ctx.state = STATE_NORMAL;
        return;
    }
    else
    {
        if (1 == g_sm_ctx.startup_end)
        {
            g_sm_ctx.state = STATE_NORMAL;
            return;
        }
        if (++g_sm_ctx.startup_cnt >= STARTUP_TICKS_PER_STEP) 
        {
            g_sm_ctx.startup_cnt = 0;
            uint16_t step = g_sm_ctx.startup_step++;
            uint16_t ambient_count = BOARD_LIGHT_TAPE_BACK_COL_NUM + BOARD_LIGHT_TAPE_SIDE_LED_NUM;
            uint16_t count = ambient_count > BOARD_KEY_LED_NUM ? ambient_count : BOARD_KEY_LED_NUM;
            light_tape_startup_effect(step, ambient_count, RGBLIGHT_EFFECT_WAKE_MAX, RGBLIGHT_EFFECT_WAKE_MAX, RGBLIGHT_EFFECT_WAKE_MAX);
            axis_startup_effect(step);
            if (g_sm_ctx.startup_step > count)
            {
                g_sm_ctx.state        = STATE_NORMAL;
                g_sm_ctx.startup_end  = 1;
                g_sm_ctx.startup_step = 0;
            }
        }
    }
}

static void handle_key_event(void)
{
    if (!IS_VALID_LED_ACTION(gbinfo.lm.cur_lmm))
    {
        g_sm_ctx.key_evt = KEY_STATUS_MAX;
        return;
    }
    if (g_sm_ctx.key_evt == KEY_STATUS_MAX)
    {
        return;
    }
    if (gbinfo.lm.cur_lmm == LED_ACTION_RAIN)
    {
        g_sm_ctx.key_evt = KEY_STATUS_MAX;
        return;
    }

    uint8_t num = 1;
    if (g_sm_ctx.key_idx > BOARD_SPACE_KEY_INDEX)
    {
        g_sm_ctx.key_idx += 2;
    }
    else if (g_sm_ctx.key_idx == BOARD_SPACE_KEY_INDEX)
    {
        num = 3;
    }

    key_led_color_t *pkr = get_key_led_color_ptr(gbinfo.kr.cur_krm, g_sm_ctx.key_idx);
    if (pkr->m != MODE_MASK) // U2D
    {
        g_sm_ctx.key_evt = KEY_STATUS_MAX;
        return;
    }

    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(g_sm_ctx.key_idx);
    led_action_t *pla   = &gmx.pla[gbinfo.lm.cur_lmm];
    const led_action_handle_t *lah = get_led_action_handle_ptr(pla->id);

    if (lah->keyp == NULL) 
    {
        g_sm_ctx.key_evt = KEY_STATUS_MAX;
        return;
    }

    for (uint8_t i = 0; i < num; i++) 
    {
        switch (g_sm_ctx.key_evt) 
        {
            case KEY_STATUS_U2U: 
                if ((pkts + i)->kps == KEY_STATUS_U2D) 
                {
                    (pkts + i)->kps = KEY_STATUS_U2U;
                    if ((pkts + i)->kpht <= KPHT_1000MS) 
                    {
                        (pkts + i)->kcs = KCS_SP;
                    }
                    lah->keyp(g_sm_ctx.key_idx + i, pla);
                }
                break;

            case KEY_STATUS_U2D: 
                if ((pkts + i)->kps == KEY_STATUS_U2U) 
                {
                    (pkts + i)->kps = KEY_STATUS_U2D;
                    (pkts + i)->kcs = KCS_NONE;
                    (pkts + i)->kpht = 0;
                    lah->keyp(g_sm_ctx.key_idx + i, pla);
                    (pkts + i)->kpht += 3;
                }
                break;

            case KEY_STATUS_D2D: 
            case KEY_STATUS_D2U: 
                break;

            default:
                break;
        }
    }

    g_sm_ctx.key_evt = KEY_STATUS_MAX;
}

void rgb_led_key_evt_update(uint8_t ki, uint8_t kps)
{
    if (!IS_VALID_LED_ACTION(gbinfo.lm.cur_lmm))
    {
        return;
    }

    g_sm_ctx.key_idx = ki;
    g_sm_ctx.key_evt = kps;
}

static void state_normal_proc(void) 
{
    static uint8_t led_swap_update = 0;

    if (!rgb_led_allow_open())
    {
        if (!hal_hid_is_ready())
        {
            if (gbinfo.power_save.en)
            {
                if (++g_sm_ctx.delay_10ms_cnt > DELAY_CNT_THRESHOLD)
                {
                    g_sm_ctx.prompt_type    = PROMPT_PC_SLEEP;
                    g_sm_ctx.state          = STATE_PROMPT;
                    g_sm_ctx.delay_10ms_cnt = 0;
                    return;
                }
            }
        }
        else if (ps_get_kb_working_mode() != NORMAL_MODE)
        {
            g_sm_ctx.prompt_type = PROMPT_KB_SLEEP;
            g_sm_ctx.state = STATE_PROMPT;
            return;
        }
    }
    else
    {
        g_sm_ctx.delay_10ms_cnt = 0;
    }
#ifdef WINDOWS_LAMP
    if (get_rgb_lamp_mode()) 
    {
        g_sm_ctx.state = STATE_LAMP;
        return;
    }
#endif

    handle_key_event();

    if (gbinfo.lm.cur_lmm == LED_ACTION_RAIN)
    {
        static uint16_t active_count = 0;
        if (active_count++ > (rand() % 1000))
        {
            uint8_t random = rand() % BOARD_LED_COLUMN_NUM;
            led_action_t *pla = &gmx.pla[gbinfo.lm.cur_lmm];
            const led_action_handle_t *lah = get_led_action_handle_ptr(pla->id);
            key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(random);
            if (pkts->ltst == LED_STA_NONE)
            {
                lah->keyp(random, pla);
            }
            active_count = 0;
        }
    }

    if (g_sm_ctx.need_refresh) 
    {
        rgb_led_set_all();
        led_refresh_sys_led();
        g_sm_ctx.need_refresh = false;
    }
    uint8_t led_swap = led_get_sys_led();
    if (led_swap != led_swap_update)
    {
        uint8_t buf[2] = {0, led_swap};
        uart_notify_screen_cmd(CMD_UART_KB_PARAM, SCMD_SET_KB_INFO, buf, sizeof(buf));
        led_swap_update = led_swap;
    }
    

    for (int i = 0; i < BOARD_KEY_LED_NUM; ++i) 
    {
        key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(i);
        if (pkts->ltst == LED_STA_NONE) continue;

        led_action_t *pla = &gmx.pla[gbinfo.lm.cur_lmm];

        key_led_color_t *pkr = get_key_led_color_ptr(gbinfo.kr.cur_krm, i);
        uint8_t action_id = get_led_action_id(pkr->m);
        if (pkr->m == MODE_MASK)
        {
            rgb_led_calc_action_set(i, pla, pkts);
            const led_action_handle_t *laction_ptr = get_led_action_handle_ptr(pla->id);
            if (laction_ptr != NULL && laction_ptr->proc != NULL)
            {
                laction_ptr->proc(i, &gbinfo.lm.cur_lmm, NULL, NULL);
            }
        }
        else if (action_id < LED_ACTION_MAX)
        {
            pla = &gmx.pla[action_id];
            if (pkr->m == pla->at)
            {
                rgb_led_calc_action_set(i, pla, pkts);
                const led_action_handle_t *laction_ptr = get_led_action_handle_ptr(pla->id);
                if (laction_ptr != NULL && laction_ptr->proc != NULL)
                {
                    laction_ptr->proc(i, &pkr->m, NULL, NULL);
                    
                }
            }
        }

    }

    if (!g_sm_ctx.indicator_comp) 
    {
        if (xTaskGetTickCount() - g_sm_ctx.indicator_tick > pdMS_TO_TICKS(INDICATOR_KEEP_MS)) 
        {
            g_sm_ctx.indicator_comp = true;
            led_refresh_sys_led();
        }
    }

    for (light_tape_part_e part = LIGHT_TAPE_PART_BACK; part < LIGHT_TAPE_PART_MAX; part++)
    {
        light_tape_handler(false, part);
        if (LIGHT_TAPE_WHOLE == gbinfo.rl_shape)
        {
            break;
        }
    }
    rgblight_flush();
    light_tape_update_sync_timer(RGB_TASK_PERIOD_MS);

    if (g_sm_ctx.frame_toogle) 
    {
        bool ret = led_update_frame(INTERRUPT_MODE);
        led_set_update_state(ret ? STATE_IDLE : STATE_READY);
    }
    g_sm_ctx.frame_toogle = !g_sm_ctx.frame_toogle;
}

static void state_lamp_proc(void) 
{
    if (!rgb_led_allow_open())
    {
        if (!hal_hid_is_ready())
        {
            g_sm_ctx.prompt_type = PROMPT_PC_SLEEP;
        }
        else if (ps_get_kb_working_mode() != NORMAL_MODE)
        {
            g_sm_ctx.prompt_type = PROMPT_KB_SLEEP;
        }
        g_sm_ctx.state = STATE_PROMPT;
        return;
    }

#ifndef WINDOWS_LAMP
    g_sm_ctx.state = STATE_NORMAL;
    return;
#endif
    if (!get_rgb_lamp_mode()) 
    {
        g_sm_ctx.state = STATE_NORMAL;
        return;
    }

    if (g_sm_ctx.need_refresh) 
    {
        rgb_led_set_all();
        g_sm_ctx.need_refresh = false;
    }

    lamp_color *p = get_rgb_lamp_rgbi();
    for (uint8_t i = 0; i < BOARD_KEY_LED_NUM; ++i, ++p) 
    {
        key_led_color_t *c = get_lamp_led_color_ptr(i);
        c->r = p->red; c->g = p->green; c->b = p->blue; c->m = p->intensity;
    }
    rgb_led_lamp_set_all();
    rgb_light_tape_lamp_set_all((uint8_t(*)[4])p);

    if (g_sm_ctx.frame_toogle) 
    {
        bool ret = led_update_frame(INTERRUPT_MODE);
        led_set_update_state(ret ? STATE_IDLE : STATE_READY);
    }
    g_sm_ctx.frame_toogle = !g_sm_ctx.frame_toogle;
}

static void state_sync_rgb_proc(void)
{
    link_mode_t link = g_sm_ctx.unit;
    color_t *p = get_rgb_sync_pixels();

    if (!rgb_led_allow_open())
    {
        if (!hal_hid_is_ready())
        {
            g_sm_ctx.prompt_type = PROMPT_PC_SLEEP;
        }
        else if (ps_get_kb_working_mode() != NORMAL_MODE)
        {
            g_sm_ctx.prompt_type = PROMPT_KB_SLEEP;
        }
        g_sm_ctx.state = STATE_PROMPT;
        return;
    }

    if (link & LINK_MODE_KEY_LED)
    {
        for (uint8_t i = 0; i < BOARD_KEY_LED_NUM; ++i, ++p)
        {
            key_led_color_t *c = get_lamp_led_color_ptr(i);
            c->r = p->red;
            c->g = p->green;
            c->b = p->blue;
        }
        rgb_led_sync_set_all();
    }
    else
    {
        static uint8_t led_swap_update = 0;

        if (g_sm_ctx.need_refresh)
        {
            rgb_led_set_all();
            g_sm_ctx.need_refresh = false;
        }

        handle_key_event();

        if (gbinfo.lm.cur_lmm == LED_ACTION_RAIN)
        {
            static uint16_t active_count = 0;
            if (active_count++ > (rand() % 1000))
            {
                uint8_t random = rand() % BOARD_LED_COLUMN_NUM;
                led_action_t *pla = &gmx.pla[gbinfo.lm.cur_lmm];
                const led_action_handle_t *lah = get_led_action_handle_ptr(pla->id);
                key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(random);
                if (pkts->ltst == LED_STA_NONE)
                {
                    lah->keyp(random, pla);
                }
                active_count = 0;
            }
        }

        if (g_sm_ctx.need_refresh)
        {
            rgb_led_set_all();
            led_refresh_sys_led();
            g_sm_ctx.need_refresh = false;
        }
        uint8_t led_swap = led_get_sys_led();
        if (led_swap != led_swap_update)
        {
            uint8_t buf[2] = {0, led_swap};
            uart_notify_screen_cmd(CMD_UART_KB_PARAM, SCMD_SET_KB_INFO, buf, sizeof(buf));
            led_swap_update = led_swap;
        }

        for (int i = 0; i < BOARD_KEY_LED_NUM; ++i)
        {
            key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(i);
            if (pkts->ltst == LED_STA_NONE)
                continue;

            led_action_t *pla = &gmx.pla[gbinfo.lm.cur_lmm];

            key_led_color_t *pkr = get_key_led_color_ptr(gbinfo.kr.cur_krm, i);
            uint8_t action_id = get_led_action_id(pkr->m);
            if (pkr->m == MODE_MASK)
            {
                rgb_led_calc_action_set(i, pla, pkts);
                const led_action_handle_t *laction_ptr = get_led_action_handle_ptr(pla->id);
                if (laction_ptr != NULL && laction_ptr->proc != NULL)
                {
                    laction_ptr->proc(i, &gbinfo.lm.cur_lmm, NULL, NULL);
                }
            }
            else if (action_id < LED_ACTION_MAX)
            {
                pla = &gmx.pla[action_id];
                if (pkr->m == pla->at)
                {
                    rgb_led_calc_action_set(i, pla, pkts);
                    const led_action_handle_t *laction_ptr = get_led_action_handle_ptr(pla->id);
                    if (laction_ptr != NULL && laction_ptr->proc != NULL)
                    {
                        laction_ptr->proc(i, &pkr->m, NULL, NULL);
                    }
                }
            }
        }

        if (!g_sm_ctx.indicator_comp)
        {
            if (xTaskGetTickCount() - g_sm_ctx.indicator_tick > pdMS_TO_TICKS(INDICATOR_KEEP_MS))
            {
                g_sm_ctx.indicator_comp = true;
                led_refresh_sys_led();
            }
        }


        p += BOARD_KEY_LED_NUM;
    }

    if (link & LINK_MODE_REAR_AMBIENT)
    {
        rgb_light_tape_sync_set_rear((uint8_t (*)[3])p);
    }
    else
    {
        light_tape_handler(true, LIGHT_TAPE_PART_BACK);
    }
    p += BOARD_LIGHT_TAPE_BACK_LED_NUM;

    if (link & LINK_MODE_SIDE_AMBIENT)
    {
        rgb_light_tape_sync_set_side((uint8_t (*)[3])p);
    }
    else
    {
        light_tape_handler(true, LIGHT_TAPE_PART_SIDE);
    }
    rgblight_flush();
    light_tape_update_sync_timer(RGB_TASK_PERIOD_MS);

    if (g_sm_ctx.frame_toogle)
    {
        led_update_frame(INTERRUPT_MODE);
        // led_set_update_state(ret ? STATE_IDLE : STATE_READY);
    }
    g_sm_ctx.frame_toogle = !g_sm_ctx.frame_toogle;
}


static void state_prompt_proc(void) 
{
    if (PROMPT_PC_SLEEP == g_sm_ctx.prompt_type)
    {
        led_set_all_color(LED_BRIGHTNESS_MAX, 0, 0, 255);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(0, 0, 255);
        vTaskDelay(300);

        led_set_all_color(LED_BRIGHTNESS_MIN, 0, 0, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(0, 0, 0);
        vTaskDelay(300);

        led_set_all_color(LED_BRIGHTNESS_MAX, 0, 0, 255);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(0, 0, 255);
        vTaskDelay(300);

        led_set_all_color(LED_BRIGHTNESS_MIN, 0, 0, 0);
        led_update_frame(QUERY_MODE);
        CAPSLOCK_LED_SET_OFF();
        light_tape_set_all(0, 0, 0);

        uint8_t mode = RGB_STATE_OFF;
        uart_ask_screen_cmd(CMD_UART_DPM_MODE, SCMD_SET_DPM_INFO, &mode, sizeof(mode));
        g_sm_ctx.need_refresh = true;
        g_sm_ctx.state        = STATE_OFF;
    }
    else if (PROMPT_KB_SLEEP == g_sm_ctx.prompt_type)
    {
        led_set_all_color(LED_BRIGHTNESS_MIN, 0, 0, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(0, 0, 0);

        uint8_t mode = RGB_STATE_OFF;
        uart_ask_screen_cmd(CMD_UART_DPM_MODE, SCMD_SET_DPM_INFO, &mode, sizeof(mode));
        g_sm_ctx.need_refresh = true;
        g_sm_ctx.state        = STATE_OFF;
    }
    else if (PROMPT_KB_WARNING_WATER == g_sm_ctx.prompt_type)
    {
        led_set_all_color(LED_BRIGHTNESS_MAX, 255, 128, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(255, 128, 0);
        vTaskDelay(300);

        led_set_all_color(LED_BRIGHTNESS_MIN, 0, 0, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(0, 0, 0);
        vTaskDelay(300);

        led_set_all_color(LED_BRIGHTNESS_MAX, 255, 128, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(255, 128, 0);
        vTaskDelay(300);

        led_set_all_color(LED_BRIGHTNESS_MIN, 0, 0, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(0, 0, 0);
        vTaskDelay(300);

        if (!(mg_detecte_get_err_status() & SM_WATER_ERROR))
        {
            g_sm_ctx.need_refresh = true;
            g_sm_ctx.state        = STATE_NORMAL;
        }
    }
    else if (PROMPT_KB_WARNING_USB_VOLT == g_sm_ctx.prompt_type)
    {
        led_set_all_color(LED_BRIGHTNESS_MAX, 255, 128, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(255, 128, 0);
        vTaskDelay(300);

        led_set_all_color(LED_BRIGHTNESS_MIN, 0, 0, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(0, 0, 0);
        vTaskDelay(300);

        led_set_all_color(LED_BRIGHTNESS_MAX, 255, 128, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(255, 128, 0);
        vTaskDelay(300);

        led_set_all_color(LED_BRIGHTNESS_MIN, 0, 0, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(0, 0, 0);
        vTaskDelay(300);

        if ((!(mg_detecte_get_err_status() & SM_VOLT_LOW_ERROR)) || (!mg_detecte_get_volt_warn_check_state()))
        {
            g_sm_ctx.need_refresh = true;
            g_sm_ctx.state        = STATE_NORMAL;
        }
    }
    else if (PROMPT_KB_RESET == g_sm_ctx.prompt_type)
    {
        led_set_all_color(LED_BRIGHTNESS_MAX, 255, 0, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(255, 0, 0);
        vTaskDelay(300);

        led_set_all_color(LED_BRIGHTNESS_MIN, 0, 0, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(0, 0, 0);
        vTaskDelay(300);

        led_set_all_color(LED_BRIGHTNESS_MAX, 255, 0, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(255, 0, 0);
        vTaskDelay(300);

        led_set_all_color(LED_BRIGHTNESS_MIN, 0, 0, 0);
        led_update_frame(QUERY_MODE);
        
        light_tape_set_all(0, 0, 0);

        uart_notify_screen_cmd(CMD_UART_SYS, SCMD_RESET, NULL, 0);
        vTaskDelay(100);

        g_sm_ctx.state = STATE_UNINIT;
    }
    else if (PROMPT_DFU_FLASH_START == g_sm_ctx.prompt_type)
    {
        led_set_all_color(LED_BRIGHTNESS_MAX, 255, 0, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(0, 0, 0);
    }
    else if (PROMPT_DFU_FLASH_PROGRESS == g_sm_ctx.prompt_type)
    {
        led_set_all_color(LED_BRIGHTNESS_MAX, 255, 0, 0);
        led_update_frame(QUERY_MODE);
    }
    else if (PROMPT_DFU_FLASH_COMPLETE == g_sm_ctx.prompt_type)
    {
        led_set_all_color(LED_BRIGHTNESS_MAX, 0, 255, 0);
        led_update_frame(QUERY_MODE);
        g_sm_ctx.prompt_type = PROMPT_MAX;
    }
    else if (PROMPT_DFU_SLAVE_FLASH_DOWNLOAD == g_sm_ctx.prompt_type)
    {
        light_tape_set_all(255, 0, 0);
    }
    else if (PROMPT_DFU_SLAVE_FLASH_COMPLETE == g_sm_ctx.prompt_type)
    {
        light_tape_set_all(0, 255, 0);
    }
    else if (PROMPT_DFU_SLAVE_CHECKOUT == g_sm_ctx.prompt_type)
    {
        led_set_all_color(LED_BRIGHTNESS_MAX, 255, 255, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(255, 255, 0);
    }
    else if (PROMPT_DFU_SLAVE_INVALID == g_sm_ctx.prompt_type)
    {
        led_set_all_color(LED_BRIGHTNESS_MAX, 0, 0, 205);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(0, 0, 205);
        vTaskDelay(200);
    }
    else if (PROMPT_DFU_SLAVE_ABORT == g_sm_ctx.prompt_type)
    {
        led_set_all_color(LED_BRIGHTNESS_MAX, 255, 165, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(255, 165, 0);
        vTaskDelay(200);
    }
    else if (PROMPT_DFU_SLAVE_FLASH_ABORT == g_sm_ctx.prompt_type)
    {
        light_tape_set_all(255, 0, 255);
        vTaskDelay(200);
    }
    else if (PROMPT_DFU_SLAVE_DISCONN == g_sm_ctx.prompt_type)
    {
        light_tape_set_all(0, 0, 0);
        vTaskDelay(200);
        light_tape_set_all(255, 255, 0);
        vTaskDelay(200);
    }
    else if (PROMPT_NETBAR_MODE_ENABLE == g_sm_ctx.prompt_type)
    {
        led_set_all_color(LED_BRIGHTNESS_MAX, 128, 0, 128);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(128, 0, 128);
        vTaskDelay(300);

        led_set_all_color(LED_BRIGHTNESS_MIN, 0, 0, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(0, 0, 0);
        vTaskDelay(300);

        led_set_all_color(LED_BRIGHTNESS_MAX, 128, 0, 128);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(128, 0, 128);
        vTaskDelay(300);

        led_set_all_color(LED_BRIGHTNESS_MIN, 0, 0, 0);
        led_update_frame(QUERY_MODE);
        light_tape_set_all(0, 0, 0);
        vTaskDelay(300);

        uart_notify_screen_cmd(CMD_UART_SYS, SCMD_RESET, NULL, 0);
        vTaskDelay(100);

        g_sm_ctx.state = STATE_UNINIT;
    }
}

void rgb_led_set_prompt(prompt_type_e type)
{
    if (type < PROMPT_MAX)
    {
       g_sm_ctx.prompt_type = type;
       g_sm_ctx.state       = STATE_PROMPT;
    }
}

void rgb_led_get_prompt(rgb_sm_state_e *state, prompt_type_e *type)
{
    *type = g_sm_ctx.prompt_type;
    *state = g_sm_ctx.state;
}

void rgb_led_set_normal(void)
{
    g_sm_ctx.state = STATE_NORMAL;
    g_sm_ctx.need_refresh = true;
}

void rgb_led_set_uninit(void)
{
    if (STATE_UNINIT != g_sm_ctx.state)
    {
        g_sm_ctx.state = STATE_UNINIT;
    }
}

void rgb_led_set_factory(factory_rgb_e type)
{
    if (type < FACTORY_RGB_MAX)
    {
        g_sm_ctx.factory_rgb = type;
        g_sm_ctx.state       = STATE_FACTORY;
    }
}
static void state_factory_proc(void)
{
    switch (g_sm_ctx.factory_rgb)
    {
        case FACTORY_RGB_NONE:
            break;

        case FACTORY_RGB_ENTER:
            if ((gbinfo.lm.cur_lmm != LED_ACTION_BRIGHT) ||
                (gbinfo.rl[LIGHT_TAPE_PART_BACK].md != RGBLIGHT_EFFECT_STATIC) ||
                (gbinfo.rl[LIGHT_TAPE_PART_SIDE].md != RGBLIGHT_EFFECT_STATIC))
            {
                gbinfo.lm.cur_lmm = LED_ACTION_BRIGHT;
                gbinfo.rl[LIGHT_TAPE_PART_BACK].md = RGBLIGHT_EFFECT_STATIC;
                gbinfo.rl[LIGHT_TAPE_PART_SIDE].md = RGBLIGHT_EFFECT_STATIC;
                rgb_led_set_all();
                led_update_frame(QUERY_MODE);
                for (light_tape_part_e part = LIGHT_TAPE_PART_BACK; part < LIGHT_TAPE_PART_MAX; part++)
                {
                    light_tape_handler(false, part);
                    if (LIGHT_TAPE_WHOLE == gbinfo.rl_shape)
                    {
                        break;
                    }
                }
                rgblight_flush();
                light_tape_update_sync_timer(RGB_TASK_PERIOD_MS);
            }
            break;

        case FACTORY_RGB_UPDATE:
            led_update_frame(QUERY_MODE);
            for (light_tape_part_e part = LIGHT_TAPE_PART_BACK; part < LIGHT_TAPE_PART_MAX; part++)
            {
                light_tape_handler(false, part);
                if (LIGHT_TAPE_WHOLE == gbinfo.rl_shape)
                {
                    break;
                }
            }
            rgblight_flush();
            light_tape_update_sync_timer(RGB_TASK_PERIOD_MS);
            g_sm_ctx.factory_rgb = FACTORY_RGB_NONE;
            break;

        case FACTORY_RGB_EXIT:
            gbinfo.lm.cur_lmm = LED_ACTION_GRADUAL;
            gbinfo.rl[LIGHT_TAPE_PART_BACK].md = RGBLIGHT_DEFAULT_MODE;
            gbinfo.rl[LIGHT_TAPE_PART_SIDE].md = RGBLIGHT_DEFAULT_MODE;
            g_sm_ctx.state        = STATE_NORMAL;
            g_sm_ctx.need_refresh = true;
            break;

        default: break;
    }
}

static void state_off_proc(void) 
{
    if (rgb_led_allow_open())
    {
        uint8_t mode = RGB_STATE_ON;
        uart_ask_screen_cmd(CMD_UART_DPM_MODE, SCMD_SET_DPM_INFO, &mode, sizeof(mode));

        if ((NETBAR_MODE_EN == common_info.netbar_info.mode_en) && (PROMPT_PC_SLEEP == g_sm_ctx.prompt_type))
        {
            common_info.netbar_info.mode_active = 1;
            db_reset();
            db_send_toggle_cfg_signal();
        }

        if (gbinfo.startup_effect_en && g_sm_ctx.startup_end)
        {
            g_sm_ctx.state       = STATE_STARTUP;
            g_sm_ctx.startup_end = 0;
            return;
        }

        g_sm_ctx.state = STATE_NORMAL;
        g_sm_ctx.need_refresh = true;
    }
}

static void state_uninit_proc(void) 
{
    led_uninit();
    board_sw_reset();
}

void rgb_led_toggle_cfg_complete_prompt(uint8_t cfg_id) 
{
    static const uint8_t color[4][3] = 
    {
        {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0}
    };

    if (cfg_id >= 4) return;
    
    led_indicator_led_ctrl(LED_BRIGHTNESS_MID, color[cfg_id][0], color[cfg_id][1], color[cfg_id][2]);

    g_sm_ctx.indicator_comp = false;
    g_sm_ctx.indicator_tick = xTaskGetTickCount();

}

bool rgb_led_allow_open(void)
{
    return hal_hid_is_ready() && (ps_get_kb_working_mode() == NORMAL_MODE);
}

static void rgb_led_task(void * p_context)
{
    (void)(p_context);

    while (1)
    {
        switch (g_sm_ctx.state)
        {
            case STATE_STARTUP:
            {
                state_startup_proc();
                break;
            }
            case STATE_NORMAL:
            {
                state_normal_proc();
                break;
            }
            case STATE_LAMP:
            {
                state_lamp_proc();
                break;
            }
            case STATE_SYNC:
            {
                state_sync_rgb_proc();
                break;
            }
            case STATE_PROMPT:
            {
                state_prompt_proc();
                break;
            }
            case STATE_FACTORY:
            {
                state_factory_proc();
                break;
            }
            case STATE_OFF:
            {
                state_off_proc();
                break;
            }
            case STATE_UNINIT:
            {
                state_uninit_proc();
                break;
            }
            default: break;
        }

        ps_timer_reset();
        vTaskDelay(10);
    }
}

void rgb_led_task_start(void)
{
    if (m_rgb_thread)
    {
        g_sm_ctx.frame_toogle = 0;
        vTaskResume(m_rgb_thread);        
    }
}

void rgb_led_task_stop(void)
{
    if (m_rgb_thread)
    {
        g_sm_ctx.frame_toogle = 0;
        led_set_update_state(STATE_IDLE);
        vTaskSuspend(m_rgb_thread);
    }
}

static void rgb_led_lamp_set_all(void)
{

    for (int i = 0; i < BOARD_KEY_LED_NUM; i++)
    {
        key_led_color_t *p_rgbl = get_lamp_led_color_ptr(i);

        led_lamp_set_rgb(i, p_rgbl->m, p_rgbl->r, p_rgbl->g, p_rgbl->b);
    }
}

static void rgb_led_sync_set_all(void)
{

    for (int i = 0; i < BOARD_KEY_LED_NUM; i++)
    {
        key_led_color_t *p_rgbl = get_lamp_led_color_ptr(i);

        led_lamp_set_rgb(i, 255, p_rgbl->r, p_rgbl->g, p_rgbl->b);
    }
}

void rgb_led_set_all(void)
{

    for (int i = 0; i < BOARD_KEY_LED_NUM; i++)
    {
        key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(i);
        key_led_color_t *pkr = get_key_led_color_ptr(gbinfo.kr.cur_krm, i);
        uint8_t action_id = get_led_action_id(pkr->m);
        if (pkr->m == MODE_MASK)
        {
            led_action_t *pla = &gmx.pla[gbinfo.lm.cur_lmm];
            const led_action_handle_t *lah = get_led_action_handle_ptr(pla->id);
            if ((pla == NULL) || (lah == NULL) || (pkts == NULL))
            {
                return;
            }
            lah->init(i, pla);
        }
        else if (action_id < LED_ACTION_MAX)
        {
            led_action_t *pla = &gmx.pla[action_id];
            const led_action_handle_t *lah = get_led_action_handle_ptr(pla->id);
            if ((pla == NULL) || (lah == NULL) || (pkts == NULL))
            {
                return;
            }
            lah->init(i, pla);
        }

        led_set_color(i, pkts->ltl, pkts->ltkrr, pkts->ltkrg, pkts->ltkrb);
    }
}

void rgb_led_set_one(uint8_t li)
{   
    led_action_t *pla   = &gmx.pla[gbinfo.lm.cur_lmm];
    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(li);
    
    if (pla == NULL)
    {
        return;
    }
    const led_action_handle_t *laction_ptr = get_led_action_handle_ptr(pla->id);
    if (laction_ptr != NULL && laction_ptr->init != NULL)
    {
        laction_ptr->init(li, pla);
    }
    led_set_color(li, pkts->ltl, pkts->ltkrr, pkts->ltkrg, pkts->ltkrb);
}

void rgb_led_unset_all(void)
{
    uint8_t action_id = get_led_action_id(LED_ACTION_NONE);
    led_action_t la = gmx.pla[action_id];
    const led_action_handle_t *laht = NULL;

    for (int i = 0; i < BOARD_KEY_LED_NUM; ++i)
    {
        laht = get_led_action_handle_ptr(la.id);
        if (laht != NULL && laht->init != NULL)
        {
            laht->init(i, &la);
        }
        led_set_color(i, LED_BRIGHTNESS_MIN, 0, 0, 0);
    }
}

static void rgb_led_task_notify(void)
{
    if(xPortIsInsideInterrupt() == pdTRUE)
    {
        BaseType_t yield_req = pdFALSE;
        vTaskNotifyGiveFromISR(m_rgb_thread, &yield_req);
        portYIELD_FROM_ISR(yield_req);	
    }
    else
    {
        xTaskNotifyGive(m_rgb_thread);
    }
}

static void rgb_led_wait_trans_complete(void)
{
    if (m_rgb_thread)
    {
        (void) ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(portMAX_DELAY));
    }
}

void rgb_led_check_para(key_led_color_info_t *para)
{
    const uint32_t *color_ptr = get_rgb_layer_color();

    if (para)
    {
        for (int i = 0; i < BOARD_LED_COLOR_LAYER_MAX; i++)
        {
            for (int j = 0; j < BOARD_KEY_LED_NUM; j++)
            {
                if (para->table[i][j].m > LED_ACTION_SYS_MAX)
                {
                    set_rgb_from_hex(color_ptr[i], &para->table[i][j]);
                }
            }
        }
    }
}

static void rgb_lamp_callback(void)
{
    rgb_led_set_all();
}

static void rgb_sync_callback(uint8_t mode, uint8_t state)
{
    if (mode == NONE)
    {
        g_sm_ctx.state = STATE_NORMAL;
        rgb_led_set_all();
        light_tape_sync();
    }
    else
    {
        if (g_sm_ctx.unit != state)
        {
            g_sm_ctx.unit = state;
        }
        g_sm_ctx.state = STATE_SYNC;
        rgb_led_set_all();
        light_tape_sync();
    }
}

void rgb_led_init(void)
{
    led_register(rgb_led_task_notify, rgb_led_wait_trans_complete);
    led_init();
    rgb_led_set_all();
    light_tape_init();
    rgb_lamp_array_autonomous_cb(rgb_lamp_callback);
    rgb_lamp_sync_autonomous_cb(rgb_sync_callback);

    if (pdPASS != xTaskCreate(rgb_led_task, "RGB_LED", STACK_SIZE_RGB, NULL, PRIORITY_RGB, &m_rgb_thread))
    {
       DBG_PRINTF("RGB task creation failure\r\n");
    }
}

void rgb_led_uninit(void)
{
    led_uninit();
    rgb_led_task_stop();
}

