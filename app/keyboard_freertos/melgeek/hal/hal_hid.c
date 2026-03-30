/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
/* FreeRTOS kernel includes. */
#include "hal_hid.h"
#include "FreeRTOS.h"
#include "usb_osal.h"
#include "event_groups.h"

#include "usbd_hid.h"
#include "usb_config.h"
#include "board.h"
#include "hpm_gpio_drv.h"
#include "app_debug.h"
#include "usbd_core.h"
#include "dfu_api.h"
#include "rgb_lamp_array.h"
#include "power_save.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "hal_aid"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

/*!< USBD CONFIG */
#define USBD_VERSION                      0x0200
#define USBD_PRODUCT_VERSION              0x0101
// #ifndef VENDOR_ID
// #define USBD_VID                          0x34b7
// #else
// #define USBD_VID                          VENDOR_ID
// #endif
// #ifndef PRODUCT_ID
// #define USBD_PID                          0xffff
// #else
// #define USBD_PID                          PRODUCT_ID
// #endif
//#define USBD_MAX_POWER                    0xfa
#define USBD_LANGID_STRING                1033

#ifdef WINDOWS_LAMP
#define USBD_CONFIG_DESCRIPTOR_SIZE       98
#else
#define USBD_CONFIG_DESCRIPTOR_SIZE       73
#endif

/*!<    USBD                              ENDPOINT CONFIG */
#define USBD_IF0_AL0_EP0_ADDR             0x81
#define USBD_IF0_AL0_EP0_SIZE             0x20
#define USBD_IF0_AL0_EP0_INTERVAL         0x01

#define USBD_IF0_AL0_EP1_ADDR             0x02
#define USBD_IF0_AL0_EP1_SIZE             0x20
#define USBD_IF0_AL0_EP1_INTERVAL         0x01

#define USBD_IF1_AL0_EP0_ADDR             0x83
#define USBD_IF1_AL0_EP0_SIZE             0x40
#define USBD_IF1_AL0_EP0_INTERVAL         0x01

#define USBD_IF1_AL0_EP1_ADDR             0x04
#define USBD_IF1_AL0_EP1_SIZE             0x40
#define USBD_IF1_AL0_EP1_INTERVAL         0x01

#define USBD_IF2_AL0_EP0_ADDR             0x85
#define USBD_IF2_AL0_EP0_SIZE             0x40
#define USBD_IF2_AL0_EP0_INTERVAL         0x05

#define USBD_IF2_AL0_EP1_ADDR             0x06
#define USBD_IF2_AL0_EP1_SIZE             0x40
#define USBD_IF2_AL0_EP1_INTERVAL         0x01

#define USBD_HID_VERSION                  0x0111
#define USBD_HID_COUNTRY_CODE             0
#define USBD_IF0_AL0_HID_REPORT_DESC_SIZE 231
#define USBD_IF1_AL0_HID_REPORT_DESC_SIZE 34
#define USBD_IF2_AL0_HID_REPORT_DESC_SIZE 327

#ifdef WINDOWS_LAMP
#define USBD_DEVICE_PRODUCT_NAME_OFFSET   138
#else
#define USBD_DEVICE_PRODUCT_NAME_OFFSET   113
#endif

static PFN_HID_SOF_CALLBACK g_sof_callback = NULL;
static usbd_endpoint_callback cb_cust_out  = NULL;
static pfn_in_cb hid_cb_in                 = NULL;
static pfn_st_cb hid_cb_st                 = NULL;
static bool b_usb_ready = false;

#ifndef WBVAL
#define WBVAL(x) (unsigned char)((x) & 0xFF), (unsigned char)(((x) >> 8) & 0xFF)
#endif
// clang-format off

