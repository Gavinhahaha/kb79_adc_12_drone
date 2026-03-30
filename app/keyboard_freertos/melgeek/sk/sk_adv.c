/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */


#include "sk_adv.h"
#include "db.h"
#include <string.h>
#include "mg_hive.h"
#include "interface.h"
#include "mg_hid.h"
#include "app_debug.h"
#include "mg_matrix.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "sk_macro"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

extern kh_t kh;
static kc_t replace_events[6];
static int key_count = 0;
static rep_hid_t *kb_report = NULL;
static rep_hid_t *ms_report = NULL;
static rep_hid_t *adv_kb_report = NULL;

static void dispatch_kc_keyevent(uint8_t press, kc_t *events, uint8_t count)
{
    bool ms_change = false;
    adv_kb_report->report_id = gbinfo.nkro ? REPORT_ID_NKRO : REPORT_ID_KEYBOARD;

    if (press == KEY_STATUS_U2D)
    {
        for (uint8_t i = 0; i < count; ++i)
        {
            if (events[i].ty == KCT_KB)
            {
                report_add_key(adv_kb_report, events[i].co);
            }
            else if (events[i].ty == KCT_MS)
            {
                ms_change = true;
                if (events[i].sty <= KC_MS_BTN_SP)
                {
                    report_add_ms_btn(ms_report, events[i].sty);
                }
                else if (events[i].sty <= KC_MS_PY)
                {
                    int8_t val = (events[i].sty == KC_MS_PY) ? events[i].co : events[i].co * -1;
                    report_set_ms_y(ms_report, val);
                }
                else if (events[i].sty <= KC_MS_PX)
                {
                    int8_t val = (events[i].sty == KC_MS_PX) ? events[i].co : events[i].co * -1;
                    report_set_ms_x(ms_report, val);
                }
                else if (events[i].sty <= KC_MS_WD)
                {
                    int8_t val = (events[i].sty == KC_MS_WD) ? -1 : 1;
                    report_set_ms_w(ms_report, val);
                }
                else
                {
                    ms_change = false;
                }
            }
        }
    }
    else
    {
        for (uint8_t i = 0; i < count; ++i)
        {
            if (events[i].ty == KCT_KB)
            {
                report_del_key(adv_kb_report, events[i].co);
            }
            else if (events[i].ty == KCT_MS)
            {
                ms_change = true;
                if (events[i].sty <= KC_MS_BTN_SP)
                {
                    report_del_ms_btn(ms_report, events[i].sty);
                }
                else if (events[i].sty <= KC_MS_PY)
                {
                    report_set_ms_y(ms_report, 0);
                }
                else if (events[i].sty <= KC_MS_PX)
                {
                    report_set_ms_x(ms_report, 0);
                }
                else if (events[i].sty <= KC_MS_WD)
                {
                    report_set_ms_w(ms_report, 0);
                }
                else
                {
                    ms_change = false;
                }
            }
        }
    }

    if (ms_change)
    {
        sk_rep_ms_change();
    }

    if (adv_kb_report->report_id == REPORT_ID_NKRO)
    {
        sk_rep_kb_change();
    }
    else
    {
        uint32_t msg = (uint32_t)adv_kb_report;
        xQueueSend(hid_queue, (const void * const )&msg, ( TickType_t ) 0 );
    }
}
static void dispatch_keyevent(uint8_t press, uint16_t kco)
{
    adv_kb_report->report_id = gbinfo.nkro ? REPORT_ID_NKRO : REPORT_ID_KEYBOARD;

    if (press)
    {
        report_add_key(adv_kb_report, kco);
    }
    else
    {
        report_del_key(adv_kb_report, kco);
    }

    if (adv_kb_report->report_id == REPORT_ID_NKRO)
    {
        sk_rep_kb_change();
    }
    else
    {
        uint32_t msg = (uint32_t)adv_kb_report;
        xQueueSend(hid_queue, (const void * const )&msg, ( TickType_t ) 0 );
    }
}


