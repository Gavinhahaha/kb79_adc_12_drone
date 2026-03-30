/*
 * Copyright (c) 2024-2025 Melgeek, All Rights Reserved.
 *
 * Description:
 *
 */

#include "rgb_lamp_sync.h"
#include "FreeRTOS.h"
#include "timers.h"

typedef struct
{
    lamp_effect current_effect;
    color_t starting_color;
    color_t pixels[LAMP_RGB_COUNT];
} lamp_controller;

static TimerHandle_t timer_handle = NULL;

static lamp_controller controller = {0};
static lamp_data_attr_t data_attr = {LINK_MODE_ALL, LAMPARRAY_LAMP_COUNT};
static sync_callback g_rgb_mode_shift_cb = NULL;

void rgb_lamp_sync_autonomous_cb(sync_callback callback)
{
    g_rgb_mode_shift_cb = callback;
}

color_t *get_rgb_sync_pixels(void)
{
    return (controller.pixels);
}

uint8_t get_rgb_sync_mode(void)
{
    return (controller.current_effect);
}

link_mode_t get_lamp_logic_attr(void)
{
    return (data_attr.link_mode);
}

void set_lamp_logic_attr(link_mode_t mode)
{
    data_attr.link_mode = mode;

    // data_attr.lamp_end =
    //     ((data_attr.link_mode & LINK_MODE_KEY_LED) ? BOARD_KEY_LED_NUM : 0) +
    //     ((data_attr.link_mode & LINK_MODE_REAR_AMBIENT) ? BOARD_LIGHT_TAPE_BACK_LED_NUM : 0) +
    //     ((data_attr.link_mode & LINK_MODE_SIDE_AMBIENT) ? BOARD_LIGHT_TAPE_SIDE_LED_NUM : 0);
    if ((data_attr.link_mode & LINK_MODE_SIDE_AMBIENT))
    {
        data_attr.lamp_end = BOARD_LIGHT_TAPE_SIDE_LED_NUM + BOARD_LIGHT_TAPE_BACK_LED_NUM + BOARD_KEY_LED_NUM;
    }
    else if ((data_attr.link_mode & LINK_MODE_REAR_AMBIENT))
    {
        data_attr.lamp_end = BOARD_LIGHT_TAPE_BACK_LED_NUM + BOARD_KEY_LED_NUM;
    }
    else if ((data_attr.link_mode & LINK_MODE_KEY_LED))
    {
        data_attr.lamp_end = BOARD_KEY_LED_NUM;
    }

    if (g_rgb_mode_shift_cb)
    {
        g_rgb_mode_shift_cb(controller.current_effect, mode);
    }
}

uint16_t get_lamp_keyboard_attr(uint8_t *buffer)
{
    lamp_keyboard_attr_t report = {
        LAMPARRAY_LAMP_COUNT,
        0xFF,
        255,
        LAMPARRAY_WIDTH,
        LAMPARRAY_HEIGHT,
        LAMPARRAY_DEPTH,
        LAMPARRAY_KIND,
        LAMPARRAY_UPDATE_INTERVAL,
        LAMPARRAY_UPDATE_TIMEOUT};

    memcpy(&buffer[0], &report, sizeof(lamp_keyboard_attr_t));

    return sizeof(lamp_keyboard_attr_t);
}

uint16_t get_lamp_sync_attr(uint16_t lamp_id, uint8_t *buffer)
{

    uint16_t id = ((uint16_t)(lamp_id + 1) >= LAMPARRAY_LAMP_COUNT) ? (LAMPARRAY_LAMP_COUNT - 1) : lamp_id;

    lamp_sync_attr_t report = {
        id,                        // LampId
        lamp_positions[id],        // Lamp position
        LAMPARRAY_UPDATE_INTERVAL, // Lamp update interval
        purpose[id],               // Lamp purpose
        255,                       // Red level count
        255,                       // Blue level count
        255,                       // Green level count
        1,                         // Is Programmable
        layers[id]                 // InputBinding
    };

    memcpy(&buffer[0], &report, sizeof(lamp_sync_attr_t));

    return sizeof(lamp_sync_attr_t);
}


