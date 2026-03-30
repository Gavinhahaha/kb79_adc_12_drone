/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include "rgb_led_ctrl.h"
#include "rgb_led_table.h"

#if BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_AW20XXX
#include "drv_aw20xxx.h"
#elif BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_IS37XX
#include "drv_is37xx.h"
#endif


#include "hpm_gpio_drv.h"

#if BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_AW20XXX
static uint8_t pwm_buf[AW20XXX_SUPPORT_LED_NUM_MAX] = {0};
#elif BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_IS37XX
static uint8_t pwm_buf[IS37XX_PWM_REG_NUM] = {0};
#endif

static bool led_init_state = 0;
static led_update_state update_state = STATE_IDLE;
static uint8_t cur_sta = 0;

static const uint8_t led_pwm_arry[64] = 
{
     0,   2,   2,   3,   4,   5,   6,   7,
     8,  10,  12,  14,  16,  18,  20,  22,
    24,  26,  29,  32,  35,  38,  41,  44,
    47,  50,  53,  55,  61,  65,  69,  73,
    77,  81,  85,  89,  94,  99, 104, 109,
   114, 119, 124, 129, 134, 140, 146, 152,
   158, 164, 170, 176, 182, 188, 195, 202,
   209, 216, 223, 230, 237, 244, 251, 255,
};

void led_set_update_state(led_update_state state)
{
    if (update_state != state)
    {
        update_state = state;
    }
}


led_update_state led_get_update_state(void)
{
    return update_state;
}


void led_set_color_dircet(uint8_t i, uint8_t l, uint8_t r, uint8_t g, uint8_t b)
{
    const is31_led *pled = get_rgb_led_ctrl_ptr(i);
    uint8_t data[3];
    uint16_t start_addr;

    #if BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_AW20XXX

    pwm_buf[pled->r] = (r * led_pwm_arry[l]) / 255;
    pwm_buf[pled->g] = (g * led_pwm_arry[l]) / 255;
    pwm_buf[pled->b] = (b * led_pwm_arry[l]) / 255;

    drv_aw20xxx_update_frame_by_idx(pled->b, (uint8_t []){pwm_buf[pled->b], pwm_buf[pled->g], pwm_buf[pled->r]});

    #elif BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_IS37XX

    if (pled->id == 0)
    {
        pwm_buf[pled->r] = (r * led_pwm_arry[l]) / 255;
        pwm_buf[pled->g] = (g * led_pwm_arry[l]) / 255;
        pwm_buf[pled->b] = (b * led_pwm_arry[l]) / 255;
    }
    else
    {
        pwm_buf[pled->r + IS37XX_PWM_REG_NUM / 2] = (r * led_pwm_arry[l]) / 255;
        pwm_buf[pled->g + IS37XX_PWM_REG_NUM / 2] = (g * led_pwm_arry[l]) / 255;
        pwm_buf[pled->b + IS37XX_PWM_REG_NUM / 2] = (b * led_pwm_arry[l]) / 255;
    }
  
    start_addr = pled->b;
    data[0] = (r * led_pwm_arry[l]) / 255;
    data[1] = (g * led_pwm_arry[l]) / 255;
    data[2] = (b * led_pwm_arry[l]) / 255;

    drv_is37xx_fill_pwm_reg_range(pled->id, data, start_addr);

    #endif
}

void led_set_color(uint8_t i, uint8_t l, uint8_t r, uint8_t g, uint8_t b)
{
    const is31_led *pled = get_rgb_led_ctrl_ptr(i);

    #if BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_AW20XXX

    pwm_buf[pled->r] = (r * led_pwm_arry[l]) / 255;
    pwm_buf[pled->g] = (g * led_pwm_arry[l]) / 255;
    pwm_buf[pled->b] = (b * led_pwm_arry[l]) / 255;

    #elif BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_IS37XX

    if (pled->id == 0)
    {
        pwm_buf[pled->r] = (r * led_pwm_arry[l]) / 255;
        pwm_buf[pled->g] = (g * led_pwm_arry[l]) / 255;
        pwm_buf[pled->b] = (b * led_pwm_arry[l]) / 255;
    }
    else
    {
        pwm_buf[pled->r + IS37XX_PWM_REG_NUM / 2] = (r * led_pwm_arry[l]) / 255;
        pwm_buf[pled->g + IS37XX_PWM_REG_NUM / 2] = (g * led_pwm_arry[l]) / 255;
        pwm_buf[pled->b + IS37XX_PWM_REG_NUM / 2] = (b * led_pwm_arry[l]) / 255;
    }

    #endif

    led_set_update_state(STATE_READY);
}