adv_match_t adv_replace_key(adv_t *padv, uint8_t press)
{
    adv_match_t match = ADV_MATCH_UNSEND;

    if (press == KEY_STATUS_U2D)
    {
        key_count = 0;
        while (padv->data.kc[key_count].ty != KCT_NONE)
        {
            memcpy(&replace_events[key_count], &padv->data.kc[key_count], sizeof(kc_t));
            key_count++;
            if (key_count >= 6)
            {
                break;
            }
        }
        if (key_count == 0)
        {
            DBG_PRINTF("adv_replace_key err\n");
        }
        match = ADV_MATCH_SEND;
    }

    dispatch_kc_keyevent(press, replace_events, key_count);

    return match;
}

adv_match_t adv_en_disable_key(adv_t *padv)
{
    uint8_t buf[24] = {0};
    uint8_t key_num = 0;
    uint8_t size;

    while (padv->data.kc[key_num].ty != KCT_NONE)
    {
        key_num++;
        if (key_num > 6)
        {
            break;
        }
    }

    if (key_num == 0)
    {
        DBG_PRINTF("adv_en_disable_key err\n");
    }

    for (int m = 0; m < BOARD_KCM_MAX; ++m)
    {
        for (int i = 0; i < BOARD_KEY_NUM; ++i)
        {
            for (int k = 0; k < key_num; ++k)
            {
                if (gmx.pkc[m][i].ty == padv->data.kc[k].ty &&
                    gmx.pkc[m][i].sty == padv->data.kc[k].sty &&
                    gmx.pkc[m][i].co == padv->data.kc[k].co)
                {
                    gmx.pkc[m][i].en = (gmx.pkc[m][i].en == KCE_ENABLE) ? KCE_DISABLE : KCE_ENABLE;
                    padv->data.kc[k].en = gmx.pkc[m][i].en;
                    //TODO:LED DISPLAY
                }
            }
        }
    }

    memcpy(buf, (uint8_t *)padv->data.kc, sizeof(kc_t) * key_num);
    cmd_do_response(CMD_NOTIFY, NTF_ADV_LOCK, sizeof(kc_t) * key_num, 0, buf);
    db_update(DB_SK_LA_LM_KC, UPDATE_LATER);

    return ADV_MATCH_UNSEND;
}

void adv_enable_key(adv_t *padv)
{
    uint8_t key_num = 0;

    while (padv->data.kc[key_num].ty != KCT_NONE)
    {
        key_num++;
        if (key_num > 6)
        {
            break;
        }
    }

    for (int m = 0; m < BOARD_KCM_MAX; ++m)
    {
        for (int i = 0; i < BOARD_KEY_NUM; ++i)
        {
            for (int k = 0; k < key_num; ++k)
            {
                if (gmx.pkc[m][i].ty == padv->data.kc[k].ty &&
                    gmx.pkc[m][i].sty == padv->data.kc[k].sty &&
                    gmx.pkc[m][i].co == padv->data.kc[k].co)
                {
                    gmx.pkc[m][i].en = KCE_ENABLE;
                }
            }
        }
    }
}

adv_match_t adv_action(adv_t *padv)
{
    uint8_t buf[2] = {0};

    buf[0] = padv->adv_co;
    buf[1] = padv->adv_ex;

    cmd_do_response(CMD_NOTIFY, NTF_ADV_APP, sizeof(buf), 0, buf);

    return ADV_MATCH_UNSEND;
}

