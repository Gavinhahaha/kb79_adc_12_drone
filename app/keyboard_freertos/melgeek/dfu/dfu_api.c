/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#include "dfu_api.h"
#include "hal_flash.h"
#include "hal_wdg.h"
#include "hpm_romapi.h"
#include "hpm_l1c_drv.h"
#include "app_debug.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "dfu_api"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#define SECTOR_SIZE                               (4  * 1024)
#define BLOCK_SIZE                                (64 * 1024)
#define OTP_ENCRYPT_WORDS_IDX                     (24U)
#define OTP_ENCRYPT_BIT_IDX_START                 (12U)

static uint32_t current_addr = 0;

extern uint8_t sector_buffer[SECTOR_SIZE];
static xpi_nor_config_t s_xpi_nor_config;
static user_fota_header_t current_fota_header;
// static uint32_t flash_size;
// static uint32_t sector_size;
// static uint32_t block_size;
// static uint32_t page_size;

void ota_board_flash_init(void)
{
    xpi_nor_config_option_t option;
    option.header.U = BOARD_APP_XPI_NOR_CFG_OPT_HDR;
    option.option0.U = BOARD_APP_XPI_NOR_CFG_OPT_OPT0;
    option.option1.U = BOARD_APP_XPI_NOR_CFG_OPT_OPT1;
    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    rom_xpi_nor_auto_config(BOARD_APP_XPI_NOR_XPI_BASE, &s_xpi_nor_config, &option);
    enable_global_irq(CSR_MSTATUS_MIE_MASK);

}

#define SOC_ROM_API_ARG_REG (*(volatile uint32_t *)(0xF40D4000UL + 0U))
#define SOC_XPI_PERSIST_REG (*(volatile uint32_t *)(0xF40D4000UL + 4U))
#define SOC_ROM_FLAG_REG (*(volatile uint32_t *)(0xF40D4000UL + 8U))

static void soc_reset(void)
{

    DBG_PRINTF("%s\n", __func__);
    board_sw_reset();
}

void ota_board_complete_reset(void)
{
    // if (current_addr != FLASH_APP1_ALL_IMG_ADDR && current_addr != FLASH_APP2_ALL_IMG_ADDR)
    //     return;

    // if (!ota_board_auto_checksum())
    //     return;

    // current_addr = 0;
    DBG_PRINTF("ota success!\n");
    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    soc_reset();
    enable_global_irq(CSR_MSTATUS_MIE_MASK);
}

hpm_stat_t ota_board_flash_read(uint32_t addr, void *buffer, uint32_t len)
{
    return hal_flash_read(addr, buffer, len);
}

uint32_t ota_board_flash_size(uint8_t ota_index)
{
    return ota_index == 0 ? FLASH_APP1_ALL_IMG_SIZE : FLASH_APP2_ALL_IMG_SIZE;
}

hpm_stat_t ota_board_flash_erase(uint8_t ota_index)
{
    uint32_t addr, user_size = 0;
    if (ota_index == 0)
    {
        addr = FLASH_APP1_ALL_IMG_ADDR - FLASH_ADDR_BASE;
        user_size = FLASH_APP1_ALL_IMG_SIZE;
    }
    else if (ota_index == 1)
    {
        addr = FLASH_APP2_ALL_IMG_ADDR - FLASH_ADDR_BASE;
        user_size = FLASH_APP2_ALL_IMG_SIZE;
    }
    else
    {
        return 1;
    }
    hal_flash_block_erase(addr, user_size);

    return status_success;
}

hpm_stat_t ota_board_flash_write(uint32_t addr, void const *src, uint32_t len)
{
    hal_wdg_manual_feed();
    return hal_flash_write( addr, src, len);
}

void ota_board_header_write(uint8_t ota_index, user_fota_header_t *fota_header)
{
    hpm_stat_t status;
    uint32_t write_addr;

    if (fota_header == NULL || sizeof(user_fota_header_t) > SECTOR_SIZE)
        return;
    if (ota_index == 0)
        write_addr = FLASH_APP1_IMGINFO_ADDR - FLASH_ADDR_BASE;
    else if (ota_index == 1)
        write_addr = FLASH_APP2_IMGINFO_ADDR - FLASH_ADDR_BASE;
    else
        return;

    if (write_addr % SECTOR_SIZE)
        return;

    DBG_PRINTF("header write addr:0x%08x\n", write_addr);
    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    status = rom_xpi_nor_erase_sector(BOARD_APP_XPI_NOR_XPI_BASE, xpi_xfer_channel_auto, &s_xpi_nor_config, write_addr);
    enable_global_irq(CSR_MSTATUS_MIE_MASK);
    if (status != status_success)
    {
        DBG_PRINTF("ERROR:erase sector failed, addr:0x%x\n", write_addr);
        return;
    }

    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    status = rom_xpi_nor_program(BOARD_APP_XPI_NOR_XPI_BASE, xpi_xfer_channel_auto, &s_xpi_nor_config,
                                 (uint32_t *)fota_header, write_addr, sizeof(user_fota_header_t));
    enable_global_irq(CSR_MSTATUS_MIE_MASK);
    if (status != status_success)
    {
        DBG_PRINTF("ERROR:program failed: status = %ld!\r\n", status);
        return;
    }
    DBG_PRINTF("write success!\n");
}

