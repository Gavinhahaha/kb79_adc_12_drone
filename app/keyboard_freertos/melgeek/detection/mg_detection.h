#ifndef __MG_DETECTION_H__
#define __MG_DETECTION_H__
#include <stdint.h>
#include <stdbool.h>
#include "mg_sm.h"

#define DETECTE_NUM_MAX                      (6)

typedef enum
{
    VOLT_WARN_LEVEL_0 = 0,
    VOLT_WARN_LEVEL_1,
    VOLT_WARN_LEVEL_2 ,
    VOLT_WARN_LEVEL_3,
    VOLT_WARN_LEVEL_MAX
}volt_warn_level_e;

typedef struct 
{
    uint8_t detecte_water_pos_buf[DETECTE_NUM_MAX];
    uint8_t valid_water_pos_num;
    uint8_t detecte_volt_pos;
}detect_data_s;

void mg_detected_suspend(void);
void mg_detecte_task_notify(void);
void mg_detectiopn_init(void);
void detecte_data_handle(sm_data_t *p_sm_data);
uint8_t mg_detecte_get_err_status(void);
void mg_detecte_set_volt_warn_check_state(bool state);
bool mg_detecte_get_volt_warn_check_state(void);
bool mg_detecte_get_volt_low_state(void);
#endif