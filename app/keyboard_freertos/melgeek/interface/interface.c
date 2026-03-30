/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#define DBG_TAG "interface"
#include "board.h"

//#include "main.h"
//#include "log.h"
#include "interface.h"
//#include "bitwise.h"
#include "app_debug.h"
#include "hpm_common.h"
#include "mg_key_code.h"
#include "db.h"
#include "mg_hid.h"
#include "layout_fn.h"
#include "mg_matrix.h"
#include "sk.h"


#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "interface"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

static rep_hid_t *kb_rpt;
static rep_hid_t *ms_rpt;
static rep_hid_t *sys_rpt;
static rep_hid_t *sk_rpt;
static uint32_t msg;

extern uint8_t usb_report_keyboard_led(void);
extern void usb_report_keyboard_send(rep_hid_t *report);

extern void usb_report_system_send(report_extra_t *report);
extern void usb_report_consumer_send(report_extra_t *report);
extern void usb_report_raw_send(uint8_t *report, uint8_t size);
extern bool usb_report_get_protocol(void);
extern rep_hid_t *matrix_get_sys_report(void);

bool interface_get_protocol(void)
{
    return usb_report_get_protocol();
}

bool interface_get_nkro(void)
{
    return true;
}



void interface_keyboard_send(rep_hid_t *report)
{
    if (interface_get_protocol() && interface_get_nkro())
    {
        report->nkro.report_id = REPORT_ID_NKRO;
    }
    else
    {
        report->report_id = REPORT_ID_KEYBOARD;
    }

    //return hal_hid_send_nkro(report->raw, 32);
}


void interface_system_send(report_extra_t *report)
{
    static uint16_t last_usage = 0;
    if (report->usage == last_usage)
    {
        return;
    }
    last_usage = report->usage;
    report->report_id = REPORT_ID_SYSTEM;
    return usb_report_system_send(report);
}

void interface_consumer_send(report_extra_t *report)
{
    static uint16_t last_usage = 0;
    if (report->usage == last_usage)
    {
        return;
    }
    last_usage = report->usage;
    report->report_id = REPORT_ID_CONSUMER;
    return usb_report_consumer_send(report);
}

ATTR_WEAK void interface_raw_recv(uint8_t *report, uint8_t size)
{
    (void)report;
    (void)size;
    DBG_PRINTF("recv raw report size %d\r\n", size);
}

void interface_raw_send(uint8_t *report, uint8_t size)
{
    return usb_report_raw_send(report, size);
}

#if 0
void interface_raw_task(void)
{
    UINT status;
    uint8_t *report;

    for (;;)
    {
        status = tx_queue_receive(&queue_hid_raw_out, &report, TX_NO_WAIT);
        if (TX_QUEUE_EMPTY == status)
        {
            break;
        }
        KBD_ASSERT_ZERO_PARAM(status);
        interface_raw_recv(report, 32);
        KBD_ASSERT_ZERO_FUNC(tx_block_release(report));
    }
}
#endif

void report_add_ms_btn(rep_hid_t *report, uint8_t btn)
{
    if (btn > KC_MS_BTN_SP)
    {
        return;
    }
    report->mouse.buttons |= (0x01 << btn);
}

void report_del_ms_btn(rep_hid_t *report, uint8_t btn)
{
    if (btn > KC_MS_BTN_SP)
    {
        return;
    }
    report->mouse.buttons &= ~(0x01 << btn);
}

void report_set_ms_x(rep_hid_t *report, int8_t val)
{
    report->mouse.x = val;
}
void report_set_ms_y(rep_hid_t *report, int8_t val)
{
    report->mouse.y = val;
}
void report_set_ms_w(rep_hid_t *report, int8_t val)
{
    report->mouse.wheel = val;
}



bool report_has_anykey(rep_hid_t *report)
{
    bool anykey = false;
    uint8_t *p = report->keys;
    uint8_t lp = sizeof(report->keys);
    if (report->report_id == REPORT_ID_NKRO)
    {
        p = report->nkro.bits;
        lp = sizeof(report->nkro.bits);
    }
    while (lp--)
    {
        if (*p++)
        {
            anykey = true;
            break;
        }
    }

    if (report->modifier_key != 0)
    {
        anykey = true;
    }

    return anykey;
}

