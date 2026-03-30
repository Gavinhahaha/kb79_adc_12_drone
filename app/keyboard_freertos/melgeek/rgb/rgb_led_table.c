/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include "rgb_led_table.h"
#include "effect_common.h"

#if BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_AW20XXX
#include "drv_aw20xxx.h"
        static const is31_led led_ctrl_table[BOARD_LED_NUM] = {

        { 0, AW20216_CS_SW(1, 1), AW20216_CS_SW(2, 1), AW20216_CS_SW(3, 1), },  /* RGB9 */
        { 0, AW20216_CS_SW(1, 2), AW20216_CS_SW(2, 2), AW20216_CS_SW(3, 2), },  /* RGB10 */
        { 0, AW20216_CS_SW(1, 3), AW20216_CS_SW(2, 3), AW20216_CS_SW(3, 3), },  /* RGB2 */
        { 0, AW20216_CS_SW(1, 4), AW20216_CS_SW(2, 4), AW20216_CS_SW(3, 4), },  /* RGB3 */
        { 0, AW20216_CS_SW(1, 5), AW20216_CS_SW(2, 5), AW20216_CS_SW(3, 5), },  /* RGB4 */
        { 0, AW20216_CS_SW(1, 6), AW20216_CS_SW(2, 6), AW20216_CS_SW(3, 6), },  /* RGB5 */
        { 0, AW20216_CS_SW(1, 7), AW20216_CS_SW(2, 7), AW20216_CS_SW(3, 7), },  /* RGB1 */
        { 0, AW20216_CS_SW(1, 8), AW20216_CS_SW(2, 8), AW20216_CS_SW(3, 8), },  /* RGB11 */
        { 0, AW20216_CS_SW(1, 9), AW20216_CS_SW(2, 9), AW20216_CS_SW(3, 9), },  /* RGB12 */
        { 0, AW20216_CS_SW(1, 10), AW20216_CS_SW(2, 10), AW20216_CS_SW(3, 10), },  /* RGB6 */
        { 0, AW20216_CS_SW(1, 11), AW20216_CS_SW(2, 11), AW20216_CS_SW(3, 11), },  /* RGB7 */
        { 0, AW20216_CS_SW(1, 12), AW20216_CS_SW(2, 12), AW20216_CS_SW(3, 12), },  /* RGB8 */
        { 0, AW20216_CS_SW(16, 7), AW20216_CS_SW(17, 7), AW20216_CS_SW(18, 7), },  /* RGB67 */
        { 0, AW20216_CS_SW(16, 8), AW20216_CS_SW(17, 8), AW20216_CS_SW(18, 8), },  /* RGB68 */
        { 0, AW20216_CS_SW(16, 9), AW20216_CS_SW(17, 9), AW20216_CS_SW(18, 9), },  /* RGB69 */

        { 0, AW20216_CS_SW(4, 1), AW20216_CS_SW(5, 1), AW20216_CS_SW(6, 1), },  /* RGB13 */
        { 0, AW20216_CS_SW(4, 2), AW20216_CS_SW(5, 2), AW20216_CS_SW(6, 2), },  /* RGB16 */
        { 0, AW20216_CS_SW(4, 3), AW20216_CS_SW(5, 3), AW20216_CS_SW(6, 3), },  /* RGB17 */
        { 0, AW20216_CS_SW(4, 4), AW20216_CS_SW(5, 4), AW20216_CS_SW(6, 4), },  /* RGB18 */
        { 0, AW20216_CS_SW(4, 5), AW20216_CS_SW(5, 5), AW20216_CS_SW(6, 5), },  /* RGB14 */
        { 0, AW20216_CS_SW(4, 6), AW20216_CS_SW(5, 6), AW20216_CS_SW(6, 6), },  /* RGB19 */
        { 0, AW20216_CS_SW(4, 7), AW20216_CS_SW(5, 7), AW20216_CS_SW(6, 7), },  /* RGB20 */
        { 0, AW20216_CS_SW(4, 8), AW20216_CS_SW(5, 8), AW20216_CS_SW(6, 8), },  /* RGB15 */
        { 0, AW20216_CS_SW(4, 9), AW20216_CS_SW(5, 9), AW20216_CS_SW(6, 9), },  /* RGB21 */
        { 0, AW20216_CS_SW(4, 10), AW20216_CS_SW(5, 10), AW20216_CS_SW(6, 10), },  /* RGB22 */
        { 0, AW20216_CS_SW(4, 11), AW20216_CS_SW(5, 11), AW20216_CS_SW(6, 11), },  /* RGB23 */
        { 0, AW20216_CS_SW(4, 12), AW20216_CS_SW(5, 12), AW20216_CS_SW(6, 12), },  /* RGB24 */
        { 0, AW20216_CS_SW(16, 12), AW20216_CS_SW(17, 12), AW20216_CS_SW(18, 12), },  /* RGB63 */
        { 0, AW20216_CS_SW(16, 11), AW20216_CS_SW(17, 11), AW20216_CS_SW(18, 11), },  /* RGB62 */
        { 0, AW20216_CS_SW(16, 10), AW20216_CS_SW(17, 10), AW20216_CS_SW(18, 10), },  /* RGB61 */

        { 0, AW20216_CS_SW(7, 1),  AW20216_CS_SW(8, 1),  AW20216_CS_SW(9, 1),  },  /* RGB25 */
        { 0, AW20216_CS_SW(7, 2),  AW20216_CS_SW(8, 2),  AW20216_CS_SW(9, 2),  },  /* RGB26 */
        { 0, AW20216_CS_SW(7, 3),  AW20216_CS_SW(8, 3),  AW20216_CS_SW(9, 3),  },  /* RGB27 */
        { 0, AW20216_CS_SW(7, 4),  AW20216_CS_SW(8, 4),  AW20216_CS_SW(9, 4),  },  /* RGB28 */
        { 0, AW20216_CS_SW(7, 5),  AW20216_CS_SW(8, 5),  AW20216_CS_SW(9, 5),  },  /* RGB29 */
        { 0, AW20216_CS_SW(7, 6),  AW20216_CS_SW(8, 6),  AW20216_CS_SW(9, 6),  },  /* RGB30 */
        { 0, AW20216_CS_SW(7, 7),  AW20216_CS_SW(8, 7),  AW20216_CS_SW(9, 7),  },  /* RGB31 */
        { 0, AW20216_CS_SW(7, 8),  AW20216_CS_SW(8, 8),  AW20216_CS_SW(9, 8),  },  /* RGB32 */
        { 0, AW20216_CS_SW(7, 9),  AW20216_CS_SW(8, 9),  AW20216_CS_SW(9, 9),  },  /* RGB33 */
        { 0, AW20216_CS_SW(7, 10), AW20216_CS_SW(8, 10), AW20216_CS_SW(9, 10), },  /* RGB34 */
        { 0, AW20216_CS_SW(7, 11), AW20216_CS_SW(8, 11), AW20216_CS_SW(9, 11), },  /* RGB35 */
        { 0, AW20216_CS_SW(7, 12), AW20216_CS_SW(8, 12), AW20216_CS_SW(9, 12), },  /* RGB36 */
        { 0, AW20216_CS_SW(16, 5), AW20216_CS_SW(17, 5), AW20216_CS_SW(18, 5), },  /* RGB71 */

        { 0, AW20216_CS_SW(10, 1),  AW20216_CS_SW(11, 1),  AW20216_CS_SW(12, 1),  },  /* RGB37 */
        { 0, AW20216_CS_SW(10, 2),  AW20216_CS_SW(11, 2),  AW20216_CS_SW(12, 2),  },  /* RGB38 */
        { 0, AW20216_CS_SW(10, 3),  AW20216_CS_SW(11, 3),  AW20216_CS_SW(12, 3),  },  /* RGB39 */
        { 0, AW20216_CS_SW(10, 4),  AW20216_CS_SW(11, 4),  AW20216_CS_SW(12, 4),  },  /* RGB40 */
        { 0, AW20216_CS_SW(10, 5),  AW20216_CS_SW(11, 5),  AW20216_CS_SW(12, 5),  },  /* RGB41 */
        { 0, AW20216_CS_SW(10, 6),  AW20216_CS_SW(11, 6),  AW20216_CS_SW(12, 6),  },  /* RGB42 */
        { 0, AW20216_CS_SW(10, 7),  AW20216_CS_SW(11, 7),  AW20216_CS_SW(12, 7),  },  /* RGB43 */
        { 0, AW20216_CS_SW(10, 8),  AW20216_CS_SW(11, 8),  AW20216_CS_SW(12, 8),  },  /* RGB44 */
        { 0, AW20216_CS_SW(10, 9),  AW20216_CS_SW(11, 9),  AW20216_CS_SW(12, 9),  },  /* RGB45 */
        { 0, AW20216_CS_SW(10, 10), AW20216_CS_SW(11, 10), AW20216_CS_SW(12, 10), },  /* RGB46 */
        { 0, AW20216_CS_SW(10, 11),  AW20216_CS_SW(11, 11),  AW20216_CS_SW(12, 11),  },  /* RGB47 */
        { 0, AW20216_CS_SW(10, 12),  AW20216_CS_SW(11, 12),  AW20216_CS_SW(12, 12),  },  /* RGB48 */
        { 0, AW20216_CS_SW(16, 6),  AW20216_CS_SW(17, 6),  AW20216_CS_SW(18, 6),  },  /* RGB70 */

        { 0, AW20216_CS_SW(13, 1),  AW20216_CS_SW(14, 1),  AW20216_CS_SW(15, 1),  },  /* RGB54 */
        { 0, AW20216_CS_SW(13, 2),  AW20216_CS_SW(14, 2),  AW20216_CS_SW(15, 2),  },  /* RGB55 */
        { 0, AW20216_CS_SW(13, 3),  AW20216_CS_SW(14, 3),  AW20216_CS_SW(15, 3),  },  /* RGB49 */
        { 0, AW20216_CS_SW(13, 4),  AW20216_CS_SW(14, 4),  AW20216_CS_SW(15, 4),  },  /* RGB50 */
        { 0, AW20216_CS_SW(13, 5),  AW20216_CS_SW(14, 5),  AW20216_CS_SW(15, 5),  },  /* RGB56 */
        { 0, AW20216_CS_SW(13, 6),  AW20216_CS_SW(14, 6),  AW20216_CS_SW(15, 6),  },  /* RGB51 */
        { 0, AW20216_CS_SW(13, 7),  AW20216_CS_SW(14, 7),  AW20216_CS_SW(15, 7),  },  /* RGB52 */
        { 0, AW20216_CS_SW(13, 8),  AW20216_CS_SW(14, 8),  AW20216_CS_SW(15, 8),  },  /* RGB57 */
        { 0, AW20216_CS_SW(13, 9),  AW20216_CS_SW(14, 9),  AW20216_CS_SW(15, 9),  },  /* RGB58 */
        { 0, AW20216_CS_SW(13, 10),  AW20216_CS_SW(14, 10),  AW20216_CS_SW(15, 10),  },  /* RGB53 */
        { 0, AW20216_CS_SW(13, 11),  AW20216_CS_SW(14, 11),  AW20216_CS_SW(15, 11),  },  /* RGB59 */
        { 0, AW20216_CS_SW(13, 12),  AW20216_CS_SW(14, 12),  AW20216_CS_SW(15, 12),  },  /* RGB60 */

        { 0, AW20216_CS_SW(16, 1),  AW20216_CS_SW(17, 1),  AW20216_CS_SW(18, 1),  },  /* RGB64 */
        { 0, AW20216_CS_SW(16, 2),  AW20216_CS_SW(17, 2),  AW20216_CS_SW(18, 2),  },  /* RGB65 */
        { 0, AW20216_CS_SW(16, 3),  AW20216_CS_SW(17, 3),  AW20216_CS_SW(18, 3),  },  /* RGB66 */

    };


