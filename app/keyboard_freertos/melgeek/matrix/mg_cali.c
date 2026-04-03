/*
* Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
* 
* Description: 
* 
*/

#include "mg_cali.h"
#include "mg_matrix_type.h"
#include "mg_matrix_config.h"
#include "mg_matrix.h"
#include "mg_hid.h"
#include "db.h"
#include "app_debug.h"
#include "app_config.h"
#include "hpm_csr_drv.h"
#include "hpm_clock_drv.h"

#include "board.h"
#include "easy_crc.h"
#include <math.h>
#include "mg_trg_fix.h"
#include "mg_trg_rt.h"
#include "mg_hive.h"
#include "hpm_gpio_drv.h"
#include "mg_cali.h"
#include "mg_uart.h"
#include "mg_filter.h"
#include "mg_factory.h"
#include "mg_detection.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (DBG_EN_CALI)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "mg_cali"
    #define   DBG_PRINTF(fmt, ...)     Mg_SEGGER_RTT_printf(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

extern kh_t kh;
TaskHandle_t m_cali_thread;

kcali_thd_t cali_thd;

static bool cali_check_ready     = false;
extern key_st_t key_st[KEYBOARD_NUMBER];
extern uint64_t key_last_time[KEY_FRAME_TOTAL_SIZE];
extern volatile bool is_timer_key_unchange_flag;
extern uint16_t test_buf[KEY_FRAME_TOTAL_SIZE];
static uint16_t last_cail_adc_min[KEYBOARD_NUMBER];
static uint16_t last_cail_adc_max[KEYBOARD_NUMBER];
//static kh_t *p_kh;
cali_tuning_t tuning;

static key_cail_param_s key_cail_param;
static key_cali_t *p_calib;
static uint8_t rpt_refresh_cnt = 0;

#define KEY_REPORT_REFRESH_MS 20

#define KEY_CALL_OFFSET_LEVEL   15
#define KEY_CALL_OFFSET_VAL     500

void cali_db_register(void)
{
    db_register(DB_CALI, (uint32_t *)&key_cali, sizeof(key_cali_t));
    db_read(DB_CALI);
    
    kh.p_calib     = &key_cali;
    uint32_t itf = db_get_itf(DB_CALI);
    
    if (itf != DB_INIT_MAGIC)//reset
    {
        key_cali.calibDone = 0;
        for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
        {
            key_cali.sw[ki] = KEY_DFT_SHAFT;
        }
    }
    else//check
    {
        if (key_cali.calibDone > 1)
        {
            key_cali.calibDone = 0;
        }
        
        for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
        {
            if ((key_cali.sw[ki] >= KEY_SHAFT_TYPE_MAX)||(key_cali.sw[ki] == KEY_SHAFT_TYPE_UNKNOWN))
            {
                key_cali.sw[ki] = KEY_DFT_SHAFT;
            }
        }
    }
    
    //置入默认参数
    if (key_cali.calibDone == 0)
    {
        for (uint32_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
        {
            key_cali.max[ki] = (sw_thd_table[KEY_DFT_SHAFT].top_max + sw_thd_table[KEY_DFT_SHAFT].top_min)/2;
            key_cali.min[ki] = (sw_thd_table[KEY_DFT_SHAFT].bottom_max + sw_thd_table[KEY_DFT_SHAFT].bottom_min)/2;;
        }
        #if (BOARD_FW_FOR_FA==0)
        key_cali.calibDone = true;
        #endif
    }
/*   else
    {
        //检查校准参数
        for (uint32_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
        {
            if ((key_cali.max[ki] > sw_thd_table[key_cali.sw[ki]].top_max) || (key_cali.max[ki] < sw_thd_table[key_cali.sw[ki]].top_min))
            {
                key_cali.max[ki] = (sw_thd_table[key_cali.sw[ki]].top_max + sw_thd_table[key_cali.sw[ki]].top_min)/2;
            }
            
            if ((key_cali.min[ki] > sw_thd_table[key_cali.sw[ki]].bottom_max) || (key_cali.min[ki] < sw_thd_table[key_cali.sw[ki]].bottom_min))
            {
                key_cali.min[ki] = (sw_thd_table[key_cali.sw[ki]].bottom_max + sw_thd_table[key_cali.sw[ki]].top_min)/2;
            }
        }
    }
*/
}

void cali_data_is_vaild(key_st_t *pst)
{
    key_sw_thd_t const *psw;
    key_st_t *pkst = pst;
    uint8_t save = false;
    for (uint32_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        psw = &sw_thd_table[key_cali.sw[ki]];
        if ((key_cali.max[ki] > psw->top_max) || (key_cali.max[ki] < psw->top_min) ||
            (key_cali.min[ki] > psw->bottom_max) || (key_cali.min[ki] < psw->bottom_min) ||
            (pkst->diff > psw->diff_max) || (pkst->diff < psw->diff_min))
        {
            kh.p_attr[ki].shaftChange = false;
            key_cali.sw[ki] = KEY_DFT_SHAFT;
            key_cali.max[ki] = pkst->cur_point;
            key_cali.min[ki] = pkst->cur_point - KEY_DFT_TRAVEL_VALUE(key_cali.sw[ki]);
            cali_calc_level_boundary(ki); /* boundary algo calc */

            save = true;
        }
        pkst++;
    }
    if (save)
    {
        fix_init(&kh);
        rt_init(&kh);
        //key_dks_init(&kh);
        db_update(DB_SYS, UPDATE_NOW);
        db_update(DB_CALI, UPDATE_NOW);
    }

}

#if (USE_KEY_LVL_MODE == KEY_LVL_MODE_BUFF)
static void boundaries_determine_calc_algo(const uint16_t keyTopValue, const uint16_t keyBottomValue,
                                    const uint8_t reserveBottomValue, const uint8_t layerMax, 
                                    const float *charactTable, uint16_t *outBoundariesBuffer)
{
    uint16_t travel= ABS2(keyTopValue , keyBottomValue);
    (void)reserveBottomValue;
    
    // float check = 0;
    // float checkRait=0;
    float upperLimit = keyTopValue;
    if(keyTopValue > keyBottomValue)
    {
        for (int i = 0; i < layerMax; i++) 
        {
            upperLimit -= (travel * charactTable[i]); //等级边界
            outBoundariesBuffer[i] = (uint16_t)ceil((upperLimit < keyBottomValue) ? keyBottomValue : upperLimit);
        }
        outBoundariesBuffer[layerMax-1] = keyBottomValue;
    }
    else    /* keyTopValue < keyBottomValue */
    {
        for (int i = 0; i < layerMax; i++) 
        {
            upperLimit += (travel * charactTable[i]); //等级边界
            outBoundariesBuffer[i] = (uint16_t)ceil((upperLimit > keyBottomValue) ? keyBottomValue : upperLimit); 
        }
        outBoundariesBuffer[layerMax-1] = keyBottomValue;
    }
    // for (int i = 0; i < layerMax; i++) 
    // {
    //     log_debug("level[%02d]: %04d, ", i, outBoundariesBuffer[i]);
    //     log_debug("base: %02d, level range value: %02f, wave ratio: %0.1f%% \r\n", gradingValueBase, (gradingValueBase * bufferRatio[i]), bufferRatio[i]*100);
    // }
    // log_debug("total: %04d\r\n", travel);
}
#endif
void cali_calc_level_boundary(uint8_t ki)
{
    // instUserMsg->calibration.data.levelBoundaryValue[round][count][layer] = (float)((1.0f + (float)keyWaveTable[layer] / 100.0f) * (float)instUserMsg->calibration.data.gradingValueBase[round][count]);
    // log_debug("Boundary%02d: %f, %f\r\n", layer, (float)instUserMsg->calibration.data.gradingValueBase[round][count], instUserMsg->calibration.data.levelBoundaryValue[round][count][layer]);
    
    key_st_t *currentKeyStatus      = (kh.p_st+ki);
    key_attr_t *p_attr = (kh.p_attr+ki);
    
    uint8_t keysw          = key_cali.sw[ki];
    
    key_cali.min[ki]      += KEY_SPARE_STROKE(ki);
    currentKeyStatus->min  = key_cali.min[ki];
    currentKeyStatus->max  = key_cali.max[ki];
    currentKeyStatus->diff = ABS2(key_cali.max[ki] , key_cali.min[ki]);
    
#if (USE_KEY_LVL_MODE == KEY_LVL_MODE_BUFF)
    uint16_t keyTopValue    = 0;
    uint16_t keyBottomValue = 0;
    keyTopValue    = key_cali.max[ki] - KEY_TOP_RESERVER(ki);
    keyBottomValue = key_cali.min[ki] + KEY_BOT_RESERVER(ki);

    boundaries_determine_calc_algo( keyTopValue, keyBottomValue, 0, 
                                    sw_config_table[keysw].maxLevel, 
                                    sw_config_table[keysw].curvTab,
                                    currentKeyStatus->levelBoundaryValue);
#endif
    /* 加入死区 */
    currentKeyStatus->triggerAddDeadLevel = (p_attr->triggerlevel > (1+p_attr->deadbandTop)) 
                                        ? (p_attr->triggerlevel)
                                        : (1+p_attr->deadbandTop);
    currentKeyStatus->raiseAddDeadLevel = (p_attr->raiseLevel < (sw_config_table[keysw].maxLevel-p_attr->deadbandBottom)) 
                                        ? (p_attr->raiseLevel)
                                        : (sw_config_table[keysw].maxLevel-p_attr->deadbandBottom);
    
    fix_special_point_update(ki, p_attr);

    uint8_t triggerLevelPos = currentKeyStatus->triggerAddDeadLevel - 1;
    uint8_t raiseLevelPos   = (currentKeyStatus->raiseAddDeadLevel - 1) < 0 ? 0 : (currentKeyStatus->raiseAddDeadLevel - 1);
    p_attr->triggerpoint = KEY_LVL_GET_VAL(ki, triggerLevelPos);
    p_attr->raisePoint   = (currentKeyStatus->raiseAddDeadLevel == 0) ? kh.p_st[ki].topRaisePoint : KEY_LVL_GET_VAL(ki, raiseLevelPos);
    //DBG_PRINTF("cali[%02d]: %d,%d,%d\n", ki, p_attr->triggerpoint,currentKeyStatus->levelBoundaryValue[0],triggerLevelPos);
    adv_mtp_reload();
/*
    if(ki==0)
    {
        DBG_PRINTF("ki(0): ");
        for(uint8_t i=0; i<sw_config_table[keysw].maxLevel; i++)
        {
            DBG_PRINTF("[%02d]: %d\n", i, (uint16_t)key_cali.levelBoundaryValue[ki][i]);
        }
    }
*/
}

void cali_calc_level_boundarys(void)
{
    for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        cali_calc_level_boundary(ki);
    }
}

