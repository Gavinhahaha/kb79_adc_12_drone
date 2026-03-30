/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include "sk_macro.h"
#include "db.h"
#include "mg_kb.h"
#include "easy_crc.h"
#include "mg_hid.h"
#include "mg_matrix.h"
#include "app_config.h"
#include "app_debug.h"
#include "rgb_led_ctrl.h" 

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "sk_macro"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#define MACRO_CMD_QUEUE_LEN     5

macro_info_t macro_info;

static QueueHandle_t xMacroCmdQueue = NULL;
static TaskHandle_t  xMacroTaskHandle = NULL;
static bool     g_macro_is_playing = false;
static bool     g_macro_loop_enable = false;     
static uint8_t  g_macro_toggle_state[SK_MACRO_NUM] = {0};
static TimerHandle_t g_macro_d2d_timer = NULL;
static int8_t g_macro_d2d_id = -1;
static bool   g_macro_is_website_playing = false;

static macro_play_done_fnc macro_play_done[SK_MACRO_NUM + 1];

static const macro_t macro_cn_website = MACRO_CN_WEBSITE;
static const macro_t macro_en_website = MACRO_EN_WEBSITE;

typedef enum {
    MACRO_CMD_PLAY_KEY,
    MACRO_CMD_PLAY_WEBSITE,
    MACRO_CMD_STOP,
} macro_cmd_type_t;

typedef struct {
    macro_cmd_type_t type;
    union 
    {
        struct 
        { 
            int id; 
            m_trg_t trigger; 
        } play;
        uint8_t website_type;
    } data;
} macro_cmd_t;

static uint32_t g_random_seed = 0;

static uint16_t get_random_time(uint16_t min, uint16_t max)
{
    if (max <= min) return min;
    
    // 使用 xorshift32 算法生成更好的随机数
    g_random_seed ^= g_random_seed << 13;
    g_random_seed ^= g_random_seed >> 17;
    g_random_seed ^= g_random_seed << 5;
    
    return min + (g_random_seed % (max - min));
}

void macro_play_end_register_cb(uint8_t idx, macro_play_done_fnc fnc)
{
    if (idx <= SK_MACRO_NUM)
    {
        macro_play_done[idx] = fnc;
    }
}

