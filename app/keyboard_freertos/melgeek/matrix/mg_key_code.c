/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include "mg_key_code.h"
#include "mg_matrix_config.h"
#include "mg_matrix_type.h"
#include "db.h"

const kc_t  kc_table_default[BOARD_KCM_MAX][BOARD_KEY_NUM] = {
    {/* Layer 0 */
        _B(KC_ESC),      _B(KC_F1),      _B(KC_F2),          _B(KC_F3),      _B(KC_F4),    	 _B(KC_F5),      _B(KC_F6),       _B(KC_F7),    _B(KC_F8),     _B(KC_F9),      _B(KC_F10),     _B(KC_F11),         _B(KC_F12),       _B(KC_INSERT),     _B(KC_DELETE),
        _B(KC_GRAVE),    _B(KC_1),       _B(KC_2),           _B(KC_3),       _B(KC_4),    	 _B(KC_5),       _B(KC_6),        _B(KC_7),     _B(KC_8),      _B(KC_9),       _B(KC_0),       _B(KC_MINUS),       _B(KC_EQUAL),     _B(KC_BSPACE),
        _B(KC_TAB),      _B(KC_Q),       _B(KC_W),           _B(KC_E),       _B(KC_R),    	 _B(KC_T),       _B(KC_Y),        _B(KC_U),     _B(KC_I),      _B(KC_O),       _B(KC_P),       _B(KC_LBRACKET),    _B(KC_RBRACKET),  _B(KC_BSLASH),
        _B(KC_CAPSLOCK), _B(KC_A),       _B(KC_S),           _B(KC_D),       _B(KC_F),    	 _B(KC_G),       _B(KC_H),        _B(KC_J),     _B(KC_K),      _B(KC_L),       _B(KC_SCOLON),  _B(KC_QUOTE),       _B(KC_ENTER), 
        _B(KC_LSHIFT),   _B(KC_Z),       _B(KC_X),           _B(KC_C),       _B(KC_V),    	 _B(KC_B),       _B(KC_N),        _B(KC_M),     _B(KC_COMMA),  _B(KC_DOT),     _B(KC_SLASH),   _B(KC_RSHIFT),                        _B(KC_UP), 
        _B(KC_LCTRL),    _B(KC_LGUI),    _B(KC_LALT),                                        _B(KC_SPACE),                                                 _B(KC_RALT),    _FN(KC_FN1),    _B(KC_RCTRL),       _B(KC_LEFT),      _B(KC_DOWN),                           _B(KC_RIGHT), 
    }, {                                                                                                                                                             
        _B(KC_TRNS),    _S(KC_BRID),     _S(KC_BRIU),        _S(MC_DESK),    _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),     _S(KC_MPRV),  _S(KC_MPLY),   _S(KC_MNXT),    _S(KC_MUTE),    _S(KC_VOLD),       _S(KC_VOLU),      _B(KC_TRNS),       _B(KC_TRNS),
        _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),        _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),  _B(KC_TRNS),   _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),       _B(KC_TRNS),      _FM(FN_MIX_KB_RESET),
        _B(KC_TRNS),    _FL(FN_LB_BACK_TOG), _FL(FN_LB_SIDE_TOG),     _B(KC_TRNS), _B(KC_TRNS), _B(KC_TRNS),    _B(KC_TRNS),     _FM(FN_MIX_SET_CFG0),  _FM(FN_MIX_SET_CFG1),   _FM(FN_MIX_SET_CFG2),    _FM(FN_MIX_SET_CFG3),     _B(KC_TRNS),       _B(KC_TRNS),         _B(KC_TRNS),
        _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),        _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),    _FM(FN_MIX_OPEN_CN_WEBSITE),     _B(KC_TRNS),  _B(KC_TRNS),   _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),       _S(KC_MPLY),
        _B(KC_TRNS),    _FR(FN_RGB_TOG), _FR(FN_RGB_MOD),    _B(KC_TRNS),_FR(FN_RGB_VAI),_FR(FN_RGB_VAD),_FM(FN_MIX_NKRO),_B(KC_TRNS),  _B(KC_TRNS),   _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),                         _S(KC_VOLU),
        _B(KC_TRNS),    _SA(SK_ADV0),    _B(KC_TRNS),                                        _B(KC_TRNS),                                                  _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),       _S(KC_MPRV),      _S(KC_VOLD),                            _S(KC_MNXT),                    
    }, {                                                                                                                                                                            
        _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),        _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),  _B(KC_TRNS),   _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),       _B(KC_TRNS),      _B(KC_TRNS),       _B(KC_TRNS), 
        _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),        _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),  _B(KC_TRNS),   _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),       _B(KC_TRNS),      _B(KC_TRNS), 
        _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),        _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),  _B(KC_TRNS),   _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),       _B(KC_TRNS),      _B(KC_TRNS), 
        _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),        _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),  _B(KC_TRNS),   _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),       _B(KC_TRNS),
        _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),        _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),  _B(KC_TRNS),   _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),                         _B(KC_TRNS),
        _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),                                        _B(KC_TRNS),                                                  _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),       _B(KC_TRNS),      _B(KC_TRNS),                            _B(KC_TRNS), 
    },                                                                                                                        
    {/* Layer 3 */                                                                                                                                                               
        _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),        _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),  _B(KC_TRNS),   _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),       _B(KC_TRNS),      _B(KC_TRNS),       _B(KC_TRNS), 
        _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),        _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),  _B(KC_TRNS),   _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),       _B(KC_TRNS),      _B(KC_TRNS), 
        _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),        _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),  _B(KC_TRNS),   _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),       _B(KC_TRNS),      _B(KC_TRNS), 
        _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),        _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),  _B(KC_TRNS),   _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),       _B(KC_TRNS), 
        _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),        _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),     _B(KC_TRNS),  _B(KC_TRNS),   _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),                         _B(KC_TRNS), 
        _B(KC_TRNS),    _B(KC_LALT),     _B(KC_LGUI),                                        _B(KC_TRNS),                                                  _B(KC_TRNS),    _B(KC_TRNS),    _B(KC_TRNS),       _B(KC_TRNS),      _B(KC_TRNS),                            _B(KC_TRNS), 
    }
};

static replace_key_t replace_key[RPL_ID_MAX] = 
{
    {KC_LSHIFT, KC_GRAVE, KC_ESC, 0},
};

replace_key_t *key_code_get_replace_key_info(uint8_t rpl_id)
{
    if (rpl_id >= RPL_ID_MAX)
    {
        return NULL;
    }
    return &replace_key[rpl_id];
}

const kc_t *kc_get_default_layer_ptr(uint8_t layer)
{
    if (layer >= BOARD_KCM_MAX)
    {
        return NULL;
    }

    return kc_table_default[layer];
}

//extern tmx_t gmx;
//kc_t   *pkc;
kc_t   (*pkcode)[BOARD_KEY_NUM];

kc_t* mx_get_kco(uint8_t layer, uint8_t ki)
{
    kc_t *keycode = &pkcode[layer][ki];
    
    if (keycode->ty == KCT_KB && keycode->co == KC_TRNS)
    {
        keycode = &pkcode[0][ki];
    }
    
    return keycode;
}

void key_code_init(kc_t      (*pkc)[BOARD_KEY_NUM])
{
    //gmx.pkc  = pkc;
    pkcode = pkc;
}

