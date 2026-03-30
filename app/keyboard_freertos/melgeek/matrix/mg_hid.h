#ifndef __MG_HID_H__
#define __MG_HID_H__

#include "queue.h"
#include "interface.h"

typedef enum
{
    HID_RET_SUCEESS         = 0,
    HID_RET_INIT_FAILSE     = (MG_ERR_BASE_HID + 0),
    HID_RET_W_QUEUE_FULL    = (MG_ERR_BASE_HID + 1),
    HID_RET_W_BUSY          = (MG_ERR_BASE_HID + 2),
    HID_RET_ERR_LEN         = (MG_ERR_BASE_HID + 3),
    HID_RET_MAX
} hid_ret;


typedef void (*PFN_SOF_SYNC_CALLBACK)(void);
extern QueueHandle_t hid_queue;
void mg_hid_init(void);
hid_ret mg_hid_send_queue(rep_hid_t *p_report);
void mg_hid_sof_sync_stop(void);
void hal_key_sync_callback(PFN_SOF_SYNC_CALLBACK pfn_cb);

#endif

