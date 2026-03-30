#ifndef _MG_FILTER_H_
#define _MG_FILTER_H_
#include <stdint.h>
#include <stdbool.h>

#define VD_GROUP                ( 1 )
#define VD_NUM                  ( 84 )
#define VD_MOVE_AVG_BUF_SIZE    ( 512 )

#define ADC_VREF                ( 2.5f )
#define ADC16_TO_VOLT_FA        ( (float)( ADC_VREF / (float)(1<<16)) )


typedef struct
{
    bool     isClr;
    uint16_t buf[VD_NUM];
    uint16_t min[VD_NUM];
    uint16_t max[VD_NUM];
    uint16_t dif[VD_NUM];
    float    vol[VD_NUM];
    uint16_t moveAvgBuf[VD_MOVE_AVG_BUF_SIZE];
    float    moveAvgResult;
} testDataView_t; 


extern testDataView_t tdv[VD_GROUP];

void tdv_clear(uint8_t idx, bool clear);
void tdv_record(uint8_t idx, uint8_t ki, uint16_t data);
uint16_t tdv_move_avg(uint8_t idx, uint8_t ki);


float mv_avg(uint8_t ki, float newData);
float lpf_dynamic_adj(float newData, float preData);
float lpf_dynamic_update(uint8_t ki, float newData, float preData);
float blpf(float input, float* output_pre, float* pre_pre_input, float* pre_pre_output);


#endif      /* _MG_FILTER_H_ */