/*!< USBD Descriptor */
unsigned char usbd_descriptor[] = {
    /********************************************** Device Descriptor */
    0x12,                                       /*!< bLength */
    0x01,                                       /*!< bDescriptorType */
    WBVAL(USBD_VERSION),                        /*!< bcdUSB */
    0x00,                                       /*!< bDeviceClass */
    0x00,                                       /*!< bDeviceSubClass */
    0x00,                                       /*!< bDeviceProtocol */
    0x40,                                       /*!< bMaxPacketSize */
    WBVAL(USBD_VID),                            /*!< idVendor */
    WBVAL(USBD_PID),                            /*!< idProduct */
    WBVAL(USBD_PRODUCT_VERSION),                /*!< bcdDevice */
    0x01,                                       /*!< iManufacturer */
    0x02,                                       /*!< iProduct */
    0x03,                                       /*!< iSerial */
    0x01,                                       /*!< bNumConfigurations */
    /********************************************** Config Descriptor */
    0x09,                                       /*!< bLength */
    0x02,                                       /*!< bDescriptorType */
    WBVAL(USBD_CONFIG_DESCRIPTOR_SIZE),         /*!< wTotalLength */
    
#ifdef WINDOWS_LAMP
    0x03,                                       /*!< bNumInterfaces */
#else
    0x02,
#endif
    0x01,                                       /*!< bConfigurationValue */
    0x00,                                       /*!< iConfiguration */
    0xa0,                                       /*!< bmAttributes */
    USBD_MAX_POWER,                             /*!< bMaxPower */
    /********************************************** Interface 0 Alternate 0 Descriptor */
    0x09,                                       /*!< bLength */
    0x04,                                       /*!< bDescriptorType */
    0x00,                                       /*!< bInterfaceNumber */
    0x00,                                       /*!< bAlternateSetting */
    0x02,                                       /*!< bNumEndpoints */
    0x03,                                       /*!< bInterfaceClass */
    0x01,                                       /*!< bInterfaceSubClass */
    0x01,                                       /*!< bInterfaceProtocol */
    0x00,                                       /*!< iInterface */
    /********************************************** Class Specific Descriptor of HID */
    0x09,                                       /*!< bLength */
    0x21,                                       /*!< bDescriptorType */
    WBVAL(USBD_HID_VERSION),                    /*!< bcdHID */
    USBD_HID_COUNTRY_CODE,                      /*!< bCountryCode */
    0x01,                                       /*!< bNumDescriptors */
    0x22,                                       /*!< bDescriptorType */
    WBVAL(USBD_IF0_AL0_HID_REPORT_DESC_SIZE),   /*!< wItemLength */
    /********************************************** Endpoint 0 Descriptor */
    0x07,                                       /*!< bLength */
    0x05,                                       /*!< bDescriptorType */
    USBD_IF0_AL0_EP0_ADDR,                      /*!< bEndpointAddress */
    0x03,                                       /*!< bmAttributes */
    WBVAL(USBD_IF0_AL0_EP0_SIZE),               /*!< wMaxPacketSize */
    USBD_IF0_AL0_EP0_INTERVAL,                  /*!< bInterval */
    /********************************************** Endpoint 1 Descriptor */
    0x07,                                       /*!< bLength */
    0x05,                                       /*!< bDescriptorType */
    USBD_IF0_AL0_EP1_ADDR,                      /*!< bEndpointAddress */
    0x03,                                       /*!< bmAttributes */
    WBVAL(USBD_IF0_AL0_EP1_SIZE),               /*!< wMaxPacketSize */
    USBD_IF0_AL0_EP1_INTERVAL,                  /*!< bInterval */
    /********************************************** Interface 1 Alternate 0 Descriptor */
    0x09,                                       /*!< bLength */
    0x04,                                       /*!< bDescriptorType */
    0x01,                                       /*!< bInterfaceNumber */
    0x00,                                       /*!< bAlternateSetting */
    0x02,                                       /*!< bNumEndpoints */
    0x03,                                       /*!< bInterfaceClass */
    0x00,                                       /*!< bInterfaceSubClass */
    0x00,                                       /*!< bInterfaceProtocol */
    0x00,                                       /*!< iInterface */
    /********************************************** Class Specific Descriptor of HID */
    0x09,                                       /*!< bLength */
    0x21,                                       /*!< bDescriptorType */
    WBVAL(USBD_HID_VERSION),                    /*!< bcdHID */
    USBD_HID_COUNTRY_CODE,                      /*!< bCountryCode */
    0x01,                                       /*!< bNumDescriptors */
    0x22,                                       /*!< bDescriptorType */
    WBVAL(USBD_IF1_AL0_HID_REPORT_DESC_SIZE),   /*!< wItemLength */
    /********************************************** Endpoint 0 Descriptor */
    0x07,                                       /*!< bLength */
    0x05,                                       /*!< bDescriptorType */
    USBD_IF1_AL0_EP0_ADDR,                      /*!< bEndpointAddress */
    0x03,                                       /*!< bmAttributes */
    WBVAL(USBD_IF1_AL0_EP0_SIZE),               /*!< wMaxPacketSize */
    USBD_IF1_AL0_EP0_INTERVAL,                  /*!< bInterval */
    /********************************************** Endpoint 1 Descriptor */
    0x07,                                       /*!< bLength */
    0x05,                                       /*!< bDescriptorType */
    USBD_IF1_AL0_EP1_ADDR,                      /*!< bEndpointAddress */
    0x03,                                       /*!< bmAttributes */
    WBVAL(USBD_IF1_AL0_EP1_SIZE),               /*!< wMaxPacketSize */
    USBD_IF1_AL0_EP1_INTERVAL,                  /*!< bInterval */
#ifdef WINDOWS_LAMP
    /********************************************** Interface 2 Alternate 0 Descriptor */
    0x09,                                        // bLength
    0x04,                                        // bDescriptorType (Interface)
    0x02,                                        // bInterfaceNumber 0
    0x00,                                        // bAlternateSetting
    0x01,                                        // bNumEndpoints 1
    0x03,                                        // bInterfaceClass
    0x00,                                        // bInterfaceSubClass
    0x00,                                        // bInterfaceProtocol
    0x00,                                        // iInterface (String Index)

    0x09,                                        // bLength
    0x21,                                        // bDescriptorType (HID)
    WBVAL(USBD_HID_VERSION),                     // bcdHID 1.11
    USBD_HID_COUNTRY_CODE,                       // bCountryCode
    0x01,                                        // bNumDescriptors
    0x22,                                        // bDescriptorType[0] (HID)
    WBVAL(USBD_IF2_AL0_HID_REPORT_DESC_SIZE),    // wDescriptorLength[0] 327

    0x07,                                        // bLength
    0x05,                                        // bDescriptorType (Endpoint)
    USBD_IF2_AL0_EP0_ADDR,                       // bEndpointAddress (IN/D2H)
    0x03,                                        // bmAttributes (Interrupt)
    WBVAL(USBD_IF2_AL0_EP0_SIZE),                // wMaxPacketSize 64
    USBD_IF2_AL0_EP0_INTERVAL,                   // bInterval (unit depends on device speed)
#endif
    /********************************************** Language ID String Descriptor */
    0x04,                                       /*!< bLength */
    0x03,                                       /*!< bDescriptorType */
    WBVAL(USBD_LANGID_STRING),                  /*!< wLangID0 */
    /********************************************** String 1 Descriptor */
                                                /* HPMicro */
    0x10,                                       /*!< bLength */
    0x03,                                       /*!< bDescriptorType */
    'M', 0x00,                                 /*!< 'H' wcChar0 */
    'e', 0x00,                                 /*!< 'P' wcChar1 */
    'l', 0x00,                                 /*!< 'M' wcChar2 */
    'G', 0x00,                                 /*!< 'i' wcChar3 */
    'e', 0x00,                                 /*!< 'c' wcChar4 */
    'e', 0x00,                                 /*!< 'r' wcChar5 */
    'k', 0x00,                                 /*!< 'o' wcChar6 */
    /********************************************** String 2 Descriptor */
                                                /* Magnetic Keyboard */
    0x22,                                       /*!< bLength */
    0x03,                                       /*!< bDescriptorType */
    'C', 0x00,                                 /*!< '' wcChar0 */
    'E', 0x00,                                 /*!< '' wcChar1 */
    'N', 0x00,                                 /*!< '' wcChar2 */
    'T', 0x00,                                 /*!< '' wcChar3 */
    'A', 0x00,                                 /*!< '' wcChar4 */
    'U', 0x00,                                 /*!< '' wcChar5 */
    'R', 0x00,                                 /*!< '' wcChar6 */
    'I', 0x00,                                 /*!< '' wcChar7 */
    '8', 0x00,                                 /*!< '' wcChar8 */
    '0', 0x00,                                 /*!< '' wcChar9 */
    0x00, 0x00,                                 /*!< '' wcCharA */
    0x00, 0x00,                                 /*!< '' wcCharB */
    0x00, 0x00,                                 /*!< '' wcCharC */
    0x00, 0x00,                                 /*!< '' wcCharD */
    0x00, 0x00,                                 /*!< '' wcCharE */
    0x00, 0x00,                                 /*!< '' wcCharF */
    /********************************************** String 3 Descriptor */
                                                /* 0000000000000000 */
    0x22,                                       /*!< bLength */
    0x03,                                       /*!< bDescriptorType */
    0x30, 0x00,                                 /*!< '0' wcChar0 */
    0x30, 0x00,                                 /*!< '0' wcChar1 */
    0x30, 0x00,                                 /*!< '0' wcChar2 */
    0x30, 0x00,                                 /*!< '0' wcChar3 */
    0x30, 0x00,                                 /*!< '0' wcChar4 */
    0x30, 0x00,                                 /*!< '0' wcChar5 */
    0x30, 0x00,                                 /*!< '0' wcChar6 */
    0x30, 0x00,                                 /*!< '0' wcChar7 */
    0x30, 0x00,                                 /*!< '0' wcChar8 */
    0x30, 0x00,                                 /*!< '0' wcChar9 */
    0x30, 0x00,                                 /*!< '0' wcChar10 */
    0x30, 0x00,                                 /*!< '0' wcChar11 */
    0x30, 0x00,                                 /*!< '0' wcChar12 */
    0x30, 0x00,                                 /*!< '0' wcChar13 */
    0x30, 0x00,                                 /*!< '0' wcChar14 */
    0x30, 0x00,                                 /*!< '0' wcChar15 */
    /********************************************** Device Qualifier Descriptor */
    0x0a,                                       /*!< bLength */
    0x06,                                       /*!< bDescriptorType */
    WBVAL(USBD_VERSION),                        /*!< bcdUSB */
    0x00,                                       /*!< bDeviceClass */
    0x00,                                       /*!< bDeviceSubClass */
    0x00,                                       /*!< bDeviceProtocol */
    0x40,                                       /*!< bMaxPacketSize0 */
    0x01,                                       /*!< bNumConfigurations */
    0x00,                                       /*!< bReserved */
    0x00,
};

