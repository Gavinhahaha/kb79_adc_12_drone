/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#ifndef __MG_KC_H_H__
#define __MG_KC_H_H__

#include <stdint.h>
#include <stdbool.h>
#include "mg_matrix_config.h"




/* USB HID Keyboard/Keypad Usage(0x07) */
enum hid_keyboard_keypad_usage {
    KC_NO                   = 0x00,
    KC_ROLL_OVER,           //0x01
    KC_POST_FAIL,           //0x02
    KC_UNDEFINED,           //0x03
    KC_A,                   //0x04
    KC_B,                   //0x05
    KC_C,                   //0x06
    KC_D,                   //0x07
    KC_E,                   //0x08
    KC_F,                   //0x09
    KC_G,                   //0x0a
    KC_H,                   //0x0b
    KC_I,                   //0x0c
    KC_J,                   //0x0d
    KC_K,                   //0x0e
    KC_L,                   //0x0f
    KC_M,                   //0x10
    KC_N,                   //0x11
    KC_O,                   //0x12
    KC_P,                   //0x13
    KC_Q,                   //0x14
    KC_R,                   //0x15
    KC_S,                   //0x16
    KC_T,                   //0x17
    KC_U,                   //0x18
    KC_V,                   //0x19
    KC_W,                   //0x1a
    KC_X,                   //0x1b
    KC_Y,                   //0x1c
    KC_Z,                   //0x1d
    KC_1,                   //0x1e
    KC_2,                   //0x1f
    KC_3,                   //0x20
    KC_4,                   //0x21
    KC_5,                   //0x22
    KC_6,                   //0x23
    KC_7,                   //0x24
    KC_8,                   //0x25
    KC_9,                   //0x26
    KC_0,                   //0x27
    KC_ENTER,               //0x28
    KC_ESC,                 //0x29
    KC_BSPACE,              //0x2a
    KC_TAB,                 //0x2b
    KC_SPACE,               //0x2c
    KC_MINUS,               //0x2d
    KC_EQUAL,               //0x2e
    KC_LBRACKET,            //0x2f
    KC_RBRACKET,            //0x30
    KC_BSLASH,              //0x31      /* \ (and |) */
    KC_NONUS_HASH,          //0x32      /* Non-US # and ~ (Typically near the Enter key) */
    KC_SCOLON,              //0x33      /* ; (and :) */
    KC_QUOTE,               //0x34      /* ' and " */
    KC_GRAVE,               //0x35      /* Grave accent and tilde */
    KC_COMMA,               //0x36      /* , and < */
    KC_DOT,                 //0x37      /* . and > */
    KC_SLASH,               //0x38      /* / and ? */
    KC_CAPSLOCK,            //0x39
    KC_F1,                  //0x3a
    KC_F2,                  //0x3b
    KC_F3,                  //0x3c
    KC_F4,                  //0x3d
    KC_F5,                  //0x3e
    KC_F6,                  //0x3f
    KC_F7,                  //0x40
    KC_F8,                  //0x41
    KC_F9,                  //0x42
    KC_F10,                 //0x43
    KC_F11,                 //0x44
    KC_F12,                 //0x45
    KC_PSCREEN,             //0x46
    KC_SCROLLLOCK,          //0x47
    KC_PAUSE,               //0x48
    KC_INSERT,              //0x49
    KC_HOME,                //0x4a
    KC_PGUP,                //0x4b
    KC_DELETE,              //0x4c
    KC_END,                 //0x4d
    KC_PGDOWN,              //0x4e
    KC_RIGHT,               //0x4f
    KC_LEFT,                //0x50
    KC_DOWN,                //0x51
    KC_UP,                  //0x52
    KC_NUMLOCK,             //0x53
    KC_KP_SLASH,            //0x54
    KC_KP_ASTERISK,         //0x55
    KC_KP_MINUS,            //0x56
    KC_KP_PLUS,             //0x57
    KC_KP_ENTER,            //0x58
    KC_KP_1,                //0x59
    KC_KP_2,                //0x5a
    KC_KP_3,                //0x5b
    KC_KP_4,                //0x5c
    KC_KP_5,                //0x5d
    KC_KP_6,                //0x5e
    KC_KP_7,                //0x5f
    KC_KP_8,                //0x60
    KC_KP_9,                //0x61
    KC_KP_0,                //0x62
    KC_KP_DOT,              //0x63
    KC_NONUS_BSLASH,        //0x64      /* Non-US \ and | (Typically near the Left-Shift key) */
    KC_APPLICATION,         //0x65
    KC_POWER,               //0x66
    KC_KP_EQUAL,            //0x67
    KC_F13,                 //0x68
    KC_F14,                 //0x69
    KC_F15,                 //0x6a
    KC_F16,                 //0x6b
    KC_F17,                 //0x6c
    KC_F18,                 //0x6d
    KC_F19,                 //0x6e
    KC_F20,                 //0x6f
    KC_F21,                 //0x70
    KC_F22,                 //0x71
    KC_F23,                 //0x72
    KC_F24,                 //0x73
    KC_EXECUTE,             //0x74
    KC_HELP,                //0x75
    KC_MENU,                //0x76
    KC_SELECT,              //0x77
    KC_STOP,                //0x78
    KC_AGAIN,               //0x79
    KC_UNDO,                //0x7a
    KC_CUT,                 //0x7b
    KC_COPY,                //0x7c
    KC_PASTE,               //0x7d
    KC_FIND,                //0x7e
    KC__MUTE,               //0x7f
    KC__VOLUP,              //0x80
    KC__VOLDOWN,            //0x81
    KC_LOCKING_CAPS,        //0x82      /* locking Caps Lock */
    KC_LOCKING_NUM,         //0x83      /* locking Num Lock */
    KC_LOCKING_SCROLL,      //0x84      /* locking Scroll Lock */
    KC_KP_COMMA,            //0x85
    KC_KP_EQUAL_AS400,      //0x86      /* equal sign on AS/400 */
    KC_INT1,                //0x87
    KC_INT2,                //0x88
    KC_INT3,                //0x89
    KC_INT4,                //0x8a
    KC_INT5,                //0x8b
    KC_INT6,                //0x8c
    KC_INT7,                //0x8d
    KC_INT8,                //0x8e
    KC_INT9,                //0x8f
    KC_LANG1,               //0x90
    KC_LANG2,               //0x91
    KC_LANG3,               //0x92
    KC_LANG4,               //0x93
    KC_LANG5,               //0x94
    KC_LANG6,               //0x95
    KC_LANG7,               //0x96
    KC_LANG8,               //0x97
    KC_LANG9,               //0x98
    KC_ALT_ERASE,           //0x99
    KC_SYSREQ,              //0x9a
    KC_CANCEL,              //0x9b
    KC_CLEAR,               //0x9c
    KC_PRIOR,               //0x9d
    KC_RETURN,              //0x9e
    KC_SEPARATOR,           //0x9f
    KC_OUT,                 //0xA0
    KC_OPER,                //0xa1
    KC_CLEAR_AGAIN,         //0xa2
    KC_CRSEL,               //0xa3
    KC_EXSEL,               //0xA4

