/**
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 */

#include "usb_config.h"

#ifndef WBVAL
#define WBVAL(x) (unsigned char)((x) & 0xFF), (unsigned char)(((x) >> 8) & 0xFF)
#endif

/*!< USBD CONFIG */
#define USBD_VERSION 0x0200
#define USBD_PRODUCT_VERSION 0x0001

#ifndef USBD_VID
#define USBD_VID 0x34b7
#else
//#define USBD_VID VENDOR_ID
#endif
#ifndef USBD_PID
#define USBD_PID 0xffff
#else
//#define USBD_PID PRODUCT_ID
#endif

#ifndef USBD_MAX_POWER
#define USBD_MAX_POWER 0xfa
#endif

#define USBD_LANGID_STRING 1033
#define USBD_CONFIG_DESCRIPTOR_SIZE 73

/*!< USBD ENDPOINT CONFIG */
#define USBD_IF0_AL0_EP0_ADDR 0x81
#define USBD_IF0_AL0_EP0_SIZE 0x20
#define USBD_IF0_AL0_EP0_INTERVAL 0x01

#define USBD_IF0_AL0_EP1_ADDR 0x02
#define USBD_IF0_AL0_EP1_SIZE 0x20
#define USBD_IF0_AL0_EP1_INTERVAL 0x01

#define USBD_IF1_AL0_EP0_ADDR 0x83
#define USBD_IF1_AL0_EP0_SIZE 0x20
#define USBD_IF1_AL0_EP0_INTERVAL 0x01

#define USBD_IF1_AL0_EP1_ADDR 0x04
#define USBD_IF1_AL0_EP1_SIZE 0x20
#define USBD_IF1_AL0_EP1_INTERVAL 0x01

/*!< USBD HID CONFIG */
#define USBD_HID_VERSION 0x0111
#define USBD_HID_COUNTRY_CODE 0
#define USBD_IF0_AL0_HID_REPORT_DESC_SIZE 231
#define USBD_IF1_AL0_HID_REPORT_DESC_SIZE 34

// clang-format off

/*!< USBD Descriptor */
const unsigned char usbd_descriptor[] = {
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
    0x02,                                       /*!< bNumInterfaces */
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
    /********************************************** Language ID String Descriptor */
    0x04,                                       /*!< bLength */
    0x03,                                       /*!< bDescriptorType */
    WBVAL(USBD_LANGID_STRING),                  /*!< wLangID0 */
    /********************************************** String 1 Descriptor */
                                                /* HPMicro */
    0x10,                                       /*!< bLength */
    0x03,                                       /*!< bDescriptorType */
    0x48, 0x00,                                 /*!< 'H' wcChar0 */
    0x50, 0x00,                                 /*!< 'P' wcChar1 */
    0x4d, 0x00,                                 /*!< 'M' wcChar2 */
    0x69, 0x00,                                 /*!< 'i' wcChar3 */
    0x63, 0x00,                                 /*!< 'c' wcChar4 */
    0x72, 0x00,                                 /*!< 'r' wcChar5 */
    0x6f, 0x00,                                 /*!< 'o' wcChar6 */
    /********************************************** String 2 Descriptor */
                                                /* Magnetic Keyboard */
    0x24,                                       /*!< bLength */
    0x03,                                       /*!< bDescriptorType */
    0x4d, 0x00,                                 /*!< 'M' wcChar0 */
    0x61, 0x00,                                 /*!< 'a' wcChar1 */
    0x67, 0x00,                                 /*!< 'g' wcChar2 */
    0x6e, 0x00,                                 /*!< 'n' wcChar3 */
    0x65, 0x00,                                 /*!< 'e' wcChar4 */
    0x74, 0x00,                                 /*!< 't' wcChar5 */
    0x69, 0x00,                                 /*!< 'i' wcChar6 */
    0x63, 0x00,                                 /*!< 'c' wcChar7 */
    0x20, 0x00,                                 /*!< ' ' wcChar8 */
    0x4b, 0x00,                                 /*!< 'K' wcChar9 */
    0x65, 0x00,                                 /*!< 'e' wcChar10 */
    0x79, 0x00,                                 /*!< 'y' wcChar11 */
    0x62, 0x00,                                 /*!< 'b' wcChar12 */
    0x6f, 0x00,                                 /*!< 'o' wcChar13 */
    0x61, 0x00,                                 /*!< 'a' wcChar14 */
    0x72, 0x00,                                 /*!< 'r' wcChar15 */
    0x64, 0x00,                                 /*!< 'd' wcChar16 */
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
    0x95, 0x20,       //   REPORT_COUNT (32)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0x09, 0x63,       //   USAGE (Raw Usage 0x63)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00, //   LOGICAL_MAXIMUM (255)
    0x95, 0x20,       //   REPORT_COUNT (32)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x91, 0x02,       //   OUTPUT (Data,Var,Abs)
    0xc0,             // END_COLLECTION
};

/*******************************************************END OF FILE*************/
