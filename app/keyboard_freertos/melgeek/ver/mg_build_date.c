/*
 * Copyright (c) 2024-2025 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#include "mg_build_date.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "hpm_gpio_drv.h"
#include "board.h"


static const uint8_t mouth_str[12][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
};

void mg_get_yyyymmddhhmmss(uint8_t *pdate)
{
    int y = 0, m = 0, d = 0;
    int h = 0, m1 = 0, s = 0;
    uint8_t buf[4] = {0};

    if (pdate == NULL) 
    {
        return;
    }

    sscanf(__DATE__, "%3s %2d %4d", buf, &d, &y);
    sscanf(__TIME__, "%2d:%2d:%2d", &h, &m1, &s);

    for (uint8_t i = 0; i < 12; ++i)
    {
        if (0 == strcmp((const char *)mouth_str[i], (const char *)buf))
        {
            m = i + 1;
            break;
        }
    }

    sprintf((char *)pdate, "%04d%02d%02d%02d%02d%02d", y, m, d, h, m1, s);

    for (uint8_t i = 0; i < 14; i++)
    {
        pdate[i] -= 0x30;
    }
}
