/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "app_debug.h"

#include "hpm_soc.h"
#include "hpm_clock_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_gptmr_drv.h"
#include "hpm_dmamux_drv.h"
#include "hpm_dmav2_drv.h"
#include "hpm_trgm_drv.h"
#include "hpm_adc16_drv.h"

#include "mg_matrix_config.h"
#include "mg_hall.h"
#include "board.h"
#include "math.h"
#include "mg_filter.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "mg_hall"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif
#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef BIT
#define BIT(n) (1UL << (n))
#endif


/* scan mode select */
#define SCAN_MODE_SELF_CYCLE                (0)
#define SCAN_MODE_TIMER_TRG                 (1)

#define USE_GPTMR_TRIG_SCAN                 SCAN_MODE_TIMER_TRG
#define ENABLE_ADC_SEQUENCE                 (1)

#define KEY_SCAN_FREQ                       (8000U) // 8khz
#define KEY_SCAN_HDMA_CHANNEL               (2)

#define KEY_SCAN_GPTMR_BASE                 (HPM_GPTMR1)
#define KEY_SCAN_GPTMR_CLOCK_NAME           (clock_gptmr1)
#define KEY_SCAN_GPTMR_TRGM_ADC_CH          (2)
#define KEY_SCAN_GPTMR_TRGM_DMA_CH          (3)

#define KEY_TRIG_SCAN_GPTMR_BASE            (HPM_GPTMR0)
#define KEY_TRIG_SCAN_GPTMR_CLOCK_NAME      (clock_gptmr0)
#define KEY_TRIG_SCAN_GPTMR_CHANNEL         (1)
#define KEY_TRIG_SCAN_GPTMR_IRQ             IRQn_GPTMR0
#define KEY_TRIG_SCAN_GPTMR_CYCLE           (125)
#define KEY_TRIG_SCAN_GPTMR_TICK_UNIT       (1000 * 1000)

#define KEY_SCAN_TRGMMUX_INPUT_SRC (HPM_TRGM0_INPUT_SRC_GPTMR1_OUT2) // HPM_TRGM0_INPUT_SRC_GPTMR0_OUT2
#define KEY_SCAN_DMA_SRC (HPM_DMA_SRC_GPTMR1_3)

#define KEY_SCAN_ADC16_COUNT (2)
#define KEY_SCAN_ADC16_PMT_TRIG_CH ADC16_CONFIG_TRG0A
#define KEY_SCAN_ADC16_PMT_IRQ_EVENT adc16_event_trig_complete

#define KEY_SCAN_SWITCH_BIT_COUNT (3)//需要多少条切换地址线

#define KEY_SCAN_GPTMR_FREQ (KEY_SCAN_FREQ * KEY_SCAN_SWITCH_CH_COUNT * KEY_SCAN_ADC_BUFF_COUNT)

static const uint8_t adc_channel[KEY_SCAN_ADC16_COUNT][KEY_SCAN_ADC_ONCE_COUNT] =
{
   // {15, 14, 12,  8,  0, 13},  /*ADC0.15, ADC0.14, ADC0.12 ADC0.08 ADC0.00 ADC0.13 */
   // { 9, 10, 11,  1,  2, 3 },  /*ADC1.09, ADC1.10, ADC1.11 ADC1.01 ADC1.02 ADC1.03 */

    {13, 0, 14, 8, 15, 12, }, 
    {3,  2, 10, 1, 9,  11, }, 
};
        
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
        
        
        
        

ATTR_PLACE_AT_WITH_ALIGNMENT(".ahb_sram", ADC_SOC_DMA_ADDR_ALIGNMENT)
static uint32_t gpio_porta_set[KEY_SCAN_SWITCH_CH_COUNT] = {0};

#ifndef ENABLE_ADC_SEQUENCE
ATTR_PLACE_AT_WITH_ALIGNMENT(".ahb_sram", ADC_SOC_DMA_ADDR_ALIGNMENT)
static uint32_t pmt_buff[KEY_SCAN_ADC16_COUNT][KEY_SCAN_ADC_ONCE_COUNT];

ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(ADC_SOC_DMA_ADDR_ALIGNMENT)
static uint32_t adc_buff[KEY_SCAN_ADC_BUFF_COUNT][(KEY_SCAN_ADC16_COUNT * KEY_SCAN_ADC_ONCE_COUNT * KEY_SCAN_SWITCH_CH_COUNT) + 1]; // 15*6=90
#else
ATTR_PLACE_AT_WITH_ALIGNMENT(".ahb_sram", ADC_SOC_DMA_ADDR_ALIGNMENT)
static uint32_t seq_buff[KEY_SCAN_ADC16_COUNT][KEY_SCAN_ADC_ONCE_COUNT * KEY_SCAN_ADC_ONCE_TIMES];

ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(ADC_SOC_DMA_ADDR_ALIGNMENT)
static uint32_t adc_buff[KEY_SCAN_ADC_BUFF_COUNT][(KEY_SCAN_ADC16_COUNT * KEY_SCAN_ADC_ONCE_COUNT * KEY_SCAN_ADC_ONCE_TIMES * KEY_SCAN_SWITCH_CH_COUNT) + 1]; // 15*6=90

//static uint32_t adc_buff1[KEY_SCAN_ADC_BUFF_COUNT][(KEY_SCAN_ADC16_COUNT * KEY_SCAN_ADC_ONCE_COUNT * KEY_SCAN_ADC_ONCE_TIMES * KEY_SCAN_SWITCH_CH_COUNT) + 1]; // 15*6=90
//uint32_t adc_buff2[8];

#endif
//ATTR_PLACE_AT_WITH_ALIGNMENT(".ahb_sram", 8)

ATTR_PLACE_AT_WITH_ALIGNMENT(".ahb_sram", 8)
static dma_linked_descriptor_t descriptors[KEY_SCAN_ADC_BUFF_COUNT * KEY_SCAN_SWITCH_CH_COUNT * (1 + 1)];

static dma_channel_config_t first_dma_ch_config;
static p_hall_isr hall_isr  = NULL;

static int hall_driver_key_scan_start(void);
void hall_driver_auto_start_enable(bool enable);
static uint16_t hall_driver_normal_avg(const uint32_t *buffer)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < KEY_SCAN_ADC_ONCE_TIMES; i++)
    {
        sum += (buffer[i] & 0xffff);
    }
    return sum / KEY_SCAN_ADC_ONCE_TIMES;
}

