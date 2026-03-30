#include "FreeRTOS.h"
#include "task.h"
#include "mg_matrix_type.h"
#include "mg_trg_rt.h"
#include "mg_trg_rt_adaptive.h"
#include "mg_trg_fix.h"
#include "app_debug.h"
#include "board.h"
#include "hpm_gpio_drv.h"
#include "mg_cali.h"
#include "hal_uart.h"

#if DEBUG_EN == 1

    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define   DBG_PRINTF     printf
#else
    #define   DBG_PRINTF(format, ...)
#endif

#else
    #define   DBG_PRINTF(format, ...)
#endif


//1:灵敏, 2:均衡, 3:稳定; 基于极限模式下增大范围
#define MX_TOP_TICK_BS          (2)
#define MX_BOT_TICK_BS          (2)
#define MX_TOP_BOUNCE_BS        (0.05f)
#define MX_BOT_BOUNCE_BS        (0.002f)
#define MX_BOT_DEFORM_BS        (0.002f)


#define TIME_OVER                           (2147483647) /*2^32-1 >> 2*/
//顶部回弹连击
#define TOP_BOUNCE_TICK(mx)                 (KEY_MS_TO_TICK(40 + (mx * MX_TOP_TICK_BS)))
#define TOP_BOUNCE_BIG_TICK(mx)             (KEY_MS_TO_TICK(70 + (mx * MX_TOP_TICK_BS)))
#define TOP_BOUNCE_SPD_CHECK(spd)           ((spd) > 20)
#define TOP_BOUNCE_BIG_SPD_CHECK(spd)       ((spd) > 40)
//底部回弹连击
#define BOT_BOUNCE_LIMIT_VAL(ki, mx)        (key_st[ki].min + (key_st[ki].diff * (0.050 + (mx * MX_BOT_BOUNCE_BS))))
#define BOT_BOUNCE_TICK(mx)                 (KEY_MS_TO_TICK(40  + (mx * MX_BOT_TICK_BS)))
#define BOT_BOUNCE_BIG_TICK(mx)             (KEY_MS_TO_TICK(70 + (mx * MX_BOT_TICK_BS)))
#define BOT_BOUNCE_SPD_CHECK(spd)           ((spd) > 100)
#define BOT_BOUNCE_BIG_SPD_CHECK(spd)       ((spd) > 100)
#define BOT_DEFORM_LIMIT_VAL(ki, mx)        (key_st[ki].min + (key_st[ki].diff * (0.015 + (mx * MX_BOT_DEFORM_BS))))
//底部异常 todo
#define BOT_RANGE_LIMIT_VAL(ki)             (key_st[ki].min + (key_st[ki].diff * 0.035))
#define BOT_RELEASE_CNT                     (3)
#define BOT_RELEASE_SPD_DIF                 (100.0)
#define BOT_RELEASE_CNT_MAX                 (8)
//动态死区
#define MIN_SPEED                           (10)
#define MAX_SPEED                           (600)
#define DZ_FACTOR                           (0.3)
//正常模式
#define KEY_SPEED_THRESHOLD_TOP             (30)
#define KEY_SPEED_THRESHOLD_BOT             (20)
#define KEY_UP_BOUNCE_TICK                  (KEY_MS_TO_TICK(25))
#define KEY_DOWN_BOUNCE_TICK                (KEY_MS_TO_TICK(10))




enum 
{ 
    BOT_REL_OK = 0,
    BOT_REL_ERR, 
    BOT_REL_WAIT,
};

extern kh_t kh;
extern uint32_t g_key_time_stamp;
extern key_st_t key_st[KEYBOARD_NUMBER];

key_rt_ada_st_t key_rtada_st[KEYBOARD_NUMBER];



