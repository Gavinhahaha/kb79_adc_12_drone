/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#ifndef __DFU_API_H
#define __DFU_API_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "board.h"
#include "db_flash_map.h"

#define USER_UPGREAD_FLAG_MAGIC   (0xbeaf5aa5)
#define BOARD_DEVICE_ID           (0x6750)

#define BOARD_SN_NAME_MAGIC       (0x7e5feb53)
#define BOARD_SN_NAME_MAGIC2       (0x4544414d)

/*ota flag structure*/
typedef struct
{
    uint32_t magic;
    uint32_t device;
    uint32_t len;
    uint32_t checksum;
} user_fota_header_t;

typedef struct
{
    uint32_t magic;
    uint32_t reserve;
    uint16_t len;
    uint16_t reserve_len;
    uint8_t device_name[16];
    uint8_t reserve_arr[0x80];
    uint32_t checksum;
} user_product_header_t;

void ota_board_flash_init(void);

void ota_board_complete_reset(void);

hpm_stat_t ota_board_flash_read(uint32_t addr, void *buffer, uint32_t len);

uint32_t ota_board_flash_size(uint8_t ota_index);

hpm_stat_t ota_board_flash_erase(uint8_t ota_index);

hpm_stat_t ota_board_flash_write(uint32_t addr, void const *src, uint32_t len);

bool ota_board_auto_write(void const* src, uint32_t len);

int ota_fota_flash_checksum(uint32_t addr, uint32_t len, uint32_t* checksum);

bool ota_board_auto_checksum(void);

void ota_board_app_jump(uint8_t ota_index);

uint8_t ota_check_current_otaindex(void);

bool hal_otp_get_exip(bool *exip);

#endif //__OTA_API_H