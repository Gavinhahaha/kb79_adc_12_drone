#ifndef _RGB_LED_COLOR_H_
#define _RGB_LED_COLOR_H_

#include <stdint.h>
#include "rgb_led_table.h"

#define MODE_MASK         0x00

#define BLACK       ((MODE_MASK << 24) | 0x000000)
#define RED         ((MODE_MASK << 24) | 0xFF0000)
#define GREEN       ((MODE_MASK << 24) | 0x00FF00)
#define BLUE        ((MODE_MASK << 24) | 0x0000FF)
#define YELLOW      ((MODE_MASK << 24) | 0xFFFF00)
#define WHITE       ((MODE_MASK << 24) | 0xFFFFFF)
#define PURPLE      ((MODE_MASK << 24) | 0xFF00FF)
#define CYAN        ((MODE_MASK << 24) | 0x00FFFF)
#define ORINGE      ((MODE_MASK << 24) | 0xFF5000)



uint32_t set_key_led_color(uint8_t id, uint8_t l, uint8_t r, uint8_t g, uint8_t b);
void set_all_key_led_color(uint8_t l, uint8_t r, uint8_t g, uint8_t b);
const uint32_t *get_rgb_layer_color(void);
void set_rgb_from_hex(uint32_t color, key_led_color_t *pkr);
void set_key_led_color_mask(uint8_t id, uint8_t krm, uint8_t flag);
#endif