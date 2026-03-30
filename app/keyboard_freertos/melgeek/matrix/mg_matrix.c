/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include "FreeRTOS.h"
#include "string.h"
#include "task.h"
#include "mg_matrix.h"
#include "hal_timer.h"
#include "hal_hid.h"
#include "board.h"
#include "hpm_adc16_drv.h"
#include "mg_matrix_type.h"
#include "app_debug.h"
//#include "hal_gpio.h"
#include "hpm_gpio_drv.h"
#include "mg_trg_fix.h"
#include "mg_trg_rt.h"
#include "mg_kb.h"
#include "interface.h"
#include "db.h"
#include "app_config.h"
#include "mg_hid.h"
#include "mg_hive.h"

#include "mg_uart.h"
#include "mg_sm.h"
#include "mg_cali.h"
#include "mg_filter.h"
#include "mg_detection.h"
#include "layout_fn.h"
#include "sk.h"
#include "power_save.h"


#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (DBG_EN_MATRIX)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          ""
    #define   DBG_PRINTF(fmt, ...)     Mg_SEGGER_RTT_printf(LOG_TAG,fmt, ##__VA_ARGS__) 
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#define WAIT_SM_NOTIFY_TIMEOUT    (60)

uint64_t key_last_time[KEY_FRAME_TOTAL_SIZE];
key_debug_t key_dbg[4];
TaskHandle_t m_key_thread;
extern TaskHandle_t m_cali_thread;

key_st_t key_st[KEYBOARD_NUMBER];
key_cali_t key_cali;
kh_t kh =  {0};
key_glb_st_t kh_gst;

//kb_t kb_table[KCT_MAX];
//kc_t key_events[BOARD_KEY_NUM];
//uint8_t cpkeys[BOARD_KEY_NUM+4];     /* current pressed keys */
//uint8_t mtCpKeys[0xFF];
//uint8_t tglCpKeys[0xFF];
//uint8_t dksCpKeys[0xFF];
uint8_t keys_processed = 0;  /* number of current keys */
hid_callback hid_kb_send;
hid_callback hid_ms_send;
hid_callback hid_sys_send;

static rep_hid_t res_kb_report;
static rep_hid_t kb_report;
static rep_hid_t ms_report;
static rep_hid_t *sk_report = NULL;
static sm_data_t unchange_sm_data;
static uint8_t sm_action = false;
uint64_t key_wait_50ms = 0;

uint32_t cpu_time;
uint64_t delta_time;
volatile bool is_timer_key_unchange_flag = false;
//#define WORD_TEST
//#define GEN_FRAME_TABLE
uint16_t test_buf[KEY_FRAME_TOTAL_SIZE] = {0};//[KEY_SCAN_ADC_BUFF_COUNT*KEY_SCAN_ADC_ONCE_COUNT*KEY_SCAN_SWITCH_CH_COUNT];

extern uint8_t all_key_trg_num;
//#define TEST_DIFF

#ifdef TEST_DIFF
uint32_t test_diff[KEY_FRAME_TOTAL_SIZE];
uint32_t test_max[KEY_FRAME_TOTAL_SIZE];
uint32_t test_min[KEY_FRAME_TOTAL_SIZE];

#endif

#ifdef GEN_FRAME_TABLE
int8_t key_frame[KEY_SCAN_ADC_ONCE_TIMES*KEY_SCAN_ADC_ONCE_COUNT*KEY_SCAN_SWITCH_CH_COUNT];
static uint8_t ttest = 0;
#endif

uint64_t g_key_time_stamp = 0;
uint16_t g_tuning_cnt;


rep_hid_t *matrix_get_kb_report(void)
{
    return &kb_report;
}
rep_hid_t *matrix_get_ms_report(void)
{
    return &ms_report;
}

key_st_t *matrix_get_key_st(uint8_t index)
{
    if (index < BOARD_KEY_NUM)
    {
        return &key_st[index];
    }
    
    return NULL;
}

void start_time(void)
{
    delta_time = hpm_csr_get_core_cycle();
}

uint32_t get_end_time(void)
{
    delta_time = hpm_csr_get_core_cycle() - delta_time;
    return delta_time;
}

#if 0
ATTR_RAMFUNC
static uint16_t adc_weight_avg2(uint8_t idx, uint32_t buffer[][145])
{
    uint8_t weight[4] = {48, 80, 80, 48}; /* sum 256 */
    uint32_t sum = 0;
    uint32_t array[4];

    //for (uint32_t i = 0; i < KEY_SCAN_ADC_ONCE_TIMES; i++)
    //{
    array[0] = buffer[0][idx*2 + 0] & 0xffff;
    array[1] = buffer[0][idx*2 + 1] & 0xffff;
    array[2] = buffer[1][idx*2 + 0] & 0xffff;
    array[3] = buffer[1][idx*2 + 1] & 0xffff;
    
    //}

    for (uint16_t i = 0; i < 4; i++)
    {
        for (uint16_t j = 0; j < 4 - i - 1; j++)
        {
            if (array[j] > array[j + 1])
            {
                uint32_t tmp = array[j];
                
                array[j] = array[j + 1];
                array[j + 1] = tmp;
            }
        }
    }

    for (uint32_t i = 0; i < 4; i++)
    {
        sum += array[i] * weight[i];
    }

    return sum >> 8;
}

//限幅平均滤波
#define A 40        //限制幅度阈值
#define LA_LEN 3

typedef struct  la_s
{
    uint8_t  out_pos;      /*  */
    uint16_t buf[LA_LEN];   /*  */
    uint16_t data0;
    uint16_t data1;
    uint32_t sum;        /*  */
} la_t;

la_t lavg_filter[72];

uint16_t LAverageFilter(la_t *p_st, uint32_t *in_data)
{
    int16_t diff;
    
    diff = in_data[0] - p_st->data0;
    
    if (diff > A)
    {
        p_st->data0 += A;
    }
    else if (diff < (-A))
    {
        p_st->data0 -= A;
    }
    else
    {
        p_st->data0 = in_data[0];
    }
    
    diff = in_data[1] - p_st->data1;
    if (diff > A)
    {
        p_st->data1 += A;
    }
    else if (diff < (-A))
    {
        p_st->data1 -= A;
    }
    else
    {
        p_st->data1 = in_data[1];
    }
    
    p_st->sum +=  p_st->data0;
    
    p_st->sum -= p_st->buf[p_st->out_pos];
    p_st->buf[p_st->out_pos] = p_st->data0;
    
    p_st->out_pos++;
    p_st->out_pos%=LA_LEN;

    

    p_st->sum +=  p_st->data1;
    
    p_st->sum -= p_st->buf[p_st->out_pos];
    p_st->buf[p_st->out_pos] = p_st->data1;
    
    p_st->out_pos++;
    p_st->out_pos%=LA_LEN;
    
    
    return p_st->sum/LA_LEN;

}

void LAverageFilter_init(la_t *p_st, uint32_t *pbuf, int size)
{
    int i;
    
    memset(p_st, 0, size * sizeof(la_t));
    
    for (i = 0; i < size; i++)
    {
        for (int len = 0; len < LA_LEN; len++)
        {
            p_st->buf[len] = pbuf[i*2];
        }
        
        p_st->data0     = pbuf[i*2];
        p_st->data1     = pbuf[i*2+1];
        p_st->out_pos   = 0;
        p_st->sum       = pbuf[i*2]*LA_LEN;
        p_st++;
    }
}

