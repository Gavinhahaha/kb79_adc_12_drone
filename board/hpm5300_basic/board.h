/* 76
 * @Description: 
 * 
 * Copyright (c) 2024 Melgeek, All Rights Reserved. 
 */

#ifndef _HPM_BOARD_H
#define _HPM_BOARD_H
#include <stdio.h>
#include <stdarg.h>
#include "hpm_common.h"
#include "hpm_clock_drv.h"
#include "hpm_soc.h"
#include "hpm_soc_feature.h"
#include "pinmux.h"
#if !defined(CONFIG_NDEBUG_CONSOLE) || !CONFIG_NDEBUG_CONSOLE
#include "hpm_debug_console.h"
#endif

#define RGB_ENABLED
#define LB_ENABLED

#define BOARD_NAME                      "CENTAURI80"
#define BOARD_BT_NAME                   "MelGeek CENTAURI80"
#define BOARD_UF2_SIGNATURE             (0x0A4D5048UL)

#define BOARD_HIVE_VER                  (0x101)
#define BOARD_FW_VERSION                (656)                  /* Firmware   Version     */
#define BOARD_HW_VERSION                (101)                  /* Hardware   Version     */

#define BOARD_SM_FW_VERSION             (128)                  /* nation firmware version     */

#define BOARD_FW_FOR_FA                 (1)

#define BOARD_INACTIVE_TIME             (5 * 60 * 1000)
#define BOARD_VN                        "MelGeek"              /* Vendor     Name        */
#define BOARD_VC                        (0x005A4848)           /* Vendor     Code        */
#define BOARD_PC                        (0x00000815)           /* Pruduction Code        */
#define BOARD_UI                        ("centauri80")           /* User       information */

#define BOARD_LED_INDICATOR_NUM         (3)
#define BOARD_LED_INDEX_INDICATOR1      (81)
#define BOARD_LED_INDEX_INDICATOR2      (82)
#define BOARD_LED_INDEX_INDICATOR3      (83)

#define BOARD_LED_ROW_NUM               (6)
#define BOARD_LED_COLUMN_NUM            (15)
#define BOARD_SPACE_LED_NUM             (3)
#define BOARD_KEY_LED_NUM               (78 + BOARD_SPACE_LED_NUM)//空格键3个灯
#define BOARD_LED_COLOR_LAYER_MAX       (1)
#define BOARD_LED_NUM                   (BOARD_LED_INDICATOR_NUM + BOARD_KEY_LED_NUM)

#define BOARD_LIGHT_TAPE_BACK_LED_NUM   (42)
#define BOARD_LIGHT_TAPE_BACK_ROW_NUM   (1U)
#define BOARD_LIGHT_TAPE_BACK_COL_NUM   (42U)
#define BOARD_LIGHT_TAPE_SIDE_DIV_NUM   (23U)
#define BOARD_LIGHT_TAPE_SIDE_LED_NUM   (BOARD_LIGHT_TAPE_SIDE_DIV_NUM * 2)
#define BOARD_LIGHT_TAPE_LED_NUM        (BOARD_LIGHT_TAPE_BACK_LED_NUM + BOARD_LIGHT_TAPE_SIDE_LED_NUM)


#define BOARD_SPACE_KEY_INDEX           (72)  


#define KB_NUMLOCK_MASK                 (1 << 0)
#define KB_CAPSLOCK_MASK                (1 << 1)

#define LED_DRV_IC_IS37XX   (0)
#define LED_DRV_IC_SN3734   (1)
#define LED_DRV_IC_AW20XXX  (2)
#define BOARD_LED_DRV_IC_MODEL   LED_DRV_IC_IS37XX

/* ACMP desction */
#define BOARD_ACMP             HPM_ACMP
#define BOARD_ACMP_CHANNEL     ACMP_CHANNEL_CHN1
#define BOARD_ACMP_IRQ         IRQn_ACMP_1
#define BOARD_ACMP_PLUS_INPUT  ACMP_INPUT_DAC_OUT  /* use internal DAC */
#define BOARD_ACMP_MINUS_INPUT ACMP_INPUT_ANALOG_4 /* align with used pin */

/* dma section */
#define BOARD_APP_HDMA      HPM_HDMA
#define BOARD_APP_HDMA_IRQ  IRQn_HDMA
#define BOARD_APP_DMAMUX    HPM_DMAMUX
#define TEST_DMA_CONTROLLER HPM_HDMA
#define TEST_DMA_IRQ        IRQn_HDMA

#ifndef BOARD_RUNNING_CORE
#define BOARD_RUNNING_CORE HPM_CORE0
#endif