int8_t adv_scene_is_pass(adv_t *padv)
{
    int8_t ret = -1;

    /* rt unit 0.01mm */
    const uint8_t unit = 10;
    uint8_t keyShaftType = 0;
    uint8_t minLevel = 0xFF;
    uint8_t minLevelKi = 0;
    uint8_t ki = 0;
    uint8_t count = 0;

    uint8_t kiSize = ARRAY_SIZE(padv->data.trg_sence.kid);
    for (uint8_t i = 0; i < kiSize; i++)
    {
        ki = padv->data.trg_sence.kid[i];
        if (ki == 0xFF)
        {
            count++;
            continue;
        }
        // keyShaftType = cali_cfg.shaftType[ki];

        // find min level
        if (minLevel > sw_config_table[keyShaftType].maxLevel)
        {
            minLevel = sw_config_table[keyShaftType].maxLevel;
            minLevelKi = ki;
        }
    } 

    if (count == kiSize) // NULL
    {
        return ret;
    }

    ki = minLevelKi;
    // keyShaftType = cali_cfg.shaftType[ki];
    DBG_PRINTF("scene: minLevelKi: %d, shaftType:%d\n", ki, keyShaftType);
#warning "debug shaft, todo jay"

    if (padv->data.trg_sence.trigger < KEY_GENERAL_TRG_ACC(ki) 
    || padv->data.trg_sence.trigger > sw_config_table[keyShaftType].maxLevel 
    || padv->data.trg_sence.raise < KEY_GENERAL_TRG_ACC(ki) 
    || padv->data.trg_sence.raise > sw_config_table[keyShaftType].maxLevel 
    || padv->data.trg_sence.pressStroke < (KEY_RT_ACC(ki) * KEY_RT_SENS_MIN(ki) * unit) 
    || padv->data.trg_sence.pressStroke > (KEY_RT_ACC(ki) * KEY_RT_SENS_MAX(ki) * unit) 
    || padv->data.trg_sence.raiseStroke < (KEY_RT_ACC(ki) * KEY_RT_SENS_MIN(ki) * unit) 
    || padv->data.trg_sence.raiseStroke > (KEY_RT_ACC(ki) * KEY_RT_SENS_MAX(ki) * unit) 
    || padv->data.trg_sence.topDead < (KEY_DEADBAND_ACC(ki) * KEY_DEADBAND_MIN(ki)) 
    || padv->data.trg_sence.topDead > (KEY_DEADBAND_ACC(ki) * KEY_DEADBAND_MAX(ki)) 
    || padv->data.trg_sence.bottomDead < (KEY_DEADBAND_ACC(ki) * KEY_DEADBAND_MIN(ki)) /* zero pointless */
    || padv->data.trg_sence.bottomDead > (KEY_DEADBAND_ACC(ki) * KEY_DEADBAND_MAX(ki)))
    {
        ret = -1;
    }
    else
    {
        ret = ki; // return min level ki
    }

    DBG_PRINTF("is pass adv rt : trg:%d, rai:%d, topD:%d, botD:%d, \n", 
        padv->data.trg_sence.trigger,  padv->data.trg_sence.raise,      padv->data.trg_sence.topDead,     padv->data.trg_sence.bottomDead);
    DBG_PRINTF("is pass adv rt : isE:%d, isC:%d, preS:%d, raiS:%d, \n",
        padv->data.trg_sence.isEnable, padv->data.trg_sence.isContinue, padv->data.trg_sence.pressStroke, padv->data.trg_sence.raiseStroke);

    return ret;
}

// adv_match_t adv_scene(adv_t *padv)
// {
//     int8_t minLevelKi = adv_scene_is_pass(padv);
//     if (minLevelKi < 0)
//     {
//         return ADV_MATCH_UNSEND;
//     }

//     /* rt unit 0.01mm */
//     uint8_t unit = 10;
//     uint8_t ki = minLevelKi;
//     uint8_t keyShaftType = 0;//cali_cfg.shaftType[ki];//TODO:
// #warning "debug shaft, todo jay"

//     key_general_mode_e mode;
//     uint8_t tmpTrigger = padv->data.trg_sence.trigger / KEY_GENERAL_TRG_ACC(ki); /* get trigger level */
//     uint8_t tmpRaise = padv->data.trg_sence.raise / KEY_GENERAL_TRG_ACC(ki);
//     uint8_t tmpTopDead = padv->data.trg_sence.topDead / KEY_DEADBAND_ACC(ki); /* get dead band */
//     uint8_t tmpBottomDead = padv->data.trg_sence.bottomDead / KEY_DEADBAND_ACC(ki);
//     uint8_t tmpPressSens = padv->data.trg_sence.pressStroke / (KEY_RT_ACC(ki) * unit); /* get trigger sensitive */
//     uint8_t tmpRaiseSens = padv->data.trg_sence.raiseStroke / (KEY_RT_ACC(ki) * unit);
//     uint8_t tmpIsEnable = padv->data.trg_sence.isEnable;
//     uint8_t tmpIsContinue = padv->data.trg_sence.isContinue;
//     bool isUpdateAttr = false;