/*!< USBD HID REPORT 0 Descriptor */
const unsigned char usbd_hid_0_report_descriptor[USBD_IF0_AL0_HID_REPORT_DESC_SIZE] = {
    0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,       // USAGE (Keyboard)
    0xa1, 0x01,       // COLLECTION (Application)
    0x85, 0x01,       //   REPORT_ID (1)
    0x05, 0x07,       //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,       //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,       //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x25, 0x01,       //   LOGICAL_MAXIMUM (1)
    0x95, 0x08,       //   REPORT_COUNT (8)
    0x75, 0x01,       //   REPORT_SIZE (1)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0x95, 0x01,       //   REPORT_COUNT (1)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x81, 0x01,       //   INPUT (Cnst,Ary,Abs)
    0x05, 0x07,       //   USAGE_PAGE (Keyboard)
    0x19, 0x00,       //   USAGE_MINIMUM (0)
    0x29, 0xf0,       //   USAGE_MAXIMUM (239)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x25, 0xf0,       //   LOGICAL_MAXIMUM (239)
    0x95, 0x06,       //   REPORT_COUNT (6)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x81, 0x00,       //   INPUT (Data,Ary,Abs)
    0x05, 0x08,       //   USAGE_PAGE (LEDs)
    0x19, 0x01,       //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,       //   USAGE_MAXIMUM (Kana)
    0x95, 0x05,       //   REPORT_COUNT (5)
    0x75, 0x01,       //   REPORT_SIZE (1)
    0x91, 0x02,       //   OUTPUT (Data,Var,Abs)
    0x95, 0x01,       //   REPORT_COUNT (1)
    0x75, 0x03,       //   REPORT_SIZE (3)
    0x91, 0x01,       //   OUTPUT (Cnst,Ary,Abs)
    0xc0,             // END_COLLECTION
    0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,       // USAGE (Mouse)
    0xa1, 0x01,       // COLLECTION (Application)
    0x85, 0x02,       //   REPORT_ID (2)
    0x09, 0x01,       //   USAGE (Pointer)
    0xa1, 0x00,       //   COLLECTION (Physical)
    0x05, 0x09,       //     USAGE_PAGE (Button)
    0x19, 0x01,       //     USAGE_MINIMUM (Button 1)
    0x29, 0x08,       //     USAGE_MAXIMUM (Button 8)
    0x15, 0x00,       //     LOGICAL_MINIMUM (0)
    0x25, 0x01,       //     LOGICAL_MAXIMUM (1)
    0x95, 0x08,       //     REPORT_COUNT (8)
    0x75, 0x01,       //     REPORT_SIZE (1)
    0x81, 0x02,       //     INPUT (Data,Var,Abs)
    0x05, 0x01,       //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,       //     USAGE (X)
    0x09, 0x31,       //     USAGE (Y)
    0x15, 0x81,       //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,       //     LOGICAL_MAXIMUM (127)
    0x95, 0x02,       //     REPORT_COUNT (2)
    0x75, 0x08,       //     REPORT_SIZE (8)
    0x81, 0x06,       //     INPUT (Data,Var,Rel)
    0x05, 0x01,       //     USAGE_PAGE (Generic Desktop)
    0x09, 0x38,       //     USAGE (Wheel)
    0x15, 0x81,       //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,       //     LOGICAL_MAXIMUM (127)
    0x95, 0x01,       //     REPORT_COUNT (1)
    0x75, 0x08,       //     REPORT_SIZE (8)
    0x81, 0x06,       //     INPUT (Data,Var,Rel)
    0x05, 0x0c,       //     USAGE_PAGE (Consumer Devices)
    0x0a, 0x38, 0x02, //     USAGE (AC PAN)
    0x15, 0x81,       //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,       //     LOGICAL_MAXIMUM (127)
    0x95, 0x01,       //     REPORT_COUNT (1)
    0x75, 0x08,       //     REPORT_SIZE (8)
    0x81, 0x06,       //     INPUT (Data,Var,Rel)
    0xc0,             //     END_COLLECTION
    0xc0,             // END_COLLECTION
    0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
    0x09, 0x80,       // USAGE (System Control)
    0xa1, 0x01,       // COLLECTION (Application)
    0x85, 0x03,       //   REPORT_ID (3)
    0x19, 0x01,       //   USAGE_MINIMUM (1)
    0x2a, 0xb7, 0x00, //   USAGE_MAXIMUM (0xb7)
    0x15, 0x01,       //   LOGICAL_MINIMUM (1)
    0x26, 0xb7, 0x00, //   LOGICAL_MAXIMUM (0xb7)
    0x95, 0x01,       //   REPORT_COUNT (1)
    0x75, 0x10,       //   REPORT_SIZE (16)
    0x81, 0x00,       //   INPUT (Data,Ary,Abs)
    0xc0,             // END_COLLECTION
    0x05, 0x0c,       // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,       // USAGE (Consumer Control)
    0xa1, 0x01,       // COLLECTION (Application)
    0x85, 0x04,       //   REPORT_ID (4)
    0x19, 0x01,       //   USAGE_MINIMUM (0)
    0x2a, 0xa0, 0x02, //   USAGE_MAXIMUM (0x2a0)
    0x15, 0x01,       //   LOGICAL_MINIMUM (0)
    0x26, 0xa0, 0x02, //   LOGICAL_MAXIMUM (0x2a0)
    0x95, 0x01,       //   REPORT_COUNT (1)
    0x75, 0x10,       //   REPORT_SIZE (16)
    0x81, 0x00,       //   INPUT (Data,Ary,Abs)
    0xc0,             // END_COLLECTION
    0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,       // USAGE (Keyboard)
    0xa1, 0x01,       // COLLECTION (Application)
    0x85, 0x06,       //   REPORT_ID (6)
    0x05, 0x07,       //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,       //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,       //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x25, 0x01,       //   LOGICAL_MAXIMUM (1)
    0x95, 0x08,       //   REPORT_COUNT (8)
    0x75, 0x01,       //   REPORT_SIZE (1)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0x05, 0x07,       //   USAGE_PAGE (Keyboard)
    0x19, 0x00,       //   USAGE_MINIMUM (0)
    0x29, 0xf0,       //   USAGE_MAXIMUM (240)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x25, 0x01,       //   LOGICAL_MAXIMUM (1)
    0x95, 0xf0,       //   REPORT_COUNT (240)
    0x75, 0x01,       //   REPORT_SIZE (1)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0xc0,             // END_COLLECTION
};