bool cali_is_ready(void)
{
    return (cali_check_ready && (key_cali.calibDone == true));
}
void cali_set_ready(bool en)
{
    mg_factory_t *factory = mg_factory_get_info();
    uint16_t fa_nr_4 = *(uint16_t *)&factory->fa[1][FA_STEP_MAX_NARUI-3];
    if (fa_nr_4 != 0xffff)
    {
        cali_check_ready = en;
    }
    else
    {
        cali_check_ready = false;
    }
    // DBG_PRINTF("cali_check_ready = %d\r\n", cali_check_ready);
}

cail_step_t cali_get_statue(void)
{
    return cali_thd.step;
}


tuning_e cali_tuning_get_state(void)
{
    //DBG_PRINTF("tuning_get_state=%d\n", tuning.en);
    return tuning.en;
}

void cali_tuning_set_state(tuning_e en)
{
    DBG_PRINTF("tuning_set_state=%d\n", en);
    if (kh.p_qty->tuning_en != en)
    {
        kh.p_qty->tuning_en = en;
        uint8_t status = kh.p_qty->tuning_en;
        if (UART_SUCCESS != mg_uart_send_to_all_nation_request_cmd(SM_SET_AUTO_CAIL_CTRL_CMD, 1, &status)) //设置帧模式
        {
            DBG_PRINTF("th mode err\n");
        }
    }
    tuning.en = kh.p_qty->tuning_en;
}

void cali_rpt_dbg_set_state(bool en)
{
    DBG_PRINTF("cali_rpt_dbg_set_state=%d\n", en);
    if (kh.p_qty->cail_dbg_rpt_en != en)
    {
        kh.p_qty->cail_dbg_rpt_en = en;
    }
    memset(last_cail_adc_max,0,sizeof(last_cail_adc_max));
    memset(last_cail_adc_min,0,sizeof(last_cail_adc_min));
}

kh_data_t *cali_peek_new_adc_fifo(void)
{
    key_st_t *pkst;
    static kh_data_t ret;
    //kh_data_t event;
    uint8_t ki;
    static uint16_t buf[KEYBOARD_NUMBER];
    
    pkst = &key_st[0];
    for (ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        buf[ki] = pkst->cur_point;
        pkst++;
    }
    DBG_PRINTF("cali_peek_new_adc_fifo\n");
    ret.buf = buf;
    ret.size = KEYBOARD_NUMBER;
    
    
    return &ret;
}

void cali_set_peek_adc(bool en)
{
    cali_set_ready(en);
    //cali_check_ready = en;
    DBG_PRINTF("cali_set_peek_adc=%d\n",en);
    vTaskDelay(20);
    /* TODO(lqhui): need sync from matrix thread */
}


#define CALI_ADC_ZOOM 1
#define RPT_FIFO_SIZE 10
#define RPT_CAIL_FIFO_SIZE 20

