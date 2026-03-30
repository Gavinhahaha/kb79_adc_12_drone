/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include "rgb_light_tape.h"
#include "drv_ws2812.h"
#include "db.h"
#include <math.h>
#include "mg_build_date.h"
#include "app_debug.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (0)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "rgb_light_tabe"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#define TIMER_EXPIRED(current, future) ((uint16_t)(current - future) < UINT16_MAX / 2)
#define TIMER_EXPIRED32(current, future) ((uint32_t)(current - future) < UINT32_MAX / 2)

#define SET_DEFAULT_MS(ms_ptr)                   \
    do                                           \
    {                                            \
        (ms_ptr)->hsv.h = RGBLIGHT_DEFAULT_HUE;  \
        (ms_ptr)->hsv.s = RGBLIGHT_DEFAULT_SAT;  \
        (ms_ptr)->hsv.v = RGBLIGHT_DEFAULT_VAL;  \
        (ms_ptr)->speed = RGBLIGHT_DEFAULT_SPD;  \
    } while (0)

static rgb_t rgb_buff[BOARD_LIGHT_TAPE_LED_NUM] = {0};
static animation_status_t animation_status[LIGHT_TAPE_PART_MAX] = {
    [LIGHT_TAPE_PART_BACK] = {.restart = true, .pos = 1},
    [LIGHT_TAPE_PART_SIDE] = {.restart = true, .pos = 1},
};
static rgblight_ranges_t rgblight_ranges[LIGHT_TAPE_PART_MAX];
static uint16_t rgb_axis_count;
static bool hardware_light = 0;

static const light_tape_state_t *p_rgb_state = NULL;
static uint8_t rgb_state_num = 0;
static uint8_t rgb_num = 0;
static uint16_t rgb_count = 0;
static uint8_t play_start = 0;
static uint8_t light_tape_status = 0;
static rgb_t lamp_buff[BOARD_LIGHT_TAPE_LED_NUM] = {0};

static bool need_refresh = false;

static rgblight_ranges_t s_ranges[LIGHT_TAPE_PART_MAX] =
{
    {
        BOARD_LIGHT_TAPE_BACK_COL_NUM,
        BOARD_LIGHT_TAPE_BACK_ROW_NUM,
        0,
        BOARD_LIGHT_TAPE_BACK_LED_NUM,
        0,
        BOARD_LIGHT_TAPE_BACK_LED_NUM,
        BOARD_LIGHT_TAPE_BACK_LED_NUM,
    },
    {
        BOARD_LIGHT_TAPE_SIDE_LED_NUM,
        1,
        BOARD_LIGHT_TAPE_BACK_LED_NUM,
        BOARD_LIGHT_TAPE_SIDE_LED_NUM,
        BOARD_LIGHT_TAPE_BACK_LED_NUM,
        BOARD_LIGHT_TAPE_LED_NUM,
        BOARD_LIGHT_TAPE_SIDE_LED_NUM,
    },
};

static const rgblight_mode mode_id[RGBLIGHT_EFFECT_MAX] = {
    RGBLIGHT_EFFECT_CLOSE,
    RGBLIGHT_EFFECT_STATIC,
    RGBLIGHT_EFFECT_BREATHING,
    RGBLIGHT_EFFECT_RAINBOW_MOOD,
    RGBLIGHT_EFFECT_RAINBOW_SWIRL,
    RGBLIGHT_EFFECT_SNAKE,
    RGBLIGHT_EFFECT_KNIGHT,
    RGBLIGHT_EFFECT_RGB_TEST,
    RGBLIGHT_EFFECT_ALTERNATING,
    RGBLIGHT_EFFECT_RANDOM,
    RGBLIGHT_EFFECT_ROTATE,
};

const uint8_t* rgblight_get_mode_list(void)
{
    return mode_id;
}

static void hsv_to_rgb(hsv_t *hsv, rgb_t *rgb) 
{
    uint8_t region;
    uint16_t remainder, p, q, t;
    uint16_t h, s, v;
    uint8_t adjusted = 0;

    if (hsv->s == 0 || hsv->v == 0) 
    {
        rgb->r = hsv->v;
        rgb->g = hsv->v;
        rgb->b = hsv->v;
        return;
    }

    h = hsv->h;
    s = (uint8_t)(hsv->s / 255.0 * 135 + 120);
    v = hsv->v;

    region = h * 6 / 255;
    remainder = (h * 2 - region * 85) * 3;

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
        case 6:
        case 0:
            rgb->r = v;
            adjusted ? (rgb->g = t) : (rgb->g = t * t / v / 2);
            adjusted ? (rgb->b = p) : (rgb->b = p * p / v / 3);
            break;
        case 1:
            rgb->r = q;
            adjusted ? (rgb->g = v) : (rgb->g = v / 2);
            adjusted ? (rgb->b = p) : (rgb->b = p * p / v / 3);
            break;
        case 2:
            rgb->r = p;
            adjusted ? (rgb->g = v) : (rgb->g = v / 2);
            adjusted ? (rgb->b = t) : (rgb->b = t * t / v / 2);
            break;
        case 3:
            rgb->r = p;
            adjusted ? (rgb->g = q) : (rgb->g = q * q / v / 2);
            adjusted ? (rgb->b = v) : (rgb->b = v / 2);
            break;
        case 4:
            adjusted ? (rgb->r = t) : (rgb->r = t * t / v);
            rgb->g = p;
            adjusted ? (rgb->b = v) : (rgb->b = v / 2);
            break;
        default:
            rgb->r = v;
            adjusted ? (rgb->g = p) : (rgb->g = p * p / v );
            adjusted ? (rgb->b = q) : (rgb->b = q * q / v / 2);
            break;
    }

    #if 0
    if (RGBLIGHT_EFFECT_STATIC == gbinfo.rl[LIGHT_TAPE_PART_BACK].md)
    {
        float coef = 255.0f / v;

        rgb->r *= coef;
        rgb->g *= coef;
        rgb->b *= coef;
    }
    #endif 
}

static void setrgb(uint8_t r, uint8_t g, uint8_t b, rgb_t *led1) 
{
    led1->r = r;
    led1->g = g;
    led1->b = b;
}

static void sethsv_raw(uint8_t hue, uint8_t sat, uint8_t val, rgb_t *led1) 
{
    hsv_t hsv = {hue, sat, val};
    rgb_t rgb;
    hsv_to_rgb(&hsv,&rgb);
    setrgb(rgb.r, rgb.g, rgb.b, led1);
}

static void sethsv(uint8_t hue, uint8_t sat, uint8_t val, uint8_t lum, rgb_t *led1)
{
    sethsv_raw(hue, sat, val > RGBLIGHT_LIMIT_VAL ? RGBLIGHT_LIMIT_VAL : val, led1);
}

static uint8_t breathe_calc(uint8_t pos) 
{
    float sin_val = sin((pos / 255.0f) * M_PI); // 正弦波
    float power = 2.0f;                         // 调整这个幂次可以控制亮度的变化曲线
    sin_val = pow(sin_val, power);

    return (exp(sin_val) - RGBLIGHT_EFFECT_BREATHE_CENTER / M_E) * (RGBLIGHT_EFFECT_BREATHE_MAX / (M_E - 1 / M_E));
}

static void rgblight_mark_refresh(void)
{
    if (need_refresh != true)
    {
        need_refresh = true;
    }
}