/* uart section */
#ifndef BOARD_APP_UART_BASE
#define BOARD_APP_UART_BASE HPM_UART2
#define BOARD_APP_UART_IRQ  IRQn_UART2
#define BOARD_APP_UART_BAUDRATE   (115200UL)
#define BOARD_APP_UART_CLK_NAME   clock_uart2
#define BOARD_APP_UART_RX_DMA_REQ HPM_DMA_SRC_UART2_RX
#define BOARD_APP_UART_TX_DMA_REQ HPM_DMA_SRC_UART2_TX
#endif

/* uart lin sample section */
#define BOARD_UART_LIN          HPM_UART3
#define BOARD_UART_LIN_IRQ      IRQn_UART3
#define BOARD_UART_LIN_CLK_NAME clock_uart3
#define BOARD_UART_LIN_TX_PORT  GPIO_DI_GPIOA
#define BOARD_UART_LIN_TX_PIN   (15U) /* PA15 should align with used pin in pinmux configuration */


#if !defined(CONFIG_NDEBUG_CONSOLE) || !CONFIG_NDEBUG_CONSOLE
#ifndef BOARD_CONSOLE_TYPE
#define BOARD_CONSOLE_TYPE CONSOLE_TYPE_UART
#endif

#if BOARD_CONSOLE_TYPE == CONSOLE_TYPE_UART
#ifndef BOARD_CONSOLE_UART_BASE
#define BOARD_CONSOLE_UART_BASE     HPM_UART0
#define BOARD_CONSOLE_UART_CLK_NAME clock_uart0
#define BOARD_CONSOLE_UART_IRQ      IRQn_UART0
#define BOARD_CONSOLE_UART_TX_DMA_REQ HPM_DMA_SRC_UART0_TX
#define BOARD_CONSOLE_UART_RX_DMA_REQ HPM_DMA_SRC_UART0_RX
#endif
#define BOARD_CONSOLE_UART_BAUDRATE (921600UL)
#endif
#endif

/* usb cdc acm uart section */
#define BOARD_USB_CDC_ACM_UART            BOARD_APP_UART_BASE
#define BOARD_USB_CDC_ACM_UART_CLK_NAME   BOARD_APP_UART_CLK_NAME
#define BOARD_USB_CDC_ACM_UART_TX_DMA_SRC BOARD_APP_UART_TX_DMA_REQ
#define BOARD_USB_CDC_ACM_UART_RX_DMA_SRC BOARD_APP_UART_RX_DMA_REQ

/* rtthread-nano finsh section */
#define BOARD_RT_CONSOLE_BASE        BOARD_CONSOLE_UART_BASE

/* modbus sample section */
#define BOARD_MODBUS_UART_BASE       BOARD_APP_UART_BASE
#define BOARD_MODBUS_UART_CLK_NAME   BOARD_APP_UART_CLK_NAME
#define BOARD_MODBUS_UART_RX_DMA_REQ BOARD_APP_UART_RX_DMA_REQ
#define BOARD_MODBUS_UART_TX_DMA_REQ BOARD_APP_UART_TX_DMA_REQ

/* nor flash section */
#define BOARD_FLASH_BASE_ADDRESS (0x80000000UL) /* Check */
#define BOARD_FLASH_SIZE         (SIZE_1MB)

/* i2c section */
#define BOARD_APP_I2C_BASE     HPM_I2C0
#define BOARD_APP_I2C_IRQ      IRQn_I2C0
#define BOARD_APP_I2C_CLK_NAME clock_i2c0
#define BOARD_APP_I2C_DMA      HPM_HDMA
#define BOARD_APP_I2C_DMAMUX   HPM_DMAMUX
#define BOARD_APP_I2C_DMA_SRC  HPM_DMA_SRC_I2C0

#define BOARD_APP_I2C2_BASE     HPM_I2C2
#define BOARD_APP_I2C2_IRQ      IRQn_I2C2
#define BOARD_APP_I2C2_CLK_NAME clock_i2c2
#define BOARD_APP_I2C2_DMA      HPM_HDMA
#define BOARD_APP_I2C2_DMAMUX   HPM_DMAMUX
#define BOARD_APP_I2C2_DMA_SRC  HPM_DMA_SRC_I2C2

#define BOARD_APP_I2C3_BASE     HPM_I2C3
#define BOARD_APP_I2C3_IRQ      IRQn_I2C3
#define BOARD_APP_I2C3_CLK_NAME clock_i2c3
#define BOARD_APP_I2C3_DMA      HPM_HDMA
#define BOARD_APP_I2C3_DMAMUX   HPM_DMAMUX
#define BOARD_APP_I2C3_DMA_SRC  HPM_DMA_SRC_I2C3