static uint16_t hall_driver_vpp_avg(const uint32_t *buffer)
{
    uint16_t min = 0xffff;
    uint16_t max = 0x0000;
    for (uint8_t i = 0; i < KEY_SCAN_ADC_ONCE_TIMES; i++)
    {
        min = MIN(min, buffer[i] & 0xffff);
        max = MAX(max, buffer[i] & 0xffff);
    }
    uint32_t vpp = max + min;
    return (vpp / 2);
}

#if 0
static uint16_t hall_driver_weight_avg(uint32_t index)
{

#if (KEY_SCAN_ADC_ONCE_TIMES*KEY_SCAN_ADC_BUFF_COUNT == 5)
    uint8_t weight[KEY_SCAN_ADC_ONCE_TIMES*KEY_SCAN_ADC_ONCE_TIMES] = {28, 50, 100, 50, 28}; /* sum 256 */
#elif (KEY_SCAN_ADC_ONCE_TIMES*KEY_SCAN_ADC_BUFF_COUNT == 4)
    uint8_t weight[KEY_SCAN_ADC_ONCE_TIMES*KEY_SCAN_ADC_ONCE_TIMES] = {48, 80, 80, 48}; /* sum 256 */
#elif (KEY_SCAN_ADC_ONCE_TIMES*KEY_SCAN_ADC_BUFF_COUNT == 3)
    uint8_t weight[KEY_SCAN_ADC_ONCE_TIMES*KEY_SCAN_ADC_ONCE_TIMES] = {48, 160, 48}; /* sum 256 */
#else
#error "KEY_SCAN_ADC_ONCE_TIMES can only be 3 or 4 or 5"
#endif
    uint32_t sum = 0, offset=0;
    uint32_t array[KEY_SCAN_ADC_BUFF_COUNT*KEY_SCAN_ADC_ONCE_TIMES];

    for (uint32_t j = 0; j < KEY_SCAN_ADC_BUFF_COUNT; j++)
    {
        for(uint32_t i = 0; i < KEY_SCAN_ADC_ONCE_TIMES; i++)
        {
            array[offset++] = adc_buff[j][index+i] & 0xffff;
        }
    }

    for (uint16_t i = 0; i < KEY_SCAN_ADC_BUFF_COUNT*KEY_SCAN_ADC_ONCE_TIMES; i++)
    {
        for (uint16_t j = 0; j < KEY_SCAN_ADC_BUFF_COUNT*KEY_SCAN_ADC_ONCE_TIMES - i - 1; j++)
        {
            if (array[j] > array[j + 1])
            {
                uint32_t tmp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = tmp;
            }
        }
    }

    for (uint32_t i = 0; i < KEY_SCAN_ADC_BUFF_COUNT*KEY_SCAN_ADC_ONCE_TIMES; i++)
    {
        sum += array[i] * weight[i];
    }

    return sum >> 8;
}


static uint16_t hall_driver_avg(uint32_t index)
{
    (void)hall_driver_normal_avg;
    (void)hall_driver_vpp_avg;
#if (KEY_SCAN_ADC_ONCE_TIMES > 2)
    //(void)hall_driver_weight_avg;
#endif

    /* return hall_driver_normal_avg(buffer); */
     //return hall_driver_vpp_avg(buffer); 
    //return hall_driver_weight_avg(index);
    return 0;
}
#endif
extern void linear_scan(uint8_t row, uint8_t col, uint16_t value);

static void isr_hdma(void)
{
    volatile hpm_stat_t stat;
    //uint32_t index;
    stat = dma_check_transfer_status(HPM_HDMA, KEY_SCAN_HDMA_CHANNEL);
    

    if ((stat & DMA_CHANNEL_STATUS_TC) || (stat & DMA_CHANNEL_STATUS_ONGOING))
    {
#ifndef ENABLE_ADC_SEQUENCE

        for (uint16_t idx = 0; idx < KEY_SCAN_ADC16_COUNT * KEY_SCAN_ADC_ONCE_COUNT * KEY_SCAN_SWITCH_CH_COUNT; idx++)
        {
            uint8_t row = idx % (KEY_SCAN_ADC16_COUNT * KEY_SCAN_ADC_ONCE_COUNT);
            uint8_t col = idx / (KEY_SCAN_ADC16_COUNT * KEY_SCAN_ADC_ONCE_COUNT);
            // do scan here, param row, col, adc_buff[0][idx] & 0xffff
            //linear_scan(row, col, adc_buff[0][idx] & 0xffff);
            linear_scan(row, col, adc_buff[0][1]);
            
        }
#else
        #if 1
        #if 0
        uint16_t buf_idx=0;
        for (buf_idx=0; buf_idx < KEY_SCAN_ADC_BUFF_COUNT; buf_idx++)
        {
            for ( index = 0; index < ((KEY_SCAN_ADC16_COUNT * KEY_SCAN_ADC_ONCE_COUNT * KEY_SCAN_ADC_ONCE_TIMES * KEY_SCAN_SWITCH_CH_COUNT)); index++)
            {
               //adc_buff[buf_idx][index] &=0xffff;
               adc_buff1[buf_idx][index] = adc_buff[buf_idx][index]&0xffff;

                #if 0
                if (index % KEY_SCAN_ADC_ONCE_TIMES == 0)
                {
                    uint8_t ki = index / KEY_SCAN_ADC_ONCE_TIMES;
                    tdv[0].buf[ki] = (float)(adc_buff1[buf_idx][index+0] + adc_buff1[buf_idx][index+1]) / 2.0f;
                    uint16_t data = (uint16_t)lpf_dynamic_update(ki, 
                                                (float)tdv[0].buf[ki], 
                                                (float)tdv[1].buf[ki]);
                    tdv_record(1, ki, data);
                }
                #endif                
            }
        }
        hall_isr(adc_buff1[0]);
        #else
        hall_isr(adc_buff[0]);

        #if (USE_GPTMR_TRIG_SCAN == SCAN_MODE_TIMER_TRG)
        gptmr_stop_counter(KEY_SCAN_GPTMR_BASE, KEY_SCAN_GPTMR_TRGM_ADC_CH);
        gptmr_stop_counter(KEY_SCAN_GPTMR_BASE, KEY_SCAN_GPTMR_TRGM_DMA_CH);
        #endif

        #endif
        #elif 1
        uint8_t ki = 0;
        if (tdv[0].isClr || tdv[1].isClr)
        {
            tdv[0].isClr = tdv[1].isClr = false;
            for (uint16_t i = 0; i < KEY_SCAN_ADC16_COUNT * KEY_SCAN_ADC_ONCE_COUNT * KEY_SCAN_ADC_ONCE_TIMES * KEY_SCAN_SWITCH_CH_COUNT; i+=KEY_SCAN_ADC_ONCE_TIMES )
            {
                ki = i / KEY_SCAN_ADC_ONCE_TIMES;
                tdv[0].max[ki] = tdv[1].max[ki] = tdv[2].max[ki] = tdv[3].max[ki] = 0;
                tdv[1].min[ki] = tdv[2].min[ki] = tdv[3].min[ki] = tdv[0].min[ki] = 0xffff;
                tdv_record(0, ki, hall_driver_normal_avg(&adc_buff[0][i]));
                tdv_record(1, ki, lpf_dynamic_adj(tdv[0].buf[ki], tdv[1].buf[ki]));
                tdv_record(2, ki, lpf_dynamic_update(ki, tdv[0].buf[ki], tdv[2].buf[ki]));
            }
        }
        else
        {
            for (uint16_t i = 0; i < KEY_SCAN_ADC16_COUNT * KEY_SCAN_ADC_ONCE_COUNT * KEY_SCAN_ADC_ONCE_TIMES * KEY_SCAN_SWITCH_CH_COUNT; i+=KEY_SCAN_ADC_ONCE_TIMES )
            {
                ki = i / KEY_SCAN_ADC_ONCE_TIMES;
                tdv_record(0, ki, hall_driver_normal_avg(&adc_buff[0][i]));
                tdv_record(1, ki, lpf_dynamic_adj(tdv[0].buf[ki], tdv[1].buf[ki]));
                tdv_record(2, ki, lpf_dynamic_update(ki, tdv[0].buf[ki], tdv[2].buf[ki]));
            }
        }
        uint8_t outKi = 11;
        printf("%d, %d, %d, %d, %d\n",  adc_buff[0][outKi*2]&0xFFFF, adc_buff[0][outKi*2+1]&0xFFFF,
                                        tdv[0].buf[outKi], tdv[1].buf[outKi], tdv[2].buf[outKi]);
        
        #endif
#endif
    }
    else if (stat & DMA_CHANNEL_STATUS_ERROR)
    {
        DBG_PRINTF("dma error!\r\n");
        // hall_driver_key_scan_start();
    }
    else if (stat & DMA_CHANNEL_STATUS_ABORT)
    {
        DBG_PRINTF("dma abort!\r\n");
        // hall_driver_key_scan_start();
    }
}