void rgblight_flush(void)
{
    if (need_refresh) 
    {
        drv_ws2812_set_leds(rgb_buff, BOARD_LIGHT_TAPE_LED_NUM);
        need_refresh = false;
    }
}

static void rgblight_sethsv_noeeprom_old(rgblight_ranges_t *ranges, uint8_t hue, uint8_t sat, uint8_t val, uint8_t lum, uint8_t part)
{
    rgb_t tmp_led;
    sethsv(hue, sat, val, lum, &tmp_led);

    for (uint8_t i = ranges[part].effect_start_pos; i < ranges[part].effect_end_pos; i++) 
    {
        rgb_buff[i].r = tmp_led.r;
        rgb_buff[i].g = tmp_led.g;
        rgb_buff[i].b = tmp_led.b;
    }

    rgblight_mark_refresh();
}

static void rgblight_setrgb(rgblight_ranges_t *ranges, uint8_t r, uint8_t g, uint8_t b, uint8_t part)
{
    for (uint8_t i = ranges[part].effect_start_pos; i < ranges[part].effect_end_pos; i++) 
    {
        rgb_buff[i].r = r;
        rgb_buff[i].g = g;
        rgb_buff[i].b = b;
    }
    rgblight_mark_refresh();
}

static uint8_t lqadd_x(uint8_t i, uint8_t j, uint8_t max)
{
    uint16_t t = i + j;
    if (t > max) t = 0;
    return t;
}

static uint16_t lqsub_x(uint8_t i, uint8_t j, uint8_t max)
{
    int16_t t = i - j;
    if (t < 0) t = max;
    return t;
}

static uint8_t qadd_x(uint8_t i, uint8_t j, uint8_t max)
{
    uint16_t t = i + j;
    if (t > max) t = max;
    return t;
}

static uint16_t qsub_x(uint8_t i, uint8_t j,uint8_t min)
{
    int16_t t = i - j;
    if (t < min) t = min;
    return t;
}

static void rgblight_sethsv_eeprom_helper(uint8_t hue, uint8_t sat, uint8_t val, uint8_t part) 
{
    if (mode_id[gbinfo.rl[part].md] == RGBLIGHT_EFFECT_BREATHING)
    {
        // breathing mode, ignore the change of val, use in memory value instead
        val = gbinfo.rl[part].ms[gbinfo.rl[part].md].hsv.v;
    }
    else if (mode_id[gbinfo.rl[part].md] == RGBLIGHT_EFFECT_RAINBOW_MOOD)
    {
        // rainbow mood, ignore the change of hue
        hue = gbinfo.rl[part].ms[gbinfo.rl[part].md].hsv.h;
    }
    else if (mode_id[gbinfo.rl[part].md] == RGBLIGHT_EFFECT_RAINBOW_SWIRL)
    {
        // rainbow swirl, ignore the change of hue
        hue = gbinfo.rl[part].ms[gbinfo.rl[part].md].hsv.h;
    }

    gbinfo.rl[part].ms[gbinfo.rl[part].md].hsv.h = hue;
    gbinfo.rl[part].ms[gbinfo.rl[part].md].hsv.s = sat;
    gbinfo.rl[part].ms[gbinfo.rl[part].md].hsv.v = val;
}

void light_tape_back_increase_hue(void) 
{
    uint8_t hue = lqadd_x(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h, 8, RGBLIGHT_LIMIT_HUE);
    rgblight_sethsv_eeprom_helper(hue, gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s, gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v, LIGHT_TAPE_PART_BACK);
}

void light_tape_back_decrease_hue(void) 
{
    uint8_t hue = lqsub_x(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h, 8, RGBLIGHT_LIMIT_HUE);
    rgblight_sethsv_eeprom_helper(hue, gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s, gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v, LIGHT_TAPE_PART_BACK);
}

void light_tape_back_increase_sat(void) 
{
    uint8_t sat = qadd_x(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s, 16, RGBLIGHT_LIMIT_SAT);
    rgblight_sethsv_eeprom_helper(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h, sat, gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v, LIGHT_TAPE_PART_BACK);
}

void light_tape_back_decrease_sat(void)
{
    uint8_t sat = qsub_x(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s, 16, 0);
    rgblight_sethsv_eeprom_helper(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h, sat, gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v, LIGHT_TAPE_PART_BACK);
}

void light_tape_back_increase_val(void) 
{
    uint8_t val = qadd_x(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v, 50, RGBLIGHT_LIMIT_VAL);
    rgblight_sethsv_eeprom_helper(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h, gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s, val, LIGHT_TAPE_PART_BACK);
}

void light_tape_back_decrease_val(void) 
{
    uint8_t val = qsub_x(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v, 50, 0);
    rgblight_sethsv_eeprom_helper(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h, gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s, val, LIGHT_TAPE_PART_BACK);
}

void light_tape_back_increase_val_by_step(uint8_t step) 
{
    uint8_t val = qadd_x(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v, step, RGBLIGHT_LIMIT_VAL);
    rgblight_sethsv_eeprom_helper(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h, gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s, val, LIGHT_TAPE_PART_BACK);
}

void light_tape_back_decrease_val_by_step(uint8_t step) 
{
    uint8_t val = qsub_x(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v, step, 0);
    rgblight_sethsv_eeprom_helper(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.h, gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.s, val, LIGHT_TAPE_PART_BACK);
}

void light_tape_back_increase_speed(void)
{
    gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].speed = qadd_x(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].speed, 1, 8);
}

void light_tape_back_decrease_speed(void)
{
    gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].speed = qsub_x(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].speed, 1, 1);
}

void light_tape_side_increase_hue(void) 
{
    uint8_t hue = lqadd_x(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h, 8, RGBLIGHT_LIMIT_HUE);
    rgblight_sethsv_eeprom_helper(hue, gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s, gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v, LIGHT_TAPE_PART_SIDE);
}
void light_tape_side_decrease_hue(void) 
{
    uint8_t hue = lqsub_x(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h, 8, RGBLIGHT_LIMIT_HUE);
    rgblight_sethsv_eeprom_helper(hue, gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s, gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v, LIGHT_TAPE_PART_SIDE);
}
void light_tape_side_increase_sat(void) 
{
    uint8_t sat = qadd_x(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s, 16, RGBLIGHT_LIMIT_SAT);
    rgblight_sethsv_eeprom_helper(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h, sat, gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v, LIGHT_TAPE_PART_SIDE);
}
void light_tape_side_decrease_sat(void)
{
    uint8_t sat = qsub_x(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s, 16, 0);
    rgblight_sethsv_eeprom_helper(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h, sat, gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v, LIGHT_TAPE_PART_SIDE);
}
void light_tape_side_increase_val(void) 
{
    uint8_t val = qadd_x(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v, 50, RGBLIGHT_LIMIT_VAL);
    rgblight_sethsv_eeprom_helper(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h, gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s, val, LIGHT_TAPE_PART_SIDE);
}
void light_tape_side_decrease_val(void) 
{
    uint8_t val = qsub_x(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v, 50, 0);
    rgblight_sethsv_eeprom_helper(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h, gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s, val, LIGHT_TAPE_PART_SIDE);
}

void light_tape_side_increase_val_by_step(uint8_t step) 
{
    uint8_t val = qadd_x(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v, step, RGBLIGHT_LIMIT_VAL);
    rgblight_sethsv_eeprom_helper(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h, gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s, val, LIGHT_TAPE_PART_SIDE);
}

