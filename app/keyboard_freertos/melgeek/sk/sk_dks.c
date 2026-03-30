/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include "sk_dks.h"

#include "db.h"
#include <string.h>
#include "mg_hive.h"
//#include "interface.h"
#include "mg_hid.h"
#include "app_debug.h"
#include "mg_matrix.h"
#include "mg_cali.h"
#include "mg_matrix_config.h"



#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "sk_dsk"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

extern float kcr_table[KEY_SHAFT_TYPE_MAX][KEY_LEVEL_BOUNDARY_BUFF_SIZE];
extern kh_t kh;
extern key_st_t key_st[];
extern kc_t key_events[BOARD_KEY_NUM];
extern uint8_t cpkeys[BOARD_KEY_NUM];     /* current pressed keys */
extern uint8_t dksCpKeys[0xFF];
extern uint8_t keys_processed;  /* number of current keys */
extern bool feature_dks_output_result(void);
extern void dispatch_keypress(bool bch);
extern void kb_wakeup(void);

bool dks_action_pop_queue(void);
void dks_tick_random_output(void);
void dks_tick_static_output(void);

//APP_TIMER_DEF(dksTickTime);
// fifo_t dksFifo[KEYSCAN_DEFAULT_DKS_VIRTUAL_KEY_NUMBER];
// uint8_t dksKeyCodeBuffer[KEYSCAN_DEFAULT_DKS_VIRTUAL_KEY_NUMBER][DKS_FIFO_NUM * DKS_FIFO_SIZE] __attribute__((aligned(4)));


static DksData_t dks_event;
DksNode_t dksNodeBuffer[KEY_DFT_DKS_VIRTUAL_KEY_NUMBER][DKS_FIFO_NUM];
DksLinkQueue_t dksLinkQueue[KEY_DFT_DKS_VIRTUAL_KEY_NUMBER];
uint8_t dksTimeStart = false;
static rep_hid_t *kb_report = NULL;
static rep_hid_t *ms_report = NULL;
static rep_hid_t *dks_kb_report = NULL;





bool dks_link_init(DksLinkQueue_t *ptrDksLink, uint8_t keyNum)
{
    memset(&dksNodeBuffer[keyNum][0], 0, sizeof(DksNode_t) * DKS_FIFO_NUM);
    memset(&dksNodeBuffer[keyNum][0].data, 0, sizeof(DksData_t));
    ptrDksLink->max         = DKS_FIFO_NUM;
    ptrDksLink->len         = 1;
    ptrDksLink->rear        = &dksNodeBuffer[keyNum][0];
    ptrDksLink->front       = &dksNodeBuffer[keyNum][0];
    ptrDksLink->front->next = NULL;
    return true;
}
bool dks_link_is_empty(DksLinkQueue_t dksLink)
{
    if (dksLink.front == dksLink.rear)  return true;
    else                                return false;
}
bool dks_link_enqueue(DksLinkQueue_t *ptrDksLink, uint8_t keyNum, DksData_t data)
{
    if (ptrDksLink->len >= ptrDksLink->max)
    {
        return false;
    }

    DksNode_t *node = &dksNodeBuffer[keyNum][ptrDksLink->len];
    node->next      = NULL;
    memcpy(&node->data, &data, sizeof(DksData_t));
	//enqueue
	ptrDksLink->rear->next = node;
	ptrDksLink->rear       = node;
	ptrDksLink->len       += 1;
	return true;
}
bool dks_link_sort_enqueue(DksLinkQueue_t *ptrDksLink, uint8_t keyNum, DksData_t data)
{
    if (ptrDksLink->len >= ptrDksLink->max)
    {
        return false;
    }
	uint8_t pos        = ptrDksLink->len;
    DksNode_t *head    = ptrDksLink->front->next;
    DksNode_t *tail    = ptrDksLink->rear;
    DksNode_t *addNode = &dksNodeBuffer[keyNum][pos];
    addNode->next      = NULL;
    memcpy(&addNode->data, &data, sizeof(DksData_t));
    
    if (head == NULL)
    {
        //enqueue
        ptrDksLink->rear->next = addNode;
        ptrDksLink->rear       = addNode;
        ptrDksLink->len       += 1;
    }
    else
    {
        //sort enqueue
        if (addNode->data.ts >= tail->data.ts) //insert tail 
        {
            ptrDksLink->rear->next = addNode;
            ptrDksLink->rear       = addNode;
            ptrDksLink->len       += 1;
        }
        else if (addNode->data.ts < head->data.ts) //insert head 
        {
            addNode->next           = head;
            ptrDksLink->front->next = addNode;
            ptrDksLink->len        += 1;
        }
        else //insert middle
        {
			DksNode_t *pNode   = head->next; //move
			DksNode_t *preNode = head;       //precursor
            uint8_t lenght     = ptrDksLink->len;
            while(pNode != NULL)
            {
                if (lenght == 0)
                {
                    return false;
                }
                else if (addNode->data.ts < pNode->data.ts)
                {
                    preNode->next    = addNode;
                    addNode->next    = pNode;
                    ptrDksLink->len += 1;
                    return true;
                }
                else if (addNode->data.ts == pNode->data.ts)
                {
                    addNode->next    = pNode->next;
					pNode->next      = addNode;
                    ptrDksLink->len += 1;
                    return true;
                }
                lenght -= 1;
                pNode   = pNode->next;
                preNode = preNode->next;
            }
        }
    }
	return true;
}
bool dks_link_dequeue(DksLinkQueue_t *ptrDksLink, DksData_t *ptrData)
{
    if (ptrDksLink->front == ptrDksLink->rear)
    {
        return false;
    }

    DksNode_t *head         = ptrDksLink->front->next;
    ptrDksLink->front->next = head->next; //update head node
	ptrDksLink->len        -= 1;

    if (head == ptrDksLink->rear)
    {
        ptrDksLink->rear = ptrDksLink->front;
		ptrDksLink->len  = 1;
    }

    memcpy(ptrData, &head->data, sizeof(DksData_t));
    return true;
}
bool dks_link_list(DksLinkQueue_t dksLink)
{
    if (dksLink.front == dksLink.rear)
    {
        return false;
    }
    
    DksNode_t *head = dksLink.front->next;
    uint8_t cnt = 0;
	while(head != NULL)
	{
		if(cnt >= dksLink.max)
			break;
        DBG_PRINTF("dks ts: %d\r\n", head->data.ts);
		head = head->next;
		cnt++;
	}
    return true;
}