/* gptmr section */
#define BOARD_GPTMR                   HPM_GPTMR2
#define BOARD_GPTMR_IRQ               IRQn_GPTMR2
#define BOARD_GPTMR_CHANNEL           2
#define BOARD_GPTMR_DMA_SRC           HPM_DMA_SRC_GPTMR2_0
#define BOARD_GPTMR_CLK_NAME          clock_gptmr2
#define BOARD_GPTMR_PWM               HPM_GPTMR0
#define BOARD_GPTMR_PWM_CHANNEL       0
#define BOARD_GPTMR_PWM_DMA_SRC       HPM_DMA_SRC_GPTMR0_0
#define BOARD_GPTMR_PWM_CLK_NAME      clock_gptmr0
#define BOARD_GPTMR_PWM_IRQ           IRQn_GPTMR0
#define BOARD_GPTMR_PWM_SYNC          HPM_GPTMR0
#define BOARD_GPTMR_PWM_SYNC_CHANNEL  1
#define BOARD_GPTMR_PWM_SYNC_CLK_NAME clock_gptmr0

/* User LED */
#define BOARD_LED_GPIO_CTRL  HPM_GPIO0
#define BOARD_LED_GPIO_INDEX GPIO_DI_GPIOA
#define BOARD_LED_GPIO_PIN   23

/* test point tigger */
#define BOARD_TEST_GPIO_EN      (1)

#define BOARD_TEST_GPIO_CTRL    HPM_GPIO0
#define BOARD_TEST_GPIO_INDEX   GPIO_DI_GPIOA
#define BOARD_TEST_GPIO_INDEX1  GPIO_DI_GPIOA
#define BOARD_TEST_GPIO_INDEX2  GPIO_DI_GPIOY
#define BOARD_TEST_GPIO_PIN     0
#define BOARD_TEST_GPIO_PIN1    9
#define BOARD_TEST_GPIO_PIN2    4

#define BOARD_HWV_GPIO_PIN1 6
#define BOARD_HWV_GPIO_PIN2 4

#define BOARD_TEST_PIN0_ENABLE
#define BOARD_TEST_PIN1_ENABLE
#define BOARD_TEST_PIN2_ENABLE

#ifndef BOARD_TEST_PIN0_ENABLE
#define BOARD_TEST_PIN0_OUTPUT()
#define BOARD_TEST_PIN0_WRITE(x)
#define BOARD_TEST_PIN0_TOGGLE()
#else
#define BOARD_TEST_PIN0_OUTPUT()    \
    HPM_PIOC->PAD[IOC_PAD_PA00].FUNC_CTL = IOC_PA00_FUNC_CTL_GPIO_A_00; \
    HPM_IOC->PAD[IOC_PAD_PA00].PAD_CTL = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1); \
    gpio_set_pin_output(BOARD_TEST_GPIO_CTRL, BOARD_TEST_GPIO_INDEX, BOARD_TEST_GPIO_PIN);
#define BOARD_TEST_PIN0_WRITE(x) \
    gpio_write_pin(BOARD_TEST_GPIO_CTRL, BOARD_TEST_GPIO_INDEX, BOARD_TEST_GPIO_PIN, x);
#define BOARD_TEST_PIN0_TOGGLE() \
    gpio_toggle_pin(BOARD_TEST_GPIO_CTRL, BOARD_TEST_GPIO_INDEX, BOARD_TEST_GPIO_PIN);
#endif

#ifndef BOARD_TEST_PIN1_ENABLE
#define BOARD_TEST_PIN1_OUTPUT()
#define BOARD_TEST_PIN1_WRITE(x)
#define BOARD_TEST_PIN1_TOGGLE()
#else
#define BOARD_TEST_PIN1_OUTPUT()    \
    HPM_PIOC->PAD[IOC_PAD_PA09].FUNC_CTL = IOC_PA09_FUNC_CTL_GPIO_A_09; \
    HPM_IOC->PAD[IOC_PAD_PA09].PAD_CTL = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1); \
    gpio_set_pin_output(BOARD_TEST_GPIO_CTRL, BOARD_TEST_GPIO_INDEX1, BOARD_TEST_GPIO_PIN1);
#define BOARD_TEST_PIN1_WRITE(x) \
    gpio_write_pin(BOARD_TEST_GPIO_CTRL, BOARD_TEST_GPIO_INDEX1, BOARD_TEST_GPIO_PIN1, x);
#define BOARD_TEST_PIN1_TOGGLE() \
    gpio_toggle_pin(BOARD_TEST_GPIO_CTRL, BOARD_TEST_GPIO_INDEX1, BOARD_TEST_GPIO_PIN1);
