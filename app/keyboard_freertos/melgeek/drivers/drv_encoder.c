/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#include "drv_encoder.h"
#include "hpm_qeiv2_drv.h"
#include "hpm_soc.h"
#include "hpm_gpio_drv.h"
#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "hal_gpio.h"
#include "app_debug.h"

#if 0

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "drv_encoder"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif



#define ENCODER_READ_A()                   gpio_read_pin(ENCODER_PIN_CTRL, ENCODER_PIN_PORT, ENCODER_PIN_A)
#define ENCODER_READ_B()                   gpio_read_pin(ENCODER_PIN_CTRL, ENCODER_PIN_PORT, ENCODER_PIN_B)

#define ENCODER_CLEAR_PIN_A_INT_FLAG()     gpio_check_clear_interrupt_flag(ENCODER_PIN_CTRL, ENCODER_PIN_PORT,  ENCODER_PIN_A)
#define ENCODER_CLEAR_PIN_B_INT_FLAG()     gpio_check_clear_interrupt_flag(ENCODER_PIN_CTRL, ENCODER_PIN_PORT,  ENCODER_PIN_B)

#define ENCODER_CHECK_PIN_A_INT_FLAG()     gpio_check_pin_interrupt_flag(ENCODER_PIN_CTRL, ENCODER_PIN_PORT, ENCODER_PIN_A)
#define ENCODER_CHECK_PIN_B_INT_FLAG()     gpio_check_pin_interrupt_flag(ENCODER_PIN_CTRL, ENCODER_PIN_PORT, ENCODER_PIN_B)

#define DEBOUNCE_TIME_MS 2
#define MIN_COUNT_INTERVAL_TICKS  pdMS_TO_TICKS(DEBOUNCE_TIME_MS)  // 最小计数更新间隔
#define ENCODER_BUFFER_SIZE 64  // 扩大缓冲区容量

static encoder_t enc;
static TimerHandle_t encoder_timer;
static SemaphoreHandle_t encoder_mutex;
static volatile uint8_t prev_state = 0;

// 累积每次转换的“步进”值（每个合法状态变化对应 ±1）
static volatile int8_t encoder_accum = 0;

// 环形缓冲区，用于保存中断中采集到的状态
static volatile uint8_t encoder_buffer[ENCODER_BUFFER_SIZE];
static volatile int encoder_buffer_head = 0;
static volatile int encoder_buffer_tail = 0;

// 16元素查找表：计算 index = (prev_state << 2) | new_state，返回转换值（只允许 ±1 或 0）
static const int8_t transition_table[16] = {
    0,  +1,  -1,   0,
   -1,   0,   0,  +1,
   +1,   0,   0,  -1,
    0,  -1,  +1,   0
};

// 新增：上一次计数更新的时间戳，防止极短间隔内重复更新
static TickType_t last_count_time = 0;


static void encoder_poll(void) 
{
    xSemaphoreTake(encoder_mutex, portMAX_DELAY);

    TickType_t now = xTaskGetTickCount();
    // 处理缓冲区中的状态变化
    while (encoder_buffer_tail != encoder_buffer_head)
    {
        uint8_t new_state = encoder_buffer[encoder_buffer_tail];
        encoder_buffer_tail = (encoder_buffer_tail + 1) % ENCODER_BUFFER_SIZE;

        uint8_t index = (prev_state << 2) | new_state;
        int8_t delta = transition_table[index];

        if (delta == 0)
        {
            prev_state = new_state;
            continue;
        }

        encoder_accum += delta;
        prev_state = new_state;
    }

    // 增加实时状态校验
    taskENTER_CRITICAL();
    uint8_t current_state = (ENCODER_READ_A() << 1) | ENCODER_READ_B();
    taskEXIT_CRITICAL();

    if (current_state != prev_state)
    {
        uint8_t index = (prev_state << 2) | current_state;
        int8_t delta = transition_table[index];
        if (delta != 0)
        {
            encoder_accum += delta;
            prev_state = current_state;
        }
    }
    int8_t rotate_flag = -1;
    // 处理累积值
    if (encoder_accum != 0 && (now - last_count_time >= MIN_COUNT_INTERVAL_TICKS))
    {
        int32_t steps = encoder_accum / 2;
        int8_t remainder = encoder_accum % 2;

        if (steps != 0)
        {
            if (steps > 0)
            {
                enc.Tx_RightRotation += steps;
                rotate_flag = 0;
            }
            else
            {
                enc.Tx_LeftRotation += (-steps);
                rotate_flag = 1 ;
            }
            encoder_accum = remainder;  // 保留余数
            last_count_time = now;
        }
    }

    enc.encoder_diff = (int32_t)enc.Tx_RightRotation - (int32_t)enc.Tx_LeftRotation;

    xSemaphoreGive(encoder_mutex);
    if (enc.m_encoder_evt && rotate_flag >= 0)
    {
        enc.m_encoder_evt(rotate_flag == 1 ? ENCODER_EVT_RIGHT_ROTATION : ENCODER_EVT_LEFT_ROTATION);
    }
}