void adv_clear_st(key_attr_t *p_attr, uint8_t kcm)
{
    p_attr->advTrg[kcm].advType = KEY_ACT_UND;
    p_attr->advTrg[kcm].dksParaTableUsedId = 0;
    p_attr->advTrg[kcm].reserver = 0;
    p_attr->advTrg[kcm].para.ptr = NULL;
}

static void dks_check_para(key_attr_t *key_attr)
{    
    if (!key_attr)
    {
        return;
    }

    for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        key_attr_t *p_attr = (key_attr + ki);

        for (uint8_t kcm = 0; kcm < BOARD_KCM_MAX; kcm++)
        {
            if (p_attr->advTrg[kcm].advType > KEY_ACT_MAX)
            {
                adv_clear_st(p_attr, kcm);
                continue;
            }

            if (p_attr->advTrg[kcm].advType == KEY_ACT_DKS)
            {
                uint8_t id = p_attr->advTrg[kcm].dksParaTableUsedId;
                if (sk_la_lm_kc_info.sk.superkey_table[SKT_DKS].free_size == SK_DKS_NUM)
                {
                    adv_clear_st(p_attr, kcm);
                }
                else if ((sk_la_lm_kc_info.sk.superkey_table[SKT_DKS].storage_mask & (0x01 << id)) == false)
                {
                    adv_clear_st(p_attr, kcm);
                }
                else if (id > SK_DKS_NUM)
                {
                    adv_clear_st(p_attr, kcm);
                }
                else if ((&sk_la_lm_kc_info.sk.dks_table[id].para) != (p_attr->advTrg[kcm].para.ptr))
                {
                    adv_clear_st(p_attr, kcm);
                }
            }
        }
    }
}


void feature_dks_set_tick_para(dks_tick_type tickType, uint8_t tickMin, uint8_t tickMax)
{
    kh.p_qty->dksTick.type = tickType;
    kh.p_qty->dksTick.min  = tickMin;
    kh.p_qty->dksTick.max  = tickMax;

    if (tickType == TICK_STATIC)
    {
        dks_tick_static_output();
    }
    else if (tickType == TICK_RANDOM)
    {
        dks_tick_random_output();
    }
}

