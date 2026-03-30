/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved.
 *
 * Description:
 *
 */
#include "effect_common.h"

void init_rain(uint8_t ki, led_action_t *pla)
{
    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);
    key_led_color_t *pkr = get_key_led_color_ptr(gbinfo.kr.cur_krm, ki);

    pkts->ltpt = pla->tpt;
    pkts->lhl = pla->hl;
    pkts->lll = pla->ll;
    pkts->lls = pla->ls;
    pkts->lhzc = pla->hzc;
    {
        pkts->ltkrr = pkr->r;
        pkts->ltkrg = pkr->g;
        pkts->ltkrb = pkr->b;
    }

    pkts->ltdy = 0;
    pkts->ext1 = 0;
    pkts->tlat = LED_ACTION_SYS_MAX;
    pkts->lttpt = pkts->ltpt;
    pkts->ltl = pla->bd ? pkts->lhl : pkts->lll;
    pkts->lltl = pkts->ltl;
    pkts->ltst = LED_STA_NONE;
}

static void do_set_rain(uint8_t oki, int ri, int ci, uint16_t dly, uint8_t r, uint8_t g, uint8_t b)
{
    if (ri < 0 || ri >= BOARD_LED_ROW_NUM)
        return;
    if (ci < 0 || ci >= BOARD_LED_COLUMN_NUM)
        return;

    uint8_t rit, cit;
    get_led_location(oki, &rit, &cit);
    if (ci != cit)
        return;

    int16_t li = lookup_key_led_value(ri, ci);
    if (li == -1 || li == oki)
        return;

    if (gmx.pla[gbinfo.lm.cur_lmm].at != LED_ACTION_RAIN)
    {
        return;
    }

    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(li);
    key_led_color_t *pkr = get_key_led_color_ptr(gbinfo.kr.cur_krm, li);
    if (pkr->m == LED_ACTION_RIPPLE || pkr->m == 0)
    {
        pkts->ltkrr = r;
        pkts->ltkrg = g;
        pkts->ltkrb = b;
    }

    if (pkts->ltst == LED_STA_NONE)
    {
        pkts->tlat = LED_ACTION_RAIN;
        pkts->ltdy = dly;
        pkts->lhdy = dly;
        pkts->ext1 = oki;
        pkts->ltst = LED_STA_START;
    }
    else
    {
        if (pkts->ltst != LED_STA_START)
        {
            pkts->lttpt = 2;
        }
    }
}

static void do_rain(uint8_t ki, uint32_t count, uint8_t cr, uint8_t cg, uint8_t cb)
{
    uint32_t dly = count;
    uint8_t r = 0, c = 0;

    get_led_location(ki, &r, &c);

    for (int i = 0;; ++i)
    {
        bool flag = false;
        dly++;

        int ri_top = (int)r - i;
        int ri_bot = (int)r + i;

        if (ri_top >= 0)
        {
            flag = true;
            do_set_rain(ki, ri_top, c, dly, cr, cg, cb);
        }

        if (ri_bot < (int)BOARD_LED_ROW_NUM)
        {
            flag = true;
            do_set_rain(ki, ri_bot, c, dly, cr, cg, cb);
        }

        if (!flag)
        {
            break;
        }

        if (ri_top < 0 && ri_bot >= (int)BOARD_LED_ROW_NUM)
        {
            break;
        }
    }
}

void proc_rain(uint8_t ki, void *pd1, void *pd2, void *pd3)
{
    // key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);
    // if (get_key_tmp_sta_ptr(ki)->ltst == LED_STA_STOP)
    // {
    //     init_random(ki, &gmx.pla[*(uint8_t *)pd1]);

    //     uint32_t dly = 0;


    //     pkts->ltst = LED_STA_START;

    //     dly = ((pkts->lhl - pkts->lll) * pkts->lhzc / pkts->lls) >> 1;
    //     do_rain(ki, dly, pkts->ltkrr, pkts->ltkrg, pkts->ltkrb);
    // }
}

void keyp_rain(uint8_t ki, led_action_t *pla)
{
    uint8_t ckrm = gbinfo.kr.cur_krm;

    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);
    key_led_color_t *pkr = get_key_led_color_ptr(ckrm, ki);

    uint32_t dly = 0;

    if (pkr->m != 0x00)
    {
        uint8_t rit, cit;
        get_led_location(ki, &rit, &cit);

        for (uint8_t i = 1; i < BOARD_LED_ROW_NUM; i++)
        {

            int16_t li = lookup_key_led_value(rit + i, cit);
            if (li == -1)
                continue;
            
            pkr = get_key_led_color_ptr(ckrm, li);
            if (pkr->m == 0x00)
            {
                pkts = get_key_tmp_sta_ptr(li);
                if (pla->rr)
                {
                    get_random_rgb(&pkts->ltkrr, &pkts->ltkrg, &pkts->ltkrb, li);
                }
                else
                {
                    pkts->ltkrr = pkr->r;
                    pkts->ltkrg = pkr->g;
                    pkts->ltkrb = pkr->b;
                }
                break;
            }
        }
    }
    else
    {
        if (pla->rr)
        {
            get_random_rgb(&pkts->ltkrr, &pkts->ltkrg, &pkts->ltkrb, ki);
        }
        else
        {
            pkts->ltkrr = pkr->r;
            pkts->ltkrg = pkr->g;
            pkts->ltkrb = pkr->b;
        }
    }

    pkts->ltst = LED_STA_START;
    pkts->ext1 = ki;

    dly = ((pkts->lhl - pkts->lll) * pkts->lhzc / pkts->lls) >> 1;
    do_rain(ki, dly, pkts->ltkrr, pkts->ltkrg, pkts->ltkrb);

}