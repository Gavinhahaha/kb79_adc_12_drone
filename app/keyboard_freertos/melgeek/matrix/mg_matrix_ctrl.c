/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include "FreeRTOS.h"
#include "task.h"
#include "mg_matrix.h"
#include "hal_timer.h"
#include "hal_hid.h"
#include "board.h"
#include "hpm_adc16_drv.h"
#include "mg_matrix_type.h"
#include "app_debug.h"
#include "mg_matrix.h"
#include "mg_trg_fix.h"
#include "mg_trg_rt.h"
#include "mg_kb.h"
#include "db.h"
#include "mg_hall.h"
#include "app_config.h"
#include "mg_hid.h"
#include "mg_cali.h"
#include <math.h>



#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "mg_matrix_ctrl"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#if defined(MODULE_LOG_ENABLED) && MODULE_LOG_ENABLED
#define log_debug printf

//static char tmpCh[256] = "\0";

#define LOG_SET_GEN     0
#define LOG_SET_RT      0
#define LOG_SET_DEAD    0
#define LOG_SET_DKS     0
#define LOG_SET_RPT     0
#define LOG_SET_SFT     0
#define LOG_SET_TUN     0
#define LOG_SET_ST      0
#define LOG_SET_HT      0
#define LOG_SET_RS      0

#define LOG_GET_KEY     0
#define LOG_GET_GEN     0
#define LOG_GET_RT      0
#define LOG_GET_DEAD    0
#define LOG_GET_DKS     0
#define LOG_GET_SFT     0
#define LOG_GET_WM      0
#define LOG_GET_CALI    0
#define LOG_GET_TUN     0

#else
#define log_debug(format, ...)
#endif


extern kh_t kh;
extern key_st_t key_st[];

#pragma pack(1)

typedef enum
{
    CMD_RESP_SUCCEED = false,
    CMD_RESP_FAIL = true,
} ModuleUartCmdRespResult_e;
/* ----- cmd cmd_get_key para ----- */
typedef enum 
{
    GET_KEY_PARA_ALL = 0,
    GET_KEY_PARA_SINGLE,
} GetKeyParaType_e;
typedef struct 
{
    GetKeyParaType_e type : 8;
    union {
        uint8_t buffer;
        struct {
            uint8_t pos:7;
            uint8_t way:1;
        }msg;
    }key;
} ModuleUartCmdGetKeyParaMsg_t;
/* ----- respond cmd_get_key para ----- */
typedef struct
{
    uint8_t remainKeyNumber;
    uint8_t keyNumber;
    union {
        uint8_t buffer[11];
        struct {
            uint8_t pos;
            key_sw_type_e sw;
            uint16_t keyRealTravel;
            uint8_t generalTriggerAcc;
            uint8_t rapidTriggerSensitiveMin;
            uint8_t rapidTriggerSensitiveMax;
            uint8_t rapidTriggerAcc;
            uint8_t dksTriggerAcc;
            uint8_t deadAcc;
            uint8_t deadMax;
        }msg;
    } key[KEYBOARD_NUMBER];
} ModuleUartCmdGetKeyParaRespMsg_t;
/* ----- cmd  set trigger travel ----- */
typedef struct
{
    uint8_t keyNumber;
    union
    {
        uint8_t buffer[5];
        struct
        {
            uint8_t pos;
            uint16_t trigger;
            uint16_t raise;
        } msg;
    } key[KEYBOARD_NUMBER];
} ModuleUartCmdSetTriggerStrokeMsg_t;
/* ----- cmd  get trigger travel ----- */
typedef struct 
{
    uint8_t startPos;
    uint8_t endPos;
} ModuleUartCmdGetTriggerStrokeMsg_t;
/* ----- respond get_trigger_travel ----- */
typedef struct 
{
    uint8_t keyNumber;
    union {
        uint8_t buffer[5];
        struct {
            uint8_t pos;
            uint16_t triggerLevel;
            uint16_t raiseLevel;
        } msg;
    } key[55/5];
}ModuleUartCmdGetTriggerStrokeRespMsg_t;
/* ----- cmd set rapid trigger ----- */
typedef struct 
{
    uint8_t keyNumber;
    uint8_t pressStroke;
    uint8_t raiseStroke;
    uint8_t rt_mode;
    uint8_t rt_ada_sens;
    union {
        uint8_t buffer[2];
        struct {
            uint8_t pos;
            uint8_t isEnable:1;
            uint8_t isContinue:1;
            uint8_t rt_ada_extreme:1;
            uint8_t rt_ada_mode:5;
        } msg;
    } key[KEYBOARD_NUMBER];
} ModuleUartCmdSetRapidTriggerMsg_t;
/* ----- cmd get rapid trigger ----- */
typedef enum 
{
    GET_RAPID_TRIGGER_PARA_ALL = 0,
    GET_RAPID_TRIGGER_PARA_SINGLE,
    GET_RAPID_TRIGGER_PARA_BE_OPEN,
} GetRapidTriggerParaType_e;
typedef struct 
{
    GetRapidTriggerParaType_e type : 8;
    union {
        uint8_t buffer;
        struct {
            uint8_t pos:7;
            uint8_t way:1;
        }msg;
    }key;
} ModuleUartCmdGetRapidTriggerMsg_t;
/* ----- respond get_rapid_trigger ----- */
typedef struct 
{
    uint8_t remainKeyNumber;
    uint8_t keyNumber;
    union {
        uint8_t buffer[6];
        struct {
            uint8_t pos;
            uint8_t pressSensitive;
            uint8_t raiseSensitive;
            uint8_t isEnable:1;
            uint8_t isContinue:1;
            uint8_t rt_ada_extreme:1;
            uint8_t rt_ada_mode:5;
            uint8_t rt_mode;
            uint8_t rt_ada_sens;
        } msg;
    } key[KEYBOARD_NUMBER];
}ModuleUartCmdGetRapidTriggerRespMsg_t;
/* ----- cmd set dead  ----- */
typedef struct 
{
    uint8_t keyNumber;
    struct
    {
        uint8_t pos;
        uint8_t topDead;
        uint8_t bottomDead;
    } key[KEYBOARD_NUMBER];
} ModuleUartCmdSetDeadBandMsg_t;
/* ----- cmd get dead  ----- */
typedef enum 
{
    GET_DEAD_BAND_PARA_ALL = 0,
    GET_DEAD_BAND_PARA_SINGLE,
} GetDeadBandParaType_e;
typedef struct 
{
    GetDeadBandParaType_e type : 8;
    union {
        uint8_t buffer;
        struct {
            uint8_t pos:7;
            uint8_t way:1;
        }msg;
    }key;
} ModuleUartCmdGetDeadBandMsg_t;
/* ----- respond get_rapid_trigger ----- */
typedef struct 
{
    uint8_t remainKeyNumber;
    uint8_t keyNumber;
    union {
        uint8_t buffer[3];
        struct {
            uint8_t pos;
            uint8_t topDead;
            uint8_t bottomDead;
        } msg;
    } key[KEYBOARD_NUMBER];
}ModuleUartCmdGetDeadBandRespMsg_t;

/* ----- cmd  key report ----- */
typedef union 
{
    key_rpt_e buffer;
} ModuleUartCmdSetKeyReportMsg_t, ModuleUartCmdGetKeyReportMsg_t;

/* ----- cmd  get key calib  ----- */
typedef union 
{
    cail_step_t buffer;
} ModuleUartCmdGetKeyCalibMsg_t;

/* ----- cmd set dks trigger ----- */
typedef enum  
{ 
    SET_DKS_PARA_OPERATE_DEL = 0x00, 
    SET_DKS_PARA_OPERATE_ADD, 
    SET_DKS_PARA_OPERATE_MOD, 
    SET_DKS_PARA_OPERATE_MAX, 
} SetDksOperate_e;
typedef struct 
{
    uint8_t id;
    SetDksOperate_e operate;
    uint8_t name[16];
    uint16_t toTopTrgLevel;
    uint16_t toBotTrgLevel;
    dks_tick_type tickType;
    uint16_t tickMin;
    uint16_t tickMax;
    dynamic_key_t keyCode[KEY_DFT_DKS_VIRTUAL_KEY_NUMBER];
    uint8_t keyEvent[KEY_DFT_DKS_VIRTUAL_KEY_NUMBER];
} ModuleUartCmdSetAdvTriggerDksMsg_t;
/* ----- cmd get dks trigger ----- */
typedef enum  
{ 
    GET_DKS_PARA_OPERATE_SINGLE = 0x00, 
    GET_DKS_PARA_OPERATE_ALL, 
    GET_DKS_PARA_TABLE_SIZE, 
    GET_DKS_PARA_OPERATE_BE_OPEN,
    GET_DKS_PARA_OPERATE_MAX, 
} GetDksOperate_e;
typedef struct 
{
    GetDksOperate_e operateType;
    union {
        uint8_t buffer;
        struct {
            uint8_t pos:7;
            uint8_t way:1;
        }msg;
    }key;
} ModuleUartCmdGetAdvTriggerDksMsg_t;
/* ----- cmd set dks bind ----- */
typedef struct 
{
    uint8_t keyCode;
    uint8_t dksId;
    uint8_t operate;
} ModuleUartCmdSetAdvTriggerDksBindMsg_t;
/* ----- respond cmd_get_dks_para ----- */
typedef struct 
{
    uint8_t remainKeyNumber;
    uint8_t transmitKeyNumber;
    union {
        // uint8_t buffer[36];
        struct {
            uint8_t id;
            uint8_t name[16];
            uint16_t toTopTrgLevel;
            uint16_t toBotTrgLevel;
            dks_tick_type tickType;
            uint16_t tickMin;
            uint16_t tickMax;
            dynamic_key_t keyCode[KEY_DFT_DKS_VIRTUAL_KEY_NUMBER];
            uint8_t keyEvent[KEY_DFT_DKS_VIRTUAL_KEY_NUMBER];
        } msg;
    } key[KEY_DFT_DKS_PARA_TABLE_MAX];
}ModuleUartCmdGetAdvTriggerDksRespMsg_t;

/* ----- respond cmd_set_dks_tick_para ----- */
typedef struct 
{
    uint8_t tickType;
    uint8_t tickMin;
    uint8_t tickMax;
}ModuleUartCmdSetAdvTriggerDksTickMsg_t,
    ModuleUartCmdGetAdvTriggerDksTickMsg_t;

/* ----- cmd set snap  ----- */
typedef enum
{
    SET_SNAP_OPERATE_DELETE = 0x00,
    SET_SNAP_OPERATE_ADD,
    SET_SNAP_OPERATE_EDIT,
    SET_SNAP_OPERATE_MAX,
} SetSnapShop_e;
typedef struct
{
    uint8_t id_bit;
    SetSnapShop_e SnapType;
    socd_t data;
} ModuleUartCmdSetSnapShop_Msg_t;
    
typedef struct
{
    uint8_t id_bit;
    SetSnapShop_e SnapType;
    ht_t data;
} ModuleUartCmdSetHyperTap_Msg_t;

typedef struct
{
    uint8_t id_bit;
    SetSnapShop_e SnapType;
    rs_t data;
} ModuleUartCmdSetRs_Msg_t;
/* ----- cmd set tuning  ----- */
    
typedef union 
{
    tuning_e tuning;
}tuning_en_t;

/* ----- cmd  set trigger travel ----- */
typedef struct
{
    uint8_t keyNumber;
    union {
        uint8_t buffer[2];
        struct {
            uint8_t pos;
            key_sw_type_e sw;
        } msg;
    } key[KEYBOARD_NUMBER];
} ModuleUartCmdSetsw_t;

typedef struct 
{
    key_sw_type_e type;
} ModuleUartCmdGetsw_t;
typedef struct 
{
    uint16_t wave;
} ModuleUartCmdGetWaveMax_t;

typedef struct 
{
    uint8_t keyNumber;
    union {
        uint8_t buffer[2];
        struct {
            uint8_t pos;
            uint8_t mode;
        } msg;
    } key[KEYBOARD_NUMBER];
} ModuleUartCmdSetRTModeMsg_t;

typedef struct 
{
    uint8_t remainKeyNumber;
    uint8_t keyNumber;
    union {
        uint8_t buffer[2];
        struct {
            uint8_t pos;
            uint8_t mode;
        } msg;
    } key[KEYBOARD_NUMBER];
}ModuleUartCmdGetRTModeRespMsg_t;

typedef struct 
{
    uint8_t keyNumber;
    union {
        uint8_t buffer[3];
        struct {
            uint8_t pos;
            uint8_t mode;
            uint8_t sensitive;
        } msg;
    } key[KEYBOARD_NUMBER];
} ModuleUartCmdSetRTAdaptiveMsg_t;

typedef struct 
{
    uint8_t remainKeyNumber;
    uint8_t keyNumber;
    union {
        uint8_t buffer[3];
        struct {
            uint8_t pos;
            uint8_t mode;
            uint8_t sensitive;
        } msg;
    } key[KEYBOARD_NUMBER];
}ModuleUartCmdSetRTAdaptiveRespMsg_t;


#pragma pack()



void kh_thread_resume_isr(void);
extern void key_clear_status(uint8_t ki);
extern bool key_pos_is_top(uint8_t ki, uint16_t new_point);
extern void key_shaft_change_acc_conv(uint8_t ki, key_sw_type_e newsw);


void cmd_show_playload(const char *funcName, const char *name, uint8_t *ptrBuffer, uint16_t lenght)
{
    #if defined(MODULE_LOG_ENABLED) && MODULE_LOG_ENABLED
    log_debug("\n%s\n", funcName);
    log_debug("    - %s: \n", name);    
    if (lenght == 0)
    {
        log_debug("    - playlod(%d): \n", lenght);
        return;
    }
    log_debug("    - playlod(%d): ", lenght);
    for (uint16_t i = 0; i < lenght; i++)
    {
        // if ((i % 8) == 0 && i != 0)
        {
        //    // log_debug(" | ");
        }
        log_debug("%02x ", ptrBuffer[i]);
    }
    log_debug("\n");
    #endif
}

uint16_t ks_cmd_get_key_para(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    ModuleUartCmdGetKeyParaMsg_t *msg         = (ModuleUartCmdGetKeyParaMsg_t *)reveBuffer; /* get cmd msg */
    ModuleUartCmdGetKeyParaRespMsg_t *respMsg = (ModuleUartCmdGetKeyParaRespMsg_t *)respBuffer;
    #if defined(LOG_GET_KEY) && (LOG_GET_KEY == 1)      
    cmd_show_playload(__func__, "receive", (uint8_t *)msg, reveBufferSize);
    #endif

    uint8_t respResult = 0xFF, startKeyIndex = 0, EndKeyIndex = 0, sendKeyNum = 0, remainSendKeyNum = 0;
    uint8_t transmitKeyMaxNum = ((respBufferSize - 2) / ARRAY_SIZE(respMsg->key[0].buffer)); /* - playload head  */


    if (reveBufferSize < sizeof(ModuleUartCmdGetKeyParaMsg_t))
    {
        respResult = 0xFF;
        #if defined(LOG_GET_KEY) && (LOG_GET_KEY == 1)      
        log_debug("    - error 0x%02x\n", respResult);
        #endif
    }
    else
    {
        if (msg->type == GET_KEY_PARA_SINGLE)
        {
            sendKeyNum       = 1;
            remainSendKeyNum = 0;
            startKeyIndex    = msg->key.msg.pos;
            EndKeyIndex      = msg->key.msg.pos;
            respResult       = CMD_RESP_SUCCEED;
            #if defined(LOG_GET_KEY) && (LOG_GET_KEY == 1)      
            sprintf(tmpCh, "    - get key para (%d) \n", msg->key.msg.pos);
            #endif
        }
        else if (msg->type == GET_KEY_PARA_ALL)
        {
            #if defined(LOG_GET_KEY) && (LOG_GET_KEY == 1)      
            log_debug("    - get all key para(%d) ", KEYBOARD_NUMBER);
            #endif
            if (msg->key.msg.way == false) //reset start
            {
                kh.p_gst->keyParaRespLen = KEYBOARD_NUMBER;
            }
            if (kh.p_gst->keyParaRespLen >= transmitKeyMaxNum)
            {
                sendKeyNum                  = transmitKeyMaxNum;
                remainSendKeyNum            = kh.p_gst->keyParaRespLen - transmitKeyMaxNum;
                startKeyIndex               = KEYBOARD_NUMBER - kh.p_gst->keyParaRespLen;
                EndKeyIndex                 = startKeyIndex + sendKeyNum - 1;
                kh.p_gst->keyParaRespLen = remainSendKeyNum; //update
                #if defined(LOG_GET_KEY) && (LOG_GET_KEY == 1)      
                sprintf(tmpCh, "send(%d) reamin(%d) (%02d-%02d) \n", sendKeyNum, remainSendKeyNum, startKeyIndex, EndKeyIndex);
                #endif
            }
            else
            {
                sendKeyNum                  = kh.p_gst->keyParaRespLen;
                remainSendKeyNum            = 0;
                startKeyIndex               = KEYBOARD_NUMBER - kh.p_gst->keyParaRespLen;
                EndKeyIndex                 = startKeyIndex + sendKeyNum - 1;
                kh.p_gst->deadKeyRespLen = KEYBOARD_NUMBER; //update
                #if defined(LOG_GET_KEY) && (LOG_GET_KEY == 1)      
                sprintf(tmpCh, "send(%d) reamin(%d) end (%02d-%02d) \n", sendKeyNum, remainSendKeyNum, startKeyIndex, EndKeyIndex);
                #endif
            }
            respResult = CMD_RESP_SUCCEED;
        }
        else
        {
            respResult = 0xFF;
            #if defined(LOG_GET_KEY) && (LOG_GET_KEY == 1)      
            sprintf(tmpCh, "    - error 0x%02x\n", respResult);
            #endif
        }
        #if defined(LOG_GET_KEY) && (LOG_GET_KEY == 1)      
        log_debug("%s", (tmpCh) );
        #endif
    }
    
    if (respResult == 0xFF)
    {
        respBuffer[*respLenght] = respResult;
        *respLenght            += 1;
        #if defined(LOG_GET_KEY) && (LOG_GET_KEY == 1)      
        cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
        #endif
        return respResult;
    }

    /* ------------------------------ resp msg ------------------------------ */
    uint8_t j                 = 0;
    uint8_t respRtKeyStartPos = startKeyIndex;
    uint8_t respRtKeyEndPos   = EndKeyIndex;
    respMsg->remainKeyNumber  = remainSendKeyNum;
    respMsg->keyNumber        = sendKeyNum;
    uint16_t respMsgLenght    = 2;
    for (uint8_t ki = respRtKeyStartPos; ki <= respRtKeyEndPos; ki++)
    {
        uint8_t keysw = key_cali.sw[ki];
        /* fill playload */
        respMsg->key[j].msg.pos                      = ki;
        respMsg->key[j].msg.sw                       = (key_sw_type_e)keysw;
        respMsg->key[j].msg.keyRealTravel            = HIVE_TXDA_001MM(KEY_REAL_TRAVEL(ki));
        respMsg->key[j].msg.generalTriggerAcc        = HIVE_TXDA_001MM(KEY_GENERAL_TRG_ACC(ki));
        respMsg->key[j].msg.rapidTriggerAcc          = HIVE_TXDA_001MM(KEY_RT_ACC(ki));
        respMsg->key[j].msg.dksTriggerAcc            = HIVE_TXDA_001MM(KEY_DKS_TRG_ACC(ki));
        respMsg->key[j].msg.deadAcc                  = HIVE_TXDA_001MM(KEY_DEADBAND_ACC(ki));
        respMsg->key[j].msg.rapidTriggerSensitiveMin = HIVE_TXDA_001MM(KEY_RT_SENS_MIN(ki));
        respMsg->key[j].msg.rapidTriggerSensitiveMax = HIVE_TXDA_001MM(KEY_RT_SENS_MAX(ki));
        respMsg->key[j].msg.deadMax                  = HIVE_TXDA_001MM(KEY_DEADBAND_MAX(ki));

        respMsgLenght                               += ARRAY_SIZE(respMsg->key[0].buffer);
        j++;
    }

    *respLenght = respMsgLenght;
    #if defined(LOG_GET_KEY) && (LOG_GET_KEY == 1)      
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif

    return respResult;
}

