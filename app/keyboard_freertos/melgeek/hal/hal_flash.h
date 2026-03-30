/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved.
 *
 * Description:
 *
 */
#ifndef _HAL_FLASH_H_
#define _HAL_FLASH_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "board.h"
#include "db_flash_map.h"
#include "app_err_code.h"

typedef enum
{
    FLASH_RET_SUCCESS       = 0,
    FLASH_RET_W_TIMEOUT     = (MG_ERR_BASE_FLASH + 0),
    FLASH_RET_R_TIMEOUT     = (MG_ERR_BASE_FLASH + 1),
    FLASH_RET_ERASE_TIMEOUT = (MG_ERR_BASE_FLASH + 2),
    FLASH_RET_ERASE_ALGIN   = (MG_ERR_BASE_FLASH + 3),
    FLASH_RET_MAX_LEN       = (MG_ERR_BASE_FLASH + 4),
    FLASH_RET_MAX
} flash_ret;

void hal_otp_get_custom_uuid(uint8_t *uuid);
void hal_otp_get_uuid(uint8_t *uuid);
void hal_otp_get_uid(uint8_t *uid);
flash_ret hal_flash_write(uint32_t dest, void const *p_src, uint32_t len);
flash_ret hal_flash_read(uint32_t dest, void *p_src, uint32_t len);
flash_ret hal_flash_erase(uint32_t page_addr, uint32_t num_pages);
flash_ret hal_flash_block_erase(uint32_t addr, uint32_t user_size);
flash_ret hal_flash_init(void);

#endif 