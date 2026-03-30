/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

#include "FreeRTOSConfig.h"


#define KEY_UPLOAD_THRESHOLD            (2)
#define KEY_FRAME_MODE                  (2) //0:完整帧 1：变化帧  2：阈值帧
#define KEY_LOW_PWR_MODE_TIME           (60) //S
#define KEY_LOW_PWR_SCAN_FREQ           (250) //HZ
#define KEY_HALL_STABLE_TIME            (35) //US


#define PRIORITY_SM                     (configMAX_PRIORITIES-1)
#define PRIORITY_KEY                    (configMAX_PRIORITIES-1)
#define PRIORITY_HID                    (configMAX_PRIORITIES-2)

#define PRIORITY_FN                     (configMAX_PRIORITIES-4)
#define PRIORITY_SK                     (configMAX_PRIORITIES-5)
//#define PRIORITY_TIMER                  (configMAX_PRIORITIES-6)

//#define PRIORITY_LOG                    0
#define PRIORITY_SCAN                   (configMAX_PRIORITIES-9)
#define PRIORITY_HIVE                   (configMAX_PRIORITIES-6)
#define PRIORITY_DB                     (configMAX_PRIORITIES-7)
#define PRIORITY_CALI                   (configMAX_PRIORITIES-8)
#define PRIORITY_RGB                    (configMAX_PRIORITIES-9)
#define PRIORITY_UART                   (configMAX_PRIORITIES-5)
#define PRIORITY_INDEV                  (configMAX_PRIORITIES-8)
#define PRIORITY_SYNC                   (configMAX_PRIORITIES-9)
#define PRIORITY_DETECTE                (configMAX_PRIORITIES-8)
#define PRIORITY_MACRO                  (configMAX_PRIORITIES-8)

//4字节为单位,256*4 = 1024
#define STACK_SIZE_SCAN                 (256)
#define STACK_SIZE_TIMER                (256)
#define STACK_SIZE_HIVE                 (300)
#define STACK_SIZE_RGB                  (256)
#define STACK_SIZE_KEY                  (300)
#define STACK_SIZE_HID                  (256)
#define STACK_SIZE_UART                 (256)
#define STACK_SIZE_SYNC                 (256)
#define STACK_SIZE_DETECTE              (256)

#define STACK_SIZE_CALI                 (256)
#define STACK_SIZE_USBD                 (256) 
#define STACK_SIZE_LOG                  (188)
#define STACK_SIZE_DB                   (256)
#define STACK_SIZE_FN                   (256)
#define STACK_SIZE_SK                   (256)
#define STACK_SIZE_INDEV                (256)
#define STACK_SIZE_SM                   (300)
#define STACK_SIZE_MACRO                (256)

#endif