uint16_t ks_cmd_set_general_trigger_stroke(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    (void)respBufferSize;
    ModuleUartCmdSetTriggerStrokeMsg_t *msg = (ModuleUartCmdSetTriggerStrokeMsg_t *)reveBuffer; /* get cmd msg */
    #if defined(LOG_SET_GEN) && (LOG_SET_GEN == 1)      
    cmd_show_playload(__func__, "receive", (uint8_t *)msg, reveBufferSize);
    #endif    
    uint8_t respResult = CMD_RESP_FAIL;
    if ((reveBufferSize < 6 && msg->keyNumber != 0) || (reveBufferSize != 6 && msg->keyNumber == 0xFF))
    {
        respResult = CMD_RESP_FAIL;
        
        respBuffer[*respLenght] = respResult;
        *respLenght            += 1;
        #if defined(LOG_SET_GEN) && (LOG_SET_GEN == 1)          
        log_debug("    - error 0x%02x\n", respResult);
        cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
        #endif
        // ret = cmd_respond_push_msg(instDevKeyscan->moduleUart.framePackReceive.cmd, instDevKeyscan->moduleUart.framePackReceive.subCmd, respResult, 0, NULL);
        return respResult;
    }


    switch (msg->keyNumber)
    {
        case 0x00: /*  Reset all to default values */
        {
            #if defined(LOG_SET_GEN) && (LOG_SET_GEN == 1)          
            log_debug("    - Reset all to default level values \n");
            #endif
            // dev_flash_read_user_msg_all();
           // db_get_binfo();
            //feature_general_trigger_default_para();
            fix_set_default_ap_para();
            db_update(DB_SYS, UPDATE_LATER);
            // respResult = dev_flash_write_user_msg_all()==true ? CMD_RESP_SUCCEED : CMD_RESP_FAIL;
            respResult = CMD_RESP_SUCCEED;
        } break;
        case 0xFF: /* set all key */
        {
            key_general_mode_e mode;

            respResult = CMD_RESP_SUCCEED;
            for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
            {
                uint8_t keysw = key_cali.sw[ki];
                uint16_t tmpTrigger  = msg->key[0].msg.trigger;
                uint16_t tmpRaise    = msg->key[0].msg.raise;
                float distTrigger    = HIVE_RXDA_001MM(tmpTrigger); /* get trigger stroke */
                float distRaise      = HIVE_RXDA_001MM(tmpRaise);

                #if defined(LOG_SET_GEN) && (LOG_SET_GEN == 1)  
                sprintf(tmpCh, "trg:%0.2fmm, raise:%0.2fmm", distTrigger, distRaise);
                log_debug("    - dist: %s\n", tmpCh);
                #endif                
                if (   tmpTrigger < HIVE_TXDA_001MM(KEY_GENERAL_TRG_MIN(ki)) || tmpTrigger > HIVE_TXDA_001MM(KEY_GENERAL_TRG_MAX(ki))
                    || tmpRaise   < HIVE_TXDA_001MM(KEY_GENERAL_TRG_MIN(ki)) || tmpRaise   > HIVE_TXDA_001MM(KEY_GENERAL_TRG_MAX(ki)))
                {
                    respResult = CMD_RESP_FAIL + 1;

                    #if defined(LOG_SET_GEN) && (LOG_SET_GEN == 1)  
                    if (ki == 0 || ki == KEYBOARD_NUMBER-1)
                    {
                        log_debug("    - error 0x%02x, [%d] trigger:%02d, raise:%02d \n", respResult, ki, tmpTrigger, tmpRaise);
                    }
                    else
                    {
                        log_debug("    - error 0x%02x, [%d]\n", respResult, ki);
                    }
                    #endif                
                    continue;
                }
                
                /* get trigger level */
                tmpTrigger = DIST_TO_LEVEL(distTrigger, KEY_GENERAL_TRG_ACC(ki));
                tmpRaise   = DIST_TO_LEVEL(distRaise,   KEY_GENERAL_TRG_ACC(ki));
                if (KEY_SUPT_HACC(ki) && distTrigger > KEY_NACC_TO_DIST(ki))
                {
                    tmpTrigger = DIST_TO_LEVEL(distTrigger - KEY_NACC_TO_DIST(ki), KEY_HACC(ki)) + KEY_NACC_TO_LVL(ki);
                }
                if (KEY_SUPT_HACC(ki) && distRaise > KEY_NACC_TO_DIST(ki))
                {
                    tmpRaise = DIST_TO_LEVEL(distRaise - KEY_NACC_TO_DIST(ki), KEY_HACC(ki)) + KEY_NACC_TO_LVL(ki);
                }
                
                #if defined(LOG_SET_GEN) && (LOG_SET_GEN == 1)  
                if (ki == 0 || ki == KEYBOARD_NUMBER-1)
                {
                    log_debug("    - Set all to level values - trigger:%02d, raise:%02d \n", tmpTrigger, tmpRaise);
                }
                else if (ki == 1)
                {
                    log_debug("    - key ...\n");
                }
                #endif
                if (tmpTrigger == tmpRaise)
                {
                    mode     = KEY_GENRAL_MODE_STROKE_SAME;
                    tmpRaise = (tmpTrigger + KEY_DFT_GENERAL_TRG_RAISE_OFFSET_LEVEL(keysw)) < 0 ? 0 : (tmpTrigger + KEY_DFT_GENERAL_TRG_RAISE_OFFSET_LEVEL(keysw));
                }
                else if (tmpTrigger > tmpRaise)
                {
                    mode = KEY_GENRAL_MODE_STROKE_LAG;
                }
                else //(tmpTrigger<tmpRaise)
                {
                    mode = KEY_GENRAL_MODE_STROKE_ADVANCE;
                }
                /* para same */
                if (kh.p_attr[ki].triggerlevel == tmpTrigger && kh.p_attr[ki].raiseLevel == tmpRaise)
                {
                //    log_debug("    - error 03 (value same): key number(%d): pos:%02d, trigger:%02d, raise:%02d\n", msg->keyNumber, ki, tmpTrigger, tmpRaise);
                    continue;
                }
                fix_set_para(ki, tmpTrigger, tmpRaise, mode);
            }
            db_update(DB_SYS, UPDATE_LATER);
        } break;
        default:
        // case 0x01 ... KEYBOARD_NUMBER:
        {
            if (!(msg->keyNumber >= 0x01 && msg->keyNumber <= KEYBOARD_NUMBER))
            {
                respResult = CMD_RESP_FAIL + 4;
                #if defined(LOG_SET_GEN) && (LOG_SET_GEN == 1)  
                log_debug("    - error 0x%02x\n", respResult);
                #endif
                break;
            }

            // uint8_t isFail = false;
            // dev_flash_read_user_msg_all();
            //db_get_binfo();
            key_general_mode_e mode;
            for (uint8_t i = 0; i < msg->keyNumber; i++)
            {
                uint8_t ki           = msg->key[i].msg.pos;
                uint16_t tmpTrigger  = msg->key[i].msg.trigger;
                uint16_t tmpRaise    = msg->key[i].msg.raise;
                uint8_t keysw        = key_cali.sw[ki];
                float distTrigger    = HIVE_RXDA_001MM(tmpTrigger); /* get trigger stroke */
                float distRaise      = HIVE_RXDA_001MM(tmpRaise);
                
                #if defined(LOG_SET_GEN) && (LOG_SET_GEN == 1)  
                sprintf(tmpCh, "trg:%0.2fmm, raise:%0.2fmm", distTrigger, distRaise);
                log_debug("    - dist: %s\n", tmpCh);
                #endif
                if (   tmpTrigger < HIVE_TXDA_001MM(KEY_GENERAL_TRG_MIN(ki)) || tmpTrigger > HIVE_TXDA_001MM(KEY_GENERAL_TRG_MAX(ki))
                    || tmpRaise   < HIVE_TXDA_001MM(KEY_GENERAL_TRG_MIN(ki)) || tmpRaise   > HIVE_TXDA_001MM(KEY_GENERAL_TRG_MAX(ki)))
                {
                    // isFail = true;
                    respResult = CMD_RESP_FAIL + 1;
                    #if defined(LOG_SET_GEN) && (LOG_SET_GEN == 1)  
                    log_debug("    - error 0x%02x, [%d] trigger:%02d, raise:%02d \n", respResult, ki, tmpTrigger, tmpRaise);
                    #endif
                    continue;
                }
                
                /* get trigger level */
                tmpTrigger = DIST_TO_LEVEL(distTrigger, KEY_GENERAL_TRG_ACC(ki));
                tmpRaise   = DIST_TO_LEVEL(distRaise,   KEY_GENERAL_TRG_ACC(ki));
                if (KEY_SUPT_HACC(ki) && distTrigger > KEY_NACC_TO_DIST(ki))
                {
                   tmpTrigger = DIST_TO_LEVEL(distTrigger - KEY_NACC_TO_DIST(ki), KEY_HACC(ki)) + KEY_NACC_TO_LVL(ki);
                }
                if (KEY_SUPT_HACC(ki) && distRaise > KEY_NACC_TO_DIST(ki))
                {
                   tmpRaise = DIST_TO_LEVEL(distRaise - KEY_NACC_TO_DIST(ki), KEY_HACC(ki)) + KEY_NACC_TO_LVL(ki);
                }
                        
                if (tmpTrigger == tmpRaise)
                {
                    mode     = KEY_GENRAL_MODE_STROKE_SAME;
                    tmpRaise = (tmpTrigger + KEY_DFT_GENERAL_TRG_RAISE_OFFSET_LEVEL(keysw)) < 0 ? 0 : (tmpTrigger + KEY_DFT_GENERAL_TRG_RAISE_OFFSET_LEVEL(keysw));
                }
                else if (tmpTrigger > tmpRaise)
                {
                    mode = KEY_GENRAL_MODE_STROKE_LAG;
                }
                else //(tmpTrigger<tmpRaise)
                {
                    mode = KEY_GENRAL_MODE_STROKE_ADVANCE;
                }
                /* para same */
                if (kh.p_attr[ki].triggerlevel == tmpTrigger && kh.p_attr[ki].raiseLevel == tmpRaise)
                {
                    //log_debug("    - error 03 (value same): key number(%d/%d): pos:%02d, trigger:%02d, raise:%02d\n",  msg->keyNumber, i, msg->key[i].msg.pos, tmpTrigger, tmpRaise);
                    continue;
                }
                
                #if defined(LOG_SET_GEN) && (LOG_SET_GEN == 1)  
                log_debug("    - key number(%d/%d): pos:%02d, trigger:%02d, raise:%02d\n", \
                                        msg->keyNumber, i, msg->key[i].msg.pos, tmpTrigger, tmpRaise);
                #endif

                fix_set_para(ki, tmpTrigger, tmpRaise, mode);
            }

            db_update(DB_SYS, UPDATE_LATER);
            // respResult = (isFail==true) ? CMD_RESP_FAIL : CMD_RESP_SUCCEED;
            // respResult = dev_flash_write_user_msg_all()==true ? CMD_RESP_SUCCEED : CMD_RESP_FAIL;
            respResult = CMD_RESP_SUCCEED;
        }break;
    }


    respBuffer[*respLenght] = respResult;
    *respLenght            += 1;
    #if defined(LOG_SET_GEN) && (LOG_SET_GEN == 1)  
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
    // ret = cmd_respond_push_msg(instDevKeyscan->moduleUart.framePackReceive.cmd, instDevKeyscan->moduleUart.framePackReceive.subCmd, respResult, 0, NULL);
    return respResult;
}

