/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#ifndef _DB_H_
#define _DB_H_

//#include "mg_matrix_type.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "rgb_led_table.h"
#include "sk.h"
#include "rgb_light_tape.h"
#define NETBAR_MODE_EN                  (1)

typedef enum
{
    DB_RET_SUCEESS       = 0,
    DB_RET_W_TIMEOUT     = (MG_ERR_BASE_DB + 0),
    DB_RET_R_TIMEOUT     = (MG_ERR_BASE_DB + 1),
    DB_RET_MAX_LEN       = (MG_ERR_BASE_DB + 2),
    DB_RET_ERR_SIZE      = (MG_ERR_BASE_DB + 3),
    DB_RET_ERR_ADDR      = (MG_ERR_BASE_DB + 4),
    DB_RET_MAX
} db_ret;

typedef enum
{
    DB_SYS         = 0,    // 3364
    DB_SK_LA_LM_KC = 1,    // 2248
    DB_KR          = 2,    // 2180
    DB_MACRO       = 3,    // 4084
    DB_CALI        = 4,    // 412
    DB_COMMON      = 5,
    DB_MAX
} db_t;
    
typedef enum {
    UPDATE_NOW = 0,//立刻写入
    UPDATE_LATER,  //延迟写入
    UPDATE_CFG,    //配置切换前,全部写入
    UPDATE_MAX
}db_act_t;

typedef struct 
{
    uint8_t is_vaild;
    uint32_t fw_ver;
    uint32_t fw_size;
    uint32_t check;
}gm_fw_info_t;

typedef void (*cfg_cb_t)(void);

typedef enum
{
    CAPS,
    PROFILE_INDEX,
    BKL_MODE,
    BKL_BRI,
    AMBL_SHAPE,
    AMBL_BACK_MODE,
    AMBL_BACK_BRI,
    AMBL_SIDE_MODE,
    AMBL_SIDE_BRI,
    BKL_BRI_TOG,
    AMBL_BACK_BRI_TOG,
    AMBL_SIDE_BRI_TOG,
    OPEN_CN_WEBSITE,
    OPEN_EN_WEBSITE,
}KB_DPM_PARAM_t;
typedef struct __attribute__((packed))
{
    uint8_t caps_lock_enable;
    uint8_t profile_index;
    uint8_t bkl_mode;
    uint8_t bkl_bri;
    uint8_t ambl_shape;
    uint8_t ambl_back_mode;
    uint8_t ambl_back_bri;
    uint8_t ambl_side_mode;
    uint8_t ambl_side_bri;
}kb_dpm_param_t;

#define VALUE_TO_PERCENT(current, max) \
    (((current) * 100U / (max)) > 100 ? 100 : ((current) * 100U / (max)))
    
#define PERCENT_TO_VALUE(percent, max) \
    (((percent) >= 100) ? (max) : ((percent) * (max) / 100U))

#define MAKE_EVEN(x) ((x) & ~1U)

#define KB_DPM_PARAM_GET() (kb_dpm_param_t){             \
    .caps_lock_enable   = led_get_sys_led(),             \
    .profile_index      = common_info.cfg_info.cfg_idx,              \
    .bkl_mode           = gbinfo.lm.cur_lmm == 0 ? LED_ACTION_MAX - 1 : gbinfo.lm.cur_lmm - 1, \
    .bkl_bri            = MAKE_EVEN(VALUE_TO_PERCENT(sk_la_lm_kc_info.la_table[gbinfo.lm.cur_lmm].hl, LED_BRIGHTNESS_MAX)), \
    .ambl_shape         = gbinfo.rl_shape, \
    .ambl_back_mode     = gbinfo.rl[LIGHT_TAPE_PART_BACK].state == 0 ? RGBLIGHT_EFFECT_MAX - 1 : gbinfo.rl[LIGHT_TAPE_PART_BACK].md - 1, \
    .ambl_back_bri      = VALUE_TO_PERCENT(gbinfo.rl[LIGHT_TAPE_PART_BACK].ms[gbinfo.rl[LIGHT_TAPE_PART_BACK].md].hsv.v, RGBLIGHT_LIMIT_VAL), \
    .ambl_side_mode     = gbinfo.rl[LIGHT_TAPE_PART_SIDE].state == 0 ? RGBLIGHT_EFFECT_MAX - 1 : gbinfo.rl[LIGHT_TAPE_PART_SIDE].md - 1, \
    .ambl_side_bri      = VALUE_TO_PERCENT(gbinfo.rl[LIGHT_TAPE_PART_SIDE].ms[gbinfo.rl[LIGHT_TAPE_PART_SIDE].md].hsv.v, RGBLIGHT_LIMIT_VAL), \
}
typedef struct
{
    uint8_t *src_addr;
    uint32_t flash_addr;
    uint32_t size;
    bool need_update;
    cfg_cb_t cfg_cb;
}db_item_t;

typedef struct _radio_peer_s {
    uint32_t state:3;
    uint32_t pipe:3;
    uint32_t tid:3;
    uint32_t channel:7;
    uint32_t id[2];
} radio_peer_t;

#define BOARD_BT_CHANNEL_MAX            (8)
#define HF_RADIO_PIPE_COUNT             (8)
#define DB_INIT_MAGIC           (0x5AA55AA5)
typedef struct 
{
    uint8_t page_use[8];
}db_page_1;

