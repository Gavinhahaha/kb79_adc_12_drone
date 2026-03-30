/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */


#ifndef RGB_LAMP_ARRAY_H
#define RGB_LAMP_ARRAY_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "hpm_common.h"
#include "board.h"

#define LAMP_PURPOSE_CONTROL        0x01
#define LAMP_PURPOSE_ACCENT         0x02
#define LAMP_PURPOSE_BRANDING       0x04
#define LAMP_PURPOSE_STATUS         0x08
#define LAMP_PURPOSE_ILLUMINATION   0x10
#define LAMP_PURPOSE_PRESENTATION   0x20
#define LAMP_PURPOSE_ACCENT_SIDE    0x10000


#define LAMPARRAY_UPDATE_TIMEOUT    3000000                 // timeout, in us
#define LAMPARRAY_UPDATE_INTERVAL   20000                   // 10 ms update interval, in us
#define LAMP_RGB_COUNT              (BOARD_KEY_LED_NUM      +  BOARD_LIGHT_TAPE_LED_NUM)

//      lamp_array                   Attributes
#define LAMPARRAY_LAMP_COUNT        LAMP_RGB_COUNT
#define LAMPARRAY_WIDTH             375680
#define LAMPARRAY_HEIGHT            148600
#define LAMPARRAY_DEPTH             37496 
#define LAMPARRAY_KIND              1                     // LampArrayKindKeyboard
#define AUTONOMOUS_LIGHTING_COLOR   (lamp_color){0, 0, 0, 0}
#define AUTONOMOUS_LIGHTING_EFFECT  BLINK

typedef enum
{
    HID,
    BLINK,

} lamp_effect;

typedef struct
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t intensity;
} __attribute__((packed, aligned(1))) lamp_color;

typedef struct
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
} __attribute__((packed, aligned(1))) Position;

typedef struct
{
    uint16_t lamp_count;

    uint32_t width;
    uint32_t height;
    uint32_t depth;

    uint32_t lamp_array_kind;
    uint32_t min_update_interval;
} __attribute__((packed, aligned(1))) lamp_array_attributes_report;

typedef struct
{
    uint16_t lamp_id;
} __attribute__((packed, aligned(1))) lamp_attributes_request_report;

typedef struct
{
    uint16_t lamp_id;

    Position lamp_position;

    uint32_t update_latency;
    uint32_t lamp_purposes;

    uint8_t red_Level_Count;
    uint8_t green_Level_Count;
    uint8_t blue_Level_Count;
    uint8_t intensity_Level_Count;

    uint8_t is_Programmable;
    uint8_t input_Binding;
} __attribute__((packed, aligned(1))) lamp_attributes_response_report;

typedef struct
{
    uint8_t lamp_count;
    uint8_t flags;
    uint16_t lamp_ids[8];

    lamp_color colors[8];
} __attribute__((packed, aligned(1))) lamp_multi_update_report;

typedef struct
{
    uint8_t flags;
    uint16_t lamp_id_start;
    uint16_t lamp_id_end;

    lamp_color color;
} __attribute__((packed, aligned(1))) lamp_range_update_report;

typedef struct
{
    uint8_t autonomousMode;
} __attribute__((packed, aligned(1))) lamp_array_control_report;



