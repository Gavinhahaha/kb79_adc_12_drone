#ifndef __HIVE_H__
#define __HIVE_H__

#include "app_err_code.h"

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    HIVE_SUCCESS              = 0, // Successful
    HIVE_ERROR_GENERIC        = (MG_ERR_BASE_HIVE                       +  0x06),
    HIVE_ERROR_INVALID_PARAM  = (MG_ERR_BASE_HIVE                       +  0x07),      ///< Invalid Parameter
    HIVE_ERROR_INVALID_LENGTH = (MG_ERR_BASE_HIVE                       +  0x09),      ///< Invalid Length
    HIVE_ERROR_DATA_SIZE      = (MG_ERR_BASE_HIVE                       +  0x0C),      ///< Data    size exceeds limit
    HIVE_RET_MAX
} mg_hive_ret;

enum {
    CMD_OFFSET_CMD      = 0,
    CMD_OFFSET_SCMD     = 1,
    CMD_OFFSET_LEN      = 2,
    CMD_OFFSET_RESERVE  = 4,
    CMD_OFFSET_CS       = 6,//CRC SUM
    CMD_OFFSET_DATA     = 8,

    CMD_HEAD_SIZE       = 8,
    CMD_DATA_MAX        = 56,
};

enum cmd_main_e {
    CMD_SYS         = 0x01,
    CMD_KC          = 0x02,
    CMD_KR          = 0x03,
    CMD_SK          = 0x04,
    CMD_LA          = 0x06,
    CMD_LB_BACK     = 0x07,
    CMD_NOTIFY      = 0x08,
    CMD_LB_SIDE     = 0x09,
    CMD_KS          = 0x0A,
    CMD_AMB         = 0x0B,
    CMD_GP          = 0x0C,
    CMD_DPM         = 0x0D,
    CMD_ENC         = 0x0E,
    CMD_LS          = 0x0F,
    CMD_DEBUG       = 0xDE,
    CMD_ERROR       = 0xEE,
    CMD_FAC         = 0xFA,
};

enum cmd_sys_e{
    SYS_RESERVE_0        = 0X00,
    SYS_HS               = 0X01,
    SYS_RESET            = 0X02,
    SYS_REBOOT           = 0X03,
    SYS_DFU_REQ          = 0X04,
    SYS_DFU_TRANS        = 0X05,
    SYS_GET_FA_INFO      = 0X06,
    SYS_GET_VER_INFO     = 0X07,
    SYS_GET_DEV_LIST     = 0X08,
    SYS_GET_DEV_NAME     = 0X09,
    SYS_SET_DEV_NAME     = 0X0A,
    SYS_GET_DEV_STATE    = 0X0B,
    SYS_GET_PS_MODE      = 0X0C,
    SYS_SET_PS_MODE      = 0X0D,
    SYS_GET_SLEEP_TIME   = 0X0E,
    SYS_SET_SLEEP_TIME   = 0X0F,
    SYS_GET_SN_CODE      = 0X10,
    SYS_SET_SN_CODE      = 0X11,
    SYS_GET_HIVE_VER     = 0X12,
    SYS_GET_CFG_IDX      = 0X13,
    SYS_SET_CFG_IDX      = 0X14,
    SYS_GET_CFG          = 0X15,
    SYS_SET_CFG          = 0X16,
    SYS_GET_CFG_NAME     = 0X17,
    SYS_SET_CFG_NAME     = 0X18,
    SYS_GET_CFG_SIZE     = 0X19,
    SYS_SET_PLAYER_NUMBER  = 0X1B,
    SYS_GET_PLAYER_NUMBER  = 0X1C,
    SYS_GET_SLAVE_INFO     = 0X1D,
    SYS_GET_STARTUP_EFF_EN = 0X1E,
    SYS_SET_STARTUP_EFF_EN = 0X1F,
    SYS_SET_VOLT_UP_EN     = 0X20,
    SYS_GET_NETBAR_MODE    = 0X29,
    SYS_SET_NETBAR_MODE    = 0X2A,
    SYS_CMD_MAX,
};


typedef enum {
    PS_MODE_PS           = 0X00,
    PS_MODE_STANDER      = 0X01,
    PS_MODE_GAME         = 0X02,
    PS_NOT_SUPPORT       = 0XFF,
}ps_e_t;

