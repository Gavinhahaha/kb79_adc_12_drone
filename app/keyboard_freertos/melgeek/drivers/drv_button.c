#include "drv_button.h"
#include "hpm_gpio_drv.h"
#include "hpm_mchtmr_drv.h"
#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "hal_gpio.h"

#if 0

#define BUTTON_READ_PIN_1()               gpio_read_pin(BUTTON_PIN_CTRL, BUTTON_PIN_PORT, BUTTON_PIN_1)
#define BUTTON_READ_PIN_2()               gpio_read_pin(BUTTON_PIN_CTRL, BUTTON_PIN_PORT, BUTTON_PIN_2) 

#define BUTTON_CLEAR_PIN_1_INT_FLAG()     gpio_check_clear_interrupt_flag(BUTTON_PIN_CTRL, BUTTON_PIN_PORT,  BUTTON_PIN_1)
#define BUTTON_CLEAR_PIN_2_INT_FLAG()     gpio_check_clear_interrupt_flag(BUTTON_PIN_CTRL, BUTTON_PIN_PORT,  BUTTON_PIN_2)

#define BUTTON_CHECK_PIN_1_INT_FLAG()     gpio_check_pin_interrupt_flag(BUTTON_PIN_CTRL, BUTTON_PIN_PORT, BUTTON_PIN_1)
#define BUTTON_CHECK_PIN_2_INT_FLAG()     gpio_check_pin_interrupt_flag(BUTTON_PIN_CTRL, BUTTON_PIN_PORT, BUTTON_PIN_2)

#define US_TO_COUNT(us)             ((uint64_t)(us) * 480)
#define BUTTON_DEBOUNCE_TIME_US     (8000) 

typedef enum 
{
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSED,
} button_state_t;

static drv_button_evt m_button_evt = NULL;
static button_state_t s_button1_state = BUTTON_STATE_RELEASED;
static button_state_t s_button2_state = BUTTON_STATE_RELEASED;
static volatile uint64_t s_button1_debounce_time = 0;
static volatile uint64_t s_button2_debounce_time = 0;


static void isr_button(void)
{
    uint64_t current_time = hpm_csr_get_core_cycle();

    if (BUTTON_CHECK_PIN_1_INT_FLAG())
    {
        BUTTON_CLEAR_PIN_1_INT_FLAG();
        bool is_pressed = !BUTTON_READ_PIN_1(); 

        if (is_pressed && (s_button1_state == BUTTON_STATE_RELEASED))
        {
            if ((current_time - s_button1_debounce_time) >= US_TO_COUNT(BUTTON_DEBOUNCE_TIME_US))
            {
                s_button1_debounce_time = current_time;
                s_button1_state = BUTTON_STATE_PRESSED;
                if (m_button_evt)
                    m_button_evt(BUTTON_EVT_BTN_1_DOWN);
            }
        }
        else if (!is_pressed && (s_button1_state == BUTTON_STATE_PRESSED))
        {
            if ((current_time - s_button1_debounce_time) >= US_TO_COUNT(BUTTON_DEBOUNCE_TIME_US))
            {
                s_button1_debounce_time = current_time;
                s_button1_state = BUTTON_STATE_RELEASED;
                if (m_button_evt)
                    m_button_evt(BUTTON_EVT_BTN_1_UP);
            }
        }
    }

    if (BUTTON_CHECK_PIN_2_INT_FLAG()) 
    {
        BUTTON_CLEAR_PIN_2_INT_FLAG();
        bool is_pressed = !BUTTON_READ_PIN_2();

        if (is_pressed && (s_button2_state == BUTTON_STATE_RELEASED))
        {
            if ((current_time - s_button2_debounce_time) >= US_TO_COUNT(BUTTON_DEBOUNCE_TIME_US))
            {
                s_button2_debounce_time = current_time;
                s_button2_state = BUTTON_STATE_PRESSED;
                if (m_button_evt)
                    m_button_evt(BUTTON_EVT_BTN_2_DOWN);
            }
        }
        else if (!is_pressed && (s_button2_state == BUTTON_STATE_PRESSED))
        {
            if ((current_time - s_button2_debounce_time) >= US_TO_COUNT(BUTTON_DEBOUNCE_TIME_US))
            {
                s_button2_debounce_time = current_time;
                s_button2_state = BUTTON_STATE_RELEASED;
                if (m_button_evt)
                    m_button_evt(BUTTON_EVT_BTN_2_UP);
            }
        }
    }
}

void drv_button_init(void (*callback)(uint32_t evt))
{
    hal_gpioa_register_button_callback((isr_gpioa_callback_t)isr_button);

    m_button_evt = (drv_button_evt)callback;

    uint32_t pad_ctl = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1);
    gpio_interrupt_trigger_t trigger = gpio_interrupt_trigger_edge_both;

    HPM_IOC->PAD[BUTTON_PIN_1].FUNC_CTL  = IOC_PA22_FUNC_CTL_GPIO_A_22;
    HPM_IOC->PAD[BUTTON_PIN_1].PAD_CTL   = pad_ctl;

    HPM_IOC->PAD[BUTTON_PIN_2].FUNC_CTL  = IOC_PA23_FUNC_CTL_GPIO_A_23;
    HPM_IOC->PAD[BUTTON_PIN_2].PAD_CTL   = pad_ctl;

    gpio_set_pin_input(BUTTON_PIN_CTRL, BUTTON_PIN_PORT, BUTTON_PIN_1);
    gpio_config_pin_interrupt(BUTTON_PIN_CTRL, BUTTON_PIN_PORT, BUTTON_PIN_1, trigger);
    gpio_enable_pin_interrupt(BUTTON_PIN_CTRL, BUTTON_PIN_PORT, BUTTON_PIN_1);
                           
    gpio_set_pin_input(BUTTON_PIN_CTRL, BUTTON_PIN_PORT, BUTTON_PIN_2);
    gpio_config_pin_interrupt(BUTTON_PIN_CTRL, BUTTON_PIN_PORT, BUTTON_PIN_2, trigger);
    gpio_enable_pin_interrupt(BUTTON_PIN_CTRL, BUTTON_PIN_PORT, BUTTON_PIN_2);

}
#endif