static void cali_report_adc(key_rpt_e type, key_st_t *pst)
{
    uint8_t test=0;
    uint8_t ki = 0;
    uint8_t size;
    
    static uint16_t last_adc[KEYBOARD_NUMBER];
    
    static kh_rpt_fifo_t rpt_fifo[RPT_FIFO_SIZE];
    static uint8_t rpt_fifo_tx_idx = 0;
    uint16_t cur_point;
    //uint16_t last_point;

    uint8_t keysw;
    key_st_t *pkst = pst;
    kh_rpt_fifo_t *p_rpt;
    
    size = 0;
    p_rpt = &rpt_fifo[rpt_fifo_tx_idx];

    
    for (ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        keysw      = key_cali.sw[ki];
        cur_point  = pkst->cur_point;//
        //cur_point  = pkst->cur_point / CALI_ADC_ZOOM;
        
        if (((pkst->cur_lvl != pkst->last_lvl) && (type == KEY_REPORT_OUTTYPE_TRIGGER) && (key_cali.calibDone == true))||
            ((abs(cur_point - last_adc[ki]) > 0) && (type == KEY_REPORT_OUTTYPE_REALTIME)) ||
            (kh.p_gst->key_rpt_refresh == 1))
        {
            //if ((key_cali_get_statue() == KEY_CALIB_START)||(key_cali_get_statue() == KEY_CALIB_SWITCH_OUT))
            {
                //pkst->last_point = pkst->cur_point;
            }
            
            
            if (kh.p_attr[ki].shaftChange) //用于hive辨别未校准的按键
            {
                pkst->cur_lvl   = sw_config_table[keysw].maxLevel;
                pkst->cur_point = sw_config_table[keysw].maxLevel;
            }
            
            // 检查是否需要切换到下一个缓冲区
            if (size >= 14)
            {
                p_rpt->size = size;
                cmd_hive_send_adc_rpt((uint8_t *)p_rpt->rpt, size*4);
                
                // 切换到下一个缓冲区
                rpt_fifo_tx_idx++;
                rpt_fifo_tx_idx %= RPT_FIFO_SIZE;
                p_rpt = &rpt_fifo[rpt_fifo_tx_idx];
                size = 0;
            }
            
            // 添加数据到当前缓冲区
            p_rpt->rpt[size].ki  = ki;
            p_rpt->rpt[size].lvl = pkst->cur_lvl;
            p_rpt->rpt[size].adc = cur_point / CALI_ADC_ZOOM;
            
            pkst->last_lvl       = pkst->cur_lvl;
            test++;
            
            size++;
        }
        pkst++;
        last_adc[ki] = cur_point;
    }

    //剩余部分打包发送
    if (size)
    {
        p_rpt->size = size;
        cmd_hive_send_adc_rpt((uint8_t *)p_rpt->rpt, size*4);
        
        // 发送完当前缓冲区后切换到下一个
        rpt_fifo_tx_idx++;
        rpt_fifo_tx_idx %= RPT_FIFO_SIZE;
    }
    
    if (rpt_refresh_cnt)
    {
        rpt_refresh_cnt--;
    }
    else
    {
        kh.p_gst->key_rpt_refresh = 0;
    }
    
}

static void cali_report_adc_cail(void)
{
    static kh_rpt_cail_fifo_t rpt_fifo[RPT_CAIL_FIFO_SIZE];
    static uint8_t rpt_fifo_tx_idx = 0;
    key_st_t *pkst = key_st;
    key_attr_t *pattr = kh.p_attr;
    static uint16_t last_adc[KEYBOARD_NUMBER];
    kh_rpt_cail_fifo_t *p_rpt;
    uint8_t ki = 0;
    uint8_t size;
    
    size = 0;
    p_rpt = &rpt_fifo[rpt_fifo_tx_idx];

    for (ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        if ((abs(key_cali.max[ki] - last_cail_adc_max[ki]) > 0) || \
            (abs(key_cali.min[ki] - last_cail_adc_min[ki]) > 0) || \
            (abs(pkst[ki].cur_point - last_adc[ki]) > 1) || \
            (pkst[ki].cur_lvl != pkst[ki].last_lvl))
            {
                // 检查是否需要切换到下一个缓冲区
                if (size >= 5)
                {
                    p_rpt->size = size;
                    cmd_hive_send_adc_cail_rpt((uint8_t *)p_rpt->rpt, size*10);
                
                    // 切换到下一个缓冲区
                    rpt_fifo_tx_idx++;
                    rpt_fifo_tx_idx %= RPT_CAIL_FIFO_SIZE;
                    p_rpt = &rpt_fifo[rpt_fifo_tx_idx];
                    size = 0;
                    vTaskDelay(10);
                }
            
                // 添加数据到当前缓冲区
                p_rpt->rpt[size].ki  = ki;
                p_rpt->rpt[size].level = pkst[ki].cur_lvl;
                if (pkst[ki].cur_point > pattr[ki].triggerpoint)
                {
                    p_rpt->rpt[size].trg_dif = ((pkst[ki].cur_point - pattr[ki].triggerpoint) <0xFF) ? \
                                               (pkst[ki].cur_point - pattr[ki].triggerpoint) : 0xFF;
                }
                else
                {
                    p_rpt->rpt[size].trg_dif = 0xFF;
                }

                if (pkst[ki].cur_point < pkst[ki].bottomPressPoint)
                {
                    p_rpt->rpt[size].rls_dif = ((pkst[ki].bottomPressPoint - pkst[ki].cur_point) <0xFF) ? \
                                               (pkst[ki].bottomPressPoint - pkst[ki].cur_point) : 0xFF;
                }
                else
                {
                    p_rpt->rpt[size].rls_dif = 0xFF;
                }
                p_rpt->rpt[size].cail_min = key_cali.min[ki];
                p_rpt->rpt[size].adc = pkst[ki].cur_point;
                p_rpt->rpt[size].cail_max = key_cali.max[ki];
                size++;
            }
            last_cail_adc_max[ki] = key_cali.max[ki];
            last_cail_adc_min[ki] = key_cali.min[ki];
            last_adc[ki] = pkst[ki].cur_point;
    }

    //剩余部分打包发送
    if (size)
    {
        p_rpt->size = size;
        cmd_hive_send_adc_cail_rpt((uint8_t *)p_rpt->rpt, size*10);
        
        // 发送完当前缓冲区后切换到下一个
        rpt_fifo_tx_idx++;
        rpt_fifo_tx_idx %= RPT_CAIL_FIFO_SIZE;
    }
}
void cali_res_feed(kcali_thd_t *p_calib, uint8_t ki)
{
    uint8_t bitPos  = ki % 8;
    uint8_t bytePos = ki / 8;
    p_calib->cali_res[bytePos] |= (1 << bitPos);
}

void cali_res_clear(kcali_thd_t *p_calib, uint8_t ki)
{
    uint8_t bitPos  = ki % 8;
    uint8_t bytePos = ki / 8;
    p_calib->cali_res[bytePos] &= ~(1 << bitPos);
}

bool cali_res_get(kcali_thd_t *p_calib, uint8_t ki)
{
    uint8_t bitPos  = ki % 8;
    uint8_t bytePos = ki / 8;
    return (p_calib->cali_res[bytePos] & (1 << bitPos));
}

void cali_para_reset(void)
{
    key_st_t      *p_st;
    
    memset(cali_thd.diff, 0, KEYBOARD_NUMBER * sizeof(uint16_t));
    memset(cali_thd.max, 0,  KEYBOARD_NUMBER * sizeof(uint16_t));
    memset(cali_thd.min, 0xFFFF, KEYBOARD_NUMBER * sizeof(uint16_t));
    memset(cali_thd.cnt, 0xFF, sizeof(cali_thd.cnt));
    memset(cali_thd.cali_res, 0, sizeof(cali_thd.cali_res));
    cali_thd.calib_num = 0;
    
    p_st = kh.p_st;
    for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        p_st->calibState = false;
        p_st++;
    }
}