enum cmd_kc_e{
    KC_GET_SIZE          = 0X00,
    KC_GET_CUR_KCM       = 0X01,
    KC_SET_CUR_KCM       = 0X02,
    KC_GET_KC            = 0X03,
    KC_SET_KC            = 0X04,
    KC_SET_DEFAULT       = 0X05,
    KC_CMD_MAX,
};
    
enum cmd_kr_e{
    KR_GET_SIZE          = 0X00,
    KR_GET_CUR_KRM       = 0X01,
    KR_SET_CUR_KRM       = 0X02,
    KR_GET_KR            = 0X03,
    KR_SET_KR            = 0X04,
    KR_SET_DEFAULT       = 0X05,
    KR_CMD_MAX,
};
    
enum cmd_kf_e{
    SK_GET_HEADER        = 0X00,
    SK_GET_DKS           = 0X01,
    SK_SET_DKS           = 0X02,
    SK_GET_ADV           = 0X03,
    SK_SET_ADV           = 0X04,
    SK_GET_MACRO         = 0X05,
    SK_SET_MACRO         = 0X06,
    SK_SET_DKS_TICK      = 0X07,
    SK_GET_DKS_TICK      = 0X08,
    SK_SET_SOCD          = 0X09,
    SK_GET_SNAP_TAP      = 0X0a,
    SK_SET_HT            = 0X0b,
    SK_GET_HT            = 0X0c,
    SK_SET_RS            = 0X0d,
    SK_GET_RS            = 0X0e,
    
    SK_CMD_MAX,
};
    
enum cmd_km_e{
    KM_GET_SIZE          = 0X00,
        
    KM_GET_KMM           = 0X01,
    KM_SET_KMM           = 0X02,
    
    KM_GET_KM            = 0X03,
    KM_SET_KM            = 0X04,
    
    KM_GET_LM_SIZE       = 0X05,
    KM_GET_LM            = 0X06,
    KM_SET_LM            = 0X07,
    
    KM_TOG_KMM           = 0X08,
    KM_CMD_MAX,
};
    
enum cmd_lb_e{
    LB_BACK_GET_SATE          = 0X00,
    LB_BACK_SET_SATE          = 0X01,
    LB_BACK_GET_PARA          = 0X02,
    LB_BACK_SET_PARA          = 0X03,
    LB_BACK_SET_LIST_ID       = 0X04,
    LB_BACK_GET_SHAPE         = 0X05,
    LB_BACK_SET_SHAPE         = 0X06,
    LB_BACK_CMD_MAX,
};

enum cmd_amb_e{
    LB_SIDE_GET_SATE          = LB_BACK_GET_SATE,
    LB_SIDE_SET_SATE          = LB_BACK_SET_SATE,
    LB_SIDE_GET_PARA          = LB_BACK_GET_PARA,
    LB_SIDE_SET_PARA          = LB_BACK_SET_PARA,
    LB_SIDE_SET_LIST_ID       = LB_BACK_SET_LIST_ID,
    LB_SIDE_CMD_MAX           = LB_BACK_CMD_MAX,
};

typedef enum {
    NTF_RESERVE_0        = 0X00,
    NTF_RESERVE_1        = 0X01,
    NTF_KR               = 0X02,
    NTF_KM               = 0X03,
    NTF_LB               = 0X04,
    NTF_KF               = 0X05,
    NTF_PAIR             = 0X06,
    NTF_RADIO            = 0X07,
    NTF_INFO             = 0X08,
    NTF_LVL_ADC          = 0X09,
    NTF_CALI1            = 0X0A,
    NTF_CALI2            = 0X0B,
    NTF_MACRO            = 0X0C,
    NTF_ADV_LOCK         = 0X0D,
    NTF_ADV_APP          = 0X0E,
    NTF_ADV_SCENE        = 0X0F,
    NTF_HS               = 0X10,
    NTF_KEY_DBG          = 0X11,
    NTF_SLAVE_INFO       = 0X14,
    NTF_ERR_INFO         = 0X15,
    NTF_VOLT             = 0X16,
    NTF_LB_SIDE          = 0X17,
    NTF_DPM_SYNC         = 0X18,
    NTF_KEY_TRIGGER_SHOW = 0X19,
    NTF_SW_FAULT         = 0X1A,
    NTF_RPT_CAIL         = 0X1F,
    NTF_CMD_MAX          = 0XFF,
}cmd_ntf_e;
    