#endif

#ifndef BOARD_TEST_PIN2_ENABLE
#define BOARD_TEST_PIN2_OUTPUT()
#define BOARD_TEST_PIN2_WRITE(x)
#define BOARD_TEST_PIN2_TOGGLE()
#else
#define BOARD_TEST_PIN2_OUTPUT() \
    HPM_PIOC->PAD[IOC_PAD_PY04].FUNC_CTL = IOC_PY04_FUNC_CTL_SOC_GPIO_Y_04; \
    HPM_IOC->PAD[IOC_PAD_PY04].PAD_CTL = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1) | IOC_PAD_PAD_CTL_HYS_SET(1); \
    gpio_set_pin_output(BOARD_TEST_GPIO_CTRL, BOARD_TEST_GPIO_INDEX2, BOARD_TEST_GPIO_PIN2);
#define BOARD_TEST_PIN2_WRITE(x) \
    gpio_write_pin(BOARD_TEST_GPIO_CTRL, BOARD_TEST_GPIO_INDEX2, BOARD_TEST_GPIO_PIN2, x);
#define BOARD_TEST_PIN2_TOGGLE() \
    gpio_toggle_pin(BOARD_TEST_GPIO_CTRL, BOARD_TEST_GPIO_INDEX2, BOARD_TEST_GPIO_PIN2);
#endif

/* mojo68rt HALL*/
#define BOARD_HALL_GPIO_CTRL HPM_GPIO0
#define BOARD_CS_A0_PIN 19
#define BOARD_CS_A1_PIN 20
#define BOARD_CS_A2_PIN 21
#define BOARD_H_EN0_PIN 31
#define BOARD_H_EN1_PIN 30
#define BOARD_H_EN2_PIN 29
#define BOARD_H_EN3_PIN 28
#define BOARD_H_EN4_PIN 27
#define BOARD_H_EN5_PIN 26
#define BOARD_H_EN7_PIN 23

#define BOARD_LED_OFF_LEVEL 1
#define BOARD_LED_ON_LEVEL  0

/* 12V Power Enable for lin transceiver */
#define BOARD_SUPPORT_LIN_TRANSCEIVER_CONTROL 1
#define BOARD_12V_EN_GPIO_CTRL  HPM_GPIO0
#define BOARD_12V_EN_GPIO_INDEX GPIO_DI_GPIOA
#define BOARD_12V_EN_GPIO_PIN   24
#define BOARD_LIN_TRANSCEIVER_GPIO_CTRL  HPM_GPIO0
#define BOARD_LIN_TRANSCEIVER_GPIO_INDEX GPIO_DI_GPIOA
#define BOARD_LIN_TRANSCEIVER_GPIO_PIN   13


/* gpiom section */
#define BOARD_APP_GPIOM_BASE            HPM_GPIOM
#define BOARD_APP_GPIOM_USING_CTRL      HPM_FGPIO
#define BOARD_APP_GPIOM_USING_CTRL_NAME gpiom_core0_fast

/* User button */
#define BOARD_APP_GPIO_CTRL  HPM_GPIO0
#define BOARD_APP_GPIO_INDEX GPIO_DI_GPIOA
#define BOARD_APP_GPIO_PIN 9
#define BOARD_APP_GPIO_IRQ   IRQn_GPIO0_A

//* Encoder section */
#if 0
#define  ENCODER_PIN_NUM        (2)
#define  ENCODER_PIN_CTRL       (HPM_GPIO0)
#define  ENCODER_PIN_PORT       (GPIO_DI_GPIOA)
#define  ENCODER_PIN_IRQ        IRQn_GPIO0_A
#define  ENCODER_PIN_A          (IOC_PAD_PA20)
#define  ENCODER_PIN_B          (IOC_PAD_PA21)

#define  BUTTON_NUM             (2)
#define  BUTTON_PIN_CTRL        (HPM_GPIO0)
#define  BUTTON_PIN_PORT        (GPIO_DI_GPIOA)
#define  BUTTON_PIN_IRQ         IRQn_GPIO0_A
#define  BUTTON_PIN_1           (IOC_PAD_PA22)
#define  BUTTON_PIN_2           (IOC_PAD_PA23)
#endif

