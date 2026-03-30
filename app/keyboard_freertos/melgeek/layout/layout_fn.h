#ifndef  _LAYOUT_FN_
#define  _LAYOUT_FN_


#include "sk_adv.h"
#include <stdint.h>
#include <stdbool.h>

#define MS(value) (value/20)



void layout_fn_init(void);
bool layout_fn_check_enter(uint8_t ki);
bool layout_fn_check_exit(uint8_t ki);
void layout_fn_set_inactive(void);
void layout_fn_task_notify(void);
void layout_fn_handle_key_event(uint8_t ki, uint8_t sta);
void layout_fn_active_key_code_rpt(kc_t kc, uint8_t state);
uint8_t layout_fn_get_trg_sta(uint8_t ki);
void layout_fn_clr_trg_sta(uint8_t ki);
bool layout_fn_is_in(void);
#endif