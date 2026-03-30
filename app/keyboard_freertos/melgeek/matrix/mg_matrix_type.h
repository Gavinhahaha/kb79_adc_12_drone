/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#ifndef _MG_MATRIX_TYPE_H_
#define _MG_MATRIX_TYPE_H_

#include "hal_gpio.h"
#include "hal_adc.h"
#include <stdio.h>
#include <stdarg.h>
#include "board.h"
#include "hpm_common.h"
#include "mg_key_code.h"
#include "mg_matrix_config.h"
#include "app_err_code.h"
#include "mg_hive.h"


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)         ( sizeof(arr) / sizeof((arr)[0]) )
#endif

#define ARRAY_SIZE_TWO(arr)     ( sizeof(arr) / sizeof((arr)[0][0]) )
#define ABS(x)                  ( (x) > 0 ? (x) : (-(x)) )
#define ABS2(x,y)               ( (x) > (y) ? (x) - (y) : (y) - (x) )

#define ADC12B_TO_VOLT              (0.0008056640625f)
#define ADC12B_DATA_REVERSE(val)    (4095 - (val))
#define ADC12B_MAX_VAL              (4095)

#define ADC16B_DATA_REVERSE(val)    (65535 - (val))
#define ADC16B_MAX_VAL              (65535)


static inline void reverseArray(uint16_t *arr, uint16_t size)
{
    uint16_t temp, start = 0, end = size - 1;  
    while (start < end)
    {
        temp       = arr[start];  
        arr[start] = arr[end];  
        arr[end]   = temp;  
  
        start++;  
        end--;  
    }  
}  
static inline uint16_t binary_search_target_range_index(const uint16_t *arr, const uint16_t length, const uint16_t target, bool isAscending) 
{
   // uint8_t count = 0;
    uint16_t left = 0, mid = 0, right = length - 1, insertIndex = length;

    while (left <= right)
    {
     //   count++;
        mid = left + (right - left) / 2;

        if (arr[mid] == target)
        {
            insertIndex = mid;
            break;
        }
        else if ((isAscending && arr[mid] < target) || (!isAscending && arr[mid] > target))
        {
            left = mid + 1;
        }
        else
        {
            right = mid - 1;
            insertIndex = mid; // 记录位置为左侧
        }
    }
   // log_debug("target:%d, tag pos:%d, calc count:%d, left:%d, right:%d, \r\n", target, insertIndex, count, left, right);
    return insertIndex;
}
static inline uint16_t stringToHex(const uint8_t* input, uint8_t* hexArray, uint16_t maxElements) 
{
    const char* delimiter = " ";
    char* token;
    uint16_t index = 0;
    // 使用 strtok 函数分割字符串
    token = strtok((char*)input, delimiter);
    // 遍历子字符串并将其转换为十六进制
    while (token != NULL && index < maxElements) 
    {
        sscanf(token, "%x", (int *)&hexArray[index]);
        index++;
        token = strtok(NULL, delimiter);
    }
    return index;
}
static inline uint16_t swap_halfword(uint16_t data)
{
    return ((data&0x00FF)<<8) | ((data&0xFF00)>>8);
}
static inline uint8_t reverse_byte(uint8_t data)
{
	data=(data<<4)|(data>>4);
	data=((data<<2)&0xcc)|((data>>2)&0x33);
	data=((data<<1)&0xaa)|((data>>1)&0x55);
	return data;
}
static inline uint8_t crc_sum_u8(uint8_t *ptrBuffer, uint32_t lenght)
{
    uint8_t crc = 0;

    for(uint32_t i=0; i<lenght; i++)
    {
        crc += *ptrBuffer;
        ptrBuffer += 1;
    }
    return crc;
}
static inline uint8_t crc_sumxor_u8(uint8_t *ptrBuffer, uint32_t lenght)
{
    uint8_t crc = crc_sum_u8(ptrBuffer, lenght);
    crc ^= 0xFF;
    return crc;
}

