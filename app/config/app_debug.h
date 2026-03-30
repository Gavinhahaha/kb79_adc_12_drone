/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef _APP_DEBUG_H_
#define _APP_DEBUG_H_
#include "SEGGER_RTT.h"



#define DEBUG_EN            0

#define DBG_EN_MATRIX       0
#define DBG_EN_HIVE         0
#define DBG_EN_SM           0
#define DBG_EN_CALI         0

#define DBG_EN_ADC          0
#define DBG_EN_HID          0
#define DBG_EN_GPIO         0
#define DBG_EN_TIMER        0
#define DBG_EN_KEY          0

#if  DEBUG_EN
#define DEBUG_RTT(tag, fmt, ...) Mg_SEGGER_RTT_printf(tag,fmt, ##__VA_ARGS__) 
#else
#define DEBUG_RTT(tag,fmt, ...) ((void)0)
#endif

#endif