static void encoder_timer_callback(TimerHandle_t xTimer)
{
    (void) xTimer;
    encoder_poll();
}


static void isr_encoder(void)
{
    if (ENCODER_CHECK_PIN_A_INT_FLAG() || ENCODER_CHECK_PIN_B_INT_FLAG())
    {
        ENCODER_CLEAR_PIN_A_INT_FLAG();
        ENCODER_CLEAR_PIN_B_INT_FLAG();

        UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
        uint8_t pin1_val = ENCODER_READ_A();
        uint8_t pin2_val = ENCODER_READ_B();
        taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);

        // 2. 硬件去抖动验证（使用空循环代替阻塞延时）
        //for (volatile int i = 0; i < 1000; i++); // 约1us延时（需根据实际时钟频率调整）

        // 3. 第二次采样验证
        uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
        uint8_t pin1_val_debounced = ENCODER_READ_A();
        uint8_t pin2_val_debounced = ENCODER_READ_B();
        taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);

        // 4. 状态一致性验证
        if (pin1_val != pin1_val_debounced || pin2_val != pin2_val_debounced)
        {
            return;  // 信号不稳定，丢弃此次采样
        }

        // 5. 状态入队
        uint8_t state = (pin1_val << 1) | pin2_val;
        int next_head = (encoder_buffer_head + 1) % ENCODER_BUFFER_SIZE;
    
        if (next_head == encoder_buffer_tail)
        {
            return;
        }
        encoder_buffer[encoder_buffer_head] = state;
        encoder_buffer_head = next_head;
    }
}


encoder_t *drv_encoder_get_enc_param(void)
{
    return &enc;
}


void drv_encoder_init(void (*callback)(uint32_t evt))
{   
    enc.m_encoder_evt = (drv_encoder_evt)callback;
    hal_gpioa_register_encoder_callback((isr_gpioa_callback_t)isr_encoder);

    uint32_t pad_ctl = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1);
    gpio_interrupt_trigger_t trigger = gpio_interrupt_trigger_edge_both;

    HPM_IOC->PAD[ENCODER_PIN_A].FUNC_CTL  = IOC_PA20_FUNC_CTL_GPIO_A_20;
    HPM_IOC->PAD[ENCODER_PIN_A].PAD_CTL   = pad_ctl;

    HPM_IOC->PAD[ENCODER_PIN_B].FUNC_CTL  = IOC_PA21_FUNC_CTL_GPIO_A_21;
    HPM_IOC->PAD[ENCODER_PIN_B].PAD_CTL   = pad_ctl;

    gpio_set_pin_input(ENCODER_PIN_CTRL, ENCODER_PIN_PORT, ENCODER_PIN_A);
    gpio_config_pin_interrupt(ENCODER_PIN_CTRL, ENCODER_PIN_PORT, ENCODER_PIN_A, trigger);
    gpio_enable_pin_interrupt(ENCODER_PIN_CTRL, ENCODER_PIN_PORT, ENCODER_PIN_A);
                           
    gpio_set_pin_input(ENCODER_PIN_CTRL, ENCODER_PIN_PORT, ENCODER_PIN_B);
    gpio_config_pin_interrupt(ENCODER_PIN_CTRL, ENCODER_PIN_PORT, ENCODER_PIN_B, trigger);
    gpio_enable_pin_interrupt(ENCODER_PIN_CTRL, ENCODER_PIN_PORT, ENCODER_PIN_B);

    prev_state = (ENCODER_READ_A() << 1) | ENCODER_READ_B();
    encoder_buffer_head = 0;
    encoder_buffer_tail = 0;
    encoder_accum = 0;
    last_count_time = xTaskGetTickCount();

    encoder_mutex = xSemaphoreCreateMutex();
    if (encoder_mutex == NULL) 
    {
        DBG_PRINTF("create encoder_mutex fail!\n");
        return;
    }

    encoder_timer = xTimerCreate("Encoder_Timer", pdMS_TO_TICKS(DEBOUNCE_TIME_MS), pdTRUE, 0, encoder_timer_callback);
    if (encoder_timer == NULL)
    {
        DBG_PRINTF("create encoder_timer fail!\n");
        return;
    }

    if (xTimerStart(encoder_timer, 0) != pdPASS)
    {
        DBG_PRINTF("timer start failure!\r\n");
    }
}

#endif
