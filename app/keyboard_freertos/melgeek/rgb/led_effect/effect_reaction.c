#include "effect_common.h"


void init_reaction(uint8_t ki, led_action_t *pla)
{
    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);

    pkts->ltpt  = pla->tpt;
    pkts->lhl   = pla->hl;
    pkts->lll   = pla->ll;
    pkts->lls   = pla->ls;
    pkts->lhzc  = pla->hzc;
    pkts->lpdy  = pla->pdy;

    pkts->tlat  = LED_ACTION_SYS_MAX;
    pkts->lttpt = pkts->ltpt;
    pkts->ltdy  = 0;
    pkts->ltl   = pkts->lll;
    pkts->lltl  = pkts->ltl;
    pkts->ltts  = (pkts->lhl - pkts->lll) << 1;
    pkts->ltst  = LED_STA_NONE;
}

void proc_reaction(uint8_t ki, void *pd1, void *pd2, void *pd3)
{
  
}

void keyp_reaction(uint8_t ki, led_action_t *pla)
{
    uint8_t ckrm = gbinfo.kr.cur_krm;

    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);
    key_led_color_t *pkr = get_key_led_color_ptr(ckrm, ki);

    if (pkts->kps == KEY_STATUS_U2D)
    {
        if (pla->pr)
        {
            get_random_rgb(&pkts->ltkrr, &pkts->ltkrg, &pkts->ltkrb, ki * 3);
        }
        else
        {
            pkts->ltkrr = pkr->r;
            pkts->ltkrg = pkr->g;
            pkts->ltkrb = pkr->b;
        }
        pkts->ltst = LED_STA_START;
    }
}