bool feature_dks_bind(uint8_t kcmid, uint8_t ki, uint8_t paraTableId)
{
    bool ret = false;

    // if (sk_la_lm_kc_info.sk.superkey_table[SKT_DKS].free_size == KEY_DFT_DKS_PARA_TABLE_MAX || sk_la_lm_kc_info.sk.superkey_table[SKT_DKS].storage_mask == 0)
    // // if(kh.p_qty->dksParaNumber==0)
    // {
    //     DBG_PRINTF("    - error bind dks para table is empty\r\n");
    //     return ret;
    // }
    // else if (sk_la_lm_kc_info.sk.dks_table[paraTableId].para.toTopLevel == 0 && sk_la_lm_kc_info.sk.dks_table[paraTableId].para.toBotLevel == 0)
    // {
    //     DBG_PRINTF("    - error  bind dks para table value NULL\r\n");
    //     return ret;
    // }

    DBG_PRINTF("    - bind dks keyPos:%02d, dks para table id(%d), \r\n", ki, paraTableId);

    /* change RT number */
    if (kh.p_attr[ki].advTrg[0].advType != KEY_ACT_DKS)
    {
        int8_t change = (kh.p_attr[ki].rt_en == true) ? (-1) : (0);
        kh.p_gst->rt_en_num += change;
    }

    /* dks para bind */
    kh.p_attr[ki].advTrg[kcmid].advType            = KEY_ACT_DKS;
    kh.p_attr[ki].advTrg[kcmid].para.dks           = &sk_la_lm_kc_info.sk.dks_table[paraTableId].para;
    kh.p_attr[ki].advTrg[kcmid].dksParaTableUsedId = paraTableId;

/*
    uint8_t sw  = cali_cfg.shaftType[ki];
    if (sw_config_table[sw].isRtHighAcc)
    {
        uint8_t tmpToTopLevel = kh.p_attr[ki].advTrg[kcmid].para.dks->toTopLevel;
        uint8_t tmpToBotLevel = kh.p_attr[ki].advTrg[kcmid].para.dks->toBotLevel;

        if (KEY_DFT_SHAFT_MAX_LEVEL(sw) - tmpToBotLevel <=  KEY_SHAFT_G_JADE_NORMAL_ACC_TO_LEVEL)
        {
            tmpToBotLevel = ACC_BETW_CONV(tmpToBotLevel, sw_config_table[KEY_DFT_SHAFT].dksAcc, KEY_SHAFT_G_JADE_HIGH_ACC);
            tmpToBotLevel = ACC_BETW_CONV(tmpToBotLevel, sw_config_table[KEY_DFT_SHAFT].dksAcc, KEY_SHAFT_G_JADE_NORMAL_ACC);
            
            tmpToBotLevel = DIST_TO_LEVEL(tmpToBotLevel - KEY_SHAFT_G_JADE_NORMAL_ACC_TO_DIST, KEY_SHAFT_G_JADE_HIGH_ACC) + KEY_SHAFT_G_JADE_NORMAL_ACC_TO_LEVEL;
        }
    }
*/

    

    /* used dks bind table and number */
    kh.p_gst->dksBindUsedTable[kh.p_gst->dksBindUsedNumber] = ki;
    kh.p_gst->dksBindUsedNumber += 1;

    /* rt number */
    kh.p_gst->rtKeyUsedTableNumber = 0;
    for (uint8_t tmpKi = 0; tmpKi < KEYBOARD_NUMBER; tmpKi++)
    {
        if (kh.p_attr[tmpKi].rt_en == true && kh.p_attr[tmpKi].advTrg[0].advType != KEY_ACT_DKS)
        {
            kh.p_gst->rtKeyUsedTable[kh.p_gst->rtKeyUsedTableNumber++] = tmpKi;
        }
    }
    db_update(DB_SYS, UPDATE_LATER);
    return true;
}

bool feature_dks_unbind(uint8_t kcmid, uint8_t ki)
{
    bool ret = false;
    if (kh.p_attr[ki].advTrg[kcmid].advType != KEY_ACT_DKS)
    {
        DBG_PRINTF("    - error unbind not dks trigger mode\r\n");
        return ret;
    }
    else if (kh.p_attr[ki].advTrg[kcmid].para.dks == NULL)
    {
        DBG_PRINTF("    - error unbind dks not bound\r\n");
        return ret;
    }
    //  else if(dksPara->startLevel==0 && dksPara->bottomLevel==0)
    //  {
    //      respResult = CMD_RESP_FAIL+5;
    //      DBG_PRINTF("    - error 0x%02x unbind dks para table value NULL\r\n", respResult);
    //      return ret;
    //  }

    DBG_PRINTF("    - unbind dks keyPos:%02d \r\n", ki);
    /* change RT number */
    if (kh.p_attr[ki].advTrg[0].advType == KEY_ACT_DKS)
    {
        int8_t change = (kh.p_attr[ki].rt_en == true) ? (1) : (0);
        kh.p_gst->rt_en_num += change;
    }

    //unbind
    kh.p_attr[ki].advTrg[kcmid].advType            = KEY_ACT_UND;
    kh.p_attr[ki].advTrg[kcmid].para.dks           = NULL;
    kh.p_attr[ki].advTrg[kcmid].dksParaTableUsedId = 0;

    /* rt number */
    /* used dks bind table and number */
    kh.p_gst->rtKeyUsedTableNumber = 0;
    kh.p_gst->dksBindUsedNumber    = 0;
    for (uint8_t tmpKi = 0; tmpKi < KEYBOARD_NUMBER; tmpKi++)
    {
        /* RT used table */
        if (kh.p_attr[tmpKi].rt_en == true && kh.p_attr[tmpKi].advTrg[0].advType != KEY_ACT_DKS)
        {
            uint8_t index = kh.p_gst->rtKeyUsedTableNumber;
            kh.p_gst->rtKeyUsedTable[index] = tmpKi;
            kh.p_gst->rtKeyUsedTableNumber += 1;
        }
        /* DKS used table */
        for (uint8_t kcmid = 0; kcmid < BOARD_KCM_MAX; kcmid++)
        {
            if (kh.p_attr[tmpKi].advTrg[kcmid].advType == KEY_ACT_DKS)
            {
                uint8_t index = kh.p_gst->dksBindUsedNumber;
                kh.p_gst->dksBindUsedTable[index] = tmpKi;
                kh.p_gst->dksBindUsedNumber      += 1;
            }
        }
    }
    db_update(DB_SYS, UPDATE_LATER);
    return true;
}

