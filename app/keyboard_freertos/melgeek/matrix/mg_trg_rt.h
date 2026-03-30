#ifndef __MG_TRG_RT_H__
#define __MG_TRG_RT_H__
#include "mg_trg_rt_adaptive.h"

bool rt_handle(uint8_t ki, uint16_t tmpData);
void rt_init(kh_t *p_kh);
uint16_t rt_get_point_depth(uint8_t ki, uint16_t point);
void rt_set_default_para(void);
void rt_set_para(uint8_t ki, bool en, bool ctn, uint8_t d_sens, uint8_t u_sens);
void rt_check_para(key_attr_t *key_attr);
#define key_rt_clear_state(ki)                                    \
    {                                                             \
        rt_ada_clear_state(ki);                                   \
        kh.p_st[ki].debounce          = 0x00;                     \
        kh.p_st[ki].rtIsEnter         = 0x00;                     \
        kh.p_st[ki].isShapeChange     = 0x00;                     \
        kh.p_st[ki].rtLimitPointLevel = 0x00;                     \
        kh.p_st[ki].rtUpperLimitPoint = 0x0000;                   \
        kh.p_st[ki].rtLowerLimitPoint = 0xffff;                   \
        kh.p_st[ki].rtPressSens = kh.p_attr[ki].rtPressSensitive; \
        kh.p_st[ki].rtRaiseSens = kh.p_attr[ki].rtRaiseSensitive; \
    }

#define key_rt_record_limit_up_point(ki, keyShaftType, point)                                                                  \
    {                                                                                                                          \
        if (pkst->rtUpperLimitPoint < point)                                                                             \
        {                                                                                                                      \
            pkst->rtLowerLimitPoint = point;                                                                             \
            pkst->rtUpperLimitPoint = point;                                                                             \
            pkst->rtLimitPointLevel = rt_get_point_depth(ki, point);                                                     \
            int16_t pslvl;                                                                                                     \
            if (KEY_SUPT_HACC(ki)) /* shaft support high acc */                                                                \
            {                                                                                                                  \
                float d0 = LEVEL_TO_DIST(pkst->rtLimitPointLevel, KEY_RT_ACC(ki));                                       \
                float d1 = LEVEL_TO_DIST(kh.p_attr[ki].rtPressSensitive, KEY_RT_ACC(ki));                                      \
                if (pkst->rtLimitPointLevel > KEY_NACC_TO_LVL(ki))                                                       \
                {                                                                                                              \
                    pslvl = ACC_BETW_CONV(kh.p_attr[ki].rtPressSensitive, KEY_RT_ACC(ki), KEY_HACC(ki));                       \
                }                                                                                                              \
                else if (d0 + d1 > KEY_NACC_TO_DIST(ki))                                                                       \
                {                                                                                                              \
                    pslvl = KEY_NACC_TO_LVL(ki) + ACC_BETW_CONV(d0 + d1 - KEY_NACC_TO_DIST(ki), KEY_RT_ACC(ki), KEY_HACC(ki)); \
                }                                                                                                              \
                else                                                                                                           \
                {                                                                                                              \
                    pslvl = ACC_BETW_CONV(kh.p_attr[ki].rtPressSensitive, KEY_RT_ACC(ki), KEY_NACC(ki));                       \
                }                                                                                                              \
            }                                                                                                                  \
            else                                                                                                               \
            {                                                                                                                  \
                pslvl = kh.p_attr[ki].rtPressSensitive;                                                                        \
            }                                                                                                                  \
            pkst->rtPressSens = (pslvl <= 0) ? 1 : (pslvl > KEY_MAX_LEVEL(ki)) ? KEY_MAX_LEVEL(ki) : pslvl;              \
            pkst->isShapeChange = false;                                                                                 \
        }                                                                                                                      \
    }

#define key_rt_record_limit_down_point(ki, keyShaftType, point)                                                    \
    {                                                                                                              \
        if (pkst->rtLowerLimitPoint > point)                                                                 \
        {                                                                                                          \
            pkst->rtLowerLimitPoint = point;                                                                 \
            pkst->rtUpperLimitPoint = point;                                                                 \
            pkst->rtLimitPointLevel = rt_get_point_depth(ki, point);                                         \
            int16_t rslvl;                                                                                         \
            if (KEY_SUPT_HACC(ki)) /* shaft support high acc */                                                    \
            {                                                                                                      \
                if (pkst->rtLimitPointLevel > KEY_NACC_TO_LVL(ki))                                           \
                {                                                                                                  \
                    uint8_t lvl = pkst->rtLimitPointLevel - KEY_NACC_TO_LVL(ki);                             \
                    float d0 = LEVEL_TO_DIST(kh.p_attr[ki].rtRaiseSensitive, KEY_RT_ACC(ki));                      \
                    float d1 = LEVEL_TO_DIST(lvl, KEY_HACC(ki));                                                   \
                    if (d0 > d1)                                                                                   \
                    {                                                                                              \
                        float ddif = d0 - d1;                                                                      \
                        rslvl = DIST_TO_LEVEL(d1, KEY_HACC(ki)) + DIST_TO_LEVEL(ddif, KEY_NACC(ki));               \
                    }                                                                                              \
                    else                                                                                           \
                    {                                                                                              \
                        rslvl = DIST_TO_LEVEL(d0, KEY_HACC(ki));                                                   \
                    }                                                                                              \
                }                                                                                                  \
                else                                                                                               \
                {                                                                                                  \
                    rslvl = ACC_BETW_CONV(kh.p_attr[ki].rtRaiseSensitive, KEY_RT_ACC(ki), KEY_NACC(ki));           \
                }                                                                                                  \
            }                                                                                                      \
            else                                                                                                   \
            {                                                                                                      \
                rslvl = kh.p_attr[ki].rtRaiseSensitive;                                                            \
            }                                                                                                      \
            pkst->rtRaiseSens = (rslvl <= 0) ? 1 : (rslvl > KEY_MAX_LEVEL(ki)) ? KEY_MAX_LEVEL(ki) : rslvl;        \
            int16_t overValue = (key_cali.min[ki] - point);                                                 \
            if (KEY_IS_BIG(ki))                                                                                    \
            {                                                                                                      \
                pkst->isShapeChange = (overValue > KEY_BIG_SHAPE_CHANGE_VAL) ? true : false;                       \
            }                                                                                                      \
            else                                                                                                   \
            {                                                                                                      \
                pkst->isShapeChange = (overValue > KEY_SHAPE_CHANGE_VAL) ? true : false;                           \
            }                                                                                                      \
            if (pkst->rtIsEnter)                                                                                   \
            {                                                                                                      \
                pkst->rtTimestamp   = hpm_csr_get_core_cycle();                                                   \
            }                                                                                                      \
        }                                                                                                          \
    }

#define key_rt_up_point(point)\
    {\
        if (pkst->rtUpperLimitPoint < point)\
        {\
            pkst->rtLowerLimitPoint = point;\
            pkst->rtUpperLimitPoint = point;\
        }\
    }

#define key_rt_down_point(ki, point)\
    {\
        if (pkst->rtLowerLimitPoint > point)\
        {\
            pkst->rtLowerLimitPoint = point;\
            pkst->rtUpperLimitPoint = point;\
            int16_t overValue = (key_cali.min[ki] - point);                                                 \
            pkst->isShapeChange = (overValue > KEY_SHAPE_CHANGE_VAL) ? true : false;                         \
        }\
    }

#endif

