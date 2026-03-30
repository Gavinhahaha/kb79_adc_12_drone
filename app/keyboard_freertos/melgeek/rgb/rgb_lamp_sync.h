/*
 * Copyright (c) 2024-2025 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */



#ifndef RGB_LAMP_SYNC_H
#define RGB_LAMP_SYNC_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "hpm_common.h"
#include "rgb_lamp_array.h"
#include "board.h"

#define AUTONOMOUS_COLOR       (color_t){0, 0, 0}

typedef enum 
{
    LINK_MODE_NONE          = 0,        // 无灯开启
    LINK_MODE_KEY_LED       = (1 << 0), // 轴灯
    LINK_MODE_REAR_AMBIENT  = (1 << 1), // 后氛围灯
    LINK_MODE_SIDE_AMBIENT  = (1 << 2), // 侧氛围灯

    LINK_MODE_KEY_AND_SIDE  = (LINK_MODE_KEY_LED | LINK_MODE_SIDE_AMBIENT),
    LINK_MODE_AMBIENT_ONLY  = (LINK_MODE_REAR_AMBIENT | LINK_MODE_SIDE_AMBIENT),
    LINK_MODE_KEY_AND_REAR  = (LINK_MODE_KEY_LED | LINK_MODE_REAR_AMBIENT),
    LINK_MODE_ALL           = (LINK_MODE_KEY_LED | LINK_MODE_REAR_AMBIENT | LINK_MODE_SIDE_AMBIENT),

} link_mode_t;

typedef enum
{
    FLAG_LAMP_NUNE = 0,
    FLAG_LAMP_UPDATE_ALL = (1 << 0),

} lamp_flags_t;

typedef enum
{
    NONE = 0,
    SYNC,
} sync_effect_t;

typedef struct
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} __attribute__((packed, aligned(1))) color_t;


typedef struct
{
    uint16_t lamp_count;
    uint8_t intensity_Level;
    uint8_t reserve;

    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t lamp_array_kind;
    uint32_t min_update_interval;
    uint32_t timeout;

} __attribute__((packed, aligned(1))) lamp_keyboard_attr_t;

typedef struct
{
    uint16_t lamp_id;

    Position lamp_position;

    uint32_t update_latency;
    uint32_t lamp_purposes;

    uint8_t red_Level_Count;
    uint8_t green_Level_Count;
    uint8_t blue_Level_Count;

    uint8_t is_Programmable;
    uint8_t input_Binding;
} __attribute__((packed, aligned(1))) lamp_sync_attr_t;

typedef struct
{
    link_mode_t link_mode;
    uint16_t    lamp_end;
} __attribute__((packed, aligned(1))) lamp_data_attr_t;

typedef struct
{
    lamp_flags_t flags;
    uint16_t lamp_count;
    uint16_t lamp_id_start;
    color_t colors[17];
} __attribute__((packed, aligned(1))) lamp_multi_update_t;

typedef struct
{
    uint8_t flags;
    uint16_t lamp_id_start;
    uint16_t lamp_id_end;
    color_t color;
} __attribute__((packed, aligned(1))) lamp_part_update_t;

// typedef struct
// {
//     uint8_t mode;
// } __attribute__((packed, aligned(1))) lamp_control_t;

typedef void (*sync_callback)(uint8_t mode, uint8_t state);

void rgb_lamp_sync_autonomous_cb(sync_callback callback);
uint16_t get_lamp_keyboard_attr(uint8_t *buffer);
uint16_t get_lamp_sync_attr(uint16_t lamp_id, uint8_t *buffer);
bool set_lamp_multiple(const uint8_t *buffer);
void set_lamp_part(const uint8_t *buffer);
void set_autonomous_sync_mode(const uint8_t *buffer);

color_t *get_rgb_sync_pixels(void);
uint8_t get_rgb_sync_mode(void);
link_mode_t get_lamp_logic_attr(void);
void set_lamp_logic_attr(link_mode_t mode);

#endif