uint16_t ks_cmd_get_general_trigger_stroke(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint8_t respResult = CMD_RESP_FAIL;
    ModuleUartCmdGetTriggerStrokeMsg_t *msg         = (ModuleUartCmdGetTriggerStrokeMsg_t *)reveBuffer; /* get cmd msg */
    ModuleUartCmdGetTriggerStrokeRespMsg_t *respMsg = (ModuleUartCmdGetTriggerStrokeRespMsg_t *)respBuffer; /* get cmd resp msg */
    respMsg->keyNumber                              = ABS2(msg->endPos , msg->startPos) + 1;
    uint8_t lenght                                  = respMsg->keyNumber;
    uint8_t tmpCount                                = 0;
    uint8_t startKeyPos                             = 0;
    uint16_t respMsgLenght                          = (respMsg->keyNumber * sizeof(respMsg->key[0])) + sizeof(respMsg->keyNumber);
    (void)respBufferSize;
    (void)reveBufferSize;

    #if defined(LOG_SET_GEN) && (LOG_GET_GEN == 1)  
    cmd_show_playload(__func__, "receive", (uint8_t *)msg, reveBufferSize);
    #endif
    if (msg->startPos >= KEYBOARD_NUMBER || msg->endPos >= KEYBOARD_NUMBER || respMsg->keyNumber > ARRAY_SIZE(respMsg->key))
    {
        respResult = CMD_RESP_FAIL;
        
        respBuffer[*respLenght] = respResult;
        *respLenght            += 1;
        
        #if defined(LOG_SET_GEN) && (LOG_GET_GEN == 1)  
        log_debug("    - error 0x%02x\n", respResult);
        cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
        #endif        
        return respResult;
    }

    if(msg->endPos > msg->startPos)
    {
        startKeyPos = msg->startPos;
    }
    else if(msg->endPos < msg->startPos)
    {
        startKeyPos = msg->endPos;
    }
    else
    {
        startKeyPos = msg->startPos;
    }

    do 
    {
        uint8_t ki               = startKeyPos;
        uint16_t tmpTriggerLevel = kh.p_attr[ki].triggerlevel;
        uint16_t tmpRaiseLevel   = kh.p_attr[ki].raiseLevel;
        float distTrigger        = LEVEL_TO_DIST(tmpTriggerLevel, KEY_GENERAL_TRG_ACC(ki));
        float distRaise          = LEVEL_TO_DIST(tmpRaiseLevel,   KEY_GENERAL_TRG_ACC(ki));
        
        if (KEY_SUPT_HACC(ki) && tmpTriggerLevel > KEY_NACC_TO_LVL(ki))
        {
            distTrigger = LEVEL_TO_DIST((tmpTriggerLevel - KEY_NACC_TO_LVL(ki)), KEY_HACC(ki)) + KEY_NACC_TO_DIST(ki);
            float tMod  = fmod(distTrigger, KEY_NACC(ki));
            distTrigger = tMod < KEY_HACC(ki) ? distTrigger : (distTrigger + (KEY_NACC(ki) - tMod));
        }
        if (KEY_SUPT_HACC(ki) && tmpRaiseLevel > KEY_NACC_TO_LVL(ki))
        {
            distRaise  = LEVEL_TO_DIST((tmpRaiseLevel - KEY_NACC_TO_LVL(ki)), KEY_HACC(ki)) + KEY_NACC_TO_DIST(ki);
            float rMod = fmod(distRaise, KEY_NACC(ki));
            distRaise  = rMod < KEY_HACC(ki) ? distRaise : (distRaise + (KEY_NACC(ki) - rMod));
        }

        respMsg->key[tmpCount].msg.pos          = ki;
        respMsg->key[tmpCount].msg.triggerLevel = HIVE_TXDA_001MM(distTrigger);
        
        switch(kh.p_attr[ki].generalMode)
        {
            case KEY_GENRAL_MODE_STROKE_SAME:
            {
                respMsg->key[tmpCount].msg.raiseLevel = HIVE_TXDA_001MM(distRaise);
                // respMsg->key[tmpCount].msg.raiseLevel = (kh.p_attr[ki].raiseLevel - KEY_DFT_GENERAL_TRG_RAISE_OFFSET_LEVEL) * KEY_GENERAL_TRG_ACC(ki); 
            } break;
            case KEY_GENRAL_MODE_STROKE_LAG:
            case KEY_GENRAL_MODE_STROKE_ADVANCE:
            {
                respMsg->key[tmpCount].msg.raiseLevel = HIVE_TXDA_001MM(distRaise);
            } break;
            default: break;
        }
        #if defined(LOG_SET_GEN) && (LOG_GET_GEN == 1)  
        if (lenght == respMsg->keyNumber || lenght == 1)
        {
            log_debug("    - key[%02d](%02d), trgLev:%02d, rasLev:%02d, trgPin:%02d, rasPin:%02d\n", ki, respMsg->keyNumber, \
                                                                                            respMsg->key[tmpCount].msg.triggerLevel, \
                                                                                            respMsg->key[tmpCount].msg.raiseLevel, \
                                                                                            kh.p_attr[ki].triggerpoint, \
                                                                                            kh.p_attr[ki].raisePoint);
        }
        else if (lenght == respMsg->keyNumber-1)
        {
            log_debug("    - key ...\n");
        }
        #endif        
        tmpCount += 1;

        startKeyPos += 1;
        lenght -= 1;
    } while(lenght!=false);
    
    *respLenght = respMsgLenght;
    #if defined(LOG_SET_GEN) && (LOG_GET_GEN == 1)  
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
    return respResult;
}
uint16_t ks_cmd_set_rapid_trigger(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    ModuleUartCmdSetRapidTriggerMsg_t *msg = (ModuleUartCmdSetRapidTriggerMsg_t *)reveBuffer; /* get cmd msg */
    #if defined(LOG_SET_RT) && (LOG_SET_RT == 1)  
    cmd_show_playload(__func__, "receive", (uint8_t *)msg, reveBufferSize);
    #endif
    uint8_t respResult   = CMD_RESP_FAIL;
    (void)respBufferSize;
    //uint8_t keysw = key_cali.sw[0];
    //#warning "debug shaft"

    switch(msg->keyNumber)
    {
        case 0x00: /* reset default */
        {
            #if defined(LOG_SET_RT) && (LOG_SET_RT == 1)  
            log_debug("    - Reset all rt \n");
            #endif            
            //dev_flash_read_user_msg_all();
           // db_get_binfo();
            //feature_rapid_trigger_default_para();
            rt_set_default_para();
            db_update(DB_SYS, UPDATE_LATER);
            respResult = CMD_RESP_SUCCEED;
            //respResult = dev_flash_write_user_msg_all()==true ? CMD_RESP_SUCCEED : CMD_RESP_FAIL;
        } break;
        case 0xFF: /* set all */
        {
            respResult = CMD_RESP_SUCCEED;
            if (reveBufferSize != 6)
            {
                respResult = CMD_RESP_FAIL+0;
                #if defined(LOG_SET_RT) && (LOG_SET_RT == 1)  
                log_debug("    - error 0x%02x\n", respResult);
                #endif
                break;
            }
            
            kh.p_gst->rtKeyUsedTableNumber = 0;
            for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
            {
                uint8_t tmpIsEnable      = msg->key[0].msg.isEnable;
                uint8_t tmpIsContinue    = msg->key[0].msg.isContinue;
                uint8_t rt_ada_mode      = msg->key[0].msg.rt_ada_mode;
                uint8_t rt_ada_extreme   = msg->key[0].msg.rt_ada_extreme;
                uint8_t pressSensitive   = msg->pressStroke;
                uint8_t raiseSensitive   = msg->raiseStroke;
                uint8_t rt_mode          = msg->rt_mode;
                uint8_t rt_ada_sens      = msg->rt_ada_sens;
                float distPressSensitive = HIVE_RXDA_001MM(pressSensitive); /* get trigger sensitive dist */
                float distRaiseSensitive = HIVE_RXDA_001MM(raiseSensitive);
                
                #if defined(LOG_SET_RT) && (LOG_SET_RT == 1)  
                sprintf(tmpCh, "    - dist: preS:%0.2fmm, raiS:%0.2fmm\n", distPressSensitive, distRaiseSensitive);
                log_debug("%s", tmpCh);
                #endif
                if(    pressSensitive < HIVE_TXDA_001MM(KEY_RT_SENS_MIN(ki)) || pressSensitive > HIVE_TXDA_001MM(KEY_RT_SENS_MAX(ki))
                    || raiseSensitive < HIVE_TXDA_001MM(KEY_RT_SENS_MIN(ki)) || raiseSensitive > HIVE_TXDA_001MM(KEY_RT_SENS_MAX(ki)) )
                {
                    respResult = CMD_RESP_FAIL+1;
                    #if defined(LOG_SET_RT) && (LOG_SET_RT == 1)  
                    log_debug("    - error 0x%02x, preS:%d, raiS:%d\n", respResult, pressSensitive, raiseSensitive);
                    #endif
                }
                else
                {
                    pressSensitive = DIST_TO_LEVEL(distPressSensitive, KEY_RT_ACC(ki)); /* get trigger sensitive */
                    raiseSensitive = DIST_TO_LEVEL(distRaiseSensitive, KEY_RT_ACC(ki));
                    #if defined(LOG_SET_RT) && (LOG_SET_RT == 1)  
                    if (ki == 0 || ki == KEYBOARD_NUMBER-1 || ki == 33)
                    {
                        log_debug("    - Set all rt - Enable:%s, Continue:%s, press_sensitive:%d, raise_sensitive:%d\n",  \
                                                        tmpIsEnable==true ? "true" : "false", tmpIsContinue==true ? "true" : "false", \
                                                        pressSensitive, raiseSensitive);
                    }
                    else if (ki == 1)
                    {
                        log_debug("    - Set all rt ...\n");
                    }
                    #endif
                    
                    key_attr_t *p_attr  = (kh.p_attr + ki);
                    p_attr->rt_mode     = rt_mode >= RT_MODE_MAX ? KEY_DFT_RT_MODE : rt_mode;
                    p_attr->rt_ada_mode = rt_ada_mode >= RT_ADA_MODE_MAX ? KEY_DFT_RT_ADAPTIVE_MODE : rt_ada_mode;
                    p_attr->rt_ada_sens = rt_ada_sens >= RT_ADA_SENS_MAX ? KEY_DFT_RT_ADAPTIVE_SENS : rt_ada_sens;
                    p_attr->rt_ada_extreme = rt_ada_extreme;
                    
                    rt_set_para(ki, tmpIsEnable, tmpIsContinue, pressSensitive, raiseSensitive);
                }
                
                //if (sw_config_table[kh.p_calib->sw[ki]].isRtHighAcc)
                //{
                    //int8_t tmpPress = ACC_BETW_CONV(kh.p_attr[ki].rtPressSensitive, KEY_HACC(ki), KEY_NACC(ki));
                    //int8_t tmpRaise = ACC_BETW_CONV(kh.p_attr[ki].rtRaiseSensitive, KEY_HACC(ki), KEY_NACC(ki));
                    //kh.p_st[ki].rtPressLowAccSens = tmpPress <= 0 ? 1 : tmpPress;
                    //kh.p_st[ki].rtRaiseLowAccSens = tmpRaise <= 0 ? 1 : tmpRaise;
                //}
                
                if ( kh.p_attr[ki].rt_en == true && kh.p_attr[ki].advTrg[0].advType != KEY_ACT_DKS)
                {
                    kh.p_gst->rtKeyUsedTable[kh.p_gst->rtKeyUsedTableNumber++] = ki;
                }
            }
            db_update(DB_SYS, UPDATE_LATER);
            //respResult = dev_flash_write_user_msg_all()==true ? CMD_RESP_SUCCEED : CMD_RESP_FAIL;
        } break;
        default: 
        //case 0x01 ... KEYBOARD_NUMBER:
        {
            if (!(msg->keyNumber >= 0x01 && msg->keyNumber <= KEYBOARD_NUMBER))
            {
                respResult = CMD_RESP_FAIL+3;
               // #if (LOG_SET_RT == 1)  
               // log_debug("    - error 0x%02x\n", respResult);
               // #endif
                break;
            }
        
            respResult = CMD_RESP_SUCCEED;
            
            for(uint8_t i=0; i<msg->keyNumber; i++)
            {
                uint8_t pressSensitive   = msg->pressStroke;
                uint8_t raiseSensitive   = msg->raiseStroke;
                uint8_t ki               = msg->key[i].msg.pos;
                uint8_t tmpIsEnable      = msg->key[i].msg.isEnable;
                uint8_t tmpIsContinue    = msg->key[i].msg.isContinue;
                uint8_t rt_ada_mode      = msg->key[i].msg.rt_ada_mode;
                uint8_t rt_ada_extreme   = msg->key[0].msg.rt_ada_extreme;
                uint8_t rt_mode          = msg->rt_mode;
                uint8_t rt_ada_sens      = msg->rt_ada_sens;
                float distPressSensitive = HIVE_RXDA_001MM(pressSensitive); /* get trigger sensitive dist */
                float distRaiseSensitive = HIVE_RXDA_001MM(raiseSensitive);
                
                #if defined(LOG_SET_RT) && (LOG_SET_RT == 1)  
                sprintf(tmpCh, "    - dist: preS:%0.2fmm, raiS:%0.2fmm\n", distPressSensitive, distRaiseSensitive);
                log_debug("%s", tmpCh);
                #endif
                if(    pressSensitive < HIVE_TXDA_001MM(KEY_RT_SENS_MIN(ki)) || pressSensitive > HIVE_TXDA_001MM(KEY_RT_SENS_MAX(ki))
                    || raiseSensitive < HIVE_TXDA_001MM(KEY_RT_SENS_MIN(ki)) || raiseSensitive > HIVE_TXDA_001MM(KEY_RT_SENS_MAX(ki)) )
                {
                    respResult = CMD_RESP_FAIL+1;
                    continue;
                }

                pressSensitive = DIST_TO_LEVEL(distPressSensitive, KEY_RT_ACC(ki)); /* get trigger sensitive */
                raiseSensitive = DIST_TO_LEVEL(distRaiseSensitive, KEY_RT_ACC(ki));
                
                // if(kh.p_attr[ki].rt_en == tmpIsEnable 
                //     && kh.p_attr[ki].rt_mode == rt_mode
                //     && kh.p_attr[ki].rtIsContinue     == tmpIsContinue
                //     && kh.p_attr[ki].rtPressSensitive == pressSensitive
                //     && kh.p_attr[ki].rtRaiseSensitive == raiseSensitive)
                // {
                //     continue;
                // }
                
                key_attr_t *p_attr  = (kh.p_attr + ki);
                p_attr->rt_mode     = rt_mode >= RT_MODE_MAX ? KEY_DFT_RT_MODE : rt_mode;
                p_attr->rt_ada_mode = rt_ada_mode >= RT_ADA_MODE_MAX ? KEY_DFT_RT_ADAPTIVE_MODE : rt_ada_mode;
                p_attr->rt_ada_sens = rt_ada_sens >= RT_ADA_SENS_MAX ? KEY_DFT_RT_ADAPTIVE_SENS : rt_ada_sens;
                p_attr->rt_ada_extreme = rt_ada_extreme;
                
                rt_set_para(ki, tmpIsEnable, tmpIsContinue, pressSensitive, raiseSensitive);
                
                //if (sw_config_table[kh.p_calib->sw[ki]].isRtHighAcc)
                //{
                    //int8_t tmpPress = ACC_BETW_CONV(kh.p_attr[ki].rtPressSensitive, KEY_HACC(ki), KEY_NACC(ki));
                    //int8_t tmpRaise = ACC_BETW_CONV(kh.p_attr[ki].rtRaiseSensitive, KEY_HACC(ki), KEY_NACC(ki));
                    //kh.p_st[ki].rtPressLowAccSens = tmpPress <= 0 ? 1 : tmpPress;
                    //kh.p_st[ki].rtRaiseLowAccSens = tmpRaise <= 0 ? 1 : tmpRaise;
                //}
            }
            #if defined(LOG_SET_RT) && (LOG_SET_RT == 1)  
            log_debug("    - rt Enable key Number:%02d\n", kh.p_gst->rt_en_num);
            #endif

            /* rt number */
            kh.p_gst->rtKeyUsedTableNumber = 0;
            for(uint8_t ki=0; ki<KEYBOARD_NUMBER; ki++)
            {
                if(kh.p_attr[ki].rt_en == true && kh.p_attr[ki].advTrg[0].advType != KEY_ACT_DKS)
                {
                    kh.p_gst->rtKeyUsedTable[kh.p_gst->rtKeyUsedTableNumber++] = ki;
                }
            }
            db_update(DB_SYS, UPDATE_LATER);
        } break;
    }
    
    respBuffer[*respLenght] = respResult;
    *respLenght            += 1;
    #if defined(LOG_SET_RT) && (LOG_SET_RT == 1)  
     cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
   // ret = cmd_respond_push_msg(instDevKeyscan->moduleUart.framePackReceive.cmd, instDevKeyscan->moduleUart.framePackReceive.subCmd, respResult, 0, NULL);
    return respResult;
}

uint16_t ks_cmd_get_rapid_trigger(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    ModuleUartCmdGetRapidTriggerMsg_t *msg = (ModuleUartCmdGetRapidTriggerMsg_t *)reveBuffer; /* get cmd msg */
    ModuleUartCmdGetRapidTriggerRespMsg_t *respMsg = (ModuleUartCmdGetRapidTriggerRespMsg_t *)respBuffer;

    uint8_t respResult = 0xFF, startKeyIndex = 0, EndKeyIndex = 0, sendKeyNum = 0, remainSendKeyNum = 0;
    uint8_t playloadLenght    = respBufferSize;
    uint8_t transmitKeyMaxNum = ((playloadLenght-2) / sizeof(respMsg->key[0])); /* - playload head  */
    uint8_t getWay            = msg->key.msg.way;
    
    if(reveBufferSize<2)
    {
        respResult = 0xFF;
    }
    else
    {

        switch(msg->type)
        {
            case GET_RAPID_TRIGGER_PARA_SINGLE: 
            {
                sendKeyNum       = 1;
                remainSendKeyNum = 0;
                startKeyIndex    = 0;
                EndKeyIndex      = 0;
                kh.p_gst->rtKeyUsedTable[0] = msg->key.msg.pos;
                #if defined(LOG_SET_RT) && (LOG_GET_RT == 1)  
                sprintf(tmpCh, "    - get rt key(%d)  isCon:%02d, pre:%02d, ras:%02d\r\n", msg->key.msg.pos, \
                                                                kh.p_attr[msg->key.msg.pos].rtIsContinue, \
                                                                kh.p_attr[msg->key.msg.pos].rtPressSensitive, \
                                                                kh.p_attr[msg->key.msg.pos].rtRaiseSensitive);
                #endif                
                respResult = CMD_RESP_SUCCEED;
            }break;
            case GET_RAPID_TRIGGER_PARA_ALL: 
            {
                if(getWay==false)
                {
                    //memcpy(kh.p_gst->rtKeyUsedTable, keyboardTableHostToMcu, KEYBOARD_NUMBER);
                    for(uint8_t ki=0; ki<KEYBOARD_NUMBER; ki++)
                    {
                        kh.p_gst->rtKeyUsedTable[ki] = ki;
                    }
                    kh.p_gst->rtKeyUsedTableNumber = KEYBOARD_NUMBER;
                }
                #if defined(LOG_SET_RT) && (LOG_GET_RT == 1)  
                log_debug("    - get all rt para, key(%d) ", KEYBOARD_NUMBER);
                #endif
                if(kh.p_gst->rtKeyUsedTableNumber >= transmitKeyMaxNum)
                {
                    sendKeyNum       = transmitKeyMaxNum;
                    remainSendKeyNum = kh.p_gst->rtKeyUsedTableNumber - transmitKeyMaxNum;
                    startKeyIndex    = KEYBOARD_NUMBER - kh.p_gst->rtKeyUsedTableNumber;
                    EndKeyIndex      = startKeyIndex + sendKeyNum - 1;
                    #if defined(LOG_SET_RT) && (LOG_GET_RT == 1)  
                    sprintf(tmpCh, "send(%d) reamin(%d) (%02d-%02d) \n", sendKeyNum, remainSendKeyNum, startKeyIndex, EndKeyIndex);
                    #endif  
                    kh.p_gst->rtKeyUsedTableNumber = remainSendKeyNum;
                }
                else
                {
                    sendKeyNum       = kh.p_gst->rtKeyUsedTableNumber;
                    remainSendKeyNum = 0;
                    startKeyIndex    = KEYBOARD_NUMBER - kh.p_gst->rtKeyUsedTableNumber;
                    EndKeyIndex      = startKeyIndex + sendKeyNum - 1;
                    #if defined(LOG_SET_RT) && (LOG_GET_RT == 1)  
                    sprintf(tmpCh, "send(%d) reamin(%d) end (%02d-%02d) \n", sendKeyNum, remainSendKeyNum, startKeyIndex, EndKeyIndex);
                    #endif
                    kh.p_gst->rtKeyUsedTableNumber = KEYBOARD_NUMBER;
                }
                respResult = CMD_RESP_SUCCEED;
            } break;
            case GET_RAPID_TRIGGER_PARA_BE_OPEN: 
            {
                if(getWay==false)
                {
                    /* rt number */
                    kh.p_gst->rtKeyUsedTableNumber = 0;
                    for(uint8_t ki=0; ki<KEYBOARD_NUMBER; ki++)
                    {
                        if( kh.p_attr[ki].rt_en == true && kh.p_attr[ki].advTrg[0].advType != KEY_ACT_DKS )
                        {
                            kh.p_gst->rtKeyUsedTable[kh.p_gst->rtKeyUsedTableNumber++] = ki;
                        }
                    }
                }
                #if defined(LOG_SET_RT) && (LOG_GET_RT == 1)  
                log_debug("    - get rt be open para, key(%d) ", kh.p_gst->rt_en_num);
                #endif
                if(kh.p_gst->rtKeyUsedTableNumber >= transmitKeyMaxNum)
                {
                    sendKeyNum       = transmitKeyMaxNum;
                    remainSendKeyNum = kh.p_gst->rtKeyUsedTableNumber - transmitKeyMaxNum;
                    startKeyIndex    = kh.p_gst->rt_en_num - kh.p_gst->rtKeyUsedTableNumber;
                    EndKeyIndex      = startKeyIndex + sendKeyNum - 1;
                    #if defined(LOG_SET_RT) && (LOG_GET_RT == 1)  
                    sprintf(tmpCh, "send(%d) reamin(%d) (%02d-%02d) \n", sendKeyNum, remainSendKeyNum, startKeyIndex, EndKeyIndex);
                    #endif
                    kh.p_gst->rtKeyUsedTableNumber = remainSendKeyNum;
                }
                else
                {
                    sendKeyNum       = kh.p_gst->rtKeyUsedTableNumber;
                    remainSendKeyNum = 0;
                    startKeyIndex    = kh.p_gst->rt_en_num - kh.p_gst->rtKeyUsedTableNumber;
                    EndKeyIndex      = startKeyIndex + sendKeyNum - 1;
                    #if defined(LOG_SET_RT) && (LOG_GET_RT == 1)  
                    sprintf(tmpCh, "send(%d) reamin(%d) end (%02d-%02d) \n", sendKeyNum, remainSendKeyNum, startKeyIndex, EndKeyIndex);
                    #endif
                    kh.p_gst->rtKeyUsedTableNumber = kh.p_gst->rt_en_num;
                }
                respResult = CMD_RESP_SUCCEED;
            } break;
            default:
            {
                respResult = 0xFF;
               // #if (LOG_GET_RT == 1)  
               // sprintf(tmpCh, "    - error 0x%02x\n", respResult);
               // #endif
            } break;
        }
        #if defined(LOG_SET_RT) && (LOG_GET_RT == 1)  
        log_debug("%s", (tmpCh));
        #endif
    }

    /* ------------------------------ resp msg ------------------------------ */

    if(respResult==0xFF)
    {
        respBuffer[*respLenght] = respResult;
        *respLenght  += 1;
        #if defined(LOG_SET_RT) && (LOG_GET_RT == 1)  
        cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
        #endif
        return respResult;
    }

    

    
    //buffer[0] = remainSendKeyNum;
   // buffer[1] = sendKeyNum;
    //buffer[2] = startKeyIndex;
   // buffer[3] = EndKeyIndex;
    //ret = cmd_respond_push_msg(instDevKeyscan->moduleUart.framePackReceive.cmd, instDevKeyscan->moduleUart.framePackReceive.subCmd, respResult, lenght, buffer);
    //return ret;

    uint16_t respMsgLenght    = sizeof(ModuleUartCmdGetRapidTriggerRespMsg_t);
    uint8_t respRtKeyStartPos = 0;
    uint8_t respRtKeyEndPos   = 0;
    uint8_t j                 = 0;
    /* fill playload */
    respMsg->remainKeyNumber = remainSendKeyNum;
    respMsg->keyNumber       = sendKeyNum;
    respRtKeyStartPos        = startKeyIndex;
    respRtKeyEndPos          = EndKeyIndex;
    
    respMsgLenght = 2;
    for(uint8_t i = respRtKeyStartPos; i <= respRtKeyEndPos; i++)
    {
        uint8_t ki               = kh.p_gst->rtKeyUsedTable[i];
        float distPressSensitive = LEVEL_TO_DIST(kh.p_attr[ki].rtPressSensitive, KEY_RT_ACC(ki));
        float distRaiseSensitive = LEVEL_TO_DIST(kh.p_attr[ki].rtRaiseSensitive, KEY_RT_ACC(ki));
        #if defined(LOG_SET_RT) && (LOG_GET_RT == 1)  
        if (i == respRtKeyStartPos || i == respRtKeyEndPos || ki == 33)
        {
            sprintf(tmpCh, "downS:%02d,%0.2fmm(%02d) upS:%02d,%0.2fmm(%02d), ",  kh.p_attr[ki].rtPressSensitive, distPressSensitive, HIVE_TXDA_001MM(distPressSensitive),
                                                                             kh.p_attr[ki].rtRaiseSensitive, distRaiseSensitive, HIVE_TXDA_001MM(distRaiseSensitive));
            log_debug("    - [%02d,%02d] %s", i, ki, tmpCh);
            log_debug("isEn:%s, isCtu:%s \n", kh.p_attr[ki].rt_en==true ? "true" : "false", kh.p_attr[ki].rtIsContinue==true ? "true" : "false" );
        }
        else if (i == respRtKeyStartPos+1)
        {
            log_debug("    - ...\n");
        }
        #endif  
       // respMsg->key[j].msg.pos = keyboardTableMcuToHost[kh.p_gst->rtKeyUsedTable[i]];
        respMsg->key[j].msg.pos            = ki;
        respMsg->key[j].msg.pressSensitive = HIVE_TXDA_001MM(distPressSensitive);
        respMsg->key[j].msg.raiseSensitive = HIVE_TXDA_001MM(distRaiseSensitive);
        respMsg->key[j].msg.isEnable       = kh.p_attr[ki].rt_en;
        respMsg->key[j].msg.isContinue     = kh.p_attr[ki].rtIsContinue;
        respMsg->key[j].msg.rt_ada_mode    = kh.p_attr[ki].rt_ada_mode;
        respMsg->key[j].msg.rt_ada_extreme = kh.p_attr[ki].rt_ada_extreme;
        respMsg->key[j].msg.rt_mode        = kh.p_attr[ki].rt_mode;
        respMsg->key[j].msg.rt_ada_sens    = kh.p_attr[ki].rt_ada_sens;
        respMsgLenght                     += ARRAY_SIZE(respMsg->key[0].buffer);
        j++;
    }
    *respLenght = respMsgLenght;
    #if defined(LOG_SET_RT) && (LOG_GET_RT == 1)  
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif

    return respResult;
}

