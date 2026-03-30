#ifndef  _SK_MACRO_H_
#define  _SK_MACRO_H_

#include <stdint.h>
#include "mg_key_code.h"
#include "sk_adv.h"

#define MACRO_NUM_MAX           (8)

#define MACRO_PER_SIZE          (512)//(BOARD_MACRO_ONE_SIZE / sizeof(mframe_t))//50*4=200BYTE*8=1600BYTE

#define MACRO_DEFAULT_NAME      "Macro"
#define MACRO_NAME_SIZE         (16)
#define MACRO_DEFAULT_DELAY     (50)
#define SK_MACRO_NUM            (8)


#define EN_WEBSITE              (0)
#define CN_WEBSITE              (1)


#define _B_PRESSED(c) { .en = KCE_ENABLE, .ex = KEY_PRESSED, .ty = KCT_KB,    .sty = 0,   .co = c }
#define _B_RELEASE(c) { .en = KCE_ENABLE, .ex = KEY_RELEASE, .ty = KCT_KB,    .sty = 0,   .co = c }

#define MACRO_CN_WEBSITE {\
    .mh = {\
        .name = "cn_Website",\
        .rvs1 = 0,\
        .delay_type = DLY_STATIC,\
        .delay_ms_min = 30,\
        .delay_ms_max = 35,\
        .repeat_times = 1,\
        .total_frames = 58,\
        .trg_action = M_TRG_U2D,\
        .trg_lvl = 0,\
        .trg_time = 0,\
        .rvs2 = 0,\
    },\
\
    .frames = {\
        [0]  = { _B_PRESSED(KC_CAPSLOCK),   20 },\
        [1]  = { _B_RELEASE(KC_CAPSLOCK),   20 },\
        [2]  = { _B_PRESSED(KC_LGUI),       20 },\
        [3]  = { _B_PRESSED(KC_R),          10 },\
        [4]  = { _B_RELEASE(KC_R),          20 },\
        [5]  = { _B_RELEASE(KC_LGUI),       100 },\
        [6]  = { _B_PRESSED(KC_H),          3 },\
        [7]  = { _B_RELEASE(KC_H),          3 },\
        [8]  = { _B_PRESSED(KC_T),          3 },\
        [9]  = { _B_RELEASE(KC_T),          3 },\
        [10] = { _B_PRESSED(KC_T),          3 },\
        [11] = { _B_RELEASE(KC_T),          3 },\
        [12] = { _B_PRESSED(KC_P),          3 },\
        [13] = { _B_RELEASE(KC_P),          3 },\
        [14] = { _B_PRESSED(KC_S),          3 },\
        [15] = { _B_RELEASE(KC_S),          3 },\
        [16] = { _B_PRESSED(KC_LSHIFT),     3 },\
        [17] = { _B_PRESSED(KC_SCOLON),     3 },\
        [18] = { _B_RELEASE(KC_SCOLON),     3 },\
        [19] = { _B_RELEASE(KC_LSHIFT),     3 },\
        [20] = { _B_PRESSED(KC_SLASH),      3 },\
        [21] = { _B_RELEASE(KC_SLASH),      3 },\
        [22] = { _B_PRESSED(KC_SLASH),      3 },\
        [23] = { _B_RELEASE(KC_SLASH),      3 },\
        [24] = { _B_PRESSED(KC_H),          3 },\
        [25] = { _B_RELEASE(KC_H),          3 },\
        [26] = { _B_PRESSED(KC_I),          3 },\
        [27] = { _B_RELEASE(KC_I),          3 },\
        [28] = { _B_PRESSED(KC_V),          3 },\
        [29] = { _B_RELEASE(KC_V),          3 },\
        [30] = { _B_PRESSED(KC_E),          3 },\
        [31] = { _B_RELEASE(KC_E),          3 },\
        [32] = { _B_PRESSED(KC_DOT),        3 },\
        [33] = { _B_RELEASE(KC_DOT),        3 },\
        [34] = { _B_PRESSED(KC_M),          3 },\
        [35] = { _B_RELEASE(KC_M),          3 },\
        [36] = { _B_PRESSED(KC_E),          3 },\
        [37] = { _B_RELEASE(KC_E),          3 },\
        [38] = { _B_PRESSED(KC_L),          3 },\
        [39] = { _B_RELEASE(KC_L),          3 },\
        [40] = { _B_PRESSED(KC_G),          3 },\
        [41] = { _B_RELEASE(KC_G),          3 },\
        [42] = { _B_PRESSED(KC_E),          3 },\
        [43] = { _B_RELEASE(KC_E),          3 },\
        [44] = { _B_PRESSED(KC_E),          3 },\
        [45] = { _B_RELEASE(KC_E),          3 },\
        [46] = { _B_PRESSED(KC_K),          3 },\
        [47] = { _B_RELEASE(KC_K),          3 },\
        [48] = { _B_PRESSED(KC_DOT),        3 },\
        [49] = { _B_RELEASE(KC_DOT),        3 },\
        [50] = { _B_PRESSED(KC_C),          3 },\
        [51] = { _B_RELEASE(KC_C),          3 },\
        [52] = { _B_PRESSED(KC_N),          3 },\
        [53] = { _B_RELEASE(KC_N),          3 },\
        [54] = { _B_PRESSED(KC_ENTER),      3 },\
        [55] = { _B_RELEASE(KC_ENTER),      3 },\
        [56] = { _B_PRESSED(KC_CAPSLOCK),   20 },\
        [57] = { _B_RELEASE(KC_CAPSLOCK),   20 },\
    }\
}