// Lamp Attributes
static const Position lamp_positions[LAMP_RGB_COUNT] =
    {
        {33507, 28800, 18000},
        {66717, 28800, 18000},
        {85790, 28800, 18000},
        {105020, 28800, 18000},
        {123700, 28800, 18000},
        {152430, 28800, 18000},
        {171722, 28800, 18000},
        {190720, 28800, 18000},
        {209870, 28800, 18000},
        {238140, 28800, 18000},
        {257160, 28800, 18000},
        {276600, 28800, 18000},
        {295510, 28800, 18000},
        {323870, 28800, 18000},
        {342780, 28800, 18000},
        {33610, 52680, 19000},
        {52350, 52680, 19000},
        {71100, 52680, 19000},
        {90490, 52680, 19000},
        {109560, 52680, 19000},
        {128650, 52680, 19000},
        {147370, 52680, 19000},
        {166444, 52680, 19000},
        {185830, 52680, 19000},
        {204100, 52680, 19000},
        {223970, 52680, 19000},
        {243040, 52680, 19000},
        {261040, 52680, 19000},
        {290545, 52680, 19000},
        {37813, 71652, 20000},
        {61729, 71652, 20000},
        {80798, 71652, 20000},
        {99866, 71652, 20000},
        {118934, 71652, 20000},
        {138328, 71652, 20000},
        {157071, 71652, 20000},
        {176463, 71652, 20000},
        {195208, 71652, 20000},
        {214599, 71652, 20000},
        {233344, 71652, 20000},
        {252734, 71652, 20000},
        {271353, 71652, 20000},
        {295138, 71652, 20000},
        {40722, 90498, 21000},
        {66900, 90498, 21000},
        {85645, 90498, 21000},
        {105023, 90498, 21000},
        {124108, 90498, 21000},
        {143178, 90498, 21000},
        {161929, 90498, 21000},
        {180989, 90498, 21000},
        {200056, 90498, 21000},
        {219122, 90498, 21000},
        {238190, 90498, 21000},
        {256933, 90498, 21000},
        {287934, 90498, 21000},
        {45246, 109885, 22000},
        {75960, 109885, 22000},
        {95340, 109885, 22000},
        {114088, 109885, 22000},
        {133478, 109885, 22000},
        {152546, 109885, 22000},
        {171297, 109885, 22000},
        {190360, 109885, 22000},
        {209418, 109885, 22000},
        {228820, 109885, 22000},
        {247060, 109885, 22000},
        {283116, 109885, 22000},
        {324161, 109885, 22000},
        {35555, 128632, 23000},
        {59464, 128632, 23000},
        {83060, 128632, 23000},
        {130740, 123709, 23000},
        {155130, 128632, 23000},
        {178758, 123709, 23000},
        {225588, 128632, 23000},
        {250150, 128632, 23000},
        {273740, 128632, 23000},
        {304770, 128632, 23000},
        {324161, 128632, 23000},
        {342907, 128632, 23000},

        {344051, 32768, 21368},
        {336440, 32768, 21368},
        {328829, 32768, 21368},
        {321218, 32768, 21368},
        {313607, 32768, 21368},
        {305996, 32768, 21368},
        {298385, 32768, 21368},
        {290774, 32768, 21368},
        {283163, 32768, 21368},
        {275552, 32768, 21368},
        {267941, 32768, 21368},
        {260330, 32768, 21368},
        {252719, 32768, 21368},
        {245108, 32768, 21368},
        {237497, 32768, 21368},
        {229886, 32768, 21368},
        {222275, 32768, 21368},
        {214664, 32768, 21368},
        {207053, 32768, 21368},
        {199442, 32768, 21368},
        {191831, 32768, 21368},
        {184220, 32768, 21368},
        {176609, 32768, 21368},
        {168998, 32768, 21368},
        {161387, 32768, 21368},
        {153776, 32768, 21368},
        {146165, 32768, 21368},
        {138554, 32768, 21368},
        {130943, 32768, 21368},
        {123332, 32768, 21368},
        {115721, 32768, 21368},
        {108110, 32768, 21368},
        {100499, 32768, 21368},
        {92888,  32768, 21368},
        {85277,  32768, 21368},
        {77666,  32768, 21368},
        {70055,  32768, 21368},
        {62444,  32768, 21368},
        {54833,  32768, 21368},
        {47222,  32768, 21368},
        {39611,  32768, 21368},
        {22000,  32768, 21368},

        {20190, 10047,  20010},
        {16111, 14310,  20574},
        {11848, 18433,  21148},
        {10143, 23609,  21722},
        {10143, 29606,  22296},
        {10143, 35603,  22870},
        {10143, 41600,  23444},
        {10143, 47597,  24018},
        {10143, 53594,  24592},
        {10143, 59591,  25166},
        {10143, 65588,  25740},
        {10143, 71585,  26314},
        {10143, 77582,  26888},
        {10143, 83579,  27462},
        {10143, 89576,  28036},
        {10143, 95573,  28610},
        {10143, 101570, 29184},
        {10143, 107567, 29758},
        {10143, 113564, 30332},
        {10143, 119561, 30906},
        {11989, 124774, 31480},
        {16082, 129100, 32054},
        {20254, 133550, 32628},

        {355438, 133428, 32628},
        {359577, 128922, 32054},
        {363592, 124771, 31480},
        {365418, 119561, 30906},
        {365418, 113564, 30332},
        {365418, 107567, 29758},
        {365418, 101570, 29184},
        {365418, 95573,  28610},
        {365418, 89576,  28036},
        {365418, 83579,  27462},
        {365418, 77582,  26888},
        {365418, 71585,  26314},
        {365418, 65588,  25740},
        {365418, 59591,  25166},
        {365418, 53594,  24592},
        {365418, 47597,  24018},
        {365418, 41600,  23444},
        {365418, 35603,  22870},
        {365418, 29606,  22296},
        {365418, 23609,  21722},
        {363639, 18433,  21148},
        {359514, 14310,  20574},
        {355490, 10047,  20010},

};

static const uint8_t layers[LAMP_RGB_COUNT] =
{
    0x29, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x4c,
    0x35, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x2d, 0x2e, 0x2a, 
    0x2b, 0x14, 0x1a, 0x08, 0x15, 0x17, 0x1c, 0x18, 0x0c, 0x12, 0x13, 0x2f, 0x30, 0x31, 
    0x39, 0x04, 0x16, 0x07, 0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x0f, 0x33, 0x34, 0x28, 
    0xe1, 0x1d, 0x1b, 0x06, 0x19, 0x05, 0x11, 0x10, 0x36, 0x37, 0x38, 0xe5, 0x52, 
    0xe0, 0xe3, 0xe2, 0x00, 0x2c, 0x00, 0xe6, 0x00, 0xe4, 0x50, 0x51, 0x4f, 
};

static const uint32_t purpose[LAMP_RGB_COUNT] =
    {
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_CONTROL,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
        LAMP_PURPOSE_ACCENT_SIDE,
};

typedef void (*pfn_callback)(void);

// Get Reports
uint16_t get_lamp_array_attributes_report(uint8_t** buffer);
uint16_t get_lamp_attributes_report(uint8_t** buffer);

// Set Reports
void set_lamp_attributesId(const uint8_t* buffer);
void set_multiple_lamps(const uint8_t* buffer);
void set_lamp_range(const uint8_t* buffer);
void set_autonomous_mode(const uint8_t* buffer);

lamp_color *get_rgb_lamp_rgbi(void);
uint8_t get_rgb_lamp_mode(void);
void rgb_lamp_array_autonomous_cb(pfn_callback callback);

#endif