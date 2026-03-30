/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#include "sk.h"
#include "db.h"
#include "app_config.h"
#include "mg_matrix.h"
#include "interface.h"
#include "app_debug.h"
#define  DKS_DEFAULT_USE_NUM  2


#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "sk"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

static TaskHandle_t m_sk_thread; 
static sk_evt_t m_evt = SK_EVT_NONE;
static uint8_t key_idx;
static uint8_t key_sta;
static rep_hid_t sk_kb_report;
static bool update_kb = false;
static bool update_ms = false;
static void sk_set_default_by_type(sk_la_lm_kc_info_t *info, skt_t type);
void sk_rep_kb_change(void)
{
    update_kb = true;
}
bool sk_rep_kb_get_change(void)
{
    return update_kb;
}
bool sk_rep_kb_is_change(void)
{
    bool val = update_kb;
    update_kb  = false;
    return val;
}

void sk_rep_ms_change(void)
{
    update_ms = true;
}
bool sk_rep_ms_is_change(void)
{
    bool val = update_ms;
    update_ms  = false;
    return val;
}

rep_hid_t *sk_get_kb_report(void)
{
    return &sk_kb_report;
}

void sk_check_para(sk_la_lm_kc_info_t *info)
{
    if (info)
    {
        uint8_t sk_type_maximums[SKT_MAX] = {
            SK_DKS_NUM,
            SK_ADV_NUM,
            SK_MACRO_NUM,
            SK_SOCD_NUM,
            SK_HT_NUM,
            SK_RS_NUM,
        };

        for (skt_t i = SKT_DKS; i < SKT_MAX; i++)
        {
            if ((info->sk.superkey_table[i].free_size    > sk_type_maximums[i])    ||
                (info->sk.superkey_table[i].total_size   > sk_type_maximums[i])    ||
                (info->sk.superkey_table[i].total_size   < info->sk.superkey_table[i].free_size) || 
                (info->sk.superkey_table[i].total_size   == 0) || 
                (info->sk.superkey_table[i].storage_mask > (uint32_t)(0x1 << (sk_type_maximums[i] + 1))))
            {
                sk_set_default_by_type(info, i);
            }
        }
    }
}