static inline uint16_t crc_sum_u16(uint8_t *ptrBuffer, uint32_t lenght)
{
    uint16_t crc = 0;

    for(uint32_t i=0; i<lenght; i++)
    {
        crc += *ptrBuffer;
        ptrBuffer += 1;
    }
    return crc;
}
static inline uint16_t crc_sumxor_u16(uint8_t *ptrBuffer, uint32_t lenght)
{
    uint16_t crc = crc_sum_u16(ptrBuffer, lenght);
    crc ^= 0xFFFF;
    return crc;
}
#if 0
/**
 * @brief  实现itoa函数的源码 整形转字符
 * @param  num: 要转换的整数
 * @param  ptrStr: 转换后的字符串
 * @param  radix: 转换进制数，如2,8,10,16 进制等
 * @retval 转换后的字符串
 */
static inline char *itoa(int num, char *ptrStr, int radix)
{
	/* 索引表 */
	char index[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	unsigned unum; /* 中间变量 */
	int i = 0, j, k;
	/* 确定unum的值 */
	if (radix == 10 && num < 0) /* 十进制负数 */
	{
		unum = (unsigned)-num;
		ptrStr[i++] = '-';
	}
	else
		unum = (unsigned)num; /* 其它情况 */
	/* 逆序 */
	do
	{
		ptrStr[i++] = index[unum % (unsigned)radix];
		unum /= radix;
	} while (unum);
	ptrStr[i] = '\0';
	/* 转换 */
	if (ptrStr[0] == '-')
		k = 1; /* 十进制负数 */
	else
		k = 0;
	/* 将原来的“/2”改为“/2.0”，保证当num在16~255之间，radix等于16时，也能得到正确结果 */
	char temp;
	for (j = k; j <= (i - k - 1) / 2.0; j++)
	{
		temp = ptrStr[j];
		ptrStr[j] = ptrStr[i - j - 1];
		ptrStr[i - j - 1] = temp;
	}
	return ptrStr;
}
#endif

typedef enum
{
    KEY_DATA_TOTAL_FRAME = 0x00,
    KEY_DATA_CHANGE_FRAME,
    KEY_DATA_THRESHOLD_FRAME,
    KEY_DATA_ZERO_FRAME,
    KEY_DATA_FRAME_MAX,
} key_data_frame_e;

typedef enum
{
    KEY_REPORT_OUTTYPE_CLOSE = 0x00,
    KEY_REPORT_OUTTYPE_TRIGGER,
    KEY_REPORT_OUTTYPE_REALTIME,
    KEY_REPORT_OUTTYPE_MAX,
} key_rpt_e;

typedef enum 
{
    KEY_STATUS_U2U = 0,
    KEY_STATUS_U2D,
    KEY_STATUS_D2D,
    KEY_STATUS_D2U,
    KEY_STATUS_MAX,
} KeyStatusTrigger_e;
    
typedef enum kcs_e {
    KCS_NONE    = 0,     /*  */
    KCS_SP      = 1,    /* Single Press */
    KCS_LP      = 2,    /* Long Press */
}kcs_e;

typedef enum 
{
    KEY_STATUS_POS_TOP = 0,
    KEY_STATUS_POS_MIDWAY,
    KEY_STATUS_POS_BOTTOM,
} KeyStatusPosition_e;
    
typedef enum 
{
    KEY_ACT_UND = 0,
    KEY_ACT_DKS,
    KEY_ACT_MT,
    KEY_ACT_TGL,
    KEY_ACT_MAX,
} key_act_adv_e;

typedef enum
{
    KEY_GENRAL_MODE_STROKE_SAME = 0,//相等
    KEY_GENRAL_MODE_STROKE_LAG,//滞后
    KEY_GENRAL_MODE_STROKE_ADVANCE,//提前
    KEY_GENRAL_MODE_MAX,
} key_general_mode_e;
    
typedef enum 
{
    PRI_PHY = 0,
    PRI_LOGIC,
    PRI__MAX,
} key_trg_pri_e;

//按键运动方向    
typedef enum //direction of movement
{
    KEY_MD_STATIC = 0,//静止
    KEY_MD_DOWN,
    KEY_MD_UP,
    KEY_MD_MAX,
} key_md_e;
    
typedef enum
{
    TICK_STATIC = 0,//静态延迟,固定一个延迟时间
    TICK_RANDOM,//动态延迟,指定一个延迟范围,在这个范围内随机变化
    TICK_MAX,
}dks_tick_type;

typedef enum
{ 
    TUNING_OFF = 0x00,
    TUNING_ON,
    TUNING_PARA_OPERATE_MAX,
} tuning_e;

typedef enum
{ 
    TUNING_ZERO_SAMPLE = (0x01 << 0),
    TUNING_ZERO_RESULT = (0x01 << 1),
    TUNING_ZERO_SAVE   = (0x01 << 2), 
} tuning_zero_e;

typedef enum
{
    KEY_SHAFT_TYPE_UNKNOWN = 0x00, 
    KEY_SHAFT_TYPE_GATERON_WHITE,   //佳达隆 磁白轴
    KEY_SHAFT_TYPE_GATERON_JADE,    //佳达隆 磁玉轴 
    KEY_SHAFT_TYPE_KAILH_GREEN,     //凯华   青蜂轴  
    KEY_SHAFT_TYPE_JW_NMF,          //禾金 糯米饭轴
    KEY_SHAFT_TYPE_TTC_WCW,         //TTC 万磁王轴
    KEY_SHAFT_TYPE_GATERON_JPRO,    //佳达隆 磁玉pro
    KEY_SHAFT_TYPE_GATERON_JMAX,    //佳达隆 磁玉max
    KEY_SHAFT_TYPE_GATERON_JGAMING, //佳达隆 磁玉gaming
    KEY_SHAFT_TYPE_TTC_WMIN,        //TTC 万磁王mini轴(矮轴)
    KEY_SHAFT_TYPE_TTC_WRGB,        //TTC 万磁王RGB
    KEY_SHAFT_TYPE_TTC_TSTD,        //TTC 天王轴标准版
    KEY_SHAFT_TYPE_TTC_TSE,         //TTC 天王轴SE版
    KEY_SHAFT_TYPE_TTC_TGAMING,     //TTC 天王轴GAMING
    KEY_SHAFT_TYPE_TTC_SNAKE,       //TTC 白蛇万磁王轴
    KEY_SHAFT_TYPE_TTC_TA,          //TTC 太阿轴
    KEY_SHAFT_TYPE_TTC_SX,          //TTC 圣心轴
    KEY_SHAFT_TYPE_TTC_PURPLE,      //TTC 紫心轴
    KEY_SHAFT_TYPE_TTC_FD,          //TTC 反斗轴
    KEY_SHAFT_TYPE_TTC_035,         //TTC 零35轴
    KEY_SHAFT_TYPE_TTC_FENGMI,      //TTC 蜂蜜轴
    KEY_SHAFT_TYPE_MIX_BABAO,       //WuqueStudio 八宝轴(鼓动版和a版本)
    KEY_SHAFT_TYPE_MIX_G_HUAHUO,    //佳达隆 花火轴
    KEY_SHAFT_TYPE_MIX_TAMSHAN,     //泰山磁轴(GT)
    KEY_SHAFT_TYPE_TTC_HOURSE,      //TTC 马年限定轴
    KEY_SHAFT_TYPE_MAX,
} key_sw_type_e;

typedef enum
{
    MTX_RET_SUCEESS         = 0,
    MTX_RET_STATE_ERR       = (MG_ERR_BASE_MATRIX + 0),
    MTX_RET_ATTR_ERR        =  (MG_ERR_BASE_MATRIX + 1),
    MTX_RET_MAX
}matrix_ret;
// #pragma pack(2)

typedef enum
{
    TAP_TYPE_SOCD = 0x01 << 0,
    TAP_TYPE_DRT  = 0x01 << 1,
    TAP_TYPE_RS   = 0x01 << 2,
    TAP_TYPE_MAX,
} tap_type_e;

typedef enum
{
    SOCD_MODE_LAST_INPUT = 0x00, //上一次输入优先
    SOCD_MODE_ABSOLUTE_1, //绝对优先某个键
    SOCD_MODE_ABSOLUTE_2, //绝对优先
    SOCD_MODE_NEUTRAL, //中立
    SOCD_MODE_MAX,
} socd_mode;

typedef enum
{
    RT_MODE_DEFAULT,
    RT_MODE_ADAPTIVE,
    RT_MODE_MAX,
} rt_mode;

typedef enum
{
    RT_ADA_MODE_DEFAULT,
    RT_ADA_MODE_QUICK,
    RT_ADA_MODE_MAX,
} rt_ada_mode;

typedef enum
{
    RT_ADA_SENS_STABLE,
    RT_ADA_SENS_BALANCED,
    RT_ADA_SENS_AGILE,
    RT_ADA_SENS_MAX,
} rt_ada_sens;

/* unit: mm */
typedef struct 
{
    bool voltAscend;
    bool isRtHighAcc;
    
    float nacc;
    float nacc2dist;
    uint16_t nacc2lvl;
    float hacc;
    float hacc2dist;
    uint16_t hacc2lvl;
    float realTravel;
    uint16_t maxLevel; 

    //trg and dead
    float generalTrgAcc;
    float deadAcc;
    float deadMin;
    float deadMax;

    //RT
    float rtSensAcc;
    float rtSensMin;
    float rtSensMax;

    //DKS
    float dksAcc;
    float dksToTopMin;
    float dksToTopMax;
    float dksToBotMin;
    float dksToBotMax;

    float curvTab[KEY_LEVEL_BOUNDARY_BUFF_SIZE];
} key_sw_para_t;

typedef struct 
{
    uint16_t top_max;
    uint16_t top_min;
    uint16_t bottom_max;
    uint16_t bottom_min;
    uint16_t diff_max;
    uint16_t diff_min;
    uint16_t def_diff;
}key_sw_thd_t;

typedef struct
{
    uint32_t min  :11;
    uint32_t max  :11;
    dks_tick_type type :2;
}dks_tick_t;

typedef struct _dynamic_key_s
{
    /** key code Type 键代码类型*/
    uint16_t ty:4;
    /** key code Subtype 键代码子类型*/
    uint16_t sty:4;
    /** key Code 键码*/
    uint16_t co:8;
} dynamic_key_t;

typedef struct 
{
    uint32_t toTopLevel:6;          /* 起始/结束动作触发等级, 到顶部值 */
    uint32_t toBotLevel:6;          /* 底部/从底部抬起触发等级,到底部值 */
   // uint32_t bottomRaiseLevel:6;    /* 从底部抬起触发等级 */
   // uint32_t endLevel  :6;          /* 结束动作触发等级 */
    uint32_t reserve   :4;

    dks_tick_t tick;

    /* 8Byte */
    dynamic_key_t  keyCode[KEY_DFT_DKS_VIRTUAL_KEY_NUMBER];           /* 四个动作对应的键码 */
    
    union 
    {
        uint8_t buffer; 
        struct  
        {   /* 所有动作触发事件 */
            uint8_t act0_u2d:1;
            uint8_t act0_d2u:1;
            uint8_t act1_u2d:1;
            uint8_t act1_d2u:1;
            uint8_t act2_u2d:1;
            uint8_t act2_d2u:1;
            uint8_t act3_u2d:1;
            uint8_t act3_d2u:1;
        }bit;
    }keyEvent[KEY_DFT_DKS_VIRTUAL_KEY_NUMBER];
}key_dks_t;
#if 1
typedef struct 
{
    uint16_t holdDuration;
    kc_t kc[2]; // [0]Hold、[1]Tap
}key_mt_t;

typedef struct 
{
    uint16_t holdDuration;
    kc_t kc; // Toggle
}key_tgl_t;
#endif
typedef struct 
{
    uint8_t  isContinue:1;          /* 停止快速触发的条件0: currentlevel < triggerlevel 时停止触发;1: currentlevel == 0 时停止触发*/
   // uint8_t  isAlone:1;             /* 0: 按下和松开为同一个变化量; 1：可单独配置按下和松开的行程变化量 */
    uint8_t  reserver:7;
    uint16_t pressSensitive;                /* 引起触发事件的行程变化量（按下灵敏度） */
    uint16_t raiseSensitive;                /* 引起释放事件的行程变化量（抬起灵敏度） */
}key_rt_t;

typedef struct 
{
    key_general_mode_e mode:2;      /* mode */
    uint16_t reserver:6;
}key_general_t;
#pragma pack(4)
//key_status_t
typedef struct 
{
    //KeyStatusTrigger_e  trg;       
    KeyStatusPosition_e pos;       /* 按键的位置状态 */
    key_trg_pri_e trg_pri;          /* Priority;触发优先级;0:物理触发;1:逻辑触发*/
    /* 按键的触发状态 */
    KeyStatusTrigger_e    trg_phy;                /* Physical triggering;物理触发状态:false:未触发;true:已触发*/
    
    KeyStatusTrigger_e    trg_logic;              /* Logical triggering;逻辑触发状态:false:未触发;true:已触发*/
    
    tap_type_e tap_type;
    uint8_t socd_idx;
    uint8_t socd_ki;
    uint8_t socd_ki_abs; //SOCD绝对优先模式下的按键ki
    uint8_t socd_hold;
    uint16_t socd_tick;
    uint8_t drt_ki[2];
    kc_t drt_kc;
    uint16_t drt_t1;
    uint16_t drt_t2;
    bool actionState;
    bool actionStateLast;

    bool calibState;
    bool keyDebugIsPackTop;
    bool enter_rt;
    key_md_e cur_md;                /* 当前运动方向 */
    key_md_e last_md;               /* 上次运动方向 */
    
    uint16_t topRaisePoint;         /* 用于按下和释放为1等级时处理 */
    uint16_t bottomPressPoint;

    uint8_t  cur_lvl;               /* 当前行程 */
    uint8_t  last_lvl;              /* 上次行程 */
    uint16_t debounce;              /* 消抖计数 */
    uint16_t cur_point;             /* 当前的adc */
    uint16_t last_point;            /* 上次的adc */
    uint16_t last_rls_point;        /* 上次的释放adc */
    
    uint8_t triggerAddDeadLevel;    /* 按下触发+死区 */
    uint8_t raiseAddDeadLevel;      /* 按键释放+死区 */
    
    uint16_t min;                   /* 行程起点的adc值,校准后填入 */
    uint16_t max;                   /* 行程终点的adc值,校准后填入 */
    uint16_t diff;                  /* 有效行程 max-min*/
#if (USE_KEY_LVL_MODE == KEY_LVL_MODE_BUFF)
   uint16_t levelBoundaryValue[KEY_LEVEL_BOUNDARY_BUFF_SIZE];
#endif

    uint16_t rt_start_point;
    uint16_t rt_trg_val;

    bool rtIsEnter;
    uint16_t rtAbortPoint;
    uint16_t rtUpperLimitPoint;
    uint16_t rtLowerLimitPoint;
    uint8_t rtLimitPointLevel;
    //uint8_t rtPressLowAccSens;
    //uint8_t rtRaiseLowAccSens;
    uint8_t rtPressSens;
    uint8_t rtRaiseSens;
    uint64_t rtTimestamp;
    uint64_t rt_rcv_time;
    uint64_t trg_phy_ts;
    uint8_t kv;

    //int16_t rts;    

    uint8_t kcmidNow;
    uint8_t kcmidLast;
    bool isShapeChange;

    uint64_t last_time;
    uint16_t last_val;
    bool key_press;
    
    uint8_t overspeed_cnt;
    uint16_t valid_cnt;

    uint16_t trg_down_data[5];
    uint64_t first_d2u_time;

    struct
    {
        bool isAction;             /* 按键正在动作 */
        bool isRaiseBack;          /* 按键释放往返 */       
        
        uint8_t triggerLevel;           /* 按下触发行程 */
        uint8_t raiseLevel;             /* 按键释放行程 */
    } general;

    union 
    {
        uint32_t buff[8];
        struct 
        {
            KeyStatusTrigger_e trg;
            bool isLoogPress;
            uint64_t timestamp;
        } mt;
        struct 
        {
            KeyStatusTrigger_e trg;
            bool selfLock;
            uint64_t timestamp;
        } tgl;
        struct 
        {
            uint8_t kc_idx;
            uint8_t enter[3];
            uint16_t point[6];
        } mtp;
        struct
        {
            bool curIsBottom;
            bool actIsIntact;
            bool isStart;
        } dks;
    } advTrg[BOARD_KCM_MAX]; /* gbinfo.kc.cur_kcm */
} key_st_t;
#pragma pack()

typedef struct 
{
    uint8_t keyCalibNumber;
    key_rpt_e key_rpt_type;
    uint8_t key_rpt_refresh;
   // key_sw_type_e sw[KEYBOARD_NUMBER];
    uint8_t useShaftMaxLevelId;

    /* DKS bind key table and number */
    uint8_t dksBindUsedNumber;
    uint8_t dksBindUsedTable[KEYBOARD_NUMBER];
    
    /* DKS para table and number */
   // uint8_t dksParaUsedNumber;
   // uint8_t dksParaUsedTable[KEY_DFT_DKS_PARA_TABLE_MAX];
    
    uint8_t dksRespParaTableLen;
    

    /* RT used key table and number */
    uint8_t rtKeyUsedTableNumber;
    uint8_t rtKeyUsedTable[KEYBOARD_NUMBER];

    /* dead para used key table and number */
    // uint8_t deadKeyUsedTableNumber;
    // uint8_t deadKeyUsedTable[KEYBOARD_NUMBER];
    uint8_t deadKeyRespLen;

    uint8_t keyParaRespLen;


    float upCompensation;
    float downCompensation;
    
    uint8_t key_dbg_sw;
   // uint8_t dksOutputKi[KEY_DFT_DKS_VIRTUAL_KEY_NUMBER];
   // uint8_t dksOutputKiCode[KEY_DFT_DKS_VIRTUAL_KEY_NUMBER];

    dynamic_key_t dksOutput[KEY_DFT_DKS_VIRTUAL_KEY_NUMBER];
    uint8_t dksOutputChange[KEY_DFT_DKS_VIRTUAL_KEY_NUMBER];
    
    uint8_t rt_en_num;
    uint8_t kb_is_pressed;
    uint8_t calib_is_change;
}key_glb_st_t;

//key_attr_t//需要存储
typedef struct 
{
    uint16_t triggerlevel : 6;  /* 触发行程 */
    uint16_t raiseLevel   : 6;  /* 释放行程 */
    uint16_t deadbandTop  : 3;  /* top死区 */
    uint16_t rt_en        : 1;  /* RT 快触发功能使能 */

    uint16_t triggerpoint   ;   /* 静态行程触发,触发行程对应的adc值 */
    uint16_t raisePoint     ;   /* 静态释放触发,触发行程对应的adc值 */
    uint32_t deadbandBottom : 3;    /* bottom死区 */
    uint32_t rtIsContinue   : 1;    /* RT 停止快速触发的条件0: currentlevel < triggerlevel 时停止触发;1: currentlevel == 0 时停止触发*/

    key_general_mode_e generalMode : 2; /* 常规触发 mode */
    uint32_t rtPressSensitive      : 9; /* RT 引起触发事件的行程变化量（按下灵敏度） */
    uint32_t rtRaiseSensitive      : 9; /* RT 引起释放事件的行程变化量（抬起灵敏度） */
    uint32_t shaftChange           : 1;
    uint32_t rt_mode               : 1;
    uint32_t rt_ada_extreme        : 1;

    uint8_t rt_ada_sens;
    uint8_t rt_ada_mode;

   // uint8_t tickRate;               /* 每个触发事件的最短间隔时间 */


    struct 
    {
        key_act_adv_e advType      :3;  /* 高级触发功能三选一 */
        uint8_t dksParaTableUsedId :4;
        uint8_t reserver           :1;
        
        union 
        {
            void      *ptr;
            //key_mt_t  *mt;            /* 高级触发 按住/单击 */
            //key_tgl_t *tgl;           /* 高级触发 开关 */
            key_dks_t *dks;           /* 高级触发 动态键程 */
        }para;
    }advTrg[BOARD_KCM_MAX]; /* gbinfo.kc.cur_kcm */
} key_attr_t;

typedef struct 
{
    // uint8_t isGlobalRtEnable:1;         /* 键盘RT功能使能 */
    // uint8_t isGlobalRtContinueEnable:1; /* 键盘连续RT功能使能 */
    // uint8_t reserver2:6;

   // uint8_t dksParaNumber;
    tuning_e tuning_en;                 /*自动校准使能*/
    dks_tick_t dksTick;

    uint8_t shaftChangeNum;
    
    uint8_t rt_separately_en;              /*rt分开设置参数*/
    bool cail_dbg_rpt_en;                   

    //key_sw_type_e  sw;
    //const key_shaft_para_t *shaftPara;
    
} key_qty_t;
    
// #pragma pack()
    
typedef enum
{
    KEY_CALIB_ABORT      = 0x00,
    KEY_CALIB_IDLE       = 0x00,         /* 中止校准 */
    KEY_CALIB_START      = 0x01,         /* 开始校准 */
    KEY_CALIB_DONE       = 0x02,         /* 校准完成 */
    KEY_CALIB_CLEAR      = 0x03,         /* 清除校准 */
    //KEY_CALIB_ZERO_LVL   = 0x04,         /* 调零 */
    //KEY_CALIB_POWERED    = 0x05,         /* 上电轴体检测 */
    KEY_CALIB_SWITCH_OUT = 0x06,         /* 未插轴 */
    KEY_CALIB_UNKONW     = 0xFF
} cail_step_t;
    
typedef enum
{
    KEY_CALIB_ACK_SUCCESS,
    KEY_CALIB_ACK_NOT_STARTED,
    KEY_CALIB_ACK_NOT_ALL,
    KEY_CALIB_ACK_NOT_READY,
    KEY_CALIB_ACK_NOT_SUPPORT,
    KEY_CALIB_ACK_MAX
}cail_ack_t;
    
typedef enum
{
    FAT_UNTESTED       = 0,
    FAT_PCBA_PASSED    = 1,
    FAT_NOISE_PASSED   = 1,
    FAT_WHOLE_PASSED   = 2,
    FAT_INIT           = 0xff,
    
}fat_test_t;

typedef struct
{
    uint32_t itf;

    uint16_t min[BOARD_KEY_NUM];
    uint16_t max[BOARD_KEY_NUM];
    //uint16_t travel[BOARD_KEY_NUM];
    //uint16_t levelBoundaryValue[BOARD_KEY_NUM][KEY_LEVEL_BOUNDARY_BUFF_SIZE]; /* 每等级分界值 */
    key_sw_type_e sw[BOARD_KEY_NUM];
    
    uint8_t calibDone;
    uint8_t rsv_cali[BOARD_KEY_NUM*10];
} __attribute__ ((aligned (4))) key_cali_t;

typedef struct
{
    key_st_t      *p_st;         /*运行状态*/
    key_attr_t    *p_attr;       /*设置参数*/
    key_cali_t     *p_calib;          /*校准数据*/
    key_glb_st_t  *p_gst;        /*其他数据*/
    key_qty_t     *p_qty;        /*其他数据*/
}kh_t;

typedef struct _kh_data_s {
    uint16_t *buf;
    uint16_t size;
} kh_data_t;

typedef struct _kh_rpt_s {
    uint8_t  ki;
    uint8_t  lvl;
    uint16_t adc;
}kh_rpt_t;

typedef struct _kh_rpt_cail_s {
    uint8_t   ki;
    uint8_t   level;
    uint8_t   trg_dif;
    uint8_t   rls_dif;
    uint16_t  cail_min;
    uint16_t  adc;
    uint16_t  cail_max;
}kh_rpt_cail_t;

typedef struct _kh_rpt_fifo_s {
    uint32_t  size;
    kh_rpt_t  rpt[14];
} __attribute__ ((aligned (4))) kh_rpt_fifo_t;

typedef struct _kh_rpt_cail_fifo_s {
    uint16_t       size;
    kh_rpt_cail_t  rpt[5];
} __attribute__ ((aligned (2))) kh_rpt_cail_fifo_t;

typedef struct
{
    uint16_t min[BOARD_KEY_NUM];
    uint16_t max[BOARD_KEY_NUM];
    uint16_t diff[BOARD_KEY_NUM];
    uint8_t  cnt[BOARD_KEY_NUM];
    //bool     press[BOARD_KEY_NUM];
    uint16_t max_sample_cnt;
    bool     max_sample_done;
    uint8_t  cali_res[14];
    uint8_t  calib_num;
    cail_step_t step;
    bool is_manual_cail_update;
} kcali_thd_t;


typedef struct
{
    tuning_e     en;
} cali_tuning_t;

typedef struct
{
    uint8_t ki;
    uint8_t trg;
    uint16_t adc;
    uint16_t lvl;
    uint8_t pid[4];
} __attribute__((packed, aligned(1))) key_debug_t;

typedef struct
{
    uint16_t adc;
    uint16_t lvl;
    uint8_t trg;
    uint16_t min_pick;
    uint16_t max_pick;
    uint16_t min_buffer[KS_TUNING_BUFFER_MIN_SIZE];//滤波缓存
    uint16_t max_buffer[KS_TUNING_BUFFER_SIZE];//滤波缓存
    uint16_t min_avg;//最小值缓存
    uint8_t  max_buf_idx;
    uint8_t  min_update_flg;
    uint16_t max_hold_cnt;
    uint32_t min_hold_cnt[KEYBOARD_NUMBER];
    uint8_t  min_sample_cnt[KEYBOARD_NUMBER];//
    uint32_t press_cnt[KEYBOARD_NUMBER];
}key_cdbg_t;

typedef struct
{
    uint32_t dt;
    uint32_t ut;
    uint8_t update;
    uint16_t min_pick;
    uint16_t max_pick;
    uint16_t val;
    
    uint16_t trg_time;
}key_noise_t;

extern key_debug_t key_dbg[4];
extern key_cali_t key_cali;
extern const key_sw_para_t sw_config_table[];
extern const key_sw_thd_t sw_thd_table[];
extern float kcr_table[KEY_SHAFT_TYPE_MAX][KEY_LEVEL_BOUNDARY_BUFF_SIZE];

#define BOARD_NKRO_ENABLE               (1)
#define BOARD_NKRO_DISABLE              (0)
#define KB_MAX_PRESSED_KEYS     (BOARD_KEY_NUM)

#define KB_BITMAP_MAX_KEYS              (144)
#define KB_BITMAP_REPORT_COUNT          (128)
#define KB_BITMAP_REPORT_BUF_SIZE       (19)

typedef uint32_t (*hid_callback)(uint8_t *pdata, uint8_t size);

#endif      /* _TYPE_H_ */ 