//     if (tmpTrigger == tmpRaise)
//     {
//         mode = KEY_GENRAL_MODE_STROKE_SAME;
//         tmpRaise = (tmpTrigger + KEY_DFT_GENERAL_TRG_RAISE_OFFSET_LEVEL(keyShaftType)) < 0 ? 0 : (tmpTrigger + KEY_DFT_GENERAL_TRG_RAISE_OFFSET_LEVEL(keyShaftType));
//     }
//     else if (tmpTrigger > tmpRaise)
//     {
//         mode = KEY_GENRAL_MODE_STROKE_LAG;
//     }
//     else //(tmpTrigger<tmpRaise)
//     {
//         mode = KEY_GENRAL_MODE_STROKE_ADVANCE;
//     }

//     uint8_t kiSize = ARRAY_SIZE(padv->data.trg_sence.kid);
//     for (uint8_t i = 0; i < kiSize; i++)
//     {
//         uint8_t ki = padv->data.trg_sence.kid[i];
//         if (ki == 0xFF)
//         {
//             continue;
//         }

//         isUpdateAttr = true;
//         feature_dead_set_para(ki, tmpTopDead, tmpBottomDead);
//         feature_general_trigger_set_para(ki, tmpTrigger, tmpRaise, mode);
//         feature_rapid_trigger_set_para(ki, tmpIsEnable, tmpIsContinue, tmpPressSens, tmpRaiseSens);

//         /* rt number */
//         kh.p_gst->rtKeyUsedTableNumber = 0;
//         for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
//         {
//             if ((kh.p_attr + ki)->rt_en == true && (kh.p_attr + ki)->advTrg[0].advType != KEY_ACT_DKS)
//             {
//                 kh.p_gst->rtKeyUsedTable[kh.p_gst->rtKeyUsedTableNumber++] = ki;
//             }
//         }
//     }

//     if (isUpdateAttr)
//     {
//         cmd_do_response(CMD_NOTIFY, NTF_ADV_SCENE, 0, 0, NULL);
//         db_update(DB_SYS, UPDATE_LATER);
//     }

//     return ADV_MATCH_UNSEND;
// }


adv_match_t adv_mt(adv_t *padv, uint8_t ki, uint8_t press)
{
    adv_match_t match = ADV_MATCH_UNSEND;
    uint8_t kcmid = kh.p_st[ki].kcmidNow;
    kc_t kc_hold  = padv->data.mt.kc[0]; //hold
    kc_t kc_tap   = padv->data.mt.kc[1]; //tap
    //DBG_PRINTF("ADV click MT, hd:%d, hco:0x%x, tco:0x%x \n", hd, kc_hold.co, kc_tap.co);

    
    if (press == KEY_STATUS_D2U)
    {
        if (kh.p_st[ki].advTrg[kcmid].mt.isLoogPress == true)
        {
            kh.p_st[ki].advTrg[kcmid].mt.isLoogPress = false;
            //kh.p_st[ki].advTrg[kcmid].mt.timestamp   = 0;
            dispatch_keyevent(false, kc_hold.co);
            match = ADV_MATCH_SEND;
        }
        else
        {
            dispatch_keyevent(true, kc_tap.co);

            vTaskDelay(1);
            dispatch_keyevent(false, kc_tap.co);
            match = ADV_MATCH_SEND;
        }
    }
   // else if (press == KEY_STATUS_U2U)
    //{
    //    if (kh.p_st[ki].advTrg[kcmid].mt.trg != press && kh.p_st[ki].advTrg[kcmid].mt.timestamp != 0)
    //    {
    //        dispatch_keyevent(false, kc_tap.co);
    //        DBG_PRINTF("[%02d]MT UP(on click) co:0x%x\r\n\r\n", ki, kco);
    //    }
   // }
    else if (press == KEY_STATUS_U2D)
    {
        //kh.p_st[ki].advTrg[kcmid].mt.timestamp = xTaskGetTickCount();
        kh.p_st[ki].advTrg[kcmid].mt.isLoogPress = false;
    }
    else if (press == KEY_STATUS_D2D)
    {
        if (kh.p_st[ki].advTrg[kcmid].mt.isLoogPress == true)
        {
            dispatch_keyevent(true, kc_hold.co);
            match = ADV_MATCH_SEND;
        }
    }
    kh.p_st[ki].advTrg[kcmid].mt.trg = press; //update mt trg


    return match;
}