static void cali_handle(key_st_t *pst)
{
    uint16_t *p_max;
    uint16_t *p_min;
    uint16_t *p_diff;
    uint8_t cali_ok_cnt = 0;
    uint16_t tmpData = 0;
    key_sw_thd_t const *psw;
    key_st_t *pkst;
    // bool stable = false;
    pkst = pst;
    
    //首先取最大值
    if (cali_thd.max_sample_done == false)
    {
        cali_thd.max_sample_cnt++;
        if (cali_thd.max_sample_cnt > 10)
        {
            cali_thd.max_sample_done = true;
        }
    }
    
    for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
    //    if (pkst->calibState == true)
    //    {
    //        pkst++;
    //        continue;
    //    }
        
        tmpData = pkst->cur_point;
        
        p_max  = &cali_thd.max[ki];
        p_min  = &cali_thd.min[ki];
        p_diff = &cali_thd.diff[ki];
        psw    = &sw_thd_table[key_cali.sw[ki]];
        
        
        if (cali_thd.max_sample_done == false)
        {
            if (cali_thd.max_sample_cnt == 1 )
            {
                *p_max = tmpData;
            }
            else
            {
                *p_max = (tmpData + (*p_max))/2;
            }
            cali_thd.cnt[ki] = 0;
        }
        else
        {
            if (tmpData < *p_min)
            {
                *p_min = tmpData;
            }
            //if( ki == 0)
            {
                //if (tmpData<3200*15)
            }


            bool in_valid_range = ((*p_max) > (*p_min)) 
                && ((*p_max) < psw->top_max)
                && ((*p_max) > psw->top_min)
                && ((*p_min) < psw->bottom_max)
                && ((*p_min) > psw->bottom_min)
                && (((*p_max) - (*p_min)) > psw->diff_min)
                && (((*p_max) - (*p_min)) < psw->diff_max);
            // && (stable == true);


            if (in_valid_range)
            {
                if (ABS2(tmpData, *p_min) > 200 )//不是稳定状态
                {
                    cali_thd.cnt[ki] = 0;
                    //stable = false;
                }
                else
                {
                    //stable = true;
                    cali_thd.cnt[ki]++;
                }
                //if( ki == 0)
                {
                }
                if (cali_thd.cnt[ki] > 4 )
                {

                    *p_diff = ABS2(*p_max , *p_min);
                    if (pkst->calibState == false)
                    {
                        //记录校准结果
                        pkst->calibState = true;
                        DBG_PRINTF("ki[%d] cali ok(%02d)\n",ki, cali_thd.calib_num);
                        
                        if (cali_thd.calib_num < KEYBOARD_NUMBER)
                        {
                            cali_thd.calib_num++;
                        }
                        
                        if (cali_thd.calib_num == KEYBOARD_NUMBER)
                        {
                            key_cali.calibDone = true;
                            DBG_PRINTF("key all cali finish\n");
                        }
                        cali_ok_cnt++;
                    }
                    if (cali_res_get(&cali_thd, ki) != true)
                    {
                        cali_res_feed(&cali_thd, ki);
                    }
                    //DBG_PRINTF("ki[%d] cali=%d,%d,%d\n", ki, key_cali.max[ki], key_cali.min[ki], cali_thd.max[ki]-cali_thd.min[ki]);
                }
            }
            else
            {
                if (cali_res_get(&cali_thd, ki) || pkst->calibState == true)
                {
                    pkst->calibState = false;
                    if (cali_thd.calib_num > 0)
                    {
                        cali_thd.calib_num--;
                    }
                    cali_res_clear(&cali_thd, ki);
                    cali_ok_cnt++;
                    DBG_PRINTF("ki[%d] cali fail\n", ki);
                    cali_thd.cnt[ki] = 0;
                }
            }
        }
        pkst++;
    }
    
    if (cali_ok_cnt )
    {
        cmd_hive_send_cali_rpt(cali_thd.cali_res, sizeof(cali_thd.cali_res));
    }
    
}

uint8_t cali_auto_is_change(void)
{
    if (kh.p_gst != NULL)
    {
        return kh.p_gst->calib_is_change;
    }
    return 0;
}
void cali_auto_set_change(uint8_t is_change)
{
    if (kh.p_gst != NULL)
    {
        kh.p_gst->calib_is_change = is_change;
    }
}

cail_step_t cali_get_rpt(void)
{
    return cali_thd.step;
}

void cali_set_rpt(key_rpt_e type)
{
    //uint8_t buf[2];
    sys_run_info_s *sys_run_info = &sys_run;
    static uint8_t last_frame_mode = KEY_DATA_FRAME_MAX;
    uint8_t cur_frame_mode = KEY_DATA_FRAME_MAX;
    DBG_PRINTF("kh_set_rpt:%d\n", type);
    rpt_refresh_cnt =10;
    if (kh.p_gst->key_rpt_type == type)
    {
        if (type != KEY_REPORT_OUTTYPE_CLOSE)
        {
            kh.p_gst->key_rpt_refresh = 1;
            if (eTaskGetState(m_cali_thread) == eSuspended) 
            {
                vTaskResume(m_cali_thread);
                vTaskDelay(10);
            }
            
            DBG_PRINTF("kh_set_rpt refresh\n");
        }
        return;
    }
    
    switch (type)
    {
        case KEY_REPORT_OUTTYPE_CLOSE:
            //app_timer_stop(key_rpt_timer);
            //fifo_reset(&adc_rpt_fifo);
            kh.p_gst->key_rpt_type      = type;
            kh.p_gst->key_rpt_refresh   = 0;
            sm_unsync_callback();
            
            if (cali_thd.is_manual_cail_update)
            {
                uint16_t sendbuf[UART_DEVICE_DPM*KEY_FRAME_SIZE];
                memset(sendbuf,0,sizeof(sendbuf));
                for (uint8_t i = 0; i < UART_DEVICE_DPM*KEY_FRAME_SIZE; i++)
                {
                    int8_t ki = key_idx_table[i];
                    if ((ki >= 0) && (key_cali.min[ki] > ADC_VAL_MIN) &&(key_cali.min[ki] < ADC_VAL_MAX))
                    {
                        sendbuf[i] = key_cali.min[ki];
                    }
                }
                for (uint8_t i = 0; i < UART_DEVICE_DPM; i++)
                {
                    uint8_t dev_id = dev_channel_map[i].uart_id;
                    if (UART_SUCCESS != mg_uart_send_to_single_nation_request_cmd(dev_id, SM_SET_SYN_BOT_CAIL_CMD, KEY_FRAME_SIZE*sizeof(uint16_t),(uint8_t*)&sendbuf[i*KEY_FRAME_SIZE],UART_RETRY_TIMES))
                    {
                        DBG_PRINTF("uart i :%d \r\n", i);
                    }
                }
                cali_thd.is_manual_cail_update = false;
                memset(&key_cail_param,0,sizeof(key_cail_param));
            }
            
            if (UART_SUCCESS != mg_uart_send_to_all_nation_request_cmd(SM_SET_KEY_MODE_CMD, 1, &last_frame_mode)) //设置帧模式
            {
                DBG_PRINTF("th mode err\n");
                break;
            }
            vTaskDelay(10);
            sys_run_info->sys_frame_mode = last_frame_mode;
            hal_key_sync_callback(sm_sync_callback);
        break;
        
        case KEY_REPORT_OUTTYPE_TRIGGER:
            //app_timer_start(key_rpt_timer, 2, NULL);
            kh.p_gst->key_rpt_type      = type;
            kh.p_gst->key_rpt_refresh = 1;
            last_frame_mode = sys_run_info->sys_frame_mode;
            cur_frame_mode = KEY_DATA_THRESHOLD_FRAME;
            sm_unsync_callback();
            
            if (UART_SUCCESS != mg_uart_send_to_all_nation_request_cmd(SM_SET_KEY_MODE_CMD, 1, &cur_frame_mode)) //设置帧模式
            {
                DBG_PRINTF("trigger mode err\n");
                break;
            }
            vTaskResume(m_cali_thread);
            vTaskDelay(10);
            sys_run_info->sys_frame_mode = cur_frame_mode;
            hal_key_sync_callback(sm_sync_callback);
        break;

        case KEY_REPORT_OUTTYPE_REALTIME:
            //buf[1] = 0;
            //buf[0] = 120;
        
            //cmd_mcu_send(CMD_KEY_PARA, MCU_KS_SET_POLLING_RATE, buf, 2);
            //app_timer_start(key_rpt_timer, 2, NULL);
            kh.p_gst->key_rpt_type      = type;
            kh.p_gst->key_rpt_refresh = 1;
            last_frame_mode = sys_run_info->sys_frame_mode;
            cur_frame_mode = KEY_DATA_TOTAL_FRAME;
            sm_unsync_callback();
            
            if (UART_SUCCESS != mg_uart_send_to_all_nation_request_cmd(SM_SET_KEY_MODE_CMD, 1, &cur_frame_mode)) //设置帧模式
            {
                DBG_PRINTF("total mode err\n");
                break;
            }
            vTaskResume(m_cali_thread);
            vTaskDelay(10);
            sys_run_info->sys_frame_mode = cur_frame_mode;
            hal_key_sync_callback(sm_sync_callback);
        break;
        
        default:
        DBG_PRINTF("kh_set_rpt:para err=%d\n", type);
        break;
    }
}

