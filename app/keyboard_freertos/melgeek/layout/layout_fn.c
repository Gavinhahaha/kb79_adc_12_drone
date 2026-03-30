/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
/* FreeRTOS kernel includes. */

#include "layout_fn.h"
#include "mg_hive.h"
#include "rgb_led_color.h"
#include "app_config.h"
#include "interface.h"
#include "db.h"
#include "mg_hid.h"
#include "sk.h"
#include "mg_matrix_ctrl.h"
#include "mg_matrix.h"
#include "mg_uart.h"
#include "app_debug.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "layout_fn"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif


typedef adv_match_t (*kfh)(void);

static bool in_fn_layout = false;
static TaskHandle_t m_fn_thread = NULL; 
static QueueHandle_t m_fn_queue = NULL;
static TickType_t in_fn_layout_ticks = 0;
static uint8_t fn_trg_record[BOARD_KEY_NUM] = {0};

typedef enum {
    LAYOUT_FN_EVT_NONE = 0,
    LAYOUT_FN_EVT_KEY_EVENT,
    LAYOUT_FN_EVT_CTRL_EVENT,
    LAYOUT_FN_EVT_MAX
} fn_evt_type_t;

typedef struct {
    fn_evt_type_t type;
    uint8_t       key_idx;
    uint8_t       key_sta;
} fn_event_t;

static rep_hid_t kb_report;
static rep_hid_t sys_report;
static rep_hid_t nkro_report;
rep_hid_t *matrix_kb_report;


static void layout_fn_sys_report_handler(void);
static adv_match_t layout_fn_kb_ctrl_handler(uint8_t key_idx);
static void layout_fn_kb_report_handler(void);
static void layout_fn_process_key_event(uint8_t key_idx, uint8_t key_sta);


static const light_tape_state_t light_tape_state1[3] = 
{
    {.r = 0, .g = 0,  .b = 0,  .delay = MS(200)},
    {.r = 0, .g = 80, .b = 0,  .delay = MS(500)},
    {.r = 0, .g = 0,  .b = 0,  .delay = MS(200)},
};

static const light_tape_state_t light_tape_state2[3] = 
{
    {.r = 0,  .g = 0, .b = 0,  .delay = MS(200)},
    {.r = 80, .g = 0, .b = 0,  .delay = MS(500)},
    {.r = 0,  .g = 0, .b = 0,  .delay = MS(200)},
};

rep_hid_t *matrix_get_sys_report(void)
{
    return &sys_report;
}