int KalmanFilter(int inData)
{
    static float prevData = 0;                           //先前数值
    static float p = 10, q = 0.001, r = 0.001, kGain = 0;//q控制误差 r控制响应速度
    
    p = p + q;
    kGain=p/(p+r);                                       //计算卡尔曼增益
    inData = prevData + (kGain * ( inData - prevData ));   //计算本次滤波估计值
    p = ( 1 - kGain ) * p;                               //更新测量方差
    prevData = inData;
    
    return inData;//返回滤波值
}
#endif
void matrix_reset(void)
{
    //board_kb_send_release();
    //memset(cpkeys, 0xff, sizeof(cpkeys));
    //memset(mtCpKeys, 0xff, ARRAY_SIZE(mtCpKeys));
    //memset(tglCpKeys, 0xff, ARRAY_SIZE(tglCpKeys));
    //memset(dksCpKeys, 0xff, ARRAY_SIZE(dksCpKeys));
    //memset(key_st, 0, sizeof(key_st_t));
    //fifo_reset(&adc_fifo);
    
}

static void key_drt_act_1(uint8_t ki0, uint8_t ki1)
{
    key_st[ki0].drt_t1++;
    key_st[ki0].drt_t2    = 0;
    key_st[ki0].trg_logic = KEY_STATUS_D2D;
    if (ki1 != 0xFF) //实体按键
    {
        key_st[ki1].trg_logic = (key_st[ki1].trg_phy == KEY_STATUS_D2D) ? KEY_STATUS_D2D : KEY_STATUS_U2U;
    }
}
static void key_drt_act_2(kc_t *pkc, bool *bupdate, bool *bupdate_ms, uint8_t ki0, uint8_t ki1)
{
    key_st[ki0].trg_logic = KEY_STATUS_U2U;
    if (ki1 != 0xFF)
    {
        key_st[ki1].trg_logic = KEY_STATUS_D2D;
    }
    else
    {
        if (pkc->ty == KCT_KB)
        {
            *bupdate = true;
            report_add_key(&kb_report, pkc->co);
        }
        else if (pkc->ty == KCT_MS)
        {
            *bupdate_ms = true;
            if (pkc->sty <= KC_MS_BTN_SP)
            {
                report_add_ms_btn(&ms_report, pkc->sty);
            }
            else if (pkc->sty <= KC_MS_PY)
            {
                int8_t val = (pkc->sty == KC_MS_PY) ? pkc->co : pkc->co * -1;
                report_set_ms_y(&ms_report, val);
            }
            else if (pkc->sty <= KC_MS_PX)
            {
                int8_t val = (pkc->sty == KC_MS_PX) ? pkc->co : pkc->co * -1;
                report_set_ms_x(&ms_report, val);
            }
            else if (pkc->sty <= KC_MS_WD)
            {
                int8_t val = (pkc->sty == KC_MS_WD) ? -1 : 1;
                report_set_ms_w(&ms_report, val);
            }
            else
            {
                *bupdate_ms = false;
            }
        }
    }
}
static void key_drt_act_3(kc_t *pkc, bool *bupdate, bool *bupdate_ms, uint8_t ki0, uint8_t ki1)
{
    key_st[ki0].drt_t2 += KEY_DRT_RAND_TICK;
    if (ki1 != 0xFF)
    {
        key_st[ki1].trg_logic = (key_st[ki1].trg_phy == KEY_STATUS_D2D) ? KEY_STATUS_D2D : KEY_STATUS_U2U;
    }
    else
    {
        if (pkc->ty == KCT_KB)
        {
            *bupdate = true;
            report_del_key(&kb_report, pkc->co);
        }
        else if ((pkc->ty == KCT_MS) && (key_st[ki0].drt_t1 != 0))
        {
            *bupdate_ms = true;
            if (pkc->sty <= KC_MS_BTN_SP)
            {
                report_del_ms_btn(&ms_report, pkc->sty);
            }
            else if (pkc->sty <= KC_MS_PY)
            {
                report_set_ms_y(&ms_report, 0);
            }
            else if (pkc->sty <= KC_MS_PX)
            {
                report_set_ms_x(&ms_report, 0);
            }
            else if (pkc->sty <= KC_MS_WD)
            {
                report_set_ms_w(&ms_report, 0);
            }
            else
            {
                *bupdate_ms = false;
            }
        }
    }
    key_st[ki0].drt_t1 = 0;
}
// DRT高级键，按下一个键，释放时触发另外一个按键或鼠标功能
static void key_drt_process(uint8_t ki, bool *bupdate, bool *bupdate_ms)
{
    kc_t *pkc      = &key_st[ki].drt_kc;
    key_st_t *pkst = kh.p_st + ki;
    uint8_t ki0    = pkst->drt_ki[0];
    uint8_t ki1    = pkst->drt_ki[1];

    if (key_st[ki0].trg_phy == KEY_STATUS_D2D)
    {
        key_drt_act_1(ki0, ki1);
    }
    else
    {
        uint16_t tick = KEY_DRT_PRESS_TICK;
#if KEY_DRT_TICK_EN
        srand(hpm_csr_get_core_cycle());
        if (key_st[ki0].drt_t1 >= KEY_DRT_OVER_TICK)
        {
            tick = rand() % KEY_DRT_LONG_RAND_TICK + KEY_DRT_LONG_RAISE_TICK;
        }
        else
        {
            tick = rand() % KEY_DRT_RAND_TICK + KEY_DRT_RAISE_TICK;
        }
#endif
        if (key_st[ki0].drt_t2 == KEY_DRT_PRESS_TICK) //另外一个键按下
        {
            key_drt_act_2(pkc, bupdate, bupdate_ms, ki0, ki1);
        }
        else if (key_st[ki0].drt_t2 >= tick)
        {
            key_st[ki0].drt_t2 = tick;
            key_drt_act_3(pkc, bupdate, bupdate_ms, ki0, ki1);
        }
        key_st[ki0].drt_t2++;
    }
}

// SOCD高级键，两个按键互斥按下
static void key_socd_process(uint8_t ki)
{
    key_st_t *pkst   = kh.p_st + ki;
    socd_t snap_team = sk_la_lm_kc_info.sk.socd[pkst->socd_idx];

    uint8_t ki0 = pkst->socd_ki;
    if ((pkst->trg_phy == KEY_STATUS_D2D) && (key_st[ki0].trg_phy == KEY_STATUS_D2D))
    {
        uint8_t rki = 0xFF;
        uint8_t dki = 0xFF;
        if ((pkst->trg_phy_ts > key_st[ki0].trg_phy_ts))
        {
            if (snap_team.mode != SOCD_MODE_NEUTRAL)
            {
                if ((pkst->socd_ki_abs == ki) || (snap_team.mode == SOCD_MODE_LAST_INPUT)) //绝对优先，后输入
                {
                    rki = ki0;
                    dki = ki;
                }
            }
            else //中立模式
            {
                rki = ki0;
            }
        }
        else
        {
            if (snap_team.mode != SOCD_MODE_NEUTRAL)
            {
                if ((pkst->socd_ki_abs != ki) || (snap_team.mode == SOCD_MODE_LAST_INPUT))
                {
                    rki = ki;
                    dki = ki0;
                }
            }
            else
            {
                rki = ki;
            }
        }

        if (dki != 0xFF)
        {
            key_st[dki].trg_logic = KEY_STATUS_D2D;
        }

#if KEY_SOCD_TICK_EN
        if (pkst->socd_tick == 0)
        {
            srand(hpm_csr_get_core_cycle());
            pkst->socd_tick = rand() % KEY_SOCD_RAND_TICK + KEY_SOCD_TICK;
        }
        pkst->socd_tick--;
#endif
        if (pkst->socd_tick == 0)
        {
            if (rki != 0xFF)
            {
                key_st[rki].trg_logic = KEY_STATUS_U2U;
                pkst->socd_tick = KEY_SOCD_RAND_TICK + KEY_SOCD_TICK;
            }
        }

        // 触底始终触发
        uint8_t upward_lvl = 2;
        if (snap_team.sw)
        {
            if ((pkst->socd_hold) && (pkst->cur_lvl >= (KEY_MAX_LEVEL(ki) - upward_lvl)) && (key_st[ki0].cur_lvl >= (KEY_MAX_LEVEL(ki) - upward_lvl)))
            {
                pkst->trg_logic = KEY_STATUS_D2D;
            }
        }
        pkst->socd_hold = 1;
    }
    else
    {
        pkst->socd_hold = 0;
        pkst->socd_tick = 0;
        if ((pkst->tap_type & TAP_TYPE_DRT) == 0)
        {
            pkst->trg_logic = pkst->trg_phy;
        }
    }
}