uint8_t cali_is_running(void)
{
    return (cali_thd.step != KEY_CALIB_IDLE);
}

cail_ack_t cali_cmd(cail_step_t calibFlag)
{
    cail_ack_t ack = KEY_CALIB_ACK_SUCCESS;
    
    if (cali_thd.step == KEY_CALIB_SWITCH_OUT)
    {
        //ack = KEY_CALIB_ACK_NOT_READY;
        DBG_PRINTF("key_cali_cmd:cmd %d,%d err, no switch!\n",calibFlag,cali_thd.step);
        //return ack;
        //cali_thd.step = KEY_CALIB_IDLE;
    }

    switch (calibFlag)
    {
        
        case KEY_CALIB_START:
        {
            if (cali_thd.step == KEY_CALIB_START)
            {
                DBG_PRINTF("error 00 calib start fail! (have been open)");
                break;
            }
            cali_para_reset();
            cali_thd.step            = KEY_CALIB_START;
            cali_thd.max_sample_cnt  = 0;
            cali_thd.max_sample_done = false;
            cali_set_ready(false);
            //cali_check_ready         = false;
            vTaskResume(m_cali_thread);
            //kh_reset();
            DBG_PRINTF("calib start ......\n");
        
        } break;
        
        case KEY_CALIB_ABORT:
        {
            if (cali_thd.step == KEY_CALIB_SWITCH_OUT)
            {
                break;
            }
            
            if (cali_thd.step != KEY_CALIB_START)
            {
                DBG_PRINTF("error 01 calib abort fail! (not open)\n");
                ack = KEY_CALIB_ACK_NOT_STARTED;
                break;
            }
            if (key_cali.calibDone != true)
            {
                DBG_PRINTF("error 02 calib abort fail! (not data)\n");
                ack = KEY_CALIB_ACK_NOT_ALL;
                break;
            }
            
            fix_init(&kh);
            rt_init(&kh);
            cali_thd.step = KEY_CALIB_IDLE;
            vTaskDelay( 10);
            cali_set_ready(true);
            //cali_check_ready = true;
            rpt_refresh_cnt  = 5;
            vTaskResume(m_cali_thread);
            DBG_PRINTF("calib abort\n");
        } break;
            
        case KEY_CALIB_DONE:
        {
            if (cali_thd.step == KEY_CALIB_SWITCH_OUT)
            {
                break;
            }
            
            if (cali_thd.step != KEY_CALIB_START)
            {
                DBG_PRINTF("error 03 calib done fail! (not open)\n");
                ack = KEY_CALIB_ACK_NOT_STARTED;
                break;
            }
            
            if (cali_thd.calib_num == 0)
            {
                if (key_cali.calibDone != true)   //还有一些按键从来没有校准过,继续执行校准
                {
                    DBG_PRINTF("some key have not been cali\n");
                    ack = KEY_CALIB_ACK_NOT_ALL;
                    break;
                }
                
                if (kh.p_qty->shaftChangeNum != 0) //有些换轴按键未校准
                {
                    DBG_PRINTF("some change shaft keys have not been calib\n");
                    ack = KEY_CALIB_ACK_NOT_ALL;
                    break;
                }
                
                fix_init(&kh);
                rt_init(&kh);
                //key_dks_init(&kh);
                ack = KEY_CALIB_ACK_SUCCESS;
                cali_thd.step = KEY_CALIB_IDLE;
                cali_set_ready(true);
                //cali_check_ready = true;
                //vTaskResume(m_cali_thread);
                break;
            }
            else if (cali_thd.calib_num == KEYBOARD_NUMBER)//全部按键校准通过
            {
                for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
                {
                    key_cali.min[ki]    = cali_thd.min[ki];
                    key_cali.max[ki]    = cali_thd.max[ki];
                    kh.p_st[ki].diff    = cali_thd.diff[ki];
                    uint8_t sw   = key_cali.sw[ki];
                    if (sw == KEY_SHAFT_TYPE_UNKNOWN || sw >= KEY_SHAFT_TYPE_MAX) //shaft exception
                    {
                        sw = KEY_DFT_SHAFT;
                    }
                    if (sw != KEY_DFT_SHAFT)
                    {
                        DBG_PRINTF("calib swerr =%d\n",sw);
                    }
                    if (kh.p_attr[ki].shaftChange)
                    {
                        kh.p_qty->shaftChangeNum -= 1;
                        kh.p_attr[ki].shaftChange = false;
                        if (kh.p_attr[ki].triggerlevel > sw_config_table[sw].maxLevel)
                        {
                            uint8_t triggerlevel = KEY_DFT_GENERAL_TRG_PRESS_LEVEL(sw);
                            uint8_t raiseLevel   = (triggerlevel + KEY_DFT_GENERAL_TRG_RAISE_OFFSET_LEVEL(sw)) < 0 ? 0 : (triggerlevel + KEY_DFT_GENERAL_TRG_RAISE_OFFSET_LEVEL(sw));
                            uint8_t generalMode  = KEY_GENRAL_MODE_STROKE_SAME;
                            fix_set_para(ki, triggerlevel, raiseLevel, generalMode);
                        }
                    }
                    // tmpMinLevel = tmpMinLevel < sw_config_table[sw].maxLevel ? tmpMinLevel : sw_config_table[sw].maxLevel;
                    // tmpMaxLevel = tmpMaxLevel > sw_config_table[sw].maxLevel ? tmpMaxLevel : sw_config_table[sw].maxLevel;
                }
                DBG_PRINTF("calib all key ok\n");
            }
            else //部分按键校准通过
            {
                if (key_cali.calibDone != true)   //还有一些按键从来没有校准过,继续执行校准
                {
                    DBG_PRINTF("some key have not been cali\n");
                    ack = KEY_CALIB_ACK_NOT_ALL;
                    break;
                }

                uint8_t changeNum = 0;
                for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
                {
                    uint8_t sw = key_cali.sw[ki];
                    if (sw == KEY_SHAFT_TYPE_UNKNOWN || sw >= KEY_SHAFT_TYPE_MAX) //shaft exception
                    {
                        sw = KEY_DFT_SHAFT;
                    }
                    
                    if (kh.p_st[ki].calibState == true)
                    {
                        key_cali.min[ki]    = cali_thd.min[ki];
                        key_cali.max[ki]    = cali_thd.max[ki];
                        kh.p_st[ki].diff    = cali_thd.diff[ki];
                        
                        if (kh.p_attr[ki].shaftChange)
                        {
                            changeNum += 1;
                            DBG_PRINTF("shaftChange [%d]\n", ki);
                            if (kh.p_attr[ki].triggerlevel > sw_config_table[sw].maxLevel)
                            {
                                uint8_t triggerlevel = KEY_DFT_GENERAL_TRG_PRESS_LEVEL(sw);
                                uint8_t raiseLevel   = (triggerlevel + KEY_DFT_GENERAL_TRG_RAISE_OFFSET_LEVEL(sw)) < 0 ? 0 : (triggerlevel + KEY_DFT_GENERAL_TRG_RAISE_OFFSET_LEVEL(sw));
                                uint8_t generalMode  = KEY_GENRAL_MODE_STROKE_SAME;
                                fix_set_para(ki, triggerlevel, raiseLevel, generalMode);
                            }
                        }
                    }
                    // tmpMinLevel = tmpMinLevel < sw_config_table[sw].maxLevel ? tmpMinLevel : sw_config_table[sw].maxLevel;
                    // tmpMaxLevel = tmpMaxLevel > sw_config_table[sw].maxLevel ? tmpMaxLevel : sw_config_table[sw].maxLevel;
                }
                
                if (kh.p_qty->shaftChangeNum == changeNum)
                {
                    for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
                    {
                        kh.p_attr[ki].shaftChange = false;
                    }
                    kh.p_qty->shaftChangeNum = 0;
                }
                else //有些换轴按键未校准
                {
                    DBG_PRINTF("some change shaft keys have not been calib\n");
                    ack = KEY_CALIB_ACK_NOT_ALL;
                    break;
                }
                
                DBG_PRINTF("calib part key ok\n");
            }
            
            //重置微校准参数
            //key_cali_tuning_reset();
            //unset_state(BS_MCU_READY);
            //cmd_mcu_stop_ks();
            //kh_reset();
            cali_calc_level_boundarys(); /* boundary algo calc */
            fix_init(&kh);
            rt_init(&kh);
            //key_dks_init(&kh);
            cali_auto_set_change(false);
            db_update(DB_SYS, UPDATE_NOW);
            db_update(DB_CALI, UPDATE_NOW);
            cali_thd.step = KEY_CALIB_IDLE;
            cali_thd.is_manual_cail_update = true;
            vTaskDelay( 10);
            cali_set_ready(true);
            //cali_check_ready = true;
            rpt_refresh_cnt  = 5;
            vTaskResume(m_cali_thread);
        } break;
        
        case KEY_CALIB_CLEAR:
        {
        #if 0
            uint8_t tmpFlag = instUserMsg->calibration.flag.bit.info;
            // dev_flash_read_calibration_flag();
            if(instUserMsg->calibration.flag.bit.viewInfo==KEY_CALIB_CLEAR)
            {
                instUserMsg->calibration.flag.bit.info = (KeyscanCalibrationFlag_e)tmpFlag;
                sprintf(tmpBuffer, "error 05 calib clear fail !!! (not data)");
                break;
            }
            //dev_flash_read_user_msg_all();
            sprintf(tmpBuffer, "calib data clear");
            cali_result_clear_travel_all();
            keyscan_result_clear_level_boundary_all();
            instUserMsg->calibration.flag.bit.info = calibFlag;
            instUserMsg->calibration.flag.bit.viewInfo = calibFlag;
            //isRespSucceed = dev_flash_write_user_msg_all(); /* save calib data */
            //isRespSucceed = dev_flash_read_user_msg_all();
            if(instUserMsg->calibration.flag.bit.info==KEY_CALIB_CLEAR 
                && instUserMsg->calibration.flag.bit.viewInfo==KEY_CALIB_CLEAR 
                && instUserMsg->calibration.data.travel[0][0] == 0 
                && instUserMsg->calibration.data.min[0][0] == 0xFFFF)
            {
                isRespSucceed = true;
                log_debug("    - %s ok\r\n", tmpBuffer);
                //drv_system_soft_reset();
            }
            else
            {
                isRespSucceed = false;
                strcat(tmpBuffer, "error 06 save fail !!!");
            }
        #endif
        } break;
            
        default:
        {
            DBG_PRINTF("calib unkonw\n");
            ack = KEY_CALIB_ACK_NOT_SUPPORT;
        } break;
    }

    return ack;
}