/*!< USBD HID REPORT 1 Descriptor */
const unsigned char usbd_hid_1_report_descriptor[USBD_IF1_AL0_HID_REPORT_DESC_SIZE] = {
    0x06, 0x60, 0xff, // USAGE_PAGE (Vendor Defined Page Raw)
    0x09, 0x61,       // USAGE (Raw Usage 0x61)
    0xa1, 0x01,       // COLLECTION (Application)
    0x09, 0x62,       //   USAGE (Raw Usage 0x62)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00, //   LOGICAL_MAXIMUM (255)
    0x95, 0x40,       //   REPORT_COUNT (32)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0x09, 0x63,       //   USAGE (Raw Usage 0x63)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00, //   LOGICAL_MAXIMUM (255)
    0x95, 0x40,       //   REPORT_COUNT (32)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x91, 0x02,       //   OUTPUT (Data,Var,Abs)
    0xc0,             // END_COLLECTION
};


/*!< USBD HID REPORT 2 Descriptor */
const unsigned char usbd_hid_2_report_descriptor[USBD_IF2_AL0_HID_REPORT_DESC_SIZE] = {
    0x05, 0x59,        // Usage Page (0x59)
    0x09, 0x01,        // Usage (0x01)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (2)
    0x09, 0x02,        //   Usage (0x02)
    0xA1, 0x02,        //   Collection (Logical)
    0x09, 0x03,        //     Usage (0x03)
    0x15, 0x00,        //     Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00,  //     Logical Maximum (65534)
    0x75, 0x10,        //     Report Size (16)
    0x95, 0x01,        //     Report Count (1)
    0xB1, 0x03,        //     Feature (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x09, 0x04,        //     Usage (0x04)
    0x09, 0x05,        //     Usage (0x05)
    0x09, 0x06,        //     Usage (0x06)
    0x09, 0x07,        //     Usage (0x07)
    0x09, 0x08,        //     Usage (0x08)
    0x15, 0x00,        //     Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //     Logical Maximum (2147483646)
    0x75, 0x20,        //     Report Size (32)
    0x95, 0x05,        //     Report Count (5)
    0xB1, 0x03,        //     Feature (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              //   End Collection
    0x85, 0x02,        //   Report ID (3)
    0x09, 0x20,        //   Usage (0x20)
    0xA1, 0x02,        //   Collection (Logical)
    0x09, 0x21,        //     Usage (0x21)
    0x15, 0x00,        //     Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00,  //     Logical Maximum (65534)
    0x75, 0x10,        //     Report Size (16)
    0x95, 0x01,        //     Report Count (1)
    0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              //   End Collection
    0x85, 0x03,        //   Report ID (4)
    0x09, 0x22,        //   Usage (0x22)
    0xA1, 0x02,        //   Collection (Logical)
    0x09, 0x21,        //     Usage (0x21)
    0x15, 0x00,        //     Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00,  //     Logical Maximum (65534)
    0x75, 0x10,        //     Report Size (16)
    0x95, 0x01,        //     Report Count (1)
    0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x09, 0x23,        //     Usage (0x23)
    0x09, 0x24,        //     Usage (0x24)
    0x09, 0x25,        //     Usage (0x25)
    0x09, 0x27,        //     Usage (0x27)
    0x09, 0x26,        //     Usage (0x26)
    0x15, 0x00,        //     Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //     Logical Maximum (2147483646)
    0x75, 0x20,        //     Report Size (32)
    0x95, 0x05,        //     Report Count (5)
    0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x09, 0x28,        //     Usage (0x28)
    0x09, 0x29,        //     Usage (0x29)
    0x09, 0x2A,        //     Usage (0x2A)
    0x09, 0x2B,        //     Usage (0x2B)
    0x09, 0x2C,        //     Usage (0x2C)
    0x09, 0x2D,        //     Usage (0x2D)
    0x15, 0x00,        //     Logical Minimum (0)
    0x26, 0xFF, 0x00,  //     Logical Maximum (255)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x06,        //     Report Count (6)
    0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              //   End Collection
    0x85, 0x04,        //   Report ID (5)
    0x09, 0x50,        //   Usage (0x50)
    0xA1, 0x02,        //   Collection (Logical)
    0x09, 0x03,        //     Usage (0x03)
    0x09, 0x55,        //     Usage (0x55)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x08,        //     Logical Maximum (8)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x02,        //     Report Count (2)
    0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x09, 0x21,        //     Usage (0x21)
    0x15, 0x00,        //     Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00,  //     Logical Maximum (65534)
    0x75, 0x10,        //     Report Size (16)
    0x95, 0x08,        //     Report Count (8)
    0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x09, 0x51,        //     Usage (0x51)
    0x09, 0x52,        //     Usage (0x52)
    0x09, 0x53,        //     Usage (0x53)
    0x09, 0x54,        //     Usage (0x54)
    0x09, 0x51,        //     Usage (0x51)
    0x09, 0x52,        //     Usage (0x52)
    0x09, 0x53,        //     Usage (0x53)
    0x09, 0x54,        //     Usage (0x54)
    0x09, 0x51,        //     Usage (0x51)
    0x09, 0x52,        //     Usage (0x52)
    0x09, 0x53,        //     Usage (0x53)
    0x09, 0x54,        //     Usage (0x54)
    0x09, 0x51,        //     Usage (0x51)
    0x09, 0x52,        //     Usage (0x52)
    0x09, 0x53,        //     Usage (0x53)
    0x09, 0x54,        //     Usage (0x54)
    0x09, 0x51,        //     Usage (0x51)
    0x09, 0x52,        //     Usage (0x52)
    0x09, 0x53,        //     Usage (0x53)
    0x09, 0x54,        //     Usage (0x54)
    0x09, 0x51,        //     Usage (0x51)
    0x09, 0x52,        //     Usage (0x52)
    0x09, 0x53,        //     Usage (0x53)
    0x09, 0x54,        //     Usage (0x54)
    0x09, 0x51,        //     Usage (0x51)
    0x09, 0x52,        //     Usage (0x52)
    0x09, 0x53,        //     Usage (0x53)
    0x09, 0x54,        //     Usage (0x54)
    0x09, 0x51,        //     Usage (0x51)
    0x09, 0x52,        //     Usage (0x52)
    0x09, 0x53,        //     Usage (0x53)
    0x09, 0x54,        //     Usage (0x54)
    0x15, 0x00,        //     Logical Minimum (0)
    0x26, 0xFF, 0x00,  //     Logical Maximum (255)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x20,        //     Report Count (32)
    0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              //   End Collection
    0x85, 0x05,        //   Report ID (6)
    0x09, 0x60,        //   Usage (0x60)
    0xA1, 0x02,        //   Collection (Logical)
    0x09, 0x55,        //     Usage (0x55)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x08,        //     Logical Maximum (8)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x01,        //     Report Count (1)
    0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x09, 0x61,        //     Usage (0x61)
    0x09, 0x62,        //     Usage (0x62)
    0x15, 0x00,        //     Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00,  //     Logical Maximum (65534)
    0x75, 0x10,        //     Report Size (16)
    0x95, 0x02,        //     Report Count (2)
    0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x09, 0x51,        //     Usage (0x51)
    0x09, 0x52,        //     Usage (0x52)
    0x09, 0x53,        //     Usage (0x53)
    0x09, 0x54,        //     Usage (0x54)
    0x15, 0x00,        //     Logical Minimum (0)
    0x26, 0xFF, 0x00,  //     Logical Maximum (255)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x04,        //     Report Count (4)
    0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              //   End Collection
    0x85, 0x06,        //   Report ID (7)
    0x09, 0x70,        //   Usage (0x70)
    0xA1, 0x02,        //   Collection (Logical)
    0x09, 0x71,        //     Usage (0x71)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x01,        //     Report Count (1)
    0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              //   End Collection
    0xC0,              // End Collection

};