#elif BOARD_LED_DRV_IC_MODEL == LED_DRV_IC_IS37XX

    #ifdef IS3741
    static const is31_led led_ctrl_table[BOARD_LED_NUM] = {
        {0, CS9_SW1, CS8_SW1 , CS7_SW1,},  /* RGB1 */
        {0, CS9_SW2, CS8_SW2 , CS7_SW2,},  /* RGB2 */
        {0, CS9_SW3, CS8_SW3 , CS7_SW3,},  /* RGB3 */
        {0, CS9_SW4, CS8_SW4 , CS7_SW4,},  /* RGB4 */
        {0, CS9_SW5, CS8_SW5 , CS7_SW5,},  /* RGB5 */
        {0, CS9_SW6, CS8_SW6 , CS7_SW6,},  /* RGB6 */
        {0, CS9_SW7, CS8_SW7 , CS7_SW7,},  /* RGB7 */
        {0, CS9_SW8, CS8_SW8 , CS7_SW8,},  /* RGB8 */
        {0, CS9_SW9, CS8_SW9 , CS7_SW9,},  /* RGB9 */
        {0, CS24_SW1,CS23_SW1, CS22_SW1}, /* RGB46 */
        {0, CS24_SW2,CS23_SW2, CS22_SW2}, /* RGB47 */
        {0, CS24_SW3,CS23_SW3, CS22_SW3}, /* RGB48 */
        {0, CS24_SW4,CS23_SW4, CS22_SW4}, /* RGB49 */
        {0, CS24_SW5,CS23_SW5, CS22_SW5}, /* RGB50 */
        {0, CS24_SW6,CS23_SW6, CS22_SW6}, /* RGB51 */
        {0, CS12_SW1,CS11_SW1, CS10_SW1}, /* RGB10 */
        {0, CS12_SW2,CS11_SW2, CS10_SW2}, /* RGB11 */
        {0, CS12_SW3,CS11_SW3, CS10_SW3}, /* RGB12 */
        {0, CS12_SW4,CS11_SW4, CS10_SW4}, /* RGB13 */
        {0, CS12_SW5,CS11_SW5, CS10_SW5}, /* RGB14 */
        {0, CS12_SW6,CS11_SW6, CS10_SW6}, /* RGB15 */
        {0, CS12_SW7,CS11_SW7, CS10_SW7}, /* RGB16 */
        {0, CS12_SW8,CS11_SW8, CS10_SW8}, /* RGB17 */
        {0, CS12_SW9,CS11_SW9, CS10_SW9}, /* RGB18 */
        {0, CS24_SW7,CS23_SW7, CS22_SW7}, /* RGB52 */
        {0, CS24_SW8,CS23_SW8, CS22_SW8}, /* RGB53 */
        {0, CS24_SW9,CS23_SW9, CS22_SW9}, /* RGB54 */
        {0, CS27_SW1,CS26_SW1, CS25_SW1}, /* RGB65 */
        {0, CS27_SW2,CS26_SW2, CS25_SW2}, /* RGB56 */
        {0, CS27_SW3,CS26_SW3, CS25_SW3}, /* RGB57 */
        {0, CS15_SW1,CS14_SW1, CS13_SW1}, /* RGB19 */
        {0, CS15_SW2,CS14_SW2, CS13_SW2}, /* RGB20 */
        {0, CS15_SW3,CS14_SW3, CS13_SW3}, /* RGB21 */
        {0, CS15_SW4,CS14_SW4, CS13_SW4}, /* RGB22 */
        {0, CS15_SW5,CS14_SW5, CS13_SW5}, /* RGB23 */
        {0, CS15_SW6,CS14_SW6, CS13_SW6}, /* RGB24 */
        {0, CS15_SW7,CS14_SW7, CS13_SW7}, /* RGB25 */
        {0, CS15_SW8,CS14_SW8, CS13_SW8}, /* RGB26 */
        {0, CS15_SW9,CS14_SW9, CS13_SW9}, /* RGB27 */
        {0, CS27_SW4,CS26_SW4, CS25_SW4}, /* RGB58 */
        {0, CS27_SW5,CS26_SW5, CS25_SW5}, /* RGB59 */
        {0, CS27_SW6,CS26_SW6, CS25_SW6}, /* RGB67 */
        {0, CS27_SW7,CS26_SW7, CS25_SW7}, /* RGB68 */
        {0, CS27_SW8,CS26_SW8, CS25_SW8}, /* RGB69 */
        {0, CS18_SW1,CS17_SW1, CS16_SW1}, /* RGB28 */
        {0, CS18_SW2,CS17_SW2, CS16_SW2}, /* RGB29 */
        {0, CS18_SW3,CS17_SW3, CS16_SW3}, /* RGB30 */
        {0, CS18_SW4,CS17_SW4, CS16_SW4}, /* RGB31 */
        {0, CS18_SW5,CS17_SW5, CS16_SW5}, /* RGB32 */
        {0, CS18_SW6,CS17_SW6, CS16_SW6}, /* RGB33 */
        {0, CS18_SW7,CS17_SW7, CS16_SW7}, /* RGB34 */
        {0, CS18_SW8,CS17_SW8, CS16_SW8}, /* RGB35 */
        {0, CS18_SW9,CS17_SW9, CS16_SW9}, /* RGB36 */
        {0, CS27_SW9,CS26_SW9, CS25_SW9}, /* RGB70 */
        {0, CS30_SW1,CS29_SW1, CS28_SW1}, /* RGB64 */
        {0, CS30_SW2,CS29_SW2, CS28_SW2}, /* RGB55 */
        {0, CS30_SW3,CS29_SW3, CS28_SW3}, /* RGB71 */
        {0, CS30_SW4,CS29_SW4, CS28_SW4}, /* RGB72 */
        {0, CS21_SW1,CS20_SW1, CS19_SW1}, /* RGB37 */
        {0, CS21_SW2,CS20_SW2, CS19_SW2}, /* RGB38 */
        {0, CS21_SW3,CS20_SW3, CS19_SW3}, /* RGB39 */
        {0, CS21_SW4,CS20_SW4, CS19_SW4}, /* RGB40 */
        {0, CS21_SW5,CS20_SW5, CS19_SW5}, /* RGB41 */
        {0, CS21_SW6,CS20_SW6, CS19_SW6}, /* RGB42 */
        {0, CS21_SW7,CS20_SW7, CS19_SW7}, /* RGB43 */
        {0, CS21_SW8,CS20_SW8, CS19_SW8}, /* RGB44 */
        {0, CS21_SW9,CS20_SW9, CS19_SW9}, /* RGB45 */
        {0, CS30_SW5,CS29_SW5, CS28_SW5}, /* RGB66 */
        {0, CS4_SW1, CS6_SW1 , CS5_SW1,}, /* RGB79 */
        {0, CS4_SW2, CS6_SW2 , CS5_SW2,}, /* RGB80 */
        {0, CS4_SW3, CS6_SW3 , CS5_SW3,}, /* RGB81 */
    };

    #else
    static const is31_led led_ctrl_table[BOARD_LED_NUM] = {
        { 1, CS_SW(1, 1), CS_SW(2, 1), CS_SW(3, 1), },  /* RGB46 */
        { 1, CS_SW(1, 5), CS_SW(2, 5), CS_SW(3, 5), },  /* RGB50 */
        { 1, CS_SW(1, 6), CS_SW(2, 6), CS_SW(3, 6), },  /* RGB51 */
        { 1, CS_SW(1, 7), CS_SW(2, 7), CS_SW(3, 7), },  /* RGB52 */
        { 1, CS_SW(1, 8), CS_SW(2, 8), CS_SW(3, 8), },  /* RGB53 */
        { 1, CS_SW(1, 9), CS_SW(2, 9), CS_SW(3, 9), },  /* RGB54 */
        { 1, CS_SW(4, 1), CS_SW(5, 1), CS_SW(6, 1), },  /* RGB55 */
        { 1, CS_SW(4, 2), CS_SW(5, 2), CS_SW(6, 2), },  /* RGB56 */
        { 1, CS_SW(4, 3), CS_SW(5, 3), CS_SW(6, 3), },  /* RGB57 */
        { 1, CS_SW(4, 4), CS_SW(5, 4), CS_SW(6, 4), },  /* RGB58 */
        { 1, CS_SW(4, 5), CS_SW(5, 5), CS_SW(6, 5), },  /* RGB59 */
        { 1, CS_SW(4, 6), CS_SW(5, 6), CS_SW(6, 6), },  /* RGB60 */
        { 1, CS_SW(4, 7), CS_SW(5, 7), CS_SW(6, 7), },  /* RGB61 */
        { 1, CS_SW(4, 8), CS_SW(5, 8), CS_SW(6, 8), },  /* RGB62 */
        { 1, CS_SW(4, 9), CS_SW(5, 9), CS_SW(6, 9), },  /* RGB63 */

        { 0, CS_SW(1, 1), CS_SW(2, 1), CS_SW(3, 1), },  /* RGB1 */
        { 0, CS_SW(1, 2), CS_SW(2, 2), CS_SW(3, 2), },  /* RGB2 */
        { 0, CS_SW(1, 3), CS_SW(2, 3), CS_SW(3, 3), },  /* RGB3 */
        { 0, CS_SW(1, 4), CS_SW(2, 4), CS_SW(3, 4), },  /* RGB4 */
        { 0, CS_SW(1, 5), CS_SW(2, 5), CS_SW(3, 5), },  /* RGB5 */
        { 0, CS_SW(1, 6), CS_SW(2, 6), CS_SW(3, 6), },  /* RGB6 */
        { 0, CS_SW(1, 7), CS_SW(2, 7), CS_SW(3, 7), },  /* RGB7 */
        { 0, CS_SW(1, 8), CS_SW(2, 8), CS_SW(3, 8), },  /* RGB8 */
        { 0, CS_SW(1, 9), CS_SW(2, 9), CS_SW(3, 9), },  /* RGB9 */
        { 1, CS_SW(7, 1), CS_SW(8, 1), CS_SW(9, 1), },  /* RGB64 */
        { 1, CS_SW(7, 2), CS_SW(8, 2), CS_SW(9, 2), },  /* RGB65 */
        { 1, CS_SW(7, 3), CS_SW(8, 3), CS_SW(9, 3), },  /* RGB66 */
        { 1, CS_SW(7, 4), CS_SW(8, 4), CS_SW(9, 4), },  /* RGB67 */
        { 1, CS_SW(7, 5), CS_SW(8, 5), CS_SW(9, 5), },  /* RGB68 */

        { 0, CS_SW(4, 1),  CS_SW(5, 1),  CS_SW(6, 1),  },  /* RGB10 */
        { 0, CS_SW(4, 2),  CS_SW(5, 2),  CS_SW(6, 2),  },  /* RGB11 */
        { 0, CS_SW(4, 3),  CS_SW(5, 3),  CS_SW(6, 3),  },  /* RGB12 */
        { 0, CS_SW(4, 4),  CS_SW(5, 4),  CS_SW(6, 4),  },  /* RGB13 */
        { 0, CS_SW(4, 5),  CS_SW(5, 5),  CS_SW(6, 5),  },  /* RGB14 */
        { 0, CS_SW(4, 6),  CS_SW(5, 6),  CS_SW(6, 6),  },  /* RGB15 */
        { 0, CS_SW(4, 7),  CS_SW(5, 7),  CS_SW(6, 7),  },  /* RGB16 */
        { 0, CS_SW(4, 8),  CS_SW(5, 8),  CS_SW(6, 8),  },  /* RGB17 */
        { 0, CS_SW(4, 9),  CS_SW(5, 9),  CS_SW(6, 9),  },  /* RGB18 */
        { 1, CS_SW(7, 6),  CS_SW(8, 6),  CS_SW(9, 6),  },  /* RGB69 */
        { 1, CS_SW(7, 7),  CS_SW(8, 7),  CS_SW(9, 7),  },  /* RGB70 */
        { 1, CS_SW(7, 8),  CS_SW(8, 8),  CS_SW(9, 8),  },  /* RGB71 */
        { 1, CS_SW(7, 9),  CS_SW(8, 9),  CS_SW(9, 9),  },  /* RGB72 */
        { 1, CS_SW(10, 5), CS_SW(11, 5), CS_SW(12, 5), },  /* RGB77 */

        { 0, CS_SW(7, 1),  CS_SW(8, 1),  CS_SW(9, 1),  },  /* RGB19 */
        { 0, CS_SW(7, 2),  CS_SW(8, 2),  CS_SW(9, 2),  },  /* RGB20 */
        { 0, CS_SW(7, 3),  CS_SW(8, 3),  CS_SW(9, 3),  },  /* RGB21 */
        { 0, CS_SW(7, 4),  CS_SW(8, 4),  CS_SW(9, 4),  },  /* RGB22 */
        { 0, CS_SW(7, 5),  CS_SW(8, 5),  CS_SW(9, 5),  },  /* RGB23 */
        { 0, CS_SW(7, 6),  CS_SW(8, 6),  CS_SW(9, 6),  },  /* RGB24 */
        { 0, CS_SW(7, 7),  CS_SW(8, 7),  CS_SW(9, 7),  },  /* RGB25 */
        { 0, CS_SW(7, 8),  CS_SW(8, 8),  CS_SW(9, 8),  },  /* RGB26 */
        { 0, CS_SW(7, 9),  CS_SW(8, 9),  CS_SW(9, 9),  },  /* RGB27 */
        { 1, CS_SW(10, 1),  CS_SW(11, 1),  CS_SW(12, 1),  },  /* RGB73 */
        { 1, CS_SW(10, 2),  CS_SW(11, 2),  CS_SW(12, 2),  },  /* RGB74 */
        { 1, CS_SW(10, 3),  CS_SW(11, 3),  CS_SW(12, 3),  },  /* RGB75 */
        { 1, CS_SW(10, 4),  CS_SW(11, 4),  CS_SW(12, 4),  },  /* RGB76 */


        { 0, CS_SW(10, 1),  CS_SW(11, 1),  CS_SW(12, 1),  },  /* RGB28 */
        { 0, CS_SW(10, 2),  CS_SW(11, 2),  CS_SW(12, 2),  },  /* RGB29 */
        { 0, CS_SW(10, 3),  CS_SW(11, 3),  CS_SW(12, 3),  },  /* RGB30 */
        { 0, CS_SW(10, 4),  CS_SW(11, 4),  CS_SW(12, 4),  },  /* RGB31 */
        { 0, CS_SW(10, 5),  CS_SW(11, 5),  CS_SW(12, 5),  },  /* RGB32 */
        { 0, CS_SW(10, 6),  CS_SW(11, 6),  CS_SW(12, 6),  },  /* RGB33 */
        { 0, CS_SW(10, 7),  CS_SW(11, 7),  CS_SW(12, 7),  },  /* RGB34 */
        { 0, CS_SW(10, 8),  CS_SW(11, 8),  CS_SW(12, 8),  },  /* RGB35 */
        { 0, CS_SW(10, 9),  CS_SW(11, 9),  CS_SW(12, 9),  },  /* RGB36 */
        { 1, CS_SW(10, 6),  CS_SW(11, 6),  CS_SW(12, 6),  },  /* RGB78 */
        { 1, CS_SW(10, 7),  CS_SW(11, 7),  CS_SW(12, 7),  },  /* RGB79 */
        { 1, CS_SW(10, 8),  CS_SW(11, 8),  CS_SW(12, 8),  },  /* RGB80 */
        { 1, CS_SW(10, 9),  CS_SW(11, 9),  CS_SW(12, 9),  },  /* RGB81 */

        { 0, CS_SW(13, 1),  CS_SW(14, 1),  CS_SW(15, 1),  },  /* RGB37 */
        { 0, CS_SW(13, 2),  CS_SW(14, 2),  CS_SW(15, 2),  },  /* RGB38 */
        { 0, CS_SW(13, 3),  CS_SW(14, 3),  CS_SW(15, 3),  },  /* RGB39 */
        { 0, CS_SW(13, 4),  CS_SW(14, 4),  CS_SW(15, 4),  },  /* RGB40 */ 
        { 0, CS_SW(13, 5),  CS_SW(14, 5),  CS_SW(15, 5),  },  /* RGB41 */
        { 0, CS_SW(13, 6),  CS_SW(14, 6),  CS_SW(15, 6),  },  /* RGB42 */
        { 0, CS_SW(13, 7),  CS_SW(14, 7),  CS_SW(15, 7),  },  /* RGB43 */
        { 0, CS_SW(13, 8),  CS_SW(14, 8),  CS_SW(15, 8),  },  /* RGB44 */ 
        { 0, CS_SW(13, 9),  CS_SW(14, 9),  CS_SW(15, 9),  },  /* RGB45 */ 
        { 1, CS_SW(13, 7),  CS_SW(14, 7),  CS_SW(15, 7),  },  /* RGB88 */
        { 1, CS_SW(13, 8),  CS_SW(14, 8),  CS_SW(15, 8),  },  /* RGB89 */
        { 1, CS_SW(13, 9),  CS_SW(14, 9),  CS_SW(15, 9),  },  /* RGB90 */ 

        { 1, CS_SW(1, 2),  CS_SW(2, 2),  CS_SW(3, 2),  },  /* RGB47 */
        { 1, CS_SW(1, 3),  CS_SW(2, 3),  CS_SW(3, 3),  },  /* RGB48 */
        { 1, CS_SW(1, 4),  CS_SW(2, 4),  CS_SW(3, 4),  },  /* RGB49 */ 

    };
    #endif