// SDK_DECLARE_EXT_ISR_M(IRQn_HDMA, isr_hdma)

static uint32_t hall_driver_adc_init_clock(ADC16_Type *ptr, bool clk_src_ahb)
{
    uint32_t freq = 0;

    if (ptr == HPM_ADC0)
    {
        if (clk_src_ahb)
        {
            clock_set_adc_source(clock_adc0, clk_adc_src_ahb0);
        }
        else
        {
            clock_set_adc_source(clock_adc0, clk_adc_src_ana0);
            clock_set_source_divider(clock_ana0, clk_src_pll0_clk2, 2U);
        }

        freq = clock_get_frequency(clock_adc0);
    }
    else if (ptr == HPM_ADC1)
    {
        if (clk_src_ahb)
        {
            clock_set_adc_source(clock_adc1, clk_adc_src_ahb0);
        }
        else
        {
            clock_set_adc_source(clock_adc1, clk_adc_src_ana1);
            clock_set_source_divider(clock_ana1, clk_src_pll0_clk2, 2U);
        }

        freq = clock_get_frequency(clock_adc1);
    }

    return freq;
}

static int hall_driver_adc_init_common_config(ADC16_Type *ptr)
{
    adc16_config_t cfg;

    /* initialize an ADC instance */
    adc16_get_default_config(&cfg);

    cfg.res = adc16_res_16_bits;
    cfg.conv_mode = adc16_conv_mode_sequence;
    cfg.adc_clk_div = adc16_clock_divider_4;
    cfg.sel_sync_ahb = (clk_adc_src_ahb0 == clock_get_source(ptr == HPM_ADC0 ? clock_adc0 : clock_adc1)) ? true : false;
    cfg.adc_ahb_en = true;

    /* adc16 initialization */
    if (adc16_init(ptr, &cfg) != status_success)
    {
        DBG_PRINTF("adc initialization failed!\n");
        return -1;
    }
    return 0;
}

const uint8_t sample_cycle = 2;
const uint8_t sample_cycle_shaft = 3;
float sampleUs = 0;
uint16_t cycle = 0;
uint16_t cmpValue = 0;


static void hall_driver_init_gptmr_trigger_source(void)
{
    gptmr_channel_config_t config;
    uint32_t gptmr_freq;
    gptmr_channel_get_default_config(KEY_SCAN_GPTMR_BASE, &config);
    gptmr_freq = clock_get_frequency(KEY_SCAN_GPTMR_CLOCK_NAME);
    config.reload = gptmr_freq / KEY_SCAN_GPTMR_FREQ - 550;
    //config.reload = 2083; //20.83us
    cycle = config.reload;
    sampleUs  = ((((float)adc16_res_16_bits + ((float)sample_cycle * pow(2.0, (float)sample_cycle_shaft))) //single sample x clock
                * (float)KEY_SCAN_ADC_ONCE_TIMES * (float)KEY_SCAN_ADC_ONCE_COUNT //one group sample x clock
                / 40.0)); //convert to us units

    cmpValue  = sampleUs * (gptmr_freq / 1000000); //to tim cmp value

    DBG_PRINTF("gptmr freq %dHz, scan freq %dHz, reload %d, sampleUs:%f cmpValue:%d\r\n", gptmr_freq, KEY_SCAN_GPTMR_FREQ, config.reload, sampleUs, cmpValue);
    
    config.cmp[0] = 10;

    config.cmp_initial_polarity_high = false;
    config.dma_request_event = gptmr_dma_request_disabled;

    gptmr_channel_config(KEY_SCAN_GPTMR_BASE, KEY_SCAN_GPTMR_TRGM_ADC_CH, &config, false);
    gptmr_channel_reset_count(KEY_SCAN_GPTMR_BASE, KEY_SCAN_GPTMR_TRGM_ADC_CH);
    
	#if 1//def MOJO68RT
    // adc :6通道x2次=12次，周期数就是12*24=288，也就是576/50M=5.76us，
    /* mux切换频率是6通道x8k=48k，每个通道的时间就是1/48k=20.8us */
    //模拟开关可用的最大稳定时间20.8us-11.52us=9.28us
    config.cmp[0] = cmpValue; //剩余时间：模拟开关稳定时间
	#else
    /*adc时钟50M，采样3周期，转换21周期，3通道x4次=12次，周期数就是24*12=288，也就是288/50M=5.76us，
    mux切换频率是15通道x8k=120k，每个通道的时间就是1/120k=8.33us
    280的时间延后就是280/100m=2.8us
    计算就是8.33-5.76=2.57，微调了一下2.8us，这个2.8us 是预留给mux切换后的信号建立时间用的
    */
    /* min is 220 */
    /* KEY_SCAN_ADC_ONCE_TIMES = 3, sample_cycle = 3, max is 420 */
    /* KEY_SCAN_ADC_ONCE_TIMES = 4, sample_cycle = 3, max is 280 */
	config.cmp[0] = config.reload - 280;
	#endif
    config.cmp_initial_polarity_high = false;
    config.dma_request_event = gptmr_dma_request_on_cmp0;

    gptmr_channel_config(KEY_SCAN_GPTMR_BASE, KEY_SCAN_GPTMR_TRGM_DMA_CH, &config, false);
    gptmr_channel_enable_dma_request(KEY_SCAN_GPTMR_BASE, KEY_SCAN_GPTMR_TRGM_DMA_CH, true);
    gptmr_channel_reset_count(KEY_SCAN_GPTMR_BASE, KEY_SCAN_GPTMR_TRGM_DMA_CH);
}