bool report_is_key_pressed(rep_hid_t *report, uint8_t key)
{
    if (key == KC_NO)
    {
        return false;
    }
    
    if (report->report_id == REPORT_ID_NKRO)
    {
        if ((key >> 3) < sizeof(report->nkro.bits))
        {
            return report->nkro.bits[key >> 3] & 1 << (key & 7);
        }
        else
        {
            return false;
        }
    }
    
    for (uint8_t i = 0; i < sizeof(report->keys); i++)
    {
        if (report->keys[i] == key)
        {
            return true;
        }
    }
    return false;
}

void report_add_key_byte(rep_hid_t *report, uint8_t code)
{
    bool is_modifier_key = false;

    is_modifier_key = key_code_is_modifier_key(code);
    if (true == is_modifier_key)
    {
        uint8_t mask = key_code_get_modifier_key_mask(code);
        report->modifier_key |= mask;
    }
    else
    {
        for (uint8_t i = 0; i < sizeof(report->keys); i++)
        {
            if (report->keys[i] == 0)
            {
                report->keys[i] = code;
                break;
            }
        }
    }
}

void report_del_key_byte(rep_hid_t *report, uint8_t code)
{
    bool is_modifier_key = false;

    is_modifier_key = key_code_is_modifier_key(code);
    if (true == is_modifier_key)
    {
        uint8_t mask = key_code_get_modifier_key_mask(code);
        report->modifier_key &= ~mask;
    }
    else
    {
        for (uint8_t i = 0; i < sizeof(report->keys); i++)
        {
            if (report->keys[i] == code)
            {
                report->keys[i] = 0;
                break;
            }
        }
    }
}
#if 1
void report_add_key_bit(rep_hid_t *report, uint8_t code)
{
    bool is_modifier_key = false;

    is_modifier_key = key_code_is_modifier_key(code);
    if (true == is_modifier_key)
    {
        uint8_t mask = key_code_get_modifier_key_mask(code);
        report->modifier_key |= mask;
    }
    else
    {
        if ((code >> 3) < sizeof(report->nkro.bits))
        {
            report->nkro.bits[code >> 3] |= 1 << (code & 7);
        }
        else
        {
            DBG_PRINTF("can't add nkro key 0x%02x\r\n", code);
        }
    }
}

void report_del_key_bit(rep_hid_t *report, uint8_t code)
{
    bool is_modifier_key = false;

    is_modifier_key = key_code_is_modifier_key(code);
    if (true == is_modifier_key)
    {
        uint8_t mask = key_code_get_modifier_key_mask(code);
        report->modifier_key &= ~mask;
    }
    else
    {
        if ((code >> 3) < sizeof(report->nkro.bits))
        {
            report->nkro.bits[code >> 3] &= ~(1 << (code & 7));
        }
        else
        {
            DBG_PRINTF("can't del nkro key 0x%02x\r\n", code);
        }
    }
}
#endif

void report_add_sys_word(rep_hid_t *report, uint16_t cmd)
{
    report->sys_mod_key.cmd = cmd;
} 

void report_del_sys_word(rep_hid_t *report, uint16_t cmd)
{
    if (report->sys_mod_key.cmd == cmd)
    {
        report->sys_mod_key.cmd = 0;
    }
}

void report_add_key(rep_hid_t *report, uint8_t key)
{
    if (report->report_id == REPORT_ID_NKRO)
    {
        report_add_key_bit(report, key);
        return;
    }
    report_add_key_byte(report, key);
}

void report_del_key(rep_hid_t *report, uint8_t key)
{
    if (report->report_id == REPORT_ID_NKRO)
    {
        report_del_key_bit(report, key);
        return;
    }
    report_del_key_byte(report, key);
}

