/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved.
 *
 * Description:
 *
 */
#include "hal_flash.h"
#include "hpm_romapi.h"
#include "hpm_ppor_drv.h"
#include "hpm_l1c_drv.h"
#include "app_debug.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "hal_flash"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)    ((void)0)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)    ((void)0)
#endif
#define SECTOR_SIZE (4 * 1024)
#define BLOCK_SIZE (64 * 1024)

#define OTP_CHIP_UID_IDX_START (8U)
#define OTP_CHIP_UID_IDX_END (11U)

#define OTP_CHIP_UUID_IDX_START (88U)
#define OTP_CHIP_UUID_IDX_END (91U)

#define OTP_CUSTOM_UUID_IDX_START (69U)
#define OTP_CUSTOM_UUID_IDX_END (70U)

uint8_t sector_buffer[SECTOR_SIZE];
static xpi_nor_config_t s_xpi_nor_config;

ATTR_RAMFUNC
void hal_otp_get_custom_uuid(uint8_t *uuid)
{
    uint32_t uuid_words[2];

    uint32_t word_idx = 0;
    ROM_API_TABLE_ROOT->otp_driver_if->init();
    for (uint32_t i = OTP_CUSTOM_UUID_IDX_START; i <= OTP_CUSTOM_UUID_IDX_END; i++)
    {
        uuid_words[word_idx++] = ROM_API_TABLE_ROOT->otp_driver_if->read_from_shadow(i);
    }

    memcpy(uuid, uuid_words, sizeof(uuid_words));
}

void hal_otp_get_uuid(uint8_t *uuid)
{
    uint32_t uuid_words[4];

    uint32_t word_idx = 0;
    ROM_API_TABLE_ROOT->otp_driver_if->init();
    for (uint32_t i = OTP_CHIP_UUID_IDX_START; i <= OTP_CHIP_UUID_IDX_END; i++)
    {
        uuid_words[word_idx++] = ROM_API_TABLE_ROOT->otp_driver_if->read_from_shadow(i);
    }

    memcpy(uuid, uuid_words, sizeof(uuid_words));

}

void hal_otp_get_uid(uint8_t *uid)
{
    uint32_t uid_words[4];

    uint32_t word_idx = 0;
    for (uint32_t i = OTP_CHIP_UID_IDX_START; i <= OTP_CHIP_UID_IDX_END; i++)
    {
        uid_words[word_idx++] = ROM_API_TABLE_ROOT->otp_driver_if->read_from_shadow(i);
    }

    memcpy(uid, uid_words, sizeof(uid_words));

}


flash_ret hal_flash_init(void)
{
    xpi_nor_config_option_t option;
    
    option.header.U  = BOARD_APP_XPI_NOR_CFG_OPT_HDR;
    option.option0.U = BOARD_APP_XPI_NOR_CFG_OPT_OPT0;
    option.option1.U = BOARD_APP_XPI_NOR_CFG_OPT_OPT1;
    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    rom_xpi_nor_auto_config(BOARD_APP_XPI_NOR_XPI_BASE, &s_xpi_nor_config, &option);
    enable_global_irq(CSR_MSTATUS_MIE_MASK);
    
    return FLASH_RET_SUCCESS;
}

flash_ret hal_flash_read(uint32_t addr, void *buffer, uint32_t len)
{
    uint32_t read_addr = addr - FLASH_ADDR_BASE;

    return rom_xpi_nor_read(BOARD_APP_XPI_NOR_XPI_BASE, xpi_xfer_channel_auto, &s_xpi_nor_config, buffer, read_addr, len);

}

flash_ret hal_flash_write(uint32_t addr, void const *src, uint32_t len)
{
    hpm_stat_t status;
    uint32_t write_addr = addr - FLASH_ADDR_BASE;
    
    if (len > SECTOR_SIZE)
    {
        DBG_PRINTF("ERROR: write size overflow!\n");
        return FLASH_RET_MAX_LEN;
    }
    memcpy(sector_buffer, src, len);

    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    status = rom_xpi_nor_program(BOARD_APP_XPI_NOR_XPI_BASE, xpi_xfer_channel_auto, &s_xpi_nor_config,
                                 (uint32_t *)sector_buffer, write_addr, len);
    enable_global_irq(CSR_MSTATUS_MIE_MASK);
    if (status != status_success)
    {
        DBG_PRINTF("ERROR:program failed: status = %ld!\r\n", status);
        return FLASH_RET_W_TIMEOUT;
    }
    
    return FLASH_RET_SUCCESS;
    // printf("write success!\n");
}


/// @brief
/// @param page_addr
/// @param num_pages
/// @return
flash_ret hal_flash_erase(uint32_t page_addr, uint32_t num_pages)
{
    uint32_t status;
    uint32_t erase_addr = page_addr - FLASH_ADDR_BASE;
    flash_ret ret       = FLASH_RET_SUCCESS;
    
    if (erase_addr % SECTOR_SIZE == 0)
    {
        disable_global_irq(CSR_MSTATUS_MIE_MASK);
        for (uint16_t i = 0; i < num_pages; i++)
        {
            erase_addr += SECTOR_SIZE * i;
            // printf(" erase_addr:0x%08x\n",  erase_addr);
            status = rom_xpi_nor_erase_sector(BOARD_APP_XPI_NOR_XPI_BASE, xpi_xfer_channel_auto, &s_xpi_nor_config, erase_addr);
            if (status != 0)
            {
                DBG_PRINTF("ERROR:erase sector failed, addr:0x%x\n", erase_addr);
                ret = FLASH_RET_ERASE_TIMEOUT;
            }
        }
        enable_global_irq(CSR_MSTATUS_MIE_MASK);
    }
    else
    {
        DBG_PRINTF("ERROR: flase erase not algin!\n");
        ret =  FLASH_RET_ERASE_ALGIN;
    }
    
    return ret;
}

flash_ret hal_flash_block_erase(uint32_t addr, uint32_t user_size)
{
    hpm_stat_t status;
    uint32_t start_index, erase_count = 0;
    flash_ret ret       = FLASH_RET_SUCCESS;

    if (!(addr % BLOCK_SIZE) && !(user_size % BLOCK_SIZE))
    {
        DBG_PRINTF("erase block mode!\n");
        
        erase_count = addr + user_size;
        disable_global_irq(CSR_MSTATUS_MIE_MASK);
        for (start_index = addr; start_index < erase_count;)
        {
            status = rom_xpi_nor_erase_block(BOARD_APP_XPI_NOR_XPI_BASE, xpi_xfer_channel_auto, &s_xpi_nor_config, start_index);
            if (status != status_success)
            {
                DBG_PRINTF("ERROR:erase block failed:%d\n", start_index);
                ret = FLASH_RET_ERASE_TIMEOUT;
                break;
            }
            start_index += BLOCK_SIZE;
        }
        enable_global_irq(CSR_MSTATUS_MIE_MASK);
        DBG_PRINTF("erase block success!\n");
    }
    else
    {
        DBG_PRINTF("ERROR: flase erase not algin!\n");
        ret = FLASH_RET_ERASE_ALGIN;
    }
    return ret;
}