/* spi section */
//#define BOARD_APP_SPI_BASE              HPM_SPI1
//#define BOARD_APP_SPI_CLK_NAME          clock_spi1
//#define BOARD_APP_SPI_IRQ               IRQn_SPI1
//#define BOARD_APP_SPI_SCLK_FREQ         (20000000UL)
//#define BOARD_APP_SPI_ADDR_LEN_IN_BYTES (1U)
//#define BOARD_APP_SPI_DATA_LEN_IN_BITS  (8U)
//#define BOARD_APP_SPI_RX_DMA            HPM_DMA_SRC_SPI1_RX
//#define BOARD_APP_SPI_TX_DMA            HPM_DMA_SRC_SPI1_TX
//#define BOARD_SPI_CS_GPIO_CTRL          HPM_GPIO0
//#define BOARD_SPI_CS_PIN                IOC_PAD_PA26
//#define BOARD_SPI_CS_ACTIVE_LEVEL       (0U)

// demo spi
//#define BOARD_APP_SPI_BASE              HPM_SPI2
//#define BOARD_APP_SPI_CLK_NAME          clock_spi2
//#define BOARD_APP_SPI_IRQ               IRQn_SPI2
//#define BOARD_APP_SPI_SCLK_FREQ         (20000000UL)
//#define BOARD_APP_SPI_ADDR_LEN_IN_BYTES (1U)
//#define BOARD_APP_SPI_DATA_LEN_IN_BITS  (8U)
//#define BOARD_APP_SPI_RX_DMA            HPM_DMA_SRC_SPI2_RX
//#define BOARD_APP_SPI_TX_DMA            HPM_DMA_SRC_SPI2_TX

/* centauri spi */
#define BOARD_SPI_SCLK_FREQ             (32000000UL)
#define BOARD_SPI1_BASE                 HPM_SPI1
#define BOARD_SPI1_CLK_NAME             clock_spi1
#define BOARD_SPI1_IRQ                  IRQn_SPI1
#define BOARD_SPI1_SCLK_FREQ            (BOARD_SPI_SCLK_FREQ)
#define BOARD_SPI1_DATA_LEN_IN_BITS     (16U)
#define BOARD_SPI1_ADDR_LEN_IN_BYTES    ((BOARD_SPI1_DATA_LEN_IN_BITS + 7) / 8)
#define BOARD_SPI1_RX_DMA               HPM_DMA_SRC_SPI1_RX
#define BOARD_SPI1_TX_DMA               HPM_DMA_SRC_SPI1_TX

#define BOARD_SPI2_BASE                 HPM_SPI2
#define BOARD_SPI2_CLK_NAME             clock_spi2
#define BOARD_SPI2_IRQ                  IRQn_SPI2
#define BOARD_SPI2_SCLK_FREQ            (BOARD_SPI_SCLK_FREQ)
#define BOARD_SPI2_DATA_LEN_IN_BITS     (16U)
#define BOARD_SPI2_ADDR_LEN_IN_BYTES    ((BOARD_SPI2_DATA_LEN_IN_BITS + 7) / 8)
#define BOARD_SPI2_RX_DMA               HPM_DMA_SRC_SPI2_RX
#define BOARD_SPI2_TX_DMA               HPM_DMA_SRC_SPI2_TX

/* centauri cs */
#define BOARD_SPI_CS_GPIO_CTRL          HPM_GPIO0
#define BOARD_SPI_CS0_PIN               IOC_PAD_PB02
#define BOARD_SPI_CS1_PIN               IOC_PAD_PB03
#define BOARD_SPI_CS2_PIN               IOC_PAD_PB04
#define BOARD_SPI_CS3_PIN               IOC_PAD_PB05
#define BOARD_SPI_CS4_PIN               IOC_PAD_PA26
#define BOARD_SPI_CS_ACTIVE_LEVEL       (0U)

/* centauri notify */
#define BOARD_NTF_PIN_NUM       (5)
#define BOARD_NTF_GPIO_CTRL     HPM_GPIO0
#define BOARD_NTF_GPIO_INDEX    GPIO_DI_GPIOA
#define BOARD_NTF_GPIO_IRQ      IRQn_GPIO0_A
#define BOARD_NTF0_PIN 14
#define BOARD_NTF1_PIN 15
#define BOARD_NTF2_PIN 30
#define BOARD_NTF3_PIN 31
#define BOARD_NTF4_PIN 11

/* centauri sync */
#define BOARD_SYNC_GPIO_CTRL   HPM_GPIO0
#define BOARD_SYNC_GPIO_INDEX  GPIO_DI_GPIOA
#define BOARD_SYNC_PIN 24

/* centauri boot */
#define BOARD_BOOT_GPIO_CTRL   HPM_GPIO0
#define BOARD_BOOT_GPIO_INDEX  GPIO_DI_GPIOA
#define BOARD_BOOT_PIN 1