// RS高级键，两个按键互斥按下
static void key_rs_process(uint8_t ki)
{
    key_st_t *pkst   = kh.p_st + ki;

    uint8_t ki0 = pkst->socd_ki;
    if ((pkst->trg_phy == KEY_STATUS_D2D) && (key_st[ki0].trg_phy == KEY_STATUS_D2D))
    {
        uint8_t rki = 0xFF;
        uint8_t dki = 0xFF;
        if (pkst->cur_lvl > key_st[ki0].cur_lvl)
        {
            rki = ki0;
            dki = ki;
        }
        else if (pkst->cur_lvl < key_st[ki0].cur_lvl)
        {
            rki = ki;
            dki = ki0;
        }

        if (dki != 0xFF)
        {
            key_st[dki].trg_logic = KEY_STATUS_D2D;
        }

        if (pkst->socd_tick == 0)
        {
#if KEY_SOCD_TICK_EN
            srand(hpm_csr_get_core_cycle());
            pkst->socd_tick = rand() % KEY_SOCD_RAND_TICK + KEY_SOCD_TICK;
#else
            pkst->socd_tick = KEY_MS_TO_TICK(1);
#endif
        }
        pkst->socd_tick--;
        if (pkst->socd_tick == 0)
        {
            if (rki != 0xFF)
            {
                key_st[rki].trg_logic = KEY_STATUS_U2U;
                pkst->socd_tick = KEY_SOCD_RAND_TICK + KEY_SOCD_TICK;
            }
        }

        // 触底始终触发
        uint8_t upward_lvl = 1;
        if ((pkst->cur_lvl >= (KEY_MAX_LEVEL(ki) - upward_lvl)) && (key_st[ki0].cur_lvl >= (KEY_MAX_LEVEL(ki) - upward_lvl)))
        {
            pkst->trg_logic = KEY_STATUS_D2D;
        }
    }
    else
    {
        pkst->socd_tick = 0;
        if ((pkst->tap_type & TAP_TYPE_DRT) == 0)
        {
            pkst->trg_logic = pkst->trg_phy;
        }
    }
}

bool tmpCali = false;
uint8_t vki = 17;
bool matrix_init_flg = false;
float run_time_us = 0;
// key_spd_t key_spd;
float delta_ms;