int ota_fota_flash_checksum(uint32_t addr, uint32_t len, uint32_t *checksum)
{
    hpm_stat_t status;
    unsigned int i;
    unsigned int allsize = 0;
    unsigned int read_len;
    unsigned int tmp;
    unsigned char buf[512];

    *checksum = 0;
    while (1)
    {
        if (len - allsize > sizeof(buf))
            read_len = sizeof(buf);
        else
            read_len = len - allsize;

        status = rom_xpi_nor_read(BOARD_APP_XPI_NOR_XPI_BASE, xpi_xfer_channel_auto, &s_xpi_nor_config, (uint32_t *)buf,
                                  addr + allsize - FLASH_ADDR_BASE, read_len);
        if (status != status_success)
        {
            DBG_PRINTF("flash read fail\r\n");
            return -1;
        }
        for (i = 0; i < read_len; i++)
        {
            tmp = buf[i];
            *checksum += tmp;
        }
        allsize += read_len;

        if (allsize >= len)
            return 0;
    }
    return -1;
}

bool ota_board_auto_checksum(void)
{
    int ret;
    uint32_t checksum;
    // checksum
    ret = ota_fota_flash_checksum(current_addr, current_fota_header.len, &checksum);
    DBG_PRINTF("current.checksum:0x%08x, checksum:0x%08x\n", current_fota_header.checksum, checksum);
    if (ret != 0 || current_fota_header.checksum != checksum)
    {
        DBG_PRINTF("checksum failed, ota fail!\n");
        current_addr = 0;
        return false;
    }
    return true;
}

bool ota_board_auto_write(void const *src, uint32_t len)
{
    static uint32_t offset = 0;
    uint8_t ota_index;
    user_fota_header_t *current_header = (user_fota_header_t *)src;
    if (current_header->magic == USER_UPGREAD_FLAG_MAGIC &&
        current_header->device == BOARD_DEVICE_ID)
    {
        ota_index = ota_check_current_otaindex();
        if (ota_index == 1)
        {
            memcpy(&current_fota_header, current_header, sizeof(user_fota_header_t));
            // DBG_PRINTF("ota1, length:%d,checksem:0x%08x\n", current_header->len, current_header->checksum);
            ota_board_header_write(0, &current_fota_header);
            current_addr = FLASH_APP1_ALL_IMG_ADDR;
            offset = 0;
            src = src + sizeof(user_fota_header_t);
            len -= sizeof(user_fota_header_t);
            // DBG_PRINTF("ota data download...\n");
        }
        else if (ota_index == 0)
        {
            memcpy(&current_fota_header, current_header, sizeof(user_fota_header_t));
            ota_board_header_write(1, &current_fota_header);
            current_addr = FLASH_APP2_ALL_IMG_ADDR;
            offset = 0;
            src = src + sizeof(user_fota_header_t);
            len -= sizeof(user_fota_header_t);
            // DBG_PRINTF("ota data download...\n");
        }
    }

    if (current_addr != 0)
    {
        ota_board_flash_write(current_addr + offset, src, len);
        offset += len;
        if (offset >= current_fota_header.len)
        {
            // complete checksum and reset
            // ota_board_complete_reset();
            offset = 0;
            return true;
        }
    }
    return false;
}

uint8_t ota_check_current_otaindex(void)
{
    hpm_stat_t status;
    uint32_t version = 0;

    uint16_t sw_version = *((uint16_t *)(FLASH_APP1_BOOT_HEAD_ADDR + FW_HEADER_SW_VERSION_OFFSET));
    uint16_t fw_version1, fw_version2;

    DBG_PRINTF("sw_version:%d\n", sw_version);

    status = rom_xpi_nor_read(BOARD_APP_XPI_NOR_XPI_BASE, xpi_xfer_channel_auto, &s_xpi_nor_config, &version,
                              FLASH_APP1_BOOT_HEAD_ADDR + FW_HEADER_SW_VERSION_OFFSET - FLASH_ADDR_BASE, 2);
    if (status != status_success)
    {
        DBG_PRINTF("flash read fail\r\n");
        return -1;
    }
    fw_version1 = (uint16_t)version;

    status = rom_xpi_nor_read(BOARD_APP_XPI_NOR_XPI_BASE, xpi_xfer_channel_auto, &s_xpi_nor_config, &version,
                              FLASH_APP2_BOOT_HEAD_ADDR + FW_HEADER_SW_VERSION_OFFSET - FLASH_ADDR_BASE, 2);
    if (status != status_success)
    {
        DBG_PRINTF("flash read fail\r\n");
        return -1;
    }
    fw_version2 = (uint16_t)version;

    DBG_PRINTF("ota1 version:%d, ota2 version:%d\n", fw_version1, fw_version2);

    if (rom_xpi_nor_is_remap_enabled(BOARD_APP_XPI_NOR_XPI_BASE))
    {
        return 1;
    }
    return 0;
}

bool hal_otp_get_exip(bool *exip)
{
    if (exip == NULL)
    {
        return false;
    }
    uint32_t word = ROM_API_TABLE_ROOT->otp_driver_if->read_from_shadow(OTP_ENCRYPT_WORDS_IDX);
    *exip = (word >> OTP_ENCRYPT_BIT_IDX_START) & 0x01;
    return true;
}