/* centauri reset */
#define BOARD_RESET_GPIOA_CTRL       HPM_GPIO0
#define BOARD_RESET_GPIOY_CTRL       HPM_PGPIO
#define BOARD_RESET0_3_GPIO_INDEX   GPIO_DI_GPIOY
#define BOARD_RESET0_PIN 0
#define BOARD_RESET1_PIN 1
#define BOARD_RESET2_PIN 2
#define BOARD_RESET3_PIN 3

#define BOARD_RESET4_GPIO_INDEX     GPIO_DI_GPIOA
#define BOARD_RESET4_PIN 10

/* ADC section */
#define BOARD_APP_ADC0_NAME     "ADC0"
#define BOARD_APP_ADC0_BASE     HPM_ADC0
#define BOARD_APP_ADC0_IRQn     IRQn_ADC0
#define BOARD_APP_ADC0_CH_1      (2u)//PB10(5U)//PB13 (6U)//PB14   (3U)//pb11 
#define BOARD_APP_ADC0_CLK_NAME (clock_adc0)
#define BOARD_APP_ADC0_HW_TRGM_OUT_SEQ TRGM_TRGOCFG_ADC0_STRGI

#define BOARD_APP_ADC1_NAME      "ADC1"
#define BOARD_APP_ADC1_BASE      HPM_ADC1                       
#define BOARD_APP_ADC1_IRQn      IRQn_ADC1
#define BOARD_APP_ADC1_CH_1      (4U)
#define BOARD_APP_ADC1_CLK_NAME  (clock_adc1)
#define BOARD_APP_ADC1_HW_TRGM_OUT_SEQ TRGM_TRGOCFG_ADC1_STRGI

#define BOARD_APP_ADC16_HW_TRIG_SRC     HPM_PWM0
#define BOARD_APP_ADC16_HW_TRGM         HPM_TRGM0
#define BOARD_APP_ADC16_HW_TRGM_IN      HPM_TRGM0_INPUT_SRC_GPTMR2_OUT2
#define BOARD_APP_ADC16_HW_TRGM_OUT_PMT TRGM_TRGOCFG_ADCX_PTRGI0A

#define BOARD_APP_ADC16_PMT_TRIG_CH ADC16_CONFIG_TRG0A

/* DAC section */
#define BOARD_DAC_BASE           HPM_DAC0
#define BOARD_DAC_IRQn           IRQn_DAC0
#define BOARD_APP_DAC_CLOCK_NAME clock_dac0

/* Flash section */
#define BOARD_APP_XPI_NOR_XPI_BASE     (HPM_XPI0)
#define BOARD_APP_XPI_NOR_CFG_OPT_HDR  (0xfcf90002U)
#define BOARD_APP_XPI_NOR_CFG_OPT_OPT0 (0x00000006U)
#define BOARD_APP_XPI_NOR_CFG_OPT_OPT1 (0x00001000U)

/* SDXC section */
#define BOARD_APP_SDCARD_SDXC_BASE   (HPM_SDXC0)
#define BOARD_APP_SDCARD_SUPPORT_1V8 (0)

/* MCAN section */
#define BOARD_APP_CAN_BASE HPM_MCAN3
#define BOARD_APP_CAN_IRQn IRQn_MCAN3

/* CALLBACK TIMER section */
#define BOARD_CALLBACK_TIMER          (HPM_GPTMR3)
#define BOARD_CALLBACK_TIMER_CH       1
#define BOARD_CALLBACK_TIMER_IRQ      IRQn_GPTMR3
#define BOARD_CALLBACK_TIMER_CLK_NAME (clock_gptmr3)

/* APP PWM */
#define BOARD_APP_PWM             HPM_PWM0
#define BOARD_APP_PWM_CLOCK_NAME  clock_mot0
#define BOARD_APP_PWM_OUT1        2
#define BOARD_APP_PWM_OUT2        3
#define BOARD_APP_TRGM            HPM_TRGM0
#define BOARD_APP_PWM_IRQ         IRQn_PWM0
#define BOARD_APP_TRGM_PWM_OUTPUT TRGM_TRGOCFG_PWM0_SYNCI