static bool key_handle(sm_data_t *p_sm_data)
{
    int8_t ki;
    uint32_t adc_val;
    uint8_t search_start;
    uint8_t sw_type;
    key_st_t *pkst;
    key_attr_t *pkattr;
    sys_run_info_s *sys_run_info = &sys_run;
    uint32_t msg;

    #ifdef TEST_DIFF
    uint32_t diff;
    static uint16_t diff_cnt = 10000;
    #endif
    
    bool bupdate    = false;
    bool bupdate_ms = false;
    uint8_t special_key_type = 0;
    uint64_t cur_timestap = hpm_csr_get_core_cycle();

    g_key_time_stamp++;

    if (sys_run_info->sys_err_status & (SM_VOLT_LOW_ERROR | SM_WATER_ERROR))
    {
        static bool had_press_flag = false;
        if ((sys_run_info->sys_err_status & SM_WATER_ERROR) || (sys_run_info->volt_err_level > VOLT_WARN_LEVEL_2))
        {
            if((sys_run_info->volt_err_level > VOLT_WARN_LEVEL_2) && (!mg_detecte_get_volt_warn_check_state()))
            {
                mg_detecte_set_volt_warn_check_state(true);
            }
            return false;
        }
        else if(mg_detecte_get_volt_warn_check_state())
        {
            if ((sys_run_info->sys_err_status & SM_VOLT_LOW_ERROR) && ((had_press_flag) || (all_key_trg_num > 0)))
            {
                static uint64_t volt_warn_start_time = 0;

                if(all_key_trg_num > 0)
                {
                    had_press_flag = true; 
                }

                if (volt_warn_start_time == 0 )
                {
                    volt_warn_start_time = cur_timestap;
                }

                if (cur_timestap  > (volt_warn_start_time + (key_wait_50ms * 60)))
                {
                    mg_detecte_set_volt_warn_check_state(false);
                }
            }
        }
    }

    for (uint32_t dev_num = 0; dev_num < KEY_FRAME_NUM; dev_num++)
    {
        uint32_t i = dev_num * KEY_FRAME_SIZE;
        uint32_t key_num = ((dev_num + 1) * KEY_FRAME_SIZE);
        for (; i < key_num; i++)
        {
    #ifdef GEN_FRAME_TABLE
            ki = i;
            kh.p_attr[ki].rt_en = false;
    #else
            if ((p_sm_data->data_type[dev_num] == DATA_CALI_TOP_FRAME_TYPE) || (p_sm_data->data_type[dev_num] == DATA_CALI_BOT_FRAME_TYPE))
            {
                break;
            }
            if ((is_timer_key_unchange_flag == false) && (p_sm_data->dev_data_num[dev_num] == 0))
            {
                break;
            }

            ki = key_idx_table[i];
            if (ki == -1)
            {
                continue;
            }
    #endif
            if ((p_sm_data->pack[i].ty == PKT_TYPE_CHANGE) || (p_sm_data->pack[i].ty == PKT_TYPE_TATOL_CALI))
            {
                if (p_sm_data->dev_data_num[dev_num] > 0)
                {
                    p_sm_data->dev_data_num[dev_num]--;
                }

                if ((p_sm_data->pack[i].adc < ADC_VAL_MIN) || (p_sm_data->pack[i].adc > ADC_VAL_MAX))
                {
                    continue;
                }
                test_buf[ki] = p_sm_data->pack[i].adc;
                p_sm_data->pack[i].adc = 0;
                p_sm_data->pack[i].ty = 0;
                //DBG_PRINTF("ki:%d,value:%d,flag:%d\n",ki,test_buf[ki],is_timer_key_unchange_flag);
            }
            else if ((p_sm_data->pack[i].ty == PKT_TYPE_BOT_CALI)\
                  || (p_sm_data->pack[i].ty == PKT_TYPE_TOP_CALI)\
                  || (p_sm_data->pack[i].ty == PKT_TYPE_ZERO_CALI))
            {
                continue;
            }
            else if (p_sm_data->pack[i].ty == PKT_TYPE_NONE)
            {
                if(is_timer_key_unchange_flag == false)
                {
                    continue;
                }
            }
        
            if(test_buf[ki] == 0)
            {
                continue;
            }

            adc_val = test_buf[ki];
            sw_type = key_cali.sw[ki];
            pkst    = &key_st[ki];
            pkattr  = kh.p_attr + ki;
        
            //找出变化方向,定向搜索
            search_start = pkst->cur_lvl;
            if (adc_val > pkst->cur_point)//向上
            {
                while (search_start)
                {
                    // #warning "kh_update_pos1:< or <=?"
                    if (adc_val < KEY_LVL_GET_VAL(ki, search_start-1))
                    {
                        break;
                    }
                    search_start--;
                }
            }
            else if (adc_val < pkst->cur_point)//向下
            {
                while (search_start < sw_config_table[sw_type].maxLevel)
                {
                    if (adc_val > KEY_LVL_GET_VAL(ki, search_start))
                    {
                        break;
                    }
                    search_start++;
                }
            }

            pkst->cur_lvl = search_start;

            float speed       = 0.0f;
            int16_t delta_val = (adc_val - kh.p_st[ki].last_point);
            if (delta_val != 0)
            {
            #if 1
                speed = (float)delta_val / 0.125f; // /ms
            #else
                // test
                if (ki == 31)
                {
                    key_spd_t *pkspd = &key_spd;
                    delta_ms    = (float)(cur_timestap - key_spd.timestap) / 480000.0;
                    key_spd.timestap = cur_timestap;
                    if (tager_ms >= -0.1)
                    {
                        speed = (float)delta_val / tager_ms; // /ms
                    }
                    else
                    {
                        speed = (float)delta_val / delta_ms; // /ms
                    }
                    pkspd->buf[pkspd->idx] = speed;
                    pkspd->idx++;
                    pkspd->idx %= KEY_SPD_BUF_SIZE;
                    pkspd->cnt++;

                }
            #endif
            }

            // 记录速度连续过大
            const float TH_VAL = 150.0f;
            if ((speed > TH_VAL) || (speed < (-1.0 * TH_VAL)))
            {
                pkst->overspeed_cnt++;
                if (pkst->overspeed_cnt >= 8) //连续8个
                {
                    pkst->overspeed_cnt = 8;
                    pkst->valid_cnt = KEY_MS_TO_TICK(1000); // 大约点击器的3个周期
                }
            }
            else
            {
                pkst->overspeed_cnt = 0;
            }

            if (pkst->valid_cnt)
            {
                pkst->valid_cnt--;
            }
        
    #ifdef WORD_TEST
            kh.p_attr[ki].rt_en = false; //test
    #endif
            pkst->kcmidNow = gbinfo.kc.cur_kcm;
            if (pkst->kcmidNow != pkst->kcmidLast) //switch layer x
            {
                // DEBUG_PRINTF("key[%d] change layer last(%d)->now(%d)! \r\n", ki, kh.p_kh_st[ki].kcmidLast, kh.p_kh_st[ki].kcmidNow);
                if (kh.p_attr[ki].advTrg[pkst->kcmidLast].advType == KEY_ACT_DKS)
                {
                    feature_dks_check_exception_handle(pkst->kcmidLast, ki); // dks key check exception enevt input
                }
                pkst->kcmidLast = pkst->kcmidNow; //update last kcmid
            }
        
            if (kh.p_attr[ki].advTrg[pkst->kcmidNow].advType == KEY_ACT_DKS)
            {
                if(kh.p_attr[ki].advTrg[pkst->kcmidNow].para.dks != NULL)
                {
                    // dks key enevt input
                    pkst->actionState = feature_dks_handle(pkst->kcmidNow, ki, adc_val); //Have changed
                    //keyscan_result_get_key_status(ki); // get key status 
                }
            }
            else if (kh.p_attr[ki].rt_en == false)
            {
    #ifndef GEN_FRAME_TABLE
                if (test_buf[i])
                {
                    if (ki >= BOARD_KEY_NUM)
                    {
                        DBG_PRINTF("[erridx:%02d]=%d,%d\r\n", ki,i,test_buf[i]);
                        continue;
                    }
                }
    #endif
        
    #ifdef GEN_FRAME_TABLE
                pkattr->triggerpoint = 38000;
                pkattr->raisePoint   = 42000;
    #endif

    #ifdef WORD_TEST
            pkattr->triggerpoint = 2400;
            pkattr->raisePoint   = 2500;
    #endif
                #if 1
                switch ((uint8_t)pkst->trg_phy)
                {
                    case KEY_STATUS_U2U:
                        //if (pkst->cur_md == KEY_MD_DOWN)
                        {
                            uint64_t limit_time = (key_last_time[ki] + key_wait_50ms);
                            if (KEY_IS_BIG(ki))
                            {
                                limit_time += key_wait_50ms;
                            }

                            if ((adc_val < pkattr->triggerpoint) && (cur_timestap > limit_time))
                            {
                                pkst->debounce   = 0;
                                pkst->trg_phy    = KEY_STATUS_D2D;
                                pkst->trg_phy_ts = g_key_time_stamp;
                                //DBG_PRINTF("ts=%d\n", pkst->trg_phy_ts);
                                //BOARD_TEST_PIN2_WRITE(1);
                                #ifdef GEN_FRAME_TABLE
                                DBG_PRINTF("[ki=%02d,idx=%02d,%d,%d\r\n", i,ttest,adc_val,pkattr->triggerpoint);
                                key_frame[i] = ttest;
                                ttest++;
                            
                                if (ttest == BOARD_KEY_NUM)
                                {
                                    ttest = 0;
                                    DBG_PRINTF("keyfram gen:\n{\n");
                                    for (uint8_t j = 0; j < total_size; j++)
                                    {
                                        DBG_PRINTF("%2d, ", key_frame[j]);
                                        if (j == 35)
                                            DBG_PRINTF("\n");
                                        key_frame[j] = -1;
                                    }
                                    DBG_PRINTF("\n}\n");
                                }
                            
                                #endif
                                break;

                            }
                        }
                    break;
                
                    case KEY_STATUS_D2D:
                        if (adc_val > pkattr->raisePoint)
                        {
                            pkst->debounce = 0;
                            pkst->trg_phy  = KEY_STATUS_U2U;
                            key_last_time[ki] = cur_timestap;
                            //BOARD_TEST_PIN2_WRITE(1);
                            //DBG_PRINTF("[ki:%02d(D2U)],%d\r\n", ki,adc_val);
                        }
                    break;
                
                    default :
                        pkst->trg_phy = KEY_STATUS_U2U;
                        DBG_PRINTF("[key:%02d state err=%d\r\n", ki, pkst->trg_phy);
                    break;
                
                }
                #endif
            
            }
            else
            {
                if (kh.p_attr[ki].rt_mode == RT_MODE_ADAPTIVE)
                {
                    rt_ada_handle(ki, adc_val);
                }
                else
                {
                    rt_handle(ki, adc_val);
                }
            }
        
            pkst->last_point = pkst->cur_point;
            pkst->cur_point  = adc_val;

    #if DBG_EN_KEY
            if (kh.p_gst->key_dbg_sw)
            {
                uint8_t len = sizeof(key_dbg) / sizeof(key_dbg[0]);
                for (uint8_t i = 0; i < len; i++)
                {
                    if (key_dbg[i].ki == 0xff)
                    {
                        break;
                    }
                    else if (key_dbg[i].ki == ki)
                    {
                        if (i == 0)
                        {
                            uint16_t adc;
                            uint8_t sy = 0;
                              if (key_rtada_st[key_dbg[0].ki].trend == 0)
                              {
                                adc = 1;
                              }
                              else if (key_rtada_st[key_dbg[0].ki].trend > 0)
                              {
                                adc = key_rtada_st[key_dbg[0].ki].trend;
                              }
                              else
                              {
                                adc = key_rtada_st[key_dbg[0].ki].trend * -1;
                                sy = 2;
                              }
                          key_dbg[i].adc = adc;
                          key_dbg[i].lvl = key_st[key_dbg[0].ki].cur_lvl;
                          key_dbg[i].trg = sy;
                        }
                        else if (i == 1) 
                        {
                          key_dbg[i].adc = key_rtada_st[key_dbg[0].ki].spd + 1;
                          key_dbg[i].lvl = pkst->cur_lvl;
                          key_dbg[i].trg = key_rtada_st[key_dbg[0].ki].is_bounce*2;
                        }
                        else if (i == 2)
                        {
                            key_dbg[i].adc = key_st[key_dbg[0].ki].cur_point;
                            key_dbg[i].lvl = key_st[key_dbg[0].ki].cur_lvl;
                            key_dbg[i].trg = key_st[key_dbg[0].ki].trg_phy;
                        }
                    }
                }
            }
    #endif

        }
    }
    // run_time_us = get_end_time() / 480.0;
    
    #ifdef GEN_FRAME_TABLE
    return false;
    #endif
    

    all_key_trg_num = 0;
    //BOARD_TEST_PIN1_WRITE(1);
    for (uint8_t ki = 0; ki < BOARD_KEY_NUM; ki++)
    {
#if KEY_TUNING_DEBUG
        if (tuning.en)
        {
            cali_tuning_update(ki);
        }
#endif
        pkst    = &key_st[ki];
        if(pkst->actionState)
        {
            all_key_trg_num++;
        }
        //通过物理触发计算逻辑触发
        if (pkst->tap_type == 0x00)
        {
            pkst->trg_logic = pkst->trg_phy;
        }
        else
        {
            uint8_t ki0 = pkst->socd_ki;
            if ((pkst->tap_type & TAP_TYPE_DRT) && \
                (!(((pkst->tap_type & TAP_TYPE_SOCD) || (pkst->tap_type & TAP_TYPE_RS)) && (pkst->trg_phy == KEY_STATUS_D2D) && (key_st[ki0].trg_phy == KEY_STATUS_D2D))))
            {
                key_drt_process(ki, &bupdate, &bupdate_ms);
            }

            if (pkst->tap_type & TAP_TYPE_SOCD)
            {
                key_socd_process(ki);
            }
            else if (pkst->tap_type & TAP_TYPE_RS)
            {
                key_rs_process(ki);
            }
        }
        
        if (pkst->trg_logic == KEY_STATUS_U2U)
        {
            pkst->actionState = false;
            if (pkst->actionState != pkst->actionStateLast)
            {
                // DBG_PRINTF("U:%d, %d\n", ki, pkst->cur_point);
                rgb_led_key_evt_update(ki, KEY_STATUS_U2U);
                pkst->actionStateLast = false;
                
                if (layout_fn_check_exit(ki))
                {
                   kc_t *pkc = &sk_la_lm_kc_info.kc_table[gbinfo.kc.cur_kcm][ki];
                   kc_t *fn_pkc = &sk_la_lm_kc_info.kc_table[FN_LAYOUT_INDEX][ki];

                   if (pkc->ty == KCT_KB)//&& pkc->co <= KC_EXSEL)
                   {
                        report_del_key(&kb_report, pkc->co);
                                                
                        if (layout_fn_get_trg_sta(ki))
                        {
                            layout_fn_clr_trg_sta(ki);
                            report_del_key(&kb_report, fn_pkc->co);
                        }
                        
                        bupdate = true;
                   }
                   else if (pkc->ty == KCT_MS)
                   {
                        bupdate_ms = true;
                        if (pkc->sty <= KC_MS_BTN_SP)
                        {
                            report_del_ms_btn(&ms_report, pkc->sty);
                        }
                        else if (pkc->sty <= KC_MS_PY)
                        {
                            report_set_ms_y(&ms_report, 0);
                        }
                        else if (pkc->sty <= KC_MS_PX)
                        {
                            report_set_ms_x(&ms_report, 0);
                        }
                        else if (pkc->sty <= KC_MS_WD)
                        {
                            report_set_ms_w(&ms_report, 0);
                        }
                        else
                        {
                            bupdate_ms = false;
                        }
                   }
                   else if (pkc->ty == KCT_FN)
                   {
                        //report_clr_key(&kb_report);
                   }
                   else if (pkc->ty == KCT_SYS)
                   {
                        layout_fn_handle_key_event(ki, KEY_STATUS_U2U);
                   }
                   else if (pkc->ty == KCT_SK)
                   {
                        special_key_type = KCT_SK;
                        sk_judge_evt(ki, KEY_STATUS_D2U);
                   }
                   else if (pkc->ty == KCT_RPL)
                   {
                        replace_key_t *rpl_key = key_code_get_replace_key_info(pkc->co);
                        bool ret = report_has_key(&kb_report, rpl_key->precond_co);

                        if (ret == true)
                        {
                            report_del_key(&kb_report, rpl_key->rpl_co);
                            report_del_key(&kb_report, rpl_key->dft_co);
                        }
                        else
                        {
                            if (rpl_key->cur_rpl_sta)
                            {
                                report_del_key(&kb_report, rpl_key->rpl_co);
                                rpl_key->cur_rpl_sta = 0;
                            }
                            report_del_key(&kb_report, rpl_key->dft_co);
                        }
                        
                        if (layout_fn_get_trg_sta(ki))
                        {
                            layout_fn_clr_trg_sta(ki);
                            report_del_key(&kb_report, fn_pkc->co);
                        }
                        
                        bupdate = true;
                   }

                }
                else
                {
                    special_key_type = KCT_FN;
                    layout_fn_handle_key_event(ki, KEY_STATUS_U2U);
                }
                gmx.pkts[ki].kcs  = KCS_NONE;
                gmx.pkts[ki].kpht = 0;
            }
            else
            {
                uint8_t kcm = gbinfo.kc.cur_kcm;
                kc_t *pkc   = &sk_la_lm_kc_info.kc_table[kcm][ki];
                adv_t *padv = &sk_la_lm_kc_info.sk.adv_table[pkc->co];
                
                if ((pkc->ty == KCT_SK) && (pkc->sty == SKT_ADV) && (padv->adv_ex == ADV_MTP))
                {
                    if (adv_mtp_evt(padv, ki) == true)
                    {
                        special_key_type = KCT_SK;
                    }
                }
            }
        }
        else
        {
            pkst->actionState = true;
            if (pkst->actionState != pkst->actionStateLast)
            {
                // DBG_PRINTF("D:%d, %d\n", ki, pkst->cur_point);
                rgb_led_key_evt_update(ki, KEY_STATUS_U2D);
                
                pkst->actionStateLast = true;
                if (layout_fn_check_enter(ki))
                {
                    special_key_type = KCT_FN;
                    layout_fn_handle_key_event(ki, KEY_STATUS_U2D);
                }
                else
                {
                   kc_t *pkc = &sk_la_lm_kc_info.kc_table[gbinfo.kc.cur_kcm][ki];
                   
                   if (pkc->ty == KCT_KB)//&& pkc->co <= KC_EXSEL)
                   {
                        if (1 == pkc->en)
                        {
                             report_add_key(&kb_report, pkc->co);
                        }
                        bupdate = true;
                   }
                   else if (pkc->ty == KCT_MS)
                   {
                        bupdate_ms = true;
                        if (pkc->sty <= KC_MS_BTN_SP)
                        {
                            report_add_ms_btn(&ms_report, pkc->sty);
                        }
                        else if (pkc->sty <= KC_MS_PY)
                        {
                            int8_t val = (pkc->sty == KC_MS_PY) ? pkc->co : pkc->co * -1;
                            report_set_ms_y(&ms_report, val);
                        }
                        else if (pkc->sty <= KC_MS_PX)
                        {
                            int8_t val = (pkc->sty == KC_MS_PX) ? pkc->co : pkc->co * -1;
                            report_set_ms_x(&ms_report, val);
                        }
                        else if (pkc->sty <= KC_MS_WD)
                        {
                            int8_t val = (pkc->sty == KC_MS_WD) ? -1 : 1;
                            report_set_ms_w(&ms_report, val);
                        }
                        else
                        {
                            bupdate_ms = false;
                        }
                   }
                   else if (pkc->ty == KCT_SYS)
                   {
                        layout_fn_handle_key_event(ki, KEY_STATUS_U2D);
                   }
                   else if (pkc->ty == KCT_FN)
                   {
                        layout_fn_handle_key_event(ki, KEY_STATUS_U2D);
                   }
                   else if (pkc->ty == KCT_SK)
                   {
                        special_key_type = KCT_SK;  
                        sk_judge_evt(ki, KEY_STATUS_U2D);
                   }
                   else if (pkc->ty == KCT_RPL)
                   {
                        replace_key_t *rpl_key = key_code_get_replace_key_info(pkc->co);
                        bool is_mod_key = key_code_is_modifier_key(rpl_key->precond_co);
                        bool ret = false;

                        if (is_mod_key)
                        {
                            uint8_t opp_mod_key_co = (rpl_key->precond_co > KC_LGUI) ? rpl_key->precond_co - 4 : rpl_key->precond_co + 4;
                            uint8_t mod_keys = report_get_mod_keys(&kb_report);
                            uint8_t mask1 = key_code_get_modifier_key_mask(rpl_key->precond_co);
                            uint8_t mask2 = key_code_get_modifier_key_mask(opp_mod_key_co);
                            bool has_precond_key = false;

                            if (report_has_key(&kb_report, rpl_key->precond_co) || report_has_key(&kb_report, opp_mod_key_co))
                            {
                                has_precond_key = true;
                            }
                            mod_keys &= ~(mask1 | mask2);

                            if ((true == has_precond_key) && (0 == mod_keys))
                            {
                                ret = true;
                            }
                        }
                        else 
                        {
                           ret = report_has_key(&kb_report, rpl_key->precond_co);
                        }

                        if (ret == true)
                        {
                            report_add_key(&kb_report, rpl_key->rpl_co);
                            rpl_key->cur_rpl_sta = 1;
                        }
                        else
                        {
                            if (rpl_key->cur_rpl_sta)
                            {
                                rpl_key->cur_rpl_sta = 0;
                                report_del_key(&kb_report, rpl_key->rpl_co);
                            }
                            report_add_key(&kb_report, rpl_key->dft_co);
                        }

                        bupdate = true;
                   }
                }
                //DBG_PRINTF("add:%d\n",i);
            }
            else
            {
                uint8_t kcm = gbinfo.kc.cur_kcm;
                kc_t *pkc   = &sk_la_lm_kc_info.kc_table[kcm][ki];
                adv_t *padv = &sk_la_lm_kc_info.sk.adv_table[pkc->co];
                key_tmp_sta_t *pkts = &gmx.pkts[ki];

                if (++pkts->kpht > KPHT_MS_TO_COUNT(KPHT_3000MS))
                {
                    if (KCS_NONE == pkts->kcs)
                    {
                        pkts->kcs = KCS_LP;
                    }
                }

                if (pkc->ty == KCT_SK && pkc->sty == SKT_ADV)
                {
                    if (padv->adv_ex == ADV_MT)
                    {
                        if (pkst->advTrg[kcm].mt.isLoogPress == false && pkts->kpht > KPHT_MS_TO_COUNT(padv->data.mt.holdDuration))
                        {
                            pkst->advTrg[kcm].mt.isLoogPress = true;
                            special_key_type = KCT_SK;
                            sk_judge_evt(ki, KEY_STATUS_D2D);
                        }
                    }
                    else if (padv->adv_ex == ADV_MTP)
                    {
                        if (adv_mtp_evt(padv, ki) == true)
                        {
                            special_key_type = KCT_SK;
                        }
                    }
                }
                else if (pkc->ty == KCT_MS)
                {
                    if (pkc->sty >= KC_MS_NY && pkc->sty <= KC_MS_PY)
                    {
                        bupdate_ms = true;
                        int8_t val = (pkc->sty == KC_MS_PY) ? pkc->co : pkc->co * -1;
                        report_set_ms_y(&ms_report, val);
                    }
                    else if (pkc->sty <= KC_MS_PX)
                    {
                        bupdate_ms = true;
                        int8_t val = (pkc->sty == KC_MS_PX) ? pkc->co : pkc->co * -1;
                        report_set_ms_x(&ms_report, val);
                    }
                }
                else if (pkc->ty == KCT_FN)
                {
                    if (FN_LAYER != pkc->sty)
                    {
                        if (KCS_LP == pkts->kcs)
                        {
                            layout_fn_handle_key_event(ki, KEY_STATUS_U2D);
                        }
                    }
                }
            }
        }
    }

    if (sk_rep_kb_is_change())
    {
        bupdate = true;
    }
    if (sk_rep_ms_is_change())
    {
        bupdate_ms = true;
    }



    if (gbinfo.nkro)
    {
        res_kb_report.nkro.report_id    = REPORT_ID_NKRO;
        if (sk_report != NULL)
        {
            res_kb_report.nkro.modifier_key = kb_report.nkro.modifier_key | sk_report->nkro.modifier_key;
            for (uint8_t i = 0; i < sizeof(kb_report.nkro.bits); i++)
            {
                res_kb_report.nkro.bits[i] = kb_report.nkro.bits[i] | sk_report->nkro.bits[i];
            }
        }
    }
    else
    {
        res_kb_report.report_id    = REPORT_ID_KEYBOARD;
        res_kb_report.modifier_key = kb_report.modifier_key;
        memcpy(res_kb_report.keys, kb_report.keys, sizeof(kb_report.keys) / sizeof(kb_report.keys[0]));
    }

    if (bupdate_ms == true)
    {
        ms_report.mouse.report_id = REPORT_ID_MOUSE;
        msg = (uint32_t)&ms_report;
        xQueueSend(hid_queue, (const void * const )&msg, ( TickType_t ) 0 );
        ps_timer_reset_later();
    }
    if (bupdate == true)
    {
        //DBG_PRINTF("kh time=%d us\r\n", msg/480);
        msg = (uint32_t)&res_kb_report;
        xQueueSend(hid_queue, (const void * const )&msg, ( TickType_t ) 0 );
        ps_timer_reset_later();
    }
    else
    {
        if (bupdate_ms == false)
        {
            dks_dispatch_keyevent();
        }
        if (special_key_type == KCT_FN)
        {
            ps_timer_reset_later();
        }
        else if (special_key_type == KCT_SK)
        {
            sk_task_notify();
            ps_timer_reset_later();
        }
    }

    //get_end_time();
#if DBG_EN_KEY
    if (kh.p_gst->key_dbg_sw)
    {
        uint32_t size = 0;
        uint8_t len = sizeof(key_dbg) / sizeof(key_dbg[0]);
        for (uint8_t i = 0; i < len; i++)
        {
            if (key_dbg[i].ki >= BOARD_KEY_NUM)
            {
                break;
            }
            else
            {
                size += sizeof(key_dbg[0]);
            }
        }
        if (size)
        {
            cmd_hive_send_key_dbg((uint8_t *)key_dbg, size);
        }
    }
#endif
    return true;
}