static void sk_set_default_by_type(sk_la_lm_kc_info_t *info, skt_t type)
{
    uint8_t init_dks   = 0;
    uint8_t init_adv   = 0;
    uint8_t init_macro = 0;
    uint8_t init_socd  = 0;
    uint8_t init_ht    = 0;
    uint8_t init_rs    = 0;
    
    switch (type)
    {
        case SKT_DKS:
            init_dks   = 1;
        break;
            
        case SKT_ADV:
            init_adv   = 1;
        break;
        
        case SKT_MACRO:
            init_macro = 1;
        break;

        case SKT_SOCD:
            init_socd  = 1;
        break;
        
        case SKT_HT:
            init_ht    = 1;
        break;
        
        case SKT_RS:
            init_rs    = 1;
        break;
        
        case SKT_MAX:
            init_dks   = 1;
            init_adv   = 1;
            init_macro = 1;
            init_socd  = 1;
            init_ht    = 1;
            init_rs    = 1;
            memset(&info->sk, 0, sizeof(sk_t));
        break;

        default: break;
    }
    
    if (init_dks)
    {
        #if 0
        uint8_t keyCode[DKS_DEFAULT_USE_NUM][2]          = {{KC_A, KC_D}, {KC_D, KC_A}};
        char urgencyStopName[DKS_DEFAULT_USE_NUM][10]    = {"A键急停", "D键急停"};
        
        info->sk.superkey_table[SKT_DKS].total_size   = SK_DKS_NUM;
        info->sk.superkey_table[SKT_DKS].free_size    = SK_DKS_NUM - DKS_DEFAULT_USE_NUM;
        info->sk.superkey_table[SKT_DKS].storage_mask = 0x00;
                
        for (uint8_t id = 0; id < DKS_DEFAULT_USE_NUM; id++)
        {
            memcpy(info->sk.dks_table[id].name, urgencyStopName[id], sizeof(urgencyStopName[id]));

            info->sk.dks_table[id].para.toTopLevel = KEY_DFT_DKS_TO_TOP_TRG_LEVEL(KEY_DFT_SHAFT);
            info->sk.dks_table[id].para.toBotLevel = KEY_DFT_DKS_TO_BOT_TRG_LEVEL(KEY_DFT_SHAFT);

            info->sk.dks_table[id].para.tick.min    = 0;
            info->sk.dks_table[id].para.tick.max    = 0;
            info->sk.dks_table[id].para.tick.type   = TICK_STATIC;
            
            info->sk.dks_table[id].para.keyCode[0].ty      = KCT_KB;
            info->sk.dks_table[id].para.keyCode[0].sty     = 0x00;
            info->sk.dks_table[id].para.keyCode[0].co      = keyCode[id][0];
            info->sk.dks_table[id].para.keyEvent[0].buffer = 0x21; //event: 00 10 00 01
            
            info->sk.dks_table[id].para.keyCode[1].ty      = KCT_KB;
            info->sk.dks_table[id].para.keyCode[1].sty     = 0x00;
            info->sk.dks_table[id].para.keyCode[1].co      = keyCode[id][1];
            info->sk.dks_table[id].para.keyEvent[1].buffer = 0x30; //event: 00 11 00 00
            
            info->sk.superkey_table[SKT_DKS].storage_mask |= (0x01 << id);
        }
        #else
        info->sk.superkey_table[SKT_DKS].type         = SKT_DKS;
        info->sk.superkey_table[SKT_DKS].total_size   = SK_DKS_NUM;
        info->sk.superkey_table[SKT_DKS].free_size    = SK_DKS_NUM;
        info->sk.superkey_table[SKT_DKS].storage_mask = 0x00;
        #endif
    }
    
    if (init_adv)
    {
        info->sk.superkey_table[SKT_ADV].type           = SKT_ADV;
        info->sk.superkey_table[SKT_ADV].total_size     = SK_ADV_NUM;
        info->sk.superkey_table[SKT_ADV].free_size      = SK_ADV_NUM-1;
        info->sk.superkey_table[SKT_ADV].storage_mask   = 0x01;
    
        memcpy(info->sk.adv_table[0].name, "win lock", 8);
        info->sk.adv_table[0].adv_ex    = ADV_LOCK;
        info->sk.adv_table[0].adv_co    = SK_ADV0;
        info->sk.adv_table[0].data.kc[0].en  = KCE_ENABLE;
        info->sk.adv_table[0].data.kc[0].ty  = KCT_KB;
        info->sk.adv_table[0].data.kc[0].sty = 0;
        info->sk.adv_table[0].data.kc[0].co  = KC_LGUI;
    }
    
    if (init_macro)
    {
        info->sk.superkey_table[SKT_MACRO].type         = SKT_MACRO;
        info->sk.superkey_table[SKT_MACRO].total_size   = SK_MACRO_NUM;
        info->sk.superkey_table[SKT_MACRO].free_size    = SK_MACRO_NUM;
        info->sk.superkey_table[SKT_MACRO].storage_mask = 0;
    }

    if (init_socd)
    {
        memset(info->sk.socd, 0xff, sizeof(info->sk.socd));
        info->sk.superkey_table[SKT_SOCD].type         = SKT_SOCD;
        info->sk.superkey_table[SKT_SOCD].total_size   = SK_SOCD_NUM;
        info->sk.superkey_table[SKT_SOCD].free_size    = SK_SOCD_NUM;
        info->sk.superkey_table[SKT_SOCD].storage_mask = 0;
        matrix_socd_reload(info);
    }
    
    if (init_ht)
    {
        memset(info->sk.ht, 0xff, sizeof(info->sk.ht));
        info->sk.superkey_table[SKT_HT].type         = SKT_HT;
        info->sk.superkey_table[SKT_HT].total_size   = SK_HT_NUM;
        info->sk.superkey_table[SKT_HT].free_size    = SK_HT_NUM;
        info->sk.superkey_table[SKT_HT].storage_mask = 0;
        matrix_ht_reload(info);
    }
    
    if (init_rs)
    {
        memset(info->sk.rs, 0xff, sizeof(info->sk.rs));
        info->sk.superkey_table[SKT_RS].type         = SKT_RS;
        info->sk.superkey_table[SKT_RS].total_size   = SK_RS_NUM;
        info->sk.superkey_table[SKT_RS].free_size    = SK_RS_NUM;
        info->sk.superkey_table[SKT_RS].storage_mask = 0;
        matrix_rs_reload(info);
    }
    db_flash_region_mark_update(DB_SK_LA_LM_KC);
}