static void cali_thread(void *pvParameters)
{
    (void)pvParameters;
    
    uint32_t msg;
    key_st_t *pkst;
    uint32_t rpt_cycle = 0;

    // 默认置入工厂数据
#if (BOARD_FW_FOR_FA == 0)
    mg_factory_set_default_data();
#endif

    cali_set_ready(false);
    //1.检查是否过了产测步骤
#if (BOARD_FW_FOR_FA==0)
    cali_thd.step = KEY_CALIB_IDLE;
#else
    uint32_t cnt = 0;
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        pkst = key_st;

        if(((cnt++) % 5) == 0)
        {
            rpt_refresh_cnt = 1;
            kh.p_gst->key_rpt_refresh = 1;
        }
        
        vTaskDelay(10);
        if (kh.p_gst->key_rpt_type != KEY_REPORT_OUTTYPE_CLOSE)
        {
            cali_report_adc(kh.p_gst->key_rpt_type, pkst);
        }
        
        mg_factory_t *factory = mg_factory_get_info();

        uint16_t fa_jn = *(uint16_t *)&factory->fa[0][FA_STEP_MAX_JIANHANG-1]; //最后一步扫码出货
        uint16_t fa_nr = *(uint16_t *)&factory->fa[1][0];

        if (fa_jn != 0xffff && fa_nr != 0xffff)
        {
            cali_thd.step = KEY_CALIB_IDLE;
            break;
        }
    }
