/*
 * Copyright (c) 2024-2025 Melgeek, All Rights Reserved.
 *
 * Description:
 *
 */
#include "effect_common.h"

#define CENTER_ROW 2
#define CENTER_COL 7
#define DIFFUSE_SPEED 3

void init_gush(uint8_t ki, led_action_t *pla)
{
    uint8_t r = 0, c = 0;
    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);

    pkts->lhl = pla->hl;
    pkts->lll = pla->ll;
    pkts->lls = pla->ls;
    pkts->lhzc = pla->hzc;
    pkts->lhdy = pla->hdy;
    pkts->lpdy = pla->pdy;
    pkts->ltpt = pla->tpt;
    pkts->lttpt = pkts->ltpt;

    pkts->ltkrr = 255;
    pkts->ltkrg = 100;
    pkts->ltkrb = 0;
    pkts->tlat = LED_ACTION_SYS_MAX;
    pkts->ltts = (pkts->lhl - pkts->lll) << 1;
    pkts->ltl = pkts->lll;
    pkts->lltl = pkts->ltl;
    pkts->ltdy = 0;
    pkts->ext1 = 0;
    pkts->ltst = LED_STA_START;

    get_led_location(ki, &r, &c);

    uint8_t distance = abs(r - CENTER_ROW) + abs(c - CENTER_COL);
    pkts->ext1 = distance;

    pkts->ltdy = distance * DIFFUSE_SPEED *
                     ((pkts->lhzc * (pkts->lhl - pkts->lll) / pkts->lls)) >>
                 3;
}

void proc_gush(uint8_t ki, void *pd1, void *pd2, void *pd3)
{
    uint8_t ckrm = gbinfo.kr.cur_krm;
    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);
    key_led_color_t *pkr = get_key_led_color_ptr(ckrm, ki);

    if (pkts->ltdy > 0)
    {
        pkts->ltdy--;
        return;
    }

    if (pkts->ltst == LED_STA_NONE)
    {
        return;
    }

    if (pkts->tlat == LED_ACTION_GUSH) 
    {
        uint16_t brightness = 255 - (pkts->ext1 * 25);
        if (brightness > 255)
            brightness = 0;

        uint8_t target_r = (pkts->ltkrr * brightness) / 255;
        uint8_t target_g = (pkts->ltkrg * brightness) / 255;
        uint8_t target_b = (pkts->ltkrb * brightness) / 255;

        do_color_gradual_change(&pkr->r, &pkr->g, &pkr->b, target_r, target_g, target_b);
    }
    else
    {
        uint8_t r = pkts->ltkrr;
        uint8_t g = pkts->ltkrg;
        uint8_t b = pkts->ltkrb;
        do_color_gradual_change(&r, &g, &b, 255, 0, 10);
        pkts->ltkrr = r;
        pkts->ltkrg = g;
        pkts->ltkrb = b;
    }

    if (LED_STA_STOP == pkts->ltst && pkts->tlat == LED_ACTION_GUSH)
    {
        pkts->tlat = 0;
        pkts->ltkrr = pkr->r;
        pkts->ltkrg = pkr->g;
        pkts->ltkrb = pkr->b;
    }
}

void keyp_gush(uint8_t ki, led_action_t *pla)
{
    uint8_t ckrm = gbinfo.kr.cur_krm;

    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);
    key_led_color_t *pkr = get_key_led_color_ptr(ckrm, ki);

    if (pkts->kps == KEY_STATUS_U2D && pla->pr)
    {
        if (pkts->tlat != LED_ACTION_GUSH)
        {
            pkr->r = pkts->ltkrr;
            pkr->g = pkts->ltkrg;
            pkr->b = pkts->ltkrb;
        }

        get_random_rgb(&pkts->ltkrr, &pkts->ltkrg, &pkts->ltkrb, ki * 17);
        pkts->tlat  = LED_ACTION_GUSH;
    }
}