#define MACRO_EN_WEBSITE {\
    .mh = {\
        .name = "en_Website",\
        .rvs1 = 0,\
        .delay_type = DLY_STATIC,\
        .delay_ms_min = 30,\
        .delay_ms_max = 35,\
        .repeat_times = 1,\
        .total_frames = 60,\
        .trg_action = M_TRG_U2D,\
        .trg_lvl = 0,\
        .trg_time = 0,\
        .rvs2 = 0,\
    },\
\
    .frames = {\
        [0]  = { _B_PRESSED(KC_CAPSLOCK),   20 },\
        [1]  = { _B_RELEASE(KC_CAPSLOCK),   20 },\
        [2]  = { _B_PRESSED(KC_LGUI),       20 },\
        [3]  = { _B_PRESSED(KC_R),          10 },\
        [4]  = { _B_RELEASE(KC_R),          20 },\
        [5]  = { _B_RELEASE(KC_LGUI),       100 },\
        [6]  = { _B_PRESSED(KC_H),          3 },\
        [7]  = { _B_RELEASE(KC_H),          3 },\
        [8]  = { _B_PRESSED(KC_T),          3 },\
        [9]  = { _B_RELEASE(KC_T),          3 },\
        [10] = { _B_PRESSED(KC_T),          3 },\
        [11] = { _B_RELEASE(KC_T),          3 },\
        [12] = { _B_PRESSED(KC_P),          3 },\
        [13] = { _B_RELEASE(KC_P),          3 },\
        [14] = { _B_PRESSED(KC_S),          3 },\
        [15] = { _B_RELEASE(KC_S),          3 },\
        [16] = { _B_PRESSED(KC_LSHIFT),     3 },\
        [17] = { _B_PRESSED(KC_SCOLON),     3 },\
        [18] = { _B_RELEASE(KC_SCOLON),     3 },\
        [19] = { _B_RELEASE(KC_LSHIFT),     3 },\
        [20] = { _B_PRESSED(KC_SLASH),      3 },\
        [21] = { _B_RELEASE(KC_SLASH),      3 },\
        [22] = { _B_PRESSED(KC_SLASH),      3 },\
        [23] = { _B_RELEASE(KC_SLASH),      3 },\
        [24] = { _B_PRESSED(KC_H),          3 },\
        [25] = { _B_RELEASE(KC_H),          3 },\
        [26] = { _B_PRESSED(KC_I),          3 },\
        [27] = { _B_RELEASE(KC_I),          3 },\
        [28] = { _B_PRESSED(KC_V),          3 },\
        [29] = { _B_RELEASE(KC_V),          3 },\
        [30] = { _B_PRESSED(KC_E),          3 },\
        [31] = { _B_RELEASE(KC_E),          3 },\
        [32] = { _B_PRESSED(KC_DOT),        3 },\
        [33] = { _B_RELEASE(KC_DOT),        3 },\
        [34] = { _B_PRESSED(KC_M),          3 },\
        [35] = { _B_RELEASE(KC_M),          3 },\
        [36] = { _B_PRESSED(KC_E),          3 },\
        [37] = { _B_RELEASE(KC_E),          3 },\
        [38] = { _B_PRESSED(KC_L),          3 },\
        [39] = { _B_RELEASE(KC_L),          3 },\
        [40] = { _B_PRESSED(KC_G),          3 },\
        [41] = { _B_RELEASE(KC_G),          3 },\
        [42] = { _B_PRESSED(KC_E),          3 },\
        [43] = { _B_RELEASE(KC_E),          3 },\
        [44] = { _B_PRESSED(KC_E),          3 },\
        [45] = { _B_RELEASE(KC_E),          3 },\
        [46] = { _B_PRESSED(KC_K),          3 },\
        [47] = { _B_RELEASE(KC_K),          3 },\
        [48] = { _B_PRESSED(KC_DOT),        3 },\
        [49] = { _B_RELEASE(KC_DOT),        3 },\
        [50] = { _B_PRESSED(KC_C),          3 },\
        [51] = { _B_RELEASE(KC_C),          3 },\
        [52] = { _B_PRESSED(KC_O),          3 },\
        [53] = { _B_RELEASE(KC_O),          3 },\
        [54] = { _B_PRESSED(KC_M),          3 },\
        [55] = { _B_RELEASE(KC_M),          3 },\
        [56] = { _B_PRESSED(KC_ENTER),      3 },\
        [57] = { _B_RELEASE(KC_ENTER),      3 },\
        [58] = { _B_PRESSED(KC_CAPSLOCK),   20 },\
        [59] = { _B_RELEASE(KC_CAPSLOCK),   20 },\
    }\
}