void sk_set_default(skt_t type)
{
    sk_set_default_by_type(&sk_la_lm_kc_info, type);
}

void sk_judge_evt(uint8_t ki, uint8_t state)
{
    kc_t *pkc = &gmx.pkc[gbinfo.kc.cur_kcm][ki];
    key_idx = ki;
    key_sta = state;

    switch (pkc->sty)
    {
        case SKT_MACRO:
        {
            m_evt = SK_EVT_MACRO;
        }break;

        case SKT_ADV:
        {
            m_evt = SK_EVT_ADV;
        }
        break;

        case SKT_DKS:
        {
            m_evt = SK_EVT_DKS;
        }break;
        
        default:
        {
            m_evt = SK_EVT_NONE;
        }break;
    }
}

void sk_task_notify(void)
{
    if (m_sk_thread && (m_evt > SK_EVT_NONE && m_evt < SK_EVT_MAX))
    {
        xTaskNotifyGive(m_sk_thread);
    }
}

static void sk_task(void * p_context)
{
    (void)(p_context);

    while (1)
    {
        (void) ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 

        switch (m_evt)
        {
            case SK_EVT_MACRO:
            {
                kc_t  *pkc  = &gmx.pkc[gbinfo.kc.cur_kcm][key_idx];
                m_trg_t trg = (key_sta == KEY_STATUS_U2D) ? M_TRG_U2D : M_TRG_D2U;

                if (pkc->co < SK_MACRO_MAX)
                {
                    macro_start(pkc->co, trg, 0xff);
                }
            }break;

            case SK_EVT_ADV:
            {
                kc_t *pkc = &gmx.pkc[gbinfo.kc.cur_kcm][key_idx];
                switch (sk_la_lm_kc_info.sk.adv_table[pkc->co].adv_ex)
                {
                    case ADV_REPLACE:
                    {
                        DBG_PRINTF("ADV_REPLACE=%d\n", key_sta);
                        if (key_sta == KEY_STATUS_U2D || key_sta == KEY_STATUS_U2U || key_sta == KEY_STATUS_D2U)
                        {
                            adv_replace_key(&sk_la_lm_kc_info.sk.adv_table[pkc->co], key_sta);
                        }
                    }break;

                    case ADV_LOCK:
                    {
                        DBG_PRINTF("ADV_LOCK\n");
                        if (key_sta == KEY_STATUS_U2D)
                        {
                            adv_en_disable_key(&sk_la_lm_kc_info.sk.adv_table[pkc->co]);
                        }
                    }
                    break;

                    // case ADV_ACTION:
                    // {
                    //     DBG_PRINTF("ADV_ACTION\n");
                    //     if (key_sta == KEY_STATUS_U2D)
                    //     {
                    //         ret = adv_action(&sk_la_lm_kc_info.sk.adv_table[pkc->co]);
                    //     }
                    // }break;

                    // case ADV_SCENE:
                    // {
                    //     DBG_PRINTF("ADV_SCENE\n");
                    //     if (key_sta == KEY_STATUS_U2D)
                    //     {
                    //         ret = adv_scene(&sk_la_lm_kc_info.sk.adv_table[pkc->co]);
                    //     }
                    // }break;

                    case ADV_MT:
                    {
                        adv_mt(&sk_la_lm_kc_info.sk.adv_table[pkc->co], key_idx, key_sta);
                    }
                    break;
                    case ADV_TGL:
                    {
                        adv_tgl(&sk_la_lm_kc_info.sk.adv_table[pkc->co], key_idx, key_sta);
                    }
                    break;
                    case ADV_MTP:
                    {
                        adv_mtp(&sk_la_lm_kc_info.sk.adv_table[pkc->co], key_idx, key_sta);
                    }
                    break;

                    default:
                        break;
                }
            }
            break;

            case SK_EVT_DKS:
            {
                // dks_dispatch_keyevent();
            }break;

            default: break;
        }
        m_evt = SK_EVT_NONE;
    }
}

void sk_init(void)
{
    macro_init();
    adv_init();

    if (pdPASS != xTaskCreate(sk_task, "sk", STACK_SIZE_SK, NULL, PRIORITY_SK, &m_sk_thread))
    {
        DBG_PRINTF("sk task creation failure\r\n");
    }
}