void feature_dks_trigger_default_para(void)
{
    // memset(kh.p_qty->dksParaTable, NULL, sizeof(key_dks_t)*KEY_DFT_DKS_PARA_TABLE_MAX);
    memset(sk_la_lm_kc_info.sk.dks_table, 0, sizeof(dks_t) * KEY_DFT_DKS_PARA_TABLE_MAX);
  //  for (uint8_t i = 0; i < KEY_DFT_DKS_PARA_TABLE_MAX; i++)
   // {
        // memset(kh.p_qty->dksParaTable[i].keyCode, 0xFF, KEY_DFT_DKS_VIRTUAL_KEY_NUMBER);
      //  memset(sk.dks_table[i].para.keyCode, 0xFF, sizeof(dynamic_key_t)*KEY_DFT_DKS_VIRTUAL_KEY_NUMBER);
  //  }

    // kh.p_qty->dksParaNumber = 0;

    sk_la_lm_kc_info.sk.superkey_table[SKT_DKS].type         = SKT_DKS;
    sk_la_lm_kc_info.sk.superkey_table[SKT_DKS].total_size   = KEY_DFT_DKS_PARA_TABLE_MAX;
    sk_la_lm_kc_info.sk.superkey_table[SKT_DKS].free_size    = KEY_DFT_DKS_PARA_TABLE_MAX;
    sk_la_lm_kc_info.sk.superkey_table[SKT_DKS].storage_mask = 0;

    kh.p_qty->dksTick.type = KEY_DFT_DKS_VIRTUAL_KEY_TICK_TYPE;
    kh.p_qty->dksTick.min  = KEY_DFT_DKS_VIRTUAL_KEY_TICK_MIN;
    kh.p_qty->dksTick.max  = KEY_DFT_DKS_VIRTUAL_KEY_TICK_MAX;

    for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        /* advanced type */
        for (uint8_t kcmid = 0; kcmid < BOARD_KCM_MAX; kcmid++)
        {
            kh.p_attr[ki].advTrg[kcmid].advType  = KEY_ACT_UND;
            kh.p_attr[ki].advTrg[kcmid].para.ptr = NULL;
        }
    }
    
    
}

void dks_tick_timer_stop(void)
{
    //app_timer_stop(dksTickTime);
}
void dks_tick_static_output(void)
{
    uint8_t tickTime = kh.p_qty->dksTick.min;

    if (tickTime == 0)
    {
        //app_timer_stop(dksTickTime);
    }
    else
    {
        uint16_t ms = (uint16_t)(1.0f / (float)tickTime * 1000.0f);
        //app_timer_start(dksTickTime, /*APP_TIMER_TICKS*/(ms), NULL);
    }
}
void dks_tick_random_output(void)
{
    uint8_t tickMin  = kh.p_qty->dksTick.min;
    uint8_t tickMax  = kh.p_qty->dksTick.max;
    uint8_t tickTime = 0;

    //nrfx_systick_state_t sys_tick;
    //nrfx_systick_get(&sys_tick);
    //srand(sys_tick.time);
    tickTime = tickMin + rand() % (tickMax - tickMin);

    uint16_t ms = (uint16_t)(1.0f / (float)tickTime * 1000.0f);

    ms = ms <= 0 ? 1 : ms;
    //app_timer_start(dksTickTime, /*APP_TIMER_TICKS*/(ms), NULL);
}

bool dks_key_find_is_d2u_action(const key_dks_t *ptrDks, uint8_t keyNum, DksActionNum_e actionNum)
{
    bool ret = false;
    if (actionNum > DKS_ACT_3)
        return ret;

    uint8_t isU2dBit  = actionNum * 2;
    uint8_t isD2uBit  = actionNum * 2 + 1;
    uint8_t keyAction = ptrDks->keyEvent[keyNum].buffer;

    bool isU2d = (((keyAction >> isU2dBit) & 0x01) == true) ? true : false;
    bool isD2u = (((keyAction >> isD2uBit) & 0x01) == true) ? true : false;

    if (isU2d == true && isD2u == false) /* need to release */
    {
        ret = true;
        // DBG_PRINTF("\tDKS check[%d]: %d\r\n", actionNum, ret);
    }
    else if (isU2d == true && isD2u == true) /* abort */
    {
        ret = false;;
    }
    else
    {
        actionNum -= 1;
        ret = dks_key_find_is_d2u_action(ptrDks, keyNum, actionNum); /* continue find */
    }
    return ret;
}

bool dks_key_is_u2d_action(uint8_t keyAction, DksActionNum_e actionNum)
{
    uint8_t isU2dBit = actionNum * 2;
    bool ret         = (((keyAction >> isU2dBit) & 0x01) == true) ? true : false;
    return ret;
}
bool dks_key_is_d2u_action(uint8_t keyAction, DksActionNum_e actionNum)
{
    uint8_t isD2uBit = actionNum * 2 + 1;
    bool ret         = (((keyAction >> isD2uBit) & 0x01) == true) ? true : false;
    return ret;
}
bool dks_key_is_d2u_action_previous(const key_dks_t *ptrDks, uint8_t keyNum, DksActionNum_e actionNum)
{
    bool ret = false;
    uint8_t isU2dBit  = actionNum * 2;
    uint8_t keyAction = ptrDks->keyEvent[keyNum].buffer;
    bool isU2d        = (((keyAction >> isU2dBit) & 0x01) == true) ? true : false;

    if ((actionNum != DKS_ACT_0 && isU2d == true) || (actionNum == DKS_ACT_3)) /* 查找先前的动作是否需要抬起 */
    {
        actionNum -= 1;
        ret = dks_key_find_is_d2u_action(ptrDks, keyNum, actionNum);
    }
    return ret;
}