enum cmd_bt_e{
    BT_RESERVE          = 0X00,
    BT_CMD_MAX,
};

enum cmd_ks_e 
{
    KS_SET_ACT_POINT       = 0X00,
    KS_GET_ACT_POINT       = 0X01,
    KS_SET_RT_PARA         = 0X02,
    KS_GET_RT_PARA         = 0X03,
    KS_SET_LOW_LATENCY     = 0X04,
    KS_GET_LOW_LATENCY     = 0X05,
    KS_SET_GAME_TICKRATE   = 0X06,
    KS_GET_GAME_TICKRATE   = 0X07,
    KS_SET_DKS             = 0X08,
    KS_GET_DKS             = 0X09,
    KS_SET_DKS_ACT_POINT   = 0X0A,
    KS_GET_DKS_ACT_POINT   = 0X0B,
    KS_SET_MT              = 0X0C,
    KS_GET_MT              = 0X0D,
    KS_SET_TGL             = 0X0E,
    KS_GET_TGL             = 0X0F,
    KS_SET_PS              = 0X10,
    KS_GET_PS              = 0X11,
    KS_GET_DKS_SIZE        = 0X12,
    KS_SET_KEY_REPORT      = 0X13,
    KS_GET_KEY_REPORT      = 0X14,
    KS_SET_CALI            = 0X15,
    KS_GET_CALI            = 0X16,
    KS_GET_AP_RES          = 0X17,
    KS_GET_DKS_LIST        = 0X18,
    KS_SET_DZ              = 0X19,
    KS_GET_DZ              = 0X1A,
    KS_SET_PR              = 0X1B,
    KS_GET_PR              = 0X1C,
    KS_SET_NKRO            = 0X1D,
    KS_GET_NKRO            = 0X1E,
    KS_GET_VER_INFO        = 0X1F,
    KS_SET_TUNING          = 0X20,
    KS_GET_TUNING          = 0X21,
    KS_SET_SHAFT_TYPE      = 0X22,
    KS_GET_SHAFT_TYPE      = 0X23,
    KS_GET_WAVE_MAX        = 0X24,
    KS_SET_RT_SEP          = 0X25,
    KS_GET_RT_SEP          = 0X26,
    
    KS_SET_TEST            = 0x5A,
    KS_GET_TEST            = 0x5B,
    KS_CMD_MAX
};

enum cmd_dpm_e
{
    DPM_CUSTOM_PIC_DL = 0,
    DPM_DATA_SYNC = 1,
};

enum cmd_enc_e
{
    ENC_SET_FUNC = 0,
    ENC_GET_FUNC,
};

typedef enum
{
    SYNC_RGB_NULL = 0,
    SYNC_GET_CONFIG,
    SYNC_GET_ARRAY,
    SYNC_SET_DATA,
    SYNC_GET_DATA,
    SYNC_SET_MODE,
    SYNC_SET_MULTI,
    SYNC_SET_PART
} cmd_sync_e;

enum cmd_dbg_e
{
    KEY_DBG_SW = 0x02,
    KEY_DBG_ID,
    KEY_DBG_RATE,
    KEY_LOG_CAIL_RPT = 0x08,
    KEY_DBG_MAX,
};
typedef enum {
    ADV_OPT_DEL = 0,
    ADV_OPT_NEW,
    ADV_OPT_MDF,
    ADV_OPT_MAX,
}adv_opt_t;

typedef enum 
{
    M_W_DEL = 0, //删除
    M_W_NEW,      //新增
    M_W_MDF,//修改参数
    M_W_TRX,//续传分包数据
    M_W_MAX,
}macro_w_t;

enum 
{
    M_TRANS_SUCCESS = 0,
    M_TOTAL_FRAMES_ERROR,
    M_PID_ERROR,
    M_CMD_ERROR,
    M_NUMBER_ERROR,
};