    /* Modifiers */
    KC_LCTRL                = 0xE0,
    KC_LSHIFT               = 0xE1,
    KC_LALT                 = 0xE2,
    KC_LGUI                 = 0xE3,
    KC_RCTRL                = 0xE4,
    KC_RSHIFT               = 0xE5,
    KC_RALT                 = 0xE6,
    KC_RGUI                 = 0xE7,

    KC_FN0                  = 0xF0,
    KC_FN1                  = 0xF1,
    KC_FN2                  = 0xF2,
    KC_FN3                  = 0xF3,
};


/* Consumer Page (0x0C)
 *
 * See https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf#page=75
 */
enum consumer_usages {
    // 15.5 Display Controls
    SNAPSHOT               = 0x065,
    BRIGHTNESS_UP          = 0x06F, // https://www.usb.org/sites/default/files/hutrr41_0.pdf
    BRIGHTNESS_DOWN        = 0x070,
    // 15.7 Transport Controls
    TRANSPORT_RECORD       = 0x0B2,
    TRANSPORT_FAST_FORWARD = 0x0B3,
    TRANSPORT_REWIND       = 0x0B4,
    TRANSPORT_NEXT_TRACK   = 0x0B5,
    TRANSPORT_PREV_TRACK   = 0x0B6,
    TRANSPORT_STOP         = 0x0B7,
    TRANSPORT_EJECT        = 0x0B8,
    TRANSPORT_RANDOM_PLAY  = 0x0B9,
    TRANSPORT_STOP_EJECT   = 0x0CC,
    TRANSPORT_PLAY_PAUSE   = 0x0CD,
    // 15.9.1 Audio Controls - Volume
    AUDIO_MUTE             = 0x0E2,
    AUDIO_VOL_UP           = 0x0E9,
    AUDIO_VOL_DOWN         = 0x0EA,
    // 15.15 Application Launch Buttons
    AL_CC_CONFIG           = 0x183,
    AL_EMAIL               = 0x18A,
    AL_CALCULATOR          = 0x192,
    AL_LOCAL_BROWSER       = 0x194,
    AL_LOCK                = 0x19E,
    AL_CONTROL_PANEL       = 0x19F,
    AL_ASSISTANT           = 0x1CB,
    AL_KEYBOARD_LAYOUT     = 0x1AE,
    // 15.16 Generic GUI Application Controls
    AC_NEW                 = 0x201,
    AC_OPEN                = 0x202,
    AC_CLOSE               = 0x203,
    AC_EXIT                = 0x204,
    AC_MAXIMIZE            = 0x205,
    AC_MINIMIZE            = 0x206,
    AC_SAVE                = 0x207,
    AC_PRINT               = 0x208,
    AC_PROPERTIES          = 0x209,
    AC_UNDO                = 0x21A,
    AC_COPY                = 0x21B,
    AC_CUT                 = 0x21C,
    AC_PASTE               = 0x21D,
    AC_SELECT_ALL          = 0x21E,
    AC_FIND                = 0x21F,
    AC_SEARCH              = 0x221,
    AC_HOME                = 0x223,
    AC_BACK                = 0x224,
    AC_FORWARD             = 0x225,
    AC_STOP                = 0x226,
    AC_REFRESH             = 0x227,
    AC_BOOKMARKS           = 0x22A,
    MC_DESKTOP             = 0x29F,
    MC_LAUNCHPAD           = 0x2A0
};



