/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#ifndef __DFU_FLASH_MAP_H
#define __DFU_FLASH_MAP_H

#define FW_OTP_SEC_IMG2_OFFSET                (1) // x*256K
#define FW_OTA_SEC_MAX_IMG_LEN                (1) // x*256K

#define FW_HEADER_SW_VERSION_OFFSET           (0x08)
#define FW_HEADER_HASH_OFFSET                 (0x19)

#define FLASH_ADDR_BASE                       (0x80000000)

#define FLASH_START_ADDR                      (0  +  FLASH_ADDR_BASE)                              // 0x80000000
#define FLASH_MAX_SIZE                        (0x100000)                                         // 1M
#define FLASH_PAGE_SIZE                       (4096)
#define FLASH_ERASE_ONCE_MAX_PAGES            (1)  //一次性最大擦除页数
#define FLASH_INVALID_ADDR                    (0xFFFFFFFF)

//APP1 -------------
//APP1 ALL IMG
#define FLASH_APP1_ALL_IMG_ADDR                (FLASH_START_ADDR)
#define FLASH_APP1_ALL_IMG_SIZE                ((FW_OTP_SEC_IMG2_OFFSET*256*1024))

#define FLASH_APP1_EXIP_INFO_ADDR              (FLASH_APP1_ALL_IMG_ADDR)
#define FLASH_APP1_EXIP_INFO_SIZE              (0x400)

#define FLASH_APP1_NOR_CFG_ADDR                (FLASH_APP1_EXIP_INFO_ADDR + FLASH_APP1_EXIP_INFO_SIZE)
#define FLASH_APP1_NOR_CFG_SIZE                (0xC00)

#define FLASH_APP1_BOOT_HEAD_ADDR              (FLASH_APP1_NOR_CFG_ADDR + FLASH_APP1_NOR_CFG_SIZE)
#define FLASH_APP1_BOOT_HEAD_SIZE              (0x2000)

#define FLASH_APP1_USER_IMG_ADDR               (FLASH_APP1_BOOT_HEAD_ADDR + FLASH_APP1_BOOT_HEAD_SIZE)
#define FLASH_APP1_USER_IMG_SIZE               (FLASH_APP1_ALL_IMG_SIZE-(FLASH_APP1_EXIP_INFO_SIZE+FLASH_APP1_NOR_CFG_SIZE+FLASH_APP1_BOOT_HEAD_SIZE))
//-----------------------------

//APP2 --------------------
//APP1 ALL IMG
#define FLASH_APP2_ALL_IMG_ADDR                (FLASH_START_ADDR + FLASH_APP1_ALL_IMG_SIZE)
#define FLASH_APP2_ALL_IMG_SIZE                ((FW_OTP_SEC_IMG2_OFFSET*256*1024))

#define FLASH_APP2_EXIP_INFO_ADDR              (FLASH_APP2_ALL_IMG_ADDR)
#define FLASH_APP2_EXIP_INFO_SIZE              (0x400)

#define FLASH_APP2_NOR_CFG_ADDR                (FLASH_APP2_EXIP_INFO_ADDR + FLASH_APP2_EXIP_INFO_SIZE)
#define FLASH_APP2_NOR_CFG_SIZE                (0xC00)

#define FLASH_APP2_BOOT_HEAD_ADDR              (FLASH_APP2_NOR_CFG_ADDR + FLASH_APP2_NOR_CFG_SIZE)
#define FLASH_APP2_BOOT_HEAD_SIZE              (0x2000)

#define FLASH_APP2_USER_IMG_ADDR               (FLASH_APP2_BOOT_HEAD_ADDR + FLASH_APP2_BOOT_HEAD_SIZE)
#define FLASH_APP2_USER_IMG_SIZE               (FLASH_APP2_ALL_IMG_SIZE-(FLASH_APP2_EXIP_INFO_SIZE+FLASH_APP2_NOR_CFG_SIZE+FLASH_APP2_BOOT_HEAD_SIZE))
//------------------------