#endif

static const int16_t key_led_lookup_table[BOARD_LED_ROW_NUM][BOARD_LED_COLUMN_NUM] = {
 /* C0  C1  C2  C3  C4  C5  C6  C7  C8  C9 C10 C11 C12 C13 C14 */
    { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14},  /*R0*/
    {15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, -1},  /*R1*/
    {29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, -1},  /*R2*/
    {43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, -1, -1},  /*R3*/
    {56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, -1, 68, -1},  /*R4*/
    {69, 70, 71, -1, 72, 73, -1, 74, -1, 75, 76, 77, 78, 79, 80},  /*R5*/
};

static key_tmp_sta_t key_tmp_sta_table[BOARD_KEY_LED_NUM] = {0};

static key_led_color_info_t key_led_color_info = {0};

static const led_action_t  led_action_default_table[LED_ACTION_DEFAULT_MAX] = {
    {.id = 0, .at = LED_ACTION_NONE,       .cy = 0, .rr = 0, .pr = 0, .bd = 0, .hl = 21, .ll = 0, .ls = 0, .tpt = 0, .hzc = 2, .hdy = 0, .pdy = 0,  },  /* 0 */
    {.id = 1, .at = LED_ACTION_BRIGHT,     .cy = 0, .rr = 0, .pr = 0, .bd = 1, .hl = 21, .ll = 0, .ls = 0, .tpt = 0, .hzc = 1, .hdy = 0, .pdy = 0,  },  /* 1 */
    {.id = 2, .at = LED_ACTION_GRADUAL,    .cy = 1, .rr = 0, .pr = 0, .bd = 1, .hl = 60, .ll = 0, .ls = 5, .tpt = 0, .hzc = 1, .hdy = 0, .pdy = 2,  },  /* 2 */
    {.id = 3, .at = LED_ACTION_BREATH,     .cy = 1, .rr = 0, .pr = 1, .bd = 0, .hl = 21, .ll = 0, .ls = 1, .tpt = 0, .hzc = 1, .hdy = 0, .pdy = 0,  },  /* 3 */
    {.id = 4, .at = LED_ACTION_WAVE,       .cy = 1, .rr = 0, .pr = 0, .bd = 0, .hl = 21, .ll = 0, .ls = 1, .tpt = 0, .hzc = 2, .hdy = 0, .pdy = 0,  },  /* 4 */
    {.id = 5, .at = LED_ACTION_RIPPLE,     .cy = 0, .rr = 0, .pr = 0, .bd = 0, .hl = 21, .ll = 0, .ls = 8, .tpt = 1, .hzc = 2, .hdy = 0, .pdy = 0,  },  /* 5 */
    {.id = 6, .at = LED_ACTION_REACTION,   .cy = 0, .rr = 0, .pr = 1, .bd = 0, .hl = 21, .ll = 0, .ls = 2, .tpt = 1, .hzc = 2, .hdy = 0, .pdy = 0,  },  /* 6 */
    {.id = 7, .at = LED_ACTION_RANDOM,     .cy = 1, .rr = 1, .pr = 0, .bd = 0, .hl = 21, .ll = 0, .ls = 1, .tpt = 0, .hzc = 5, .hdy = 0, .pdy = 15, },  /* 7 */
    {.id = 8, .at = LED_ACTION_RAY,        .cy = 0, .rr = 0, .pr = 0, .bd = 0, .hl = 21, .ll = 0, .ls = 8, .tpt = 1, .hzc = 2, .hdy = 0, .pdy = 0,  },  /* 8 */
    {.id = 9, .at = LED_ACTION_GUSH,       .cy = 1, .rr = 0, .pr = 0, .bd = 0, .hl = 21, .ll = 0, .ls = 1, .tpt = 0, .hzc = 2, .hdy = 0, .pdy = 0,  },  /* 9 */
    {.id = 10, .at = LED_ACTION_RAIN,      .cy = 0, .rr = 1, .pr = 0, .bd = 0, .hl = 21, .ll = 0, .ls = 2, .tpt = 1, .hzc = 2, .hdy = 0, .pdy = 15,  },
};