bool report_has_key(rep_hid_t *report, uint8_t key)
{
    bool has_key = false;

    bool is_modifier_key = key_code_is_modifier_key(key);
    if (true == is_modifier_key)
    {
        uint8_t mask = key_code_get_modifier_key_mask(key);
        if (report->modifier_key & mask)
        {
            return true;
        }
    }
    else
    {
        if (report->report_id == REPORT_ID_NKRO)
        {
            if ((key >> 3) < sizeof(report->nkro.bits))
            {
                if (report->nkro.bits[key >> 3] & (1 << (key & 7)))
                {
                    return true;
                }
            }
            else
            {
                DBG_PRINTF("can't get nkro key 0x%02x\r\n", key);
            }
        }
        else if (report->report_id == REPORT_ID_KEYBOARD)
        {
            for (uint8_t i = 0; i < sizeof(report->keys); i++)
            {
                if (report->keys[i] == key)
                {
                    return true;
                }
            }
        }
    }

    return has_key;
}

void report_clr_key(rep_hid_t *report)
{
    memset((uint8_t *)report + 1, 0, sizeof(rep_hid_t) - 1);
}

uint8_t report_get_mod_keys(rep_hid_t *report)
{
    if (report->report_id == REPORT_ID_KEYBOARD || report->report_id == REPORT_ID_NKRO)                                                           
    {
        return report->modifier_key;
    }
    return 0;
}

void report_add_sys_cmd(rep_hid_t *report, uint16_t cmd)
{
    if (report->sys_mod_key.report_id == REPORT_ID_SYSTEM)
    {
        report_add_sys_word(report, cmd);
    }
}

void report_del_sys_cmd(rep_hid_t *report, uint16_t cmd)
{
    if (report->sys_mod_key.report_id == REPORT_ID_SYSTEM)
    {
        report_del_sys_word(report, cmd);
    }
}


void interface_release_report(void)
{
    kb_rpt  = matrix_get_kb_report();
    ms_rpt  = matrix_get_ms_report();
    sys_rpt = matrix_get_sys_report();
    sk_rpt  = sk_get_kb_report();

    msg = (uint32_t)sk_rpt;
    memset(sk_rpt->raw, 0, sizeof(sk_rpt->raw));
    sk_rpt->report_id = gbinfo.nkro ? REPORT_ID_NKRO : REPORT_ID_KEYBOARD;
    xQueueSend(hid_queue, (const void * const )&msg, ( TickType_t ) 0 );

    msg = (uint32_t)kb_rpt;
    memset(kb_rpt->raw, 0, sizeof(kb_rpt->raw));
    kb_rpt->report_id = gbinfo.nkro ? REPORT_ID_NKRO : REPORT_ID_KEYBOARD;
    xQueueSend(hid_queue, (const void * const )&msg, ( TickType_t ) 0 );

    msg = (uint32_t)ms_rpt;
    memset(ms_rpt->raw, 0, sizeof(ms_rpt->raw));
    ms_rpt->report_id = REPORT_ID_MOUSE;
    xQueueSend(hid_queue, (const void * const )&msg, ( TickType_t ) 0 );
    
    msg = (uint32_t)sys_rpt;
    memset(sys_rpt->raw, 0, sizeof(sys_rpt->raw));
    sys_rpt->report_id = REPORT_ID_SYSTEM;
    xQueueSend(hid_queue, (const void * const )&msg, ( TickType_t ) 0 );
}


#if 0
void interface_keyboard_test(void)
{
    static uint8_t idx = 0;
    const uint8_t array[] = {
        KC_H,
        KC_E,
        KC_L,
        KC_L,
        KC_O,
        KC_SPACE,
        KC_H,
        KC_P,
        KC_M,
        KC_I,
        KC_C,
        KC_R,
        KC_O,
        KC_ENTER,
    };
    rep_hid_t report;
    memset(&report, 0, sizeof(rep_hid_t));

    if (idx % 2)
    {
        report_del_key(&report, array[idx / 2]);
    }
    else
    {
        report_add_key(&report, array[idx / 2]);
    }

    idx++;
    if (idx >= 2 * sizeof(array))
    {
        idx = 0;
    }
    interface_keyboard_send(&report);
}
#endif

