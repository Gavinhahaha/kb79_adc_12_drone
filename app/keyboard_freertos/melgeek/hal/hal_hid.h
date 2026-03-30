/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#ifndef _HAL_HID_H_
#define _HAL_HID_H_


#include "usbd_core.h"
#include "interface.h"




typedef void (*PFN_HID_SOF_CALLBACK)(void);
typedef void (*pfn_in_cb)(void);
typedef void (*pfn_st_cb)(uint8_t *pbuf, uint16_t size);

typedef enum
{
    USB_CB_CUSTOM_IN    = 0,
    USB_CB_CUSTOM_OUT   = 1,
    USB_CB_MAX
} usb_cb_t; 
    
#define USB_EVENT_RESET                 (0x01 << 0)
#define USB_EVENT_CONNECTED             (0x01 << 1)//设置事件掩码的位 0
#define USB_EVENT_DISCONNECTED          (0x01 << 2)//设置事件掩码的位 1
#define USB_EVENT_RESUME                (0x01 << 3)
#define USB_EVENT_SUSPEND               (0x01 << 4)
#define USB_EVENT_CONFIGURED            (0x01 << 5)
#define USB_EVENT_SOF                   (0x01 << 6)



void hal_hid_set_sof_callback(PFN_HID_SOF_CALLBACK pfn_cb);
void hal_hid_set_out_callback(usbd_endpoint_callback pfn_cb);
int hal_hid_read_data(uint8_t *pbuf, uint32_t size);
int hal_hid_write_data(uint8_t *pbuf, uint32_t size);
int hal_hid_kb_report_send(uint8_t *buf, uint8_t len);
void hal_hid_ms_report_send(uint8_t *buf, uint8_t len);
void hal_hid_sys_report_send(uint8_t *buf, uint8_t len);
void hal_hid_vd_send_report(uint8_t *buf, uint8_t len);
int hal_hid_send_nkro(rep_hid_t *report);
void hal_hid_set_cb_in(pfn_in_cb cb);
void hal_hid_set_cb_st(pfn_st_cb cb);
int hal_hid_send_report(rep_hid_t *report);
bool hal_hid_is_ready(void);
void hal_hid_init(void);

#endif