static const led_action_handle_t led_action_handle_table[LED_ACTION_DEFAULT_MAX] = {
    { .init  = init_none,         .proc  = proc_none,       .keyp = keyp_none,       },  /* 0 */
    { .init  = init_bright,       .proc  = proc_bright,     .keyp = keyp_bright,     },  /* 1 */
    { .init  = init_gradual,      .proc  = proc_gradual,    .keyp = keyp_gradual,    },  /* 2 */
    { .init  = init_breath,       .proc  = proc_breath,     .keyp = keyp_breath,     },  /* 3 */
    { .init  = init_wave,         .proc  = proc_wave,       .keyp = keyp_wave,       },  /* 4 */
    { .init  = init_ripple,       .proc  = proc_ripple,     .keyp = keyp_ripple,     },  /* 5 */
    { .init  = init_reaction,     .proc  = proc_reaction,   .keyp = keyp_reaction,   },  /* 6 */
    { .init  = init_random,       .proc  = proc_random,     .keyp = keyp_random,     },  /* 7 */
    { .init  = init_ray,          .proc  = proc_ray,        .keyp = keyp_ray,        },  /* 8 */   
    { .init  = init_gush,         .proc  = proc_gush,       .keyp = keyp_gush,       },
    { .init  = init_rain,         .proc  = proc_rain,       .keyp = keyp_rain,       }, 
};

