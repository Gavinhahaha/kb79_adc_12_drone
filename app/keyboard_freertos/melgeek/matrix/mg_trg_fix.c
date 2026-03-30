/**
  *@author: JayWang
  *@brief : Stroke trigger general handle
  Fixed-point triggering
*/
  
#include "mg_matrix_type.h"
#include "mg_trg_fix.h"
#include "app_debug.h"




#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "mg_trg_fix"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif



extern kh_t kh;
extern key_st_t key_st[];



void fix_dft_ap_para(uint8_t ki, key_attr_t *p_attr)
{
    key_st_t *p_st = (kh.p_st+ki);
    uint8_t keysw  = kh.p_calib->sw[ki];
    
    if (keysw == KEY_SHAFT_TYPE_UNKNOWN || keysw >= KEY_SHAFT_TYPE_MAX) //shaft exception
    {
        keysw = KEY_DFT_SHAFT;
    }

    /* stroke trigger level and point */
    p_attr->triggerlevel = KEY_DFT_GENERAL_TRG_PRESS_LEVEL(keysw);
    p_attr->raiseLevel   = (p_attr->triggerlevel + KEY_DFT_GENERAL_TRG_RAISE_OFFSET_LEVEL(keysw)) < 0 ? 0 : (p_attr->triggerlevel + KEY_DFT_GENERAL_TRG_RAISE_OFFSET_LEVEL(keysw));
    /* General */
    p_attr->generalMode  = KEY_GENRAL_MODE_STROKE_SAME;

    /* 加入死区 */
    p_st->triggerAddDeadLevel = (p_attr->triggerlevel > (1 + p_attr->deadbandTop)) 
                                            ? (p_attr->triggerlevel)
                                            : (1+p_attr->deadbandTop);
    p_st->raiseAddDeadLevel = (p_attr->raiseLevel < (sw_config_table[keysw].maxLevel - p_attr->deadbandBottom)) 
                                        ? (p_attr->raiseLevel)
                                        : (sw_config_table[keysw].maxLevel-p_attr->deadbandBottom);
    
    fix_special_point_update(ki, p_attr);

    uint8_t triggerLevelPos = p_st->triggerAddDeadLevel - 1;
    uint8_t raiseLevelPos   = (p_st->raiseAddDeadLevel - 1) < 0 ? 0 : (p_st->raiseAddDeadLevel - 1);
    p_attr->triggerpoint    = KEY_LVL_GET_VAL(ki, triggerLevelPos);
    p_attr->raisePoint      = (p_st->raiseAddDeadLevel == 0) ? p_st->topRaisePoint : KEY_LVL_GET_VAL(ki, raiseLevelPos);
    p_attr->shaftChange     = false;
}
void fix_dft_dead_para(uint8_t ki, key_attr_t *p_attr)
{
    uint8_t keysw  = key_cali.sw[ki];
    key_st_t *p_st = &key_st[ki];

    if (keysw == KEY_SHAFT_TYPE_UNKNOWN || keysw >= KEY_SHAFT_TYPE_MAX) //shaft exception
    {
        keysw = KEY_DFT_SHAFT;
    }

    p_attr->deadbandTop    = KEY_DFT_DEAD_TOP_LEVEL(keysw);
    p_attr->deadbandBottom = KEY_DFT_DEAD_BOT_LEVEL(keysw);
    
    /* 加入死区 */
    p_st->triggerAddDeadLevel = (p_attr->triggerlevel > (1 + p_attr->deadbandTop)) 
                                            ? (p_attr->triggerlevel)
                                            : (1+p_attr->deadbandTop);
    p_st->raiseAddDeadLevel   = (p_attr->raiseLevel < (sw_config_table[keysw].maxLevel - p_attr->deadbandBottom)) 
                                            ? (p_attr->raiseLevel)
                                            : (sw_config_table[keysw].maxLevel - p_attr->deadbandBottom);
    
    fix_special_point_update(ki, p_attr);

    uint8_t triggerLevelPos = p_st->triggerAddDeadLevel - 1;
    uint8_t raiseLevelPos   = (p_st->raiseAddDeadLevel - 1) < 0 ? 0 : (p_st->raiseAddDeadLevel - 1);
    p_attr->triggerpoint    = KEY_LVL_GET_VAL(ki, triggerLevelPos);
    p_attr->raisePoint      = (p_st->raiseAddDeadLevel == 0) ? p_st->topRaisePoint : KEY_LVL_GET_VAL(ki, raiseLevelPos);
}