uint16_t ks_cmd_set_dead_band(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    ModuleUartCmdSetDeadBandMsg_t *msg = (ModuleUartCmdSetDeadBandMsg_t *)reveBuffer; /* get cmd msg */
    #if defined(LOG_SET_DEAD) && (LOG_SET_DEAD == 1)  
    cmd_show_playload(__func__, "receive", (uint8_t *)msg, reveBufferSize);
    #endif
    uint8_t respResult = CMD_RESP_FAIL;
   (void)respBufferSize;
   
    switch(msg->keyNumber)
    {
        case 0x00: /* reset default */
        {
            #if defined(LOG_SET_DEAD) && (LOG_SET_DEAD == 1)  
            log_debug("    - Set dead band Reset all \n");
            #endif
            //dev_flash_read_user_msg_all();
            //db_get_binfo();
            fix_set_default_dead_para();
            db_update(DB_SYS, UPDATE_LATER);
            respResult = CMD_RESP_SUCCEED;
            //respResult = dev_flash_write_user_msg_all()==true ? CMD_RESP_SUCCEED : CMD_RESP_FAIL;
        } break;
        case 0xFF: /* set all */
        {
            respResult = CMD_RESP_SUCCEED;
            if (reveBufferSize != 4)
            {
                respResult = CMD_RESP_FAIL+0;
                #if defined(LOG_SET_DEAD) && (LOG_SET_DEAD == 1)  
                log_debug("    - error 0x%02x\n", respResult);
                #endif
                break;
            }
            
            for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
            {
                uint8_t tmpTopDead = msg->key[0].topDead;
                uint8_t tmpBotDead = msg->key[0].bottomDead;
                float distTopDead  = HIVE_RXDA_001MM(tmpTopDead); /* get dead band */
                float distBotDead  = HIVE_RXDA_001MM(tmpBotDead);
                
                #if defined(LOG_SET_DEAD) && (LOG_SET_DEAD == 1)  
                sprintf(tmpCh, "topDead:%0.2fmm, botDead:%0.2fmm", distTopDead, distBotDead);
                log_debug("    - dist: %s\n", tmpCh);
                #endif
                if (   (int8_t)tmpTopDead < HIVE_TXDA_001MM(KEY_DEADBAND_MIN(ki)) || (int8_t)tmpTopDead > HIVE_TXDA_001MM(KEY_DEADBAND_MAX(ki))
                    || (int8_t)tmpBotDead < HIVE_TXDA_001MM(KEY_DEADBAND_MIN(ki)) || (int8_t)tmpBotDead > HIVE_TXDA_001MM(KEY_DEADBAND_MAX(ki))) /* zero pointless */
                {
                    respResult = CMD_RESP_FAIL+0;
                    #if defined(LOG_SET_DEAD) && (LOG_SET_DEAD == 1)  
                    log_debug("    - error 0x%02x\n", respResult);
                    #endif
                    break;
                }
                
                tmpTopDead = DIST_TO_LEVEL(distTopDead, KEY_DEADBAND_ACC(ki)); /* get dead band */
                tmpBotDead = DIST_TO_LEVEL(distBotDead, KEY_DEADBAND_ACC(ki));

                if (tmpTopDead == kh.p_attr[ki].deadbandTop && tmpBotDead == kh.p_attr[ki].deadbandBottom)
                {
                    //log_debug("    - wran %d (same value): - pos:%02d, topDead:%d, bottomDead:%d\n", CMD_RESP_FAIL+3, ki, tmpTopDead, tmpBotDead);
                    continue;
                }
                
                #if defined(LOG_SET_DEAD) && (LOG_SET_DEAD == 1)  
                if (ki == 0 || ki == KEYBOARD_NUMBER-1 || ki == 33)
                {
                    log_debug("    - Set dead band all - pos:%02d, topDead:%d, bottomDead:%d\n", ki, tmpTopDead, tmpBotDead);
                }
                else if (ki == 1)
                {
                    log_debug("    - Set dead band all ...\n");
                }
                #endif

                
                for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
                {
                    fix_set_dead_para(ki, tmpTopDead, tmpBotDead);
                }
            }
            db_update(DB_SYS, UPDATE_LATER);
        } break;
        default:     
        //case 0x01 ... KEYBOARD_NUMBER:
        {
            respResult = CMD_RESP_SUCCEED;
            if (!(msg->keyNumber >= 0x01 && msg->keyNumber <= KEYBOARD_NUMBER))
            {
                respResult = CMD_RESP_FAIL+4;
                #if defined(LOG_SET_DEAD) && (LOG_SET_DEAD == 1)  
                log_debug("    - error 0x%02x\n", respResult);
                #endif
                break;
            }
            
            for(uint8_t i = 0; i < msg->keyNumber; i++)
            {
                uint8_t ki         = msg->key[i].pos;
                uint8_t tmpTopDead = msg->key[i].topDead;
                uint8_t tmpBotDead = msg->key[i].bottomDead;
                float distTopDead  = HIVE_RXDA_001MM(tmpTopDead); /* get dead band */
                float distBotDead  = HIVE_RXDA_001MM(tmpBotDead);
                
                #if defined(LOG_SET_DEAD) && (LOG_SET_DEAD == 1)  
                sprintf(tmpCh, "topDead:%0.2fmm, botDead:%0.2fmm", distTopDead, distBotDead);
                log_debug("    - dist: %s\n", tmpCh);
                #endif
                if (   (int8_t)tmpTopDead > HIVE_TXDA_001MM(KEY_DEADBAND_MAX(ki))
                    || (int8_t)tmpBotDead < HIVE_TXDA_001MM(KEY_DEADBAND_MIN(ki)) || (int8_t)tmpBotDead > HIVE_TXDA_001MM(KEY_DEADBAND_MAX(ki))) /* zero pointless */
                {
                    #if defined(LOG_SET_DEAD) && (LOG_SET_DEAD == 1)  
                    log_debug("    - error %d (overmaximal):  Set dead band number(%d/%d) - pos:%02d(%02d,%02d), topDead:%d, bottomDead:%d\n", CMD_RESP_FAIL+2,  \
                                                        msg->keyNumber, i, ki, tmpTopDead, tmpBotDead);
                    #endif
                    respResult = CMD_RESP_FAIL+5;
                    break;
                }
                    
                tmpTopDead = DIST_TO_LEVEL(distTopDead, KEY_DEADBAND_ACC(ki)); /* get dead band */
                tmpBotDead = DIST_TO_LEVEL(distBotDead, KEY_DEADBAND_ACC(ki));
                

                if (tmpTopDead == kh.p_attr[ki].deadbandTop && tmpBotDead == kh.p_attr[ki].deadbandBottom)
                {
                    //log_debug("    - wran %d (same value):  Set dead band number(%d/%d) - pos:%02d, topDead:%d, bottomDead:%d\n", CMD_RESP_FAIL+3,  msg->keyNumber, i, ki, tmpTopDead, tmpBotDead);
                    continue;
                }
                #if defined(LOG_SET_DEAD) && (LOG_SET_DEAD == 1)  
                if (i == 0 || i == msg->keyNumber-1 || ki == 33)
                {
                    log_debug("    - Set dead band number(%d/%d) - pos:%02d, topDead:%d, bottomDead:%d\n", msg->keyNumber, i, ki, tmpTopDead, tmpBotDead);
                }
                else if (ki == 1)
                {
                    log_debug("    - Set dead band number ...\n");
                }
                #endif
                
                fix_set_dead_para(ki, tmpTopDead, tmpBotDead);
            }
            db_update(DB_SYS, UPDATE_LATER);
        } break;
    }

    
    respBuffer[*respLenght] = respResult;
    *respLenght            += 1;
    #if defined(LOG_SET_DEAD) && (LOG_SET_DEAD == 1)  
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
  //  ret = cmd_respond_push_msg(instDevKeyscan->moduleUart.framePackReceive.cmd, instDevKeyscan->moduleUart.framePackReceive.subCmd, respResult, 0, NULL);
    return respResult;
}

uint16_t ks_cmd_get_dead_band(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    ModuleUartCmdGetDeadBandMsg_t *msg = (ModuleUartCmdGetDeadBandMsg_t *)reveBuffer; /* get cmd msg */
    #if defined(LOG_SET_DEAD) && (LOG_GET_DEAD == 1)  
    cmd_show_playload(__func__, "receive", (uint8_t *)msg, reveBufferSize);
    #endif

    uint8_t respResult = 0xFF, startKeyIndex = 0, EndKeyIndex = 0, sendKeyNum = 0, remainSendKeyNum = 0;
    
   // const uint8_t transmitSize = 64;
   // const uint8_t lenght = 4;
   // uint8_t buffer[lenght] = {0};
    
    uint8_t playloadLenght = respBufferSize;
    uint8_t transmitKeyMaxNum = ((playloadLenght-2) / 3); /* - playload head  */
    uint8_t getWay = msg->key.msg.way;
    
    if(reveBufferSize < 2)
    {
        respResult = 0xFF;
        #if defined(LOG_SET_DEAD) && (LOG_GET_DEAD == 1)  
        log_debug("    - error 0x%02x\n", respResult);
        #endif
    }
    else
    {
        switch(msg->type)
        {
            case GET_DEAD_BAND_PARA_SINGLE: 
            {
                sendKeyNum       = 1;
                remainSendKeyNum = 0;
                startKeyIndex    = msg->key.msg.pos;
                EndKeyIndex      = msg->key.msg.pos;
                // kh.p_gst->deadKeyUsedTable[0] = msg->key.msg.pos;
                #if defined(LOG_SET_DEAD) && (LOG_GET_DEAD == 1)  
                sprintf(tmpCh, "    - get dead para key(%d) \n", msg->key.msg.pos);
                #endif
                
                respResult = CMD_RESP_SUCCEED;
            }break;
            case GET_DEAD_BAND_PARA_ALL: 
            {
                if(getWay==false) //reset start
                {
                    // for(uint8_t ki=0; ki<KEYBOARD_NUMBER; ki++)
                    // {
                    //     kh.p_gst->deadKeyUsedTable[ki] = ki;
                    // }
                    kh.p_gst->deadKeyRespLen = KEYBOARD_NUMBER;
                }
                #if defined(LOG_SET_DEAD) && (LOG_GET_DEAD == 1)  
                log_debug("    - get all dead para para, key(%d) ", KEYBOARD_NUMBER);
                #endif
                if(kh.p_gst->deadKeyRespLen >= transmitKeyMaxNum)
                {
                    sendKeyNum = transmitKeyMaxNum;

                    remainSendKeyNum = kh.p_gst->deadKeyRespLen - transmitKeyMaxNum;
                    
                    startKeyIndex = KEYBOARD_NUMBER - kh.p_gst->deadKeyRespLen;
                    EndKeyIndex = startKeyIndex + sendKeyNum - 1;
                    
                    #if defined(LOG_SET_DEAD) && (LOG_GET_DEAD == 1)  
                    sprintf(tmpCh, "send(%d) reamin(%d) (%02d-%02d) \n", sendKeyNum, remainSendKeyNum, startKeyIndex, EndKeyIndex);
                    #endif
                    kh.p_gst->deadKeyRespLen = remainSendKeyNum;
                }
                else
                {
                    sendKeyNum = kh.p_gst->deadKeyRespLen;
                    remainSendKeyNum = 0;
                    
                    startKeyIndex = KEYBOARD_NUMBER - kh.p_gst->deadKeyRespLen;
                    EndKeyIndex = startKeyIndex + sendKeyNum - 1;
                    
                    #if defined(LOG_SET_DEAD) && (LOG_GET_DEAD == 1)  
                    sprintf(tmpCh, "send(%d) reamin(%d) end (%02d-%02d) \n", sendKeyNum, remainSendKeyNum, startKeyIndex, EndKeyIndex);
                    #endif
                    kh.p_gst->deadKeyRespLen = KEYBOARD_NUMBER;
                }
                respResult = CMD_RESP_SUCCEED;
            } break;
            default:
            {
                respResult = 0xFF;
                #if defined(LOG_SET_DEAD) && (LOG_GET_DEAD == 1)  
                sprintf(tmpCh, "    - error 0x%02x\n", respResult);
                #endif
            } break;
        }
        #if defined(LOG_SET_DEAD) && (LOG_GET_DEAD == 1)  
        log_debug("%s", (tmpCh) );
        #endif
    }
    if(respResult==0xFF)
    {
        respBuffer[*respLenght] = respResult;
        *respLenght  += 1;
        #if defined(LOG_SET_DEAD) && (LOG_GET_DEAD == 1)  
        cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
        #endif
        return respResult;
    }

    /* ------------------------------ resp msg ------------------------------ */
   // buffer[0] = remainSendKeyNum;
  //  buffer[1] = sendKeyNum;
   // buffer[2] = startKeyIndex;
   // buffer[3] = EndKeyIndex;

   // ret = cmd_respond_push_msg(instDevKeyscan->moduleUart.framePackReceive.cmd, instDevKeyscan->moduleUart.framePackReceive.subCmd, respResult, lenght, buffer);
   // return ret;



    ModuleUartCmdGetDeadBandRespMsg_t *respMsg = (ModuleUartCmdGetDeadBandRespMsg_t *)respBuffer;
    uint16_t respMsgLenght    = sizeof(ModuleUartCmdGetDeadBandRespMsg_t);
    uint8_t respRtKeyStartPos = 0;
    uint8_t respRtKeyEndPos   = 0;
    uint8_t j                 = 0;

    /* fill playload */
    respMsg->remainKeyNumber = remainSendKeyNum;
    respMsg->keyNumber       = sendKeyNum;
    respRtKeyStartPos        = startKeyIndex;
    respRtKeyEndPos          = EndKeyIndex;

    respMsgLenght = 2;
    for (uint8_t ki = respRtKeyStartPos; ki <= respRtKeyEndPos; ki++)
    {
        float distTopDead = LEVEL_TO_DIST(kh.p_attr[ki].deadbandTop, KEY_DEADBAND_ACC(ki));
        float distBotDead = LEVEL_TO_DIST(kh.p_attr[ki].deadbandBottom, KEY_DEADBAND_ACC(ki));
        #if defined(LOG_SET_DEAD) && (LOG_GET_DEAD == 1)  
        if (ki == respRtKeyStartPos || ki == respRtKeyEndPos || ki == 33)
        {
            sprintf(tmpCh, "dt:%02d,%0.2fmm db:%02d,%0.2fmm, ",  kh.p_attr[ki].deadbandTop, distTopDead, 
                                                                 kh.p_attr[ki].deadbandBottom, distBotDead);
            log_debug("    - [%02d] %s \n", ki, tmpCh);
        }
        else if (ki == respRtKeyStartPos+1)
        {
            log_debug("    - ...\n");
        }
        #endif
        
        respMsg->key[j].msg.pos        = ki;
        respMsg->key[j].msg.topDead    = HIVE_TXDA_001MM(distTopDead);
        respMsg->key[j].msg.bottomDead = HIVE_TXDA_001MM(distBotDead);
        respMsgLenght                 += ARRAY_SIZE(respMsg->key[0].buffer);
        j++;
    }

    *respLenght = respMsgLenght;
    #if defined(LOG_SET_DEAD) && (LOG_GET_DEAD == 1)  
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
    return respResult;
}

uint16_t ks_cmd_set_key_report(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint8_t respResult = CMD_RESP_FAIL;
    ModuleUartCmdSetKeyReportMsg_t *msg = (ModuleUartCmdSetKeyReportMsg_t *)reveBuffer;
    (void)respBufferSize;
    (void)reveBufferSize;

    #if defined(LOG_SET_RPT) && (LOG_SET_RPT == 1)  
    cmd_show_playload(__func__, "receive", (uint8_t *)msg, reveBufferSize);
    log_debug("    - keyReport:%s\n", msg->buffer==KEY_REPORT_OUTTYPE_CLOSE    ? "close" \
                                      : msg->buffer==KEY_REPORT_OUTTYPE_REALTIME ? "realtime"  \
                                      : msg->buffer==KEY_REPORT_OUTTYPE_TRIGGER  ? "trigger" \
                                      : "unkonw");
    #endif


    if(msg->buffer >= KEY_REPORT_OUTTYPE_MAX)
    {
        respResult = CMD_RESP_FAIL;
    }
    else
    {
        // debug
        // uint8_t buf[2] = {0x02, 0x58}; //600
        // // uint8_t buf[2] = {0x01, 0x90}; //400
        // cmd_mcu_send(CMD_KEY_PARA, MCU_KS_SET_POLLING_RATE, buf, 2);

    
        //kh.p_gst->key_rpt_type = msg->buffer;
        cali_set_rpt(msg->buffer);
        respResult = CMD_RESP_SUCCEED;
    }

    respBuffer[0] = respResult;
    *respLenght += 1;
    #if defined(LOG_SET_RPT) && (LOG_SET_RPT == 1)  
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
    return respResult;
}
uint16_t ks_cmd_get_key_report(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint8_t respResult = CMD_RESP_FAIL;
    ModuleUartCmdGetKeyReportMsg_t *respMsg = (ModuleUartCmdGetKeyReportMsg_t *)respBuffer;
    uint16_t respMsgLenght    = sizeof(ModuleUartCmdGetKeyReportMsg_t);
    (void)respBufferSize;
    (void)reveBufferSize;
    (void)reveBuffer;
    #if defined(LOG_SET_RPT) && (LOG_SET_RPT == 1)  
    cmd_show_playload(__func__, "receive", (uint8_t *)reveBuffer, reveBufferSize);

    log_debug("    - keyReport:%s\n", respMsg->buffer==KEY_REPORT_OUTTYPE_CLOSE    ? "close" \
                                      : respMsg->buffer==KEY_REPORT_OUTTYPE_REALTIME ? "realtime"  \
                                      : respMsg->buffer==KEY_REPORT_OUTTYPE_TRIGGER  ? "trigger" \
                                      : "unkonw");
    #endif

    respMsg->buffer = kh.p_gst->key_rpt_type;
    respResult = CMD_RESP_SUCCEED;
    
    *respLenght = respMsgLenght;
    #if defined(LOG_SET_RPT) && (LOG_SET_RPT == 1)  
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
    return respResult;
}

