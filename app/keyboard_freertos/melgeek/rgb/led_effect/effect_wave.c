/*
 * Copyright (c) 2024-2025 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#include "effect_common.h"


void init_wave(uint8_t ki, led_action_t *pla)
{
    uint8_t r   = 0, c = 0;
    int16_t li  = 0;

    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);

    pkts->lhl   = pla->hl;
    pkts->lll   = pla->ll;
    pkts->lls   = pla->ls;
    pkts->lhzc  = pla->hzc;
    pkts->lhdy  = pla->hdy;
    pkts->lpdy  = pla->pdy;
    pkts->ltpt  = pla->tpt;
    pkts->lttpt = pkts->ltpt;

    pkts->ltkrr = 255;
    pkts->ltkrg = 0;
    pkts->ltkrb = 255;
    pkts->tlat  = LED_ACTION_SYS_MAX;
    pkts->ltts  = (pkts->lhl - pkts->lll) << 1;
    pkts->ltl   = pkts->lll;
    pkts->lltl  = pkts->ltl;
    pkts->ltdy  = 0;
    pkts->ext1  = 0;
    pkts->ltst  = LED_STA_START;

    get_led_location(ki, &r, &c);
    li = lookup_key_led_value(r, c);

    if (li != -1)
    {
        key_tmp_sta_t *pkts_l = get_key_tmp_sta_ptr(li);
        pkts_l->ltdy = c * ((pkts_l->lhzc * (pkts_l->lhl - pkts_l->lll) / pkts_l->lls)) >> 3;
    }
}

void proc_wave(uint8_t ki, void *pd1, void *pd2, void *pd3)
{
    uint8_t ckrm = gbinfo.kr.cur_krm;

    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);
    key_led_color_t *pkr = get_key_led_color_ptr(ckrm, ki);

    if (pkts->ltdy != 0 || pkts->ltst == LED_STA_NONE)
    {
        return;
    }

    if (pkts->tlat == LED_ACTION_WAVE)
    {
        do_color_gradual_change(&pkr->r, &pkr->g, &pkr->b, 255, 0, 10);
    }
    else
    {
        uint8_t r = 0, g = 0, b = 0;
        r = pkts->ltkrr; 
        g = pkts->ltkrg; 
        b = pkts->ltkrb;
        do_color_gradual_change(&r, &g, &b, 255, 0, 10);
        pkts->ltkrr = r; 
        pkts->ltkrg = g;
        pkts->ltkrb = b;
    }

    if (LED_STA_STOP == pkts->ltst && pkts->tlat == LED_ACTION_WAVE)
    {
        pkts->tlat  = 0;
        pkts->ltkrr = pkr->r;
        pkts->ltkrg = pkr->g;
        pkts->ltkrb = pkr->b;
    }
}

void keyp_wave(uint8_t ki, led_action_t *pla)
{
    uint8_t ckrm = gbinfo.kr.cur_krm;

    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);
    key_led_color_t *pkr = get_key_led_color_ptr(ckrm, ki);

    if (pkts->kps == KEY_STATUS_U2D && pla->pr)
    {
        if (pkts->tlat != LED_ACTION_WAVE)
        {
            pkr->r = pkts->ltkrr;
            pkr->g = pkts->ltkrg;
            pkr->b = pkts->ltkrb;
        }

        get_random_rgb(&pkts->ltkrr, &pkts->ltkrg, &pkts->ltkrb, ki * 17);
        pkts->tlat  = LED_ACTION_WAVE;
    }
}