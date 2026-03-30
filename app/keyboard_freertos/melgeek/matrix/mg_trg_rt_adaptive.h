#ifndef __MG_TRG_RT_ADAPTIVE_H__
#define __MG_TRG_RT_ADAPTIVE_H__

#define SPEED_HISTORY_SIZE              (1 * 8)

typedef enum {
    TREND_STABLE = 0,
    TREND_RISING,
    TREND_FALLING
} trend_state_t;

typedef struct 
{
    int32_t trend;
    uint16_t last_spd;
    uint16_t spd;
    uint16_t spd_buf[SPEED_HISTORY_SIZE];
    uint8_t buf_s;
    uint8_t buf_e;
    

    //顶底部反弹
    bool is_top_exit;
    bool is_bot_bounce;
    int32_t top_max_td;
    int32_t bot_max_td;
    uint16_t top_bounce_tick;
    uint16_t bot_bounce_tick;

    //延迟消抖
    bool is_bounce;
    bool bounce_start;
    uint16_t bounce_t;

    //顶底部异常
    uint8_t cycle;
    uint8_t trg_spd_cnt;
    uint8_t trg_val_cnt;
    uint16_t trg_val;
    uint16_t trg_spd;

   // int32_t test_dif_spd;
   // int16_t test_dif_val;

    
    // uint16_t trend_cnt;
    // trend_state_t trend_state;  // 状态
    // uint8_t trend_max;        // 状态保持时间
    // uint8_t trend_hold;        // 状态保持时间
    // bool key_action;           // 按键动作标志
} key_rt_ada_st_t;
extern key_rt_ada_st_t key_rtada_st[KEYBOARD_NUMBER];


void rt_ada_init(void);
void rt_ada_clear_state(uint8_t ki);
bool rt_ada_handle(uint8_t ki, uint16_t value);

#endif