/// @brief
/// @param pvParameters
static void matrix_thread(void *pvParameters)
{
    (void)pvParameters;
    sm_data_t *p_data;
    sys_run_info_s *sys_run_info = &sys_run;
    uint32_t ticks_per_ms = clock_get_core_clock_ticks_per_ms();
    static uint8_t last_frame_mode = KEY_DATA_ZERO_FRAME;

    key_wait_50ms = ticks_per_ms * 50UL;
    sk_report = sk_get_kb_report();
    hal_key_sync_callback(sm_sync_callback);

#if (USE_KEY_LVL_MODE == KEY_LVL_MODE_REALTIME)
    kcr_table_update();
#endif
    
    DBG_PRINTF("key_thread start now.\n");
    
    //4.启动矩阵处理任务
    while (1)
    {
        p_data = (sm_data_t *)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        hal_timer_sw_stop();
        sm_data_t p_sm_data;
        if (NULL != p_data)
        {
            if((uint32_t*)p_data == (uint32_t*)&unchange_sm_data)
            {
                is_timer_key_unchange_flag = true;
            }
            else if(p_data->all_key_num == 0)
            {
                continue;
            }
            ulTaskNotifyTake(pdTRUE, 0);
            memcpy(&p_sm_data,p_data,sizeof(sm_data_t));
        }
        sm_action = true;
        
        if (last_frame_mode != sys_run_info->sys_frame_mode)
        {
            static uint32_t wait_cnt = 0;
            if ((wait_cnt++) > 10)
            {
                last_frame_mode = sys_run_info->sys_frame_mode;
                wait_cnt = 0;
            }
            continue;
        }

        if (!cali_is_ready())
        {
            cali_manual_updata_handle(&p_sm_data);
        }
        else if (!cali_data_updata_handle(&p_sm_data))
        {
            key_handle(&p_sm_data);
            detecte_data_handle(&p_sm_data);
        }
        is_timer_key_unchange_flag = false;
    }
}

