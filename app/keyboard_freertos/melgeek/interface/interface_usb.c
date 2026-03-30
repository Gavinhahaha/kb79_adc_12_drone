/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#define DBG_TAG "interface usb"

//#include "main.h"
//#include "log.h"
#include "interface.h"
#include "usbd_core.h"
#include "usbd_hid.h"
#include "usb_descriptor.c"
#include "hpm_gpio_drv.h"

extern void interface_usbd_evt_cb(uint8_t busid, uint8_t event);
extern void intf0_ep0_in_cb(uint8_t busid, uint8_t ep, uint32_t size);
extern void intf0_ep1_out_cb(uint8_t busid, uint8_t ep, uint32_t size);
extern void intf1_ep0_in_cb(uint8_t busid, uint8_t ep, uint32_t size);
extern void intf1_ep1_out_cb(uint8_t busid, uint8_t ep, uint32_t size);
extern void usbd_event_handler(uint8_t busid, uint8_t event);
extern void board_init_usb_pins(void);

static struct usbd_interface intf0;
static struct usbd_interface intf1;

static uint8_t keyboard_led[2];

void intf0_ep1_out_cb(uint8_t busid, uint8_t ep, uint32_t size)
{
    (void)size;
    usbd_ep_start_read(busid, ep, (void *)&keyboard_led, 2);
}

void intf0_ep0_in_cb(uint8_t busid, uint8_t ep, uint32_t size)
{
    (void)busid;
    (void)ep;
    (void)size;
    //tx_event_flags_set(&eventflags_usb, EVENTFLAGS_USB_INTF0_EP0_IN_IDLE, TX_OR);
}

void intf1_ep0_in_cb(uint8_t busid, uint8_t ep, uint32_t size)
{
    (void)busid;
    (void)ep;
    (void)size;
    //tx_event_flags_set(&eventflags_usb, EVENTFLAGS_USB_INTF0_EP0_IN_IDLE, TX_OR);
}

void intf1_ep1_out_cb(uint8_t busid, uint8_t ep, uint32_t size)
{
    (void)busid;
    (void)ep;
    (void)size;
    //tx_event_flags_set(&eventflags_usb, EVENTFLAGS_USB_INTF0_EP0_IN_IDLE, TX_OR);
}

struct usbd_endpoint intf0_ep0_in = {
    .ep_cb = intf0_ep0_in_cb,
    .ep_addr = USBD_IF0_AL0_EP0_ADDR,
};

struct usbd_endpoint intf0_ep1_out = {
    .ep_cb = intf0_ep1_out_cb,
    .ep_addr = USBD_IF0_AL0_EP1_ADDR,
};

struct usbd_endpoint intf1_ep0_in = {
    .ep_cb = intf1_ep0_in_cb,
    .ep_addr = USBD_IF1_AL0_EP0_ADDR,
};

struct usbd_endpoint intf1_ep1_out = {
    .ep_cb = intf1_ep1_out_cb,
    .ep_addr = USBD_IF1_AL0_EP1_ADDR,
};

int interface_usb_init(void)
{
    board_init_usb_pins();
    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);
    usbd_desc_register(USB_BUSID, usbd_descriptor);
    usbd_add_interface(USB_BUSID, usbd_hid_init_intf(USB_BUSID, &intf0, usbd_hid_0_report_descriptor, USBD_IF0_AL0_HID_REPORT_DESC_SIZE));
    usbd_add_interface(USB_BUSID, usbd_hid_init_intf(USB_BUSID, &intf1, usbd_hid_1_report_descriptor, USBD_IF1_AL0_HID_REPORT_DESC_SIZE));
    usbd_add_endpoint(USB_BUSID, &intf0_ep0_in);
    usbd_add_endpoint(USB_BUSID, &intf0_ep1_out);
    usbd_add_endpoint(USB_BUSID, &intf1_ep0_in);
    usbd_add_endpoint(USB_BUSID, &intf1_ep1_out);
    usbd_initialize(USB_BUSID, CONFIG_HPM_USBD_BASE, usbd_event_handler);

    return 0;
}

/*
 * Appendix G: HID Request Support Requirements
 *
 * The following table enumerates the requests that need to be supported by various types of HID class devices.
 * Device type     GetReport   SetReport   GetIdle     SetIdle     GetProtocol SetProtocol
 * ------------------------------------------------------------------------------------------
 * Boot Mouse      Required    Optional    Optional    Optional    Required    Required
 * Non-Boot Mouse  Required    Optional    Optional    Optional    Optional    Optional
 * Boot Keyboard   Required    Optional    Required    Required    Required    Required
 * Non-Boot Keybrd Required    Optional    Required    Required    Optional    Optional
 * Other Device    Required    Optional    Optional    Optional    Optional    Optional
 */

void usbd_hid_get_report(uint8_t busid, uint8_t intf, uint8_t report_id, uint8_t report_type, uint8_t **data, uint32_t *len)
{
    (void)busid;
    (void)intf;
    (void)report_id;
    (void)report_type;
    (*data[0]) = 0;
    *len = 1;
}

uint8_t usbd_hid_get_idle(uint8_t busid, uint8_t intf, uint8_t report_id)
{
    (void)busid;
    (void)intf;
    (void)report_id;
    return 0;
}

uint8_t usbd_hid_get_protocol(uint8_t busid, uint8_t intf)
{
    (void)busid;
    (void)intf;
    return 0;
}

void usbd_hid_set_report(uint8_t busid, uint8_t intf, uint8_t report_id, uint8_t report_type, uint8_t *report, uint32_t report_len)
{
    (void)busid;
    (void)intf;
    (void)report_id;
    (void)report_type;
    (void)report;
    (void)report_len;
}

void usbd_hid_set_idle(uint8_t busid, uint8_t intf, uint8_t report_id, uint8_t duration)
{
    (void)busid;
    (void)intf;
    (void)report_id;
    (void)duration;
}

void usbd_hid_set_protocol(uint8_t busid, uint8_t intf, uint8_t protocol)
{
    (void)busid;
    (void)intf;
    (void)protocol;
}

bool usb_report_get_protocol(void)
{
    return true;
}