#endif

    cali_set_ready(true);
    DBG_PRINTF("cali_thread ready!\n");

    //5.校准业务(自动校准,手动校准,校准预览)
    while (1)
    {
        DBG_PRINTF("cali_thread sleep,read=%d,step=%d,rpt=%d!\n",cali_check_ready, cali_thd.step, kh.p_gst->key_rpt_type);
        vTaskSuspend(m_cali_thread);
        
        while (1)
        {
            if (kh.p_gst->key_rpt_type != KEY_REPORT_OUTTYPE_CLOSE)
            {
                if (cali_thd.step == KEY_CALIB_START)
                {
                    msg = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
                    if (msg == pdFALSE)
                    {
                        DBG_PRINTF("matrix_thread timeout,read=%d,step=%d,rpt=%d!\n",cali_check_ready, cali_thd.step, kh.p_gst->key_rpt_type);
                        continue;
                    }
                    pkst = (key_st_t *)msg;
                    cali_handle(pkst);
                    rpt_cycle++;
                    rpt_cycle %= 64;
                    if (rpt_cycle == 0)
                    {
                        cali_report_adc(kh.p_gst->key_rpt_type, pkst);
                    }
                }
                else
                {
                    pkst = key_st;
                    static uint32_t cnt = 0;
                    if(((cnt++) % 5) == 0)
                    {
                        rpt_refresh_cnt = 1;
                        kh.p_gst->key_rpt_refresh = 1;
                    }
                    
                    vTaskDelay(30);
                    cali_report_adc(kh.p_gst->key_rpt_type, pkst);
                }
            }
            else
            {
                cali_set_ready(true);
                break;
            }
            
        }
    }
    
}

void cali_manual_updata_handle(sm_data_t *p_sm_data)
{
    int32_t ki;
    uint16_t adc_value;
    key_st_t *pkst;
    uint8_t sw_type;
    uint16_t search_start;
    uint8_t total_key = KEY_FRAME_NUM*KEY_FRAME_SIZE;

    for (uint32_t i = 0; i < total_key; i++)
    {
        ki = key_idx_table[i];
        adc_value  =  p_sm_data->pack[i].adc;

        if ((ki == -1) || (adc_value == 0))
        {
            continue;
        }

        pkst      = &key_st[ki];

        key_rt_up_point(adc_value);

        if ((key_cali.calibDone != true) || (cali_thd.step != KEY_CALIB_IDLE))
        {
            search_start = 0;
        }
        else
        {
            sw_type = key_cali.sw[ki];
            //找出变化方向,定向搜索
            search_start = pkst->cur_lvl;
            if (adc_value > pkst->cur_point)//向上
            {
                //pkst->cur_md = KEY_MD_UP;
                while (search_start)
                {
                    // #warning "kh_update_pos1:< or <=?"
                    if (adc_value < KEY_LVL_GET_VAL(ki, search_start-1))
                    {
                        break;
                    }
                    search_start--;
                }
            }
            else if (adc_value < pkst->cur_point)//向下
            {
                //pkst->cur_md = KEY_MD_DOWN;
                while (search_start < sw_config_table[sw_type].maxLevel)
                {
                    if (adc_value > KEY_LVL_GET_VAL(ki, search_start))
                    {
                        break;
                    }
                    search_start++;
                }
            }
        }
        pkst->last_point = pkst->cur_point;
        pkst->cur_point  = adc_value;
        pkst->cur_lvl    = search_start;
        pkst->last_lvl   = 0;
    }
    //把数据发给cali任务进行调零初始化

    xTaskNotify( m_cali_thread, (uint32_t)key_st, eSetValueWithOverwrite);
    vTaskResume(m_cali_thread);
}