uint16_t ks_cmd_get_cali(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint8_t respResult = CMD_RESP_FAIL;
    ModuleUartCmdGetKeyCalibMsg_t *respMsg = (ModuleUartCmdGetKeyCalibMsg_t *)respBuffer;
    uint16_t respMsgLenght   = sizeof(ModuleUartCmdGetKeyCalibMsg_t);
    cail_step_t calibStatus = cali_get_rpt();
    (void)respBufferSize;
    (void)reveBufferSize;
    (void)reveBuffer;

    #if defined(LOG_GET_CALI) && (LOG_GET_CALI == 1)  
    cmd_show_playload(__func__, "receive", (uint8_t *)reveBuffer, reveBufferSize);
    log_debug("    - key calib status:%s\n",calibStatus==KEY_CALIB_IDLE       ? "idle"  \
                                          : calibStatus==KEY_CALIB_START      ? "start" \
                                          : calibStatus==KEY_CALIB_DONE       ? "done"  \
                                          : calibStatus==KEY_CALIB_CLEAR      ? "clear" \
                                          : calibStatus==KEY_CALIB_SWITCH_OUT ? "swless" \
                                          : "unkonw");
    #endif

    uint8_t isStartCalib = (calibStatus == KEY_CALIB_START) ? true : false;

    respMsg->buffer = (cail_step_t)isStartCalib;
    respResult = CMD_RESP_SUCCEED;
    
    *respLenght = respMsgLenght;
    #if defined(LOG_GET_CALI) && (LOG_GET_CALI == 1)  
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
    return respResult;
}

#if 0

uint16_t ks_cmd_set_dks_bind(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint16_t ret = true;
    ModuleUartCmdSetAdvTriggerDksBindMsg_t *msg = (ModuleUartCmdSetAdvTriggerDksBindMsg_t *)reveBuffer; /* get cmd msg */
    cmd_show_playload(__func__, "receive", (uint8_t *)reveBuffer, reveBufferSize);

    uint8_t respResult      = CMD_RESP_FAIL;
    uint8_t tmpDksParaIndex = msg->dksId;
    uint8_t tmpOperate      = msg->operate;
    uint8_t tmpKi           = msg->keyCode;
    key_dks_t *dksPara      = NULL;
    
    log_debug("    - ki: %d\r\n", tmpKi);
    log_debug("    - dksParaId: %d\r\n", tmpDksParaIndex);
    log_debug("    - operate: %s\r\n", (tmpOperate == 1) ? "bind" : "unbind");
    

    if(tmpDksParaIndex >= KEY_DFT_DKS_PARA_TABLE_MAX || tmpKi >= KEYBOARD_NUMBER)
    {
        respResult = CMD_RESP_FAIL+0;
        log_debug("    - error 0x%02x\r\n", respResult);
        goto ERROR;
    }

    //dev_flash_read_user_msg_all();
    dksPara = &kh.p_qty->dksParaTable[tmpDksParaIndex];
    switch(tmpOperate)
    {
        case 0x01: /* bind */
        {
            if(kh.p_qty->dksParaNumber==0)
            {
                respResult = CMD_RESP_FAIL+1;
                log_debug("    - error 0x%02x bind dks para table is empty\r\n", respResult);
                break;
            }
            else if(dksPara->startLevel==0 && dksPara->bottomLevel==0)
            {
                respResult = CMD_RESP_FAIL+2;
                log_debug("    - error 0x%02x bind dks para table value NULL\r\n", respResult);
                break;
            }
            
            log_debug("    - bind dks keyPos:%02d, dks para table id(%d), \r\n", tmpKi, tmpDksParaIndex);
            /* change RT number */
            if(kh.p_attr[tmpKi].advType!=KEY_ACT_DKS) 
            {
                int8_t change = (kh.p_attr[tmpKi].rt_en==true) ? (-1) : (0);
                kh.p_gst->rt_en_num += change;
                
                /* used dks bind table and number */
                kh.p_gst->dksBindUsedTable[kh.p_gst->dksBindUsedNumber] = tmpKi;
                kh.p_gst->dksBindUsedNumber += 1;
            }
            
            kh.p_attr[tmpKi].advType = KEY_ACT_DKS;
            kh.p_attr[tmpKi].advTriggerPara.dks = dksPara;
            kh.p_attr[tmpKi].dksParaTableUsedId = tmpDksParaIndex;

            /* rt number */
            kh.p_gst->rtKeyUsedTableNumber = 0;
            for(uint8_t ki=0; ki<KEYBOARD_NUMBER; ki++)
            {
                if(kh.p_attr[ki].rt_en==true && kh.p_attr[ki].advType!=KEY_ACT_DKS)
                    kh.p_gst->rtKeyUsedTable[kh.p_gst->rtKeyUsedTableNumber++] = ki;
            }
            db_update(DB_SYS);
            respResult = CMD_RESP_SUCCEED;
//            respResult = dev_flash_write_user_msg_all()==true ? CMD_RESP_SUCCEED : CMD_RESP_FAIL;
        } break;
        case 0x00: /* unbind */
        {
            if(kh.p_attr[tmpKi].advType!=KEY_ACT_DKS)
            {
                respResult = CMD_RESP_FAIL+3;
                log_debug("    - error 0x%02x unbind not dks trigger mode\r\n", respResult);
                break;
            }
            else if(kh.p_attr[tmpKi].advTriggerPara.dks==NULL)
            {
                respResult = CMD_RESP_FAIL+4;
                log_debug("    - error 0x%02x unbind dks not bound\r\n");
                break;
            }
            else if(dksPara->startLevel==0 && dksPara->bottomLevel==0)
            {
                respResult = CMD_RESP_FAIL+5;
                log_debug("    - error 0x%02x unbind dks para table value NULL\r\n", respResult);
                break;
            }

            log_debug("    - unbind dks keyPos:%02d, dks para table id(%d), \r\n", tmpKi, tmpDksParaIndex);
            /* change RT number */
            if(kh.p_attr[tmpKi].advType==KEY_ACT_DKS)
            {
                int8_t change = (kh.p_attr[tmpKi].rt_en==true) ? (1) : (0);
                kh.p_gst->rt_en_num += change;
            }
            
            kh.p_attr[tmpKi].advType            = KEY_ACT_UND;
            kh.p_attr[tmpKi].advTriggerPara.dks = NULL;
            kh.p_attr[tmpKi].dksParaTableUsedId = 0;
            
            /* rt number */
            /* used dks bind table and number */
            kh.p_gst->rtKeyUsedTableNumber        = 0;
            kh.p_gst->dksBindUsedNumber = 0;
            for(uint8_t ki=0; ki<KEYBOARD_NUMBER; ki++)
            {
                /* RT used table */
                if(kh.p_attr[ki].rt_en==true && kh.p_attr[ki].advType!=KEY_ACT_DKS)
                { 
                    uint8_t index = kh.p_gst->rtKeyUsedTableNumber;
                    kh.p_gst->rtKeyUsedTable[index] = ki;
                    kh.p_gst->rtKeyUsedTableNumber += 1;
                }
                /* DKS used table */
                if(kh.p_attr[ki].advType==KEY_ACT_DKS)
                {
                    uint8_t index = kh.p_gst->dksBindUsedNumber;
                    kh.p_gst->dksBindUsedTable[index] = ki;
                    kh.p_gst->dksBindUsedNumber      += 1;
                    
                }
            }
            db_update(DB_SYS);
            respResult = CMD_RESP_SUCCEED;
//            respResult = dev_flash_write_user_msg_all()==true ? CMD_RESP_SUCCEED : CMD_RESP_FAIL;
        } break;
        default: 
        {
            respResult = CMD_RESP_FAIL+6;
            log_debug("    - error 0x%02x (unkonw operate)\r\n", respResult);
            break;
        } 
    }

ERROR:
    respBuffer[*respLenght] = respResult;
    *respLenght            += 1;
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
   // ret = cmd_respond_push_msg(instDevKeyscan->moduleUart.framePackReceive.cmd, instDevKeyscan->moduleUart.framePackReceive.subCmd, respResult, 0, NULL);

    return ret;
}
#endif
#if 1
uint16_t ks_cmd_set_dks_para_table(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint16_t ret = true;
    ModuleUartCmdSetAdvTriggerDksMsg_t *msg = (ModuleUartCmdSetAdvTriggerDksMsg_t *)reveBuffer; /* get cmd msg */
    #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
    cmd_show_playload(__func__, "receive", (uint8_t *)reveBuffer, reveBufferSize);
    #endif

    uint8_t respResult         = CMD_RESP_FAIL;
    uint8_t dksId              = msg->id;
    SetDksOperate_e dksOperate = msg->operate;
    uint16_t toTopTrgLevel     = msg->toTopTrgLevel;
    uint16_t toBotTrgLevel     = msg->toBotTrgLevel;
    dks_t *dks                 = NULL;
    
    uint8_t keysw = KEY_DFT_SHAFT;
    //#warning "debug shaft, todo jay"
    
    // dynamic_key_t dksKeyCode[KEY_DFT_DKS_VIRTUAL_KEY_NUMBER];
    // uint8_t dksKeyEvent[KEY_DFT_DKS_VIRTUAL_KEY_NUMBER];

    
    float distToTopTrgLevel = HIVE_RXDA_001MM(toTopTrgLevel); /* get trigger sensitive */
    float distToBotTrgLevel = HIVE_RXDA_001MM(toBotTrgLevel);
    #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
    sprintf(tmpCh, "toTopTrgLev:%0.2fmm, toBotTrgLev:%0.2fmm", distToTopTrgLevel, distToBotTrgLevel);
    log_debug("    - dist: %s\n", tmpCh);
    #endif
    

    if( dksId >= KEY_DFT_DKS_PARA_TABLE_MAX)
    {
        respResult = CMD_RESP_FAIL+0;
        #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
        log_debug("    - error 0x%02x\r\n", respResult);
        #endif
    }
    else if((dksOperate!=SET_DKS_PARA_OPERATE_DEL) && (reveBufferSize!=sizeof(ModuleUartCmdSetAdvTriggerDksMsg_t)))
    {
        respResult = CMD_RESP_FAIL+1;
        #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
        log_debug("    - error 0x%02x, reve:%d(%d)\r\n", respResult, reveBufferSize, sizeof(ModuleUartCmdSetAdvTriggerDksMsg_t));
        #endif
    }
    else if(dksOperate!=SET_DKS_PARA_OPERATE_DEL&& 
            (  toTopTrgLevel < HIVE_TXDA_001MM(sw_config_table[keysw].dksToTopMin) || toTopTrgLevel > HIVE_TXDA_001MM(sw_config_table[keysw].dksToTopMax)
            || toBotTrgLevel < HIVE_TXDA_001MM(sw_config_table[keysw].dksToBotMin) || toBotTrgLevel > HIVE_TXDA_001MM(sw_config_table[keysw].dksToBotMax)) )
    {
        respResult = CMD_RESP_FAIL+2;
        #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
        log_debug("    - error 0x%02x\r\n", respResult);
        #endif
    }
    else 
    {
        /* get trigger level */
        toTopTrgLevel = DIST_TO_LEVEL(distToTopTrgLevel, sw_config_table[keysw].dksAcc);
        toBotTrgLevel = DIST_TO_LEVEL(distToBotTrgLevel, sw_config_table[keysw].dksAcc);
        // if (sw_config_table[keysw].isRtHighAcc && distTrigger > KEY_SHAFT_G_JADE_NORMAL_ACC_TO_DIST)
        // {
        //    toTopTrgLevel = DIST_TO_LEVEL(distTrigger - KEY_SHAFT_G_JADE_NORMAL_ACC_TO_DIST, KEY_SHAFT_G_JADE_HIGH_ACC) + KEY_SHAFT_G_JADE_NORMAL_ACC_TO_LEVEL;
        // }
        // if (sw_config_table[keysw].isRtHighAcc && distRaise > KEY_SHAFT_G_JADE_NORMAL_ACC_TO_DIST)
        // {
        //    toBotTrgLevel = DIST_TO_LEVEL(distRaise - KEY_SHAFT_G_JADE_NORMAL_ACC_TO_DIST, KEY_SHAFT_G_JADE_HIGH_ACC) + KEY_SHAFT_G_JADE_NORMAL_ACC_TO_LEVEL;
        // }


        
     //   memcpy(dksKeyCode, msg->keyCode, KEY_DFT_DKS_VIRTUAL_KEY_NUMBER*sizeof(dynamic_key_t));
     //   memcpy(dksKeyEvent, msg->keyEvent, KEY_DFT_DKS_VIRTUAL_KEY_NUMBER);
            
       // dev_flash_read_user_msg_all();
       // dksPara = &kh.p_qty->dksParaTable[tmpDksId];
        dks = &sk_la_lm_kc_info.sk.dks_table[dksId];
        switch(dksOperate)
        {
            case SET_DKS_PARA_OPERATE_DEL:
            {
                if(sk_la_lm_kc_info.sk.superkey_table[SKT_DKS].free_size == KEY_DFT_DKS_PARA_TABLE_MAX || sk_la_lm_kc_info.sk.superkey_table[SKT_DKS].storage_mask == 0)
                //if(kh.p_qty->dksParaNumber==0)
                {
                    respResult = CMD_RESP_FAIL+3;
                    #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
                    log_debug("    - error 0x%02x delete dks para tabel is empty\r\n", respResult);
                    #endif
                    break;
                }
                else if(dks->para.toTopLevel == 0 && dks->para.toBotLevel == 0)
                {
                    respResult = CMD_RESP_FAIL+4;
                    #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
                    log_debug("    - error 0x%02x \r\n", respResult);
                    #endif
                    break;
                }
                #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
                log_debug("    - delete dks para id(%d)\r\n", dksId);
                #endif
                
                memset(dks, 0, sizeof(dks_t));
                
               // memset(dks->para.keyCode, 0xFF, sizeof(dynamic_key_t)*KEY_DFT_DKS_VIRTUAL_KEY_NUMBER);
                //kh.p_qty->dksParaNumber -= 1;
                sk_la_lm_kc_info.sk.superkey_table[SKT_DKS].free_size    += 1;
                sk_la_lm_kc_info.sk.superkey_table[SKT_DKS].storage_mask &= ~(0x01<<dksId); //clear bit
                

                bool isUpdateKc   = false;
                bool isUpdateAttr = false;

                /* delete bind key */
                uint8_t usedNumber = kh.p_gst->dksBindUsedNumber;
                for (uint8_t i = 0; i<usedNumber; i++)
                {
                    uint8_t ki = kh.p_gst->dksBindUsedTable[i];
                    for (uint8_t kcmid = 0; kcmid < BOARD_KCM_MAX; kcmid++)
                    {
                        if (kh.p_attr[ki].advTrg[kcmid].advType == KEY_ACT_DKS && kh.p_attr[ki].advTrg[kcmid].dksParaTableUsedId == dksId)
                        {
                            kh.p_gst->dksBindUsedNumber                   -= 1;
                            kh.p_attr[ki].advTrg[kcmid].advType            = KEY_ACT_UND;
                            kh.p_attr[ki].advTrg[kcmid].para.dks           = NULL;
                            kh.p_attr[ki].advTrg[kcmid].dksParaTableUsedId = 0;
                            #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
                            log_debug("    - delete dks band key(%d)\r\n", ki);
                            #endif
                            //todo reset default para
                            // if (kcmid == 0) // handle 0 layer
                            // {
                            //     kc_table[kcmid][ki].ty  = KCT_KB;
                            //     kc_table[kcmid][ki].sty = 0;
                            //     kc_table[kcmid][ki].co  = ki;
                            //     isUpdateKc = true;
                            // }

                           // kc_t *pkc  = mx_get_kco(gbinfo.kc.cur_kcm, ki);                    
                           // memcpy(&kc_table[0][ki], pkc, sizeof(kc_t));

                            isUpdateAttr = true;
                        }
                    }
                }
                
                kh.p_gst->rtKeyUsedTableNumber = 0;
                kh.p_gst->dksBindUsedNumber    = 0;
                for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
                {
                    /* RT used table */
                    if (kh.p_attr[ki].rt_en == true && kh.p_attr[ki].advTrg[0].advType != KEY_ACT_DKS)
                    {
                        uint8_t index = kh.p_gst->rtKeyUsedTableNumber;
                        kh.p_gst->rtKeyUsedTable[index] = ki;
                        kh.p_gst->rtKeyUsedTableNumber += 1;
                    }
                    /* DKS used table */
                    for (uint8_t kcmid = 0; kcmid < BOARD_KCM_MAX; kcmid++)
                    {
                        if (kh.p_attr[ki].advTrg[kcmid].advType == KEY_ACT_DKS)
                        {
                            uint8_t index = kh.p_gst->dksBindUsedNumber;
                            kh.p_gst->dksBindUsedTable[index] = ki;
                            kh.p_gst->dksBindUsedNumber      += 1;
                        }
                    }
                }

                if (isUpdateAttr == true)
                {
                    db_update(DB_SYS, UPDATE_LATER);
                }
                if (isUpdateKc == true)
                {
                    //db_update(DB_KC, UPDATE_LATER);
                }
                db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
                respResult = CMD_RESP_SUCCEED;
                // respResult = dev_flash_write_user_msg_all()==true ? CMD_RESP_SUCCEED : CMD_RESP_FAIL;
            } break;
            case SET_DKS_PARA_OPERATE_ADD:
            case SET_DKS_PARA_OPERATE_MOD: 
            {
                #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
                /*
                char outBuf[KEY_DFT_DKS_VIRTUAL_KEY_NUMBER][16] = {'\0'};
                itoa(msg->keyEvent[0], outBuf[0], 2);
                itoa(msg->keyEvent[1], outBuf[1], 2);
                itoa(msg->keyEvent[2], outBuf[2], 2);
                itoa(msg->keyEvent[3], outBuf[3], 2);
                log_debug("    - modify dks para id(%d), toTopTrgLevel:%02d, toBotTrgLevel:%02d\r\n", dksId, toTopTrgLevel, toBotTrgLevel);
                for(uint8_t i=0; i<KEY_DFT_DKS_VIRTUAL_KEY_NUMBER; i++)
                {
                    log_debug("    - keyCode[%d]:ty:%02d, sty:%02d, c0:%02d, event:0x%02x(%08s)\r\n", i, 
                                    msg->keyCode[i].ty, msg->keyCode[i].sty, msg->keyCode[i].co, 
                                    msg->keyEvent[i], NRF_LOG_PUSH(outBuf[i]));
                    //NRF_LOG_FLUSH();
                }
                */
                log_debug("    - tick type:%d(%d-%d)\r\n", msg->tickType, msg->tickMin, msg->tickMax);
                #endif
                
                if (dks->para.toTopLevel == 0 && dks->para.toBotLevel == 0) /* add */
                {
                    //kh.p_qty->dksParaNumber += 1;
                    sk_la_lm_kc_info.sk.superkey_table[SKT_DKS].free_size    -= 1;
                    sk_la_lm_kc_info.sk.superkey_table[SKT_DKS].storage_mask |= 0x01<<dksId; //set bit
                }

                dks->para.tick.min   = msg->tickMin & 0x07FF;
                dks->para.tick.max   = msg->tickMax & 0x07FF;
                dks->para.tick.type  = msg->tickType;
                dks->para.toTopLevel = toTopTrgLevel;
                dks->para.toBotLevel = toBotTrgLevel;
                
                memcpy(dks->name,          msg->name,     sizeof(dks->name)/sizeof(dks->name[0]));
                memcpy(dks->para.keyCode,  msg->keyCode,  KEY_DFT_DKS_VIRTUAL_KEY_NUMBER*sizeof(dynamic_key_t));
                memcpy(dks->para.keyEvent, msg->keyEvent, KEY_DFT_DKS_VIRTUAL_KEY_NUMBER);
                
                db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
                respResult = CMD_RESP_SUCCEED;
                //respResult = dev_flash_write_user_msg_all()==true ? CMD_RESP_SUCCEED : CMD_RESP_FAIL;
            } break;
            default:
            {
                respResult = CMD_RESP_FAIL+7;
                #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
                log_debug("    - error 0x%02x (unkonw operate)\r\n", respResult);
                #endif
            } break;
        }
    }

    respBuffer[*respLenght] = respResult;
    *respLenght            += 1;
    #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
   // ret = cmd_respond_push_msg(instDevKeyscan->moduleUart.framePackReceive.cmd, instDevKeyscan->moduleUart.framePackReceive.subCmd, respResult, 0, NULL);

    return ret;
}
uint16_t ks_cmd_get_dks_para_table(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint16_t ret = true;
    ModuleUartCmdGetAdvTriggerDksMsg_t *msg         = (ModuleUartCmdGetAdvTriggerDksMsg_t *)reveBuffer; /* get cmd msg */
    ModuleUartCmdGetAdvTriggerDksRespMsg_t *respMsg = (ModuleUartCmdGetAdvTriggerDksRespMsg_t *)respBuffer;
    
    #if defined(LOG_SET_DKS) && (LOG_GET_DKS == 1)  
    uint8_t tmpCh[128];
    cmd_show_playload(__func__, "receive", (uint8_t *)msg, reveBufferSize);
    #endif
    
    uint8_t respResult = 0xFF, startKeyIndex = 0, EndKeyIndex = 0, sendKeyNum = 0, remainSendKeyNum = 0;
    uint8_t playloadLenght    = respBufferSize;
    uint8_t transmitKeyMaxNum = ((playloadLenght-2) / sizeof(respMsg->key[0])); /* - playload head  */
    uint8_t getWay            = msg->key.msg.way;
    //uint8_t ki                = 0;
    uint8_t keysw      = KEY_DFT_SHAFT;
    #warning "debug shaft, todo jay"
    
    if (reveBufferSize < 2)
    {
        respResult = 0xFF;
        #if defined(LOG_SET_DKS) && (LOG_GET_DKS == 1)  
        log_debug("    - error 0x%02x\r\n", respResult);
        #endif
    }
    else
    {
        switch(msg->operateType)
        {
            case GET_DKS_PARA_OPERATE_SINGLE:
            {
                sendKeyNum       = 1;
                remainSendKeyNum = 0;
                startKeyIndex    = msg->key.msg.pos;
                EndKeyIndex      = msg->key.msg.pos;
                respResult       = CMD_RESP_SUCCEED;
                #if defined(LOG_SET_DKS) && (LOG_GET_DKS == 1)  
                sprintf(tmpCh, "    - get dks para table id(%d) \r\n", msg->key.msg.pos);
                #endif
            } break;
            case GET_DKS_PARA_OPERATE_ALL: 
            {
                if(getWay == false)
                {
                    kh.p_gst->dksRespParaTableLen = KEY_DFT_DKS_PARA_TABLE_MAX;
                }
                #if defined(LOG_SET_DKS) && (LOG_GET_DKS == 1)  
                log_debug("    - get all dks para table (%d) ", KEY_DFT_DKS_PARA_TABLE_MAX);
                #endif
                if(kh.p_gst->dksRespParaTableLen >= transmitKeyMaxNum)
                {
                    sendKeyNum       = transmitKeyMaxNum;
                    remainSendKeyNum = kh.p_gst->dksRespParaTableLen - transmitKeyMaxNum;
                    startKeyIndex    = KEY_DFT_DKS_PARA_TABLE_MAX - kh.p_gst->dksRespParaTableLen;
                    EndKeyIndex      = startKeyIndex + sendKeyNum - 1;
                    
                    #if defined(LOG_SET_DKS) && (LOG_GET_DKS == 1)  
                    sprintf(tmpCh, "send(%d) reamin(%d) (%02d-%02d) \r\n", sendKeyNum, remainSendKeyNum, startKeyIndex, EndKeyIndex);
                    #endif
                    kh.p_gst->dksRespParaTableLen = remainSendKeyNum;
                }
                else
                {
                    sendKeyNum       = kh.p_gst->dksRespParaTableLen;
                    remainSendKeyNum = 0;
                    startKeyIndex    = KEY_DFT_DKS_PARA_TABLE_MAX - kh.p_gst->dksRespParaTableLen;
                    EndKeyIndex      = startKeyIndex + sendKeyNum - 1;
                    
                    #if defined(LOG_SET_DKS) && (LOG_GET_DKS == 1)  
                    sprintf(tmpCh, "send(%d) reamin(%d) end (%02d-%02d) \r\n", sendKeyNum, remainSendKeyNum, startKeyIndex, EndKeyIndex);
                    #endif
                    kh.p_gst->dksRespParaTableLen = KEY_DFT_DKS_PARA_TABLE_MAX;
                }
                respResult = CMD_RESP_SUCCEED;
            } break;
            /*
            case GET_DKS_PARA_TABLE_SIZE: 
            {
                sprintf(tmpCh, "    - get dks para table size\r\n");
                respResult = 0xFE;
            }
            break;
            */
            case GET_DKS_PARA_OPERATE_BE_OPEN:
            default: 
            {
                respResult = 0xFF;
                #if defined(LOG_SET_DKS) && (LOG_GET_DKS == 1)  
                log_debug("    - error 0x%02x opeate unkonw\r\n", respResult);
                #endif
            } break;
        }
        #if defined(LOG_SET_DKS) && (LOG_GET_DKS == 1)  
        log_debug("%s", (tmpCh));
        #endif
    }
    
    /* ------------------------------ resp msg ------------------------------ */
    if(respResult==0xFF)
    {
        respBuffer[*respLenght] = respResult;
        *respLenght            += 1;
        #if defined(LOG_SET_DKS) && (LOG_GET_DKS == 1)  
        cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
        #endif
        return respResult;
    }
    /*
    else if(respResult==0xFE)
    {
        respBuffer[*respLenght] = KEY_DFT_DKS_PARA_TABLE_MAX;
        *respLenght            += 1;
        cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
        return respResult;
    }
*/
    
 //   buffer[0] = remainSendKeyNum;
 //   buffer[1] = sendKeyNum;
 //   buffer[2] = startKeyIndex;
 //   buffer[3] = EndKeyIndex;

   // ret = cmd_respond_push_msg(instDevKeyscan->moduleUart.framePackReceive.cmd, instDevKeyscan->moduleUart.framePackReceive.subCmd, respResult, lenght, buffer);
   // return ret;
    
    
   // if(buffLenght!=4)
     //   return false;


   // int ret = true;
    uint16_t respMsgLenght     = sizeof(ModuleUartCmdGetAdvTriggerDksRespMsg_t);
    uint8_t respRtKeyEndPos    = 0;
    uint8_t respRtKeyStartPos  = 0;
    uint8_t j                  = 0;
    
    /* fill playload */
    respMsg->remainKeyNumber   = remainSendKeyNum;
    respMsg->transmitKeyNumber = sendKeyNum;
    respRtKeyEndPos            = EndKeyIndex;
    respRtKeyStartPos          = startKeyIndex;
    
    respMsgLenght = 2;
    for(uint8_t id=respRtKeyStartPos; id<=respRtKeyEndPos; id++)
    {
      //  key_dks_t *dksPara                     = &kh.p_qty->dksParaTable[i];
        dks_t *dks              = &sk_la_lm_kc_info.sk.dks_table[id];
        float distToTopTrgLevel = LEVEL_TO_DIST(dks->para.toTopLevel, sw_config_table[keysw].dksAcc);
        float distToBotTrgLevel = LEVEL_TO_DIST(dks->para.toBotLevel, sw_config_table[keysw].dksAcc);
       
       // sprintf(tmpCh, "toTopTrgLev:%0.2fmm, toBotTrgLev:%0.2fmm", distToTopTrgLevel, distToBotTrgLevel);
       // log_debug("    - dist: %s\n", tmpCh);
       
        respMsg->key[j].msg.id                 = id;
        respMsg->key[j].msg.toTopTrgLevel      = HIVE_TXDA_001MM(distToTopTrgLevel);
        respMsg->key[j].msg.toBotTrgLevel      = HIVE_TXDA_001MM(distToBotTrgLevel);
        respMsg->key[j].msg.tickType           = dks->para.tick.type;
        respMsg->key[j].msg.tickMin            = dks->para.tick.min;
        respMsg->key[j].msg.tickMax            = dks->para.tick.max;

        memcpy(respMsg->key[j].msg.name,     dks->name,          ARRAY_SIZE(dks->name));
        memcpy(respMsg->key[j].msg.keyCode,  dks->para.keyCode,  KEY_DFT_DKS_VIRTUAL_KEY_NUMBER*sizeof(dynamic_key_t));
        memcpy(respMsg->key[j].msg.keyEvent, dks->para.keyEvent, ARRAY_SIZE(dks->para.keyEvent));
        
        respMsgLenght += sizeof(respMsg->key[0]);
        j++;
    }

    *respLenght = respMsgLenght;
    ///* send respond */
   // ret = cmd_respond_send(cmdCode, cmdSubCode, &respMsg, respMsgLenght);
    #if defined(LOG_SET_DKS) && (LOG_GET_DKS == 1)  
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
    return ret;
}