static inline uint16_t dynamic_dead_zone(uint8_t ki, uint16_t spd)
{
    int16_t dir, res;
    float fk, dif, dz_fk;
    float bs_dz = 2.0;
    key_attr_t *pattr = kh.p_attr + ki;

    if (pattr->rt_ada_extreme)
    {
        dz_fk = DZ_FACTOR * 3;
    }
    else 
    {
        uint8_t tmp = (RT_ADA_SENS_MAX - pattr->rt_ada_sens);
        if (pattr->rt_ada_mode == RT_ADA_MODE_QUICK)
        {
            tmp = pattr->rt_ada_sens + 1;
        }

        dz_fk = DZ_FACTOR * tmp;
        bs_dz += (bs_dz * (float)(tmp / RT_ADA_SENS_MAX)); //根据偏好设置调整死区
    }
    
    dir = (pattr->rt_ada_mode == RT_ADA_MODE_QUICK) ? -1 : 1;
    spd = (spd > MAX_SPEED) ? MAX_SPEED : (spd < MIN_SPEED) ? MIN_SPEED : spd;
    fk  = (float)(spd - MIN_SPEED) / (float)(MAX_SPEED - MIN_SPEED); // scale to 0-1

    dif = bs_dz * dz_fk * dir;
    res = bs_dz + (dif * fk);

    if (res < 10)
    {
        res = 10;
    }

    return res;
}

static inline int32_t update_spd_result(uint8_t ki)
{
    key_rt_ada_st_t *pspd = key_rtada_st + ki;
    int32_t td = 0;
    int32_t trend = 0;
    uint8_t count = 0;
    uint8_t prev_idx = 0;
    uint8_t idx = pspd->buf_s;
    
    while (idx != pspd->buf_e)
    {
        prev_idx = (idx + SPEED_HISTORY_SIZE - 1) % SPEED_HISTORY_SIZE;
        if (prev_idx != pspd->buf_e)
        {
            // if (pspd->hist_buf[idx] == 0)
            // {
            //     idx = (idx + 1) % SPEED_HISTORY_SIZE;
            //     continue;
            // }
            trend += (pspd->spd_buf[idx] - pspd->spd_buf[prev_idx]);
            count++;
        }
        idx = (idx + 1) % SPEED_HISTORY_SIZE;
    }
    
    if (count > 0)
    {
        td = trend / count;
    }
    return td;
}

static inline void update_spd_hist(uint8_t ki, uint16_t spd)
{
    key_rt_ada_st_t *pspd = key_rtada_st + ki;

    pspd->spd_buf[pspd->buf_e] = spd;
    pspd->buf_e = (pspd->buf_e + 1) % SPEED_HISTORY_SIZE;
    if (pspd->buf_e == pspd->buf_s)
    {
        pspd->buf_s = (pspd->buf_s + 1) % SPEED_HISTORY_SIZE;
    }
}

static inline uint16_t spd_calc(uint8_t ki, uint16_t value, uint32_t delta_time)
{
    uint32_t spd = 0;
    uint16_t delta_val = ABS2(value, kh.p_st[ki].last_val);

    if (delta_time > 0)
    {
        spd = (float)delta_val / (float)delta_time * 1000000.0;
    }
    else
    {
        spd = 0;
    }
    
    if (spd > 65535)
    {
        spd = 65535;
    }
    key_rtada_st[ki].spd = spd;
    update_spd_hist(ki, spd);

    return spd;
}

static inline void check_up_bounce(uint8_t ki, uint16_t t)
{
    key_rt_ada_st_t *pspd = key_rtada_st + ki;
    if (pspd->bounce_start == true)
    {
        if ((pspd->is_bounce == false) && (TOP_BOUNCE_BIG_SPD_CHECK(pspd->trend)))
        {
            pspd->bounce_t     = t;
            pspd->is_bounce    = true;
            pspd->bounce_start = false;
        }
    }
}
static inline void check_down_bounce(uint8_t ki, uint16_t t)
{
    key_rt_ada_st_t *pspd = key_rtada_st + ki;
    // if (pspd->bounce_start == false)
    {
        if ((pspd->is_bounce == false) && (BOT_BOUNCE_BIG_SPD_CHECK(pspd->trend)))
        {
            pspd->bounce_t     = t;
            pspd->is_bounce    = true;
            pspd->bounce_start = true;
        }
    }
}

