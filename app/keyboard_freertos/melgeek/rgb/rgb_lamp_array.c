/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved.
 *
 * Description:
 *
 */

#include "rgb_lamp_array.h"

typedef struct
{
    lamp_effect current_effect;
    lamp_color starting_color;
    lamp_color pixels[LAMP_RGB_COUNT];
} lamp_controller;

static lamp_controller controller = {0};
static uint16_t current_lamp_id = 0;
static pfn_callback g_rgb_mode_shift_cb = NULL;


void rgb_lamp_array_autonomous_cb(pfn_callback callback)
{
    g_rgb_mode_shift_cb = callback;
}

lamp_color *get_rgb_lamp_rgbi(void)
{
    return (controller.pixels);
}

uint8_t get_rgb_lamp_mode(void)
{
    return (controller.current_effect);
}

uint16_t get_lamp_array_attributes_report(uint8_t **buffer)
{
    lamp_array_attributes_report report = {
        LAMPARRAY_LAMP_COUNT,
        LAMPARRAY_WIDTH,
        LAMPARRAY_HEIGHT,
        LAMPARRAY_DEPTH,
        LAMPARRAY_KIND,
        LAMPARRAY_UPDATE_INTERVAL};

    memcpy(&buffer[0][1], &report, sizeof(lamp_array_attributes_report));

    return sizeof(lamp_array_attributes_report);
}

uint16_t get_lamp_attributes_report(uint8_t **buffer)
{
    lamp_attributes_response_report report = {
        current_lamp_id,                 // LampId
        lamp_positions[current_lamp_id], // Lamp position
        LAMPARRAY_UPDATE_INTERVAL,       // Lamp update interval
        purpose[current_lamp_id],        // Lamp purpose
        255,                             // Red level count
        255,                             // Blue level count
        255,                             // Green level count
        255,                             // Intensity
        1,                               // Is Programmable
        layers[current_lamp_id]          // InputBinding
    };

    memcpy(&buffer[0][1], &report, sizeof(lamp_attributes_response_report));
    current_lamp_id = (uint16_t)(current_lamp_id + 1) >= LAMPARRAY_LAMP_COUNT ? current_lamp_id : current_lamp_id + 1;

    return sizeof(lamp_attributes_response_report);
}

void set_lamp_attributesId(const uint8_t *buffer)
{
    lamp_attributes_request_report *report = (lamp_attributes_request_report *)buffer;
    current_lamp_id = report->lamp_id;
}
/**
 * @description:
 * @param {uint16_t} lamp_id
 * @param {lamp_color} color
 * @return {*}
 */
static void lamp_set_color(uint16_t lamp_id, lamp_color color)
{
    if (lamp_id >= LAMP_RGB_COUNT)
    {
        return;
    }

    controller.pixels[lamp_id] = color;
}

void set_multiple_lamps(const uint8_t *buffer)
{
    lamp_multi_update_report *report = (lamp_multi_update_report *)buffer;

    for (int i = 0; i < report->lamp_count; i++)
    {
        lamp_set_color(report->lamp_ids[i], report->colors[i]);
    }
}


static void lamp_set_color_range(uint16_t lamp_id_start, uint16_t lamp_id_end, lamp_color color)
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

void set_lamp_range(const uint8_t *buffer)
{
    lamp_range_update_report *report = (lamp_range_update_report *)buffer;
    lamp_set_color_range(report->lamp_id_start, report->lamp_id_end, report->color);
}

/**
 * @description: 
 * @param {lamp_effect} effect
 * @param {lamp_color} effect_color
 * @return {*}
 */
static void lamp_set_effect(lamp_effect effect, lamp_color effect_color)
{
    controller.current_effect = effect;
    controller.starting_color = effect_color;

    for (uint16_t i = 0; i < LAMP_RGB_COUNT; i++)
    {
        controller.pixels[i] = effect_color;
    }

    if (g_rgb_mode_shift_cb)
    {
        g_rgb_mode_shift_cb();
    }
}

void set_autonomous_mode(const uint8_t *buffer)
{
    lamp_array_control_report *report = (lamp_array_control_report *)buffer;
    lamp_set_effect(report->autonomousMode ? HID : AUTONOMOUS_LIGHTING_EFFECT, AUTONOMOUS_LIGHTING_COLOR);
}