static void macro_task(void *pvParameters)
{
    (void)pvParameters;

    macro_cmd_t cmd;
    const macro_t *current_macro = NULL;
    int      play_times = 0;
    int      frame_index = 0;
    uint16_t key_press_cnt = 0;
    uint8_t  caps_status = 0;

    rep_hid_t *macro_kb_report = sk_get_kb_report();

    for (;;)
    {
        xQueueReceive(xMacroCmdQueue, &cmd, portMAX_DELAY);

        if (cmd.type == MACRO_CMD_STOP)
        {
            goto macro_stop;
        }

        if (cmd.type == MACRO_CMD_PLAY_KEY)
        {
            current_macro = &macro_info.macro_table[cmd.data.play.id];
            frame_index = 0;
            g_macro_is_website_playing = false;
        }
        else if (cmd.type == MACRO_CMD_PLAY_WEBSITE)
        {
            current_macro = (cmd.data.website_type == CN_WEBSITE) ?
                            &macro_cn_website : &macro_en_website;
            caps_status = led_get_caps_led_status();
            frame_index = caps_status ? 2 : 0;
            g_macro_is_website_playing = true;
        }
        else
        {
            continue;
        }

        play_times = current_macro->mh.repeat_times;
        g_macro_is_playing = true;
        key_press_cnt = 0;

        while (g_macro_is_playing)
        {
            macro_cmd_t new_cmd;
            if (xQueueReceive(xMacroCmdQueue, &new_cmd, 0) == pdTRUE)
            {
                if (new_cmd.type == MACRO_CMD_STOP)
                {
                    goto macro_stop;
                }

                if (new_cmd.type == MACRO_CMD_PLAY_KEY ||
                    new_cmd.type == MACRO_CMD_PLAY_WEBSITE)
                {
                    cmd = new_cmd;
                    if (current_macro && key_press_cnt)
                    {
                        for (int i = 0; i < current_macro->mh.total_frames; i++)
                        {
                            report_del_key(macro_kb_report, current_macro->frames[i].kc.co);
                        }
                        uint32_t msg = (uint32_t)macro_kb_report;
                        xQueueSend(hid_queue, &msg, 0);
                    }

                    g_macro_is_playing = false;
                    key_press_cnt = 0;
                    
                    if (cmd.type == MACRO_CMD_PLAY_KEY)
                    {
                        current_macro = &macro_info.macro_table[cmd.data.play.id];
                        frame_index = 0;
                        g_macro_is_website_playing = false;
                    }
                    else if (cmd.type == MACRO_CMD_PLAY_WEBSITE)
                    {
                        current_macro = (cmd.data.website_type == CN_WEBSITE) ?
                                        &macro_cn_website : &macro_en_website;
                        caps_status = led_get_caps_led_status();
                        frame_index = caps_status ? 2 : 0;
                        g_macro_is_website_playing = true;
                    }
                    else
                    {
                        continue;
                    }

                    play_times = current_macro->mh.repeat_times;
                    g_macro_is_playing = true;
                    key_press_cnt = 0;
                }
            }

            if (frame_index < current_macro->mh.total_frames)
            {
                const mframe_t *frame = &current_macro->frames[frame_index];
                uint32_t macro_delay_ms = frame->ms;

                if (current_macro->mh.delay_type == DLY_RANDOM)
                {
                    macro_delay_ms = get_random_time(current_macro->mh.delay_ms_min, current_macro->mh.delay_ms_max);
                }

                if (frame->kc.ty == KCT_KB)
                {
                    if (frame->kc.ex == KEY_PRESSED)
                    {
                        report_add_key(macro_kb_report, frame->kc.co);
                        key_press_cnt++;
                    }
                    else
                    {
                        report_del_key(macro_kb_report, frame->kc.co);
                        if (key_press_cnt) key_press_cnt--;
                    }

                    uint32_t msg = (uint32_t)macro_kb_report;
                    xQueueSend(hid_queue, &msg, 0);
                }

                frame_index++;
                vTaskDelay(pdMS_TO_TICKS(macro_delay_ms | 1));
            }
            else
            {
                if ((current_macro->mh.trg_action == M_TRG_HOLD ||
                     current_macro->mh.trg_action == M_TRG_TOG) &&
                    g_macro_loop_enable)
                {
                    frame_index = 0;
                    vTaskDelay(pdMS_TO_TICKS(current_macro->mh.trg_time | 1));
                    continue;
                }

                if (play_times > 1)
                {
                    play_times--;
                    frame_index = 0;
                }
                else
                {
                    goto macro_stop;
                }
            }
        }

macro_stop:
        if (current_macro && key_press_cnt)
        {
            for (int i = 0; i < current_macro->mh.total_frames; i++)
            {
                report_del_key(macro_kb_report, current_macro->frames[i].kc.co);
            }
            uint32_t msg = (uint32_t)macro_kb_report;
            xQueueSend(hid_queue, &msg, 0);
        }

        g_macro_is_playing = false;
        g_macro_loop_enable = false;
        g_macro_is_website_playing = false;
        key_press_cnt = 0;
        continue;
    }
}

void macro_open_website(uint8_t website_type)
{
    if (NULL == xMacroCmdQueue) return;

    if (g_macro_is_website_playing) return;

    macro_cmd_t cmd = 
    {
        .type = MACRO_CMD_PLAY_WEBSITE,
        .data.website_type = website_type
    };
    xQueueSend(xMacroCmdQueue, &cmd, 0);
}

void macro_stop_all(void)
{
    if (xMacroCmdQueue == NULL) return;
    macro_cmd_t cmd = { .type = MACRO_CMD_STOP };
    xQueueSend(xMacroCmdQueue, &cmd, 0);
}

static void macro_d2d_timer_cb(TimerHandle_t xTimer)
{
    (void)xTimer;

    if (xMacroCmdQueue == NULL)
        return;

    if (g_macro_d2d_id < 0 || g_macro_d2d_id >= SK_MACRO_NUM)
        return;

    macro_cmd_t cmd =
    {
        .type = MACRO_CMD_PLAY_KEY,
        .data.play = {
            .id = g_macro_d2d_id,
            .trigger = M_TRG_D2U,
        },
    };

    xQueueSend(xMacroCmdQueue, &cmd, 0);

}