static key_led_color_t lamp_tmp_sta_table[BOARD_KEY_LED_NUM] = {0};

uint8_t get_rgb_para_len(void)
{
    return sizeof(led_action_default_table) / sizeof(led_action_default_table[0]);
}

table_info_t get_key_tmp_sta_table_info(void)
{
    table_info_t table_info;

    table_info.table = key_tmp_sta_table;
    table_info.size  = sizeof(key_tmp_sta_table);

    return table_info;
}

const led_action_handle_t *get_led_action_handle_table(void)
{
    return led_action_handle_table;
}

const led_action_t *get_led_action_default_table(void)
{
    return led_action_default_table;
}

key_led_color_info_t *get_key_led_color_table_info(void)
{
    return &key_led_color_info;
}

const is31_led *get_rgb_led_ctrl_ptr(uint8_t index)
{
    if (index >= BOARD_LED_NUM)
    {
        return NULL;
    }
    return &led_ctrl_table[index];
}

int16_t lookup_key_led_value(uint8_t row, uint8_t col)
{
    if ((row >= BOARD_LED_ROW_NUM) || (col >= BOARD_LED_COLUMN_NUM))
    {
        return -1;
    }
    return key_led_lookup_table[row][col];
}

key_tmp_sta_t *get_key_tmp_sta_ptr(uint8_t index)
{
    if (index >= BOARD_KEY_LED_NUM)
    {
        return NULL;
    }
    return &key_tmp_sta_table[index];
}

key_led_color_t *get_lamp_led_color_ptr(uint8_t index)
{
    if (index >= BOARD_KEY_LED_NUM)
    {
        return NULL;
    }
    return &lamp_tmp_sta_table[index];
}

key_led_color_t *get_key_led_color_ptr(uint8_t color_idx, uint8_t led_idx)
{
    if ((color_idx >= BOARD_LED_COLOR_LAYER_MAX) || (led_idx >= BOARD_KEY_LED_NUM))
    {
        return NULL;
    }
    return &key_led_color_info.table[color_idx][led_idx];
}

const led_action_t *get_led_action_ptr(uint8_t index)
{
    if (index >= LED_ACTION_MAX)
    {
        return NULL;
    }
    return &led_action_default_table[index];
}
uint8_t get_led_action_id(uint8_t at)
{
    for (uint8_t i = 0; i < LED_ACTION_MAX; i++)
    {
        if (led_action_default_table[i].at == at)
        {
            return led_action_default_table[i].id;
        }
    }
    return LED_ACTION_NONE;
}

const led_action_handle_t *get_led_action_handle_ptr(uint8_t index)
{
    if (index >= LED_ACTION_MAX)
    {
        return NULL;
    }
    return &led_action_handle_table[index];
}