void light_tape_side_decrease_val_by_step(uint8_t step) 
{
    uint8_t val = qsub_x(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v, step, 0);
    rgblight_sethsv_eeprom_helper(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.h, gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.s, val, LIGHT_TAPE_PART_SIDE);
}

void light_tape_side_increase_speed(void)
{
    gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].speed = qadd_x(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].speed, 1, 8);
}
void light_tape_side_decrease_speed(void)
{
    gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].speed = qsub_x(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].speed, 1, 1);
}

static uint16_t sync_timer_read(void)
{
    return rgb_axis_count;
}

void light_tape_update_sync_timer(uint8_t task_tick)
{
    rgb_axis_count += task_tick;
}

static void light_tape_prcess(light_tape_t *rl)
{
    rl->state = 1;
    rl->md = RGBLIGHT_DEFAULT_MODE;
    for (uint8_t i = 0; i < RGBLIGHT_EFFECT_MAX; i++)
    {
        SET_DEFAULT_MS(&rl->ms[i]);
        switch (mode_id[i])
        {
        case RGBLIGHT_EFFECT_RAINBOW_MOOD:
        case RGBLIGHT_EFFECT_RAINBOW_SWIRL:
        case RGBLIGHT_EFFECT_CHRISTMAS:
        case RGBLIGHT_EFFECT_ROTATE:
            rl->ms[i].hsv.h = RGBLIGHT_DEFAULT_HUE;
            rl->ms[i].hsv.s = 255;
            rl->ms[i].hsv.v = RGBLIGHT_DEFAULT_VAL;
            rl->ms[i].speed = RGBLIGHT_DEFAULT_SPD;
            break;

        default:
            break;
        }
    }
}

void light_tape_check_para(light_tape_t *rl_array)
{
    for (uint8_t part = 0; part < LIGHT_TAPE_PART_MAX; part++)
    {
        light_tape_t *rl = &rl_array[part];
        uint8_t reset_flag = 0;

        if ((rl->state > 1) || (rl->md >= RGBLIGHT_EFFECT_MAX))
        {
            reset_flag = 1;
        }
        else
        {
            for(uint8_t i = 0; i < RGBLIGHT_EFFECT_MAX; i++)
            {
                if ((rl->ms[i].speed > 8) || (rl->ms[i].hsv.v > RGBLIGHT_LIMIT_VAL) || ((rl->ms[i].hsv.v == 0) && (rl->ms[i].speed == 0)))
                {
                    reset_flag = 1;
                    break;
                }
            }
        }

        if (reset_flag)
        {
            light_tape_prcess(rl);
        }
    }
}

void light_tape_set_default(void)
{
    for (uint8_t part = 0; part < LIGHT_TAPE_PART_MAX; part++)
    {
        light_tape_prcess(&gbinfo.rl[part]);
    }
}

#define IGNORE  (254U)
#define AUTOMA  (255U)

static void rgb_apply_at_index(uint16_t i, uint16_t mode_row, uint16_t mode_col,
                               uint8_t hue, uint8_t sat, uint8_t val, uint8_t lum, light_dir_t dir, bool inv)
{
    const uint16_t rows = BOARD_LIGHT_TAPE_BACK_ROW_NUM;
    const uint16_t cols = BOARD_LIGHT_TAPE_BACK_COL_NUM;
    bool row_dir = inv ? (!dir) : (dir);
    bool col_dir = inv ? (!dir) : (dir);

    if (mode_row == IGNORE && mode_col != IGNORE)
    {
        if (i < cols)
        {
            uint16_t target_col = (col_dir) ? (cols - 1 - i) : i;

            if (mode_col == AUTOMA)
            {
                for (uint16_t row = 0; row < rows; row++)
                {
                    uint16_t adjusted_row = (row_dir) ? (rows - 1 - row) : row;
                    uint16_t idx = adjusted_row * cols + target_col;
                    sethsv(hue, sat, val, lum, (rgb_t *)&rgb_buff[idx]);
                }
            }
            else
            {
                uint16_t adjusted_row = (row_dir) ? (rows - 1 - mode_col) : mode_col;
                uint16_t idx = adjusted_row * cols + target_col;
                sethsv(hue, sat, val, lum, (rgb_t *)&rgb_buff[idx]);
            }
        }
        return;
    }

    if (mode_col == IGNORE && mode_row != IGNORE)
    {
        if (i < rows)
        {
            uint16_t target_row = (row_dir) ? (rows - 1 - i) : i;

            if (mode_row == AUTOMA)
            {
                for (uint16_t col = 0; col < cols; col++)
                {
                    uint16_t adjusted_col = (col_dir) ? (cols - 1 - col) : col;
                    uint16_t idx = target_row * cols + adjusted_col;
                    sethsv(hue, sat, val, lum, (rgb_t *)&rgb_buff[idx]);
                }
            }
            else
            {
                uint16_t adjusted_col = (col_dir) ? (cols - 1 - mode_row) : mode_row;
                uint16_t idx = target_row * cols + adjusted_col;
                sethsv(hue, sat, val, lum, (rgb_t *)&rgb_buff[idx]);
            }
        }
        return;
    }

    if (i < rows * cols)
    {
        uint16_t adjusted_row = (row_dir) ? (rows - 1 - i / cols) : (i / cols);
        uint16_t adjusted_col = (col_dir) ? (cols - 1 - i % cols) : (i % cols);
        uint16_t idx = adjusted_row * cols + adjusted_col;
        sethsv(hue, sat, val, lum, (rgb_t *)&rgb_buff[idx]);
    }
}

static void rgb_apply_at_dir(uint16_t i, uint8_t hue, uint8_t sat, uint8_t val, uint8_t lum, light_dir_t dir, bool inv)
{
    bool col_dir = inv ? (!dir) : dir;
    if (col_dir)
    {
        sethsv(hue, sat, val, lum, &rgb_buff[BOARD_LIGHT_TAPE_LED_NUM - 1 + BOARD_LIGHT_TAPE_BACK_LED_NUM - i]);
    }
    else
    {
        sethsv(hue, sat, val, lum, &rgb_buff[i]);
    }
}

typedef void (*effect_func_t)(animation_status_t *anim, rgblight_ranges_t *ranges, uint8_t part);

/********************** Mode 0: close ***************************/
static void rgblight_effect_close(animation_status_t *anim, rgblight_ranges_t *ranges, uint8_t part)
{
    rgblight_sethsv_noeeprom_old(ranges, 0, 0, 0, 0, part);
}

