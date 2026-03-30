/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef _MG_CALI_H_
#define _MG_CALI_H_
#include "mg_matrix_type.h"
#include "mg_sm.h"

extern cali_tuning_t tuning;

enum
{
    NO_UPDATA = 0,
    LATER_UPDATA,
    NEED_UPDATA,
};

enum
{
    KEY_TOP = 0,
    KEY_BOT,
    KEY_CAIL_STATE_MAX,
};

typedef struct
{
    uint8_t waite_top_cali_updata_buf[BOARD_KEY_NUM];
    uint8_t waite_bot_cali_updata_buf[BOARD_KEY_NUM];
    uint16_t key_temp_cail_buf[KEY_CAIL_STATE_MAX][BOARD_KEY_NUM];
    uint64_t cail_wait_1000ms;
}key_cail_param_s;

void cali_db_register(void);
bool cali_is_ready(void);
void cali_tuning_set_state(tuning_e en);
void cali_rpt_dbg_set_state(bool en);
void cali_set_peek_adc(bool en);
void cali_set_rpt(key_rpt_e type);
void cali_calc_level_boundarys(void);
void cali_calc_level_boundary(uint8_t ki);
cail_ack_t cali_cmd(cail_step_t calibFlag);
cail_step_t cali_get_rpt(void);
kh_data_t *cali_peek_new_adc_fifo(void);
tuning_e cali_tuning_get_state(void);
cail_step_t cali_get_statue(void);
void cali_init(kh_t *p_kh);
void cali_tuning_update(uint8_t ki);
uint8_t cali_auto_is_change(void);
void cali_auto_set_change(uint8_t is_change);
uint8_t cali_is_running(void);
bool cali_data_updata_handle(sm_data_t *p_sm_data);
void cali_manual_updata_handle(sm_data_t *p_sm_data);
#endif
