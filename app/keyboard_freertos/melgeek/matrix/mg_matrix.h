/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef _MG_MATRIX_H_
#define _MG_MATRIX_H_

#include "hal_gpio.h"
#include "hal_adc.h"
#include "db.h"
#include <stdio.h>
#include <stdarg.h>
#include "board.h"
#include "hpm_common.h"
#include "interface.h"



void matrix_ht_reload(sk_la_lm_kc_info_t *pdb);
void matrix_socd_reload(sk_la_lm_kc_info_t *pdb);
void matrix_rs_reload(sk_la_lm_kc_info_t *pdb);
void matrix_para_reset(void);
void matrix_para_clear(void);
void matrix_set_nkro(uint8_t nkro);
void matrix_reset(void);
void matrix_init(void);
void matrix_uninit(void);
void matrix_para_reload(void);
rep_hid_t *matrix_get_kb_report(void);
rep_hid_t *matrix_get_ms_report(void);
void matrix_para_check(key_attr_t *key_attr);
void  matrix_para_register(void);
void sm_sync_callback(void);
void sm_unsync_callback(void);
key_st_t *matrix_get_key_st(uint8_t index);
#endif
