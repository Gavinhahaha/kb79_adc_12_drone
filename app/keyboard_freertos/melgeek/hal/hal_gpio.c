/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include "hal_gpio.h"
#include "hal_spi.h"
#include "board.h"
#include "hpm_gpio_drv.h"
#include "app_debug.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "hal_gpio"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif


#define GPIO_ISR_GPIOA_REGISTER_CB_NUM  5

static volatile isr_gpioa_callback_t isr_gpio_notify = NULL;
static volatile isr_gpioa_callback_t isr_gpio_button = NULL;
static volatile isr_gpioa_callback_t isr_gpio_encoder = NULL;


static const uint8_t reset_pin[SM_DEV_NUM_MAX][2] =
{
    {BOARD_RESET0_3_GPIO_INDEX, BOARD_RESET0_PIN},
    {BOARD_RESET0_3_GPIO_INDEX, BOARD_RESET1_PIN},
    {BOARD_RESET0_3_GPIO_INDEX, BOARD_RESET2_PIN},
    {BOARD_RESET0_3_GPIO_INDEX, BOARD_RESET3_PIN},
    {BOARD_RESET4_GPIO_INDEX,   BOARD_RESET4_PIN},
};

static const uint32_t notify_pin_mask = (1UL << BOARD_NTF0_PIN) | \
                                        (1UL << BOARD_NTF1_PIN) | \
                                        (1UL << BOARD_NTF2_PIN) | \
                                        (1UL << BOARD_NTF3_PIN) | \
                                        (1UL << BOARD_NTF4_PIN);

#if 0
static const uint32_t button_pin_mask = (1UL << BUTTON_PIN_1) | \
                                        (1UL << BUTTON_PIN_2);

static const uint32_t encoder_pin_mask = (1UL << ENCODER_PIN_A) | \
                                         (1UL << ENCODER_PIN_B);

static const uint32_t isr_gpioa_mask =  notify_pin_mask | button_pin_mask | encoder_pin_mask;
#else
static const uint32_t isr_gpioa_mask =  notify_pin_mask;
#endif


SDK_DECLARE_EXT_ISR_M(IRQn_GPIO0_A, isr_gpioa)
void isr_gpioa(void)
{
    uint32_t state = gpio_get_port_interrupt_flags(HPM_GPIO0, GPIO_DI_GPIOA) & isr_gpioa_mask;

    if ((state & notify_pin_mask) != 0) 
    {
        gpio_disable_port_interrupt_with_mask(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, notify_pin_mask);
        if (isr_gpio_notify != NULL) 
        {
            isr_gpio_notify();
        }
        gpio_clear_port_interrupt_flags_with_mask(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, notify_pin_mask);
    }

    #if 0
    if ((state & button_pin_mask) != 0) 
    {
        if (isr_gpio_button != NULL) 
        {
            isr_gpio_button();
        }
        gpio_clear_port_interrupt_flags_with_mask(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, button_pin_mask);
    }

    if ((state & encoder_pin_mask) != 0) 
    {
        if (isr_gpio_encoder != NULL) 
        {
            isr_gpio_encoder();
        }
        gpio_clear_port_interrupt_flags_with_mask(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, encoder_pin_mask);
    }
    #endif
}



uint8_t hal_notify_get_state(void)
{
    uint32_t io_state = ((uint32_t)gpio_read_port(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX));// & notify_pin_mask);
    uint8_t ntf_state = ((io_state >> BOARD_NTF3_PIN) & 1) << SM_DEV_NUM_0 \
                      | ((io_state >> BOARD_NTF0_PIN) & 1) << SM_DEV_NUM_1 \
                      | ((io_state >> BOARD_NTF2_PIN) & 1) << SM_DEV_NUM_2 \
                      | ((io_state >> BOARD_NTF1_PIN) & 1) << SM_DEV_NUM_3 \
                      | ((io_state >> BOARD_NTF4_PIN) & 1) << SM_DEV_NUM_4;
    return ntf_state;
}