// void set_lamp_attributesId(const uint8_t *buffer)
// {
//     lamp_attributes_request_report *report = (lamp_attributes_request_report *)buffer;
//     current_lamp_id = report->lamp_id;
// }
/**
 * @description:
 * @param {uint16_t} lamp_id
 * @param {color_t} color
 * @return {*}
 */
static void lamp_set_color(uint16_t lamp_id, color_t color)
{
    if (lamp_id >= LAMP_RGB_COUNT)
    {
        return;
    }

    controller.pixels[lamp_id] = color;

    if (timer_handle != NULL)
    {
        xTimerReset(timer_handle, 0); // Reset the timer to ensure the effect continues without interruption
    }
}

bool set_lamp_multiple(const uint8_t *buffer)
{
    lamp_multi_update_t *report = (lamp_multi_update_t *)buffer;

    for (int i = 0; i < report->lamp_count; i++)
    {
        lamp_set_color(report->lamp_id_start + i, report->colors[i]);
    }

    if ((report->lamp_id_start + report->lamp_count >= data_attr.lamp_end) && (report->flags & FLAG_LAMP_UPDATE_ALL))
    {
        // todo : creat backup buffer, we can consider this a full update.
        return true;
    }
    return false;
}


static void lamp_set_color_part(uint16_t lamp_id_start, uint16_t lamp_id_end, color_t color)
{
    if (lamp_id_start >= LAMP_RGB_COUNT)
    {
        return;
    }

    for (uint16_t i = lamp_id_start; i <= lamp_id_end && i < LAMP_RGB_COUNT; i++)
    {
        lamp_set_color(i, color);
    }
}

void set_lamp_part(const uint8_t *buffer)
{
    lamp_part_update_t *report = (lamp_part_update_t *)buffer;
    lamp_set_color_part(report->lamp_id_start, report->lamp_id_end, report->color);
}

/**
 * @description:
 * @param {lamp_control_t} effect
 * @param {color_t} effect_color
 * @return {*}
 */
static void lamp_set_effect(sync_effect_t effect, color_t effect_color)
{
    controller.current_effect = (lamp_effect)effect;
    controller.starting_color = effect_color;

    for (uint16_t i = 0; i < LAMP_RGB_COUNT; i++)
    {
        controller.pixels[i] = effect_color;
    }

    if (g_rgb_mode_shift_cb)
    {
        g_rgb_mode_shift_cb(controller.current_effect, data_attr.link_mode);
    }
}

static void timer_callback(TimerHandle_t xTimer)
{
    (void)xTimer;
    if (g_rgb_mode_shift_cb)
    {
        g_rgb_mode_shift_cb(HID, data_attr.link_mode);
    }
}

static void lamp_set_effect_trace(sync_effect_t effect)
{
    if (controller.current_effect == BLINK)
    {
        if (timer_handle == NULL)
        {
            timer_handle = xTimerCreate("Timer_sync", pdMS_TO_TICKS(LAMPARRAY_UPDATE_TIMEOUT/1000), pdFALSE, 0, timer_callback);
            if (timer_handle == NULL)
            {
                // DBG_PRINTF("RGB timer creation failure\r\n");
            }
        }
        if (timer_handle != NULL)
        {
            xTimerStart(timer_handle, 0);
        }
    }
    else
    {
        if (timer_handle != NULL)
        {
            xTimerStop(timer_handle, 0);
        }
    }
}

void set_autonomous_sync_mode(const uint8_t *buffer)
{
    sync_effect_t report = *(sync_effect_t *)buffer;
    lamp_set_effect(report, AUTONOMOUS_COLOR);
    lamp_set_effect_trace(report);
}
