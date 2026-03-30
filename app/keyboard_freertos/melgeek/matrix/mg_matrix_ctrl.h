/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef _MG_MATRIX_CTRL_H_
#define _MG_MATRIX_CTRL_H_

#include "hal_gpio.h"
#include "hal_adc.h"
#include <stdio.h>
#include <stdarg.h>
#include "board.h"
#include "hpm_common.h"



bool key_cmd_set_rt_en(void);
bool key_cmd_set_rt_continue_en(void);

uint16_t ks_cmd_set_general_trigger_stroke(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);
uint16_t ks_cmd_get_general_trigger_stroke(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);

uint16_t ks_cmd_get_key_para(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);

uint16_t ks_cmd_set_general_trigger(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);
uint16_t ks_cmd_get_general_trigger(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);

uint16_t ks_cmd_set_rapid_trigger(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);
uint16_t ks_cmd_get_rapid_trigger(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);

uint16_t ks_cmd_set_dead_band(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);
uint16_t ks_cmd_get_dead_band(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);

uint16_t ks_cmd_set_key_report(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);
uint16_t ks_cmd_get_key_report(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);

uint16_t ks_cmd_get_cali(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);

uint16_t ks_cmd_set_dks_bind(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);
uint16_t ks_cmd_set_dks_para_table(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);
uint16_t ks_cmd_get_dks_para_table(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);
uint16_t ks_cmd_set_dks_tick(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);
uint16_t ks_cmd_get_dks_tick(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);

uint16_t key_cmd_set_tuning(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);
uint16_t key_cmd_get_tuning(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);

uint16_t ks_cmd_set_shaft_type(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);
uint16_t ks_cmd_get_default_shaft_type(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);

uint16_t ks_cmd_get_wave_max(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);

uint16_t ks_cmd_set_socd(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);
uint16_t ks_cmd_get_socd(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);

uint16_t ks_cmd_set_ht(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);
uint16_t ks_cmd_get_ht(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);

uint16_t ks_cmd_set_rs(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);
uint16_t ks_cmd_get_rs(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght);

#endif