static void gptmr_isr(void)
{
    if (gptmr_check_status(KEY_TRIG_SCAN_GPTMR_BASE, GPTMR_CH_RLD_STAT_MASK(KEY_TRIG_SCAN_GPTMR_CHANNEL))) 
    {
        gptmr_clear_status(KEY_TRIG_SCAN_GPTMR_BASE, GPTMR_CH_RLD_STAT_MASK(KEY_TRIG_SCAN_GPTMR_CHANNEL));
        hall_driver_auto_start_enable(true);
    }
}

static void hall_driver_trigger_scan_gptmr_init(uint32_t time_us)
{
    uint32_t gptmr_freq;
    gptmr_channel_config_t config;

    gptmr_channel_get_default_config(KEY_TRIG_SCAN_GPTMR_BASE, &config);

    gptmr_freq = clock_get_frequency(KEY_TRIG_SCAN_GPTMR_CLOCK_NAME);
    config.reload = gptmr_freq / KEY_TRIG_SCAN_GPTMR_TICK_UNIT * time_us;
    gptmr_channel_config(KEY_TRIG_SCAN_GPTMR_BASE, KEY_TRIG_SCAN_GPTMR_CHANNEL, &config, false);
    gptmr_enable_irq(KEY_TRIG_SCAN_GPTMR_BASE, GPTMR_CH_RLD_IRQ_MASK(KEY_TRIG_SCAN_GPTMR_CHANNEL));
    intc_m_enable_irq_with_priority(KEY_TRIG_SCAN_GPTMR_IRQ, 1);

    gptmr_start_counter(KEY_TRIG_SCAN_GPTMR_BASE, KEY_TRIG_SCAN_GPTMR_CHANNEL);
}
SDK_DECLARE_EXT_ISR_M(KEY_TRIG_SCAN_GPTMR_IRQ, gptmr_isr);

void hall_driver_auto_start_enable(bool enable)
{
    if (enable)
    {
        gptmr_channel_reset_count(KEY_SCAN_GPTMR_BASE, KEY_SCAN_GPTMR_TRGM_ADC_CH);
        gptmr_channel_reset_count(KEY_SCAN_GPTMR_BASE, KEY_SCAN_GPTMR_TRGM_DMA_CH);
        gptmr_start_counter(KEY_SCAN_GPTMR_BASE, KEY_SCAN_GPTMR_TRGM_ADC_CH);
        gptmr_start_counter(KEY_SCAN_GPTMR_BASE, KEY_SCAN_GPTMR_TRGM_DMA_CH);
        gptmr_trigger_channel_software_sync(KEY_SCAN_GPTMR_BASE, GPTMR_CH_GCR_SWSYNCT_MASK(KEY_SCAN_GPTMR_TRGM_ADC_CH) | GPTMR_CH_GCR_SWSYNCT_MASK(KEY_SCAN_GPTMR_TRGM_DMA_CH));
    }
    else
    {
        gptmr_stop_counter(KEY_SCAN_GPTMR_BASE, KEY_SCAN_GPTMR_TRGM_ADC_CH);
        gptmr_stop_counter(KEY_SCAN_GPTMR_BASE, KEY_SCAN_GPTMR_TRGM_DMA_CH);
    }
}

static void hall_driver_init_trigger_mux(void)
{
    /* Trigger mux initialization */
    trgm_output_t trgm_output_cfg;
    HPM_IOC->PAD[IOC_PAD_PA16].FUNC_CTL = IOC_PA16_FUNC_CTL_TRGM0_P_04;
    
    HPM_IOC->PAD[IOC_PAD_PA17].FUNC_CTL = IOC_PA17_FUNC_CTL_TRGM0_P_05;

    trgm_output_cfg.invert = false;
    trgm_output_cfg.type = trgm_output_same_as_input;

    trgm_output_cfg.input = KEY_SCAN_TRGMMUX_INPUT_SRC;
    // trigger adc sampling
#ifndef ENABLE_ADC_SEQUENCE
    trgm_output_config(HPM_TRGM0, TRGM_TRGOCFG_ADCX_PTRGI0A, &trgm_output_cfg);
#else
    trgm_output_config(HPM_TRGM0, TRGM_TRGOCFG_ADC0_STRGI, &trgm_output_cfg);
    trgm_output_config(HPM_TRGM0, TRGM_TRGOCFG_ADC1_STRGI, &trgm_output_cfg);
    //trgm_output_cfg.input = KEY_SCAN_DMA_SRC;
    //trgm_output_cfg.type = trgm_output_pulse_at_input_both_edge;
    trgm_output_config(HPM_TRGM0, TRGM_TRGOCFG_TRGM_P_04, &trgm_output_cfg);
    trgm_output_cfg.input = HPM_TRGM0_INPUT_SRC_GPTMR1_OUT3;
    trgm_output_config(HPM_TRGM0, TRGM_TRGOCFG_TRGM_P_05, &trgm_output_cfg);
    trgm_enable_io_output(HPM_TRGM0, 1 << 4);
    trgm_enable_io_output(HPM_TRGM0, 1 << 5);
#endif
}