uint16_t ks_cmd_get_dks_head(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint16_t ret = true;
    *respLenght = sizeof(skh_t);
    memcpy(respBuffer, &sk_la_lm_kc_info.sk.superkey_table[SKT_DKS], *respLenght);
    #if defined(LOG_SET_DKS) && (LOG_GET_DKS == 1)  
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
    return ret;
}

uint16_t ks_cmd_get_dks_tick(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint16_t ret = true;
    ModuleUartCmdGetAdvTriggerDksTickMsg_t *respMsg = (ModuleUartCmdGetAdvTriggerDksTickMsg_t *)respBuffer;
    *respLenght                                     = sizeof(ModuleUartCmdGetAdvTriggerDksTickMsg_t);

    respMsg->tickType = kh.p_qty->dksTick.type;
    respMsg->tickMin = kh.p_qty->dksTick.min;
    respMsg->tickMax = kh.p_qty->dksTick.max;
    
    #if defined(LOG_SET_DKS) && (LOG_GET_DKS == 1)  
    log_debug("dks get tick: %s: %02d~%02d tick\r\n",  
                respMsg->tickType==TICK_STATIC ? "static" 
                : respMsg->tickType==TICK_RANDOM ? "random" : "unkonw", 
                respMsg->tickMin, respMsg->tickMax);
    
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
    return ret;
}

uint16_t ks_cmd_set_dks_tick(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint8_t respResult = CMD_RESP_SUCCEED;
    uint8_t tickMin    = 0;
    uint8_t tickMax    = 0;

    ModuleUartCmdSetAdvTriggerDksTickMsg_t *msg = (ModuleUartCmdSetAdvTriggerDksTickMsg_t *)reveBuffer;
    #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
    cmd_show_playload(__func__, "receive", (uint8_t *)reveBuffer, reveBufferSize);
    #endif
    
    dks_tick_type tickType = (dks_tick_type)msg->tickType;
    if(tickType==TICK_STATIC)
    {
        tickMin = msg->tickMin;
        tickMax = 0;
        //feature_dks_set_tick_para(tickType, tickMin, tickMax); //todo
        #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
        log_debug("dks set tick: static: %02d tick\r\n", tickMin);
        #endif
        db_update(DB_SYS, UPDATE_LATER);
    }
    else if(tickType==TICK_RANDOM)
    {
        tickMin = MIN(msg->tickMin, msg->tickMax);
        tickMax = MAX(msg->tickMin, msg->tickMax);
        //feature_dks_set_tick_para(tickType, tickMin, tickMax); //todo
        #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
        log_debug("dks set tick: random: %02d~%02d tick\r\n", tickMin, tickMax);
        #endif
        db_update(DB_SYS, UPDATE_LATER);
    }
    else
    {
        respResult = CMD_RESP_FAIL;
        #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
        log_debug("dks set tick error\r\n");
        #endif
    }

    respBuffer[*respLenght] = respResult;
    *respLenght            += 1;
    #if defined(LOG_SET_DKS) && (LOG_SET_DKS == 1)  
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
    return respResult;
}
#endif

uint16_t ks_cmd_set_socd(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint16_t ret = CMD_RESP_FAIL;
    ModuleUartCmdSetSnapShop_Msg_t *receive = (ModuleUartCmdSetSnapShop_Msg_t *)reveBuffer;
    socd_t *snap_team= sk_la_lm_kc_info.sk.socd;
    sk_t *sk_team = &sk_la_lm_kc_info.sk;
    
#if defined(LOG_SET_ST) && (LOG_SET_ST == 1)  
    log_debug("ks_cmd_set_socd=%d\r\n", receive->SnapType);
#endif

    switch (receive->SnapType)
    {
    case SET_SNAP_OPERATE_DELETE:
        if ((snap_team[receive->id_bit].ki[0] > KEYBOARD_NUMBER) || (snap_team[receive->id_bit].ki[1] > KEYBOARD_NUMBER))
        {
#if defined(LOG_SET_ST) && (LOG_SET_ST == 1)  
            log_debug("ks_cmd_set_socd err\n");
#endif
            break;
        }
        
        sk_la_lm_kc_info.sk.superkey_table[SKT_SOCD].storage_mask &= ~((1 << receive->id_bit));
        sk_la_lm_kc_info.sk.superkey_table[SKT_SOCD].free_size++;
        
        key_st[snap_team[receive->id_bit].ki[0]].socd_ki     = 0xFF;
        key_st[snap_team[receive->id_bit].ki[1]].socd_ki     = 0xFF;
        key_st[snap_team[receive->id_bit].ki[0]].socd_ki_abs = 0xFF;
        key_st[snap_team[receive->id_bit].ki[1]].socd_ki_abs = 0xFF;
        key_st[snap_team[receive->id_bit].ki[0]].socd_idx    = 0xFF;
        key_st[snap_team[receive->id_bit].ki[1]].socd_idx    = 0xFF;
        key_st[snap_team[receive->id_bit].ki[0]].tap_type   &= ~TAP_TYPE_SOCD;
        key_st[snap_team[receive->id_bit].ki[1]].tap_type   &= ~TAP_TYPE_SOCD;
        
        snap_team[receive->id_bit].ki[0] = 0xFF;
        snap_team[receive->id_bit].ki[1] = 0xFF;
        snap_team[receive->id_bit].mode  = 0;
        snap_team[receive->id_bit].sw    = 0;
        
        #if defined(LOG_SET_ST) && (LOG_SET_ST == 1)  
        log_debug("ks_cmd_set_socd;del=%d,%d,%d\r\n",receive->id_bit,snap_team[receive->id_bit].ki[0],snap_team[receive->id_bit].ki[1]);
        #endif

        db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
        ret = CMD_RESP_SUCCEED;
        break;
    case SET_SNAP_OPERATE_ADD:
        if ((receive->id_bit > 7) || (receive->data.ki[0] > KEYBOARD_NUMBER) || (receive->data.ki[1] > KEYBOARD_NUMBER))
        {
            break;
        }

        for (uint8_t i = 0; i < sk_team->superkey_table[SKT_RS].total_size; i++)
        {
            if (sk_team->superkey_table[SKT_RS].storage_mask & (1 << i))
            {
                if ((sk_team->rs[i].ki[0] == receive->data.ki[0]) || (sk_team->rs[i].ki[0] == receive->data.ki[1])
                    || (sk_team->rs[i].ki[1] == receive->data.ki[0]) || (sk_team->rs[i].ki[1] == receive->data.ki[1]))
                {
                    respBuffer[(*respLenght)++] = ret;
                    return ret;
                }
            }
        }

        sk_la_lm_kc_info.sk.superkey_table[SKT_SOCD].storage_mask |= (1 << receive->id_bit);
        if (sk_la_lm_kc_info.sk.superkey_table[SKT_SOCD].free_size)
        {
            sk_la_lm_kc_info.sk.superkey_table[SKT_SOCD].free_size--;
        }
        
    case SET_SNAP_OPERATE_EDIT:
        if (( receive->data.ki[0] > KEYBOARD_NUMBER) || ( receive->data.ki[1] > KEYBOARD_NUMBER))
        {
#if defined(LOG_SET_ST) && (LOG_SET_ST == 1)  
            log_debug("ks_cmd_set_socd err\n");
#endif
            break;
        }
        
        for (uint8_t i = 0; i < sk_team->superkey_table[SKT_RS].total_size; i++)
        {
            if (sk_team->superkey_table[SKT_RS].storage_mask & (1 << i))
            {
                if ((sk_team->rs[i].ki[0] == receive->data.ki[0]) || (sk_team->rs[i].ki[0] == receive->data.ki[1])
                    || (sk_team->rs[i].ki[1] == receive->data.ki[0]) || (sk_team->rs[i].ki[1] == receive->data.ki[1]))
                {
                    respBuffer[(*respLenght)++] = ret;
                    return ret;
                }
            }
        }
        
        key_st[snap_team[receive->id_bit].ki[0]].socd_ki     = 0xFF;
        key_st[snap_team[receive->id_bit].ki[1]].socd_ki     = 0xFF;
        key_st[snap_team[receive->id_bit].ki[0]].socd_ki_abs = 0xFF;
        key_st[snap_team[receive->id_bit].ki[1]].socd_ki_abs = 0xFF;
        key_st[snap_team[receive->id_bit].ki[0]].tap_type   &= ~TAP_TYPE_SOCD;
        key_st[snap_team[receive->id_bit].ki[1]].tap_type   &= ~TAP_TYPE_SOCD;
        
        snap_team[receive->id_bit].ki[0] = receive->data.ki[0];
        snap_team[receive->id_bit].ki[1] = receive->data.ki[1];
        snap_team[receive->id_bit].mode  = receive->data.mode;
        snap_team[receive->id_bit].sw    = receive->data.sw;

        key_st[receive->data.ki[0]].socd_idx = receive->id_bit;
        key_st[receive->data.ki[1]].socd_idx = receive->id_bit;
        key_st[receive->data.ki[0]].socd_ki  = receive->data.ki[1];
        key_st[receive->data.ki[1]].socd_ki  = receive->data.ki[0];
        key_st[receive->data.ki[0]].tap_type |= TAP_TYPE_SOCD;
        key_st[receive->data.ki[1]].tap_type |= TAP_TYPE_SOCD;
        
        if (receive->data.mode == SOCD_MODE_ABSOLUTE_1)
        {
            key_st[receive->data.ki[0]].socd_ki_abs = receive->data.ki[0];
            key_st[receive->data.ki[1]].socd_ki_abs = receive->data.ki[0];
        }
        else if (receive->data.mode == SOCD_MODE_ABSOLUTE_2)
        {
            key_st[receive->data.ki[0]].socd_ki_abs = receive->data.ki[1];
            key_st[receive->data.ki[1]].socd_ki_abs = receive->data.ki[1];
        }
        
#if defined(LOG_SET_ST) && (LOG_SET_ST == 1)  
        log_debug("ks_cmd_set_socd;t=%d,%d,%d,%d\r\n",receive->SnapType,receive->id_bit,snap_team[receive->id_bit].ki[0],snap_team[receive->id_bit].ki[1]);
#endif

        db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
        ret = CMD_RESP_SUCCEED;
        break;

    default:
        break;
    }

    respBuffer[(*respLenght)++] = ret;

#if defined(LOG_SET_ST) && (LOG_SET_ST == 1)
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
#endif
    return ret;
}