#define MODIFIER_L_CTRL_OFFSET      (0)
#define MODIFIER_L_SHIFT_OFFSET     (1)
#define MODIFIER_L_ALT_OFFSET       (2)
#define MODIFIER_L_GUI_OFFSET       (3)
#define MODIFIER_R_CTRL_OFFSET      (4)
#define MODIFIER_R_SHIFT_OFFSET     (5)
#define MODIFIER_R_ALT_OFFSET       (6)
#define MODIFIER_R_GUI_OFFSET       (7)

#define BOARD_KCM_MAX                   (4)
#define BOARD_USED_KCM_MAX              (2)
//#define BOARD_KEY_NUM                   (68)

/* Key Code Enable/Disable */
enum {
    KCE_DISABLE = 0,
    KCE_ENABLE  = 1
};

/* Key Code Type */
enum {
    KCT_NONE = 0,       /* 0 */
    KCT_KB,             /* 1 */
    KCT_SYS,            /* 2 */
    KCT_MS,             /* 3 */
    KCT_FN,             /* 4 */
    KCT_SK,             /* 5 */
    KCT_RPL,            /* 6 */
    KCT_MAX,            /* 7 */
};

/* Sub Type KCT_SK */
enum {
    SK_DKS,             /* 0 */
    SK_ADV,             /* 1 */
    SK_MACRO,           /* 2 */
    SK_MAX,             /*  */
};
    
/* Sub Type KCT_FN */
enum {
    FN_LAYER,           /* 0 */
    FN_BT,              /* 1 */
    FN_RADIO,           /* 2 */
    FN_RGB,             /* 3 */
    FN_AUDIO,           /* 4 */
    FN_DISP,            /* 5 */
    FN_LB,              /* 6 */  //light bar
    FN_MIX,             /* 7 */
    FN_MAX
};

typedef enum _sk_adv_ex_s
{
    ADV_REPLACE = 0,//替换键
    ADV_ACTION, //快捷键
    ADV_LOCK,   //锁键
    ADV_SCENE,  //场景切换
    ADV_MT,     //MT
    ADV_TGL,    //TGL
    ADV_MTP,    //MTP
    ADV_MAX,
}sk_adv_ex_t;

enum {
    FN_RGB_TOG             = 0,
    FN_RGB_MOD             = 1,
    // FN_RGB_COL             = 2,
    FN_RGB_VAI             = 3,
    FN_RGB_VAD             = 4,
    FN_RGB_MAX,
};
//
enum {