/*BLDC pwm*/
/*PWM define*/
#define BOARD_BLDCPWM              HPM_PWM0
#define BOARD_BLDC_UH_PWM_OUTPIN   (6U)
#define BOARD_BLDC_UL_PWM_OUTPIN   (7U)
#define BOARD_BLDC_VH_PWM_OUTPIN   (4U)
#define BOARD_BLDC_VL_PWM_OUTPIN   (5U)
#define BOARD_BLDC_WH_PWM_OUTPIN   (2U)
#define BOARD_BLDC_WL_PWM_OUTPIN   (3U)
#define BOARD_BLDCPWM_TRGM         HPM_TRGM0
#define BOARD_BLDCAPP_PWM_IRQ      IRQn_PWM0
#define BOARD_BLDCPWM_CMP_INDEX_0  (0U)
#define BOARD_BLDCPWM_CMP_INDEX_1  (1U)
#define BOARD_BLDCPWM_CMP_INDEX_2  (2U)
#define BOARD_BLDCPWM_CMP_INDEX_3  (3U)
#define BOARD_BLDCPWM_CMP_INDEX_4  (4U)
#define BOARD_BLDCPWM_CMP_INDEX_5  (5U)
#define BOARD_BLDCPWM_CMP_INDEX_6  (6U)
#define BOARD_BLDCPWM_CMP_INDEX_7  (7U)
#define BOARD_BLDCPWM_CMP_TRIG_CMP (20U)

/*HALL define*/

/*RDC*/
#define BOARD_RDC_TRGM            HPM_TRGM0
#define BOARD_RDC_TRGIGMUX_IN_NUM HPM_TRGM0_INPUT_SRC_RDC_TRGO_0
#define BOARD_RDC_TRG_NUM         TRGM_TRGOCFG_MOT_GPIO0
#define BOARD_RDC_TRG_ADC_NUM     TRGM_TRGOCFG_ADCX_PTRGI0A
#define BOARD_RDC_ADC_I_BASE      HPM_ADC0
#define BOARD_RDC_ADC_Q_BASE      HPM_ADC1
#define BOARD_RDC_ADC_I_CHN       (5U)
#define BOARD_RDC_ADC_Q_CHN       (6U)
#define BOARD_RDC_ADC_IRQn        IRQn_ADC0
#define BOARD_RDC_ADC_TRIG_FLAG   adc16_event_trig_complete
#define BOARD_RDC_ADC_TRG         ADC16_CONFIG_TRG0A

/*QEI*/
#define BOARD_BLDC_QEI_TRGM                      HPM_TRGM0
#define BOARD_BLDC_QEIV2_BASE                    HPM_QEI1
#define BOARD_BLDC_QEIV2_IRQ                     IRQn_QEI1
#define BOARD_BLDC_QEI_MOTOR_PHASE_COUNT_PER_REV (16U)
#define BOARD_BLDC_QEI_CLOCK_SOURCE              clock_mot0
#define BOARD_BLDC_QEI_FOC_PHASE_COUNT_PER_REV   (4000U)
#define BOARD_BLDC_QEI_ADC_MATRIX_ADC0           trgm_adc_matrix_output_to_qei1_adc0
#define BOARD_BLDC_QEI_ADC_MATRIX_ADC1           trgm_adc_matrix_output_to_qei1_adc1

/*Timer define*/
#define BOARD_BLDC_TMR_1MS    HPM_GPTMR2
#define BOARD_BLDC_TMR_CH     0
#define BOARD_BLDC_TMR_CMP    0
#define BOARD_BLDC_TMR_IRQ    IRQn_GPTMR2
#define BOARD_BLDC_TMR_RELOAD (100000U)

/*adc*/
#define BOARD_BLDC_ADC_MODULE    (ADCX_MODULE_ADC16)
#define BOARD_BLDC_ADC_U_BASE    HPM_ADC0
#define BOARD_BLDC_ADC_V_BASE    HPM_ADC1
#define BOARD_BLDC_ADC_W_BASE    HPM_ADC1
#define BOARD_BLDC_ADC_TRIG_FLAG adc16_event_trig_complete

#define BOARD_BLDC_ADC_CH_U                   (5U)
#define BOARD_BLDC_ADC_CH_V                   (6U)
#define BOARD_BLDC_ADC_CH_W                   (4U)
#define BOARD_BLDC_ADC_IRQn                   IRQn_ADC0
#define BOARD_BLDC_ADC_PMT_DMA_SIZE_IN_4BYTES (ADC_SOC_PMT_MAX_DMA_BUFF_LEN_IN_4BYTES)
#define BOARD_BLDC_ADC_TRG                    ADC16_CONFIG_TRG0A
#define BOARD_BLDC_ADC_PREEMPT_TRIG_LEN       (1U)
#define BOARD_BLDC_PWM_TRIG_CMP_INDEX         (8U)
#define BOARD_BLDC_TRIGMUX_IN_NUM             HPM_TRGM0_INPUT_SRC_PWM0_CH8REF
#define BOARD_BLDC_TRG_NUM                    TRGM_TRGOCFG_ADCX_PTRGI0A

