/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef _RGB_LED_ACTION_H_
#define _RGB_LED_ACTION_H_

#include <stdint.h>
#include <stdbool.h>


#define KPHT_1000MS (1000)
#define KPHT_3000MS (3000)

#define KPHT_MS_TO_COUNT(ms)   ((uint32_t)((ms) << 3))   // count = ms / 0.125


typedef enum 
{
    STATE_STARTUP,
    STATE_NORMAL,
    STATE_LAMP,
    STATE_PROMPT,
    STATE_FACTORY,
    STATE_OFF,
    STATE_UNINIT,
    STATE_SYNC,
    STATE_MAX,
} rgb_sm_state_e;
typedef enum
{
    PROMPT_PC_SLEEP                 = 0x00,
    PROMPT_KB_SLEEP                 = 0x10,
    PROMPT_KB_RESET                 = 0x11,
    PROMPT_KB_WARNING_WATER         = 0x12,
    PROMPT_KB_WARNING_USB_VOLT      = 0x13,
    PROMPT_DFU_FLASH_START          = 0x20,
    PROMPT_DFU_FLASH_PROGRESS       = 0x21,
    PROMPT_DFU_FLASH_COMPLETE       = 0x22,
    PROMPT_DFU_SLAVE_FLASH_DOWNLOAD = 0x30,
    PROMPT_DFU_SLAVE_FLASH_COMPLETE = 0x31,
    PROMPT_DFU_SLAVE_FLASH_ABORT    = 0x32,
    PROMPT_DFU_SLAVE_CHECKOUT       = 0x33,
    PROMPT_DFU_SLAVE_INVALID        = 0x34,
    PROMPT_DFU_SLAVE_DISCONN        = 0x35,
    PROMPT_DFU_SLAVE_ABORT          = 0x36,
    PROMPT_NETBAR_MODE_ENABLE       = 0x40,
    PROMPT_MAX,
} prompt_type_e;

typedef enum
{
    FACTORY_RGB_NONE,
    FACTORY_RGB_ENTER,
    FACTORY_RGB_UPDATE,
    FACTORY_RGB_EXIT,
    FACTORY_RGB_MAX,
} factory_rgb_e;



enum 
{
    LED_BRIGHTNESS_MIN = 0,
    LED_BRIGHTNESS_MID = 31,
    LED_BRIGHTNESS_MAX = 63,
};

typedef enum
{
    LED_ACTION_NONE = 0, /* 0 */
    LED_ACTION_BRIGHT,   /* 1 */
    LED_ACTION_GRADUAL,  /* 2 */
    LED_ACTION_BREATH,   /* 3 */
    LED_ACTION_WAVE,     /* 4 */
    LED_ACTION_RIPPLE,   /* 5 */
    LED_ACTION_REACTION, /* 6 */
    LED_ACTION_RANDOM,   /* 7 */
    LED_ACTION_RAY,      /* 8 */
    LED_ACTION_GUSH,
    LED_ACTION_RAIN,
    LED_ACTION_VALENTINE,
    LED_ACTION_SYS_MAX,
} action_mode_t;

enum
{
    LED_CYC_DISABLE = 0,
    LED_CYC_ENABLE  = 1,
};

enum
{
    LED_BRI_L2H  = 0,
    LED_BRI_H2L  = 1,
};

enum
{
    LED_STA_NONE   = 0,
    LED_STA_START  = 1,
    LED_STA_L2H    = 2,
    LED_STA_H2L    = 3,
    LED_STA_PAUSE  = 4,
    LED_STA_STOP   = 5,
};

typedef struct _la_s {
    uint32_t id    :6;      /* id */
    uint32_t at    :4;      /* LED Action Type */
    uint32_t cy    :1;      /* Cycle flag */
    uint32_t rr    :1;      /* Random */
    uint32_t pr    :1;      /* Press Random */
    uint32_t bd    :1;      /* Brightness Directory */
    uint32_t hl    :6;      /* Highest Level */
    uint32_t ll    :6;      /* Lowest Level */
    uint32_t ls    :6;      /* Level Step */
    uint32_t tpt   :10;     /* Total Period Times */
    uint32_t hzc   :10;     /* HZ Counter hzc=(1000 / HZ) */
    uint32_t hdy   :16;     /* Highest Delay */
    uint32_t pdy   :16;     /* Period Delay */
} __attribute__ ((aligned (4))) led_action_t;


typedef struct _kts_s {
    uint32_t kps   :3;      /* Key Press State */
    uint32_t kcs   :2;      /* Key Click State */
    uint32_t tlat  :4;      /* Key Temp Action Type */
    uint32_t ltst  :3;      /* Led Temp State */
    uint32_t ltl   :6;      /* Led Temp Level */
    uint32_t lltl  :6;      /* Led Last Temp Level */
    uint32_t ltts  :8;      /* Led Temp Total Steps */
    /* 4 Byte */
    uint32_t lll   :6;      /* Lowest Level */
    uint32_t lls   :6;      /* Level Step */
    uint32_t ltpt  :10;     /* Total Period Times */
    uint32_t lhzc  :10;     /* HZ Counter hzc=(1000 / HZ) */
    /* 4 Byte */
    uint32_t lhl   :6;      /* Highest Level */
    uint32_t lttpt :10;     /* Led Temp Total Period Times */
    uint32_t ltdy  :16;     /* Led Temp Delay */
    /* 4 Byte */
    uint32_t lhdy  :16;     /* Led Highest Delay */
    uint32_t lpdy  :16;     /* Led Period Delay */
    uint32_t kpht  :16;     /* Key Press Holding Time */
    /* 4 Byte */
    uint8_t  ltkrr;
    uint8_t  ltkrg;
    uint8_t  ltkrb;
    uint8_t  ext1;
    uint8_t  key_debounce;  /* 按键消抖计数 */
} __attribute__ ((aligned (4))) key_tmp_sta_t;


typedef void (*lah_init)(uint8_t ki, led_action_t *pla);
typedef void (*lah_proc)(uint8_t ki, void *pd1, void *pd2, void *pd3);
typedef void (*lah_keyp)(uint8_t ki, led_action_t *pla);

typedef struct _ka_handle_s {
    lah_init init;       /* init function handle */
    lah_proc proc;       /* process function handle */
    lah_keyp keyp;       /* key press function handle */
} __attribute__ ((aligned (4))) led_action_handle_t;


void rgb_led_init(void);
void rgb_led_uninit(void);
void rgb_led_task_start(void);
void rgb_led_task_stop(void);
void rgb_led_set_all(void);
void rgb_led_set_one(uint8_t li);
void rgb_led_key_evt_update(uint8_t ki, uint8_t kps);
bool rgb_led_allow_open(void);
void rgb_led_toggle_cfg_complete_prompt(uint8_t cfg_id);
void rgb_led_set_prompt(prompt_type_e type);
void rgb_led_set_normal(void);
void rgb_led_set_factory(factory_rgb_e type);
void rgb_led_get_prompt(rgb_sm_state_e *state, prompt_type_e *type);
void rgb_led_set_uninit(void);
#endif