void dks_key_is_action(const key_dks_t *ptrDks, uint8_t keyNum, DksActionNum_e actionNum)
{
    uint8_t len = 0;
    DksData_t buffer[5];
    memset(buffer, 0, sizeof(DksData_t)*5);

//    DksKeyQueueOp_t dropBuffer;
    dynamic_key_t keyCode = ptrDks->keyCode[keyNum];
    DksData_t *dksKeyQueueWd;

    switch (actionNum)
    {
        case DKS_ACT_0:
        {
            /* U2D */
            if (dks_key_is_u2d_action(ptrDks->keyEvent[keyNum].buffer, actionNum) == true)
            {
                dksKeyQueueWd         = (DksData_t *)&buffer[len++];
                dksKeyQueueWd->code   = keyCode;
                dksKeyQueueWd->status = DKS_U2D;
            }
            /* D2U */
            if (dks_key_is_d2u_action(ptrDks->keyEvent[keyNum].buffer, actionNum) == true)
            {
                dksKeyQueueWd         = (DksData_t *)&buffer[len++];
                dksKeyQueueWd->code   = keyCode;
                dksKeyQueueWd->status = DKS_D2U;
            }
        }
        break;
        case DKS_ACT_1:
        case DKS_ACT_2:
        {
            /* previous D2U */
            if (dks_key_is_d2u_action_previous(ptrDks, keyNum, actionNum) == true)
            {
                dksKeyQueueWd         = (DksData_t *)&buffer[len++];
                dksKeyQueueWd->code   = keyCode;
                dksKeyQueueWd->status = DKS_D2U;
            }
            /* U2D */
            if (dks_key_is_u2d_action(ptrDks->keyEvent[keyNum].buffer, actionNum) == true)
            {
                dksKeyQueueWd         = (DksData_t *)&buffer[len++];
                dksKeyQueueWd->code   = keyCode;
                dksKeyQueueWd->status = DKS_U2D;
            }
            /* D2U */
            if (dks_key_is_d2u_action(ptrDks->keyEvent[keyNum].buffer, actionNum) == true)
            {
                dksKeyQueueWd         = (DksData_t *)&buffer[len++];
                dksKeyQueueWd->code   = keyCode;
                dksKeyQueueWd->status = DKS_D2U;
            }
        }
        break;
        case DKS_ACT_3:
        {
            /* previous D2U */
            if (dks_key_is_d2u_action_previous(ptrDks, keyNum, actionNum) == true)
            {
                dksKeyQueueWd         = (DksData_t *)&buffer[len++];
                dksKeyQueueWd->code   = keyCode;
                dksKeyQueueWd->status = DKS_D2U;
            }
            /* U2D */
            if (dks_key_is_u2d_action(ptrDks->keyEvent[keyNum].buffer, actionNum) == true)
            {
                dksKeyQueueWd         = (DksData_t *)&buffer[len++];
                dksKeyQueueWd->code   = keyCode;
                dksKeyQueueWd->status = DKS_U2D;

                /* D2U */
                dksKeyQueueWd         = (DksData_t *)&buffer[len++];
                dksKeyQueueWd->code   = keyCode;
                dksKeyQueueWd->status = DKS_D2U;
            }
        }
        break;
    }
    
    for(uint8_t i = 0; i < len;  i++)
    {
        dks_link_enqueue(&dksLinkQueue[keyNum], keyNum, buffer[i]);
    }

}

void dks_action_push_queue(const key_dks_t *ptrDks, DksActionNum_e actionNum)
{
    for (uint8_t i = 0; i < KEY_DFT_DKS_VIRTUAL_KEY_NUMBER; i++)
    {
        dks_key_is_action(ptrDks, i, actionNum);
    }
}

bool dks_action_pop_queue(void)
{
    bool ret = false;

    for (uint8_t i = 0; i < KEY_DFT_DKS_VIRTUAL_KEY_NUMBER; i++)
    {
        if (dks_link_is_empty(dksLinkQueue[i]) == false)
        {
            dks_link_dequeue(&dksLinkQueue[i], &dks_event);
            ret = true;
        }
    }
    return ret;
}