#ifndef ENABLE_ADC_SEQUENCE
static void hall_driver_adc_init_preemption_config(ADC16_Type *ptr)
{
    uint8_t i;
    adc16_channel_config_t ch_cfg;
    adc16_pmt_config_t pmt_cfg;

    /* get a default channel config */
    adc16_get_channel_default_config(&ch_cfg);

    /* initialize an ADC channel */
    ch_cfg.sample_cycle = 3;
    for (i = 0; i < KEY_SCAN_ADC_ONCE_COUNT; i++)
    {
        if (ptr == HPM_ADC0)
        {
            ch_cfg.ch = adc_channel[0][i];
            pmt_cfg.adc_ch[i] = adc_channel[0][i];
        }
        else if (ptr == HPM_ADC1)
        {
            ch_cfg.ch = adc_channel[1][i];
            pmt_cfg.adc_ch[i] = adc_channel[1][i];
        }
        adc16_init_channel(ptr, &ch_cfg);
        pmt_cfg.inten[i] = false;
    }

    /* Trigger target initialization */
    pmt_cfg.trig_len = KEY_SCAN_ADC_ONCE_COUNT;
    pmt_cfg.trig_ch = KEY_SCAN_ADC16_PMT_TRIG_CH;

    adc16_set_pmt_config(ptr, &pmt_cfg);
    adc16_set_pmt_queue_enable(ptr, KEY_SCAN_ADC16_PMT_TRIG_CH, true);

    /* Set DMA start address for preemption mode */
    adc16_init_pmt_dma(ptr, core_local_mem_to_sys_address(HPM_CORE0, (ptr == HPM_ADC0) ? (uint32_t)&pmt_buff[0][0] : (uint32_t)&pmt_buff[1][0]));
}

#else
// sequence
static void hall_driver_adc_init_sequence_config(ADC16_Type *ptr)
{
    uint8_t i, j, index;
    adc16_seq_config_t seq_cfg;
    adc16_dma_config_t dma_cfg;
    adc16_channel_config_t ch_cfg;

    /* get a default channel config */
    adc16_get_channel_default_config(&ch_cfg);

    /* initialize an ADC channel */
    ch_cfg.sample_cycle = sample_cycle;
    ch_cfg.sample_cycle_shift = sample_cycle_shaft;

    for (i = 0; i < KEY_SCAN_ADC_ONCE_COUNT; i++)
    {
        if (ptr == HPM_ADC0)
        {
            ch_cfg.ch = adc_channel[0][i];
        }
        else if (ptr == HPM_ADC1)
        {
            ch_cfg.ch = adc_channel[1][i];
        }
        if (adc16_init_channel(ptr, &ch_cfg) != status_success)
        {
            DBG_PRINTF("adc initialization channel failed!\n");
            return;
        }
    }

    /* Set a sequence config */
    seq_cfg.seq_len = KEY_SCAN_ADC_ONCE_COUNT * KEY_SCAN_ADC_ONCE_TIMES;
    seq_cfg.restart_en = false;
    seq_cfg.cont_en = true;
    seq_cfg.hw_trig_en = true;
    seq_cfg.sw_trig_en = false;

    index = 0;
    for (i = 0; i < KEY_SCAN_ADC_ONCE_COUNT; i++)
    {
        for (j = 0; j < KEY_SCAN_ADC_ONCE_TIMES; j++)
        {
            seq_cfg.queue[index].seq_int_en = false;
            if (ptr == HPM_ADC0)
            {
                seq_cfg.queue[index].ch = adc_channel[0][i];
            }
            else if (ptr == HPM_ADC1)
            {
                seq_cfg.queue[index].ch = adc_channel[1][i];
            }
            index++;
        }
    }

    /* Enable the single complete interrupt for the last conversion */
    // seq_cfg.queue[seq_cfg.seq_len - 1].seq_int_en = true;

    /* Initialize a sequence */
    if (adc16_set_seq_config(ptr, &seq_cfg) != status_success)
    {
        DBG_PRINTF("adc set seq config failed!\n");
        return;
    }

    /* Set a DMA config */
    dma_cfg.start_addr = (uint32_t *)core_local_mem_to_sys_address(HPM_CORE0, (ptr == HPM_ADC0) ? (uint32_t)&seq_buff[0][0] : (uint32_t)&seq_buff[1][0]);
    dma_cfg.buff_len_in_4bytes = KEY_SCAN_ADC_ONCE_COUNT * KEY_SCAN_ADC_ONCE_TIMES;
    dma_cfg.stop_en = false;
    dma_cfg.stop_pos = 0;

    /* Initialize DMA for the sequence mode */
    adc16_init_seq_dma(ptr, &dma_cfg);

    /* Enable sequence complete interrupt */
    // adc16_enable_interrupts(BOARD_APP_ADC16_BASE, APP_ADC16_SEQ_IRQ_EVENT);
}
#endif

static void hall_driver_key_scan_dma_chain_config(void)
{
    uint8_t buf_index, switch_data, des_index;
    dma_channel_config_t dma_ch_config;

    dma_default_channel_config(HPM_HDMA, &dma_ch_config);
    des_index = 0;
    
    for (buf_index = 0; buf_index < KEY_SCAN_ADC_BUFF_COUNT; buf_index++)
    {
        // swtich_data, bit->A0, A1, A2, A3
        
        for (switch_data = 0; switch_data < KEY_SCAN_SWITCH_CH_COUNT; switch_data++)
        {
            dma_ch_config.size_in_byte = 4;
            if (switch_data == (KEY_SCAN_SWITCH_CH_COUNT - 1))
            {
                dma_ch_config.src_addr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)&gpio_porta_set[0]);
            }
            else
            {
                dma_ch_config.src_addr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)&gpio_porta_set[switch_data + 1]);
            }
            dma_ch_config.dst_addr = (uint32_t)&HPM_GPIO0->DO[GPIO_DO_GPIOA].VALUE;
            dma_ch_config.src_width = DMA_TRANSFER_WIDTH_WORD;
            dma_ch_config.dst_width = DMA_TRANSFER_WIDTH_WORD;
            dma_ch_config.src_burst_size = DMA_NUM_TRANSFER_PER_BURST_1T;
            dma_ch_config.src_mode = DMA_HANDSHAKE_MODE_HANDSHAKE;
            dma_ch_config.dst_mode = DMA_HANDSHAKE_MODE_NORMAL;
            dma_ch_config.src_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
            dma_ch_config.dst_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
            dma_ch_config.interrupt_mask = DMA_INTERRUPT_MASK_ALL;
            dma_ch_config.handshake_opt = DMA_HANDSHAKE_OPT_ALL_TRANSIZE;
            dma_ch_config.priority = DMA_CHANNEL_PRIORITY_HIGH;
            dma_ch_config.linked_ptr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)&descriptors[des_index + 1]);
            dma_config_linked_descriptor(HPM_HDMA, &descriptors[des_index], KEY_SCAN_HDMA_CHANNEL, &dma_ch_config);
            
            if (des_index == 0)
            {
                memcpy(&first_dma_ch_config, &dma_ch_config, sizeof(dma_channel_config_t));
            }
            des_index++;

                // trgm copy adc buffer
