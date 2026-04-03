/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef _RGB_LIGHT_TAPE_H_
#define _RGB_LIGHT_TAPE_H_

#include <stdbool.h>
#include <stdint.h>

#define CUBED(x)                              ((x) * (x) * (x))

#define   M_E                                 2.7182818284590452354   /* e */
#define   M_LOG2E                             1.4426950408889634074   /* log_2 e */
#define   M_LOG10                             0.43429448190325182765  /* log_10 e */
#define   M_LN2                               0.69314718055994530942  /* log_e 2 */
#define   M_LN10                              2.30258509299404568402  /* log_e 10 */
#define   M_PI                                3.14159265358979323846  /* pi */
#define   M_PI_2                              1.57079632679489661923  /* pi/2 */
#define   M_PI_4                              0.78539816339744830962  /* pi/4 */
#define   M_1_PI                              0.31830988618379067154  /* 1/pi */
#define   M_2_PI                              0.63661977236758134308  /* 2/pi */
#define   M_2_SQRTPI                          1.12837916709551257390  /* 2/sqrt(pi) */
#define   M_SQRT2                             1.41421356237309504880  /* sqrt(2) */
#define   M_SQRT1_2                           0.70710678118654752440  /* 1/sqrt(2) */

#define   RGBLIGHT_DEFAULT_MODE   RGBLIGHT_EFFECT_RAINBOW_MOOD
#define   RGBLIGHT_DEFAULT_HUE    0
#define   RGBLIGHT_DEFAULT_SAT    0//255
#define   RGBLIGHT_DEFAULT_VAL    150
#define   RGBLIGHT_DEFAULT_SPD    3


#define   RGBLIGHT_LIMIT_HUE                  255           // 0 to 255
#define   RGBLIGHT_LIMIT_SAT                  255           // 0 to 255
#define   RGBLIGHT_LIMIT_VAL                  150           // 0 to 255
#define   RGBLIGHT_LIMIT_LOAD                 (0.34f)
#define   RGBLIGHT_LIMIT_LUM                  150           // 0 to 255

#define   RGBLIGHT_EFFECT_WAKE_MAX           (RGBLIGHT_LIMIT_LUM/2)
#define   RGBLIGHT_EFFECT_BREATHE_CENTER      1.9    // 1 to 2.7
#define   RGBLIGHT_EFFECT_BREATHE_MAX         RGBLIGHT_LIMIT_VAL   
#define   RGBLIGHT_RAINBOW_SWIRL_RANGE        RGBLIGHT_LIMIT_HUE
#define   RGBLIGHT_EFFECT_SNAKE_INCREMENT     1
#define   RGBLIGHT_EFFECT_KNIGHT_LENGTH       7
#define   RGBLIGHT_EFFECT_KNIGHT_INCREMENT    1
#define   RGBLIGHT_EFFECT_CHRISTMAS_STEP      2
#define   RGBLIGHT_EFFECT_MAX                 11

#define RGB_WHITE_TUNING 1
typedef enum
{
    RGBLIGHT_EFFECT_CLOSE = 0,
    RGBLIGHT_EFFECT_STATIC,
    RGBLIGHT_EFFECT_BREATHING,
    RGBLIGHT_EFFECT_RAINBOW_MOOD,
    RGBLIGHT_EFFECT_RAINBOW_SWIRL,
    RGBLIGHT_EFFECT_SNAKE,
    RGBLIGHT_EFFECT_KNIGHT,
    RGBLIGHT_EFFECT_CHRISTMAS,
    RGBLIGHT_EFFECT_RGB_TEST,
    RGBLIGHT_EFFECT_ALTERNATING,
    RGBLIGHT_EFFECT_RANDOM,
    RGBLIGHT_EFFECT_ROTATE,
    RGBLIGHT_EFFECT_MICROPHONE_DOT,
    RGBLIGHT_EFFECT_GAME,
    RGBLIGHT_EFFECT_MICROPHONE_RAINROW,
    RGBLIGHT_EFFECT_MOSAIC,
    RGBLIGHT_EFFECT_STARS,
    RGBLIGHT_EFFECT_METEOR,
    RGBLIGHT_EFFECT_RAMBLE,
    RGBLIGHT_EFFECT_FLOW,
    RGBLIGHT_EFFECT_PARTICLE,
    // RGBLIGHT_EFFECT_SINE_WAVE,
    // RGBLIGHT_EFFECT_TRIANGLE_WAVE,
    RGBLIGHT_EFFECT_SYS_MAX,

} rgblight_mode;