static inline void bottom_anomaly_state_clear(uint8_t ki)
{
    key_rtada_st[ki].cycle = BOT_RELEASE_CNT_MAX;
}
static inline uint8_t bottom_anomaly_check(uint8_t ki, uint16_t val, uint32_t spd)
{
    key_rt_ada_st_t *p_rtada_st = key_rtada_st + ki;
    
    if (val > BOT_RANGE_LIMIT_VAL(ki))
    {
        p_rtada_st->cycle = BOT_RELEASE_CNT_MAX;
        return BOT_REL_OK;
    }

    if (p_rtada_st->cycle == BOT_RELEASE_CNT_MAX) //first
    {
        p_rtada_st->trg_val_cnt++;
        p_rtada_st->trg_spd_cnt++;
        p_rtada_st->trg_spd = spd;
        p_rtada_st->trg_val = val;
    }
    
    p_rtada_st->cycle--;
    int32_t dif_spd = spd - p_rtada_st->trg_spd;
    int16_t dif_val = val - p_rtada_st->trg_val;
    // p_rtada_st->test_dif_spd = dif_spd;
    // p_rtada_st->test_dif_val = dif_val;
/*
    if (dif_spd > BOT_RELEASE_SPD_DIF) //提前退出
    {
        return BOT_REL_OK;
    }
    else if(dif_spd > 0.0)
    {
        p_rtada_st->trg_spd_cnt++;
        if (p_rtada_st->trg_spd_cnt >= BOT_RELEASE_CNT) //提前退出
        {
            return BOT_REL_OK;
        }
    }

*/
    if (dif_val > 500)
    {
        return BOT_REL_OK;
    }
    else if(dif_spd > 400)
    {
        p_rtada_st->trg_val_cnt++;
        if (p_rtada_st->trg_val_cnt >= BOT_RELEASE_CNT) //提前退出
        {
            return BOT_REL_OK;
        }
    }

    if (p_rtada_st->cycle == 0)
    {
        uint8_t spd_exit = BOT_REL_ERR;//(p_rtada_st->trg_spd_cnt >= BOT_RELEASE_CNT) ? BOT_REL_OK : BOT_REL_ERR;
        uint8_t val_exit = (p_rtada_st->trg_val_cnt >= BOT_RELEASE_CNT) ? BOT_REL_OK : BOT_REL_ERR;

        
        return ((spd_exit == BOT_REL_OK) || (val_exit == BOT_REL_OK)) ? BOT_REL_OK : BOT_REL_ERR;
    }
    else
    {
        return BOT_REL_WAIT;
    }
}
static inline bool bottom_bounce_check(uint8_t ki, uint16_t value)
{
    bool ret = false;
    key_attr_t *pattr = kh.p_attr + ki;
    key_rt_ada_st_t *p_rtada_st = key_rtada_st + ki;
    uint8_t mode = 0;
    if (pattr->rt_ada_extreme == false)
    {
        mode = RT_ADA_SENS_MAX - pattr->rt_ada_sens;
    }
    
    if (value < BOT_BOUNCE_LIMIT_VAL(ki, mode) || KEY_IS_BIG(ki))
    {
        //更新向下的速度
        uint16_t th_abs   = abs(p_rtada_st->trend);
        if (th_abs > p_rtada_st->bot_max_td)
        {
            p_rtada_st->bot_bounce_tick = 0;
            p_rtada_st->bot_max_td = th_abs;

            if (KEY_IS_BIG(ki))
            {
                //if (BOT_BOUNCE_BIG_SPD_CHECK(p_rtada_st->bot_max_td))
                {
                    p_rtada_st->bot_bounce_tick = BOT_BOUNCE_BIG_TICK(mode);
                    ret = true;
                }
            }
            else
            {
                if (BOT_BOUNCE_SPD_CHECK(p_rtada_st->bot_max_td))
                {
                    p_rtada_st->bot_bounce_tick = BOT_BOUNCE_TICK(mode);
                    ret = true;
                }
            }
        }
    }
    return ret;
}
static inline bool bottom_trigger_check(uint8_t ki)
{
    key_rt_ada_st_t *p_rtada_st = key_rtada_st + ki;

    if (p_rtada_st->bot_bounce_tick)
    {
        p_rtada_st->bot_bounce_tick--;
        return false;
    }
    else
    {
        return true;
    }
}