#define BOARD_PLB_COUNTER              HPM_PLB
#define BOARD_PLB_PWM_BASE             HPM_PWM0
#define BOARD_PLB_PWM_CLOCK_NAME       clock_mot0
#define BOARD_PLB_TRGM                 HPM_TRGM0
#define BOARD_PLB_PWM_TRG              (HPM_TRGM0_INPUT_SRC_PWM0_CH8REF)
#define BOARD_PLB_IN_PWM_TRG_NUM       (TRGM_TRGOCFG_PLB_IN_00)
#define BOARD_PLB_IN_PWM_PULSE_TRG_NUM (TRGM_TRGOCFG_PLB_IN_02)
#define BOARD_PLB_OUT_TRG              (HPM_TRGM0_INPUT_SRC_PLB_OUT00)
#define BOARD_PLB_IO_TRG_NUM           (TRGM_TRGOCFG_MOT_GPIO2)
#define BOARD_PLB_IO_TRG_SHIFT         (2)
#define BOARD_PLB_PWM_CMP              (8U)
#define BOARD_PLB_PWM_CHN              (8U)
#define BOARD_PLB_CHN                  plb_chn0

/* QEO */
#define BOARD_QEO          HPM_QEO0
#define BOARD_QEO_TRGM_POS trgm_pos_matrix_output_to_qeo0

/* moto */
#define BOARD_MOTOR_CLK_NAME clock_mot0

/* SEI */
#define BOARD_SEI      HPM_SEI
#define BOARD_SEI_CTRL SEI_CTRL_1
#define BOARD_SEI_IRQn IRQn_SEI1

/* USB */
#define BOARD_USB HPM_USB0

/* OPAMP */
#define BOARD_APP_OPAMP HPM_OPAMP0

#ifndef BOARD_SHOW_CLOCK
#define BOARD_SHOW_CLOCK 1
#endif
#ifndef BOARD_SHOW_BANNER
#define BOARD_SHOW_BANNER 0
#endif

/* FreeRTOS Definitions */
#define BOARD_FREERTOS_TIMER          HPM_GPTMR2
#define BOARD_FREERTOS_TIMER_CHANNEL  1
#define BOARD_FREERTOS_TIMER_IRQ      IRQn_GPTMR2
#define BOARD_FREERTOS_TIMER_CLK_NAME clock_gptmr2

/* Threadx Definitions */
#define BOARD_THREADX_TIMER           HPM_GPTMR2
#define BOARD_THREADX_TIMER_CHANNEL   1
#define BOARD_THREADX_TIMER_IRQ       IRQn_GPTMR2
#define BOARD_THREADX_TIMER_CLK_NAME  clock_gptmr2



#define ATTR_DLM ATTR_PLACE_AT(".dataindlm")

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

typedef void (*board_timer_cb)(void);

void board_init(void);
void board_init_console(void);
void board_init_gpio_pins(void);
void board_init_led_pins(void);
void board_init_usb_pins(void);
void board_led_write(uint8_t state);
void board_led_toggle(void);
void board_init_uart(UART_Type *ptr);
uint32_t board_init_spi_clock(SPI_Type *ptr);
void board_init_spi_pins(SPI_Type *ptr);
void board_usb_vbus_ctrl(uint8_t usb_index, uint8_t level);
uint32_t board_init_adc16_clock(ADC16_Type *ptr, bool clk_src_ahb);
void board_init_adc16_pins(void);
uint32_t board_init_dac_clock(DAC_Type *ptr, bool clk_src_ahb);
void board_init_can(MCAN_Type *ptr);
uint32_t board_init_can_clock(MCAN_Type *ptr);
void board_init_rgb_pwm_pins(void);
void board_disable_output_rgb_led(uint8_t color);
void board_enable_output_rgb_led(uint8_t color);
void board_init_dac_pins(DAC_Type *ptr);
void board_write_spi_cs(uint32_t pin, uint8_t state);
void board_init_spi_pins_with_gpio_as_cs(SPI_Type *ptr);

void board_init_usb_dp_dm_pins(void);
void board_init_clock(void);
void board_delay_us(uint32_t us);
void board_delay_ms(uint32_t ms);
void board_timer_create(uint32_t ms, board_timer_cb cb);
void board_ungate_mchtmr_at_lp_mode(void);

uint8_t board_get_led_gpio_off_level(void);
uint8_t board_get_led_pwm_off_level(void);

void board_init_pmp(void);
void board_init_dcdc_mode(void);
void board_sw_reset(void);

uint32_t board_init_uart_clock(UART_Type *ptr);
void board_init_sei_pins(SEI_Type *ptr, uint8_t sei_ctrl_idx);

void board_init_i2c(I2C_Type *ptr);

void board_init_adc_qeiv2_pins(void);

void board_lin_transceiver_control(bool enable);
#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* _HPM_BOARD_H */