void hal_sm_restart(sm_restart_state_t restart_sta)
{
    // boot
    gpio_write_pin(BOARD_BOOT_GPIO_CTRL, BOARD_BOOT_GPIO_INDEX, BOARD_BOOT_PIN, restart_sta);

    // restart
    gpio_set_pin_output_with_initial(BOARD_RESET_GPIOY_CTRL, reset_pin[0][0],  reset_pin[0][1],  0);
    gpio_set_pin_output_with_initial(BOARD_RESET_GPIOY_CTRL, reset_pin[1][0],  reset_pin[1][1],  0);
    gpio_set_pin_output_with_initial(BOARD_RESET_GPIOY_CTRL, reset_pin[2][0],  reset_pin[2][1],  0);
    gpio_set_pin_output_with_initial(BOARD_RESET_GPIOY_CTRL, reset_pin[3][0],  reset_pin[3][1],  0);
    gpio_set_pin_output_with_initial(BOARD_RESET_GPIOA_CTRL, reset_pin[4][0],  reset_pin[4][1],  0);

    board_delay_ms(10); // restart time
    gpio_set_pin_output_with_initial(BOARD_RESET_GPIOY_CTRL, reset_pin[0][0],  reset_pin[0][1],  1);
    gpio_set_pin_output_with_initial(BOARD_RESET_GPIOY_CTRL, reset_pin[1][0],  reset_pin[1][1],  1);
    gpio_set_pin_output_with_initial(BOARD_RESET_GPIOY_CTRL, reset_pin[2][0],  reset_pin[2][1],  1);
    gpio_set_pin_output_with_initial(BOARD_RESET_GPIOY_CTRL, reset_pin[3][0],  reset_pin[3][1],  1);
    gpio_set_pin_output_with_initial(BOARD_RESET_GPIOA_CTRL, reset_pin[4][0],  reset_pin[4][1],  1);
    board_delay_ms(10);
}

void hal_sync_start(void)
{
    gpio_enable_port_interrupt_with_mask(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, notify_pin_mask);
    gpio_toggle_pin(BOARD_SYNC_GPIO_CTRL, BOARD_SYNC_GPIO_INDEX, BOARD_SYNC_PIN);
    gpio_toggle_pin(BOARD_SYNC_GPIO_CTRL, BOARD_SYNC_GPIO_INDEX, BOARD_SYNC_PIN);
}


void hal_notify_enable(uint8_t en)
{
    if (en)
    {
        gpio_enable_port_interrupt_with_mask(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, notify_pin_mask);
    }
    else
    {
        gpio_disable_port_interrupt_with_mask(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, notify_pin_mask);
    }
}