    FN_LB_TOG                  = 0,
    FN_LB_MOD                  = 1,
    FN_LB_VAI                  = 2,
    FN_LB_VAD                  = 3,

    FN_LB_BACK_TOG             = 4,
    FN_LB_BACK_MOD             = 5,
    FN_LB_BACK_VAI             = 6,
    FN_LB_BACK_VAD             = 7,
    
    FN_LB_SIDE_TOG             = 8,
    FN_LB_SIDE_MOD             = 9,
    FN_LB_SIDE_VAI             = 10,
    FN_LB_SIDE_VAD             = 11,
    
    FN_LB_MAX,
};

enum {
    FN_MIX_LAYOUT         = 0,
    FN_MIX_NKRO           = 1,
    FN_MIX_RT             = 2,
    FN_MIX_RT_CONTINUE    = 3,
    FN_MIX_KB_RESET       = 4,
    FN_MIX_SET_CFG0       = 5,
    FN_MIX_SET_CFG1       = 6,
    FN_MIX_SET_CFG2       = 7,
    FN_MIX_SET_CFG3       = 8,
    FN_MIX_OPEN_CN_WEBSITE = 9,
    FN_MIX_OPEN_EN_WEBSITE = 10,
    FN_MIX_MAX,
};

enum {
    RPL_ID_0,
    RPL_ID_MAX,
};
    
/* SK_DKS Type */
typedef enum  {
    SK_DKS0               = 0x00,
    SK_DKS1               = 0x01,
    SK_DKS2               = 0x02,
    SK_DKS3               = 0x03,
    SK_DKS4               = 0x04,
    SK_DKS5               = 0x05,
    SK_DKS6               = 0x06,
    SK_DKS7               = 0x07,
    SK_DKS_MAX,
}sk_dks_co_t;

/* SK_ADV Type */
typedef enum {
    SK_ADV0               = 0,
    SK_ADV1               = 1,
    SK_ADV2               = 2,
    SK_ADV3               = 3,
    SK_ADV4               = 4,
    SK_ADV5               = 5,
    SK_ADV6               = 6,
    SK_ADV7               = 7,
    SK_ADV8               = 8,
    SK_ADV9               = 9,
    SK_ADV10              = 10,
    SK_ADV11              = 11,
    SK_ADV12              = 12,
    SK_ADV13              = 13,
    SK_ADV14              = 14,
    SK_ADV15              = 15,
    SK_ADV_MAX,
}sk_adv_co_t;

/* SK_ADV Type */
typedef enum {
    SK_M0               = 0x00,
    SK_M1               = 0x01,
    SK_M2               = 0x02,
    SK_M3               = 0x03,
    SK_M4               = 0x04,
    SK_M5               = 0x05,
    SK_M6               = 0x06,
    SK_M7               = 0x07,
    SK_MACRO_MAX,
}sk_macro_co_t;

enum {
    KC_BT0            = 0,
    KC_BT1            = 1,
    KC_BT2            = 2,
    KC_BT3            = 3,
    KC_BT4            = 4,
    KC_BT5            = 5,
    KC_BT6            = 6,
    KC_BT7            = 7,
    KC_USB            = 8,
    KC_24G            = 9,
    KC_FR             = 10,
};

enum {
    KCT_SYS_CONSUME = 0,
    KCT_SYS_CTRL    = 1,
};

enum {
    KC_MS_BTN_L = 0,
    KC_MS_BTN_R,
    KC_MS_BTN_M,
    KC_MS_BTN_SN,
    KC_MS_BTN_SP,
//    KC_MS_BTN5,
//    KC_MS_BTN6,
//    KC_MS_BTN7,
    KC_MS_NY,
    KC_MS_PY,
    KC_MS_NX,
    KC_MS_PX,
    KC_MS_WU,
    KC_MS_WD,
};
    
typedef struct _kc_s {
    uint32_t en  :1;    /* enable or disable */
    uint32_t ex  :7;    /* ex @sk_adv_ex_t*/
    uint32_t ty  :4;    /* key code Type */
    uint32_t sty :4;    /* key code Subtype */
    uint32_t co  :16;   /* key Code */
} __attribute__ ((aligned (4))) kc_t;