bool dks_dispatch_keyevent(void)
{
    bool ret          = false;
    bool kb_change    = false;
    bool ms_change    = false;
    uint8_t nkro      = gbinfo.nkro;
    rep_hid_t *kb_rpt = NULL;
    if (nkro)
    {
        kb_rpt = dks_kb_report;
        kb_rpt->report_id = REPORT_ID_NKRO;
    }
    else
    {
        kb_rpt = kb_report;
    }

    for (uint8_t i = 0; i < KEY_DFT_DKS_VIRTUAL_KEY_NUMBER; i++)
    {
        if (dks_link_dequeue(&dksLinkQueue[i], &dks_event) != true)
        {
            continue;
        }
    
        if (dks_event.status == DKS_U2D)
        {
            if (dks_event.code.ty == KCT_KB)
            {
                kb_change  = true;
                int16_t ki = kc_to_ki[dks_event.code.co];
                if ((nkro == false) || (ki == -1))
                {
                    report_add_key(kb_rpt, dks_event.code.co);
                }
                else if (ki != -1 && nkro)
                {
                    // if (kh.p_st[ki].trg_logic == KEY_STATUS_U2U)
                    {
                        report_add_key(kb_rpt, dks_event.code.co);
                        kh.p_st[ki].trg_logic = KEY_STATUS_D2D;
                    }
                }
            }
            else if (dks_event.code.ty == KCT_MS)
            {
                ms_change  = true;
                if (dks_event.code.sty <= KC_MS_BTN_SP)
                {
                    report_add_ms_btn(ms_report, dks_event.code.sty);
                }
                else if (dks_event.code.sty <= KC_MS_PY)
                {
                    int8_t val = (dks_event.code.sty == KC_MS_PY) ? dks_event.code.co : dks_event.code.co * -1;
                    report_set_ms_y(ms_report, val);
                }
                else if (dks_event.code.sty <= KC_MS_PX)
                {
                    int8_t val = (dks_event.code.sty == KC_MS_PX) ? dks_event.code.co : dks_event.code.co * -1;
                    report_set_ms_x(ms_report, val);
                }
                else if (dks_event.code.sty <= KC_MS_WD)
                {
                    int8_t val = (dks_event.code.sty == KC_MS_WD) ? -1 : 1;
                    report_set_ms_w(ms_report, val);
                }
                else
                {
                    ms_change = false;
                }
            }
        }
        else
        {
            if (dks_event.code.ty == KCT_KB)
            {
                kb_change  = true;
                int16_t ki = kc_to_ki[dks_event.code.co];
                if ((nkro == false) || (ki == -1))
                {
                    report_del_key(kb_rpt, dks_event.code.co);
                }
                else if (ki != -1 && nkro)
                {
                    uint8_t kcm = gbinfo.kc.cur_kcm;
                    bool update = (kh.p_attr[ki].advTrg[kcm].advType != KEY_ACT_DKS) || 
                                    ((kh.p_attr[ki].advTrg[kcm].advType == KEY_ACT_DKS) && (kh.p_st[ki].trg_phy == KEY_STATUS_U2U));

                    if (update)
                    {
                        report_del_key(kb_rpt, dks_event.code.co);
                        kh.p_st[ki].trg_logic = KEY_STATUS_U2U;
                    }
                }
            }
            else if (dks_event.code.ty == KCT_MS)
            {
                ms_change  = true;
                if (dks_event.code.sty <= KC_MS_BTN_SP)
                {
                    report_del_ms_btn(ms_report, dks_event.code.sty);
                }
                else if (dks_event.code.sty <= KC_MS_PY)
                {
                    report_set_ms_y(ms_report, 0);
                }
                else if (dks_event.code.sty <= KC_MS_PX)
                {
                    report_set_ms_x(ms_report, 0);
                }
                else if (dks_event.code.sty <= KC_MS_WD)
                {
                    report_set_ms_w(ms_report, 0);
                }
                else
                {
                    ms_change = false;
                }
            }
        }
        ret = true;
    }
    if (kb_change)
    {
        sk_rep_kb_change();
    }
    if (ms_change)
    {
        sk_rep_ms_change();
    }
    return ret;
}

static void dks_tick_handle(void *p_context)
{
/*
    if (kh.p_gst->dksBindUsedNumber != 0)
    {
        if (feature_dks_output_result() == true)
        {
            keys_processed = 0;
            kc_t *pkc;
            kc_t dksOutputKc;
            dksOutputKc.en   = KCE_ENABLE;
            dksOutputKc.ex   = 0;
            dksOutputKc.ty   = KCT_KB;
            dksOutputKc.sty  = 0;

            for (uint16_t i = 0; i < 0xFF; i++)
            {
                if (dksCpKeys[i] != 0xFF) //fill
                {
                    dksOutputKc.co             = dksCpKeys[i];
                    key_events[keys_processed] = dksOutputKc;
                    keys_processed            += 1;
                }
                
                // noraml key enevt output
                if (i < KEYBOARD_NUMBER)
                {
                    if (cpkeys[i] < BOARD_KEY_NUM)
                    {
                        pkc  = mx_get_kco(gbinfo.kc.cur_kcm, cpkeys[i]);
                        memcpy(&key_events[keys_processed], pkc, sizeof(kc_t));
                        keys_processed++;
                    }
                }
            }
            
            if (is_ready())
            {
                //if (is_state(BS_MACRO_PLAY))
                {
                    //return;
                }
        
                dispatch_keypress(true);
            }
            else
            {
                kb_wakeup();
            }
        }
    }
    
    if (kh.p_qty->dksTick.type == TICK_RANDOM)
    {
        dks_tick_random_output();
    }
    else
    {
        dks_tick_static_output();
    }
    */
}




