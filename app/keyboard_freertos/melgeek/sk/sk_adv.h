/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#ifndef  _SK_ADV_H_
#define  _SK_ADV_H_

#include "mg_matrix_type.h"

#define ADV_MTP_POINT_NUM    (3)

enum {
    KCM_PREV = 0,
    KCM_NEXT = 1,
};

typedef enum {
    ADV_MATCH_NONE     = 0,
    ADV_MATCH_UNSEND   = 1,
    ADV_MATCH_SEND     = 2,
    ADV_MATCH_MAX      = 3,
}adv_match_t;

enum {
    KFE_DISABLE = 0,
    KFE_ENABLE  = 1,
};

typedef struct 
{
    kc_t kc[ADV_MTP_POINT_NUM];
    uint16_t trg_dist[ADV_MTP_POINT_NUM];
}adv_mtp_t;

typedef struct _adv_s {
    uint8_t  name[16];
    sk_adv_ex_t  adv_ex :3;
    sk_adv_co_t  adv_co :5;
    union
    {
        kc_t kc[6];
        struct
        {
            uint8_t kid[9];//空缺的填0xff
            uint8_t trigger; //触发行程（单位：0.1mm）
            uint8_t raise;
            uint8_t topDead; //死区 (单位：0.1mm）
            uint8_t bottomDead;
            uint8_t isEnable:1;
            uint8_t isContinue:1;
            uint8_t reserve:6;
            uint8_t pressStroke; //灵敏度（单位：0.01mm）
            uint8_t raiseStroke;
        } trg_sence;

        key_mt_t mt;
        key_tgl_t tgl;
        adv_mtp_t mtp;
    }data;
} adv_t;


adv_match_t adv_mt(adv_t *padv, uint8_t ki, uint8_t press);
adv_match_t adv_tgl(adv_t *padv, uint8_t ki, uint8_t press);
adv_match_t adv_mtp(adv_t *padv, uint8_t ki, uint8_t lvl);
uint8_t adv_mtp_evt(adv_t *padv, uint8_t ki);
void adv_mtp_update_point(adv_t adv, uint8_t id);
void adv_mtp_reload(void);
adv_match_t adv_replace_key(adv_t *padv, uint8_t press);
adv_match_t adv_en_disable_key(adv_t *padv);
// adv_match_t adv_action(adv_t *padv);
// adv_match_t adv_scene(adv_t *padv);
// adv_match_t adv_mt(adv_t *padv, uint8_t ki);
// adv_match_t adv_tgl(adv_t *padv, uint8_t ki);
int8_t adv_scene_is_pass(adv_t *padv);
void adv_enable_key(adv_t *padv);
void adv_init(void);



#endif