/*******************************************************END OF FILE*************/




usb_osal_sem_t semaphore_cinout_done;
usb_osal_sem_t semaphore_nkro_done;

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[USBD_IF1_AL0_EP1_SIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read0_buffer[USBD_IF0_AL0_EP1_SIZE];

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    (void)busid;

    switch (event)
    {
    case USBD_EVENT_ERROR:
        DBG_PRINTF("ERROR\r\n");
        break;
    case USBD_EVENT_RESET:
        DBG_PRINTF("RESET\r\n");
        //xEventGroupSetBitsFromISR(Event_Handle, USB_EVENT_RESET, &xHigherPriorityTaskWoken);
        break;
        
    case USBD_EVENT_SOF:
        if (g_sof_callback)
        {
            g_sof_callback();
        }
        break;
        
    case USBD_EVENT_CONNECTED:
        DBG_PRINTF("CONNECTED\r\n");
        b_usb_ready = true;
        //xEventGroupSetBitsFromISR(Event_Handle, USB_EVENT_CONNECTED, &xHigherPriorityTaskWoken);
        break;
        
    case USBD_EVENT_DISCONNECTED:
        DBG_PRINTF("DISCONNECTED\r\n");
        //xEventGroupSetBitsFromISR(Event_Handle, USB_EVENT_DISCONNECTED, &xHigherPriorityTaskWoken);
        break;
        
    case USBD_EVENT_RESUME:
        DBG_PRINTF("RESUME\r\n");
        //xEventGroupSetBitsFromISR(Event_Handle, USB_EVENT_RESUME, &xHigherPriorityTaskWoken);
        break;
        
    case USBD_EVENT_SUSPEND:
        DBG_PRINTF("SUSPEND\r\n");
        b_usb_ready = false;                                             
        //xEventGroupSetBitsFromISR(Event_Handle, USB_EVENT_SUSPEND, &xHigherPriorityTaskWoken);
        break;
        
    case USBD_EVENT_CONFIGURED:
        DBG_PRINTF("CONFIGURED\r\n");
        //xEventGroupSetBitsFromISR(Event_Handle, USB_EVENT_CONFIGURED, &xHigherPriorityTaskWoken);
        /* setup first out ep read transfer */
        usbd_ep_start_read(busid, USBD_IF1_AL0_EP1_ADDR, read_buffer, USBD_IF1_AL0_EP1_SIZE);
        usbd_ep_start_read(busid, USBD_IF0_AL0_EP1_ADDR, read0_buffer, USBD_IF0_AL0_EP1_SIZE);
        break;
    
    case USBD_EVENT_SET_INTERFACE:
        DBG_PRINTF("SET_INTERFACE\r\n");
        break;
    
    case USBD_EVENT_SET_REMOTE_WAKEUP:
        DBG_PRINTF("SET_REMOTE_WAKEUP\r\n");
        break;
        
    case USBD_EVENT_CLR_REMOTE_WAKEUP:
        DBG_PRINTF("CLR_REMOTE_WAKEUP\r\n");

        break;
        
    case USBD_EVENT_INIT:
        DBG_PRINTF("INIT\r\n");
        break;
    
    case USBD_EVENT_DEINIT:
        DBG_PRINTF("DEINIT\r\n");
        break;

    default:
        DBG_PRINTF("UNKNOWN 0x%02x\r\n", event);
        break;
    }
}

