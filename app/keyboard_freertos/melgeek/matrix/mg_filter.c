/**
  *@author: JayWang
  *@brief : Abnormal data filtering processing
  */
#include "mg_filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mg_matrix_type.h"


#define FILTER_MOVE_AVG_BUF_SIZE    ( 10 )


extern key_st_t key_st[];

testDataView_t tdv[VD_GROUP] = {0};


void tdv_clear(uint8_t idx, bool clear)
{
    if (tdv[idx].isClr || clear)
    {
        tdv[idx].isClr = false;
        for (uint8_t i = 0; i < VD_NUM; i++)
        {
            tdv[idx].max[i] = 0;
            tdv[idx].min[i] = 0xffff;
        }
    }
}
void tdv_record(uint8_t idx, uint8_t ki, uint16_t data)
{
    tdv[idx].buf[ki] = data;
    if (tdv[idx].buf[ki] > tdv[idx].max[ki])
    {
        tdv[idx].max[ki] = tdv[idx].buf[ki];
    }
    if (tdv[idx].buf[ki] < tdv[idx].min[ki])
    {
        tdv[idx].min[ki] = tdv[idx].buf[ki];
    }
    tdv[idx].dif[ki] = abs(tdv[idx].max[ki] - tdv[idx].min[ki]);
    tdv[idx].vol[ki] = ADC16_TO_VOLT_FA * tdv[idx].dif[ki];
}

uint16_t tdv_move_avg(uint8_t idx, uint8_t ki)
{
    static uint32_t cnt[VD_GROUP] = {0};
    float total             = 0;
    tdv[idx].moveAvgBuf[ cnt[idx] ] = tdv[idx].buf[ki];
    cnt[idx] = (cnt[idx] + 1) % VD_MOVE_AVG_BUF_SIZE;
    for (uint16_t i = 0; i < VD_MOVE_AVG_BUF_SIZE; i++)
    {
        total += tdv[idx].moveAvgBuf[i];
    }
    tdv[idx].moveAvgResult = total / (float)VD_MOVE_AVG_BUF_SIZE;
    return tdv[idx].moveAvgResult;
}
typedef struct 
{
    bool dirPre;
    uint16_t count;
} lpf_t;

typedef struct 
{
    float outputPre;
    float outputPrePre;
    float inputPrePre;
} blpf_t;

typedef struct 
{
    uint8_t len;
    uint8_t idx;
    uint16_t buf[FILTER_MOVE_AVG_BUF_SIZE];
    uint32_t total;
} mv_avg_t;

typedef union 
{
    lpf_t lpf;
    blpf_t blpf;
    mv_avg_t avg;
} filter_t;

static filter_t filt[VD_NUM];

float mv_avg(uint8_t ki, float newData)
{
    uint8_t idx = filt[ki].avg.idx;
    if (filt[ki].avg.len < FILTER_MOVE_AVG_BUF_SIZE)
    {
        filt[ki].avg.len++;
        filt[ki].avg.buf[idx] = newData;
        filt[ki].avg.idx = (idx + 1) % FILTER_MOVE_AVG_BUF_SIZE;
        return newData;
    }
    else
    {
        filt[ki].avg.total = 0;
        filt[ki].avg.buf[idx] = newData;
        filt[ki].avg.idx = (idx + 1) % FILTER_MOVE_AVG_BUF_SIZE;
        for (uint16_t i = 0; i < FILTER_MOVE_AVG_BUF_SIZE; i++)
        {
            filt[ki].avg.total += filt[ki].avg.buf[i];
        }
        return filt[ki].avg.total / FILTER_MOVE_AVG_BUF_SIZE;
    }
}


float lpf(float newData, float preData, float k)
{
    return (newData * k) + (preData * (1.0f - k));
}
float lpf_dynamic_adj(float newData, float preData)
{
    float kp = ABS2(preData , newData) > 90 ? 0.93f : 0.18f; 
    return lpf(newData, preData, kp);
}
float lpf_dynamic_update(uint8_t ki, float newData, float preData)
{
    float kp      = 0.15f; //default
    float step    = 0.30f;
    int16_t dif   = newData - preData;
    uint16_t udif = ABS2(newData, preData);
    uint8_t dir   = (dif > 0);
    
    float stepFollow = (udif) / (key_st[ki].diff / 2);

    if (dir == filt[ki].lpf.dirPre)
    {
        // x cycles
        if (udif > 35)          
        {
            filt[ki].lpf.count++;
        }
        if (stepFollow > 0.7f)
        {
            kp += stepFollow;
        }
        else if (filt[ki].lpf.count >= 2)
        {
            kp += step;
        }
    }
    else //update dir
    {
        kp += (stepFollow > 0.7f) ? stepFollow : 0.05f;
        filt[ki].lpf.count  = 0;
        filt[ki].lpf.dirPre = dir;
    }
    kp = (kp > 1.0f) ? 1.0f : kp;
    return lpf(newData, preData, kp);
}

#if 0
static const float blpf_Wn = 8000.0f; // cutoff frequency
static const float blpf_Q = (float)(0x01 << 16);
static const float blpf_a0 = (blpf_Wn * blpf_Wn) / (1.0 + (sqrt(2) * blpf_Wn / blpf_Q) + (blpf_Wn * blpf_Wn));
static const float blpf_a1 = 2.0 * blpf_a0;
static const float blpf_a2 = blpf_a0;
static const float blpf_b1 = 2.0 * blpf_a0 * (1.0 - (blpf_Wn * blpf_Wn)) / (1.0 + (sqrt(2) * blpf_Wn / blpf_Q) + (blpf_Wn * blpf_Wn));
static const float blpf_b2 = (1.0 - (sqrt(2) * blpf_Wn / blpf_Q) + (blpf_Wn * blpf_Wn)) / (1.0 + (sqrt(2) * blpf_Wn / blpf_Q) + (blpf_Wn * blpf_Wn));

float blpf(float input, float* outputPre, float* inputPrePre, float* outputPrePre) 
{
    float output = blpf_a0 * input + blpf_a1 * (*inputPrePre) + blpf_a2 * (*outputPrePre) - blpf_b1 * (*outputPre) - blpf_b2 * (*outputPrePre);
    *inputPrePre = *outputPre;
    *outputPrePre = *outputPre;
    *outputPre = output;
    return output;
}

float test_vd_blpf(uint8_t ki, float input) 
{
    blpf(input, &filt[ki].blpf.outputPre, &filt[ki].blpf.inputPrePre, &filt[ki].blpf.outputPrePre);
    return filt[ki].blpf.outputPre;
}
#endif