bool cali_data_updata_handle(sm_data_t *p_sm_data)
{
    static uint64_t wait_cnt = 0;
    sys_run_info_s *sys_run_info = &sys_run;
    uint32_t ticks_per_ms = clock_get_core_clock_ticks_per_ms();
    uint16_t k_value = 0;
    key_st_t *pkst;
    uint16_t temp_top_val = 0;
    uint16_t temp_bot_val = 0;
    key_cail_param.cail_wait_1000ms = ticks_per_ms * 1000UL;

    if (ONLINE_CONSUMER == get_hive_state())
    {
        if ((kh.p_gst->key_rpt_type == KEY_REPORT_OUTTYPE_CLOSE) && \
            (kh.p_qty->cail_dbg_rpt_en) && (((wait_cnt++) % 500) == 0))
        {
            cali_report_adc_cail();
        }
    }
    else
    {
        if (kh.p_qty->cail_dbg_rpt_en)
        {
            kh.p_qty->cail_dbg_rpt_en = false;
        }
    }

    if(is_timer_key_unchange_flag)
    {
        static uint8_t index = 0;
        if ((key_cail_param.waite_top_cali_updata_buf[index] != NO_UPDATA) || \
            (key_cail_param.waite_bot_cali_updata_buf[index] != NO_UPDATA))
        {
            bool is_idle_updata_flag = false;
            bool is_immed_updata_flag = false;
            pkst = &key_st[index];

            if(!pkst->actionState)
            {
                is_idle_updata_flag = ((hpm_csr_get_core_cycle() - key_last_time[index]) > (key_cail_param.cail_wait_1000ms)) ? true : false;
            } 

            if ((key_cail_param.waite_top_cali_updata_buf[index] == NEED_UPDATA) || \
                (key_cail_param.waite_bot_cali_updata_buf[index] == NEED_UPDATA))
            {
                is_immed_updata_flag = true;
            }

            if(is_idle_updata_flag || is_immed_updata_flag)
            {
                if((key_cail_param.key_temp_cail_buf[KEY_TOP][index] >= sw_thd_table[key_cali.sw[index]].top_min) && \
                   (key_cail_param.key_temp_cail_buf[KEY_TOP][index] <= sw_thd_table[key_cali.sw[index]].top_max))
                {
                    key_cali.max[index] = key_cail_param.key_temp_cail_buf[KEY_TOP][index];
                }

                if((key_cail_param.key_temp_cail_buf[KEY_BOT][index] >= sw_thd_table[key_cali.sw[index]].bottom_min) && \
                   (key_cail_param.key_temp_cail_buf[KEY_BOT][index] <= sw_thd_table[key_cali.sw[index]].bottom_max))
                {
                    key_cali.min[index] = key_cail_param.key_temp_cail_buf[KEY_BOT][index];
                }

                key_st[index].diff  = key_cali.max[index] - key_cali.min[index];
                cali_calc_level_boundary(index);
                key_cail_param.waite_top_cali_updata_buf[index] = NO_UPDATA;
                key_cail_param.waite_bot_cali_updata_buf[index] = NO_UPDATA;
            }
        }

        index = (index + 1) % BOARD_KEY_NUM;
        return (sys_run_info->sys_frame_mode != KEY_DATA_ZERO_FRAME) ? false : true;
    }

    if(sys_run_info->sys_frame_mode == KEY_DATA_ZERO_FRAME)
    {
        static uint8_t first_flag = 1;
        static uint64_t expected_ticks = 0;

        if (first_flag)
        {
            first_flag = 0;
            const uint64_t WAITE_500MS = clock_get_frequency(clock_cpu0)/1;
            expected_ticks = hpm_csr_get_core_cycle() + WAITE_500MS;
        }

        /*1、等待所有按键都更新完成*/
        for (uint32_t i = 0; i < KEY_FRAME_NUM; i++)
        {
            if (p_sm_data->data_type[i] != DATA_CALI_ZERO_FRAME_TYPE)
            {
                continue;
            }

            uint32_t j = i * KEY_FRAME_SIZE;
            uint32_t key_num = ((i + 1) * KEY_FRAME_SIZE);
            for (; j < key_num; j++)
            {
                int8_t ki = key_idx_table[j];
                k_value  =  p_sm_data->pack[j].adc;

                if ((ki == -1) || (k_value == 0))
                {
                    continue;
                }

                if ((key_cali.max[ki] == k_value) && (key_cail_param.waite_top_cali_updata_buf[ki] == NO_UPDATA))
                {
                    test_buf[ki] = k_value;
                    continue;
                }

                temp_top_val = (sw_thd_table[key_cali.sw[ki]].top_max + sw_thd_table[key_cali.sw[ki]].top_min)/2;
                if ((ABS2(temp_top_val, k_value) >= KEY_CALL_OFFSET_VAL) || (k_value < KEY_LVL_GET_VAL(ki, KEY_CALL_OFFSET_LEVEL)))
                {
                    DBG_PRINTF("[%d]=%d,max=%d,l=%d,%d\n",k_value,key_cali.max[ki],KEY_LVL_GET_VAL(ki, 0),KEY_LVL_GET_VAL(ki, 7));
                }
                else
                {
                    test_buf[ki]        = k_value;
                    key_cail_param.key_temp_cail_buf[KEY_TOP][ki] = k_value;
                    if (key_cail_param.waite_top_cali_updata_buf[ki] != NEED_UPDATA)
                    {
                        key_cail_param.waite_top_cali_updata_buf[ki] = NEED_UPDATA;
                    }
                }
                
                //DBG_PRINTF("zero cali :%d,%d\n",ki,k_value);
            } 
        }
        if (hpm_csr_get_core_cycle() >= expected_ticks) //等待500ms
        {
            /*2、下发指令设置阈值帧*/
            uint8_t send_buf[UART_SM_CMD_BUF_SIZE] = {0};
            uint16_t indx = 0;
            send_buf[indx++] = KEY_DATA_THRESHOLD_FRAME;
            sm_unsync_callback();
            
            if (UART_SUCCESS != mg_uart_send_to_all_nation_request_cmd(SM_SET_KEY_MODE_CMD, indx, send_buf)) //设置按键帧模式
            {
            }
            DBG_PRINTF("cali change mode to throushold,%ld,%ld,%ld\n",hpm_csr_get_core_cycle(),expected_ticks,clock_get_frequency(clock_cpu0));

            vTaskDelay(20);
            sys_run_info->sys_frame_mode = KEY_DATA_THRESHOLD_FRAME;
            hal_key_sync_callback(sm_sync_callback);
        }
    }
    else 
    {
        bool have_key_chg_flag = false;
        for (uint32_t i = 0; i < KEY_FRAME_NUM; i++)
        {
            if ((p_sm_data->data_type[i] == DATA_CALI_TOP_FRAME_TYPE) || (p_sm_data->data_type[i] == DATA_CALI_BOT_FRAME_TYPE))
            {
                if ((!cali_tuning_get_state()) || (sys_run.volt_err_level > VOLT_WARN_LEVEL_1))//校准数据不同步
                {
                    continue;
                }

                uint32_t j = i * KEY_FRAME_SIZE;
                uint32_t key_num = ((i + 1) * KEY_FRAME_SIZE);
                for (; j < key_num; j++)
                {
                    int8_t ki = key_idx_table[j];
                    bool need_updata_flag = false;
                    k_value  =  p_sm_data->pack[j].adc;

                    if (p_sm_data->dev_data_num[i] == 0)
                    {
                        break;
                    }

                    if ((ki == -1) || (k_value == 0))
                    {
                        continue;
                    }

                    p_sm_data->dev_data_num[i]--;
                    
                    if (p_sm_data->data_type[i] == DATA_CALI_TOP_FRAME_TYPE)
                    {
                        if (p_sm_data->pack[j].ty != PKT_TYPE_TOP_CALI)
                        {
                            continue;
                        }

                        if ((key_cali.max[ki] == k_value) && (key_cail_param.waite_top_cali_updata_buf[ki] == NO_UPDATA))
                        {
                            continue;
                        }

                        temp_top_val = (sw_thd_table[key_cali.sw[ki]].top_max + sw_thd_table[key_cali.sw[ki]].top_min)/2;
                        if ((k_value < KEY_LVL_GET_VAL(ki, KEY_CALL_OFFSET_LEVEL)) || \
                            (ABS2(k_value, temp_top_val) > KEY_CALL_OFFSET_VAL))
                        {
                            DBG_PRINTF("auto top cali err : %d,%d\n",ki,k_value);
                            continue;
                        }

                        key_cail_param.key_temp_cail_buf[KEY_TOP][ki] = k_value;
                        if (key_cail_param.waite_top_cali_updata_buf[ki] != NEED_UPDATA)
                        {
                            key_cail_param.waite_top_cali_updata_buf[ki] = (need_updata_flag == true) ? NEED_UPDATA : LATER_UPDATA;
                        }
                        DBG_PRINTF("top :%d\n",ki);
                    }
                    else if (p_sm_data->data_type[i] == DATA_CALI_BOT_FRAME_TYPE)
                    {
                        if (p_sm_data->pack[j].ty != PKT_TYPE_BOT_CALI)
                        {
                            continue;
                        }

                        if ((key_cali.min[ki] == k_value) && (key_cail_param.waite_bot_cali_updata_buf[ki] == NO_UPDATA))
                        {
                            continue;
                        }

                        temp_bot_val = (sw_thd_table[key_cali.sw[ki]].bottom_max + sw_thd_table[key_cali.sw[ki]].bottom_min)/2;
                        if ((k_value > KEY_LVL_GET_VAL(ki, sw_config_table[KEY_DFT_SHAFT].maxLevel - KEY_CALL_OFFSET_LEVEL)) || \
                            (ABS2(k_value, temp_bot_val) > KEY_CALL_OFFSET_VAL))
                        {
                            DBG_PRINTF("auto bot cali err : %d,%d\n",ki,k_value);
                            continue;
                        }
                        if ((k_value > key_cali.min[ki]) && (pkst->actionState))
                        {
                             need_updata_flag = true;
                        }

                        key_cail_param.key_temp_cail_buf[KEY_BOT][ki] = k_value;
                        if (key_cail_param.waite_bot_cali_updata_buf[ki] != NEED_UPDATA)
                        {
                            key_cail_param.waite_bot_cali_updata_buf[ki] = (need_updata_flag == true) ? NEED_UPDATA : LATER_UPDATA;
                        }
                        DBG_PRINTF("bot :%d\n",ki);
                    }
                    else
                    {
                        continue;
                    }
                }

                p_sm_data->dev_data_num[i] = 0;
            }
            else
            {
                if ((p_sm_data->data_type[i] == DATA_KEY_FRAME_TYPE) || (p_sm_data->data_type[i] == DATA_CALI_TATOL_FRAME_TYPE) || (p_sm_data->data_type[i] == DATA_NONE_FRAME_TYPE))
                {
                    have_key_chg_flag = true;
                }
            }
        }

        if (have_key_chg_flag)
        {
            return false;
        }
    }
    
    return true;
}

void cali_init(kh_t *p_kh)
{
    p_calib = p_kh->p_calib;

    if (xTaskCreate(cali_thread, "cali_thread", STACK_SIZE_CALI, NULL, PRIORITY_CALI, &m_cali_thread) != pdPASS)
    {
        DBG_PRINTF("cali_thread creation failed!.\n");
        for (;;)
        {
            ;
        }
    }
}