/*!< hid state ! Data can be sent only when state is idle  */
static volatile uint8_t custom_state;
static void usbd_hid_custom_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;
    (void)nbytes;
    

    if (hid_cb_in)
    {
        hid_cb_in();
    }
    //DBG_PRINTF("c in len:%d\r\n", nbytes);
}

static void usbd_hid_custom_out_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    DBG_PRINTF("c out len:%d\r\n", nbytes);
    usbd_ep_start_read(busid, ep, read_buffer, 64);
    read_buffer[0] = 0x02; /* IN: report id */
    usbd_ep_start_write(busid, USBD_IF1_AL0_EP0_ADDR, read_buffer, nbytes);
}
static void usbd_hid_if0_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;
    (void)nbytes;
    //BOARD_TEST_PIN2_WRITE(0);
    if (hid_cb_in)
    {
        hid_cb_in();
    }

    //DBG_PRINTF("nkro in len:%d\r\n", nbytes);
}

static void usbd_hid_if0_out_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    
    usbd_ep_start_read(busid, ep, read0_buffer, USBD_IF0_AL0_EP1_SIZE);
    DBG_PRINTF("kb out len:%d\r\n", nbytes);
    if (hid_cb_st)
    {
        hid_cb_st(read0_buffer, nbytes);
    }
    
    // read0_buffer[0] = 0x03; /* IN: report id */
    // usbd_ep_start_write(busid, USBD_IF0_AL0_EP0_ADDR, read0_buffer, nbytes);
}

