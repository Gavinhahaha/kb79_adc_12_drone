/**
 *@author: JayWang
 *@brief : Rapid trigger handle
 */

#include "FreeRTOS.h"
#include "task.h"
#include "mg_matrix_type.h"
#include "mg_trg_rt.h"
#include "mg_trg_fix.h"
#include "app_debug.h"
#include "board.h"
#include "hpm_gpio_drv.h"
#include "mg_cali.h"
#include "app_config.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "mg_trg_rt"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif



extern uint64_t key_last_time[KEY_FRAME_TOTAL_SIZE];
extern uint64_t key_wait_50ms;
extern kh_t kh;
extern uint32_t g_key_time_stamp;
extern uint16_t g_tuning_cnt;
extern key_st_t key_st[KEYBOARD_NUMBER];

uint8_t all_key_trg_num = 0;

void rt_dft_para(uint8_t ki, key_attr_t *p_attr)
{
    uint8_t keysw      = key_cali.sw[ki];
    
    if (keysw == KEY_SHAFT_TYPE_UNKNOWN || keysw >= KEY_SHAFT_TYPE_MAX) //shaft exception
    {
        keysw = KEY_DFT_SHAFT;
    }
    /* RT para*/
    p_attr->rt_en            = KEY_DFT_RT_IS_ENABLE;
    p_attr->rtPressSensitive = KEY_DFT_RT_PRESS_SENS_LEVEL(keysw);
    p_attr->rtRaiseSensitive = KEY_DFT_RT_RAISE_SENS_LEVEL(keysw);
    p_attr->rtIsContinue     = KEY_DFT_RT_IS_CONTINUE;
    p_attr->rt_mode          = KEY_DFT_RT_MODE;
    p_attr->rt_ada_extreme   = KEY_DFT_RT_ADAPTIVE_EXTREME_EN;
    p_attr->rt_ada_sens      = KEY_DFT_RT_ADAPTIVE_SENS;
    p_attr->rt_ada_mode      = KEY_DFT_RT_ADAPTIVE_MODE;
    
    //W'A'S'D key default enable rt
    if (ki == 31 || ki == 44 || ki == 45 || ki == 46)
    //if (ki == 0) //debug
    {
        p_attr->rt_en = true;
    }
    fix_special_point_update(ki, p_attr);
}

void rt_check_para(key_attr_t *key_attr)
{
    extern key_glb_st_t kh_gst;

    kh_gst.rt_en_num = 0;
    kh_gst.rtKeyUsedTableNumber = 0;
    
    for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        key_attr_t *p_attr = (key_attr + ki);
        
        if ((p_attr->rtPressSensitive < DIST_TO_LEVEL(KEY_RT_SENS_MIN(ki), KEY_RT_ACC(ki))) || 
            (p_attr->rtPressSensitive > DIST_TO_LEVEL(KEY_RT_SENS_MAX(ki), KEY_RT_ACC(ki))) || 
            (p_attr->rtRaiseSensitive < DIST_TO_LEVEL(KEY_RT_SENS_MIN(ki), KEY_RT_ACC(ki))) || 
            (p_attr->rtRaiseSensitive > DIST_TO_LEVEL(KEY_RT_SENS_MAX(ki), KEY_RT_ACC(ki))))
        {
            rt_dft_para(ki, p_attr);
        }
        if (p_attr->advTrg[0].advType == KEY_ACT_DKS) /* break RT count */
        {
            continue;
        }
        if (p_attr->rt_en == true)
        {
            kh_gst.rt_en_num += 1;
            kh_gst.rtKeyUsedTable[kh_gst.rtKeyUsedTableNumber++] = ki;
        }
    }
}

