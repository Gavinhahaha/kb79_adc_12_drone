#ifndef  _MG_INDEV_
#define  _MG_INDEV_


#include <stdint.h>

typedef enum
{
    KEY_CODE_HOME = 0,
    KEY_CODE_PGUP,
    KEY_CODE_END,
    KEY_CODE_PGDN,
    KEY_CODE_VOLD,
    KEY_CODE_MUTE,
    KEY_CODE_VOLU,
    KEY_CODE_MPRV,
    KEY_CODE_MNXT,
    KEY_CODE_MPLY,
    KEY_CODE_MAX,
} key_code_type_e;

#define INEDV_EVT_KEY_CODE_RPT               (1 << 0)
#define INEDV_EVT_CUSTOM_KEY_RPT             (1 << 1)
#define INEDV_EVT_BKL_BRI_TOG                (1 << 2)
#define INEDV_EVT_AMB_BACK_BRI_TOG           (1 << 3)
#define INEDV_EVT_AMB_SIDE_BRI_TOG           (1 << 4)
#define INEDV_EVT_SWITCH_CFG                 (1 << 5)
#define INEDV_EVT_OPEN_CN_WEBSITE            (1 << 6)
#define INEDV_EVT_OPEN_EN_WEBSITE            (1 << 7)


void mg_indev_handler(uint32_t evt, uint8_t *param);
void mg_indev_init(void);

#endif