uint16_t ks_cmd_get_socd(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint16_t ret = true;
    socd_t *snap_team = sk_la_lm_kc_info.sk.socd;
    
    for (uint8_t i = 0; i < sk_la_lm_kc_info.sk.superkey_table[SKT_SOCD].total_size; i++)
    {
        if (sk_la_lm_kc_info.sk.superkey_table[SKT_SOCD].storage_mask & (1 << i))
        {
        }
        else
        {
            sk_la_lm_kc_info.sk.socd[i].ki[0] = 0xff;
            sk_la_lm_kc_info.sk.socd[i].ki[1] = 0xff;
            sk_la_lm_kc_info.sk.socd[i].mode  = 0;
            sk_la_lm_kc_info.sk.socd[i].sw    = 0;
        }
        respBuffer[(*respLenght)++] = snap_team[i].ki[0];
        respBuffer[(*respLenght)++] = snap_team[i].ki[1];
        respBuffer[(*respLenght)++] = snap_team[i].mode;
        respBuffer[(*respLenght)++] = snap_team[i].sw;
    }
#if defined(LOG_SET_ST) && (LOG_SET_ST == 1)
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
#endif
    return ret;
}

uint16_t ks_cmd_set_ht(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint16_t ret = CMD_RESP_FAIL;
    uint8_t ki0;
    uint8_t ki1;
    uint8_t kibind;
    ht_t *ht = NULL;
    ModuleUartCmdSetHyperTap_Msg_t *receive = (ModuleUartCmdSetHyperTap_Msg_t *)reveBuffer;
    ht_t *hyper_team= sk_la_lm_kc_info.sk.ht;
    
#if defined(LOG_SET_HT) && (LOG_SET_HT == 1)  
    log_debug("ks_cmd_set_ht=%d\r\n", receive->SnapType);
#endif

    switch (receive->SnapType)
    {
    case SET_SNAP_OPERATE_DELETE:
        if ((hyper_team[receive->id_bit].ki > KEYBOARD_NUMBER))
        {
#if defined(LOG_SET_HT) && (LOG_SET_HT == 1)  
            log_debug("ks_cmd_set_ht err\n");
#endif
            return ret;
        }
        
        sk_la_lm_kc_info.sk.superkey_table[SKT_HT].storage_mask &= ~((1 << receive->id_bit));
        sk_la_lm_kc_info.sk.superkey_table[SKT_HT].free_size++;
        

        //clear
        ht = &hyper_team[receive->id_bit];
        ki0 = ht->ki;
        ki1 = 0xff;
        kibind = ki0;
        if ((ht->kc.ty == KCT_KB) && (kc_to_ki[ht->kc.co] != -1))
        {
            ki1 = kc_to_ki[ht->kc.co];
            kibind = ki1;
        }
        memset(&key_st[kibind].drt_kc, 0, sizeof(kc_t));
        key_st[kibind].drt_ki[0] = 0xFF;
        key_st[kibind].drt_ki[1] = 0xFF;
        key_st[kibind].tap_type &= ~TAP_TYPE_DRT;

        hyper_team[receive->id_bit].ki = 0xFF;
        memset(&hyper_team[receive->id_bit].kc, 0, sizeof(kc_t));

        #if defined(LOG_SET_ST) && (LOG_SET_ST == 1)  
        log_debug("ks_cmd_set_ht;del=%d,%d,%d\r\n",receive->id_bit,hyper_team[receive->id_bit].ki[0],hyper_team[receive->id_bit].ki[1]);
        #endif

        db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
        ret = CMD_RESP_SUCCEED;
        break;
    case SET_SNAP_OPERATE_ADD:
        if (receive->id_bit > 7)
        {
            return ret;
        }
        sk_la_lm_kc_info.sk.superkey_table[SKT_HT].storage_mask |= (1 << receive->id_bit);
        if (sk_la_lm_kc_info.sk.superkey_table[SKT_HT].free_size)
        {
            sk_la_lm_kc_info.sk.superkey_table[SKT_HT].free_size--;
        }
        
    case SET_SNAP_OPERATE_EDIT:
        if (( receive->data.ki > KEYBOARD_NUMBER))
        {
#if defined(LOG_SET_HT) && (LOG_SET_HT == 1)  
            log_debug("ks_cmd_set_ht err\n");
#endif
            return ret;
        }

        //clear
        ht = &hyper_team[receive->id_bit];
        ki0 = ht->ki;
        ki1 = 0xff;
        kibind = ki0;
        if ((ht->kc.ty == KCT_KB) && (kc_to_ki[ht->kc.co] != -1))
        {
            ki1 = kc_to_ki[ht->kc.co];
            kibind = ki1;
        }
        memset(&key_st[kibind].drt_kc, 0, sizeof(kc_t));
        key_st[kibind].drt_ki[0] = 0xFF;
        key_st[kibind].drt_ki[1] = 0xFF;
        key_st[kibind].tap_type &= ~TAP_TYPE_DRT;

        //add
        ki0 = receive->data.ki;
        ki1 = 0xff;
        kibind = ki0;
        if ((receive->data.kc.ty == KCT_KB) && (kc_to_ki[receive->data.kc.co] != -1))
        {
            ki1 = kc_to_ki[receive->data.kc.co];
            kibind = ki1;
        }
        key_st[kibind].drt_kc    = receive->data.kc;
        key_st[kibind].drt_ki[0] = ki0;
        key_st[kibind].drt_ki[1] = ki1;
        key_st[kibind].tap_type |= TAP_TYPE_DRT;

        hyper_team[receive->id_bit].ki  = receive->data.ki;
        hyper_team[receive->id_bit].kc  = receive->data.kc;
        
#if defined(LOG_SET_HT) && (LOG_SET_HT == 1)  
        log_debug("ks_cmd_set_ht;t=%d,%d,%d,%d\r\n",receive->SnapType,receive->id_bit,hyper_team[receive->id_bit].ki[0],hyper_team[receive->id_bit].ki[1]);
#endif

        db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
        ret = CMD_RESP_SUCCEED;
        break;

    default:
        break;
    }

    respBuffer[(*respLenght)++] = ret;

#if defined(LOG_SET_HT) && (LOG_SET_HT == 1)
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
#endif
    return ret;
}

uint16_t ks_cmd_get_ht(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint16_t ret = true;
    ht_t *hyper_team = sk_la_lm_kc_info.sk.ht;
    
    *respLenght = 0;
    for (uint8_t i = 0; i < sk_la_lm_kc_info.sk.superkey_table[SKT_HT].total_size; i++)
    {
        if (sk_la_lm_kc_info.sk.superkey_table[SKT_HT].storage_mask & (1 << i))
        {
        }
        else
        {
            sk_la_lm_kc_info.sk.ht[i].ki = 0xff;
            memset(&sk_la_lm_kc_info.sk.ht[i].kc, 0, sizeof(kc_t));
        }
        memcpy(&respBuffer[(*respLenght)], &hyper_team[i], sizeof(ht_t));
        (*respLenght) += sizeof(ht_t);
    }
#if defined(LOG_SET_HT) && (LOG_SET_HT == 1)
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
#endif
    return ret;
}

uint16_t ks_cmd_set_rs(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint16_t ret = CMD_RESP_FAIL;
    ModuleUartCmdSetRs_Msg_t *receive = (ModuleUartCmdSetRs_Msg_t *)reveBuffer;
    rs_t *rs_team= sk_la_lm_kc_info.sk.rs;
    sk_t *sk_team = &sk_la_lm_kc_info.sk;
    
#if (LOG_SET_RS == 1)  
    log_debug("ks_cmd_set_rs=%d\r\n", receive->SnapType);
#endif

    switch (receive->SnapType)
    {
    case SET_SNAP_OPERATE_DELETE:
        if ((rs_team[receive->id_bit].ki[0] > KEYBOARD_NUMBER) || (rs_team[receive->id_bit].ki[1] > KEYBOARD_NUMBER))
        {
#if (LOG_SET_RS == 1)  
            log_debug("ks_cmd_set_rs err\n");
#endif
            break;
        }
        
        sk_la_lm_kc_info.sk.superkey_table[SKT_RS].storage_mask &= ~((1 << receive->id_bit));
        sk_la_lm_kc_info.sk.superkey_table[SKT_RS].free_size++;
        
        key_st[rs_team[receive->id_bit].ki[0]].socd_ki  = 0xFF;
        key_st[rs_team[receive->id_bit].ki[1]].socd_ki  = 0xFF;
        key_st[rs_team[receive->id_bit].ki[0]].tap_type &= ~TAP_TYPE_RS;
        key_st[rs_team[receive->id_bit].ki[1]].tap_type &= ~TAP_TYPE_RS;
        
        rs_team[receive->id_bit].ki[0]                  = 0xFF;
        rs_team[receive->id_bit].ki[1]                  = 0xFF;
        
        #if (LOG_SET_RS == 1)  
        log_debug("ks_cmd_set_rs;del=%d,%d,%d\r\n",receive->id_bit,rs_team[receive->id_bit].ki[0],rs_team[receive->id_bit].ki[1]);
        #endif

        db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
        ret = CMD_RESP_SUCCEED;
        break;
    case SET_SNAP_OPERATE_ADD:
        if ((receive->id_bit > 7) || ( receive->data.ki[0] > KEYBOARD_NUMBER) || ( receive->data.ki[1] > KEYBOARD_NUMBER))
        {
            break;
        }

        for (uint8_t i = 0; i < sk_team->superkey_table[SKT_SOCD].total_size; i++)
        {
            if (sk_team->superkey_table[SKT_SOCD].storage_mask & (1 << i))
            {
                if ((sk_team->socd[i].ki[0] == receive->data.ki[0]) || (sk_team->socd[i].ki[0] == receive->data.ki[1])
                    || (sk_team->socd[i].ki[1] == receive->data.ki[0]) || (sk_team->socd[i].ki[1] == receive->data.ki[1]))
                {
                    respBuffer[(*respLenght)++] = ret;
                    return ret;
                }
            }
        }
        
        sk_team->superkey_table[SKT_RS].storage_mask |= (1 << receive->id_bit);
        if (sk_team->superkey_table[SKT_RS].free_size)
        {
            sk_team->superkey_table[SKT_RS].free_size--;
        }
        
    case SET_SNAP_OPERATE_EDIT:
        if (( receive->data.ki[0] > KEYBOARD_NUMBER) || ( receive->data.ki[1] > KEYBOARD_NUMBER))
        {
#if (LOG_SET_RS == 1)  
            log_debug("ks_cmd_set_rs err\n");
#endif
            break;
        }

        for (uint8_t i = 0; i < sk_team->superkey_table[SKT_SOCD].total_size; i++)
        {
            if (sk_team->superkey_table[SKT_SOCD].storage_mask & (1 << i))
            {
                if ((sk_team->socd[i].ki[0] == receive->data.ki[0]) || (sk_team->socd[i].ki[0] == receive->data.ki[1])
                    || (sk_team->socd[i].ki[1] == receive->data.ki[0]) || (sk_team->socd[i].ki[1] == receive->data.ki[1]))
                {
                    respBuffer[(*respLenght)++] = ret;
                    return ret;
                }
            }
        }
        
        key_st[rs_team[receive->id_bit].ki[0]].socd_ki  = 0xFF;
        key_st[rs_team[receive->id_bit].ki[1]].socd_ki  = 0xFF;
        key_st[rs_team[receive->id_bit].ki[0]].tap_type &= ~TAP_TYPE_RS;
        key_st[rs_team[receive->id_bit].ki[1]].tap_type &= ~TAP_TYPE_RS;
        
        rs_team[receive->id_bit].ki[0]       = receive->data.ki[0];
        rs_team[receive->id_bit].ki[1]       = receive->data.ki[1];
        key_st[receive->data.ki[0]].socd_ki  = receive->data.ki[1];
        key_st[receive->data.ki[1]].socd_ki  = receive->data.ki[0];
        key_st[receive->data.ki[0]].tap_type |= TAP_TYPE_RS;
        key_st[receive->data.ki[1]].tap_type |= TAP_TYPE_RS;
#if (LOG_SET_RS == 1)  
        log_debug("ks_cmd_set_rs;t=%d,%d,%d,%d\r\n",receive->SnapType,receive->id_bit,rs_team[receive->id_bit].ki[0],rs_team[receive->id_bit].ki[1]);
#endif

        db_update(DB_SK_LA_LM_KC, UPDATE_LATER);
        ret = CMD_RESP_SUCCEED;
        break;

    default:
        break;
    }

    respBuffer[(*respLenght)++] = ret;

#if (LOG_SET_RS == 1)
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
#endif
    return ret;
}

uint16_t ks_cmd_get_rs(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint16_t ret = true;
    rs_t *rs_team = sk_la_lm_kc_info.sk.rs;
    
    for (uint8_t i = 0; i < sk_la_lm_kc_info.sk.superkey_table[SKT_RS].total_size; i++)
    {
        if (sk_la_lm_kc_info.sk.superkey_table[SKT_RS].storage_mask & (1 << i))
        {
        }
        else
        {
            sk_la_lm_kc_info.sk.rs[i].ki[0] = 0xff;
            sk_la_lm_kc_info.sk.rs[i].ki[1] = 0xff;
        }
        respBuffer[(*respLenght)++] = rs_team[i].ki[0];
        respBuffer[(*respLenght)++] = rs_team[i].ki[1];
    }
#if (LOG_SET_RS == 1)
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
#endif
    return ret;
}

bool key_cmd_set_rt_en(void)
{
    bool en = true;
    if ((kh.p_attr + 0)->rt_en == true)
    {
        en = false;
    }
    
    for (uint8_t i = 0; i < KEYBOARD_NUMBER ; i++)
    {
        (kh.p_attr + i)->rt_en = en;
    }
    db_update(DB_SYS, UPDATE_LATER);
    return en;
}

bool key_cmd_set_rt_continue_en(void)
{
    bool en = true;
    if ((kh.p_attr + 0)->rtIsContinue == true)
    {
        en = false;
    }
    
    for (uint8_t i = 0; i < KEYBOARD_NUMBER ; i++)
    {
        (kh.p_attr + i)->rtIsContinue = en;
    }
    db_update(DB_SYS, UPDATE_LATER);
    return en;
}

uint16_t key_cmd_set_tuning(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint8_t respResult     = CMD_RESP_FAIL;
    tuning_en_t *respMsg      = (tuning_en_t *)reveBuffer;
    uint16_t respMsgLenght = sizeof(tuning_en_t);
    tuning_e en = respMsg->tuning;
    (void)respBufferSize;
    (void)reveBufferSize;
    (void)reveBuffer;
    (void)respBuffer;

    #if defined(LOG_SET_TUN) && (LOG_SET_TUN == 1)  
    cmd_show_playload(__func__, "receive", (uint8_t *)reveBuffer, reveBufferSize);
    log_debug("    - tuning:%s\n", respMsg->tuning==TUNING_OFF    ? "OFF" \
                                      : respMsg->tuning==TUNING_ON ? "ON"  \
                                      : "unkonw");
    #endif

    respResult  = CMD_RESP_SUCCEED;
    *respLenght = respMsgLenght;
    
    
    cali_tuning_set_state(en);
    #if defined(LOG_SET_TUN) && (LOG_SET_TUN == 1)  
    NRF_LOG_RAW_INFO("==%d,%d,%d,%d,%d\n", respBuffer[0], respMsg->tuning,TUNING_ON,TUNING_OFF,en);
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
    return respResult;
}


uint16_t key_cmd_get_tuning(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint8_t respResult     = CMD_RESP_FAIL;
    tuning_en_t *respMsg      = (tuning_en_t *)respBuffer;
    uint16_t respMsgLenght = sizeof(tuning_en_t);
    (void)respBufferSize;
    (void)reveBufferSize;
    (void)reveBuffer;

    #if defined(LOG_SET_TUN) && (LOG_GET_TUN == 1)  
    cmd_show_playload(__func__, "receive", (uint8_t *)reveBuffer, reveBufferSize);
    log_debug("    - tuning:%s\n", respMsg->tuning==TUNING_OFF    ? "OFF" \
                                      : respMsg->tuning==TUNING_ON ? "ON"  \
                                      : "unkonw");
    #endif

    respMsg->tuning = cali_tuning_get_state();
    respResult  = CMD_RESP_SUCCEED;
    *respLenght = respMsgLenght;
    #if defined(LOG_SET_TUN) && (LOG_GET_TUN == 1)  
    log_debug("key_cmd_get_tuning=%d\n",respMsg->tuning);
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
    return respResult;
}