adv_match_t adv_tgl(adv_t *padv, uint8_t ki, uint8_t press)
{
    adv_match_t match = ADV_MATCH_UNSEND;
    uint8_t kcmid = kh.p_st[ki].kcmidNow;
    kc_t kc_tgl   = padv->data.tgl.kc;
    key_tmp_sta_t *pkts = &gmx.pkts[ki];

    
    if (press == KEY_STATUS_D2U)
    {
        if (ABS(xTaskGetTickCount() - kh.p_st[ki].advTrg[kcmid].tgl.timestamp) >= padv->data.tgl.holdDuration)
        {
            kh.p_st[ki].advTrg[kcmid].tgl.selfLock = false;
            dispatch_keyevent(false, kc_tgl.co);
            match = ADV_MATCH_SEND;
        }
        else /* press*/
        {
            kh.p_st[ki].advTrg[kcmid].tgl.selfLock = (kh.p_st[ki].advTrg[kcmid].tgl.selfLock==true) ? false : true; /* self lock */
            if(kh.p_st[ki].advTrg[kcmid].tgl.selfLock == false)
            {
                dispatch_keyevent(false, kc_tgl.co);
                match = ADV_MATCH_SEND;
            }
        }
        kh.p_st[ki].advTrg[kcmid].tgl.timestamp = 0;
    }
    else if (press == KEY_STATUS_U2D)
    {
        kh.p_st[ki].advTrg[kcmid].tgl.timestamp = xTaskGetTickCount();
        if(kh.p_st[ki].advTrg[kcmid].tgl.selfLock != true)
        {
            dispatch_keyevent(true, kc_tgl.co);
            match = ADV_MATCH_SEND;
        }
        //else
        //{
        //    log_debug("[%02d]TGL DOWN()\r\n", ki);
        //}
    }
    
    kh.p_st[ki].advTrg[kcmid].tgl.trg = press; //update mt trg

    return ADV_MATCH_UNSEND;
}

adv_match_t adv_mtp(adv_t *padv, uint8_t ki, uint8_t press)
{
    uint8_t kcm = gbinfo.kc.cur_kcm;
    uint8_t idx = kh.p_st[ki].advTrg[kcm].mtp.kc_idx;
    kc_t *kc = &padv->data.mtp.kc[idx];

    if (press == KEY_STATUS_D2D)
    {
        dispatch_kc_keyevent(KEY_STATUS_U2D, kc, 1);
    }
    else if (press == KEY_STATUS_U2U)
    {
        dispatch_kc_keyevent(press, kc, 1);
    }
    
    return ADV_MATCH_UNSEND;
}

uint8_t adv_mtp_evt(adv_t *padv, uint8_t ki)
{
    uint8_t is_evt = false;
    key_st_t *pkst = &kh.p_st[ki];
    uint8_t kcm    = gbinfo.kc.cur_kcm;
    
    if (sk_rep_kb_get_change())
    {
        return false; // if kb report changed, skip mtp event to avoid conflict
    }

    uint64_t cur_timestap  = hpm_csr_get_core_cycle();
    uint64_t key_wait_50ms = clock_get_core_clock_ticks_per_ms() * 50;
    uint64_t limit_time    = pkst->first_d2u_time + key_wait_50ms;
    if (KEY_IS_BIG(ki))
    {
        limit_time += key_wait_50ms;
    }

    
    for (uint8_t i = 0; i < ADV_MTP_POINT_NUM; i++)
    {
        if ((pkst->advTrg[kcm].mtp.enter[i] == false) && (pkst->cur_point < pkst->advTrg[kcm].mtp.point[i]) && (cur_timestap > limit_time))
        {
            pkst->advTrg[kcm].mtp.kc_idx = i;
            pkst->advTrg[kcm].mtp.enter[i] = true;
            sk_judge_evt(ki, KEY_STATUS_D2D);
            is_evt = true;
            break;
        }
        else if ((pkst->advTrg[kcm].mtp.enter[i] == true) && (pkst->cur_point > pkst->advTrg[kcm].mtp.point[ADV_MTP_POINT_NUM+i]))
        {
            pkst->first_d2u_time = cur_timestap;
            pkst->advTrg[kcm].mtp.kc_idx = i;
            pkst->advTrg[kcm].mtp.enter[i] = false;
            sk_judge_evt(ki, KEY_STATUS_U2U);
            is_evt = true;
            break;
        }
    }
    return is_evt;
}

