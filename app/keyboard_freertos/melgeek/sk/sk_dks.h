#ifndef  _SK_DKS_H_
#define  _SK_DKS_H_

#include "mg_matrix_type.h"


#define DKS_FIFO_NUM (32)
#define DKS_FIFO_SIZE (sizeof(DksData_t))

typedef enum
{
    DKS_D2U = false,
    DKS_U2D = true,
} DksActionType_e;
typedef enum
{
    DKS_ACT_0 = 0,
    DKS_ACT_1,
    DKS_ACT_2,
    DKS_ACT_3,
} DksActionNum_e;

typedef struct
{
    dynamic_key_t code;
    DksActionType_e status : 1;
    uint32_t ts            : 10;
    uint32_t reserver      : 5;
} DksData_t;

typedef struct dks_node
{
    struct dks_node *next;
    DksData_t data;
} DksNode_t;

typedef struct 
{
    DksNode_t *front;
    DksNode_t *rear;
    uint8_t max;
    uint8_t len;
} DksLinkQueue_t;




void key_dks_init(kh_t *p_kh);
bool feature_dks_bind(uint8_t kcmid, uint8_t ki, uint8_t paraTableId);
bool feature_dks_unbind(uint8_t kcmid, uint8_t ki);
void feature_dks_set_tick_para(dks_tick_type tickType, uint8_t tickMin, uint8_t tickMax);
void feature_dks_trigger_default_para(void);
bool feature_dks_output_result(void);
void feature_dks_check_exception_handle(uint8_t kcmid, uint8_t ki);
bool feature_dks_handle(uint8_t kcmid, uint8_t ki, uint16_t val);
bool dks_dispatch_keyevent(void);

#endif