/********************** Mode 1: static ***************************/
static void rgblight_effect_static(animation_status_t *anim, rgblight_ranges_t *ranges, uint8_t part)
{
    const light_tape_config_t config = gbinfo.rl[part].ms[gbinfo.rl[part].md];
    #if(RGB_WHITE_TUNING==0)
	rgblight_sethsv_noeeprom_old(ranges, config.hsv.h, config.hsv.s, config.hsv.v);
    DBG_PRINTF("h = %d,s = %d,v=%d\n", config.hsv.h, config.hsv.s,config.hsv.v);
    //rgblight_setrgb_noeeprom_old(config.hsv.h, config.hsv.s, config.hsv.v);
    #else
    uint8_t v = hardware_light ? (config.hsv.v) : (config.hsv.v * 0.59);
    // if ((h == 0) && (s == 0))
    // {
    //     #if 0
    //     for (uint8_t i = 0; i < BOARD_LIGHT_TAPE_LED_NUM; i++)
    //     {
    //         rgb_buff[i].r = v; // tmp_led.r;
    //         rgb_buff[i].g = v; // tmp_led.g;
    //         rgb_buff[i].b = v; // tmp_led.b;
    //     }
    //     drv_ws2812_set_leds(rgb_buff, BOARD_LIGHT_TAPE_LED_NUM);
    //     #else
    //     rgblight_setrgb(ranges, l, l, l, part);
    //     #endif
    //     //s = v * 0.4;
    //     //h = v * 0.4;
    // }
    // else
    {
        rgblight_sethsv_noeeprom_old(ranges, config.hsv.h, config.hsv.s, v, 0, part);
    }
    #endif
}


/********************** Mode 1: Breathing ***************************/

static void rgblight_effect_breathing(animation_status_t *anim, rgblight_ranges_t *ranges, uint8_t part)
{
    const light_tape_config_t config = gbinfo.rl[part].ms[gbinfo.rl[part].md];
    uint8_t val = breathe_calc(anim[part].pos);
    #if(RGB_WHITE_TUNING==0)
    rgblight_sethsv_noeeprom_old(ranges, config.hsv.h, config.hsv.s, val);
	#else

    // if ((h == 0) && (s == 0))
    // {
    //     #if 0
    //     for (uint8_t i = 0; i < BOARD_LIGHT_TAPE_LED_NUM; i++)
    //     {
    //         rgb_buff[i].r = val;		//tmp_led.r;
    //         rgb_buff[i].g = val * 0.4; //tmp_led.g;
    //         rgb_buff[i].b = val * 0.4; // tmp_led.b;
    //     }
    //     drv_ws2812_set_leds(rgb_buff, BOARD_LIGHT_TAPE_LED_NUM);

    //     #else
    //     rgblight_setrgb(ranges, val, val, val, part);
    //     #endif
    // }
    // else
    {
        rgblight_sethsv_noeeprom_old(ranges, config.hsv.h, config.hsv.s, val, 0, part);
    }
    #endif
    anim[part].pos = (anim[part].pos + 3);
}

/********************** Mode 2: rainbow ***************************/

static void rgblight_effect_rainbow_mood(animation_status_t *anim, rgblight_ranges_t *ranges, uint8_t part)
{
    const light_tape_config_t config = gbinfo.rl[part].ms[gbinfo.rl[part].md];

    rgblight_sethsv_noeeprom_old(ranges, anim[part].current_hue, config.hsv.s, config.hsv.v, 0, part);
    anim[part].current_hue++;
}

/****************** Mode 3: rainbow swirl **************************/
static void rgblight_effect_rainbow_swirl(animation_status_t *anim, rgblight_ranges_t *ranges, uint8_t part)
{
    const uint16_t start_pos = ranges[part].effect_start_pos;
    const light_tape_config_t config = gbinfo.rl[part].ms[gbinfo.rl[part].md];
    bool dir = (config.speed % 2) ? false : true;
    const uint16_t section_leds = ranges[part].clipping_col;
    for (uint8_t i = 0; i < section_leds; i++)
    {
        uint16_t hue16 = (RGBLIGHT_RAINBOW_SWIRL_RANGE * i )/ section_leds + anim[part].current_hue;
        uint8_t hue = (uint8_t)(hue16 & 0xFF);
        if (i < BOARD_LIGHT_TAPE_BACK_COL_NUM && part == LIGHT_TAPE_PART_BACK)
        {
            rgb_apply_at_index(i, IGNORE, AUTOMA, hue, config.hsv.s, config.hsv.v, 0, dir, false);
        }
        else if (i >= BOARD_LIGHT_TAPE_BACK_COL_NUM && part == LIGHT_TAPE_PART_BACK)
        {
            rgb_apply_at_dir(i + start_pos + ranges[part].clipping_num_leds - section_leds, hue, config.hsv.s, config.hsv.v, 0, dir, false);
        }
        else
        {
            rgb_apply_at_dir(i + start_pos, hue, config.hsv.s, config.hsv.v, 0, dir, false);
        }
    }
    anim[part].current_hue++;

    rgblight_mark_refresh();

}

/****************** Mode 4: snake **************************/

static void rgblight_effect_snake(animation_status_t *anim, rgblight_ranges_t *ranges, uint8_t part)
{
    static uint8_t pos[LIGHT_TAPE_PART_MAX] = {0};
    const uint16_t section_leds = ranges[part].clipping_col;
    const uint16_t start_pos = ranges[part].effect_start_pos;
    const light_tape_config_t config = gbinfo.rl[part].ms[gbinfo.rl[part].md];
    const uint8_t active_len = 7;

    bool dir = (config.speed % 2) ? false : true;

    rgblight_sethsv_noeeprom_old(ranges, 0, 0, 0, 0, part);

    for (uint8_t i = 0; i < section_leds; i++)
    {
        for (uint8_t j = 0; j < active_len; j++)
        {
            uint16_t k = (pos[part] + j + section_leds) % section_leds;
            if (i == k)
            {
                if (i < BOARD_LIGHT_TAPE_BACK_COL_NUM && part == LIGHT_TAPE_PART_BACK)
                {
                    rgb_apply_at_index(i, IGNORE, AUTOMA, config.hsv.h, config.hsv.s, (uint8_t)(config.hsv.v * (active_len - j) / active_len), 0, dir, false);
                }
                else if (i >= BOARD_LIGHT_TAPE_BACK_COL_NUM && part == LIGHT_TAPE_PART_BACK)
                {
                    rgb_apply_at_dir(i + start_pos + ranges[part].clipping_num_leds - section_leds, config.hsv.h, config.hsv.s, (uint8_t)(config.hsv.v * (active_len - j) / active_len), 0, dir, false);
                }
                else
                {
                    rgb_apply_at_dir(i + start_pos, config.hsv.h, config.hsv.s, (uint8_t)(config.hsv.v * (active_len - j) / active_len), 0, dir, false);
                }
            }
        }
    }

    pos[part] = (pos[part] - RGBLIGHT_EFFECT_SNAKE_INCREMENT + section_leds) % section_leds;

    rgblight_mark_refresh();

}

/****************** Mode 5: knight **************************/