static void usbd_hid_if2_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;
    (void)nbytes;
}



static struct usbd_endpoint intf0_ep0_in = {
    .ep_cb = usbd_hid_if0_in_callback,
    .ep_addr = USBD_IF0_AL0_EP0_ADDR};

static struct usbd_endpoint intf0_ep1_out = {
    .ep_cb = usbd_hid_if0_out_callback,
    .ep_addr = USBD_IF0_AL0_EP1_ADDR};

static struct usbd_endpoint intf1_ep0_in = {
    .ep_cb = usbd_hid_custom_in_callback,
    .ep_addr = USBD_IF1_AL0_EP0_ADDR};

static struct usbd_endpoint intf1_ep1_out = {
    .ep_cb = usbd_hid_custom_out_callback,
    .ep_addr = USBD_IF1_AL0_EP1_ADDR};

static struct usbd_endpoint intf2_ep0_in = {
    .ep_cb = usbd_hid_if2_in_callback,
    .ep_addr = USBD_IF2_AL0_EP0_ADDR};

// static struct usbd_endpoint intf2_ep1_out = {
//     .ep_cb = usbd_hid_if2_out_callback,
//     .ep_addr = USBD_IF2_AL0_EP1_ADDR};


struct usbd_interface intf0;

struct usbd_interface intf1;

struct usbd_interface intf2;


void hal_hid_set_sof_callback(PFN_HID_SOF_CALLBACK pfn_cb)
{
    g_sof_callback = pfn_cb;
}

void hal_hid_set_out_callback(usbd_endpoint_callback pfn_cb)
{
    cb_cust_out = pfn_cb;
}

int hal_hid_read_data(uint8_t *pbuf, uint32_t size)
{
    int ret;
    ret = usbd_ep_start_read(0, USBD_IF1_AL0_EP1_ADDR, read_buffer, size);
    
    // for (uint8_t i = 0;i < size; i++)
    // {
    //     USB_LOG_RAW("%x,", read_buffer[i]);
    // }
    // USB_LOG_RAW("len %d\r\n", size);
    memcpy(pbuf, read_buffer, size);
    
    return ret;
}

int hal_hid_write_data(uint8_t *pbuf, uint32_t size)
{
    if (usb_osal_sem_take(semaphore_cinout_done, portMAX_DELAY) != 0)
    {
        DBG_PRINTF("usb_osal_sem_take error %d\r\n", __LINE__);
    }
    return usbd_ep_start_write(0, USBD_IF1_AL0_EP0_ADDR, pbuf, size);
}

