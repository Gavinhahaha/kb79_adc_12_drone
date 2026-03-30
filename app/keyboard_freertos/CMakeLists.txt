# Copyright (c) 2021-2023 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(CONFIG_CHERRYUSB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_HID 1)
set(CONFIG_FREERTOS 1)

set(CONFIG_HPM_SPI 1)
set(CONFIG_DMA_MGR 1)


# the following lines before "### END ###" are added during project localization
### START ###
set(ENV{HPM_SDK_BASE} "${CMAKE_CURRENT_SOURCE_DIR}/hpm_sdk_localized_for_hpm5300_basic")
set(LOCALIZED_BOARD "hpm5300_basic")
if(BOARD)
  if(NOT ${BOARD} MATCHES ${LOCALIZED_BOARD})
    message(FATAL_ERROR "ABORT:\n hpm sdk has been localized for ${LOCALIZED_BOARD} already.")
  endif()
endif()
set(BOARD ${LOCALIZED_BOARD})
### END ###


find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})
project(cherryusb_device_hid_keyboard_freertos)

sdk_inc(../../../config)
sdk_app_src(src/main.c)
sdk_app_src(src/hid_keyboard.c)

sdk_compile_definitions(-DUSE_NONVECTOR_MODE=1)
sdk_compile_definitions(-DDISABLE_IRQ_PREEMPTIVE=1)
generate_ide_projects()