static void rgblight_effect_knight(animation_status_t *anim, rgblight_ranges_t *ranges, uint8_t part)
{

    const uint16_t section_leds = ranges[part].clipping_col;
    const uint16_t start_pos = ranges[part].effect_start_pos;
    const light_tape_config_t config = gbinfo.rl[part].ms[gbinfo.rl[part].md];

    static int16_t low_bound[LIGHT_TAPE_PART_MAX] = {0};
    static int16_t high_bound[LIGHT_TAPE_PART_MAX] = {RGBLIGHT_EFFECT_KNIGHT_LENGTH, RGBLIGHT_EFFECT_KNIGHT_LENGTH};
    static int16_t increment[LIGHT_TAPE_PART_MAX] = {RGBLIGHT_EFFECT_KNIGHT_INCREMENT, RGBLIGHT_EFFECT_KNIGHT_INCREMENT};

    if (anim[part].restart)
    {
        low_bound[part] = 0;
        high_bound[part] = RGBLIGHT_EFFECT_KNIGHT_LENGTH;
        increment[part] = RGBLIGHT_EFFECT_KNIGHT_INCREMENT;
    }

    rgblight_sethsv_noeeprom_old(ranges, 0, 0, 0, 0, part);
    // Determine which LEDs should be lit up

    if (LIGHT_TAPE_WHOLE == gbinfo.rl_shape)
    {
        for (uint16_t i = 0; i < section_leds; i++)
        {
            if (i >= low_bound[part] && i <= high_bound[part])
            {
                if (i < BOARD_LIGHT_TAPE_SIDE_DIV_NUM)
                {
                    uint16_t id = start_pos + BOARD_LIGHT_TAPE_BACK_LED_NUM + BOARD_LIGHT_TAPE_SIDE_DIV_NUM - 1 - i;
                    sethsv(config.hsv.h, config.hsv.s, config.hsv.v, 0, &rgb_buff[id]);
                }
                else if (i < BOARD_LIGHT_TAPE_BACK_COL_NUM + BOARD_LIGHT_TAPE_SIDE_DIV_NUM)
                {
                    rgb_apply_at_index(i - BOARD_LIGHT_TAPE_SIDE_DIV_NUM, IGNORE, AUTOMA, config.hsv.h, config.hsv.s, config.hsv.v, 0, 0, true);
                }
                else
                {
                    uint16_t id = start_pos + BOARD_LIGHT_TAPE_BACK_LED_NUM + BOARD_LIGHT_TAPE_SIDE_DIV_NUM + section_leds - 1 - i;
                    sethsv(config.hsv.h, config.hsv.s, config.hsv.v, 0, &rgb_buff[id]);
                }
            }
        }

    }
    else
    {
        for (uint16_t i = 0; i < section_leds; i++)
        {
            if (i >= low_bound[part] && i <= high_bound[part])
            {
                if (i < BOARD_LIGHT_TAPE_BACK_COL_NUM && part == LIGHT_TAPE_PART_BACK)
                {
                    rgb_apply_at_index(i, IGNORE, AUTOMA, config.hsv.h, config.hsv.s, config.hsv.v, 0, 0, false);
                }
                else
                {
                    rgb_apply_at_dir(i % section_leds + start_pos, config.hsv.h, config.hsv.s, config.hsv.v, 0, 0, false);
                }
            }
        }
    }
    rgblight_mark_refresh();

    // Move from low_bound to high_bound changing the direction we increment each
    // time a boundary is hit.
    low_bound[part] += increment[part];
    high_bound[part] += increment[part];

    if (high_bound[part] <= 0 || low_bound[part] >= section_leds - 1)
    {
        increment[part] = -increment[part];
    }
}

/****************** Mode 6: christmas **************************/
static void rgblight_effect_christmas(animation_status_t *anim, rgblight_ranges_t *ranges, uint8_t part)
{
    static int8_t increment[LIGHT_TAPE_PART_MAX] = {1, 1};
    const uint8_t max_pos   = 32;
    const uint8_t hue_green = 85;

    uint32_t xa;
    uint8_t  hue, val;
    uint8_t  i;

    // The effect works by animating anim->pos from 0 to 32 and back to 0.
    // The pos is used in a cubic bezier formula to ease-in-out between red and green, leaving the interpolated colors visible as short as possible.
    xa  = CUBED((uint32_t)anim[part].pos);
    hue = ((uint32_t)hue_green) * xa / (xa + CUBED((uint32_t)(max_pos - anim[part].pos)));
    // Additionally, these interpolated colors get shown with a slightly darker value, to make them less prominent than the main colors.
    val = 255 - (3 * (hue < hue_green / 2 ? hue : hue_green - hue) / 2);

    for (i = 0; i < ranges[part].effect_num_leds; i++) 
    {
        uint8_t local_hue = (i / RGBLIGHT_EFFECT_CHRISTMAS_STEP) % 2 ? hue : hue_green - hue;
        sethsv(local_hue, gbinfo.rl[part].ms[gbinfo.rl[part].md].hsv.s, val / 2, 0, (rgb_t *)&rgb_buff[i + ranges[part].effect_start_pos]);
    }
    rgblight_mark_refresh();

    if (anim[part].pos == 0) 
    {
        increment[part] = 1;
    }
    else if (anim[part].pos == max_pos) 
    {
        increment[part] = -1;
    }
    anim[part].pos += increment[part];
}

/****************** Mode 7: rgbtest **************************/
static void rgblight_effect_rgbtest(animation_status_t *anim, rgblight_ranges_t *ranges, uint8_t part)
{
    const light_tape_config_t config = gbinfo.rl[part].ms[gbinfo.rl[part].md];

    if (anim[part].restart)
    {
        anim[part].pos = 0;
    }

    uint8_t hue = anim[part].pos * 85;

    rgblight_sethsv_noeeprom_old(ranges, hue, RGBLIGHT_LIMIT_SAT, RGBLIGHT_LIMIT_VAL, 0, part);

    anim[part].pos = (anim[part].pos + 1) % 3;
}

/****************** Mode 8: alternating **************************/
static void rgblight_effect_alternating(animation_status_t *anim, rgblight_ranges_t *ranges, uint8_t part)
{
    const uint16_t section_leds = ranges[part].clipping_col;
    const uint16_t start_pos = ranges[part].effect_start_pos;
    const uint16_t stop_pos = ranges[part].effect_end_pos;
    const light_tape_config_t config = gbinfo.rl[part].ms[gbinfo.rl[part].md];
    const uint16_t effect_num_leds = section_leds / 2;

    rgblight_sethsv_noeeprom_old(ranges, 0, 0, 0, 0, part);

    if (LIGHT_TAPE_WHOLE == gbinfo.rl_shape)
    {

        for (uint16_t i = 0; i < section_leds; i++)
        {
            if ((i < effect_num_leds) && anim[part].pos)
            {
                if (i < BOARD_LIGHT_TAPE_BACK_COL_NUM / 2)
                {
                    rgb_apply_at_index(i, IGNORE, AUTOMA, config.hsv.h, config.hsv.s, config.hsv.v, 0, 0, false);
                }
                else
                {
                    uint16_t id = i + start_pos + BOARD_LIGHT_TAPE_BACK_LED_NUM - (BOARD_LIGHT_TAPE_BACK_COL_NUM / 2) + BOARD_LIGHT_TAPE_SIDE_DIV_NUM;
                    rgb_apply_at_dir(id, config.hsv.h, config.hsv.s, config.hsv.v, 0, 0, false);
                }
            }
            else if ((i >= effect_num_leds) && !anim[part].pos)
            {
                uint8_t id = i - BOARD_LIGHT_TAPE_SIDE_DIV_NUM;
                if (id < BOARD_LIGHT_TAPE_BACK_COL_NUM)
                {
                    rgb_apply_at_index(id, IGNORE, AUTOMA, config.hsv.h, config.hsv.s, config.hsv.v, 0, 0, false);
                }
                else
                {
                    uint16_t id = i + start_pos + ranges[part].clipping_num_leds - section_leds - BOARD_LIGHT_TAPE_SIDE_DIV_NUM;
                    rgb_apply_at_dir(id, config.hsv.h, config.hsv.s, config.hsv.v, 0, 0, false);
                }
            }
        }
    }
    else
    {
        for (uint16_t i = 0; i < section_leds; i++)
        {
            rgb_t *ledp = rgb_buff + i + start_pos;
            if ((i < effect_num_leds) && anim[part].pos)
            {
                if (i < BOARD_LIGHT_TAPE_BACK_COL_NUM && part == LIGHT_TAPE_PART_BACK)
                {
                    rgb_apply_at_index(i, IGNORE, AUTOMA, config.hsv.h, config.hsv.s, config.hsv.v, 0, 0, false);
                }
                else
                {
                    sethsv(config.hsv.h, config.hsv.s, config.hsv.v, 0, ledp);
                }
            }
            else if ((i >= effect_num_leds) && !anim[part].pos)
            {
                if (i < BOARD_LIGHT_TAPE_BACK_COL_NUM && part == LIGHT_TAPE_PART_BACK)
                {
                    rgb_apply_at_index(i, IGNORE, AUTOMA, config.hsv.h, config.hsv.s, config.hsv.v, 0, 0, false);
                }
                else
                {
                    sethsv(config.hsv.h, config.hsv.s, config.hsv.v, 0, ledp);
                }
            }
        }
    }

    rgblight_mark_refresh();
    anim[part].pos = (anim[part].pos + 1) % 2;
}