void rt_set_default_para(void)
{
    kh.p_gst->rt_en_num = 0;
    kh.p_gst->rtKeyUsedTableNumber = 0;
//#warning "feature_rapid_trigger_default_para: need globMsg"

    for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        key_attr_t *p_attr = (kh.p_attr + ki);

        rt_dft_para(ki, p_attr);

        //if (sw_config_table[kh.p_calib->sw[ki]].isRtHighAcc)
        //{
            //int8_t tmpPress = ACC_BETW_CONV(kh.p_attr[ki].rtPressSensitive, KEY_HACC(ki), KEY_NACC(ki));
            //int8_t tmpRaise = ACC_BETW_CONV(kh.p_attr[ki].rtRaiseSensitive, KEY_HACC(ki), KEY_NACC(ki));
            //kh.p_st[ki].rtPressLowAccSens = tmpPress <= 0 ? 1 : tmpPress;
            //kh.p_st[ki].rtRaiseLowAccSens = tmpRaise <= 0 ? 1 : tmpRaise;
        //}

        if (p_attr->advTrg[0].advType == KEY_ACT_DKS) /* break RT count */
        {
            continue;
        }
        if (p_attr->rt_en == true)
        {
            kh.p_gst->rt_en_num += 1;
            kh.p_gst->rtKeyUsedTable[kh.p_gst->rtKeyUsedTableNumber++] = ki;
        }
    }
}

void rt_set_para(uint8_t ki, bool en, bool ctn, uint8_t d_sens, uint8_t u_sens)
{
    key_st_t *currentKeyStatus      = (kh.p_st + ki);
    key_attr_t *p_attr = (kh.p_attr + ki);

    /* 处于快速触发按下时，取消快速触发；常规触发按键的动作需要做相应的动作清除 */
    if (currentKeyStatus->trg_phy == KEY_STATUS_U2D || currentKeyStatus->trg_phy == KEY_STATUS_D2D)
    {
        currentKeyStatus->general.isAction = true;
    }

    int8_t change = (en == true)
                  ? (p_attr->rt_en != en) ? +1 : 0
                  : (p_attr->rt_en != en) ? -1
                  : 0;

    change = (p_attr->advTrg[0].advType == KEY_ACT_DKS) ? 0 : change; /* is DKS break count */
    kh.p_gst->rt_en_num += change;

    /* RT para*/
    p_attr->rt_en            = en;
    p_attr->rtPressSensitive = d_sens;
    p_attr->rtRaiseSensitive = u_sens;
   // p_attr->rt_extreme       = ext;
    bool tmpRtContinue       = ctn;
	

    if(tmpRtContinue != p_attr->rtIsContinue)
    {
        p_attr->rtIsContinue = tmpRtContinue;
        fix_special_point_update(ki, p_attr);
    }
}


uint16_t rt_get_point_depth(uint8_t ki, uint16_t point)
{
    uint16_t search_start = kh.p_st[ki].cur_lvl;
#if (USE_KEY_LVL_MODE == KEY_LVL_MODE_BUFF)
    if (point > KEY_LVL_GET_VAL(ki, 0))
    {
        search_start = 0;
    }
    else if (point < KEY_LVL_GET_VAL(ki, KEY_MAX_LEVEL(ki) - 1))
    {
        search_start = KEY_MAX_LEVEL(ki);
    }
    else
    {
        search_start = binary_search_target_range_index(kh.p_st[ki].levelBoundaryValue, KEY_MAX_LEVEL(ki), point, false);           
    }
#elif (USE_KEY_LVL_MODE == KEY_LVL_MODE_REALTIME)
    if (point > kh.p_st[ki].cur_point)
    {
        while (search_start)
        {
            if (point < KEY_LVL_GET_VAL(ki, search_start-1))
            {
                break;
            }
            search_start--;
        }
    }
    else if (point < kh.p_st[ki].cur_point)
    {
        while (search_start < sw_config_table[key_cali.sw[ki]].maxLevel)
        {
            if (point > KEY_LVL_GET_VAL(ki, search_start))
            {
                break;
            }
            search_start++;
        }
    }
#endif
    return search_start;
}


/**
 * @brief  This function handles
 * @param  None
 * @retval None
 */