//APP1 IMGINFO,checksum todo
#define FLASH_APP1_IMGINFO_ADDR                (FLASH_APP2_USER_IMG_ADDR + FLASH_APP2_USER_IMG_SIZE)
#define FLASH_APP1_IMGINFO_SIZE                (0x1000)

//APP2 IMGINFO, checksum todo
#define FLASH_APP2_IMGINFO_ADDR                (FLASH_APP1_IMGINFO_ADDR + FLASH_APP1_IMGINFO_SIZE)
#define FLASH_APP2_IMGINFO_SIZE                (0x1000)



//customer para 
#define FLASH_CALIB_ADDR                       (FLASH_APP2_IMGINFO_ADDR + FLASH_APP2_IMGINFO_SIZE)
#define FLASH_CALIB_SIZE                       (0x1000)

#define FLASH_SYS_ADDR                         (FLASH_CALIB_ADDR + FLASH_CALIB_SIZE)
#define FLASH_SYS_SIZE                         (0x1000 * 2)

#define FLASH_SK_ADDR                          (FLASH_SYS_ADDR + FLASH_SYS_SIZE)
#define FLASH_SK_SIZE                          (0x1000)

#define FLASH_KR_ADDR                          (FLASH_SK_ADDR + FLASH_SK_SIZE)
#define FLASH_KR_SIZE                          (0x1000)

#define FLASH_MACRO_ADDR                       (FLASH_KR_ADDR + FLASH_KR_SIZE)
#define FLASH_MACRO_SIZE                       (0x1000)

#define FLASH_ONE_CFG_TOTAL_SIZE                (FLASH_SYS_SIZE + FLASH_SK_SIZE + FLASH_KR_SIZE + FLASH_MACRO_SIZE)
#define FLASH_ALL_CFG_TOTAL_SIZE                (FLASH_ONE_CFG_TOTAL_SIZE * 4)

#define FLASH_CFG_CACHE_ADDR                    ((FLASH_MACRO_ADDR + FLASH_MACRO_SIZE) + FLASH_ALL_CFG_TOTAL_SIZE)
#define FLASH_CFG_CACHE_SIZE                    (FLASH_ONE_CFG_TOTAL_SIZE) //20k

#define FLASH_COMMON_INFO_ADDR                  (FLASH_CFG_CACHE_ADDR + FLASH_CFG_CACHE_SIZE)
#define FLASH_COMMON_INFO_SIZE                  (0x1000) 

#define FLASH_PLACEHOLDER_ADDR                  (FLASH_COMMON_INFO_ADDR + FLASH_COMMON_INFO_SIZE)
#define FLASH_PLACEHOLDER_SIZE                  (0x18000) //k



//gm farmware
#define FLASH_GM_FW_INFO_ADDR                    (FLASH_PLACEHOLDER_ADDR + FLASH_PLACEHOLDER_SIZE)
#define FLASH_GM_FW_INFO_SIZE                    (0x1000)

#define FLASH_GM_FW_ADDR                        (FLASH_GM_FW_INFO_ADDR + FLASH_GM_FW_INFO_SIZE)
#define FLASH_GM_FW_SIZE                        (0xF000) //60k

#define FLASH_GM_FW2_INFO_ADDR                  (FLASH_GM_FW_ADDR + FLASH_GM_FW_SIZE)
#define FLASH_GM_FW2_INFO_SIZE                  (0x1000)

#define FLASH_GM_FW2_ADDR                       (FLASH_GM_FW2_INFO_ADDR + FLASH_GM_FW2_INFO_SIZE)
#define FLASH_GM_FW2_SIZE                       (0xF000) //60k

#define FLASH_BOARD_INFO_ADDR                   (FLASH_START_ADDR + FLASH_MAX_SIZE -  0x4000)
#define FLASH_BOARD_INFO_SIZE                   (0x1000)           // 4k

#define FLASH_FACTORY_ADDR                      (FLASH_BOARD_INFO_ADDR + FLASH_BOARD_INFO_SIZE)
#define FLASH_FACTORY_SIZE                      (0x1000)

#endif //__FLASH_MAP_H