/****************** Mode 9: alternating **************************/
static void rgblight_effect_random(animation_status_t *anim, rgblight_ranges_t *ranges, uint8_t part)
{
    const light_tape_config_t config = gbinfo.rl[part].ms[gbinfo.rl[part].md];

    for (int i = 0; i < ranges[part].effect_num_leds; i++)
    {
        srand(xTaskGetTickCount());
        rgb_t *ledp = rgb_buff + i + ranges[part].effect_start_pos;
        uint8_t hue = rand() % 255;
        uint8_t sat = rand() % 255;
        DBG_PRINTF("hue = %d\t sat = %d\n", hue, sat);

        sethsv(hue, sat, config.hsv.v, 0, ledp);
    }
    rgblight_mark_refresh();
}

/****************** Mode 10: -------- **************************/
static void rgblight_effect_rotate(animation_status_t *anim, rgblight_ranges_t *ranges, uint8_t part)
{
    static uint16_t offset[LIGHT_TAPE_PART_MAX] = {0};
    const uint16_t section_leds = ranges[part].clipping_col;
    const uint16_t start_pos = ranges[part].effect_start_pos;
    const light_tape_config_t config = gbinfo.rl[part].ms[gbinfo.rl[part].md];
    const uint8_t half = section_leds / 2;

    rgblight_sethsv_noeeprom_old(ranges, (config.hsv.h + 128) & 0xFF, config.hsv.s, config.hsv.v, 0, part);

    for (uint16_t i = 0; i < section_leds; i++)
    {
        uint16_t real_index = (i + offset[part]) % section_leds;

        if (i < BOARD_LIGHT_TAPE_BACK_COL_NUM && part == LIGHT_TAPE_PART_BACK)
        {
            if (real_index < half)
            {
                rgb_apply_at_index(i, IGNORE, AUTOMA, config.hsv.h, config.hsv.s, config.hsv.v, 0, 0, false);
            }
        }
        else if (i >= BOARD_LIGHT_TAPE_BACK_COL_NUM && part == LIGHT_TAPE_PART_BACK)
        {
            if (real_index < half)
            {
                rgb_apply_at_dir(start_pos + i + ranges[part].clipping_num_leds - section_leds, config.hsv.h, config.hsv.s, config.hsv.v, 0, 0, false);
            }
        }
        else
        {
            if (real_index < half)
            {
                rgb_apply_at_dir(start_pos + i, config.hsv.h, config.hsv.s, config.hsv.v, 0, 0, false);
            }
        }
    }

    rgblight_mark_refresh();

    // 更新偏移量实现旋转
    offset[part] = (offset[part] + 1) % section_leds;
}


static void rgb_state_set(void)
{
    if (play_start == 0)//播放第一个节拍
    {
        light_tape_set_all(p_rgb_state[rgb_num].r, p_rgb_state[rgb_num].g, p_rgb_state[rgb_num].b);
        play_start = 1;
        DBG_PRINTF("play start=%d,%d\n",rgb_num,p_rgb_state[rgb_num].delay);
        return;
    }
    
    rgb_count++;
    if (rgb_count >= p_rgb_state[rgb_num].delay)//播放下一节拍
    {
        rgb_count = 0;
        rgb_num++;
        DBG_PRINTF("play=%d,%d,%d\n",rgb_num,p_rgb_state[rgb_num].delay,rgb_state_num);
        if (rgb_num < rgb_state_num)
        {
            light_tape_set_all(p_rgb_state[rgb_num].r, p_rgb_state[rgb_num].g, p_rgb_state[rgb_num].b);
        }
        else
        {
            DBG_PRINTF("play end\n");
            light_tape_status = 0;
        }

    }
    DBG_PRINTF("%d\n",rgb_count);
}

void light_tape_play(const light_tape_state_t *p, uint8_t size)
{
    p_rgb_state = p;
    rgb_state_num = size;
    rgb_count = 0;
    rgb_num = 0;
    play_start = 0;
    DBG_PRINTF("rgb_state_config:%d\n", size);
    light_tape_status = 1;
}

void light_tape_init_ranges(void)
{
    if (LIGHT_TAPE_SPLIT == gbinfo.rl_shape)
    {
        rgblight_ranges[LIGHT_TAPE_PART_BACK].effect_start_pos = 0;
        rgblight_ranges[LIGHT_TAPE_PART_BACK].effect_end_pos = BOARD_LIGHT_TAPE_BACK_LED_NUM;
        rgblight_ranges[LIGHT_TAPE_PART_BACK].effect_num_leds = BOARD_LIGHT_TAPE_BACK_LED_NUM;
        rgblight_ranges[LIGHT_TAPE_PART_BACK].clipping_start_pos = 0;
        rgblight_ranges[LIGHT_TAPE_PART_BACK].clipping_num_leds = BOARD_LIGHT_TAPE_BACK_LED_NUM;
        rgblight_ranges[LIGHT_TAPE_PART_BACK].clipping_row = BOARD_LIGHT_TAPE_BACK_ROW_NUM;
        rgblight_ranges[LIGHT_TAPE_PART_BACK].clipping_col = BOARD_LIGHT_TAPE_BACK_COL_NUM;

        rgblight_ranges[LIGHT_TAPE_PART_SIDE].effect_start_pos = BOARD_LIGHT_TAPE_BACK_LED_NUM;
        rgblight_ranges[LIGHT_TAPE_PART_SIDE].effect_end_pos = BOARD_LIGHT_TAPE_LED_NUM;
        rgblight_ranges[LIGHT_TAPE_PART_SIDE].effect_num_leds = BOARD_LIGHT_TAPE_SIDE_LED_NUM;
        rgblight_ranges[LIGHT_TAPE_PART_SIDE].clipping_start_pos = BOARD_LIGHT_TAPE_BACK_LED_NUM;
        rgblight_ranges[LIGHT_TAPE_PART_SIDE].clipping_num_leds = BOARD_LIGHT_TAPE_SIDE_LED_NUM;
        rgblight_ranges[LIGHT_TAPE_PART_SIDE].clipping_row = 1;
        rgblight_ranges[LIGHT_TAPE_PART_SIDE].clipping_col = BOARD_LIGHT_TAPE_SIDE_LED_NUM;
    }
    else if (LIGHT_TAPE_WHOLE == gbinfo.rl_shape)
    {
        rgblight_ranges[LIGHT_TAPE_PART_BACK].effect_start_pos = 0;
        rgblight_ranges[LIGHT_TAPE_PART_BACK].effect_end_pos = BOARD_LIGHT_TAPE_LED_NUM;
        rgblight_ranges[LIGHT_TAPE_PART_BACK].effect_num_leds = BOARD_LIGHT_TAPE_LED_NUM;
        rgblight_ranges[LIGHT_TAPE_PART_BACK].clipping_start_pos = 0;
        rgblight_ranges[LIGHT_TAPE_PART_BACK].clipping_num_leds = BOARD_LIGHT_TAPE_LED_NUM;
        rgblight_ranges[LIGHT_TAPE_PART_BACK].clipping_row = BOARD_LIGHT_TAPE_BACK_ROW_NUM;
        rgblight_ranges[LIGHT_TAPE_PART_BACK].clipping_col = BOARD_LIGHT_TAPE_BACK_COL_NUM + BOARD_LIGHT_TAPE_SIDE_LED_NUM;
    }

    memset(animation_status, 0, sizeof(animation_status));
    animation_status[LIGHT_TAPE_PART_BACK].restart = true;
    animation_status[LIGHT_TAPE_PART_BACK].pos = 1;

    animation_status[LIGHT_TAPE_PART_SIDE].restart = true;
    animation_status[LIGHT_TAPE_PART_SIDE].pos = 1;

    rgb_axis_count = 0;
}