bool is_enter_rt(uint8_t ki)
{
    bool isEnter         = (kh.p_st[ki].cur_point < kh.p_attr[ki].triggerpoint);
    return isEnter;
}
/**
 * @brief  This function handles
 * @param  None
 * @retval None
 */
 bool is_abort_rt(uint8_t ki)
{
    bool isAbort         = (kh.p_st[ki].cur_point >= kh.p_st[ki].rtAbortPoint);
    return isAbort;
}
uint64_t last_trg_time = 0;
bool rt_handle(uint8_t ki, uint16_t tmpData)
{
    bool ret = false;
    uint8_t sw_type = key_cali.sw[ki];
    uint64_t cur_timestap = hpm_csr_get_core_cycle();
    key_st_t *pkst = kh.p_st + ki;

    if (tmpData >= pkst->rtAbortPoint)
    {
        if (kh.p_st[ki].rtIsEnter == true)
        {
            pkst->rt_rcv_time = cur_timestap;
            // DBG_PRINTF("[%02d]exit(cur:%04d), abort:%04d\n\n\n", ki, tmpData, kh.p_st[ki].rtAbortPoint);
        }
       //kh.p_st[ki].rts = 0;
        key_rt_clear_state(ki);
        ret = false;
    }
    else if (pkst->rtIsEnter == false)
    {
        uint64_t us = (cur_timestap - pkst->rt_rcv_time) / 480;
        uint8_t trg = true;
        uint32_t key_b2t_time = KEY_IS_BIG(ki) ? KEY_RT_BIG_B2T_US : KEY_RT_B2T_US;
        bool isOk = false;

        if (us < key_b2t_time)
        {
            trg = false;
        }

        if (pkst->trg_down_data[0] == 0) 
        {
            for (int i = 0; i < 5; ++i) 
            {
                pkst->trg_down_data[i] = tmpData;
            }
        } 
        else 
        {
            memmove(pkst->trg_down_data, pkst->trg_down_data + 1, sizeof(uint16_t)*4);
            pkst->trg_down_data[4] = tmpData;
        }
        isOk = ((pkst->trg_down_data[0] >= pkst->trg_down_data[1]) &&
                (pkst->trg_down_data[1] >= pkst->trg_down_data[2]) &&
                (pkst->trg_down_data[2] >= pkst->trg_down_data[3]) &&
                (pkst->trg_down_data[3] >= pkst->trg_down_data[4]) &&
                ((pkst->trg_down_data[0] - pkst->trg_down_data[4]) > 3));

        if(!isOk)
        {
            isOk = (tmpData < kh.p_attr[ki].triggerpoint) ? true : false;
        }

        if ((isOk) && (trg))
        {
            pkst->rt_rcv_time = 0;
            pkst->rtIsEnter = true;
            // pkst->first_d2u_time = cur_timestap;
            key_rt_record_limit_down_point(ki, sw_type, tmpData);
            // DBG_PRINTF("[%02d]E(cur:%04d) trg_phy:%04d, abort:%04d, min/max:%04d,%04d, trgLv:%02d, rs:%d\r\n", ki, tmpData, kh.p_attr[ki].triggerpoint, kh.p_st[ki].rtAbortPoint, kh.p_st[ki].min, kh.p_st[ki].max, kh.p_attr[ki].triggerlevel, kh.p_st[ki].rtPressSens);
            ret = true;
        }
        else
        {
            key_rt_clear_state(ki);
            ret = false;
        }
    }
    else
    {
        uint8_t i = 0;
        bool isOk = false;
        float tmpStep = 0;
        int16_t tmpLevel = 0;
        uint16_t tmpsPoint = 0, change = 0;
        uint32_t tag_up_dbnc = 0, tag_down_dbnc = 0, /*tag_fst_up_us = 0,*/tag_bot_stable_us = 0;
        if (KEY_IS_BIG(ki))
        {
            tag_up_dbnc   = KEY_RT_BIG_DEBOUNCE_MS;
            tag_down_dbnc = KEY_RT_BIG_DEBOUNCE_MS;
            //tag_fst_up_us = KEY_RT_BIG_FIRST_D2U_US;
            tag_bot_stable_us = KEY_RT_BIG_D2U_STABBLE_US;
        }
        else
        {
            tag_up_dbnc   = KEY_RT_UP_DEBOUNCE_MS;
            tag_down_dbnc = KEY_RT_DOWN_DEBOUNCE_MS;
            //tag_fst_up_us = KEY_RT_FIRST_D2U_US;
            tag_bot_stable_us = KEY_RT_D2U_STABBLE_US;
        }

        /* key trigger status */
        if (pkst->trg_phy == KEY_STATUS_U2U)
        {
            // tmpStep = 0.0035f + kh.p_gst->downCompensation; /* Integral compensation */
            uint8_t pslvl = (pkst->rtLimitPointLevel < pkst->rtPressSens)
                            ? ((pkst->rtLimitPointLevel == 0) ? 1 :pkst->rtLimitPointLevel)
                            : pkst->rtPressSens; /* 防止介于退出点与最低触发点之间 */

            for (i = 0; i < pslvl; i++)
            {
                tmpLevel = (pkst->rtLimitPointLevel + i - 1);
                if ((tmpLevel >= KEY_MAX_LEVEL(ki)) || (tmpLevel < 0))
                {
                    if (pkst->rtLimitPointLevel == 0) //防止导致变化量未计算
                    {
                        tmpStep += sw_config_table[sw_type].curvTab[0];
                    }
                    break;
                }
                tmpStep += sw_config_table[sw_type].curvTab[tmpLevel];
            }
            change = (uint16_t)((float)pkst->diff * tmpStep);

            // 速度过大时，释放更快些
             //if (pkst->valid_cnt)
             //{
             //    change *= 0.6f;
             //}

            //kh.p_st[ki].rts = -change;
            tmpsPoint = (pkst->rtUpperLimitPoint - change);
            if ((tmpsPoint <= pkst->min) && (tmpsPoint < (pkst->last_rls_point - change)))
            {
                tmpsPoint = pkst->min;
            }
            isOk = (tmpData < tmpsPoint);
            
            // if (isOk == true)
            // {
            //     BOARD_TEST_PIN0_TOGGLE();
            // }

            if ((isOk == true) && ((cur_timestap - pkst->rt_rcv_time) > tag_down_dbnc))
            {
                key_rt_record_limit_down_point(ki, sw_type, tmpData);
                // DBG_PRINTF("    [%02d]D(cur:%04d) chg:%02d lvl:%02d(%02d) point:%d,%d\n", ki, tmpData, change, tmpLevel, kh.p_st[ki].rtLimitPointLevel, kh.p_st[ki].rtUpperLimitPoint,tmpsPoint);
                ret = true;
            }
            else
            {
                key_rt_record_limit_up_point(ki, sw_type, tmpData);
                ret = false;
            }
        }
        else
        {
            uint8_t rslvl = (pkst->rtLimitPointLevel < pkst->rtRaiseSens)
                            ? ((pkst->rtLimitPointLevel == 0) ? 1 : pkst->rtLimitPointLevel)
                            : pkst->rtRaiseSens; /* 防止有效释放行程不够：灵敏度 > 有效释放行程 */
            
            // tmpRaiseSensitive    = kh.p_st[ki].rtRaiseSens; //debug
            int8_t tmpDeadChange = (kh.p_attr[ki].deadbandBottom - (KEY_MAX_LEVEL(ki) - pkst->rtLimitPointLevel));
            rslvl += ((tmpDeadChange <= 0) ? 0 : tmpDeadChange);

            if ((pkst->rtLimitPointLevel - rslvl - 1) <= 0)
            {
                isOk = (tmpData >= pkst->topRaisePoint);
            }
            else
            {
                /* Sensitive change value */
                for (i = 0; i < rslvl; i++)
                {
                    tmpLevel = (pkst->rtLimitPointLevel - i - 1);
                    tmpStep += sw_config_table[sw_type].curvTab[tmpLevel];
                }

                change = (uint16_t)((float)pkst->diff * tmpStep);

                // 速度过大时，释放更快些
                if ((pkst->valid_cnt) && (rslvl >= 3))
                {
                   change *= 0.8f;
                }

                tmpsPoint = (pkst->rtLowerLimitPoint + change);
                if (tmpsPoint >= pkst->max)
                {
                    tmpsPoint = pkst->max;
                }
                isOk = (tmpData >= tmpsPoint);
            }
            
            // if (isOk == true)
            // {
            //     BOARD_TEST_PIN0_TOGGLE();
            //     BOARD_TEST_PIN0_TOGGLE();
            // }
            
            
            if ((isOk == true) && ((cur_timestap - pkst->rt_rcv_time) > tag_up_dbnc))
            {
                // limit spd
                uint32_t hold_us = ((cur_timestap) - pkst->rtTimestamp) / 480;
                //uint32_t key_fst_up_us = (cur_timestap - pkst->first_d2u_time) / 480;

                if (((pkst->isShapeChange) && (rslvl < 3)) 
                    || (((KEY_MAX_LEVEL(ki)-2) <= pkst->rtLimitPointLevel) && (hold_us < tag_bot_stable_us)))
                {
                    key_rt_clear_state(ki);
                    key_rt_record_limit_down_point(ki, sw_type, tmpData);
                    pkst->rtIsEnter = true;
                    // DBG_PRINTF("    [%02d]Err isc:%d,ht%d,%d\n", ki, kh.p_st[ki].isShapeChange, ht, us);
                    ret = true;
                }
                else
                {
                    pkst->last_rls_point = tmpData;
                    key_rt_record_limit_up_point(ki, sw_type, tmpData);
                    /* DBG_PRINTF multi-line kept as block to avoid //-continuation [-Wcomment] */

                    ret = false;
                }
            }
            else
            {
                key_rt_record_limit_down_point(ki, sw_type, tmpData);
                ret = true;
            }
        }
    }

    if (pkst->trg_phy == KEY_STATUS_U2U)
    {
        if (ret == true) /* down */
        {
            // b_changed  = true;
            //BOARD_TEST_PIN2_WRITE(1);
            pkst->debounce = 0;
            pkst->rt_rcv_time = cur_timestap;
            pkst->trg_phy    = KEY_STATUS_D2D;
            pkst->trg_phy_ts = g_key_time_stamp;
            memset(pkst->trg_down_data, 0, sizeof(pkst->trg_down_data));
            //pkst->rtTimestamp = cur_timestap;
            // if (kh.p_attr[ki].triggerpoint!=cali_cfg.levelBoundaryValue[ki][0])
            //  {
            //      DBG_PRINTF("[%d]=%d,%d,%d,%d,",ki,tmpData,kh.p_attr[ki].triggerpoint,cali_cfg.levelBoundaryValue[ki][0],cali_cfg.max[ki]);
            //      DBG_PRINTF("%d,%d-rt\n",kh.p_attr[ki].triggerlevel,kh.p_attr[ki].raiseLevel);
            //  }
            
        }
    }
    else if (pkst->trg_phy == KEY_STATUS_D2D)
    {
        if (ret == false)/* up */
        {
            pkst->rt_rcv_time = cur_timestap;
            pkst->debounce = 0;
            pkst->trg_phy  = KEY_STATUS_U2U;
            key_last_time[ki] = cur_timestap;
        }
    }
    else
    {
        //DBG_PRINTF("[%d]rt_err %d\n", ki, kh.p_st[ki].trg_phy);
        pkst->trg_phy = KEY_STATUS_U2U;
        /* TODO: need check key state err 2 */
    }
    return ret;
}