void matrix_para_check(key_attr_t *key_attr)
{
    if (key_attr)
    {
        fix_check_para(key_attr);
        rt_check_para(key_attr);
    }
}

void matrix_para_reset(void)
{
    if (!matrix_init_flg)
    {
        DBG_PRINTF("matrix_para_reset after init\r\n");
        return;
    }
    
    fix_set_default_dead_para();
    fix_set_default_ap_para();
    rt_set_default_para();
}

void matrix_snap_reset(void)
{
    for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        key_st[ki].socd_ki = 0xff;
    }
}

void matrix_socd_reload(sk_la_lm_kc_info_t *pdb)
{
    uint8_t ki_0;
    uint8_t ki_1;
    
    if (!matrix_init_flg)
    {
        DBG_PRINTF("matrix_socd_reload after init\r\n");
        return;
    }
    
    DBG_PRINTF("matrix_socd_reload\n");
    //for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    //{
    //    key_st[ki].cover_ki[0] = 0xff;
    //}
    
    if ((pdb->sk.superkey_table[SKT_SOCD].total_size > SK_SOCD_NUM) || (pdb->sk.superkey_table[SKT_SOCD].free_size > SK_SOCD_NUM)|| (pdb->sk.superkey_table[SKT_SOCD].storage_mask > 0xff))
    {
        memset(pdb->sk.socd, 0xff, sizeof(pdb->sk.socd));
        pdb->sk.superkey_table[SKT_SOCD].total_size   = SK_SOCD_NUM;
        pdb->sk.superkey_table[SKT_SOCD].free_size    = SK_SOCD_NUM;
        pdb->sk.superkey_table[SKT_SOCD].storage_mask = 0;
    }
    
    for (uint8_t i = 0; i < pdb->sk.superkey_table[SKT_SOCD].total_size; i++)
    {
        if (pdb->sk.superkey_table[SKT_SOCD].storage_mask & (1 << i))
        {
            ki_0 = pdb->sk.socd[i].ki[0];
            ki_1 = pdb->sk.socd[i].ki[1];
            uint8_t mode = pdb->sk.socd[i].mode;
            
            if ((ki_0 != ki_1) && (ki_0 < KEYBOARD_NUMBER) && (ki_1 < KEYBOARD_NUMBER))
            {
                key_st[ki_0].socd_idx = i;
                key_st[ki_0].socd_ki  = ki_1;
                key_st[ki_1].socd_ki  = ki_0;
                key_st[ki_0].tap_type |= TAP_TYPE_SOCD;
                key_st[ki_1].tap_type |= TAP_TYPE_SOCD;
                if (mode == SOCD_MODE_ABSOLUTE_1)
                {
                    key_st[ki_0].socd_ki  = ki_0;
                    key_st[ki_1].socd_ki  = ki_0;
                }
                else if (mode == SOCD_MODE_ABSOLUTE_2)
                {
                    key_st[ki_0].socd_ki  = ki_1;
                    key_st[ki_1].socd_ki  = ki_1;
                }
            }
            else
            {
                //DBG_PRINTF("matrix_socd_reload:err %d,%d\n", ki_0, ki_1);
            }
        }
        else
        {
            pdb->sk.socd[i].ki[0] = 0xff;
            pdb->sk.socd[i].ki[1] = 0xff;
        }
    }
}
void matrix_rs_reload(sk_la_lm_kc_info_t *pdb)
{
    uint8_t ki_0;
    uint8_t ki_1;
    
    if (!matrix_init_flg)
    {
        DBG_PRINTF("matrix_rs_reload after init\r\n");
        return;
    }
    
    DBG_PRINTF("matrix_rs_reload\n");
    //for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    //{
    //    key_st[ki].cover_ki[0] = 0xff;
    //}
    
    if ((pdb->sk.superkey_table[SKT_RS].total_size > SK_RS_NUM) || (pdb->sk.superkey_table[SKT_RS].free_size > SK_RS_NUM)|| (pdb->sk.superkey_table[SKT_RS].storage_mask > 0xff))
    {
        memset(pdb->sk.rs, 0xff, sizeof(pdb->sk.rs));
        pdb->sk.superkey_table[SKT_RS].total_size   = SK_RS_NUM;
        pdb->sk.superkey_table[SKT_RS].free_size    = SK_RS_NUM;
        pdb->sk.superkey_table[SKT_RS].storage_mask = 0;
    }
    
    for (uint8_t i = 0; i < pdb->sk.superkey_table[SKT_RS].total_size; i++)
    {
        if (pdb->sk.superkey_table[SKT_RS].storage_mask & (1 << i))
        {
            ki_0 = pdb->sk.rs[i].ki[0];
            ki_1 = pdb->sk.rs[i].ki[1];
            
            if ((ki_0 != ki_1) && (ki_0 < KEYBOARD_NUMBER) && (ki_1 < KEYBOARD_NUMBER))
            {
                key_st[ki_0].socd_ki   = ki_1;
                key_st[ki_1].socd_ki   = ki_0;
                key_st[ki_0].tap_type |= TAP_TYPE_RS;
                key_st[ki_1].tap_type |= TAP_TYPE_RS;
            }
            else
            {
                //DBG_PRINTF("matrix_rs_reload:err %d,%d\n", ki_0, ki_1);
            }
        }
        else
        {
            pdb->sk.rs[i].ki[0] = 0xff;
            pdb->sk.rs[i].ki[1] = 0xff;
        }
    }
}

