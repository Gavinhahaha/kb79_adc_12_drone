#ifndef __WK_KB_H_H__
#define __WK_KB_H_H__

//#include "wk_kc.h"
//#include "wk_macro.h"
#include "easy_fifo.h"

//#include <stdint.h>
//#include <stdbool.h>
//#include "wk_adv.h"

#define KB_MAX_PRESSED_KEYS     (BOARD_KEY_NUM)

#define KB_BITMAP_MAX_KEYS              (144)
#define KB_BITMAP_REPORT_COUNT          (128)
#define KB_BITMAP_REPORT_BUF_SIZE       (19)

typedef enum {
    SCAN_BY_TIMER  = 0,
    SCAN_TYPE_TIMER2GPIOTE,
    SCAN_BY_GPIOTE,
    SCAN_TYPE_GPIOTE2TIMER,
}scan_type_e;





typedef uint32_t (*kb_callback)(void *pdata);
typedef uint32_t (*kb_click)(uint8_t ki);

typedef uint32_t (*hid_callback)(uint8_t *pdata, uint8_t size);


typedef struct _kb_s {
    uint32_t    size:8;
    uint32_t    in:1;    /* input flag */
    uint32_t    rf:1;    /* release flag */
    uint32_t    cfh:1;   /* change flag handle or not */
    uint8_t     *buf;
    fifo_t      *fifo;
    kb_callback init;
    kb_callback feed;
    kb_callback send;
    kb_click    click;
} kb_t;

typedef struct _keypos_s {
    uint8_t row;
    uint8_t col;
} keypos_t;

extern kb_t kb_table[KCT_MAX];
extern kc_t key_events[BOARD_KEY_NUM];


extern hid_callback hid_kb_send;
extern hid_callback hid_ms_send;
extern hid_callback hid_sys_send;

void kb_init(void);
void kb_deinit(void);
void kb_wakeup(void);

void kb_scan_reset(void);

void kb_dispatch_keys(bool bch);

uint32_t kb_get_inactive_time(void);
void kb_set_inactive_time(uint32_t t);
void kb_set_scan_timer(uint32_t ms);
void kb_scan_init(void);
void kb_scan_uninit(void);

#endif