void rt_init(kh_t *p_kh)
{
    kh.p_gst->rt_en_num            = 0;
    kh.p_gst->upCompensation       = 0.0f;
    kh.p_gst->downCompensation     = 0.0f;
    kh.p_gst->rtKeyUsedTableNumber = 0;
    rt_ada_init();
    for(uint8_t ki=0; ki<KEYBOARD_NUMBER; ki++)
    {
        key_rt_clear_state(ki);
        /* RT used table */
        if (kh.p_attr[ki].rt_en == true)
        {
            uint8_t index = kh.p_gst->rtKeyUsedTableNumber;
            kh.p_gst->rtKeyUsedTable[index] = ki;
            kh.p_gst->rtKeyUsedTableNumber += 1;
            kh.p_gst->rt_en_num += 1;
        }
        //if (sw_config_table[kh.p_calib->sw[ki]].isRtHighAcc)
        //{
            //int8_t tmpPress = ACC_BETW_CONV(kh.p_attr[ki].rtPressSensitive, KEY_HACC(ki), KEY_NACC(ki));
            //int8_t tmpRaise = ACC_BETW_CONV(kh.p_attr[ki].rtRaiseSensitive, KEY_HACC(ki), KEY_NACC(ki));
            //kh.p_st[ki].rtPressLowAccSens = tmpPress <= 0 ? 1 : tmpPress;
            //kh.p_st[ki].rtRaiseLowAccSens = tmpRaise <= 0 ? 1 : tmpRaise;
        //}
    }
    
}