static adv_match_t kf_kr_toggle(void)
{
    uint8_t buf[2] = {0};

    if (gbinfo.lm.cur_lmm != LED_ACTION_NONE)
    {
        gbinfo.lm.last_lmm = gbinfo.lm.cur_lmm;
        gbinfo.lm.cur_lmm  = LED_ACTION_NONE;
    }
    else
    {
        gbinfo.lm.cur_lmm  = gbinfo.lm.last_lmm;
        gbinfo.lm.last_lmm = LED_ACTION_MAX;
    }
    DBG_PRINTF("kf_kr_toggle=%d,%d\n", gbinfo.lm.cur_lmm,gbinfo.lm.last_lmm);

    rgb_led_set_all();
    db_update(DB_SYS, UPDATE_LATER);

    buf[0] = 0;
    buf[1] = gbinfo.lm.cur_lmm;

    cmd_do_response(CMD_NOTIFY, NTF_KM, sizeof(buf), 0, buf);

    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_kr_set_mode(void)
{
    uint8_t lmi = gbinfo.lm.cur_lmm;
    uint8_t buf[2] = {0};

    if (gbinfo.lm.cur_lmm == LED_ACTION_NONE)
    {
        return ADV_MATCH_UNSEND;
    }

    lmi = (lmi + 1) % LED_ACTION_MAX;

    if (lmi == LED_ACTION_NONE)
    {
        lmi = lmi + 1;
    }

    gbinfo.lm.cur_lmm = lmi;

    rgb_led_set_all();
    db_update(DB_SYS, UPDATE_LATER);

    buf[0] = 0;
    buf[1] = gbinfo.lm.cur_lmm;
    DBG_PRINTF("kf_kr_set_mode=%d,%d\n", gbinfo.lm.cur_lmm,gbinfo.lm.last_lmm);

    cmd_do_response(CMD_NOTIFY, NTF_KM, sizeof(buf), 0, buf);

    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_kr_select_matrix(void)
{
    uint8_t buf[2] = {0};
    uint8_t ckrm = 0;
    
    if (gbinfo.lm.cur_lmm == LED_ACTION_NONE)
    {
        return ADV_MATCH_UNSEND;
    }

    gbinfo.kr.cur_krm = (gbinfo.kr.cur_krm + 1) % BOARD_LED_COLOR_LAYER_MAX;

    ckrm = gbinfo.kr.cur_krm;

    for (int i = 0; i < BOARD_KEY_LED_NUM; ++i)
    {
        set_key_led_color(i, gmx.pkts[i].ltl, gmx.pkr[ckrm][i].r, gmx.pkr[ckrm][i].g, gmx.pkr[ckrm][i].b);
    }

    db_update(DB_SYS, UPDATE_LATER);

    buf[0] = 0;
    buf[1] = ckrm;

    cmd_do_response(CMD_NOTIFY, NTF_KR, sizeof(buf), 0, buf);

    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_kr_inc_bri(void)
{
    uint8_t tmp = sk_la_lm_kc_info.la_table[gbinfo.lm.cur_lmm].hl;
    tmp += 4;

    if (tmp > LED_BRIGHTNESS_MAX)
    {
        tmp = LED_BRIGHTNESS_MAX;
    }

    sk_la_lm_kc_info.la_table[gbinfo.lm.cur_lmm].hl = tmp;

    DBG_PRINTF("kf_kr_inc_bri=%d\n",sk_la_lm_kc_info.la_table[gbinfo.lm.cur_lmm].hl);

    db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
    rgb_led_set_all();

    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_kr_dec_bri(void)
{
    int8_t tmp = sk_la_lm_kc_info.la_table[gbinfo.lm.cur_lmm].hl;
    tmp -= 4;

    if (tmp < 0)
    {
       tmp = 0;
    }
    
    sk_la_lm_kc_info.la_table[gbinfo.lm.cur_lmm].hl = tmp;
    
    DBG_PRINTF("kf_kr_dec_bri=%d\n",sk_la_lm_kc_info.la_table[gbinfo.lm.cur_lmm].hl);

    db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
    rgb_led_set_all();

    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_lb_back_toggle(void) 
{
    uint8_t buf[6] = {0};

    if (gbinfo.rl[LIGHT_TAPE_PART_BACK].state)
    {
        gbinfo.rl[LIGHT_TAPE_PART_BACK].state = 0;
    }
    else
    {
        gbinfo.rl[LIGHT_TAPE_PART_BACK].state = 1;
    }

    buf[0] = gbinfo.rl[LIGHT_TAPE_PART_BACK].md;
    buf[1] = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h;
    buf[2] = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s;
    buf[3] = VALUE_TO_PERCENT(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v, RGBLIGHT_LIMIT_VAL);
    buf[4] = 9 - gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].speed;
    buf[5] = gbinfo.rl[LIGHT_TAPE_PART_BACK].state;

    db_update(DB_SYS, UPDATE_LATER);
    cmd_do_response(CMD_NOTIFY, NTF_LB, sizeof(buf), 0, buf);

    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_lb_back_set_mode(void)
{
    uint8_t buf[6] = {0};

    if (gbinfo.rl[LIGHT_TAPE_PART_BACK].state)
    {
        light_tape_back_effect_switch_next();
        
        buf[0] = gbinfo.rl[LIGHT_TAPE_PART_BACK].md;
        buf[1] = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h;
        buf[2] = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s;
        buf[3] = VALUE_TO_PERCENT(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v, RGBLIGHT_LIMIT_VAL);
        buf[4] = 9 - gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].speed;
        buf[5] = gbinfo.rl[LIGHT_TAPE_PART_BACK].state;
        db_update(DB_SYS, UPDATE_LATER);
        cmd_do_response(CMD_NOTIFY, NTF_LB, sizeof(buf), 0, buf);
    }

    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_lb_back_inc_bri(void) 
{
    uint8_t buf[6] = {0};

    if(gbinfo.rl[LIGHT_TAPE_PART_BACK].state)
    {
        light_tape_back_increase_val();

        buf[0] = gbinfo.rl[LIGHT_TAPE_PART_BACK].md;
        buf[1] = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h;
        buf[2] = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s;
        buf[3] = VALUE_TO_PERCENT(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v, RGBLIGHT_LIMIT_VAL);
        buf[4] = 9 - gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].speed;
        buf[5] = gbinfo.rl[LIGHT_TAPE_PART_BACK].state;
        db_update(DB_SYS, UPDATE_LATER);
        cmd_do_response(CMD_NOTIFY, NTF_LB, sizeof(buf), 0, buf);
    }
	
    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_lb_back_dec_bri(void) 
{
    uint8_t buf[6] = {0};

    if(gbinfo.rl[LIGHT_TAPE_PART_BACK].state)
    {
        light_tape_back_decrease_val();
        db_update(DB_SYS, UPDATE_LATER);

        buf[0] = gbinfo.rl[LIGHT_TAPE_PART_BACK].md;
        buf[1] = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h;
        buf[2] = gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s;
        buf[3] = VALUE_TO_PERCENT(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v, RGBLIGHT_LIMIT_VAL);
        buf[4] = 9 - gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].speed;
        buf[5] = gbinfo.rl[LIGHT_TAPE_PART_BACK].state;

        cmd_do_response(CMD_NOTIFY, NTF_LB, sizeof(buf), 0, buf);
    }

    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_lb_side_toggle(void) 
{
    uint8_t buf[6] = {0};

    if(gbinfo.rl[LIGHT_TAPE_PART_SIDE].state)
    {
        gbinfo.rl[LIGHT_TAPE_PART_SIDE].state = 0;
    }
    else
    {
        gbinfo.rl[LIGHT_TAPE_PART_SIDE].state = 1;
    }

    buf[0] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].md;
    buf[1] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h;
    buf[2] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s;
    buf[3] = VALUE_TO_PERCENT(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v, RGBLIGHT_LIMIT_VAL);
    buf[4] = 9 - gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].speed;
    buf[5] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].state;

    db_update(DB_SYS, UPDATE_LATER);
    cmd_do_response(CMD_NOTIFY, NTF_LB, sizeof(buf), 0, buf);

    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_lb_side_set_mode(void)
{
    uint8_t buf[6] = {0};

    if(gbinfo.rl[LIGHT_TAPE_PART_SIDE].state)
    {
        light_tape_side_effect_switch_next();
        
        buf[0] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].md;
        buf[1] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h;
        buf[2] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s;
        buf[3] = VALUE_TO_PERCENT(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v, RGBLIGHT_LIMIT_VAL);
        buf[4] = 9 - gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].speed;
        buf[5] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].state;
        db_update(DB_SYS, UPDATE_LATER);
        cmd_do_response(CMD_NOTIFY, NTF_LB, sizeof(buf), 0, buf);
    }

    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_lb_side_inc_bri(void) 
{
    uint8_t buf[6] = {0};

    if(gbinfo.rl[LIGHT_TAPE_PART_SIDE].state)
    {
        light_tape_side_increase_val();

        buf[0] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].md;
        buf[1] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h;
        buf[2] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s;
        buf[3] = VALUE_TO_PERCENT(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v, RGBLIGHT_LIMIT_VAL);
        buf[4] = 9 - gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].speed;
        buf[5] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].state;
        db_update(DB_SYS, UPDATE_LATER);
        cmd_do_response(CMD_NOTIFY, NTF_LB, sizeof(buf), 0, buf);
    }
	
    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_lb_side_dec_bri(void) 
{
    uint8_t buf[6] = {0};

    if(gbinfo.rl[LIGHT_TAPE_PART_SIDE].state)
    {
        light_tape_side_decrease_val();
        db_update(DB_SYS, UPDATE_LATER);

        buf[0] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].md;
        buf[1] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h;
        buf[2] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s;
        buf[3] = VALUE_TO_PERCENT(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v, RGBLIGHT_LIMIT_VAL);
        buf[4] = 9 - gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].speed;
        buf[5] = gbinfo.rl[LIGHT_TAPE_PART_SIDE].state;

        cmd_do_response(CMD_NOTIFY, NTF_LB, sizeof(buf), 0, buf);
    }

    return ADV_MATCH_UNSEND;
}


