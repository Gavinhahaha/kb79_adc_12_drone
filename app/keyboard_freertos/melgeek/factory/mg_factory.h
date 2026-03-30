/*
 * Copyright (c) 2024-2025 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#ifndef _MG_FACTORY_H_
#define _MG_FACTORY_H_
#include "stdint.h"
#include "stdbool.h"

#define FACTORY_VENDORS_MAGIC          (0xCB5706E9) 
#define VENDORS_MAX                    (2) //

// 工厂代号
typedef enum
{
    FA_IDX_EXIT = 0,
    FA_IDX_JIANHANG = 1, // 建航
    FA_IDX_NARUI = 2,    // 纳锐
    FA_IDX_SIFANGGE = 3, // 四方格
    FA_IDX_ABITE = 4,    // 阿比特
    FA_IDX_MAX
} fa_idx_t;

// 工厂产测步骤
typedef enum
{
    FA_STEP_0 = 0,
    FA_STEP_1 = 1,
    FA_STEP_2 = 2,
    FA_STEP_3 = 3,
    FA_STEP_4 = 4,
    FA_STEP_5 = 5,
    FA_STEP_6 = 6,
    FA_STEP_7 = 7,
    FA_STEP_8 = 8,
    FA_STEP_9 = 9,
    FA_STEP_MAX,
} fa_sub_t;

typedef enum
{
    FA_STEP_MAX_JIANHANG = 3, // 建航
    FA_STEP_MAX_NARUI = 7,    // 纳锐
    FA_STEP_MAX_SIFANGGE = 0, // 四方格
    FA_STEP_MAX_ABITE = 0,    // 阿比特
} fa_cnt_t;

typedef struct
{
    fa_idx_t fa_idx;
    fa_sub_t fa_step;
} fa_t;

typedef struct 
{
    uint32_t magic;
    fa_t fa[VENDORS_MAX][FA_STEP_MAX];

} mg_factory_t;

void mg_factory_init(void);
void mg_factory_set_info(mg_factory_t * update);
mg_factory_t* mg_factory_get_info(void);
void mg_factory_set_default_data(void);
bool mg_factory_checkout(void);
#endif