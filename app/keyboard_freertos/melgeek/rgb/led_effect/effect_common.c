#include "effect_common.h"

bool get_led_location(uint8_t ki, uint8_t *r, uint8_t *c)
{
    for (int i = 0; i < BOARD_LED_ROW_NUM; ++i)
    {
        for (int j = 0; j < BOARD_LED_COLUMN_NUM; ++j)
        {
            if (ki == lookup_key_led_value(i, j))
            {
                *r = i; *c = j;
                return true;
            }
        }
    }
    return false;
}

void do_color_gradual_change(uint8_t *pr, uint8_t *pg, uint8_t *pb, uint8_t max, uint8_t min, uint8_t step)
{
    if (*pr == max && *pg == min)
    {
        *pb = (*pb >= step) ? (*pb - step) : min;
    }

    if (*pr == min && *pg == max)
    {
        *pb = (*pb <= (max - step)) ? (*pb + step) : max;
    }

    if (*pr == max && *pb == min)
    {
        *pg = (*pg <= (max - step)) ? (*pg + step) : max;
    }

    if (*pr == min && *pb == max)
    {
        *pg = (*pg >= step) ? (*pg - step) : min;
    }

    if (*pg == max && *pb == min)
    {
        *pr = (*pr >= step) ? (*pr - step) : min;
    }

    if (*pg == min && *pb == max)
    {
        *pr = (*pr <= (max - step)) ? (*pr + step) : max;
    }
}

void get_random_rgb(uint8_t *pr, uint8_t *pg, uint8_t *pb, uint32_t data)
{
    uint32_t color = 0x0;
    uint8_t t1 = 0, t2 = 0, t3 = 0;

    srand(xTaskGetTickCount() + *pr + *pg + *pb + data);

    t1 = (rand() >> 17) % 16;
    t2 = (rand() >> 14) % 11;
    t3 = (rand() >> 11) % 6;

    srand(xTaskGetTickCount() + *pr + *pg + *pb + data);
    color = rand() & 0xFFFFFF;
    *pr = ~((color >> t1) & 0xFF);

    srand(color + *pr + *pb + ~data);
    color  = rand() & 0xFFFFFF;;
    *pg = (color >> t2) & 0xFF;

    srand(~color + *pr + *pg);
    color  = rand() & 0xFFFFFF;;
    *pb = (color >> t3) & 0xFF;
}