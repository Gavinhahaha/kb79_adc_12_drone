/*
 * Copyright (c) 2021-2022 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include <stdint.h>
#include <stdbool.h>
//#include "keycode.h"

#define USB_BUSID 0

enum hid_report_ids
{
    REPORT_ID_KEYBOARD = 1,
    REPORT_ID_MOUSE = 2,
    REPORT_ID_SYSTEM = 4,
    REPORT_ID_PROGRAMMABLE_BUTTON = 5,
    REPORT_ID_NKRO = 6,
    REPORT_ID_JOYSTICK = 7,
    REPORT_ID_DIGITIZER = 8,
    REPORT_ID_DIAL = 9,
    REPORT_ID_CONSUMER = 10,
    REPORT_ID_MAX = 10,
};

enum mouse_button
{
    MOUSE_BTN1 = 0x01,
    MOUSE_BTN2 = 0x02,
    MOUSE_BTN3 = 0x04,
    MOUSE_BTN4 = 0x08,
    MOUSE_BTN5 = 0x10,
    MOUSE_BTN6 = 0x20,
    MOUSE_BTN7 = 0x40,
    MOUSE_BTN8 = 0x80,
};
#if 0
enum consumer_usages
{
    // 15.5 Display Controls
    SNAPSHOT = 0x065,
    BRIGHTNESS_UP = 0x06F,
    BRIGHTNESS_DOWN = 0x070,
    // 15.7 Transport Controls
    TRANSPORT_RECORD = 0x0B2,
    TRANSPORT_FAST_FORWARD = 0x0B3,
    TRANSPORT_REWIND = 0x0B4,
    TRANSPORT_NEXT_TRACK = 0x0B5,
    TRANSPORT_PREV_TRACK = 0x0B6,
    TRANSPORT_STOP = 0x0B7,
    TRANSPORT_EJECT = 0x0B8,
    TRANSPORT_RANDOM_PLAY = 0x0B9,
    TRANSPORT_STOP_EJECT = 0x0CC,
    TRANSPORT_PLAY_PAUSE = 0x0CD,
    // 15.9.1 Audio Controls - Volume
    AUDIO_MUTE = 0x0E2,
    AUDIO_VOL_UP = 0x0E9,
    AUDIO_VOL_DOWN = 0x0EA,
    // 15.15 Application Launch Buttons
    AL_CC_CONFIG = 0x183,
    AL_EMAIL = 0x18A,
    AL_CALCULATOR = 0x192,
    AL_LOCAL_BROWSER = 0x194,
    AL_LOCK = 0x19E,
    AL_CONTROL_PANEL = 0x19F,
    AL_ASSISTANT = 0x1CB,
    AL_KEYBOARD_LAYOUT = 0x1AE,
    // 15.16 Generic GUI Application Controls
    AC_NEW = 0x201,
    AC_OPEN = 0x202,
    AC_CLOSE = 0x203,
    AC_EXIT = 0x204,
    AC_MAXIMIZE = 0x205,
    AC_MINIMIZE = 0x206,
    AC_SAVE = 0x207,
    AC_PRINT = 0x208,
    AC_PROPERTIES = 0x209,
    AC_UNDO = 0x21A,
    AC_COPY = 0x21B,
    AC_CUT = 0x21C,
    AC_PASTE = 0x21D,
    AC_SELECT_ALL = 0x21E,
    AC_FIND = 0x21F,
    AC_SEARCH = 0x221,
    AC_HOME = 0x223,
    AC_BACK = 0x224,
    AC_FORWARD = 0x225,
    AC_STOP = 0x226,
    AC_REFRESH = 0x227,
    AC_BOOKMARKS = 0x22A,
    AC_MISSION_CONTROL = 0x29F,
    AC_LAUNCHPAD = 0x2A0
};
#endif
enum desktop_usages
{
    // 4.5.1 System Controls - Power Controls
    SYSTEM_POWER_DOWN = 0x81,
    SYSTEM_SLEEP = 0x82,
    SYSTEM_WAKE_UP = 0x83,
    SYSTEM_RESTART = 0x8F,
    // 4.10 System Display Controls
    SYSTEM_DISPLAY_TOGGLE_INT_EXT = 0xB5
};


enum lighting_report
{
    REPORT_ID_LIGHTING_LAMP_ARRAY_ATTRIBUTES = 1, 
    REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_REQUEST,
    REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_RESPONSE, 
    REPORT_ID_LIGHTING_LAMP_MULTI_UPDATE,
    REPORT_ID_LIGHTING_LAMP_RANGE_UPDATE,
    REPORT_ID_LIGHTING_LAMP_ARRAY_CONTROL,
    REPORT_ID_COUNT
};

/*
 * keyboard report is 8-byte array retains state of 8 modifiers and 6 keys.
 *
 * byte |0       |1       |2       |3       |4       |5       |6       |7
 * -----+--------+--------+--------+--------+--------+--------+--------+--------
 * desc |mods    |reserved|keys[0] |keys[1] |keys[2] |keys[3] |keys[4] |keys[5]
 *
 * It is exended to 16 bytes to retain 120keys+8mods when NKRO mode.
 *
 * byte |0       |1       |2       |3       |4       |5       |6       |7        ... |15
 * -----+--------+--------+--------+--------+--------+--------+--------+--------     +--------
 * desc |mods    |bits[0] |bits[1] |bits[2] |bits[3] |bits[4] |bits[5] |bits[6]  ... |bit[14]
 *
 * mods retains state of 8 modifiers.
 *
 *  bit |0       |1       |2       |3       |4       |5       |6       |7
 * -----+--------+--------+--------+--------+--------+--------+--------+--------
 * desc |Lcontrol|Lshift  |Lalt    |Lgui    |Rcontrol|Rshift  |Ralt    |Rgui
 *
 */

