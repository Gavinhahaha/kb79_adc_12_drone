/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef _EFFECT_COMMON_H_
#define _EFFECT_COMMON_H_

#include "db.h"



bool get_led_location(uint8_t ki, uint8_t *r, uint8_t *c);
void do_color_gradual_change(uint8_t *pr, uint8_t *pg, uint8_t *pb, uint8_t max, uint8_t min, uint8_t step);
void get_random_rgb(uint8_t *pr, uint8_t *pg, uint8_t *pb, uint32_t data);



void init_none(uint8_t ki, led_action_t *pla);
void proc_none(uint8_t ki, void *pd1, void *pd2, void *pd3);
void keyp_none(uint8_t ki, led_action_t *pla);

void init_bright(uint8_t ki, led_action_t *pla);
void proc_bright(uint8_t ki, void *pd1, void *pd2, void *pd3);
void keyp_bright(uint8_t ki, led_action_t *pla);

void init_gradual(uint8_t ki, led_action_t *pla);
void proc_gradual(uint8_t ki, void *pd1, void *pd2, void *pd3);
void keyp_gradual(uint8_t ki, led_action_t *pla);

void init_breath(uint8_t ki, led_action_t *pla);
void proc_breath(uint8_t ki, void *pd1, void *pd2, void *pd3);
void keyp_breath(uint8_t ki, led_action_t *pla);

void init_wave(uint8_t ki, led_action_t *pla);
void proc_wave(uint8_t ki, void *pd1, void *pd2, void *pd3);
void keyp_wave(uint8_t ki, led_action_t *pla);

void init_ripple(uint8_t ki, led_action_t *pla);
void proc_ripple(uint8_t ki, void *pd1, void *pd2, void *pd3);
void keyp_ripple(uint8_t ki, led_action_t *pla);

void init_reaction(uint8_t ki, led_action_t *pla);
void proc_reaction(uint8_t ki, void *pd1, void *pd2, void *pd3);
void keyp_reaction(uint8_t ki, led_action_t *pla);

void init_random(uint8_t ki, led_action_t *pla);
void proc_random(uint8_t ki, void *pd1, void *pd2, void *pd3);
void keyp_random(uint8_t ki, led_action_t *pla);

void init_rain(uint8_t ki, led_action_t *pla);
void proc_rain(uint8_t ki, void *pd1, void *pd2, void *pd3);
void keyp_rain(uint8_t ki, led_action_t *pla);

void init_ray(uint8_t ki, led_action_t *pla);
void proc_ray(uint8_t ki, void *pd1, void *pd2, void *pd3);
void keyp_ray(uint8_t ki, led_action_t *pla);

void init_gush(uint8_t ki, led_action_t *pla);
void proc_gush(uint8_t ki, void *pd1, void *pd2, void *pd3);
void keyp_gush(uint8_t ki, led_action_t *pla);

#endif