void key_shaft_change_acc_conv(uint8_t ki, key_sw_type_e newShaftType)
{
    //key shaft change acc
    uint16_t geneTrgLevel   = ACC_BETW_CONV(kh.p_attr[ki].triggerlevel, KEY_GENERAL_TRG_ACC(ki), sw_config_table[newShaftType].generalTrgAcc);
    uint16_t geneRaiseLevel = ACC_BETW_CONV(kh.p_attr[ki].raiseLevel, KEY_GENERAL_TRG_ACC(ki), sw_config_table[newShaftType].generalTrgAcc);
    uint16_t rtPressSens    = ACC_BETW_CONV(kh.p_attr[ki].rtPressSensitive, KEY_RT_ACC(ki), sw_config_table[newShaftType].rtSensAcc);
    uint16_t rtRaiseSens    = ACC_BETW_CONV(kh.p_attr[ki].rtRaiseSensitive, KEY_RT_ACC(ki), sw_config_table[newShaftType].rtSensAcc);
    uint16_t deadTopLevel   = ACC_BETW_CONV(kh.p_attr[ki].deadbandTop, KEY_DEADBAND_ACC(ki), sw_config_table[newShaftType].deadAcc);
    uint16_t deadBotLevel   = ACC_BETW_CONV(kh.p_attr[ki].deadbandBottom, KEY_DEADBAND_ACC(ki), sw_config_table[newShaftType].deadAcc);
    
    #if defined(MODULE_LOG_ENABLED) && (MODULE_LOG_ENABLED == 1)
    char tmpCh[128] = {"\0"};
    sprintf(tmpCh, "old shaft: %s(%02d,%02d,%02d,%02d,%02d,%02d) ", 
                                      key_cali.sw[ki] == KEY_SHAFT_TYPE_GATERON_WHITE ? "GDL_WHITE"
                                    : key_cali.sw[ki] == KEY_SHAFT_TYPE_GATERON_JADE  ? "GDL_JADE"
                                    : key_cali.sw[ki] == KEY_SHAFT_TYPE_KAILH_GREEN  ? "KH_YELLOW" 
                                    : key_cali.sw[ki] == KEY_SHAFT_TYPE_TTC_WCW  ? "TTC_WCW"
                                    : "UNKNOWN", 
                                    kh.p_attr[ki].triggerlevel, kh.p_attr[ki].raiseLevel, 
                                    kh.p_attr[ki].rtPressSensitive, kh.p_attr[ki].rtRaiseSensitive, 
                                    kh.p_attr[ki].deadbandTop,kh.p_attr[ki].deadbandBottom);
    DBG_PRINTF("    - [%02d]: %s \n", ki, (tmpCh));

    sprintf(tmpCh, "change shaft : %s(%02d,%02d,%02d,%02d,%02d,%02d) ", 
                                  newShaftType == KEY_SHAFT_TYPE_GATERON_WHITE ? "GDL_WHITE"
                                : newShaftType == KEY_SHAFT_TYPE_GATERON_JADE  ? "GDL_JADE"
                                : newShaftType == KEY_SHAFT_TYPE_KAILH_GREEN  ? "KH_YELLOW" 
                                : newShaftType == KEY_SHAFT_TYPE_TTC_WCW  ? "TTC_WCW" 
                                : "UNKNOWN",
                                geneTrgLevel,geneRaiseLevel,rtPressSens,rtRaiseSens,deadTopLevel,deadBotLevel);
    DBG_PRINTF("    - [%02d]: %s \n", ki, (tmpCh));
    #endif

    // change shaft
    key_cali.sw[ki]            = newShaftType;
    kh.p_attr[ki].triggerlevel     = geneTrgLevel;
    kh.p_attr[ki].raiseLevel       = geneRaiseLevel;
    kh.p_attr[ki].rtPressSensitive = rtPressSens;
    kh.p_attr[ki].rtRaiseSensitive = rtRaiseSens;
    kh.p_attr[ki].deadbandTop      = deadTopLevel;
    kh.p_attr[ki].deadbandBottom   = deadBotLevel;

    if (   LEVEL_TO_DIST(geneTrgLevel, KEY_GENERAL_TRG_ACC(ki)) > KEY_REAL_TRAVEL(ki)
        || LEVEL_TO_DIST(geneRaiseLevel, KEY_GENERAL_TRG_ACC(ki)) > KEY_REAL_TRAVEL(ki) )
    {
        DBG_PRINTF("    - general para error\n");
        uint8_t triggerlevel  = KEY_DFT_GENERAL_TRG_PRESS_LEVEL(newShaftType);
        uint8_t raiseLevel    = (triggerlevel + KEY_DFT_GENERAL_TRG_RAISE_OFFSET_LEVEL(newShaftType)) < 0 ? 0 : (triggerlevel + KEY_DFT_GENERAL_TRG_RAISE_OFFSET_LEVEL(newShaftType));
        uint8_t generalMode   = KEY_GENRAL_MODE_STROKE_SAME;
        fix_set_para(ki, triggerlevel, raiseLevel, generalMode);
    }
        
    if(    rtPressSens < DIST_TO_LEVEL(KEY_RT_SENS_MIN(ki), KEY_RT_ACC(ki)) 
        || rtPressSens > DIST_TO_LEVEL(KEY_RT_SENS_MAX(ki), KEY_RT_ACC(ki))
        || rtRaiseSens < DIST_TO_LEVEL(KEY_RT_SENS_MIN(ki), KEY_RT_ACC(ki))
        || rtRaiseSens > DIST_TO_LEVEL(KEY_RT_SENS_MAX(ki), KEY_RT_ACC(ki)) )
    {
        DBG_PRINTF("    - rt para error\n");
        
        rt_set_para(ki, kh.p_attr[ki].rt_en , kh.p_attr[ki].rtIsContinue, \
                        KEY_DFT_RT_PRESS_SENS_LEVEL(newShaftType), KEY_DFT_RT_RAISE_SENS_LEVEL(newShaftType));
    }
        
    if (   deadTopLevel > DIST_TO_LEVEL(KEY_DEADBAND_MAX(ki), KEY_DEADBAND_ACC(ki)) 
        || deadBotLevel < DIST_TO_LEVEL(KEY_DEADBAND_MIN(ki), KEY_DEADBAND_ACC(ki)) || deadBotLevel > DIST_TO_LEVEL(KEY_DEADBAND_MAX(ki), KEY_DEADBAND_ACC(ki)) )    
    {
        DBG_PRINTF("    - dead para error\n");
        fix_set_dead_para(ki, KEY_DFT_DEAD_TOP_LEVEL(newShaftType), KEY_DFT_DEAD_BOT_LEVEL(newShaftType));
    }

    //if (sw_config_table[kh.p_calib->sw[ki]].isRtHighAcc)
    //{
        //int8_t tmpPress = ACC_BETW_CONV(kh.p_attr[ki].rtPressSensitive, KEY_HACC(ki), KEY_NACC(ki));
        //int8_t tmpRaise = ACC_BETW_CONV(kh.p_attr[ki].rtRaiseSensitive, KEY_HACC(ki), KEY_NACC(ki));
        //kh.p_st[ki].rtPressLowAccSens = tmpPress <= 0 ? 1 : tmpPress;
        //kh.p_st[ki].rtRaiseLowAccSens = tmpRaise <= 0 ? 1 : tmpRaise;
    //}

    #if defined(MODULE_LOG_ENABLED) && (MODULE_LOG_ENABLED == 1)
    sprintf(tmpCh, "new shaft: %s(%02d,%02d,%02d,%02d,%02d,%02d) ", 
                                  key_cali.sw[ki] == KEY_SHAFT_TYPE_GATERON_WHITE ? "GDL_WHITE"
                                : key_cali.sw[ki] == KEY_SHAFT_TYPE_GATERON_JADE  ? "GDL_JADE"
                                : key_cali.sw[ki] == KEY_SHAFT_TYPE_KAILH_GREEN  ? "KH_YELLOW"
                                : key_cali.sw[ki] == KEY_SHAFT_TYPE_TTC_WCW  ? "TTC_WCW"
                                : "UNKNOWN", 
                                kh.p_attr[ki].triggerlevel, kh.p_attr[ki].raiseLevel, 
                                kh.p_attr[ki].rtPressSensitive, kh.p_attr[ki].rtRaiseSensitive, 
                                kh.p_attr[ki].deadbandTop,kh.p_attr[ki].deadbandBottom);
    DBG_PRINTF("    - [%02d]: %s \n", ki, (tmpCh));
    #endif
}

uint16_t ks_cmd_set_shaft_type(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    ModuleUartCmdSetsw_t *msg = (ModuleUartCmdSetsw_t *)reveBuffer; /* get cmd msg */
    (void)respBufferSize;
    
    #if defined(LOG_SET_SFT) && (LOG_SET_SFT == 1)  
    cmd_show_playload(__func__, "receive", (uint8_t *)msg, reveBufferSize);
    #endif

    uint8_t ki            = 0;
    uint8_t respResult    = CMD_RESP_FAIL;
    
    if (cali_get_statue() == KEY_CALIB_SWITCH_OUT)
    {
        respResult = CMD_RESP_FAIL;
        respBuffer[*respLenght] = respResult;
        *respLenght            += 1;
        return respResult;
    }
    
    cali_set_peek_adc(false);
    
    kh_data_t *p_adc_data = cali_peek_new_adc_fifo();
//    uint8_t frameCnt      = p_adc_data->buf[p_adc_data->size-1]&0xFF;
    //uint8_t adcBuffSize   = (p_adc_data->buf[p_adc_data->size-1]>>8)&0xFF;

    //uint16_t tmpData      = KEY_DFT_TOP_ADC_VALUE;
    //NRF_LOG_RAW_INFO("%d,%d,[%d]%d\n", frameCnt, buffSize, keyboardTableMcuToHostRound[frameCnt][0], p_adc_data->buf[0]);
    
    if ((msg->keyNumber != 0 && reveBufferSize < 3) || (msg->keyNumber ==0xFF && reveBufferSize != 3) || p_adc_data == NULL)
    {
        respResult = CMD_RESP_FAIL;
        #if defined(LOG_SET_SFT) && (LOG_SET_SFT == 1)  
        log_debug("    - error 0x%02x\n", respResult);
        #endif
    }
    else if (msg->keyNumber == 0x00 || msg->keyNumber == 0xFF) /*  Reset all to default values */ /* set all key */ 
    {
        respResult = CMD_RESP_SUCCEED;
        key_sw_type_e newsw = msg->keyNumber == 0x00 ? KEY_DFT_SHAFT : msg->key[0].msg.sw;
        if (newsw == KEY_SHAFT_TYPE_UNKNOWN || newsw >= KEY_SHAFT_TYPE_MAX)
        {
            respResult = CMD_RESP_FAIL + 1;
            #if defined(LOG_SET_SFT) && (LOG_SET_SFT == 1)  
            log_debug("    - error: 0x%02x, set all \n", respResult);
            #endif
        }
        else
        {
            #if defined(LOG_SET_SFT) && (LOG_SET_SFT == 1)  
            log_debug("    - set all \n");
            #endif
            kh.p_qty->shaftChangeNum   = 0;
            for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
            {
                //ki = msg->key[i].msg.pos;
                //tmpData = p_adc_data->buf[i];
                if (sw_config_table[newsw].voltAscend)
                {
                     p_adc_data->buf[ki] = ADC16B_DATA_REVERSE( p_adc_data->buf[ki]);
                }
                
                //keyIsTop = key_pos_is_top(ki, tmpData);
                //if (keyIsTop != true)
                //{
                //    respResult = CMD_RESP_FAIL + 2;
                //    log_debug("    - error:0x%02x, key[%d] not top\n", respResult, ki);
                //    continue;
                //}
                
                /* para same */
                
                //if (key_cali.sw[ki] == newsw)
                //{
                    //continue;
                //}

            #if 0
                kh.p_qty->shaftChangeNum  += 1;
                kh.p_attr[ki].shaftChange = true;
            #else
                kh.p_qty->shaftChangeNum  = 0;
                kh.p_attr[ki].shaftChange = false;

                //key shaft change
                key_shaft_change_acc_conv(ki, newsw);
                DBG_PRINTF("HERE 1\n");
                key_st[ki].diff = sw_thd_table[newsw].def_diff;
                
                if(p_adc_data->buf[ki] != 0)
                {
                    key_cali.max[ki] = p_adc_data->buf[ki];
                    key_cali.min[ki] = p_adc_data->buf[ki] - key_st[ki].diff;
                }
                else
                {
                    key_cali.max[ki] = (sw_thd_table[newsw].top_max + sw_thd_table[newsw].top_min)/2;
                    key_cali.min[ki] = (sw_thd_table[newsw].bottom_max + sw_thd_table[newsw].bottom_min)/2;;
                }
                
                //cali_calc_level_boundary(ki);
                //key_rt_clear_state(ki);
                //key_clear_status(ki);
            #endif
            }
            DBG_PRINTF("HERE 2\n");

            DBG_PRINTF("HERE 3\n");
            cali_calc_level_boundarys(); /* boundary algo calc */
            DBG_PRINTF("HERE 4\n");
            matrix_reset();
            DBG_PRINTF("HERE 5\n");
            fix_init(&kh);
            DBG_PRINTF("HERE 6\n");
            rt_init(&kh);
            //key_dks_init(&kh);
            DBG_PRINTF("HERE 7\n");
            db_update(DB_SYS, UPDATE_LATER);
            db_update(DB_CALI, UPDATE_LATER);
        }
    } 
    else if (msg->keyNumber >= 0x01 && msg->keyNumber <= KEYBOARD_NUMBER) /* set key */
    {
        #if defined(LOG_SET_SFT) && (LOG_SET_SFT == 1)  
        log_debug("    - set partial, %d\n", msg->keyNumber);
        #endif
        respResult = CMD_RESP_SUCCEED;
        for (uint8_t i = 0; i < msg->keyNumber; i++)
        {
            ki = msg->key[i].msg.pos;
            //tmpData = p_adc_data->buf[i];
            key_sw_type_e newsw = msg->key[i].msg.sw;
            
            #if defined(LOG_SET_SFT) && (LOG_SET_SFT == 1)
            log_debug("    - set partial, [%d] %d(%d) \n", i, ki, p_adc_data->buf[ki] );
            #endif
        
            if (sw_config_table[newsw].voltAscend)
            {
                p_adc_data->buf[ki] = ADC12B_DATA_REVERSE(p_adc_data->buf[ki]);
            }
            //keyIsTop = key_pos_is_top(ki, tmpData);
            //if (keyIsTop != true)
            //{
            //    respResult = CMD_RESP_FAIL + 2;
            //    log_debug("    - error:0x%02x, key[%d] not top\n", respResult, ki);
            //    continue;
            //}
            if (newsw == KEY_SHAFT_TYPE_UNKNOWN || newsw >= KEY_SHAFT_TYPE_MAX)
            {
                respResult = CMD_RESP_FAIL + 1;
                #if defined(LOG_SET_SFT) && (LOG_SET_SFT == 1)  
                log_debug("    - error: 0x%02x,  key[%d]\n", respResult, ki);
                #endif
                continue;
            }
            /* para same */
            if (key_cali.sw[ki] == newsw)
            {
                continue;
            }
            
        #if 0
            if (kh.p_attr[ki].shaftChange == false)
            {
                kh.p_qty->shaftChangeNum += 1;
            }
            kh.p_attr[ki].shaftChange = true;
        #else
            kh.p_attr[ki].shaftChange = false;
    
            //key shaft change
            key_shaft_change_acc_conv(ki, newsw);
            
            key_st[ki].diff = sw_thd_table[newsw].def_diff;
            if(p_adc_data->buf[ki] != 0)
            {
                key_cali.max[ki] = p_adc_data->buf[ki];
                key_cali.min[ki] = p_adc_data->buf[ki] - key_st[ki].diff;
            }
            else
            {
                key_cali.max[ki] = (sw_thd_table[newsw].top_max + sw_thd_table[newsw].top_min)/2;
                key_cali.min[ki] = (sw_thd_table[newsw].bottom_max + sw_thd_table[newsw].bottom_min)/2;;
            }
            
            //cali_calc_level_boundary(ki);
            //key_rt_clear_state(ki);
            //key_clear_status(ki);
        #endif
        }
        
       // kh.p_qty->shaftChangeNum = 0;
       // for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
       // {
       //     if (kh.p_attr[ki].shaftChange == true)
       //     {
       //         kh.p_qty->shaftChangeNum += 1;
       //     }
       // }
       
        matrix_reset();
        cali_calc_level_boundarys(); /* boundary algo calc */
        fix_init(&kh);
        rt_init(&kh);
        //key_dks_init(&kh);
        
        db_update(DB_SYS, UPDATE_NOW);
        db_update(DB_CALI, UPDATE_NOW);
    }
    else
    {
        respResult = CMD_RESP_FAIL + 3;
        #if defined(LOG_SET_SFT) && (LOG_SET_SFT == 1)  
        log_debug("    - error 0x%02x\n", respResult);
        #endif
    }

    cali_set_peek_adc(true);
    respBuffer[*respLenght] = respResult;
    *respLenght            += 1;
    #if defined(LOG_SET_SFT) && (LOG_SET_SFT == 1)  
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
    // ret = cmd_respond_push_msg(instDevKeyscan->moduleUart.framePackReceive.cmd, instDevKeyscan->moduleUart.framePackReceive.subCmd, respResult, 0, NULL);
    return respResult;
}

uint16_t ks_cmd_get_default_shaft_type(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint8_t respResult = CMD_RESP_SUCCEED;    
    ModuleUartCmdGetsw_t *respMsg = (ModuleUartCmdGetsw_t *)respBuffer;
    uint16_t respMsgLenght = sizeof(ModuleUartCmdGetsw_t);
    (void)respBufferSize;
    (void)reveBufferSize;
    (void)reveBuffer;

    #if defined(LOG_SET_SFT) && (LOG_GET_SFT == 1)  
    cmd_show_playload(__func__, "receive", (uint8_t *)reveBuffer, reveBufferSize);
    #endif

    // key_sw_type_e keysw = KEY_DFT_SHAFT;
    // #warning "debug shaft, todo jay"

    respMsg->type = KEY_DFT_SHAFT;

//    char tmp[64] = {'\0'};
//    sprintf(tmp, "    - get default shaft type: %d(%s) \n", respMsg->type, 
//                                                            respMsg->type == KEY_SHAFT_TYPE_GATERON_WHITE ? "WHITE" 
//                                                            : respMsg->type == KEY_SHAFT_TYPE_GATERON_JADE ? "JADE" 
//                                                            : "UNKONW!" );
//    log_debug("%s", NRF_LOG_PUSH(tmp) );

    *respLenght = respMsgLenght;
    #if defined(LOG_SET_SFT) && (LOG_GET_SFT == 1)  
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
    return respResult;
}

uint16_t ks_cmd_get_wave_max(uint8_t *reveBuffer, uint16_t reveBufferSize, uint8_t *respBuffer, uint16_t respBufferSize, uint16_t *respLenght)
{
    uint8_t respResult = CMD_RESP_SUCCEED;    
    ModuleUartCmdGetWaveMax_t *respMsg = (ModuleUartCmdGetWaveMax_t *)respBuffer;
    uint16_t respMsgLenght = sizeof(ModuleUartCmdGetWaveMax_t);
    (void)respBufferSize;
    (void)reveBufferSize;

    #if defined(LOG_GET_WM) && (LOG_GET_WM == 1)  
    cmd_show_playload(__func__, "receive", (uint8_t *)reveBuffer, reveBufferSize);
    #endif
    

    uint8_t ki = reveBuffer[0];
    if (ki >= KEYBOARD_NUMBER)
    {
        respResult = CMD_RESP_FAIL;
        #if defined(LOG_GET_WM) && (LOG_GET_WM == 1)  
        log_debug("    - error 0x%02x\n", respResult);
        #endif
    }
    else
    {
        float tmpStep           = 0.0f;
        uint16_t sw      = key_cali.sw[ki];
        //uint16_t keyTopValue    = key_cali.max[ki] - KEY_TOP_RESERVER(ki);
        //uint16_t keyBottomValue = key_cali.min[ki] + KEY_BOT_RESERVER(ki);
        uint16_t travel         = key_st[ki].diff; //ABS(keyTopValue - keyBottomValue);
        uint16_t level          = KEY_MAX_LEVEL(ki) - 1;
        uint16_t tmpRaiseSens   = kh.p_attr[ki].rtRaiseSensitive + kh.p_attr[ki].deadbandBottom;
        for (uint8_t i = 0; i < tmpRaiseSens; i++)
        {
            level = (KEY_MAX_LEVEL(ki) - 1 - i) < 0 ? 0 : (KEY_MAX_LEVEL(ki) - 1 - i);
            tmpStep += sw_config_table[sw].curvTab[level];    
        }
        respMsg->wave           = (uint16_t)(tmpStep * travel);
        #if defined(LOG_GET_WM) && (LOG_GET_WM == 1)  
        log_debug("    - [%02d]: wave max: %d, %d\n", ki, respMsg->wave, (uint16_t)(tmpStep * key_st[ki].diff));
        #endif
    }


    *respLenght = respMsgLenght;
    #if defined(LOG_GET_WM) && (LOG_GET_WM == 1)  
    cmd_show_playload(__func__, "respond", (uint8_t *)respBuffer, *respLenght);
    #endif
    return respResult;
    
}