void light_tape_sync(void)
{
    animation_status[LIGHT_TAPE_PART_BACK].restart = true;
    animation_status[LIGHT_TAPE_PART_SIDE].restart = true;
}

void light_tape_handler(bool split, light_tape_part_e part)
{
    effect_func_t effect_func = NULL;
    uint16_t interval_time = 100;
    static light_tape_t last_rl[LIGHT_TAPE_PART_MAX] = {0};
    static uint8_t last_md[LIGHT_TAPE_PART_MAX] = {0};

    if (1 == light_tape_status) 
    {
        rgb_state_set();
        animation_status[LIGHT_TAPE_PART_BACK].restart = true;
        animation_status[LIGHT_TAPE_PART_SIDE].restart = true;
        return;
    }

    {
        if (!gbinfo.rl[part].state)
        {
            effect_func   = rgblight_effect_close;
            goto LB_DEAL;
        }

        switch (mode_id[gbinfo.rl[part].md]) 
        {
            case RGBLIGHT_EFFECT_STATIC:
                interval_time = 500;
                effect_func = rgblight_effect_static;
                break;
            case RGBLIGHT_EFFECT_BREATHING:
                interval_time = gbinfo.rl[part].ms[gbinfo.rl[part].md].speed * 10;
                effect_func = rgblight_effect_breathing;
                break;
            case RGBLIGHT_EFFECT_RAINBOW_MOOD:
                interval_time = gbinfo.rl[part].ms[gbinfo.rl[part].md].speed * 10;
                effect_func = rgblight_effect_rainbow_mood;
                break;
            case RGBLIGHT_EFFECT_RAINBOW_SWIRL:
                interval_time = gbinfo.rl[part].ms[gbinfo.rl[part].md].speed * 10;
                effect_func = rgblight_effect_rainbow_swirl;
                break;
            case RGBLIGHT_EFFECT_SNAKE:
                interval_time = gbinfo.rl[part].ms[gbinfo.rl[part].md].speed * 10;
                effect_func = rgblight_effect_snake;
                break;
            case RGBLIGHT_EFFECT_KNIGHT:
                interval_time = gbinfo.rl[part].ms[gbinfo.rl[part].md].speed * 10;
                effect_func = rgblight_effect_knight;
                break;
            case RGBLIGHT_EFFECT_CHRISTMAS:
                interval_time = gbinfo.rl[part].ms[gbinfo.rl[part].md].speed * 10;
                effect_func = (effect_func_t)rgblight_effect_christmas;
                break;
            case RGBLIGHT_EFFECT_RGB_TEST:
                interval_time = 1024;
                effect_func = (effect_func_t)rgblight_effect_rgbtest;
                break;
            case RGBLIGHT_EFFECT_ALTERNATING:
                interval_time = 500;
                effect_func = (effect_func_t)rgblight_effect_alternating;
                break;
            case RGBLIGHT_EFFECT_RANDOM:
                interval_time = gbinfo.rl[part].ms[gbinfo.rl[part].md].speed * 100;
                effect_func = (effect_func_t)rgblight_effect_random;
                break;
            case RGBLIGHT_EFFECT_ROTATE:
                interval_time = gbinfo.rl[part].ms[gbinfo.rl[part].md].speed * 10;
                effect_func = (effect_func_t)rgblight_effect_rotate;
                break;
            default:
                effect_func = rgblight_effect_close;
                break;
        }

    LB_DEAL:
        if (animation_status[part].restart) 
        {
            animation_status[part].last_timer = sync_timer_read();
        }

        if (last_md[part] != gbinfo.rl[part].md)
        {
            animation_status[part].restart = true;
            last_md[part] = gbinfo.rl[part].md;
        }

        uint16_t now = sync_timer_read();
        if (TIMER_EXPIRED(now, animation_status[part].last_timer)) 
        {
            animation_status[part].last_timer += interval_time;
            if (split)
            {
                effect_func(&animation_status[0], s_ranges, part);
            }
            else
            {
                effect_func(&animation_status[0], rgblight_ranges, part);
            }
        } 
        else if (memcmp(&last_rl[part], &gbinfo.rl[part], sizeof(light_tape_t))) 
        {
            memcpy(&last_rl[part], &gbinfo.rl[part], sizeof(light_tape_t));
            if (split)
            {
                effect_func(&animation_status[0], s_ranges, part);
            }
            else
            {
                effect_func(&animation_status[0], rgblight_ranges, part);
            }
        }

        animation_status[part].restart    = false;

    }

}

void light_tape_init(void)
{
    light_tape_init_ranges();
    drv_ws2812_pin_init();
    light_tape_power_on();
    drv_ws2812_init();

}

void light_tape_power_on(void)
{
    drv_ws2812_power(true);
}

void light_tape_power_off(void)
{
    memset(rgb_buff, 0x0, sizeof(rgb_buff));
    drv_ws2812_set_leds(rgb_buff, BOARD_LIGHT_TAPE_LED_NUM);
    drv_ws2812_power(false);
}

void light_tape_back_effect_switch_next(void)
{
    gbinfo.rl[LIGHT_TAPE_PART_BACK].md++;
    if (gbinfo.rl[LIGHT_TAPE_PART_BACK].md >= RGBLIGHT_EFFECT_MAX)
    {
        gbinfo.rl[LIGHT_TAPE_PART_BACK].md = RGBLIGHT_EFFECT_STATIC;
    }
}

void light_tape_back_effect_switch_prev(void)
{
    if (gbinfo.rl[LIGHT_TAPE_PART_BACK].md <= RGBLIGHT_EFFECT_STATIC)
    {
        gbinfo.rl[LIGHT_TAPE_PART_BACK].md = RGBLIGHT_EFFECT_MAX;
    }
    gbinfo.rl[LIGHT_TAPE_PART_BACK].md--;
}