static inline bool release_check(uint8_t ki, uint16_t value)
{
    bool ret = false;
    key_st_t *pkst = kh.p_st + ki;
    key_attr_t *pattr = kh.p_attr + ki;
    //key_rt_ada_st_t *p_rtada_st = key_rtada_st + ki;

    uint8_t mode = 0;
    if (pattr->rt_ada_extreme == false)
    {
        mode = RT_ADA_SENS_MAX - pattr->rt_ada_sens;
    }
    if ((pkst->isShapeChange == true) || (value < BOT_DEFORM_LIMIT_VAL(ki, mode))) //底部变形
    {
        pkst->isShapeChange     = 0x00;
        pkst->rtUpperLimitPoint = 0x0000;
        pkst->rtLowerLimitPoint = 0xffff;
        key_rt_down_point(ki, value);
        ret = true;
    }
    else
    {
        /*
        ret = true;
        uint8_t bot_st = bottom_anomaly_check(ki, value, p_rtada_st->spd); //检查底部异常状态
        if (bot_st == BOT_REL_ERR)
        {
            bottom_anomaly_state_clear(ki);
            key_rt_down_point(ki, value);
            ret = true;
        }
        else if (bot_st == BOT_REL_OK)
        */
        {
            //p_rtada_st->bounce_start = true;
            bottom_anomaly_state_clear(ki);
            key_rt_up_point(value);
            ret = false;
        }
    }

    return ret;
}


static inline bool top_trigger_check(uint8_t ki)
{
    key_rt_ada_st_t *p_rtada_st = key_rtada_st + ki;

    if (p_rtada_st->top_bounce_tick)
    {
        p_rtada_st->top_bounce_tick--;
        return false;
    }
    else
    {
        return true;
    }
}
static inline void top_bounce_check(uint8_t ki)
{
    key_attr_t *pattr = kh.p_attr + ki;
    key_rt_ada_st_t *p_rtada_st = key_rtada_st + ki;

    uint8_t mode = 0;
    if (pattr->rt_ada_extreme == false)
    {
        mode = RT_ADA_SENS_MAX - pattr->rt_ada_sens;
    }
    if (p_rtada_st->is_top_exit == false)
    {
        p_rtada_st->is_top_exit = true;
        p_rtada_st->trend = update_spd_result(ki);
        uint16_t th_abs   = abs(p_rtada_st->trend);
        if (th_abs > p_rtada_st->top_max_td)
        {
            p_rtada_st->top_max_td = th_abs;
        }
        
        p_rtada_st->top_bounce_tick = 0;
        if (KEY_IS_BIG(ki))
        {
            //if (TOP_BOUNCE_BIG_SPD_CHECK(p_rtada_st->top_max_td))
            {
                p_rtada_st->top_bounce_tick = TOP_BOUNCE_BIG_TICK(mode);
            }
        }
        else
        {
            if (TOP_BOUNCE_SPD_CHECK(p_rtada_st->top_max_td))
            {
                p_rtada_st->top_bounce_tick = TOP_BOUNCE_TICK(mode);
            }
        }
    }
}

//test
uint32_t max_spd = 0;