void matrix_ht_reload(sk_la_lm_kc_info_t *pdb)
{
    // uint8_t ki_0;
    // uint8_t ki_1;
    
    if (!matrix_init_flg)
    {
        DBG_PRINTF("matrix_ht_reload after init\r\n");
        return;
    }
    
    DBG_PRINTF("matrix_ht_reload\n");
    //for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    //{
    //    key_st[ki].cover_ki[0] = 0xff;
    //    key_st[ki].cover_ki[1] = 0xff;
    //}
    
    if ((pdb->sk.superkey_table[SKT_HT].total_size > SK_HT_NUM) || (pdb->sk.superkey_table[SKT_HT].free_size > SK_HT_NUM)|| (pdb->sk.superkey_table[SKT_HT].storage_mask > 0xff))
    {
        memset(pdb->sk.ht, 0xff, sizeof(pdb->sk.ht));
        pdb->sk.superkey_table[SKT_HT].total_size   = SK_HT_NUM;
        pdb->sk.superkey_table[SKT_HT].free_size    = SK_HT_NUM;
        pdb->sk.superkey_table[SKT_HT].storage_mask = 0;
    }
    
    for (uint8_t i = 0; i < pdb->sk.superkey_table[SKT_HT].total_size; i++)
    {
        if (pdb->sk.superkey_table[SKT_HT].storage_mask & (1 << i))
        {
            ht_t *ht = &pdb->sk.ht[i];
            uint8_t ki0 = ht->ki;
            uint8_t ki1 = 0xff;
            uint8_t kibind = ki0;
            if ((ht->kc.ty == KCT_KB) && (kc_to_ki[ht->kc.co] != -1))
            {
                ki1 = kc_to_ki[ht->kc.co];
                kibind = ki1;
            }

            if (ki0 < KEYBOARD_NUMBER)
            {
                key_st[kibind].drt_kc    = ht->kc;
                key_st[kibind].drt_ki[0] = ki0;
                key_st[kibind].drt_ki[1] = ki1;
                key_st[kibind].tap_type |= TAP_TYPE_DRT;

            }
            else
            {
                memset(&key_st[kibind].drt_kc, 0, sizeof(kc_t));
                key_st[kibind].drt_ki[0] = 0xFF;
                key_st[kibind].drt_ki[1] = 0xFF;
                key_st[kibind].tap_type &= ~TAP_TYPE_DRT;
            }
        }
        else
        {
            pdb->sk.ht[i].ki = 0xff;
            memset(&pdb->sk.ht[i].kc, 0, sizeof(kc_t));
        }
    }
}

