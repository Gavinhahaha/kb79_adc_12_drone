#include "effect_common.h"



void init_breath(uint8_t ki, led_action_t *pla)
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

    {
        pkts->ltkrr = pkr->r;
        pkts->ltkrg = pkr->g;
        pkts->ltkrb = pkr->b;
    }

    pkts->ltdy = 0;
    pkts->tlat = LED_ACTION_SYS_MAX;
    pkts->ltst = LED_STA_START;
}

void proc_breath(uint8_t ki, void *pd1, void *pd2, void *pd3)
{
    static uint8_t ckrm = 0;
    static uint32_t rgb = 0;

    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);
    led_action_t *pla = &gmx.pla[*(uint8_t*)pd1];

    if (pla->rr)
    {
        if (pkts->ltst != LED_STA_L2H)
        {
            srand(xTaskGetTickCount());
            rgb = ((uint32_t)(rand() % 256) << 16) |
                  ((uint32_t)(rand() % 256) << 8) |
                  (uint32_t)(rand() % 256);
        }				
    }
    else
    {
        ckrm = gbinfo.kr.cur_krm;
    }
    key_led_color_t *pkr = get_key_led_color_ptr(ckrm, ki);
  
    if (pkts->ltst == LED_STA_STOP)
    {
        if (pla->rr)
        {
            pkts->ltkrr = (rgb >> 16) & 0xFF;
            pkts->ltkrg = (rgb >> 8) & 0xFF;
            pkts->ltkrb =  rgb & 0xFF;
        }
        else
        {
            pkts->ltkrr = pkr->r;
            pkts->ltkrg = pkr->g;
            pkts->ltkrb = pkr->b;
        }
    }
}

void keyp_breath(uint8_t ki, led_action_t *pla)
{
    uint8_t ckrm = gbinfo.kr.cur_krm;

    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);
    key_led_color_t *pkr = get_key_led_color_ptr(ckrm, ki);
		
    if (pkts->kps == KEY_STATUS_U2D)
    {
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
    }
}