//检查参数
void fix_check_para(key_attr_t *key_attr)
{
    for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        key_attr_t *p_attr = (key_attr + ki);
        if ((p_attr->deadbandTop > DIST_TO_LEVEL(KEY_DEADBAND_MAX(ki), KEY_DEADBAND_ACC(ki))) || 
            (p_attr->deadbandBottom > DIST_TO_LEVEL(KEY_DEADBAND_MAX(ki), KEY_DEADBAND_ACC(ki))))
        {
            fix_dft_dead_para(ki, p_attr);
        }

        if ((p_attr->triggerlevel > KEY_MAX_LEVEL(ki)) || 
            (p_attr->triggerlevel < DIST_TO_LEVEL(KEY_GENERAL_TRG_MIN(ki), KEY_GENERAL_TRG_ACC(ki))) || 
            (p_attr->raiseLevel > KEY_MAX_LEVEL(ki)))
        {
            fix_dft_ap_para(ki, p_attr);
        }
    }
}

//复位参数
void fix_set_default_dead_para(void)
{
    for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        key_attr_t *p_attr = (kh.p_attr+ki);
        fix_dft_dead_para(ki, p_attr);
    }
}

void fix_set_default_ap_para(void)
{
    for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        key_attr_t *p_attr = (kh.p_attr+ki);
        fix_dft_ap_para(ki, p_attr);
    }
}

void fix_set_para(uint8_t ki, uint8_t triggerLevel, uint8_t raiseLevel, uint8_t mode)
{    
    key_st_t *p_st        = (kh.p_st+ki);
    key_attr_t *p_attr    = (kh.p_attr+ki);
    uint8_t keysw  = kh.p_calib->sw[ki];

    /* stroke trigger level and point */
    p_attr->triggerlevel = triggerLevel;
    p_attr->raiseLevel   = raiseLevel;
    /* General */
    p_attr->generalMode  = (key_general_mode_e)mode;
    
    /* 加入死区 */
    p_st->triggerAddDeadLevel = (p_attr->triggerlevel > (1+p_attr->deadbandTop)) 
                                          ? (p_attr->triggerlevel)
                                          : (1+p_attr->deadbandTop);
    p_st->raiseAddDeadLevel   = (p_attr->raiseLevel < (sw_config_table[keysw].maxLevel-p_attr->deadbandBottom)) 
                                          ? (p_attr->raiseLevel)
                                          : (sw_config_table[keysw].maxLevel-p_attr->deadbandBottom);
    
    fix_special_point_update(ki, p_attr);

    uint8_t triggerLevelPos           = p_st->triggerAddDeadLevel - 1;
    uint8_t raiseLevelPos             = (p_st->raiseAddDeadLevel - 1) < 0 ? 0 : (p_st->raiseAddDeadLevel - 1);
    p_attr->triggerpoint = KEY_LVL_GET_VAL(ki, triggerLevelPos);
    p_attr->raisePoint   = (p_st->raiseAddDeadLevel == 0) ? p_st->topRaisePoint : KEY_LVL_GET_VAL(ki, raiseLevelPos);
    
}

void fix_set_dead_para(uint8_t ki, uint8_t topDead, uint8_t bottomDead)
{
    // if (topDead > DIST_TO_LEVEL(KEY_DEADBAND_MAX(ki), KEY_DEADBAND_ACC(ki)) || bottomDead > DIST_TO_LEVEL(KEY_DEADBAND_MAX(ki), KEY_DEADBAND_ACC(ki)) )
    // {
    //     return;
    // }
    
    key_st_t *p_st        = (kh.p_st+ki);
    key_attr_t *p_attr    = (kh.p_attr+ki);
    uint8_t keysw  = kh.p_calib->sw[ki];
    
    p_attr->deadbandTop    = topDead;
    p_attr->deadbandBottom = bottomDead;
    
    /* 加入死区 */
    p_st->triggerAddDeadLevel = (p_attr->triggerlevel > (1+p_attr->deadbandTop)) 
                                          ? (p_attr->triggerlevel)
                                          : (1+p_attr->deadbandTop);
    p_st->raiseAddDeadLevel   = (p_attr->raiseLevel < (sw_config_table[keysw].maxLevel-p_attr->deadbandBottom)) 
                                          ? (p_attr->raiseLevel)
                                          : (sw_config_table[keysw].maxLevel-p_attr->deadbandBottom);
    
    fix_special_point_update(ki, p_attr);
    
    uint8_t triggerLevelPos           = p_st->triggerAddDeadLevel - 1;
    uint8_t raiseLevelPos             = (p_st->raiseAddDeadLevel - 1) < 0 ? 0 : (p_st->raiseAddDeadLevel - 1);
    p_attr->triggerpoint = KEY_LVL_GET_VAL(ki, triggerLevelPos);
    p_attr->raisePoint   = (p_st->raiseAddDeadLevel == 0) ? p_st->topRaisePoint : KEY_LVL_GET_VAL(ki, raiseLevelPos);
    
}

