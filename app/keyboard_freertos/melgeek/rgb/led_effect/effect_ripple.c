#include "effect_common.h"


void init_ripple(uint8_t ki, led_action_t *pla)
{
    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);
    key_led_color_t *pkr = get_key_led_color_ptr(gbinfo.kr.cur_krm, ki);

    pkts->ltpt  = pla->tpt;
    pkts->lhl   = pla->hl;
    pkts->lll   = pla->ll;
    pkts->lls   = pla->ls;
    pkts->lhzc  = pla->hzc;
    if (pla->rr)
    {
    }
    else
    {
        pkts->ltkrr = pkr->r;
        pkts->ltkrg = pkr->g;
        pkts->ltkrb = pkr->b;
    }

    pkts->ltdy  = 0;
    pkts->ext1  = 0;
    pkts->tlat  = LED_ACTION_SYS_MAX;
    pkts->lttpt = pkts->ltpt;
    pkts->ltl   = pla->bd ? pkts->lhl : pkts->lll;
    pkts->lltl  = pkts->ltl;
    pkts->ltst  = LED_STA_NONE;
}

static void do_set_ripple(uint8_t oki, int ri, int ci, uint16_t dly, uint8_t r, uint8_t g, uint8_t b)
{
    if ((ri >= 0 && ri < BOARD_LED_ROW_NUM) && (ci >= 0 && ci < BOARD_LED_COLUMN_NUM))
    {
        int16_t li = lookup_key_led_value(ri, ci);

        if (li == -1 || li == oki)
        {
            return;
        }

        if (gmx.pla[gbinfo.lm.cur_lmm].at != LED_ACTION_RIPPLE) 
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
            pkts->tlat = LED_ACTION_RIPPLE;
            pkts->ltdy = dly;
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
}

static void do_ripple(uint8_t ki, uint32_t count, uint8_t cr, uint8_t cg, uint8_t cb)
{
    uint32_t dly = 0;
    uint8_t r = 0, c = 0;

    get_led_location(ki, &r, &c);

    for (int i = 0; ; ++i)
    {
        int ci0 = 0, ci1 = 0;
        bool flag = false;

        dly = dly + count;

        ci0 = c + i; ci1 = c - i;

        if ((ci0 - 6) < BOARD_LED_COLUMN_NUM)
        {
            flag = true;
            for (int k = 0; k <= i; ++k)
            {
                do_set_ripple(ki, r + i - k, c + k, dly, cr, cg, cb);
                do_set_ripple(ki, r - i + k, c + k, dly, cr, cg, cb);
            }
        }

        if ((ci1 + 6) >= 0)
        {
            flag = true;
            for (int k = 0; k <= i; ++k)
            {
                do_set_ripple(ki, r - i + k, c - k, dly, cr, cg, cb);
                do_set_ripple(ki, r + i - k, c - k, dly, cr, cg, cb);
            }
        }

        if (!flag)
        {
            break;
        }
    }
}

void proc_ripple(uint8_t ki, void *pd1, void *pd2, void *pd3)
{

}

void keyp_ripple(uint8_t ki, led_action_t *pla)
{
    uint8_t ckrm = gbinfo.kr.cur_krm;

    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);
    key_led_color_t *pkr = get_key_led_color_ptr(ckrm, ki);

    if (pkts->kps == KEY_STATUS_U2D)
    {
        uint32_t dly = 0;

        if (pla->pr)
        {
            get_random_rgb(&pkts->ltkrr, &pkts->ltkrg, &pkts->ltkrb, ki);
        }
        else
        {
            pkts->ltkrr = pkr->r;
            pkts->ltkrg = pkr->g;
            pkts->ltkrb = pkr->b;
        }

        pkts->ltst = LED_STA_START;
        pkts->ext1 = ki;

        dly = ((pkts->lhl - pkts->lll) * pkts->lhzc / pkts->lls) >> 1;
        do_ripple(ki, dly, pkts->ltkrr, pkts->ltkrg, pkts->ltkrb);
    }
}