void feature_dks_check_exception_handle(uint8_t kcmid, uint8_t ki)
{
    if (kh.p_st[ki].advTrg[kcmid].dks.isStart == false)
    {
        return;
    }
    DBG_PRINTF("DKS exception handle! kcmid:%02d, kid:%02d \r\n", kcmid, ki);

    DksData_t buffer;
    buffer.status  = DKS_D2U;
    key_dks_t *dks = kh.p_attr[ki].advTrg[kcmid].para.dks;
    kh.p_st[ki].advTrg[kcmid].dks.isStart = false; //clear

    for (uint8_t i = 0; i < KEY_DFT_DKS_VIRTUAL_KEY_NUMBER; i++)
    {
        if (dks->keyCode[i].co != 0 && dks->keyEvent[i].buffer != 0)
        {
            buffer.code = dks->keyCode[i];
            
            dks_link_enqueue(&dksLinkQueue[i], i, buffer);
            /*
            if (fifo_avail(&dksFifo[i]) >= 1)
            {
                fifo_in(&dksFifo[i], (uint8_t *)&buffer, 1);
            }
            else
            {
                DBG_PRINTF("DKS fifo underlength\r\n");
            }
            */
        }
    }
}

bool feature_dks_handle(uint8_t kcmid, uint8_t ki, uint16_t val)
{
    bool ret = false;
    key_dks_t *dks          = kh.p_attr[ki].advTrg[kcmid].para.dks;
    key_sw_type_e sw        = key_cali.sw[ki];
    uint8_t tmpTrgLevel     = (dks->toTopLevel > (1 + kh.p_attr[ki].deadbandTop)) ? (dks->toTopLevel) : (1 + kh.p_attr[ki].deadbandTop);
    uint8_t triggerLevelPos = (tmpTrgLevel - 1) <= 0 ? 0 : (tmpTrgLevel - 1);

    // DBG_PRINTF("[%02d]DKS(trg:%d): %04d < %04d\r\n", ki, kh.p_st[ki].trg, kh.p_st[ki].cur_point,  kh.p_st[ki].levelBoundaryValue[triggerLevelPos]);
    // DBG_PRINTF("DKS: %d < %d, %d\r\n", currentKeyStatus->currentpoint, dks->s_e_point, (currentKeyStatus->currentpoint < dks->s_e_point));
    /* key trigger status */
    switch (kh.p_st[ki].trg_phy)
    {
        //case KEY_STATUS_D2U:
        //{
        //}
        case KEY_STATUS_U2U:
        {
            /* action 0 */
            uint16_t startPoint = KEY_LVL_GET_VAL(ki, triggerLevelPos);
            ret = (val < startPoint);
            
            if (ret > kh.p_st[ki].actionStateLast)
            {
                dks_action_push_queue(dks, DKS_ACT_0);
                kh.p_st[ki].advTrg[kcmid].dks.actIsIntact = false; //clear
                kh.p_st[ki].advTrg[kcmid].dks.isStart     = true;
                kh.p_st[ki].trg_phy = KEY_STATUS_D2D;
                //DBG_PRINTF("DKS_ACT_0\r\n");
            }
        }
        break;
        //case KEY_STATUS_U2D:
        //{
        //}
        case KEY_STATUS_D2D:
        {
            kh.p_st[ki].advTrg[kcmid].dks.isStart = true;
            /* action 1 */
            bool isBottomOk = false;
            bool isBottomRaiseOk = false;
            if (kh.p_st[ki].advTrg[kcmid].dks.curIsBottom == false)
            {
                int8_t bottomLevel   = sw_config_table[sw].maxLevel - dks->toBotLevel;
                bottomLevel          = (bottomLevel - 1) <= 0 ? 0 : (bottomLevel - 1);
                uint16_t bottomPoint = KEY_LVL_GET_VAL(ki, bottomLevel);
                isBottomOk           = (val < bottomPoint);
            }
            else /* action 2 */
            {
                int8_t bottomRaiseLevel       = sw_config_table[sw].maxLevel - dks->toBotLevel + KEY_DFT_DKS_TRG_RAISE_OFFSET_LEVEL(sw);
                bottomRaiseLevel              = (bottomRaiseLevel) < 0 ? 0 : (bottomRaiseLevel);
                uint8_t bottomRaiseLevelPoint = (bottomRaiseLevel - 1) <= 0 ? 0 : (bottomRaiseLevel - 1); /* point level */
                uint16_t bottomRaisePoint     = KEY_LVL_GET_VAL(ki, bottomRaiseLevelPoint);
                isBottomRaiseOk               = (val > bottomRaisePoint);
                
                /* dead point */
                uint16_t bottmDeadPoint = KEY_LVL_GET_VAL(ki, sw_config_table[sw].maxLevel - kh.p_attr[ki].deadbandBottom - 1);
                isBottomRaiseOk         = (val < bottmDeadPoint) ? false : isBottomRaiseOk;
            }
            if (kh.p_st[ki].advTrg[kcmid].dks.curIsBottom == false && (isBottomOk == true || (val < KEY_LVL_GET_VAL(ki, sw_config_table[sw].maxLevel - 1))))
            {
                dks_action_push_queue(dks, DKS_ACT_1);
                kh.p_st[ki].advTrg[kcmid].dks.curIsBottom = true;
                //DBG_PRINTF("DKS_ACT_1\r\n");
            }
            else if (kh.p_st[ki].advTrg[kcmid].dks.curIsBottom == true && isBottomRaiseOk == true)
            {
                dks_action_push_queue(dks, DKS_ACT_2);
                kh.p_st[ki].advTrg[kcmid].dks.curIsBottom = false; //clear
                kh.p_st[ki].advTrg[kcmid].dks.actIsIntact = true;
                //DBG_PRINTF("DKS_ACT_2\r\n");
            }

            /* action 3 */
            uint8_t endLevel      = (dks->toTopLevel + KEY_DFT_DKS_TRG_RAISE_OFFSET_LEVEL(sw)) < 0 ? 0 : (dks->toTopLevel + KEY_DFT_DKS_TRG_RAISE_OFFSET_LEVEL(sw));
            uint8_t endLevelPoint = (endLevel - 1) <= 0 ? 0 : (endLevel - 1); /* point level */
            if (endLevelPoint == 0) // 0.1特殊处理，不需要置顶条件
            {
                ret = (val < kh.p_st[ki].topRaisePoint);
            }
            else
            {
                ret = (val < KEY_LVL_GET_VAL(ki, endLevelPoint));
            }
            
            if (ret < kh.p_st[ki].actionStateLast)
            {
                if (kh.p_st[ki].advTrg[kcmid].dks.actIsIntact == true)
                {
                    kh.p_st[ki].advTrg[kcmid].dks.actIsIntact = false;
                    dks_action_push_queue(dks, DKS_ACT_3);
                }
                else
                {
                    uint8_t tmpEvent[KEY_DFT_DKS_VIRTUAL_KEY_NUMBER] = {0};
                    for (uint8_t i = 0; i < KEY_DFT_DKS_VIRTUAL_KEY_NUMBER; i++)
                    {
                        tmpEvent[i]              = dks->keyEvent[i].buffer;
                        dks->keyEvent[i].buffer &= 0xC3; //clear 1~2 event
                    }
                    dks_action_push_queue(dks, DKS_ACT_3);
                    memcpy(dks->keyEvent, tmpEvent, ARRAY_SIZE(tmpEvent));
                }
                //DBG_PRINTF("DKS_ACT_3\r\n");
                kh.p_st[ki].advTrg[kcmid].dks.isStart = false;
                kh.p_st[ki].trg_phy = KEY_STATUS_U2U;
            }
        }
        break;
        default:
            break;
    }

    return ret;
}

