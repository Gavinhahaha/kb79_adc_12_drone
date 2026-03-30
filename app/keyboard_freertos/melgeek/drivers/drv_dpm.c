#include "drv_dpm.h"
#include "hpm_gpio_drv.h"
#include "app_debug.h"
#include "board.h"


void drv_dpm_power_en(bool en)
{
    gpio_write_pin(DPM_PIN_CTRL, DPM_PIN_PORT, DPM_PWR_PIN, en ? IO_HIGH : IO_LOW);
}

void drv_dpm_reset(void)
{
    gpio_write_pin(DPM_PIN_CTRL, DPM_PIN_PORT, DPM_RESET_PIN, IO_HIGH);
    board_delay_ms(50);
    gpio_write_pin(DPM_PIN_CTRL, DPM_PIN_PORT, DPM_RESET_PIN, IO_LOW);
    board_delay_ms(50);
    gpio_write_pin(DPM_PIN_CTRL, DPM_PIN_PORT, DPM_RESET_PIN, IO_HIGH);
}

void drv_dpm_gpio_init(void)
{
    HPM_IOC->PAD[DPM_PWR_PIN].FUNC_CTL = IOC_PA20_FUNC_CTL_GPIO_A_20;
    gpio_set_pin_output_with_initial(DPM_PIN_CTRL, DPM_PIN_PORT, DPM_PWR_PIN, IO_HIGH);

    HPM_IOC->PAD[DPM_RESET_PIN].FUNC_CTL = IOC_PA21_FUNC_CTL_GPIO_A_21;
    gpio_set_pin_output_with_initial(DPM_PIN_CTRL, DPM_PIN_PORT, DPM_RESET_PIN, IO_HIGH);
}



