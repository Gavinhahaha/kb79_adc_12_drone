/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef _RGB_LED_CTRL_H_
#define _RGB_LED_CTRL_H_

#include <stdint.h>		
#include <stdbool.h>

typedef  enum
{
    STATE_IDLE,
    STATE_READY,
}led_update_state;

#define  CAPSLOCK_LED_SET_ON()    led_indicator_led_ctrl(LED_BRIGHTNESS_MAX, 255, 255, 255)
#define  CAPSLOCK_LED_SET_OFF()   led_indicator_led_ctrl(LED_BRIGHTNESS_MIN, 0, 0, 0)


void led_set_color_dircet(uint8_t i, uint8_t l, uint8_t r, uint8_t g, uint8_t b);
void led_set_color(uint8_t i, uint8_t l, uint8_t r, uint8_t g, uint8_t b);
void led_lamp_set_rgb(uint8_t i, uint8_t l, uint8_t r, uint8_t g, uint8_t b);
void led_set_all_color(uint8_t l, uint8_t r, uint8_t g, uint8_t b);
bool led_update_frame(uint8_t mode);
void led_init(void);
void led_uninit(void);
void led_set_update_state(led_update_state state);
uint8_t led_refresh_sys_led(void);
uint8_t led_get_sys_led(void);
void led_set_sys_led_status(uint8_t status);
void led_set_factory_sys_led(uint8_t status);
void led_indicator_led_ctrl(uint8_t l, uint8_t r, uint8_t g, uint8_t b);
void led_register(void *complete_handler, void *wait_trans_handler);
uint8_t led_get_caps_led_status(void);
led_update_state led_get_update_state(void);


#endif