#ifndef ENABLE_ADC_SEQUENCE
            dma_ch_config.size_in_byte = (KEY_SCAN_ADC16_COUNT * KEY_SCAN_ADC_ONCE_COUNT) * sizeof(uint32_t);
            dma_ch_config.src_addr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)&pmt_buff);
            dma_ch_config.dst_addr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)&adc_buff[buf_index][(switch_data * KEY_SCAN_ADC16_COUNT * KEY_SCAN_ADC_ONCE_COUNT)]);
#else
            dma_ch_config.size_in_byte = (KEY_SCAN_ADC16_COUNT * KEY_SCAN_ADC_ONCE_COUNT * KEY_SCAN_ADC_ONCE_TIMES) * sizeof(uint32_t);
            dma_ch_config.src_addr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)&seq_buff);
            dma_ch_config.dst_addr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)&adc_buff[buf_index][(switch_data * KEY_SCAN_ADC16_COUNT * KEY_SCAN_ADC_ONCE_COUNT * KEY_SCAN_ADC_ONCE_TIMES)]);
#endif
            dma_ch_config.src_width = DMA_TRANSFER_WIDTH_WORD;
            dma_ch_config.dst_width = DMA_TRANSFER_WIDTH_WORD;
            dma_ch_config.src_burst_size = DMA_NUM_TRANSFER_PER_BURST_1T;
            dma_ch_config.src_mode = DMA_HANDSHAKE_MODE_NORMAL;
            dma_ch_config.dst_mode = DMA_HANDSHAKE_MODE_NORMAL;
            dma_ch_config.src_addr_ctrl = DMA_ADDRESS_CONTROL_INCREMENT;
            dma_ch_config.dst_addr_ctrl = DMA_ADDRESS_CONTROL_INCREMENT;
            dma_ch_config.interrupt_mask = DMA_INTERRUPT_MASK_ALL;
            dma_ch_config.handshake_opt = DMA_HANDSHAKE_OPT_ALL_TRANSIZE;
            dma_ch_config.priority = DMA_CHANNEL_PRIORITY_HIGH;
            //DBG_PRINTF("des_index=%d\n",des_index);
            //1,3,5,7,9,11
            #if (USE_GPTMR_TRIG_SCAN == SCAN_MODE_SELF_CYCLE)
            if (des_index == 9) //adjust
            {
                dma_ch_config.interrupt_mask = DMA_INTERRUPT_MASK_HALF_TC;
            }
            #endif
            
            if (des_index == (KEY_SCAN_ADC_BUFF_COUNT * KEY_SCAN_SWITCH_CH_COUNT * (1 + 1)) - 1)
            {
                #if (USE_GPTMR_TRIG_SCAN == SCAN_MODE_TIMER_TRG)
                dma_ch_config.interrupt_mask = DMA_INTERRUPT_MASK_HALF_TC;
                #endif
                dma_ch_config.linked_ptr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)&descriptors[0]); // NULL;
            }
            else
            {
                dma_ch_config.linked_ptr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)&descriptors[des_index + 1]);
            }
            dma_config_linked_descriptor(HPM_HDMA, &descriptors[des_index], KEY_SCAN_HDMA_CHANNEL, &dma_ch_config);
            des_index++;
        }
    }
}