void light_tape_side_effect_switch_next(void)
{
    gbinfo.rl[LIGHT_TAPE_PART_SIDE].md++;
    if (gbinfo.rl[LIGHT_TAPE_PART_SIDE].md >= RGBLIGHT_EFFECT_MAX)
    {
        gbinfo.rl[LIGHT_TAPE_PART_SIDE].md = RGBLIGHT_EFFECT_STATIC;
    }
}

void light_tape_side_effect_switch_prev(void)
{
    if (gbinfo.rl[LIGHT_TAPE_PART_SIDE].md <= RGBLIGHT_EFFECT_STATIC)
    {
        gbinfo.rl[LIGHT_TAPE_PART_SIDE].md = RGBLIGHT_EFFECT_MAX;
    }
    gbinfo.rl[LIGHT_TAPE_PART_SIDE].md--;
}

void light_tape_set_all(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t r_scaled = (r * 8) >> 5;
    uint8_t g_scaled = (g * 8) >> 5;
    uint8_t b_scaled = (b * 8) >> 5;
    for (uint8_t i = 0; i < BOARD_LIGHT_TAPE_LED_NUM; i++)
    {
        rgb_buff[i].r = r_scaled;
        rgb_buff[i].g = g_scaled;
        rgb_buff[i].b = b_scaled;
    }

    drv_ws2812_set_leds(rgb_buff, BOARD_LIGHT_TAPE_LED_NUM);
}

void light_tape_startup_effect(uint8_t step, uint16_t count, uint8_t r, uint8_t g, uint8_t b)
{
    const uint16_t side_base = BOARD_LIGHT_TAPE_BACK_LED_NUM;
    const uint8_t mid = BOARD_LIGHT_TAPE_SIDE_DIV_NUM;
    const uint16_t half_count = (count + 1) / 2;

    if (0 == step)
    {
        memset(rgb_buff, 0, sizeof(rgb_buff));
    }

    if (step < half_count)
    {
        if (step < mid)
        {
            int16_t left_idx = (int16_t)mid - 1 - (int16_t)step;
            int16_t right_idx = (int16_t)mid + (int16_t)step;

            if (left_idx >= 0 && (side_base + left_idx) < (int16_t)BOARD_LIGHT_TAPE_LED_NUM)
            {
                rgb_buff[side_base + left_idx].r = r;
                rgb_buff[side_base + left_idx].g = g;
                rgb_buff[side_base + left_idx].b = b;
            }

            if (right_idx < (int16_t)BOARD_LIGHT_TAPE_SIDE_LED_NUM && (side_base + right_idx) < (int16_t)BOARD_LIGHT_TAPE_LED_NUM)
            {
                rgb_buff[side_base + right_idx].r = r;
                rgb_buff[side_base + right_idx].g = g;
                rgb_buff[side_base + right_idx].b = b;
            }
        }
        else
        {
            hsv_t hsv = {0, 0, RGBLIGHT_EFFECT_WAKE_MAX};
            rgb_apply_at_index(step - mid, IGNORE, AUTOMA, hsv.h, hsv.s, hsv.v, RGBLIGHT_EFFECT_WAKE_MAX, false, false);
            rgb_apply_at_index(step - mid, IGNORE, AUTOMA, hsv.h, hsv.s, hsv.v, RGBLIGHT_EFFECT_WAKE_MAX, true, false);
        }

    }
    else if (step < count)
    {
        uint16_t fade_step = step - half_count;
        
        if (fade_step < (half_count - mid))
        {
            int16_t back_offset = (int16_t)(half_count - mid) - 1 - (int16_t)fade_step;
            if (back_offset >= 0)
            {
                hsv_t hsv = {0, 0, 0};
                rgb_apply_at_index(back_offset, IGNORE, AUTOMA, hsv.h, hsv.s, hsv.v, RGBLIGHT_EFFECT_WAKE_MAX, false, false);
                rgb_apply_at_index(back_offset, IGNORE, AUTOMA, hsv.h, hsv.s, hsv.v, RGBLIGHT_EFFECT_WAKE_MAX, true, false);
            }
        }
        else
        {
            uint16_t side_fade_step = fade_step - (half_count - mid);
            int16_t left_idx = side_fade_step;
            int16_t right_idx = (int16_t)BOARD_LIGHT_TAPE_SIDE_LED_NUM - 1 - (int16_t)side_fade_step;

            if (left_idx >= 0 && left_idx < (int16_t)BOARD_LIGHT_TAPE_SIDE_LED_NUM && (side_base + left_idx) < (int16_t)BOARD_LIGHT_TAPE_LED_NUM)
            {
                rgb_buff[side_base + left_idx].r = 0;
                rgb_buff[side_base + left_idx].g = 0;
                rgb_buff[side_base + left_idx].b = 0;
            }

            if (right_idx >= 0 && right_idx < (int16_t)BOARD_LIGHT_TAPE_SIDE_LED_NUM && (side_base + right_idx) < (int16_t)BOARD_LIGHT_TAPE_LED_NUM)
            {
                rgb_buff[side_base + right_idx].r = 0;
                rgb_buff[side_base + right_idx].g = 0;
                rgb_buff[side_base + right_idx].b = 0;
            }
        }
    }

    drv_ws2812_set_leds(rgb_buff, BOARD_LIGHT_TAPE_LED_NUM);
}

void rgb_light_tape_lamp_set_all(uint8_t (*rgbi)[4])
{
    memset(lamp_buff, 0, sizeof(lamp_buff));

    for (uint8_t i = 0; i < BOARD_LIGHT_TAPE_LED_NUM; i++)
    {
        rgb_t *ledp = lamp_buff + i;
        ledp->r = (*rgbi)[0] * (*rgbi)[3] / 0xFF;
        ledp->g = (*rgbi)[1] * (*rgbi)[3] * 0.4 / 0xFF;
        ledp->b = (*rgbi)[2] * (*rgbi)[3] * 0.4 / 0xFF;
        rgbi++;
    }

    drv_ws2812_set_leds(lamp_buff, BOARD_LIGHT_TAPE_LED_NUM);
}

void rgb_light_tape_sync_set_rear(uint8_t (*rgbi)[3])
{
    for (uint8_t i = 0; i < BOARD_LIGHT_TAPE_BACK_LED_NUM; i++)
    {
        rgb_t *ledp = rgb_buff + i;
        ledp->r = (*rgbi)[0] * RGBLIGHT_LIMIT_LOAD;
        ledp->g = (*rgbi)[1] * RGBLIGHT_LIMIT_LOAD;
        ledp->b = (*rgbi)[2] * RGBLIGHT_LIMIT_LOAD;
        rgbi++;
    }
    rgblight_mark_refresh();
}
void rgb_light_tape_sync_set_side(uint8_t (*rgbi)[3])
{
    for (uint8_t i = BOARD_LIGHT_TAPE_BACK_LED_NUM; i < BOARD_LIGHT_TAPE_LED_NUM; i++)
    {
        rgb_t *ledp = rgb_buff + i;
        ledp->r = (*rgbi)[0] * RGBLIGHT_LIMIT_LOAD;
        ledp->g = (*rgbi)[1] * RGBLIGHT_LIMIT_LOAD;
        ledp->b = (*rgbi)[2] * RGBLIGHT_LIMIT_LOAD;
        rgbi++;
    }
    rgblight_mark_refresh();
}