typedef enum 
{
    M_R_NEW = 0, //重新开始读取
    M_R_TRX,       //续传分包数据
    M_R_ERR_END,//读取参数错误
    M_R_MAX,
}macro_r_t;

typedef enum {
    REQ_START    = 0,//请求开始//出错了可以请求重新开始
    REQ_BLOCK_0  = 1,//开始传输
    REQ_BLOCK_1  = 2,//开始传输
    REQ_BLOCK_2  = 3,//开始传输
    REQ_BLOCK_3  = 4,//开始传输
    REQ_FINISH   = 5,//传输完成
    REQ_MAX      = 6,
}cfg_req_t;

typedef struct _cmd_head_s {
    uint8_t  cmd;       /* Command */
    uint8_t  scmd;      /* Sub Command */
    uint16_t len;       /* Length */
    uint16_t reserve;   /* Offset */
    uint16_t cs;        /* Checksum */
} cmdh_t;

typedef enum
{
    FACTORY_SET_MODE   = 0x00,
    FACTORY_GET_MODE   = 0x01,
    FACTORY_SET_COLOR  = 0x10,
    FACTORY_GET_RSSI   = 0x20,
    FACTORY_GET_VERIFY = 0x30,
    FACTORY_GET_TOUCH  = 0x50,
    FACTORY_SET_MAX,
} fac_cmd_t;



typedef enum 
{
    PIC_TRANS_START    = 0,   //图片传输请求开始
    PIC_TRANS_CANCEL   = 1,   //图片传输取消
    PIC_TRANS_PROG     = 2,   //图片传输进行中
    PIC_TRANS_FINISH   = 3,   //图片传输结束
}pic_trans_state_e;

typedef struct
{
    uint32_t total_pkg_num;
    uint16_t payload_max_len;
    uint32_t curr_pkg_index;
    uint16_t last_payload_len;
}packet_trans_info_t;

typedef struct __attribute__((packed))
{
    uint8_t start_flag;
    uint8_t pic_id;
    uint8_t type;          //0:jpg 1:png 2:mp4
    uint32_t total_size;
    packet_trans_info_t group_info;
    packet_trans_info_t sub_info;
    uint8_t *p_buf;
}pic_trans_info_t;


typedef uint32_t (*cmd_func)(uint16_t len, uint16_t reserve, uint8_t *pdata);

typedef struct _cmd_pro {
    uint8_t   cmd;
    uint8_t   scmd;
    uint16_t  len;
    cmd_func  func;
} cmd_pro;

typedef struct _hive_data_s {
    uint8_t *buf;
    uint16_t size;
} hive_data_t;

typedef enum
{
    OFFLINE = 0,
    ONLINE_CONSUMER,
    ONLINE_FACTORY,
} hive_state_t;

typedef struct
{
    uint8_t sys_frame_mode;
    uint8_t sys_err_status;
    uint8_t volt_up_en;
    uint16_t volt;
    uint8_t volt_err_level;
} sys_run_info_s;

extern sys_run_info_s sys_run;


void hive_init(void);
void hive_deinit(void);

uint32_t cmd_task(void);
uint32_t cmd_send_data(uint8_t *pdata, uint8_t len);
uint32_t cmd_do_response(uint8_t cmd, uint8_t subcmd, uint16_t len, uint16_t reserve, uint8_t *pdata);
uint32_t do_error_report(uint8_t cmd, uint8_t scmd, uint16_t len, uint16_t reserve, uint16_t cs, uint32_t error_num);

hive_state_t get_hive_state(void);
uint16_t get_crc(uint16_t len, uint8_t *pbuf);

uint32_t cmd_trans_bt_dfu(uint8_t *pdata, uint8_t len);
bool is_crc_right(uint16_t crc, uint16_t len, uint8_t *pbuf);
void cmd_set_patch_callback(void(*ext_fnc)(uint8_t));
uint32_t cmd_hive_send_adc_rpt(uint8_t *pdata, uint8_t len);
uint32_t cmd_hive_send_adc_cail_rpt(uint8_t *pdata, uint8_t len);
uint32_t cmd_hive_check_alive(void);
uint32_t cmd_hive_send_cali_rpt(uint8_t *pdata, uint8_t len);
uint32_t cmd_hive_send_key_dbg(uint8_t *pdata, uint8_t len);

#endif