void adv_mtp_update_point(adv_t adv, uint8_t id)
{
    uint8_t kcmid = gbinfo.kc.cur_kcm;
    sk_la_lm_kc_info_t *pdb = &sk_la_lm_kc_info;

    for (uint8_t ki = 0; ki < BOARD_KEY_NUM; ki++)
    {
        if (id == pdb->kc_table[kcmid][ki].co)
        {
            for (uint8_t k = 0; k < ADV_MTP_POINT_NUM; k++)
            {
                uint16_t trg_dist = adv.data.mtp.trg_dist[k];
                float distTrigger = HIVE_RXDA_001MM(trg_dist); /* get trigger stroke */
        
                DBG_PRINTF("mtp","ki: %d, trg_dist: %d | ", ki, trg_dist);
        
                uint16_t rls_lvl, trg_lvl = DIST_TO_LEVEL(distTrigger, KEY_GENERAL_TRG_ACC(ki));
                if (KEY_SUPT_HACC(ki) && (distTrigger > KEY_NACC_TO_DIST(ki)))
                {
                    trg_lvl = DIST_TO_LEVEL(distTrigger - KEY_NACC_TO_DIST(ki), KEY_HACC(ki)) + KEY_NACC_TO_LVL(ki);
                }
                if (trg_lvl > KEY_MAX_LEVEL(ki))
                {
                    trg_lvl = KEY_MAX_LEVEL(ki);
                }
                trg_lvl -= 1;
                kh.p_st[ki].advTrg[kcmid].mtp.point[k] = KEY_LVL_GET_VAL(ki, trg_lvl);
                
                rls_lvl = trg_lvl - 1;
                kh.p_st[ki].advTrg[kcmid].mtp.point[ADV_MTP_POINT_NUM+k] = (trg_lvl == 0) ? kh.p_st[ki].topRaisePoint : KEY_LVL_GET_VAL(ki, rls_lvl);
            }
            DBG_PRINTF("mtp", "\r\n");
        }
    }
}

void adv_mtp_reload(void)
{
    uint8_t kcmid = gbinfo.kc.cur_kcm;
    sk_la_lm_kc_info_t *pdb = &sk_la_lm_kc_info;
    if ((pdb->sk.superkey_table[SKT_ADV].total_size > SK_ADV_NUM) || (pdb->sk.superkey_table[SKT_ADV].free_size > SK_ADV_NUM)|| (pdb->sk.superkey_table[SKT_ADV].storage_mask > 0xff))
    {
        memset(pdb->sk.ht, 0xff, sizeof(pdb->sk.ht));
        pdb->sk.superkey_table[SKT_ADV].total_size   = SK_ADV_NUM;
        pdb->sk.superkey_table[SKT_ADV].free_size    = SK_ADV_NUM;
        pdb->sk.superkey_table[SKT_ADV].storage_mask = 0;
    }
    
    for (uint8_t i = 0; i < pdb->sk.superkey_table[SKT_ADV].total_size; i++)
    {
        if (pdb->sk.superkey_table[SKT_ADV].storage_mask & (1 << i))
        {
            if (pdb->sk.adv_table[i].adv_ex == ADV_MTP)
            {
                adv_mtp_update_point(pdb->sk.adv_table[i], i);
            }
        }
    }
}

void adv_init(void)
{
    adv_kb_report = sk_get_kb_report();
    kb_report     = matrix_get_kb_report();
    ms_report     = matrix_get_ms_report();
}
