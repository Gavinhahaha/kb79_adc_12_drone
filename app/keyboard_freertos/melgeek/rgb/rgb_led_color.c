/*
 * Copyright (c) 2024-2025 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#include "rgb_led_color.h"
#include "rgb_led_ctrl.h"
#include "db.h"

static const uint32_t layer_default_color[BOARD_LED_COLOR_LAYER_MAX] = {
    WHITE,      /* 1 */
    // RED,        /* 2 */
    // GREEN,      /* 3 */
    // BLUE,       /* 4 */
    // YELLOW,     /* 5 */
    // ORINGE,     /* 6 */
    // PURPLE,     /* 7 */
    // CYAN,       /* 8 */
};


void set_rgb_from_hex(uint32_t color, key_led_color_t *pkr)
{
    pkr->m = (color >> 24) & 0xFF;
    pkr->r = (color >> 16) & 0xFF;
    pkr->g = (color >>  8) & 0xFF;
    pkr->b = (color >>  0) & 0xFF;
}

uint32_t get_hex_from_rgb(key_led_color_t *pkr)
{
    uint32_t color = 0x0;

    color = (pkr->m << 24) | (pkr->r << 16) | (pkr->g << 8) | (pkr->b << 0);

    return color;
}

uint32_t set_key_led_color(uint8_t id, uint8_t l, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t ckrm = gbinfo.kr.cur_krm; 

    key_tmp_sta_t *pkts = get_key_tmp_sta_ptr(id);
    key_led_color_t *pkr = get_key_led_color_ptr(ckrm, id);
      
    if (id >= BOARD_KEY_LED_NUM)
    {
        return 1;
    }

    pkr->r = r;
    pkr->g = g;
    pkr->b = b;

    pkts->ltl   = l;
    pkts->ltkrr = r;
    pkts->ltkrg = g;
    pkts->ltkrb = b;

    led_set_color(id, l, r, g, b);

    return 0;
}

void set_key_led_color_mask(uint8_t id, uint8_t krm, uint8_t flag)
{
    key_led_color_t *pkr = get_key_led_color_ptr(krm, id);
    pkr->m = flag;

}

void set_all_key_led_color(uint8_t l, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < BOARD_KEY_LED_NUM; ++i)
    {
        set_key_led_color(i, l, r, g, b);
    }
}

const uint32_t *get_rgb_layer_color(void)
{
    return layer_default_color;
}