/**
 * @brief  触发按键的按下与释放事件，由设置的行程点触发的 (有效行程在两点之间)
 * @param  None
 * @retval None
 */
int fix_process(uint8_t ki, uint16_t new_point)
{
    bool ret = false;
    
    key_st_t *p_st        = (kh.p_st+ki);
    key_attr_t *p_attr    = (kh.p_attr+ki);
    
    uint8_t keysw  = kh.p_calib->sw[ki];
    uint16_t triggerpoint = p_attr->triggerpoint;
    uint16_t raisePoint   = p_attr->raisePoint;

    if (p_attr->generalMode == KEY_GENRAL_MODE_STROKE_ADVANCE) /* 提前释放，需经过释放点后 */
    {
        p_st->general.raiseLevel = (p_st->raiseAddDeadLevel - 1) < 0 ? 0 : (p_st->raiseAddDeadLevel - 1);
        p_st->general.raiseLevel = ((p_st->general.raiseLevel+1) >= sw_config_table[keysw].maxLevel) ? (sw_config_table[keysw].maxLevel-1) : (p_st->general.raiseLevel+1);
        raisePoint               = KEY_LVL_GET_VAL(ki, p_st->general.raiseLevel);
    }

    KeyStatusPosition_e keyPos = p_st->pos;
    if (p_st->general.raiseLevel == 0)
    {
        p_st->pos = KEY_STATUS_POS_MIDWAY; //特殊处理，不需要置顶条件
    }
    
    bool actionEndIsTop = (p_attr->generalMode == KEY_GENRAL_MODE_STROKE_SAME) ? false 
                        : (p_attr->generalMode == KEY_GENRAL_MODE_STROKE_LAG) ? true 
                        : (p_attr->generalMode == KEY_GENRAL_MODE_STROKE_ADVANCE) ? true 
                        : true;

    /* key trigger status */
    if (p_st->trg_phy == KEY_STATUS_D2U || p_st->trg_phy == KEY_STATUS_U2U)
    {
        bool isOk = false;
        if (p_attr->generalMode < KEY_GENRAL_MODE_MAX) //all mode
        {
            isOk = (new_point < triggerpoint);
            // isOk = (new_point < triggerpoint) ^ sw_config_table[keysw].voltAscend;
        }
        if (isOk == true && p_st->general.isAction == false)
        {
            p_st->general.isAction = true;
            DBG_PRINTF("U2U, %d\n", ret);
        }
        ret = isOk;
    }
    else if (p_st->trg_phy == KEY_STATUS_U2D || p_st->trg_phy == KEY_STATUS_D2D)
    {
        bool isOk = false;
        if (p_attr->generalMode == KEY_GENRAL_MODE_STROKE_SAME || p_attr->generalMode == KEY_GENRAL_MODE_STROKE_LAG)
        {
            isOk = (new_point >= raisePoint);
            // isOk = (new_point >= raisePoint) ^ sw_config_table[keysw].voltAscend;
        }
        else if (p_attr->generalMode == KEY_GENRAL_MODE_STROKE_ADVANCE)
        {
            if (p_st->general.isRaiseBack == false)
            {
                isOk = (new_point < raisePoint);
                // isOk = (new_point < raisePoint) ^ sw_config_table[keysw].voltAscend;
                if (isOk == true)
                {
                    p_st->general.isRaiseBack = isOk;
                    p_st->general.raiseLevel -= 1; //come back raise point
                    raisePoint = KEY_LVL_GET_VAL(ki, p_st->general.raiseLevel);
                    isOk       = false; //clear
                }
            }
            else if (p_st->general.isRaiseBack == true)
            {
                isOk = (new_point > raisePoint);
                // isOk = (new_point > raisePoint) ^ sw_config_table[keysw].voltAscend;
            }
        }
        
        if ((isOk==true || p_st->pos == KEY_STATUS_POS_TOP) && p_st->general.isAction==true)
        {
            p_st->general.isAction    = actionEndIsTop==false ? false : true; /* End this action */
            p_st->general.isRaiseBack = false;
            DBG_PRINTF("D2D, %d\r\n", ret);
        }
        ret = !isOk;
    }

    if (actionEndIsTop == true && p_st->pos == KEY_STATUS_POS_TOP ) /* End this action */
    {
        p_st->general.isAction = false;
    }
    
    p_st->pos = keyPos; //restore key pos status

    return ret;
}