adv_match_t macro_start(int id, m_trg_t trg, uint16_t trg_time)
{
    if (NULL == xMacroCmdQueue) return ADV_MATCH_NONE;

    if (id < 0 || id >= SK_MACRO_NUM) return ADV_MATCH_NONE;

    if (!(sk_la_lm_kc_info.sk.superkey_table[SK_MACRO].storage_mask & (1U << id)))
        return ADV_MATCH_NONE;

    m_trg_t action = macro_info.macro_table[id].mh.trg_action;

    if (action == M_TRG_HOLD)
    {
        if (trg == M_TRG_D2U)
        {
            macro_cmd_t stop = { .type = MACRO_CMD_STOP };
            xQueueSend(xMacroCmdQueue, &stop, 0);
            return ADV_MATCH_NONE;
        }
        g_macro_loop_enable = true;
    }

    if (action == M_TRG_TOG)
    {
        if (trg != M_TRG_D2U)
        {
            return ADV_MATCH_NONE;
        }
            
        g_macro_toggle_state[id] ^= 1;
        if (g_macro_toggle_state[id] == 0)
        {
            macro_cmd_t stop = { .type = MACRO_CMD_STOP };
            xQueueSend(xMacroCmdQueue, &stop, 0);
            return ADV_MATCH_NONE;
        }
        g_macro_loop_enable = true;
    }

    if (action == M_TRG_D2D)
    {
        if (trg != M_TRG_D2U)
        {
            return ADV_MATCH_NONE;
        }

        if (g_macro_d2d_timer == NULL)
        {
            return ADV_MATCH_NONE;
        }

        if ((macro_info.macro_table[id].mh.trg_time > 1000 * 10) || 
            (macro_info.macro_table[id].mh.trg_time < 1))
        {
            return ADV_MATCH_NONE;
        }

        g_macro_d2d_id = id;
        xTimerChangePeriod(g_macro_d2d_timer, pdMS_TO_TICKS(macro_info.macro_table[id].mh.trg_time), 0);
        xTimerStart(g_macro_d2d_timer, 0);
        return ADV_MATCH_SEND;
    }

    if ((action != trg) &&
        (action != M_TRG_HOLD) &&
        (action != M_TRG_TOG))
    {
        return ADV_MATCH_NONE;
    }

    macro_cmd_t cmd =
    {
        .type = MACRO_CMD_PLAY_KEY,
        .data.play = { id, trg }
    };

    xQueueSend(xMacroCmdQueue, &cmd, 0);
    return ADV_MATCH_SEND;
}

void macro_init(void)
{
    // 使用当前 tick 和一些噪声初始化随机种子
    g_random_seed = xTaskGetTickCount() ^ 0x12345678;
    
    db_register(DB_MACRO, (uint32_t *)&macro_info, sizeof(macro_info));
    DBG_PRINTF("macro_init: size=%zu\n", sizeof(macro_info));

    xMacroCmdQueue = xQueueCreate(MACRO_CMD_QUEUE_LEN, sizeof(macro_cmd_t));
    if (xMacroCmdQueue == NULL) 
    {
        DBG_PRINTF("Failed to create macro command queue!\n");
        return;
    }

    if (xTaskCreate(macro_task, "MacroTask", STACK_SIZE_MACRO, NULL, PRIORITY_MACRO, &xMacroTaskHandle) != pdPASS) 
    {
        DBG_PRINTF("Failed to create macro task!\n");
        vQueueDelete(xMacroCmdQueue);
        xMacroCmdQueue = NULL;
    }

    g_macro_d2d_timer = xTimerCreate("macro_d2d", pdMS_TO_TICKS(10), pdFALSE, NULL, macro_d2d_timer_cb);
    if (g_macro_d2d_timer == NULL)
    {
        DBG_PRINTF("Failed to create macro d2d timer!\n");
    }          

}

void macro_uninit(void)
{
    if (xMacroTaskHandle) 
    {
        vTaskDelete(xMacroTaskHandle);
        xMacroTaskHandle = NULL;
    }

    if (xMacroCmdQueue) 
    {
        vQueueDelete(xMacroCmdQueue);
        xMacroCmdQueue = NULL;
    }
    g_macro_is_playing = false;
    g_macro_loop_enable = false;
    g_macro_is_website_playing = false;
}