typedef enum
{
    LIGHT_TAPE_PART_BACK = 0,
    LIGHT_TAPE_PART_SIDE,
    LIGHT_TAPE_PART_MAX,
}light_tape_part_e;

typedef enum : uint16_t
{
    LIGHT_DIR_NONE  = 0,
    LIGHT_DIR_UP    = (1 << 0),
    LIGHT_DIR_DOWN  = (1 << 1),
    LIGHT_DIR_LEFT  = (1 << 2),
    LIGHT_DIR_RIGHT = (1 << 3),
    LIGHT_DIR_IN    = (1 << 4),
    LIGHT_DIR_OUT   = (1 << 5),
    LIGHT_DIR_CW    = (1 << 6),
    LIGHT_DIR_CCW   = (1 << 7),
    LIGHT_DIR_FORWARD  = (1 << 8),
    LIGHT_DIR_REVERSE  = (1 << 9)
} light_dir_t;

typedef enum
{
    LIGHT_TYPE_NONE    = 0,
    LIGHT_TYPE_RANDOM  = (1 << 0),
    LIGHT_TYPE_ALL     = (1 << 1),
} light_type_t;

typedef enum
{
    LIGHT_TAPE_SPLIT = 0,
    LIGHT_TAPE_WHOLE,
}light_tape_shape_e;

typedef struct 
{
    uint8_t h;
    uint8_t s;
    uint8_t v;
} hsv_t;


typedef struct 
{
    hsv_t hsv;
    uint8_t speed;
}light_tape_config_t;


typedef struct 
{
    uint8_t state;
    uint8_t md;
    light_tape_config_t ms[RGBLIGHT_EFFECT_MAX];
}light_tape_t;



typedef struct _rgblight_ranges_t 
{
    uint8_t clipping_col;
    uint8_t clipping_row;
    uint8_t clipping_start_pos;
    uint8_t clipping_num_leds;
    uint8_t effect_start_pos;
    uint8_t effect_end_pos;
    uint8_t effect_num_leds;
} rgblight_ranges_t;

typedef struct _animation_status_t 
{
    uint16_t last_timer;
    bool     restart;
    union 
    {
        uint8_t  pos;
        uint8_t current_hue;
    };
} animation_status_t;

typedef struct 
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint32_t delay; 
}light_tape_state_t;


void light_tape_init(void);
void light_tape_handler(bool split, light_tape_part_e part);
void light_tape_power_on(void);
void light_tape_power_off(void);

void light_tape_back_increase_hue(void);
void light_tape_back_decrease_hue(void);
void light_tape_back_increase_sat(void);
void light_tape_back_decrease_sat(void);
void light_tape_back_increase_val(void);
void light_tape_back_decrease_val(void);
void light_tape_back_increase_val_by_step(uint8_t step);
void light_tape_back_decrease_val_by_step(uint8_t step);
void light_tape_back_increase_speed(void);
void light_tape_back_decrease_speed(void);

void light_tape_side_increase_hue(void);
void light_tape_side_decrease_hue(void);
void light_tape_side_increase_sat(void);
void light_tape_side_decrease_sat(void);
void light_tape_side_increase_val(void);
void light_tape_side_decrease_val(void);
void light_tape_side_increase_val_by_step(uint8_t step);
void light_tape_side_decrease_val_by_step(uint8_t step);
void light_tape_side_increase_speed(void);
void light_tape_side_decrease_speed(void);

void light_tape_set_default(void);

void light_tape_back_effect_switch_next(void);
void light_tape_back_effect_switch_prev(void);

void light_tape_side_effect_switch_next(void);
void light_tape_side_effect_switch_prev(void);

void light_tape_update_sync_timer(uint8_t task_tick);
void light_tape_play(const light_tape_state_t *p, uint8_t size);
void light_tape_set_all(uint8_t r, uint8_t g, uint8_t b);
const rgblight_mode *rgblight_get_mode_list(void);
void rgb_light_tape_lamp_set_all(uint8_t (*rgbi)[4]);
void light_tape_check_para(light_tape_t *rl_array);
void light_tape_init_ranges(void);
void light_tape_startup_effect(uint8_t step, uint16_t count, uint8_t r, uint8_t g, uint8_t b);
void rgb_light_tape_sync_set_rear(uint8_t (*rgbi)[3]);
void rgb_light_tape_sync_set_side(uint8_t (*rgbi)[3]);
void light_tape_sync(void);
void rgblight_flush(void);
#endif