void led_lamp_set_rgb(uint8_t i, uint8_t l, uint8_t r, uint8_t g, uint8_t b)
{
    const is31_led *pled = get_rgb_led_ctrl_ptr(i);

    #if BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_AW20XXX

    pwm_buf[pled->r] = (r * l) / 255;
    pwm_buf[pled->g] = (g * l) / 255;
    pwm_buf[pled->b] = (b * l) / 255;

    #elif BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_IS37XX

    if (pled->id == 0)
    {
        pwm_buf[pled->r] = (r * l) / 255;
        pwm_buf[pled->g] = (g * l) / 255;
        pwm_buf[pled->b] = (b * l) / 255;
    }
    else
    {
        pwm_buf[pled->r + IS37XX_PWM_REG_NUM / 2] = (r * l) / 255;
        pwm_buf[pled->g + IS37XX_PWM_REG_NUM / 2] = (g * l) / 255;
        pwm_buf[pled->b + IS37XX_PWM_REG_NUM / 2] = (b * l) / 255;
    }

    #endif

    led_set_update_state(STATE_READY);
}

void led_set_all_color(uint8_t l, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < BOARD_LED_NUM; ++i)
    {
       led_set_color(i, l, r, g, b);
    }
}

bool led_update_frame(uint8_t mode)
{
    #if BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_AW20XXX
    drv_aw20xxx_update_frame((uint8_t *)pwm_buf, mode);
    return true;
    #elif  BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_IS37XX
    return drv_is37xx_fill_pwm_reg((uint8_t *)pwm_buf, mode);
    #endif
    
}

static void led_scaling_init(uint8_t r, uint8_t g, uint8_t b)
{
    #if BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_AW20XXX

    const is31_led *pled = NULL;

    for (int i = 0; i < BOARD_LED_NUM; ++i)
    {
        pled = get_rgb_led_ctrl_ptr(i);
        pwm_buf[pled->r] = r;
        pwm_buf[pled->g] = g;
        pwm_buf[pled->b] = b;
    }
    //drv_aw20xxx_update_frame(pwm_buf, QUERY_MODE);

    #elif BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_IS37XX

    #ifdef IS3741
        const is31_led *pled = NULL;

        for (int i = 0; i < BOARD_LED_NUM; ++i)
        {
            pled = get_rgb_led_ctrl_ptr(i);
            pwm_buf[pled->r] = r;
            pwm_buf[pled->g] = g;
            pwm_buf[pled->b] = b;
        }

    #else

    #endif
    drv_is37xx_fill_ctrl_led(pwm_buf);

    #endif
}

void led_set_factory_sys_led(uint8_t status)
{
    if (true == rgb_led_allow_open())
    {
        if (status & KB_CAPSLOCK_MASK)
        {
            CAPSLOCK_LED_SET_ON();
        }
        else
        {
            CAPSLOCK_LED_SET_OFF();
        }
    }
}

uint8_t led_refresh_sys_led(void)
{
    if (cur_sta & KB_CAPSLOCK_MASK)
    {
        CAPSLOCK_LED_SET_ON();
    }
    else
    {
        CAPSLOCK_LED_SET_OFF();
    }

    return cur_sta;
}

uint8_t led_get_sys_led(void)
{
    return cur_sta;
}

void led_set_sys_led_status(uint8_t status)
{
    uint8_t tmp_sta = status & KB_CAPSLOCK_MASK;//(KB_NUMLOCK_MASK | KB_CAPSLOCK_MASK)

    if (cur_sta == tmp_sta)
    {
        return;
    }

    if (tmp_sta & KB_CAPSLOCK_MASK)
    {
        CAPSLOCK_LED_SET_ON();
    }
    else
    {
        CAPSLOCK_LED_SET_OFF();
    }

    cur_sta = tmp_sta;
}

uint8_t led_get_caps_led_status(void)
{
    return cur_sta & KB_CAPSLOCK_MASK;
}


void led_indicator_led_ctrl(uint8_t l, uint8_t r, uint8_t g, uint8_t b) 
{
    led_set_color(BOARD_LED_INDEX_INDICATOR1, l, r, g, b);
    led_set_color(BOARD_LED_INDEX_INDICATOR2, l, r, g, b);
    led_set_color(BOARD_LED_INDEX_INDICATOR3, l, r, g, b);
}

void led_uninit(void)
{
    if (led_init_state)
    {
        led_init_state = 0;
        memset(pwm_buf, 0, sizeof(pwm_buf));

        #if BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_AW20XXX
        drv_aw20xxx_uninit();

        #elif BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_IS37XX

        drv_is37xx_fill_pwm_reg(pwm_buf, QUERY_MODE);
        drv_is37xx_uninit();

        #endif
    }
}

void led_init(void)
{
    if (!led_init_state)
    {
        led_init_state = 1;
        led_scaling_init(90, 105, 105);
        led_set_all_color(31, 0, 0, 0);
    }
}

void led_register(void *complete_handler, void *wait_trans_handler)
{
    #if BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_AW20XXX
    drv_aw20xxx_init((i2c_complete_handler) complete_handler);
    drv_aw20xxx_register_wait_trans_complete((aw20xxx_wait_large_data_trans) wait_trans_handler);
    #elif BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_IS37XX
    drv_is37xx_init((i2c_complete_handler) complete_handler);
    drv_is37xx_register_wait_trans_complete((is37xx_wait_large_data_trans) wait_trans_handler);
    #endif
}