typedef union
{
    uint8_t raw[68];
    struct
    {
        uint8_t report_id;//@hid_report_ids
        uint8_t modifier_key;
        uint8_t reserved;
        uint8_t keys[6];
    }__attribute__((packed));
    
    struct nkro_report
    {
        uint8_t report_id;
        uint8_t modifier_key;
        uint8_t bits[30];
    }nkro;

    struct 
    {
        uint8_t report_id;
        uint16_t cmd;
    }__attribute__((packed))sys_mod_key;
    
    struct
    {
        uint8_t  report_id;//@hid_report_ids
        struct
        {
            uint8_t  cmd;
            uint8_t  scmd;
            uint16_t len;
            uint16_t reserved;
            uint16_t crc;
            uint8_t  data[56];
        }payload;
    } hive;

    struct 
    {
        uint8_t report_id;
        uint8_t buttons;
        int8_t x;
        int8_t y;
        int8_t wheel;
        int8_t v;
    } __attribute__((packed)) mouse;

} __attribute__((packed)) rep_hid_t;


typedef struct
{
    uint8_t report_id;
    uint16_t usage;
} __attribute__((packed)) report_extra_t;

extern int interface_usb_init(void);

extern bool interface_get_protocol(void);

extern uint8_t interface_keyboard_led(void);

extern void interface_keyboard_send(rep_hid_t *report);

extern void interface_system_send(report_extra_t *report);
extern void interface_consumer_send(report_extra_t *report);

extern void interface_raw_recv(uint8_t *report, uint8_t size);
extern void interface_raw_send(uint8_t *report, uint8_t size);
extern void interface_raw_task(void);

extern bool report_has_anykey(rep_hid_t *report);
extern uint8_t report_get_first_key(rep_hid_t *report);
extern bool report_is_key_pressed(rep_hid_t *report, uint8_t key);

extern void report_add_key_byte(rep_hid_t *report, uint8_t code);
extern void report_del_key_byte(rep_hid_t *report, uint8_t code);

extern void report_add_key(rep_hid_t *report, uint8_t key);
extern void report_del_key(rep_hid_t *report, uint8_t key);
extern void report_clr_key(rep_hid_t *report);
extern bool report_has_key(rep_hid_t *report, uint8_t key);
extern uint8_t report_get_mod_keys(rep_hid_t *report);

extern void report_add_sys_cmd(rep_hid_t *report, uint16_t cmd);
extern void report_del_sys_cmd(rep_hid_t *report, uint16_t cmd);

void report_add_ms_btn(rep_hid_t *report, uint8_t btn);   
void report_del_ms_btn(rep_hid_t *report, uint8_t btn);
void report_set_ms_x(rep_hid_t *report, int8_t val);
void report_set_ms_y(rep_hid_t *report, int8_t val);
void report_set_ms_w(rep_hid_t *report, int8_t val);
void interface_release_report(void);


extern void interface_keyboard_test(void);
#if 0
static inline void report_add_key_bit(rep_hid_t *report, uint8_t code)
{
    if ((code >> 3) < sizeof(report->nkro.bits))
    {
        report->nkro.bits[code >> 3] |= 1 << (code & 7);
    }
    else
    {
        //DBG_PRINTF("can't add nkro key 0x%02x\r\n", code);
    }
}

static inline void report_del_key_bit(rep_hid_t *report, uint8_t code)
{
    if ((code >> 3) < sizeof(report->nkro.bits))
    {
        report->nkro.bits[code >> 3] &= ~(1 << (code & 7));
    }
    else
    {
        //DBG_PRINTF("can't del nkro key 0x%02x\r\n", code);
    }
}
#else
extern void report_add_key_bit(rep_hid_t *report, uint8_t code);
extern void report_del_key_bit(rep_hid_t *report, uint8_t code);

#endif

#endif