void key_dks_init(kh_t *p_kh)
{
    dks_kb_report = sk_get_kb_report();
    kb_report     = matrix_get_kb_report();
    ms_report     = matrix_get_ms_report();
    kh.p_gst->dksBindUsedNumber = 0;

    dks_check_para(p_kh->p_attr);
    for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        /* DKS used table */
        for (uint8_t kcmid = 0; kcmid < BOARD_KCM_MAX; kcmid++)
        {
            memset(kh.p_st[ki].advTrg, false, ARRAY_SIZE(kh.p_st[ki].advTrg[0].buff));
            if (kh.p_attr[ki].advTrg[kcmid].advType == KEY_ACT_DKS)
            {
                uint8_t index = kh.p_gst->dksBindUsedNumber;
                kh.p_gst->dksBindUsedTable[index] = ki;
                kh.p_gst->dksBindUsedNumber      += 1;
            }
        }
    }

    for (uint8_t i = 0; i < KEY_DFT_DKS_VIRTUAL_KEY_NUMBER; i++)
    {
        dks_link_init(&dksLinkQueue[i], i);
    }

    /* debug tick */
    // kh.p_qty->dksTick.type = TICK_RANDOM;
    // kh.p_qty->dksTick.min  = 30;
    // kh.p_qty->dksTick.max  = 40;

    kh.p_qty->dksTick.type = TICK_STATIC;
    kh.p_qty->dksTick.min  = 100;
    
    
    //app_timer_create(&dksTickTime, APP_TIMER_MODE_SINGLE_SHOT, dks_tick_handle);
    if (kh.p_qty->dksTick.type == TICK_STATIC)
    {
        dks_tick_static_output();
    }
    else if (kh.p_qty->dksTick.type == TICK_RANDOM)
    {
        dks_tick_random_output();
    }

    // /* debug dks */
    // memset(debugDks.keyCode, 0xFF, ARRAY_SIZE(debugDks.keyCode));
    // memset(debugDks.keyEvent, 0x00, ARRAY_SIZE(debugDks.keyEvent));

    // debugDks.startLevel  = 1;
    // debugDks.bottomLevel = 35;

    // debugDks.keyCode[0] = 81;
    // debugDks.keyEvent[0].buffer = 0x03;

    // debugDks.keyCode[1] = 82;
    // debugDks.keyEvent[1].buffer = 0xFF;

    // debugDks.keyCode[2] = 47;
    // debugDks.keyEvent[2].buffer = 0xFF;

    // debugDks.keyCode[3] = 46;
    // debugDks.keyEvent[3].buffer = 0xFF;

    // //if(kh.p_attr[46].advType == KEY_ACT_UND)
    // {
    //     kh.p_attr[46].advType = KEY_ACT_DKS;
    //     kh.p_attr[46].advTriggerPara.dks = &debugDks;
    // }
}
