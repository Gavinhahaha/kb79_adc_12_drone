/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#ifndef __DRV_ENCODER_H__
#define __DRV_ENCODER_H__

#include <stdbool.h>
#include <stdint.h>

typedef void (*drv_encoder_evt)(uint32_t evt);

typedef struct {
    uint32_t Tx_RightRotation;      // 顺时针旋转计数
    uint32_t Tx_LeftRotation;       // 逆时针旋转计数s
    int32_t encoder_diff;           // 旋转方向的累积差异
    drv_encoder_evt m_encoder_evt;
} encoder_t;


#define ENCODER_EVT_RIGHT_ROTATION      (1 << 0)
#define ENCODER_EVT_LEFT_ROTATION       (1 << 1)



void drv_encoder_init(void (*callback)(uint32_t evt));
encoder_t *drv_encoder_get_enc_param(void);







#endif