static adv_match_t kf_layout_toggle(void)
{
    uint8_t space_key_idx = 0;

    for (; space_key_idx < BOARD_KEY_NUM; space_key_idx++)
    {
        if (KC_SPACE == gmx.pkc[gbinfo.layout][space_key_idx].co)
        {
            break;
        }
    }

    if ( KC_LALT == gmx.pkc[gbinfo.layout][space_key_idx - 1].co &&
         KC_LGUI == gmx.pkc[gbinfo.layout][space_key_idx - 2].co )
    {
        gmx.pkc[gbinfo.layout][space_key_idx - 1].co = KC_LGUI;
        gmx.pkc[gbinfo.layout][space_key_idx - 2].co = KC_LALT;
    }
    else if ( KC_LGUI == gmx.pkc[gbinfo.layout][space_key_idx - 1].co &&
              KC_LALT == gmx.pkc[gbinfo.layout][space_key_idx - 2].co )
    {
        gmx.pkc[gbinfo.layout][space_key_idx - 1].co = KC_LALT;
        gmx.pkc[gbinfo.layout][space_key_idx - 2].co = KC_LGUI;
    }

    db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
    
    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_kb_null(void)
{
    DBG_PRINTF("kf_kb_null\n");
    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_nkro_toggle(void)
{
    if (gbinfo.nkro == BOARD_NKRO_ENABLE)
    {
        gbinfo.nkro = BOARD_NKRO_DISABLE;
        light_tape_play(light_tape_state2, sizeof(light_tape_state2) / sizeof(light_tape_state_t));  
    }
    else
    {
        gbinfo.nkro = BOARD_NKRO_ENABLE;
        light_tape_play(light_tape_state1, sizeof(light_tape_state1) / sizeof(light_tape_state_t));
    }
    matrix_set_nkro(gbinfo.nkro);
    if (matrix_kb_report)
    {
        kb_report = *matrix_kb_report;
    }
    db_update(DB_SYS, UPDATE_LATER);
    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_toggle_rt(void)
{
    bool ret;
    ret = key_cmd_set_rt_en();
    
    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_toggle_rt_continue(void)
{
    bool ret;
    ret = key_cmd_set_rt_continue_en();

    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_kb_reset(void)
{
    DBG_PRINTF("on_sys_reset\n");
    db_reset();
    db_task_suspend();
    layout_fn_set_inactive();
    rgb_led_set_prompt(PROMPT_KB_RESET);
    uart_notify_screen_cmd(CMD_UART_SYS, SCMD_RESET, NULL, 0);
    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_kb_set_cfg0(void)
{
    DBG_PRINTF("kf_kb_set_cfg0\n");
    if (common_info.cfg_info.cfg_idx != 0)
    {
        common_info.cfg_info.cfg_idx = 0;
        db_send_toggle_cfg_signal();
    }
    return ADV_MATCH_UNSEND;
}
static adv_match_t kf_kb_set_cfg1(void)
{
    DBG_PRINTF("kf_kb_set_cfg1\n");
    if (common_info.cfg_info.cfg_idx != 1)
    {
        common_info.cfg_info.cfg_idx = 1;
        db_send_toggle_cfg_signal();
    }
    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_kb_set_cfg2(void)
{
    DBG_PRINTF("kf_kb_set_cfg2\n");
    if (common_info.cfg_info.cfg_idx != 2)
    {
        common_info.cfg_info.cfg_idx = 2;
        db_send_toggle_cfg_signal();
    }
    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_kb_set_cfg3(void)
{
    DBG_PRINTF("kf_kb_set_cfg3\n");
    if (common_info.cfg_info.cfg_idx != 3)
    {
        common_info.cfg_info.cfg_idx = 3;
        db_send_toggle_cfg_signal();
    }
    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_open_cn_website(void)
{
    macro_open_website(CN_WEBSITE);
    return ADV_MATCH_UNSEND;
}

static adv_match_t kf_open_en_website(void)
{
    macro_open_website(EN_WEBSITE);
    return ADV_MATCH_UNSEND;
}

static const kfh kf_kr_handler[FN_RGB_MAX] = {
    [FN_RGB_TOG] = kf_kr_toggle,
    [FN_RGB_MOD] = kf_kr_set_mode,
    // [FN_RGB_COL] = kf_kr_select_matrix,
    [FN_RGB_VAI] = kf_kr_inc_bri,
    [FN_RGB_VAD] = kf_kr_dec_bri,
};

static const kfh kf_mix_handler[FN_MIX_MAX] = {
    [FN_MIX_LAYOUT]      = kf_layout_toggle,
    [FN_MIX_NKRO]        = kf_nkro_toggle,
    [FN_MIX_RT]          = kf_toggle_rt,
    [FN_MIX_RT_CONTINUE] = kf_toggle_rt_continue,
    [FN_MIX_KB_RESET]    = kf_kb_reset,
    [FN_MIX_SET_CFG0]    = kf_kb_set_cfg0,
    [FN_MIX_SET_CFG1]    = kf_kb_set_cfg1,
    [FN_MIX_SET_CFG2]    = kf_kb_set_cfg2,
    [FN_MIX_SET_CFG3]    = kf_kb_set_cfg3,
    [FN_MIX_OPEN_CN_WEBSITE] = kf_open_cn_website,
    [FN_MIX_OPEN_EN_WEBSITE] = kf_open_en_website,
};

static const kfh kf_lb_handler[FN_LB_MAX] = {
    [FN_LB_BACK_TOG] = kf_lb_back_toggle,
    [FN_LB_BACK_MOD] = kf_lb_back_set_mode,
    [FN_LB_BACK_VAI] = kf_lb_back_inc_bri,
    [FN_LB_BACK_VAD] = kf_lb_back_dec_bri,

    [FN_LB_SIDE_TOG] = kf_lb_side_toggle,
    [FN_LB_SIDE_MOD] = kf_lb_side_set_mode,
    [FN_LB_SIDE_VAI] = kf_lb_side_inc_bri,
    [FN_LB_SIDE_VAD] = kf_lb_side_dec_bri,
};

uint8_t layout_fn_get_trg_sta(uint8_t ki)
{
    if (ki < BOARD_KEY_NUM)
    {
        return fn_trg_record[ki];
    }

    return 0;
}

void layout_fn_clr_trg_sta(uint8_t ki)
{
    if (ki < BOARD_KEY_NUM)
    {
        fn_trg_record[ki] = 0;
    }
}

static bool is_fn_trigger_key(uint8_t ki)
{
    kc_t *pkc = &gmx.pkc[gbinfo.layout][ki];
    return ((pkc->ty == KCT_FN) && (pkc->sty == FN_LAYER) && 
            (pkc->co >= KC_FN0) && (pkc->co <= KC_FN3));
}

bool layout_fn_check_enter(uint8_t ki)
{
    if (in_fn_layout)
    {
        return true;
    }

    if (is_fn_trigger_key(ki))
    { 
        key_st_t *pkst = matrix_get_key_st(ki);

        if ((pkst->cur_lvl > (KEY_MAX_LEVEL(ki) >> 1)))
        {
            kc_t *pkc = &gmx.pkc[gbinfo.layout][ki];
            gbinfo.kc.cur_kcm = pkc->co - KC_FN0;
            matrix_kb_report = NULL;

            matrix_kb_report = matrix_get_kb_report();
            kb_report = *matrix_kb_report;
            memset(&sys_report, 0, sizeof(rep_hid_t));
            in_fn_layout = true;
            in_fn_layout_ticks = xTaskGetTickCount();
            return true;
        }
        else
        {
            pkst->actionState     = true;
            pkst->actionStateLast = false;
        }
    }

    return false;
}

bool layout_fn_check_exit(uint8_t ki)
{
    if (!in_fn_layout)
    {
        return true;
    }

    if (is_fn_trigger_key(ki))
    {
        gbinfo.kc.cur_kcm = gbinfo.layout;
        in_fn_layout = false;
        in_fn_layout_ticks = 0;
        
        memset(&sys_report, 0, sizeof(rep_hid_t));
        sys_report.sys_mod_key.report_id = REPORT_ID_SYSTEM;
        uint32_t msg_sys = (uint32_t)&sys_report;
        xQueueSend(hid_queue, (const void * const )&msg_sys, ( TickType_t ) 0 );

        uint32_t msg_kb = (uint32_t)&kb_report;
        if (matrix_kb_report)
        {
            if (matrix_kb_report->report_id == kb_report.report_id)
            {
                *matrix_kb_report = kb_report;
            }
        }
        xQueueSend(hid_queue, (const void * const )&msg_kb, ( TickType_t ) 0 );

        return true;
    }

    return false;
}

void layout_fn_set_inactive(void)
{
    if (in_fn_layout)
    {
        gbinfo.kc.cur_kcm = gbinfo.layout;
        in_fn_layout = false;
        in_fn_layout_ticks = 0;
        matrix_kb_report = NULL;

        memset(&sys_report, 0, sizeof(rep_hid_t));
        memset(&kb_report, 0, sizeof(rep_hid_t));
        memset(&nkro_report, 0, sizeof(rep_hid_t));

        sys_report.sys_mod_key.report_id = REPORT_ID_SYSTEM;
        kb_report.report_id              = REPORT_ID_KEYBOARD;
        nkro_report.report_id            = REPORT_ID_NKRO;

        uint32_t msg_sys = (uint32_t)&sys_report;
        xQueueSend(hid_queue, (const void * const )&msg_sys, ( TickType_t ) 0 );

        uint32_t msg_kb = (uint32_t)&kb_report;
        xQueueSend(hid_queue, (const void * const )&msg_kb, ( TickType_t ) 0 );

        uint32_t msg_nkro = (uint32_t)&nkro_report;
        xQueueSend(hid_queue, (const void * const )&msg_nkro, ( TickType_t ) 0 );
    }
}

static adv_match_t layout_fn_kb_ctrl_handler(uint8_t key_idx)
{
    kc_t  *pkc  = &gmx.pkc[gbinfo.kc.cur_kcm][key_idx];
    key_tmp_sta_t *pkts = &gmx.pkts[key_idx];
    adv_match_t ret = ADV_MATCH_NONE;

    switch (pkc->sty)
    {
        case FN_RGB:
            if ((pkc->co < FN_RGB_MAX) && kf_kr_handler[pkc->co])
            {
                if (pkts->kcs != KCS_LP)
                {
                    ret = kf_kr_handler[pkc->co]();
                }
            }
            break;

        case FN_LB:
            if ((pkc->co < FN_LB_MAX) && kf_lb_handler[pkc->co])
            {
                if (pkts->kcs != KCS_LP)
                {
                    ret = kf_lb_handler[pkc->co]();
                }
            }
            break;
        
        case FN_MIX:
            if ((pkc->co < FN_MIX_MAX) && kf_mix_handler[pkc->co])
            {
                if (pkc->co == FN_MIX_KB_RESET)
                {
                    key_st_t *pkst = matrix_get_key_st(key_idx);
                    
                    if ((pkts->kcs == KCS_LP) && 
                        ((xTaskGetTickCount() - in_fn_layout_ticks) * portTICK_PERIOD_MS > pdMS_TO_TICKS(5000)) &&
                        pkst->cur_lvl > (KEY_MAX_LEVEL(key_idx) >> 1))
                    {
                        ret = kf_mix_handler[pkc->co]();
                    }
                }
                else
                {
                    if (pkts->kcs != KCS_LP)
                    {
                        ret = kf_mix_handler[pkc->co]();
                    }
                }
            }
            break;
            
        default:
            break;
    }

    return ret;
}

static void layout_fn_sys_report_handler(void)
{
    uint32_t msg = (uint32_t)&sys_report;
    xQueueSend(hid_queue, (const void * const )&msg, ( TickType_t ) 0 );
}

static void layout_fn_kb_report_handler(void)
{
    if (matrix_kb_report)
    {
        uint32_t msg = (uint32_t)matrix_kb_report;
        xQueueSend(hid_queue, (const void * const )&msg, ( TickType_t ) 0 );
    }
}

void layout_fn_active_key_code_rpt(kc_t kc, uint8_t state)
{  
    switch (kc.ty)   
    {
        case KCT_SYS:
        {
            if (sys_report.sys_mod_key.report_id != REPORT_ID_SYSTEM)
            {
                sys_report.sys_mod_key.report_id = REPORT_ID_SYSTEM;
            }

            if (state == KEY_STATUS_U2U)
            {
                report_del_sys_cmd(&sys_report, kc.co);
            }
            else if (state == KEY_STATUS_U2D)
            {
                report_add_sys_cmd(&sys_report, kc.co);
            }
            layout_fn_sys_report_handler();
        }break;

        case KCT_KB:
        {
            matrix_kb_report = matrix_get_kb_report();

            if (state == KEY_STATUS_U2U)
            {
                report_del_key(matrix_kb_report, kc.co);
            }
            else if (state == KEY_STATUS_U2D)
            {
                report_add_key(matrix_kb_report, kc.co);
            }

            uint32_t msg = (uint32_t)matrix_kb_report;
            xQueueSend(hid_queue, (const void * const )&msg, ( TickType_t ) 0 );
        }break;
        
        default: break;
    }
}

static void layout_fn_process_key_event(uint8_t key_idx, uint8_t key_sta)
{
    kc_t *pkc = &gmx.pkc[gbinfo.kc.cur_kcm][key_idx];

    switch (pkc->ty)
    {
        case KCT_FN:
        {
            if (pkc->sty != FN_LAYER)
            {
                if (key_sta == KEY_STATUS_U2D)
                {
                    layout_fn_kb_ctrl_handler(key_idx);
                }
                else if (key_sta == KEY_STATUS_U2U)
                {
                    if (report_has_key(matrix_kb_report, gmx.pkc[gbinfo.layout][key_idx].co))
                    {
                        report_del_key(matrix_kb_report, gmx.pkc[gbinfo.layout][key_idx].co);
                        report_del_key(&kb_report, gmx.pkc[gbinfo.layout][key_idx].co);
                        layout_fn_kb_report_handler();
                    }
                }
            }
        } break;

        case KCT_SYS:
        {
            sys_report.sys_mod_key.report_id = REPORT_ID_SYSTEM;

            if (key_sta == KEY_STATUS_U2U)
            {
                if ((gmx.pkc[gbinfo.layout][key_idx].ty == KCT_KB) &&
                    report_has_key(matrix_kb_report, gmx.pkc[gbinfo.layout][key_idx].co))
                {
                    report_del_key(matrix_kb_report, gmx.pkc[gbinfo.layout][key_idx].co);
                    report_del_key(&kb_report, gmx.pkc[gbinfo.layout][key_idx].co);
                    layout_fn_kb_report_handler();
                }
                report_del_sys_cmd(&sys_report, pkc->co);
            }
            else if (key_sta == KEY_STATUS_U2D)
            {
                report_add_sys_cmd(&sys_report, pkc->co);
            }

            layout_fn_sys_report_handler();
        } break;

        case KCT_KB:
        {
            if (key_sta == KEY_STATUS_U2U)
            {
                report_del_key(matrix_kb_report, pkc->co);
                report_del_key(&kb_report, pkc->co);
                if (gmx.pkc[gbinfo.layout][key_idx].ty == KCT_RPL)
                {
                    replace_key_t *rpl_key = key_code_get_replace_key_info(gmx.pkc[gbinfo.layout][key_idx].co);
                    report_del_key(matrix_kb_report, rpl_key->dft_co);
                    report_del_key(&kb_report, rpl_key->dft_co);
                }
                else if (gmx.pkc[gbinfo.layout][key_idx].ty == KCT_KB)
                {
                    report_del_key(matrix_kb_report, gmx.pkc[gbinfo.layout][key_idx].co);
                    report_del_key(&kb_report, gmx.pkc[gbinfo.layout][key_idx].co);
                }
                fn_trg_record[key_idx] = 0;
            }
            else if (key_sta == KEY_STATUS_U2D)
            {
                if (pkc->co == KC_TRNS)
                {
                    if (gmx.pkc[gbinfo.layout][key_idx].ty == KCT_KB)
                    {
                        report_add_key(matrix_kb_report, gmx.pkc[gbinfo.layout][key_idx].co);
                        report_add_key(&kb_report, gmx.pkc[gbinfo.layout][key_idx].co);
                    }
                }
                else
                {
                    report_add_key(matrix_kb_report, pkc->co);
                    report_add_key(&kb_report, pkc->co);
                    fn_trg_record[key_idx] = 1;
                }
            }
            layout_fn_kb_report_handler();
        } break;

        case KCT_SK:
        {
            if (key_sta == KEY_STATUS_U2U)
            {
                if ((gmx.pkc[gbinfo.layout][key_idx].ty == KCT_KB) &&
                    report_has_key(matrix_kb_report, gmx.pkc[gbinfo.layout][key_idx].co))
                {
                    report_del_key(matrix_kb_report, gmx.pkc[gbinfo.layout][key_idx].co);
                    report_del_key(&kb_report, gmx.pkc[gbinfo.layout][key_idx].co);
                    layout_fn_kb_report_handler();
                }
            }
            sk_judge_evt(key_idx, key_sta);
            sk_task_notify();
        } break;

        default:
            break;
    }
}

bool layout_fn_is_in(void)
{
    return in_fn_layout;
}

void layout_fn_handle_key_event(uint8_t ki, uint8_t sta)
{
    fn_event_t evt = {
        .type     = LAYOUT_FN_EVT_KEY_EVENT,
        .key_idx  = ki,
        .key_sta  = sta
    };

    kc_t *pkc = &gmx.pkc[gbinfo.kc.cur_kcm][ki];
    if ((pkc->ty == KCT_FN) && (sta == KEY_STATUS_U2D))
    {
        evt.type = LAYOUT_FN_EVT_CTRL_EVENT;
        xQueueSendToFront(m_fn_queue, &evt, 0);
    }
    else
    {
        xQueueSend(m_fn_queue, &evt, 0);
    }

    xTaskNotifyGive(m_fn_thread);
}

static void layout_fn_task(void * p_context)
{
    (void)(p_context);
    fn_event_t evt;

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (xQueueReceive(m_fn_queue, &evt, 0) == pdPASS)
        {
            if (evt.type == LAYOUT_FN_EVT_CTRL_EVENT)
            {
                layout_fn_kb_ctrl_handler(evt.key_idx);
            }
            else if (evt.type == LAYOUT_FN_EVT_KEY_EVENT)
            {
                layout_fn_process_key_event(evt.key_idx, evt.key_sta);
            }
        }
    }
}

void layout_fn_init(void)
{
    in_fn_layout = false;

    m_fn_queue = xQueueCreate(16, sizeof(fn_event_t));
    if (m_fn_queue == NULL)
    {
        DBG_PRINTF("fn queue create failed\r\n");
        return;
    }

    if (pdPASS != xTaskCreate(layout_fn_task, "layout_fn", STACK_SIZE_FN, NULL, PRIORITY_FN, &m_fn_thread))
    {
        DBG_PRINTF("layout_fn task creation failure\r\n");
    }
}