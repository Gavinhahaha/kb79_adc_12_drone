#include "effect_common.h"



void init_bright(uint8_t ki, led_action_t *pla)
{
    uint8_t ckrm = gbinfo.kr.cur_krm;

    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);
    key_led_color_t *pkr = get_key_led_color_ptr(ckrm, ki);
    

    pkts->lhl   = pla->hl;
    pkts->lll   = pla->ll;
    pkts->lls   = pla->ls;
    pkts->lhzc  = pla->hzc;
    pkts->lhdy  = pla->hdy;
    pkts->lpdy  = pla->pdy;
    pkts->ltpt  = pla->tpt;
    pkts->lttpt = pkts->ltpt;

    if (pla->rr)
    {
        get_random_rgb(&pkts->ltkrr, &pkts->ltkrg, &pkts->ltkrb, ki * 17);
    }
    else
    {
        pkts->ltkrr = pkr->r;
        pkts->ltkrg = pkr->g;
        pkts->ltkrb = pkr->b;
    }

    pkts->ltdy = 0;
    pkts->tlat = LED_ACTION_SYS_MAX;
    pkts->ltl  = pla->bd ? pkts->lhl : pkts->lll;
    pkts->ltts = (pkts->lhl - pkts->lll) << 1;
    pkts->ltst = LED_STA_NONE;
}

void proc_bright(uint8_t ki, void *pd1, void *pd2, void *pd3)
{

}

void keyp_bright(uint8_t ki, led_action_t *pla)
{
    
}