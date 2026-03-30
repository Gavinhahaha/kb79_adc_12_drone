#include "effect_common.h"



void init_gradual(uint8_t ki, led_action_t *pla)
{
    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);

    pkts->lhl   = pla->hl;
    pkts->lll   = pla->ll;
    pkts->lls   = 0;
    pkts->lhzc  = 1;
    pkts->lhdy  = pla->hdy;
    pkts->lpdy  = pla->pdy;
    pkts->ltpt  = pla->tpt;
    pkts->lttpt = pkts->ltpt;

    pkts->tlat  = LED_ACTION_SYS_MAX;
    pkts->ltkrr = 255;
    pkts->ltkrg = 255;
    pkts->ltkrb = 0;
    pkts->ltl   = pkts->lhl;
    pkts->lltl  = pkts->ltl;
    pkts->ltts  = (pkts->lhl - pkts->lll) << 1;
    pkts->ltdy  = pkts->lhzc;
    pkts->ltst  = LED_STA_START;
}

void proc_gradual(uint8_t ki, void *pd1, void *pd2, void *pd3)
{
    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);
    led_action_t *pla = &gmx.pla[*(uint8_t *)pd1];

    if (pkts->ltdy != 0 || pkts->ltst == LED_STA_NONE)
    {
        return;
    }

    if (pkts->tlat == LED_ACTION_SYS_MAX)
    {
        do_color_gradual_change(&pkts->ltkrr, &pkts->ltkrg, &pkts->ltkrb, 255, 0, pla->ls);
    }
}

void keyp_gradual(uint8_t ki, led_action_t *pla)
{

}