int hal_hid_send_report(rep_hid_t *report)
{
    int      ret;
    uint32_t len;
    const uint8_t report_size[10]=
    {
        0,//REPORT_ID_NULL                =0,
        9,//REPORT_ID_KEYBOARD            = 1,
        6,//REPORT_ID_MOUSE               = 2,
        65,//REPORT_ID_CONSUMER           = 10,
        3,//REPORT_ID_SYSTEM              = 4,
        0,//REPORT_ID_PROGRAMMABLE_BUTTON = 5,
        32,//REPORT_ID_NKRO                = 6,
        0,//REPORT_ID_JOYSTICK            = 7,
        0,//REPORT_ID_DIGITIZER           = 8,
        0//REPORT_ID_DIAL                 = 9,
    };

    if (false == b_usb_ready)
    {
        usbd_send_remote_wakeup(0);
        memset((uint8_t *)report + 1, 0, sizeof(rep_hid_t) - 1);
    }
    
    if (report->report_id == REPORT_ID_CONSUMER) 
    {
        len = report->hive.payload.len + 8;
        if (len > 64)
        {
            DBG_PRINTF("hal_hid_send_report:len err=%d\n", len);
            return -1;
        }
        ret = usbd_ep_start_write(0, USBD_IF1_AL0_EP0_ADDR, (const uint8_t *)&(report->hive.payload), USBD_IF1_AL0_EP0_SIZE);
    }
    else
    {
        //ep = USBD_IF0_AL0_EP0_ADDR;
        //len = report_size[report->report_id];
        // BOARD_TEST_PIN2_TOGGLE();
        ret = usbd_ep_start_write(0, USBD_IF0_AL0_EP0_ADDR, report->raw, report_size[report->report_id]);

    }

    return ret;
}


#ifdef WINDOWS_LAMP
void usbd_hid_get_report(uint8_t busid, uint8_t intf, uint8_t report_id, uint8_t report_type, uint8_t **data, uint32_t *len)
{
    
    (void)busid;
    (void)intf;
    (void)report_type;
    switch (report_id) 
    {
        case REPORT_ID_LIGHTING_LAMP_ARRAY_ATTRIBUTES: {
            *data[0] = report_id;
            *len     = get_lamp_array_attributes_report(data) + 1;
            break;
        }
        case REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_RESPONSE: { 
            *data[0] = report_id;
            *len     = get_lamp_attributes_report(data) + 1;
            break;
        }
        default: {
            break;
        }
    }
}

void usbd_hid_set_report(uint8_t busid, uint8_t intf, uint8_t report_id, uint8_t report_type, uint8_t *report, uint32_t report_len)
{
    (void)busid;
    (void)intf;
    (void)report_type;
    (void)report_len;

    ps_timer_reset_later();

    switch (report_id) 
    {
        case REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_REQUEST: {
            
            set_lamp_attributesId(&report[1]);
            break;
        }
        case REPORT_ID_LIGHTING_LAMP_MULTI_UPDATE: {
            set_multiple_lamps(&report[1]);
            break;   
        }
        case REPORT_ID_LIGHTING_LAMP_RANGE_UPDATE: {
            set_lamp_range(&report[1]);
            break;   
        }
        case REPORT_ID_LIGHTING_LAMP_ARRAY_CONTROL: {
            set_autonomous_mode(&report[1]);
            break;
        }
        default: {
            break;
        }
    }
}
#endif

void hal_hid_start(void)
{
    
}

void hal_hid_stop(void)
{
    
}

bool hal_hid_is_ready(void)
{
    return b_usb_ready;
}

void hal_hid_set_cb_in(pfn_in_cb cb)
{
    hid_cb_in = cb;
}

void hal_hid_set_cb_st(pfn_st_cb cb)
{
    hid_cb_st = cb;
}

void hal_hid_name(void)
{
    user_product_header_t product_info;

    ota_board_flash_read(FLASH_BOARD_INFO_ADDR, &product_info, sizeof(user_product_header_t));

    if (product_info.magic == BOARD_SN_NAME_MAGIC)
    {
        for (uint16_t i = 0; i < product_info.len; i++)
        {
            usbd_descriptor[USBD_DEVICE_PRODUCT_NAME_OFFSET+i*2] = product_info.device_name[i];
        }
        
    }
    else if(product_info.magic == BOARD_SN_NAME_MAGIC2)
    {
        for (uint8_t i = 0; i < 16; i++)
        {
            if (((uint8_t*)&product_info)[i] == 0x00)
            {
                break;
            }
            
            usbd_descriptor[USBD_DEVICE_PRODUCT_NAME_OFFSET+i*2] = ((uint8_t*)&product_info)[i];
        }
        
    }

}

void hal_hid_init(void)
{
    board_init_usb_pins();

    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 4);

    hal_hid_name(); 
    usbd_desc_register(0, usbd_descriptor);
    usbd_add_interface(0, usbd_hid_init_intf(0, &intf0, usbd_hid_0_report_descriptor, USBD_IF0_AL0_HID_REPORT_DESC_SIZE));
    usbd_add_interface(0, usbd_hid_init_intf(0, &intf1, usbd_hid_1_report_descriptor, USBD_IF1_AL0_HID_REPORT_DESC_SIZE));
    
#ifdef WINDOWS_LAMP
    usbd_add_interface(0, usbd_hid_init_intf(0, &intf2, usbd_hid_2_report_descriptor, USBD_IF2_AL0_HID_REPORT_DESC_SIZE));
#endif
    usbd_add_endpoint(0, &intf0_ep0_in);
    usbd_add_endpoint(0, &intf0_ep1_out);
    intf1_ep1_out.ep_cb = cb_cust_out;
    usbd_add_endpoint(0, &intf1_ep0_in);
    usbd_add_endpoint(0, &intf1_ep1_out);
    
#ifdef WINDOWS_LAMP
    usbd_add_endpoint(0, &intf2_ep0_in);
#endif

    usbd_initialize(0, CONFIG_HPM_USBD_BASE, usbd_event_handler);
    
    DBG_PRINTF("initialize usb\r\n");
}
