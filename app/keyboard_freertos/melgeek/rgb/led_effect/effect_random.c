#include "effect_common.h"


void init_random(uint8_t ki, led_action_t *pla)
{
    uint8_t ckrm = gbinfo.kr.cur_krm;

    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);
    key_led_color_t *pkr = get_key_led_color_ptr(ckrm, ki);

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

    srand(xTaskGetTickCount() + ki * 3);

    pkts->lhl   = pla->hl;
    pkts->lll   = pla->ll;
    pkts->lls   = pla->ls;
    pkts->lhzc  = 1  + rand() % pla->hzc;
    pkts->lpdy  = 20 + rand() % pla->pdy;

    pkts->tlat  = LED_ACTION_SYS_MAX;
    pkts->ltdy  = pkts->lpdy;
    pkts->lttpt = pla->tpt;
    pkts->ltst  = LED_STA_START;
}

void proc_random(uint8_t ki, void *pd1, void *pd2, void *pd3)
{
    if (get_key_tmp_sta_ptr(ki)->ltst == LED_STA_STOP)
    {
        init_random(ki, &gmx.pla[*(uint8_t *)pd1]);
    }
}

void keyp_random(uint8_t ki, led_action_t *pla)
{
    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);

    if (pkts->kps == KEY_STATUS_U2D && pla->pr)
    {
        led_action_t la;

        memcpy(&la, pla, sizeof(led_action_t));
        la.rr = 1;
        init_random(ki, &la);
        pkts->ltdy = 0;
    }
}