static void hall_driver_update_global_gpio(void)
{
    
    
    uint32_t current_value = HPM_GPIO0->DO[GPIO_DO_GPIOA].VALUE;
    #if 0//def MOJO68RT
	uint32_t gpio_mask[KEY_SCAN_SWITCH_BIT_COUNT] = {1<<BOARD_CS_A0_PIN, 1<<BOARD_CS_A1_PIN, 1<<BOARD_CS_A2_PIN, 0x400U};
    uint8_t switch_data, i;
    for (switch_data = 0; switch_data < KEY_SCAN_SWITCH_CH_COUNT; switch_data++)
    {
        gpio_porta_set[switch_data] = current_value;
        for (i = 0; i < KEY_SCAN_SWITCH_BIT_COUNT; i++)
        {
            if (BIT(i) & switch_data)
            {
                gpio_porta_set[switch_data] |= gpio_mask[i];
            }
            else
            {
                gpio_porta_set[switch_data] &= ~gpio_mask[i];
            }
        }
    }
    #else
	uint32_t gpio_mask[KEY_SCAN_SWITCH_BIT_COUNT] = {1<<BOARD_CS_A0_PIN, 1<<BOARD_CS_A1_PIN, 1<<BOARD_CS_A2_PIN};
        
#if 0
        gpio_porta_set[0] = current_value;
        gpio_porta_set[0] &= ~gpio_mask[0];
        gpio_porta_set[0] &= ~gpio_mask[1];
        gpio_porta_set[0] &= ~gpio_mask[2];
    
        gpio_porta_set[1] = current_value;
        gpio_porta_set[1] |=  gpio_mask[0];
        gpio_porta_set[1] &= ~gpio_mask[1];
        gpio_porta_set[1] &= ~gpio_mask[2];
    
        gpio_porta_set[2] = current_value;
        gpio_porta_set[2] &= ~gpio_mask[0];
        gpio_porta_set[2] |=  gpio_mask[1];
        gpio_porta_set[2] &= ~gpio_mask[2];

        gpio_porta_set[3] = current_value;
        gpio_porta_set[3] |=  gpio_mask[0];
        gpio_porta_set[3] |=  gpio_mask[1];
        gpio_porta_set[3] &= ~gpio_mask[2];

    
        //gpio_porta_set[4] = current_value;
        //gpio_porta_set[4] &= ~gpio_mask[0];
        //gpio_porta_set[4] &= ~gpio_mask[1];
        //gpio_porta_set[4] |= gpio_mask[2];
    
        gpio_porta_set[4] = current_value;
        gpio_porta_set[4] |= gpio_mask[0];
        gpio_porta_set[4] &= ~gpio_mask[1];
        gpio_porta_set[4] |= gpio_mask[2];
    
        //gpio_porta_set[5] = current_value;
        //gpio_porta_set[5] &= ~gpio_mask[0];
        //gpio_porta_set[5] |= gpio_mask[1];
        //gpio_porta_set[5] |= gpio_mask[2];
    
        gpio_porta_set[5] = current_value;
        gpio_porta_set[5] |=  gpio_mask[0];
        gpio_porta_set[5] |=  gpio_mask[1];
        gpio_porta_set[5] |=  gpio_mask[2];
#else
        uint8_t cnt = 0;
        //02135746
        //1
        gpio_porta_set[cnt]    =  current_value;
        gpio_porta_set[cnt]   |=  gpio_mask[0];
        gpio_porta_set[cnt]   &= ~gpio_mask[1];
        gpio_porta_set[cnt++] &= ~gpio_mask[2];
        
        //5
        gpio_porta_set[cnt]    =  current_value;
        gpio_porta_set[cnt]   |=  gpio_mask[0];
        gpio_porta_set[cnt]   &= ~gpio_mask[1];
        gpio_porta_set[cnt++] |=  gpio_mask[2];
        
        //7
        gpio_porta_set[cnt]    = current_value;
        gpio_porta_set[cnt]   |=  gpio_mask[0];
        gpio_porta_set[cnt]   |=  gpio_mask[1];
        gpio_porta_set[cnt++] |=  gpio_mask[2];
        
        //0
        gpio_porta_set[cnt]    =  current_value;
        gpio_porta_set[cnt]   &= ~gpio_mask[0];
        gpio_porta_set[cnt]   &= ~gpio_mask[1];
        gpio_porta_set[cnt++] &= ~gpio_mask[2];
        
        //2
        gpio_porta_set[cnt]    =  current_value;
        gpio_porta_set[cnt]   &= ~gpio_mask[0];
        gpio_porta_set[cnt]   |=  gpio_mask[1];
        gpio_porta_set[cnt++] &= ~gpio_mask[2];

        //3
        gpio_porta_set[cnt]    =  current_value;
        gpio_porta_set[cnt]   |=  gpio_mask[0];
        gpio_porta_set[cnt]   |=  gpio_mask[1];
        gpio_porta_set[cnt++] &= ~gpio_mask[2];
        
        //4    
        //gpio_porta_set[cnt]    =  current_value;
        //gpio_porta_set[cnt]   &= ~gpio_mask[0];
        //gpio_porta_set[cnt]   &= ~gpio_mask[1];
        //gpio_porta_set[cnt++] |=  gpio_mask[2];
        //6
        //gpio_porta_set[cnt]    =  current_value;
        //gpio_porta_set[cnt]   &= ~gpio_mask[0];
        //gpio_porta_set[cnt]   |=  gpio_mask[1];
        //gpio_porta_set[cnt++] |=  gpio_mask[2];

#endif
    #endif
}

static int hall_driver_key_scan_dma_config(void)
{
    dma_default_channel_config(HPM_HDMA, &first_dma_ch_config);
    
    // A0
    gpio_write_pin(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_CS_A0_PIN, 0);
    // A1
    gpio_write_pin(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_CS_A1_PIN, 0);
    // A2
    gpio_write_pin(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_CS_A2_PIN, 0);
    
    hall_driver_update_global_gpio();

    hall_driver_key_scan_dma_chain_config();

    first_dma_ch_config.interrupt_mask = DMA_INTERRUPT_MASK_ALL;
    // dma_reset(HPM_HDMA);

    if (status_success != dma_setup_channel(HPM_HDMA, KEY_SCAN_HDMA_CHANNEL, &first_dma_ch_config, true))
    {
        DBG_PRINTF(" dma setup channel failed 0\n");
        return -1;
    }

    intc_m_enable_irq_with_priority(IRQn_HDMA, 1);
    dmamux_config(HPM_DMAMUX, DMA_SOC_CHN_TO_DMAMUX_CHN(HPM_HDMA, KEY_SCAN_HDMA_CHANNEL), KEY_SCAN_DMA_SRC, true);
    return 0;
}

static int hall_driver_key_scan_start(void)
{
    if (status_success != dma_setup_channel(HPM_HDMA, KEY_SCAN_HDMA_CHANNEL, &first_dma_ch_config, true))
    {
        DBG_PRINTF(" dma setup channel failed 0\n");
        return -1;
    }
    return 0;
}

static void hall_driver_pin_init(void)
{
    // HALL_EN.0
    HPM_IOC->PAD[IOC_PAD_PA31].FUNC_CTL = IOC_PA31_FUNC_CTL_GPIO_A_31;
    // EN.1
    HPM_IOC->PAD[IOC_PAD_PA30].FUNC_CTL = IOC_PA30_FUNC_CTL_GPIO_A_30;
    // EN.2
    HPM_IOC->PAD[IOC_PAD_PA29].FUNC_CTL = IOC_PA29_FUNC_CTL_GPIO_A_29;
    // EN.3
    HPM_IOC->PAD[IOC_PAD_PA28].FUNC_CTL = IOC_PA28_FUNC_CTL_GPIO_A_28;
    // EN.4
    HPM_IOC->PAD[IOC_PAD_PA27].FUNC_CTL = IOC_PA27_FUNC_CTL_GPIO_A_27;
    // EN.5
    HPM_IOC->PAD[IOC_PAD_PA26].FUNC_CTL = IOC_PA26_FUNC_CTL_GPIO_A_26;
    // EN.7
    HPM_IOC->PAD[IOC_PAD_PA23].FUNC_CTL = IOC_PA23_FUNC_CTL_GPIO_A_23;

    //// EN
    //HPM_IOC->PAD[IOC_PAD_PA31].FUNC_CTL = IOC_PA31_FUNC_CTL_GPIO_A_31;

    // A0
    HPM_IOC->PAD[IOC_PAD_PA19].FUNC_CTL = IOC_PA19_FUNC_CTL_GPIO_A_19;
    // A1
    HPM_IOC->PAD[IOC_PAD_PA20].FUNC_CTL = IOC_PA20_FUNC_CTL_GPIO_A_20;
    // A2
    HPM_IOC->PAD[IOC_PAD_PA21].FUNC_CTL = IOC_PA21_FUNC_CTL_GPIO_A_21;
    // A0
    //HPM_IOC->PAD[IOC_PAD_PA10].FUNC_CTL = IOC_PA10_FUNC_CTL_GPIO_A_10;

    // ADC0.0
    HPM_IOC->PAD[IOC_PAD_PB00].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    // ADC0.1
    HPM_IOC->PAD[IOC_PAD_PB01].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    // ADC0.2
    HPM_IOC->PAD[IOC_PAD_PB02].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    // ADC1.0
    HPM_IOC->PAD[IOC_PAD_PB03].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    // ADC1.1
    HPM_IOC->PAD[IOC_PAD_PB04].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    // ADC1.2
    HPM_IOC->PAD[IOC_PAD_PB05].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;

    
    HPM_IOC->PAD[IOC_PAD_PB06].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    // ADC0.1
    HPM_IOC->PAD[IOC_PAD_PB07].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    // ADC0.2
    HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    // ADC1.0
    HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    // ADC1.1
    HPM_IOC->PAD[IOC_PAD_PB10].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    // ADC1.2
    HPM_IOC->PAD[IOC_PAD_PB11].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;

    
}