// uint16_t p1 = 0, p2 = 0;

 /**
  * @brief  This function handles update special point
  * @param  None
  * @retval None
  */
void fix_special_point_update(uint8_t ki, key_attr_t *p_attr)
{
    uint16_t tmpAbortPoint;
    
    key_st_t *p_st      = &key_st[ki];
    key_cali_t *p_calib = kh.p_calib;
    // uint8_t keysw = key_cali.sw[ki];
    // if (sw_config_table[keysw].voltAscend == false)
    {
        // p1 = KEY_TOP_WAVE(ki);
        // p2 = KEY_BOT_WAVE(ki);
        p_st->topRaisePoint    = p_calib->max[ki] - KEY_TOP_WAVE(ki);
        p_st->bottomPressPoint = p_calib->min[ki] + KEY_BOT_WAVE(ki);
    }
    // else
    // {
    //     kh.p_st[ki].topRaisePoint    = key_cali.min[ki] + (KEY_TOP_WAVE);
    //     kh.p_st[ki].bottomPressPoint = key_cali.max[ki] - (KEY_BOT_WAVE);
    // }
    
    if (p_attr->rtIsContinue == true)
    {
        tmpAbortPoint = (KEY_LVL_GET_VAL(ki, 0));
    }
    else 
    {
        //tmpAbortPoint = (kh.p_attr[ki].raisePoint);
        uint8_t raiseLevelPos = (p_st->raiseAddDeadLevel - 1) < 0 ? 0 : (p_st->raiseAddDeadLevel - 1);
        tmpAbortPoint = (p_st->raiseAddDeadLevel == 0) ? p_st->topRaisePoint : KEY_LVL_GET_VAL(ki, raiseLevelPos);
    }

    if (p_st->triggerAddDeadLevel == 1) //special
    {
        tmpAbortPoint = p_st->topRaisePoint;
    }
    
    p_st->rtAbortPoint = tmpAbortPoint;
}

void fix_init(kh_t *p_kh)
{
    
    key_st_t *p_st;
    key_attr_t *p_attr;
    key_cali_t *p_calib   = kh.p_calib;
//    kh.p_gst->useShaftMaxLevelId = 0;
    for(uint8_t ki=0; ki<KEYBOARD_NUMBER; ki++)
    {
        p_st   = (kh.p_st+ki);
        p_attr = (kh.p_attr+ki);
        uint8_t keysw = p_calib->sw[ki];
        //kh.p_gst->useShaftMaxLevelId    = (kh.p_gst->useShaftMaxLevelId > sw_config_table[keysw].maxLevel)
        //                                   ? kh.p_gst->useShaftMaxLevelId
        //                                   : sw_config_table[keysw].maxLevel;
        
        p_st->triggerAddDeadLevel = (p_attr->triggerlevel > (1 + p_attr->deadbandTop)) 
                                           ? (p_attr->triggerlevel)
                                           : (1 + p_attr->deadbandTop);
        p_st->raiseAddDeadLevel   = (p_attr->raiseLevel < (sw_config_table[keysw].maxLevel - p_attr->deadbandBottom)) 
                                           ? (p_attr->raiseLevel)
                                           : (sw_config_table[keysw].maxLevel - p_attr->deadbandBottom);
    
        fix_special_point_update(ki, p_attr);
    }

}