typedef void (* macro_play_done_fnc)(void);

typedef struct _mdelay_s {
    uint32_t ms:24;
    uint32_t ex:8;
} mdelay_t;

typedef enum
{
    DLY_STATIC,//静态延迟,固定一个延迟时间
    DLY_RANDOM,//动态延迟,指定一个延迟范围,在这个范围内随机变化
    DLY_MAX,
}m_delay_t;

typedef enum
{
    M_TRG_U2D,//按下时开始播放宏
    M_TRG_D2D,//按住到某个时间(xx ms)开始播放宏
    M_TRG_D2U,//释放时开始播放宏
    M_TRG_HOLD,
    M_TRG_TOG,
    M_TRG_MAX,
}m_trg_t;

#if 1
#pragma pack(2)

typedef struct{//一帧数据包括键码和对应的延迟
        kc_t     kc;
        uint16_t ms;
} mframe_t;

typedef struct {
    uint8_t    name[16];
    uint8_t    rvs1;
    m_delay_t  delay_type;
    uint16_t   delay_ms_min;
    uint16_t   delay_ms_max;
    uint16_t   repeat_times;
    uint8_t    total_frames;
    m_trg_t    trg_action;
    uint8_t    trg_lvl; //按键行程到达某个点(1-40)的时候播放宏9
    uint16_t   trg_time;//按住到这个时间开始播放宏
    uint8_t    rvs2;
} __attribute__ ((packed)) mh_t;

typedef struct{
    mh_t     mh;//28bbyte
    mframe_t frames[80];//6byte//最多80个事件
}macro_t;

typedef struct{
  uint32_t itf;
  macro_t  macro_table[SK_MACRO_NUM];
} __attribute__ ((aligned (4))) macro_info_t;

#pragma pack()

extern macro_info_t  macro_info;

#endif

enum {
    KEY_PRESSED = 1,
    KEY_RELEASE = 2,
    KEY_DELAY   = 3,
};

void macro_init(void);
void macro_uninit(void);
void macro_start_ex(mframe_t *pmf, int size);
adv_match_t macro_start(int id, m_trg_t trg, uint16_t trg_time);
void macro_play_end_register_cb(uint8_t marco_idx, macro_play_done_fnc fnc);
void macro_open_website(uint8_t website_type);

#endif