static void hall_driver_gpio_init(void)
{
    
    gpio_set_pin_output(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_H_EN0_PIN);

    gpio_set_pin_output(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_H_EN1_PIN);

    gpio_set_pin_output(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_H_EN2_PIN);

    gpio_set_pin_output(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_H_EN3_PIN);

    gpio_set_pin_output(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_H_EN4_PIN);

    gpio_set_pin_output(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_H_EN5_PIN);

    gpio_set_pin_output(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_H_EN7_PIN);

    gpio_set_pin_output(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_CS_A0_PIN);

    gpio_set_pin_output(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_CS_A1_PIN);

    gpio_set_pin_output(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_CS_A2_PIN);
    
    gpio_write_pin(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_H_EN0_PIN, 0);

    gpio_write_pin(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_H_EN1_PIN, 0);

    gpio_write_pin(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_H_EN2_PIN, 0);

    gpio_write_pin(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_H_EN3_PIN, 0);

    gpio_write_pin(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_H_EN4_PIN, 0);

    gpio_write_pin(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_H_EN5_PIN, 0);

    gpio_write_pin(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_H_EN7_PIN, 0);

    gpio_write_pin(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_CS_A0_PIN, 0);

    gpio_write_pin(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_CS_A1_PIN, 0);

    gpio_write_pin(BOARD_HALL_GPIO_CTRL, GPIO_DI_GPIOA, BOARD_CS_A2_PIN, 0);
    
    BOARD_TEST_PIN0_OUTPUT();
    BOARD_TEST_PIN1_OUTPUT();
    BOARD_TEST_PIN2_OUTPUT();
}

void hall_driver_init(p_hall_isr pfn)
{
    uint32_t freq;
        
    hall_isr = pfn;

    hall_driver_pin_init();

    hall_driver_gpio_init();

    freq = hall_driver_adc_init_clock(HPM_ADC0, true);
    DBG_PRINTF("adc1 clock %dHz\r\n", freq);
    freq = hall_driver_adc_init_clock(HPM_ADC1, true);
    DBG_PRINTF("adc2 clock %dHz\r\n", freq);

    hall_driver_adc_init_common_config(HPM_ADC0);
    hall_driver_adc_init_common_config(HPM_ADC1);

#ifndef ENABLE_ADC_SEQUENCE
    hall_driver_adc_init_preemption_config(HPM_ADC0);
    hall_driver_adc_init_preemption_config(HPM_ADC1);
#else
    hall_driver_adc_init_sequence_config(HPM_ADC0);
    hall_driver_adc_init_sequence_config(HPM_ADC1);
#endif

    hall_driver_init_trigger_mux();

    hall_driver_init_gptmr_trigger_source();

    hall_driver_key_scan_dma_config();

    //hall_driver_mux_enable(true);
    //hall_driver_power(0, true);
    //hall_driver_power(1, true);

    #if (USE_GPTMR_TRIG_SCAN == SCAN_MODE_TIMER_TRG)
    hall_driver_auto_start_enable(false);
    hall_driver_trigger_scan_gptmr_init(KEY_TRIG_SCAN_GPTMR_CYCLE);
    #else
    hall_driver_auto_start_enable(true);
    #endif

    (void)hall_driver_key_scan_start;

    DBG_PRINTF("initialize\r\n");
}

void hall_driver_power(uint8_t index, bool enable)
{
    if (enable)
    {
        gpio_write_pin(HPM_GPIO0, GPIO_DO_GPIOY, index == 0 ? 0 : 1, 1);
        gpio_write_pin_extra(GPIO_DO_GPIOY, index == 0 ? 0 : 1, 1);
    }
    else
    {
        gpio_write_pin(HPM_GPIO0, GPIO_DO_GPIOY, index == 0 ? 0 : 1, 0);
        gpio_write_pin_extra(GPIO_DO_GPIOY, index == 0 ? 0 : 1, 0);
    }
}

void hall_driver_mux_enable(bool enable)
{
    if (enable)
    {
        gpio_write_pin(HPM_GPIO0, GPIO_DO_GPIOA, 31, 0);
        gpio_write_pin_extra(GPIO_DO_GPIOA, 31, 0);
    }
    else
    {
        gpio_write_pin(HPM_GPIO0, GPIO_DO_GPIOA, 31, 1);
        gpio_write_pin_extra(GPIO_DO_GPIOA, 31, 1);
    }
}

void gpio_write_pin_extra(uint32_t port, uint8_t pin, uint8_t high)
{
    if (port != GPIO_DO_GPIOA)
    {
        return;
    }

    uint8_t switch_data, i;
    uint32_t gpio_mask[KEY_SCAN_SWITCH_BIT_COUNT] = {1<<BOARD_CS_A0_PIN, 1<<BOARD_CS_A1_PIN, 1<<BOARD_CS_A2_PIN};
	uint32_t current_value = HPM_GPIO0->DO[GPIO_DO_GPIOA].VALUE;
    
    if (high)
    {
        current_value |= 1 << pin;
    }
    else
    {
        current_value &= ~(1 << pin);
    }

    for (switch_data = 0; switch_data < KEY_SCAN_SWITCH_CH_COUNT; switch_data++)
    {
        gpio_porta_set[switch_data] = current_value;
        for (i = 0; i < KEY_SCAN_SWITCH_BIT_COUNT; i++)
        {
            if (BIT(i) & switch_data)
            {
                gpio_porta_set[switch_data] |= gpio_mask[i];
            }
            else
            {
                gpio_porta_set[switch_data] &= ~gpio_mask[i];
            }
        }
    }
}
