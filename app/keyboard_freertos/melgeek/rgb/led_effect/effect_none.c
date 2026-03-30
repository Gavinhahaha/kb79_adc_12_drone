#include "effect_common.h"


void init_none(uint8_t ki, led_action_t *pla)
{
    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(ki);

    pkts->lhl   = LED_BRIGHTNESS_MIN;
    pkts->lll   = LED_BRIGHTNESS_MIN;
    pkts->lls   = 0;
    pkts->lhzc  = 0;
    pkts->lpdy  = 0;
    pkts->ltkrr = 0;
    pkts->ltkrg = 0;
    pkts->ltkrb = 0;

    pkts->tlat  = LED_ACTION_SYS_MAX;
    pkts->ltdy  = 0;
    pkts->ltst  = LED_STA_NONE;
}

void proc_none(uint8_t ki, void *pd1, void *pd2, void *pd3)
{

}

void keyp_none(uint8_t ki, led_action_t *pla)
{
    
}