void matrix_get_baord_ver(void)
{
}
void matrix_para_clear(void)
{
    //memset(cpkeys, 0xff, sizeof(cpkeys));
    //memset(mtCpKeys, 0xff, ARRAY_SIZE(mtCpKeys));
    //memset(tglCpKeys, 0xff, ARRAY_SIZE(tglCpKeys));
    //memset(dksCpKeys, 0xff, ARRAY_SIZE(dksCpKeys));
    //memset(&kh_gst, 0, sizeof(kh_gst));

    for (uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        memset(&key_st[ki], 0, sizeof(key_st_t));
        cali_calc_level_boundary(ki);
        key_rt_clear_state(ki);
        key_st[ki].socd_ki   = 0xff;
        key_st[ki].drt_ki[0] = 0xff;
        key_st[ki].drt_ki[1] = 0xff;
        key_st[ki].tap_type  = 0;
        key_st[ki].drt_t2    = KEY_DRT_OVER_TICK;
    }
}

void matrix_para_reload(void)
{
    //bool isAttributeDefault = false;
    
    if (!matrix_init_flg)
    {
        DBG_PRINTF("matrix_para_reload after init\r\n");
        return;
    }
    DBG_PRINTF("matrix_para_reload\r\n");
    matrix_para_clear();
    matrix_get_baord_ver();
    fix_check_para(kh.p_attr);
    rt_check_para(kh.p_attr);
    
    fix_init(&kh);
    rt_init(&kh);
    key_dks_init(&kh);
    matrix_socd_reload(&sk_la_lm_kc_info);
    matrix_ht_reload(&sk_la_lm_kc_info);
    matrix_rs_reload(&sk_la_lm_kc_info);
    adv_mtp_reload();
}

void  matrix_para_register(void)
{
    kh.p_st             = key_st;
    kh.p_attr           = gbinfo.key_attr;
    kh.p_gst            = &kh_gst;
    kh.p_qty            = &gbinfo.kh_qty;
    kh.p_calib          = &key_cali;
    matrix_init_flg     = true;
    matrix_get_baord_ver();
}

matrix_ret matrix_para_init(uint8_t size)
{
    DBG_PRINTF("matrix_para_init\n");
    
    if ((kh.p_st == NULL) || (kh.p_attr == NULL) || (kh.p_gst == NULL) || (kh.p_qty == NULL) || (kh.p_calib == NULL))
    {
        return MTX_RET_STATE_ERR;
    }
    
    kh.p_qty->tuning_en = TUNING_ON;
    
    
    memset(kh.p_st,0, sizeof(key_st_t)*size);

    matrix_para_reload();
    cali_tuning_set_state(TUNING_ON);

    return MTX_RET_SUCEESS;
}

void matrix_set_nkro(uint8_t nkro)
{
    uint32_t msg;
    if (true == report_has_anykey(&kb_report))
    {
        report_clr_key(&kb_report);
        msg = (uint32_t)&kb_report;
        xQueueSend(hid_queue, (const void * const )&msg, ( TickType_t ) 0 );
        vTaskDelay(10);//等待发送完成
        
        DBG_PRINTF("matrix_set_nkro: release all key\r\n");
    }
    
    DBG_PRINTF("matrix_set_nkro:%d\r\n", nkro);
    if (nkro == BOARD_NKRO_DISABLE)
    {
        kb_report.report_id = REPORT_ID_KEYBOARD;
    }
    else
    {
        kb_report.report_id = REPORT_ID_NKRO;
    }
}

static void key_unchange_handle(void)
{
    uint32_t value = (uint32_t)(&unchange_sm_data);
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    if (pdFAIL != xTaskNotifyFromISR(m_key_thread, value, eSetValueWithoutOverwrite, &higherPriorityTaskWoken))
    {
        memset(&unchange_sm_data, 0, sizeof(unchange_sm_data));
        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }
}

static void sm_recv_timeout_callback(void)
{
    //从控数据接收超时
    hal_timer_sw_stop();
    key_unchange_handle();
}

void sm_sync_callback(void)
{
    sys_run_info_s *sys_run_info = &sys_run;
    if (cali_is_ready() && sm_action)
    {
        hal_timer_sw_start();
    }

    if((sys_run_info->sys_frame_mode == KEY_DATA_ZERO_FRAME) || (sys_run_info->sys_frame_mode == KEY_DATA_TOTAL_FRAME))
    {
        static bool invert_flag = true;
        invert_flag = invert_flag == false ? true : false;
        if(invert_flag)
        {
            hal_sync_start(); //从控同步
        }
    }
    else
    {
        hal_sync_start(); //从控同步
    }
}

void sm_unsync_callback(void)
{
    hal_timer_sw_stop();
    hal_key_sync_callback(NULL);
}

void matrix_init(void)
{
    cali_init(&kh);
    
    matrix_para_init(KEYBOARD_NUMBER);
    hal_timer_sw_init(WAIT_SM_NOTIFY_TIMEOUT, sm_recv_timeout_callback);

    memset(&kb_report, 0, sizeof(rep_hid_t));
    matrix_set_nkro(gbinfo.nkro);
    
    if (xTaskCreate(matrix_thread, "matrix_thread", STACK_SIZE_KEY, NULL, PRIORITY_KEY, &m_key_thread) != pdPASS)
    {
        DBG_PRINTF("matrix_thread creation failed!\n");
        for (;;)
        {
            ;
        }
    }

    ps_timer_init(gbinfo.power_save.en, gbinfo.power_save.time);
}

extern void hall_driver_auto_start_enable(bool enable);
void matrix_uninit(void)
{
    // hall_driver_auto_start_enable(false);
    
    hal_key_sync_callback(NULL);
    hal_notify_enable(false);
    hal_timer_sw_stop();
    
    if (m_key_thread)
    {
        vTaskSuspend(m_key_thread);
    }
}