/// @brief
/// @param
void hal_gpio_init(void)
{
    // boot 高有效
    uint32_t pad_ctl = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(0); //下拉使能
    HPM_IOC->PAD[IOC_PAD_PA01].PAD_CTL = pad_ctl;
    gpio_set_pin_output_with_initial(HPM_GPIO0, GPIO_DI_GPIOA, 1, 0);

    // sync
    pad_ctl = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1); //上拉使能
    HPM_IOC->PAD[IOC_PAD_PA24].PAD_CTL = pad_ctl;
    gpio_set_pin_output_with_initial(BOARD_SYNC_GPIO_CTRL, BOARD_SYNC_GPIO_INDEX, BOARD_SYNC_PIN, 1);

    // notify input interrupt
    gpio_interrupt_trigger_t trigger = gpio_interrupt_trigger_edge_rising;
    HPM_IOC->PAD[IOC_PAD_PA14].PAD_CTL = pad_ctl;
    HPM_IOC->PAD[IOC_PAD_PA15].PAD_CTL = pad_ctl;
    HPM_IOC->PAD[IOC_PAD_PA30].PAD_CTL = pad_ctl;
    HPM_IOC->PAD[IOC_PAD_PA31].PAD_CTL = pad_ctl;
    HPM_IOC->PAD[IOC_PAD_PA11].PAD_CTL = pad_ctl;
    gpio_set_pin_input(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, BOARD_NTF0_PIN);
    gpio_set_pin_input(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, BOARD_NTF1_PIN);
    gpio_set_pin_input(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, BOARD_NTF2_PIN);
    gpio_set_pin_input(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, BOARD_NTF3_PIN);
    gpio_set_pin_input(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, BOARD_NTF4_PIN);
    gpio_config_pin_interrupt(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, BOARD_NTF0_PIN, trigger);
    gpio_config_pin_interrupt(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, BOARD_NTF1_PIN, trigger);
    gpio_config_pin_interrupt(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, BOARD_NTF2_PIN, trigger);
    gpio_config_pin_interrupt(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, BOARD_NTF3_PIN, trigger);
    gpio_config_pin_interrupt(BOARD_NTF_GPIO_CTRL, BOARD_NTF_GPIO_INDEX, BOARD_NTF4_PIN, trigger);

    hal_notify_enable(1);
    intc_m_enable_irq_with_priority(BOARD_NTF_GPIO_IRQ, 3);

    // reset 低有效
    HPM_IOC->PAD[IOC_PAD_PY00].PAD_CTL = pad_ctl;// | IOC_PAD_PAD_CTL_HYS_SET(1); 
    HPM_IOC->PAD[IOC_PAD_PY01].PAD_CTL = pad_ctl;// | IOC_PAD_PAD_CTL_HYS_SET(1);
    HPM_IOC->PAD[IOC_PAD_PY02].PAD_CTL = pad_ctl;// | IOC_PAD_PAD_CTL_HYS_SET(1);
    HPM_IOC->PAD[IOC_PAD_PY03].PAD_CTL = pad_ctl;// | IOC_PAD_PAD_CTL_HYS_SET(1);
    HPM_IOC->PAD[IOC_PAD_PA10].PAD_CTL = pad_ctl;// | IOC_PAD_PAD_CTL_HYS_SET(1);
    gpio_set_pin_output_with_initial(BOARD_RESET_GPIOY_CTRL, reset_pin[0][0],  reset_pin[0][1],  1);
    gpio_set_pin_output_with_initial(BOARD_RESET_GPIOY_CTRL, reset_pin[1][0],  reset_pin[1][1],  1);
    gpio_set_pin_output_with_initial(BOARD_RESET_GPIOY_CTRL, reset_pin[2][0],  reset_pin[2][1],  1);
    gpio_set_pin_output_with_initial(BOARD_RESET_GPIOY_CTRL, reset_pin[3][0],  reset_pin[3][1],  1);
    gpio_set_pin_output_with_initial(BOARD_RESET_GPIOA_CTRL, reset_pin[4][0],  reset_pin[4][1],  1);

    hal_sm_restart(SM_RESTART_APP);
    
    //test point
    BOARD_TEST_PIN0_OUTPUT();
    BOARD_TEST_PIN1_OUTPUT();
    BOARD_TEST_PIN2_OUTPUT();
}


void hal_gpioa_register_notify_callback(isr_gpioa_callback_t isr_cb)
{
    isr_gpio_notify = isr_cb;
}
void hal_gpioa_unregister_notify_callback(void)
{
    isr_gpio_notify = NULL;
}

void hal_gpioa_register_button_callback(isr_gpioa_callback_t isr_cb)
{
    isr_gpio_button = isr_cb;
}
void hal_gpioa_unregister_button_callback(void)
{
    isr_gpio_button = NULL;
}

void hal_gpioa_register_encoder_callback(isr_gpioa_callback_t isr_cb)
{
    isr_gpio_encoder = isr_cb;
}
void hal_gpioa_unregister_encoder_callback(void)
{
    isr_gpio_encoder = NULL;
}