bool rt_ada_handle(uint8_t ki, uint16_t value)
{
    key_st_t *pkst    = kh.p_st + ki;
    key_attr_t *pattr = kh.p_attr + ki;
    key_rt_ada_st_t *p_rtada_st = key_rtada_st + ki;

    bool ret = false;
    uint8_t filter;
    uint16_t dz_val;
    //uint16_t up_t       = KEY_UP_BOUNCE_TICK;
    //uint16_t down_t     = KEY_DOWN_BOUNCE_TICK;
    uint64_t cur_time   = hpm_csr_get_core_cycle();
    uint64_t delta_time = cur_time - pkst->last_time;

    if (delta_time > TIME_OVER) //timestamp overflow
    {
        pkst->last_val  = value;
        pkst->last_time = cur_time;
        return (pkst->trg_phy == KEY_STATUS_U2U) ? false : true;
    }

    spd_calc(ki, value, delta_time);
    if (max_spd < p_rtada_st->spd)
    {
        max_spd = p_rtada_st->spd;
    }
    
/*
    if (KEY_IS_BIG(ki))
    {
        up_t   <<= 1;
        down_t <<= 1;
    }

*/

    if (value >= pkst->rtAbortPoint)
    {
        if (pkst->rtIsEnter)
        {
            pkst->rtIsEnter = false;
        }
        top_bounce_check(ki); //更新退出时的速度，检查滤除顶部回弹
        key_rt_up_point(value);
        ret = false;
    }
    else if (((value < pattr->triggerpoint) && top_trigger_check(ki)) || pkst->rtIsEnter)
    {
        p_rtada_st->trend = update_spd_result(ki);
        dz_val = dynamic_dead_zone(ki, ABS(p_rtada_st->trend));
        // tmp_dz_val = dz_val;

        if ((pkst->trg_phy == KEY_STATUS_U2U) || (pkst->rtIsEnter == false)) //release
        {
            pkst->rtIsEnter = true;
            // if (pattr->rt_ada_extreme) 
            // {
                //更新向上的速度
                uint16_t th_abs = abs(p_rtada_st->trend);
                if (th_abs > p_rtada_st->top_max_td)
                {
                    p_rtada_st->top_max_td = th_abs;
                }
                filter = true;
            // }
            // else
            // {
            //     check_up_bounce(ki, up_t);
            //     filter = (p_rtada_st->trend > KEY_SPEED_THRESHOLD_TOP) && (p_rtada_st->is_bounce == false); // && (pkst->debounce > KEY_RT_DEBOUNCE_TICK)
            // }
            
            uint16_t pt = (pkst->rtUpperLimitPoint - dz_val);
            
            if (((value < pt) || (value <= pkst->min)) && filter)
            {
                // bottom_anomaly_state_clear(ki);
                key_rt_down_point(ki, value);
                ret = true;
            }
            else
            {
                key_rt_up_point(value);
                ret = false;
            }
        }
        else // trigger 
        {
            // if (pattr->rt_ada_extreme) 
            // {
                bottom_bounce_check(ki, value); //检查滤底部回弹
                // filter = bottom_trigger_check(ki);
            // }
            // else
            // {
            //     check_down_bounce(ki, down_t);
            //     filter = (p_rtada_st->trend > KEY_SPEED_THRESHOLD_BOT) && (p_rtada_st->is_bounce == false); // && (pkst->debounce > KEY_RT_DEBOUNCE_TICK)
            // }
            
            uint16_t pt = (pkst->rtLowerLimitPoint + dz_val);
            
            if ((value >= pt) && bottom_trigger_check(ki))
            {
                ret = release_check(ki, value);
            }
            else
            {
                key_rt_down_point(ki, value);
                ret = true;
            }
        }
    }
    else
    {
        top_bounce_check(ki); //更新退出时的速度，检查滤除顶部回弹

/*
        //检测回弹：强制消抖
        pkst->debounce = 0;
        if (p_rtada_st->bounce_start == true)
        {
            p_rtada_st->bounce_t   = up_t;
            p_rtada_st->is_bounce  = true;
            p_rtada_st->bounce_start = false;
        }
*/
        key_rt_up_point(value);
        ret = false;
    }

    if (pkst->trg_phy == KEY_STATUS_U2U)
    {
        if (ret == true)
        {
            pkst->debounce   = 0;
            pkst->trg_phy    = KEY_STATUS_D2D;
            pkst->trg_phy_ts = g_key_time_stamp;
            p_rtada_st->top_max_td  = 0;
            p_rtada_st->bot_max_td  = 0;
            p_rtada_st->is_top_exit = false;
            
            // chg_val = value;
            // chg_spd = p_rtada_st->spd;
        }
    }
    else if (pkst->trg_phy == KEY_STATUS_D2D)
    {
        if (ret == false)
        {
            pkst->debounce = 0;
            pkst->trg_phy  = KEY_STATUS_U2U;
            
            // chg_val = value;
            // chg_spd = p_rtada_st->spd;
        }
    }
    else
    {
        pkst->trg_phy = KEY_STATUS_U2U;
    }

    pkst->last_val  = value;
    pkst->last_time = cur_time;
    p_rtada_st->last_spd = p_rtada_st->spd;

    return ret;
}

void rt_ada_clear_state(uint8_t ki)
{
    key_rt_ada_st_t *p_rtada_st = key_rtada_st + ki;
    memset(p_rtada_st, 0, sizeof(key_rt_ada_st_t));
    
    p_rtada_st->is_top_exit = true;
    p_rtada_st->cycle = BOT_RELEASE_CNT_MAX;
}


void rt_ada_init(void)
{
    for(uint8_t ki = 0; ki < KEYBOARD_NUMBER; ki++)
    {
        rt_ada_clear_state(ki);
    }
}



