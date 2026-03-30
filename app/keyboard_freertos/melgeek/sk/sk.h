#ifndef  _SK_H_
#define  _SK_H_


#include <stdint.h>
#include <stdbool.h>
#include "sk_macro.h"
#include "sk_adv.h"
#include "sk_dks.h"
#include "interface.h"



#define SK_DKS_NUM          (8)
#define SK_ADV_NUM          (16)
#define SK_MACRO_NUM        (8)
#define SK_SOCD_NUM         (8)
#define SK_HT_NUM           (8)
#define SK_RS_NUM           (8)

#define SK_TOTAL_NUM        (SK_DKS_NUM + SK_ADV_NUM + SK_MACRO_NUM + SK_SOCD_NUM + SK_HT_NUM + SK_RS_NUM)
#define SK_NAME_LEN         (16)

typedef enum 
{
    SK_EVT_NONE = 0,
    SK_EVT_ADV,
    SK_EVT_MACRO,
    SK_EVT_DKS,
    SK_EVT_MAX,
}sk_evt_t;

typedef enum _sk_type_s
{
    SKT_DKS = 0,
    SKT_ADV,
    SKT_MACRO,
    SKT_SOCD,
    SKT_HT,
    SKT_RS,
    SKT_MAX,
}skt_t;


//super key header(dks shortcut macro)
typedef struct 
{
    skt_t    type;          //类型
    uint8_t  total_size;    //总容量
    uint8_t  free_size;     //剩余容量
    uint32_t storage_mask;  //存储表,一个bit代表一块存储区;bit0:表示第0块存储空间是否已有存储信息;
    union 
    {
        uint32_t ex;
        uint32_t macro_max_frame;            //扩展信息//一条宏的最大支持帧数
    }sk_ex;
} skh_t;

//dks config
typedef struct _dks_s {
    uint8_t name[16];
    key_dks_t para;
} dks_t;

//socd tap
typedef struct _socd_s {
    uint8_t ki[2];//当里面的值是0xff,或者相等,代表无效
    socd_mode mode;
    uint8_t sw;
} socd_t;

//hyper tap
typedef struct _ht_s {
    uint8_t ki;//当里面的值是0xff,或者相等,代表无效
    kc_t kc;
} __attribute__ ((packed)) ht_t;

//rs 
typedef struct _rs_s {
    uint8_t ki[2];//当里面的值是0xff,或者相等,代表无效
} __attribute__ ((packed)) rs_t;

typedef struct {
    skh_t       superkey_table[SKT_MAX];
    dks_t       dks_table[SK_DKS_NUM];
    adv_t       adv_table[SK_ADV_NUM];
    socd_t      socd[SK_SOCD_NUM];
    ht_t        ht[SK_HT_NUM];
    rs_t        rs[SK_RS_NUM];
} __attribute__ ((aligned (4))) sk_t;


void sk_init(void);
void sk_task_notify(void);
void sk_set_default(skt_t type);
void sk_judge_evt(uint8_t ki, uint8_t state);
rep_hid_t *sk_get_kb_report(void);

void sk_rep_kb_change(void);
bool sk_rep_kb_get_change(void);
bool sk_rep_kb_is_change(void);
void sk_rep_ms_change(void);
bool sk_rep_ms_is_change(void);
//void sk_check_para(sk_la_lm_kc_info_t *info);

#endif