typedef struct 
{
    uint8_t  precond_co;   //前提条件对应的键值 (例如当shift处于按下状态时，esc键替换成`键)
    uint8_t  rpl_co;       //需要替换的键值
    uint8_t  dft_co;       //默认键值
    uint8_t  cur_rpl_sta;  //当前处于默认键值状态还是替换键值状态(0:默认 1:替换)
}replace_key_t;


/* Consumer Page (0x0C) */
#define KC_MUTE AUDIO_MUTE
#define KC_VOLU AUDIO_VOL_UP
#define KC_VOLD AUDIO_VOL_DOWN
#define KC_MNXT TRANSPORT_NEXT_TRACK
#define KC_MPRV TRANSPORT_PREV_TRACK
#define KC_MSTP TRANSPORT_STOP
#define KC_MPLY TRANSPORT_PLAY_PAUSE
#define KC_MSEL AL_CC_CONFIG
#define KC_EJCT TRANSPORT_STOP_EJECT
#define KC_CALC AL_CALCULATOR
#define KC_MYCM AL_LOCAL_BROWSER
#define KC_WSCH AC_SEARCH
#define KC_WHOM AC_HOME
#define KC_WBAK AC_BACK
#define KC_WFWD AC_FORWARD
#define KC_WSTP AC_STOP
#define KC_WREF AC_REFRESH
#define KC_WFAV AC_BOOKMARKS
#define KC_MFFD TRANSPORT_FAST_FORWARD
#define KC_MRWD TRANSPORT_REWIND
#define KC_BRIU BRIGHTNESS_UP
#define KC_BRID BRIGHTNESS_DOWN

/* Modifiers */
#define KC_LCTL KC_LCTRL
#define KC_LSFT KC_LSHIFT
#define KC_LOPT KC_LALT
#define KC_LCMD KC_LGUI
#define KC_LWIN KC_LGUI
#define KC_RCTL KC_RCTRL
#define KC_RSFT KC_RSHIFT
#define KC_ALGR KC_RALT
#define KC_ROPT KC_RALT
#define KC_RCMD KC_RGUI
#define KC_RWIN KC_RGUI
#define KC_TRNS KC_ROLL_OVER

#define MC_DESK MC_DESKTOP
#define MC_LPAD MC_LAUNCHPAD

#define FN_LAYOUT_INDEX    (1)

#define _B(c)  { .en = KCE_ENABLE, .ty = KCT_KB,    .sty = 0,        .co = c }
#define _S(c)  { .en = KCE_ENABLE, .ty = KCT_SYS,   .sty = 0,        .co = c }
#define _M(c)  { .en = KCE_ENABLE, .ty = KCT_MS,    .sty = 0,        .co = c }
#define _FN(c) { .en = KCE_ENABLE, .ty = KCT_FN,    .sty = FN_LAYER, .co = c }
#define _FB(c) { .en = KCE_ENABLE, .ty = KCT_FN,    .sty = FN_BT,    .co = c }
#define _FL(c) { .en = KCE_ENABLE, .ty = KCT_FN,    .sty = FN_LB,    .co = c }
#define _FR(c) { .en = KCE_ENABLE, .ty = KCT_FN,    .sty = FN_RGB,   .co = c }
#define _FM(c) { .en = KCE_ENABLE, .ty = KCT_FN,    .sty = FN_MIX,   .co = c }
#define _SA(c) { .en = KCE_ENABLE, .ty = KCT_SK,    .sty = SK_ADV,   .co = c }
#define _RPL(c){ .en = KCE_ENABLE, .ty = KCT_RPL,   .sty = 0,        .co = c }


static inline bool key_code_is_modifier_key(uint8_t code)
{
    if (code >= KC_LCTRL && code <= KC_RGUI)
    {
        return true;
    }
    return false;
}

static inline uint8_t key_code_get_modifier_key_mask(uint8_t code)
{
    return (1U << (code - KC_LCTRL));
}

const kc_t *kc_get_default_layer_ptr(uint8_t layer);

void key_code_init(kc_t      (*pkc)[BOARD_KEY_NUM]);
kc_t* mx_get_kco(uint8_t layer, uint8_t ki);
replace_key_t *key_code_get_replace_key_info(uint8_t rpl_id);

//void kc_init(void);
//void kc_deinit(void);

extern const kc_t  kc_table_default[BOARD_KCM_MAX][BOARD_KEY_NUM];
#endif