typedef struct _borad_info {
    //uint32_t crc;
    uint32_t itf;   /* Init Flag */
    uint16_t sms;   /* Scanning millisecond */
    uint8_t  nkro;  /* NKey Rollover */
    uint8_t  mode;  /* USB, Bluetooth and Radio */
    uint8_t  layout; /*windows or mac*/
    uint8_t  startup_effect_en;
    
    uint8_t rl_shape;
    light_tape_t  rl[LIGHT_TAPE_PART_MAX];    /* rgblight */
    uint8_t reserve_0[92];
    struct {
        uint8_t  btid;
        uint8_t  name[24];
        uint8_t  mac[BOARD_BT_CHANNEL_MAX][6];
        uint16_t peer[BOARD_BT_CHANNEL_MAX];
    } bt;
    struct {
        uint8_t pipe;
        radio_peer_t peer[HF_RADIO_PIPE_COUNT];
    } radio;
    struct {
        uint32_t vc;            /* Vendor Code */
        uint32_t pc;            /* Production Code */
        uint32_t inat;          /* Inactive time */
        uint8_t  vn[16];        /* Vendor Name */
        uint8_t  pn[16];        /* Production Name */
        uint16_t fv;            /* Firmware Version */
        uint16_t hv;            /* Hardware Version */
        uint8_t  cd[16];        /* Compile Date */
        uint8_t  ui[47];        /* User Information */
    } pi;                       /* Product Info */
    struct {
        uint8_t max_kcm;
        uint8_t cur_kcm;
        uint8_t max_kc;
    } kc;   /* key code */
    struct {
        uint8_t max_krm;
        uint8_t cur_krm;
        uint8_t max_kr;
    } kr;   /* key color */
    struct {
        uint8_t max_lmm;
        uint8_t cur_lmm;
        uint8_t last_lmm;
        uint8_t tmp_lmm;
        uint8_t max_lm;
    } lm;   /* led matrix */
    
    union {
        uint8_t  u8[32];
        uint16_t u16[16];
        uint32_t u32[8];
    } u;
    struct {
        bool     en;  /*on off*/
        uint16_t time;/*second*/
    }power_save;

    uint32_t player_number;
    uint8_t reserve_1[124];  //reserve之前的总长度 len = offsetof(binfo_t, reserve); 如len长度变长x则reserve[256 - x]反之reserve[256 + x]
    
    key_attr_t key_attr[KEYBOARD_NUMBER];
    key_qty_t kh_qty;

} __attribute__ ((aligned (4))) binfo_t;

typedef struct
{
    uint32_t itf;
    led_action_t la_table[LED_ACTION_DEFAULT_MAX];
    uint8_t reserve_la[104];
    kc_t  kc_table[BOARD_KCM_MAX][BOARD_KEY_NUM];
    uint8_t reserve[512];
    sk_t sk;
} __attribute__ ((aligned (4))) sk_la_lm_kc_info_t;

typedef struct
{ 
    uint8_t cfg_idx;
    uint16_t cfg_ver;
    uint8_t cfg_name[KEYBOARD_CFG_MAX][KEYBOARD_CFG_NAME_SIZE];
    uint8_t rsv_cfg[64];
}cfg_info_t;

typedef struct
{ 
    uint8_t mode_en;
    uint8_t mode_active;
}netbar_info_t;

typedef struct
{
    uint32_t itf;
    cfg_info_t cfg_info;
    netbar_info_t netbar_info;
    uint8_t reserve[128];

} __attribute__ ((aligned (4))) common_info_t;

typedef struct _the_matrix_s
{
    kc_t (*pkc)[BOARD_KEY_NUM];
    key_led_color_t (*pkr)[BOARD_KEY_LED_NUM];
    key_tmp_sta_t *pkts;
    led_action_t *pla;
    const led_action_handle_t *laht; /* ka handle table */
} tmx_t;

extern sk_la_lm_kc_info_t sk_la_lm_kc_info;
extern binfo_t gbinfo;
extern tmx_t gmx;
extern common_info_t common_info;
extern void rgb_led_check_para(key_led_color_info_t *para);

uint32_t db_get_itf(db_t idx);
void db_update(db_t idx, db_act_t act);
void db_register(db_t idx, uint32_t *p_buf, int size);
uint32_t db_read(db_t idx);
void db_reset(void);
void db_reset_kr(void);
void db_reset_la(void);
void db_read_cfg_block(uint8_t cfg_id, db_t db_id, uint16_t offset, uint8_t *buff, uint16_t size);
void db_write_cfg_block(uint8_t cfg_id, db_t db_id, uint8_t *buff, uint16_t size);
void db_read_cfg_cache(uint8_t block_id, uint8_t *buff, uint16_t size);
void db_write_cfg_cache(uint8_t block_id, uint16_t offset, uint8_t *buff, uint16_t size);
void db_erase_cfg_cache(uint8_t block_id);
void db_task_suspend(void);
void db_task_resume(void);
void db_init(void);
void db_send_toggle_cfg_signal(void);
void db_sys_set_cfg(uint8_t cfg_id);
void db_check_sk_la_lm_kc_para(sk_la_lm_kc_info_t *para);
void db_check_binfo(binfo_t * binfo);
void db_check_kr_para(key_led_color_info_t *para);
void db_flash_region_mark_update(db_t region);
void db_set_with_dpm_refresh(void);
void db_save_now(void);

void db_erase_gm_fw_info(uint32_t addr);
void db_write_gm_fw_info(uint32_t addr, gm_fw_info_t *fw_info);
void db_read_gm_fw_info(uint32_t addr, gm_fw_info_t *fw_info);
void db_erase_gm_fw(uint32_t addr);
void db_write_gm_fw(uint32_t offset, uint8_t *buff, uint16_t size);
void db_read_gm_fw(uint32_t offset, uint8_t *buff, uint16_t size);

#endif

