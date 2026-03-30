/*
 * Copyright (c) 2024-2025 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "mg_sm.h"
#include "board.h"
#include "hal_gpio.h"
#include "hal_spi.h"
#include "app_debug.h"
#include "app_config.h"
#include "db.h"
#include "mg_uart.h"
#include "hpm_gpio_drv.h"
#include "hal_wdg.h"

#if DEBUG_EN
    #define   MODULE_LOG_ENABLED            (DBG_EN_SM)
#if MODULE_LOG_ENABLED == 1
    #include "SEGGER_RTT.h"
    #define   LOG_TAG          "mg_sm"
    #define   DBG_PRINTF(fmt, ...) Mg_SEGGER_RTT_printf(LOG_TAG,fmt, ##__VA_ARGS__) 
#else
    #define   DBG_PRINTF(format, ...)
#endif

#else
    #define   DBG_PRINTF(format, ...)
#endif

//获取按键映射表
#define GET_KEY_IDX         (0)
//测试丢包率
#define TEST_LOSS_STAT_EN   (0)

#if ((GET_KEY_IDX == 1) || (TEST_LOSS_STAT_EN == 1))
    #define SM_DATA_SEND_EN     (0)
#else
    //是否发送数据到键盘处理任务
    #define SM_DATA_SEND_EN     (1)
#endif

#define TEST_DATA_FIRST     (0x7F0)
#define TEST_DATA_SECOND    (0x7F2)

#define MCU0_ADC_CH                         (0x55555555)
#define MCU1_ADC_CH                         (0x55555555)
#define MCU2_ADC_CH                         (0x55555555)
#define MCU3_ADC_CH                         (0x55555555)
#define MCU4_ADC_CH                         (0x55655555)

dev_channel_map_s dev_channel_map[SM_DEV_NUM_MAX] = 
{
    {UART_DEVICE_3,MCU0_ADC_CH},
    {UART_DEVICE_0,MCU1_ADC_CH},
    {UART_DEVICE_2,MCU2_ADC_CH},
    {UART_DEVICE_1,MCU3_ADC_CH},
    {UART_DEVICE_4,MCU4_ADC_CH},
};
extern TaskHandle_t m_key_thread;
TaskHandle_t m_sm_thread;
static uint32_t teset_first_err_cnt = 0;
static uint32_t teset_first2_err_cnt = 0;
static uint32_t teset_second_err_cnt = 0;
ATTR_RAMFUNC void spi1_rx_callback(spi_msg_t *p_msg);
ATTR_RAMFUNC void spi2_rx_callback(spi_msg_t *p_msg);
ATTR_RAMFUNC void sm_notify_callback(void);

#if ((defined GET_KEY_IDX) && (GET_KEY_IDX == 1))
uint8_t test_sm_num = 0;
int8_t test_sm_frame_idx_buf[SM_DEV_NUM_MAX*SPI_RX_KEY_MAX] = {0};
#endif

void mg_nation_cfg_init(void);

sm_data_t sm_data;

sm_dev_state_t sm_dev_sta = 
{
    .bus_act = {0},
    .dev_cur_num = {0},
};

ATTR_RAMFUNC void spi_data_rx_handle(spi_msg_t *p_msg,sm_dev_data_s *raw_data_s,key_data_frame_e frame_mode)
{
    sm_data_t *p_data = &sm_data;
    key_packet_t key_packet[SPI_RX_KEY_MAX];
    uint8_t rx_len = p_msg->rx_len;
    uint8_t cs_num = p_msg->cur_cs_num;
    uint8_t copy_len = (p_msg->rx_len > SPI_RX_KEY_MAX) ? SPI_RX_KEY_MAX : p_msg->rx_len;
    memset(key_packet,0,sizeof(key_packet_t)*SPI_RX_KEY_MAX);
    memcpy(key_packet, p_msg->p_rx_buf, sizeof(key_packet_t)*copy_len);
     
    if ((frame_mode == KEY_DATA_TOTAL_FRAME) || (frame_mode == KEY_DATA_ZERO_FRAME))
    {
        uint8_t len = (rx_len > SPI_RX_KEY_MAX) ? SPI_RX_KEY_MAX : rx_len;
        memcpy(raw_data_s->sm_raw_data,key_packet,sizeof(uint16_t) * len);
        raw_data_s->remain_size = 0;
        if(frame_mode == KEY_DATA_ZERO_FRAME)
        {
            p_data->data_type[cs_num] = DATA_CALI_ZERO_FRAME_TYPE;
        }
        else
        {
            p_data->data_type[cs_num] = DATA_CALI_TATOL_FRAME_TYPE;
        }
        raw_data_s->is_complete = SPI_REV_SECOND_OK;
    }
    else
    {
        if (raw_data_s->remain_size == 0)
        {
            if (rx_len != RX_FIRST_PACK_LEN)
            {
                raw_data_s->is_complete = SPI_REV_FIRST_LEN_ERROR;
                teset_first_err_cnt++;
            }
            else if ((key_packet[RX_FIRST_KEY_NUM].bit_property.mode > DATA_CALI_BOT_FRAME_TYPE) || \
                     (key_packet[RX_FIRST_KEY_NUM].bit_property.mode == DATA_NONE_FRAME_TYPE) || \
                     (key_packet[RX_FIRST_KEY_NUM].bit_property.num > (SPI_RX_KEY_MAX - RX_FIRST_KEY_NUM)))
            {
                raw_data_s->is_complete = SPI_REV_FIRST_MODE_ERROR;
                teset_first2_err_cnt++;
                
            }
            else
            {
                if(key_packet[RX_FIRST_KEY_NUM].bit_property.num > 0)
                {
                    raw_data_s->remain_size = key_packet[RX_FIRST_KEY_NUM].bit_property.num;
                }
                else
                {
                    raw_data_s->remain_size = 0;
                }
                raw_data_s->is_complete = SPI_REV_FIRST_OK;
                memcpy(raw_data_s->sm_raw_data,key_packet,sizeof(uint16_t) * RX_FIRST_KEY_NUM);
                p_data->data_type[cs_num] = key_packet[RX_FIRST_KEY_NUM].bit_property.mode;
                //DBG_PRINTF("sm cs :%d,type:%d\n",cs_num,p_data->data_type[cs_num]);
            }
        }
        else
        {
            if (rx_len != raw_data_s->remain_size)
            {
                raw_data_s->is_complete = SPI_REV_SECOND_ERROR;
                teset_second_err_cnt++;
            }
            else
            {
                uint8_t len = (rx_len > (SPI_RX_KEY_MAX - RX_FIRST_KEY_NUM)) ? (SPI_RX_KEY_MAX - RX_FIRST_KEY_NUM) : p_msg->rx_len;
                memcpy(raw_data_s->sm_raw_data + RX_FIRST_KEY_NUM,key_packet,sizeof(uint16_t) * len);
                //p_data->data_type[p_msg->cur_cs_num] = p_buf[RX_FIRST_KEY_NUM].bit_property.mode;
                raw_data_s->is_complete = SPI_REV_SECOND_OK;
            }
            raw_data_s->remain_size = 0;
        }
    }            
}



ATTR_RAMFUNC void sm_state_clear(void)
{
    portENTER_CRITICAL();// 禁用中断，确保清零过程不被中断篡改
    memset(&sm_data, 0, sizeof(sm_data_t));
    memset(&sm_dev_sta, 0, sizeof(sm_dev_state_t));
    portEXIT_CRITICAL();// 恢复中断
}

ATTR_RAMFUNC void mg_scan_task_notify(void)
{
    if (m_sm_thread != NULL)
    {
        xTaskNotifyGive(m_sm_thread);
    }
}

ATTR_RAMFUNC spi_remain_state_s mg_get_remain_dev_num(void)
{
    spi_remain_state_s spi_remain;
    sm_dev_state_t *p_dev_sta = &sm_dev_sta;
    uint8_t state = hal_notify_get_state(); //读完周期内的变化

    memset(&spi_remain, 0, sizeof(spi_remain_state_s));

    for (uint8_t i = 0; i < SM_DEV_NUM_MAX; i++)
    {
        if(p_dev_sta->dev_data[i].rev_ready != 2)
        {
            if ((state >> i) & 0x01)
            {
                p_dev_sta->dev_data[i].rev_ready = 1;
            }
            if(p_dev_sta->dev_data[i].rev_ready == 0)
            {
                spi_remain.remain_all_num++;
                if(i < SM_DEV_NUM_2)
                {
                    spi_remain.ntf_not_rdy_num[SM_BUS_1]++;
                }
                else
                {
                    spi_remain.ntf_not_rdy_num[SM_BUS_0]++;
                }
            }
            else
            {
                spi_remain.remain_all_num++;
                if(i < SM_DEV_NUM_2)
                {
                    spi_remain.ntf_rdy_num[SM_BUS_1]++;
                }
                else
                {
                    spi_remain.ntf_rdy_num[SM_BUS_0]++;
                }
            }
        }
    }
    return spi_remain;
}

ATTR_RAMFUNC void mg_spi_receive_data(bus_num_t sm_bus)
{
    sm_dev_state_t *p_dev_sta = &sm_dev_sta;
    sys_run_info_s *sys_run_info = &sys_run;
    uint8_t start_num = (sm_bus == SM_BUS_0) ? SM_DEV_NUM_2 : SM_DEV_NUM_0;
    uint8_t end_num = (sm_bus == SM_BUS_0) ? SM_DEV_NUM_MAX : SM_DEV_NUM_2;
    uint8_t target_dev = 0xFF; // 目标设备号（初始化为无效值）
    uint8_t len = 0;           // 接收长度

    // 检查总线是否忙碌
    bool bus_busy = (p_dev_sta->bus_act[SM_BUS_0] || p_dev_sta->bus_act[SM_BUS_1]);
    if (!bus_busy && !p_dev_sta->bus_act[sm_bus])
    {
        // 遍历设备，找到需要接收的目标设备
        for (uint8_t i = start_num; i < end_num; i++)
        {
            if (p_dev_sta->dev_data[i].rev_ready == 1)
            {
                target_dev = i;
                len = ((sys_run_info->sys_frame_mode == KEY_DATA_TOTAL_FRAME) || 
                       (sys_run_info->sys_frame_mode == KEY_DATA_ZERO_FRAME)) ? 
                       (SPI_RX_KEY_MAX) : RX_FIRST_PACK_LEN;
                len += SPI_CRC_LEN;
                break;
            }
        }
    }

    if (target_dev != 0xFF) // 存在目标设备
    {
        p_dev_sta->dev_data[target_dev].rev_ready = 2;
        p_dev_sta->dev_cur_num[sm_bus] = target_dev;
		
        if (hal_spi_receive(target_dev, len) == 0)
        {
            p_dev_sta->bus_act[sm_bus] = true;
            // DBG_PRINTF("sm_bus start receive dev %d\n", target_dev);
        }
        else
        {
            p_dev_sta->bus_act[sm_bus] = true;
        }
    }
}

ATTR_RAMFUNC void mg_spi_receive_timeout(void)
{
    sm_dev_state_t *p_dev_sta = &sm_dev_sta;
    p_dev_sta->bus_act[0] = 0;
    p_dev_sta->bus_act[1] = 0;
    p_dev_sta->rev_all_ok = 0;
     
    spi_clear_interrupt_status(BOARD_SPI1_BASE, 
                               spi_rx_fifo_overflow_int | spi_tx_fifo_underflow_int);
    spi_receive_fifo_reset(BOARD_SPI1_BASE);
    
    spi_clear_interrupt_status(BOARD_SPI2_BASE, 
                               spi_rx_fifo_overflow_int | spi_tx_fifo_underflow_int);
    spi_receive_fifo_reset(BOARD_SPI2_BASE);

    hal_spi_write_cs(0, !BOARD_SPI_CS_ACTIVE_LEVEL);
    hal_spi_write_cs(1, !BOARD_SPI_CS_ACTIVE_LEVEL);
    hal_spi_write_cs(2, !BOARD_SPI_CS_ACTIVE_LEVEL);
    hal_spi_write_cs(3, !BOARD_SPI_CS_ACTIVE_LEVEL);
    hal_spi_write_cs(4, !BOARD_SPI_CS_ACTIVE_LEVEL);
    
}

uint32_t data_err_cnt = 0;
uint32_t data_finish_err_cnt = 0;
uint32_t data_timeout_err_cnt = 0;
uint32_t data_timeout_err2_cnt = 0;
ATTR_RAMFUNC void mg_data_handle(void)
{
    sm_dev_state_t *p_dev_sta = &sm_dev_sta;
    sm_data_t *p_data = &sm_data;

    if (!p_dev_sta->timeout_flag)
    {
        if ((p_dev_sta->bus_act[SM_BUS_0] == true) || (p_dev_sta->bus_act[SM_BUS_1] == true))
        {
            return;
        }
    }
    
    memset(p_data->pack, 0, sizeof(p_data->pack));
    memset(p_data->dev_data_num, 0, sizeof(p_data->dev_data_num));
    p_data->all_key_num = 0;

    for(uint8_t i = 0; i< SM_DEV_NUM_MAX; i++)
    {
        key_packet_t *p_buf = (key_packet_t *)p_dev_sta->dev_data[i].sm_raw_data;
        
        if((p_dev_sta->dev_data[i].rev_ready == 2) && (p_dev_sta->dev_data[i].is_complete != SPI_REV_FIRST_OK) && (p_dev_sta->dev_data[i].is_complete != SPI_REV_SECOND_OK))
        {
            data_finish_err_cnt++;
        }
        if(p_dev_sta->dev_data[i].rev_ready == 1)
        {
            data_timeout_err2_cnt++;
        }

        if(p_data->data_type[i] == DATA_NONE_FRAME_TYPE)
        {
            continue;
        }

        bool data_is_ok = true;
        for (uint8_t j = 0; j< SPI_RX_KEY_MAX; j++)
        {
            if (p_buf[j].val == 0)
            {
                continue;
            }

            if(p_buf[j].bit.id >= SPI_RX_KEY_MAX)
            {
                data_is_ok = false;
            }

            if (j > 0) 
            {
                if (p_buf[j].bit.id < p_buf[j - 1].bit.id)
                {
                    data_is_ok = false;
                }
            }

            uint8_t ch_id = (i * SPI_RX_KEY_MAX) + p_buf[j].bit.id;
            if(ch_id >= (SM_DEV_NUM_MAX * SPI_RX_KEY_MAX))
            {
                data_is_ok = false;
            }

            if(!data_is_ok)
            {
                data_err_cnt++;
                p_data->dev_data_num[i] = 0;
                memset(&p_data->pack[i * SPI_RX_KEY_MAX], 0, sizeof(uint16_t) * SPI_RX_KEY_MAX);
                break;
            }

            if ((p_buf[j].bit.adc > ADC_VAL_MIN) && (p_buf[j].bit.adc < ADC_VAL_MAX))
            {
                if(p_data->data_type[i] == DATA_CALI_ZERO_FRAME_TYPE)
                {
                    p_data->pack[ch_id].ty  = PKT_TYPE_ZERO_CALI;
                }
                else if(p_data->data_type[i] == DATA_CALI_TOP_FRAME_TYPE)
                {
                    p_data->pack[ch_id].ty  = PKT_TYPE_TOP_CALI;
                }
                else if(p_data->data_type[i] == DATA_CALI_BOT_FRAME_TYPE)
                {
                    p_data->pack[ch_id].ty  = PKT_TYPE_BOT_CALI;
                }
                else if(p_data->data_type[i] == DATA_KEY_FRAME_TYPE)
                {
                    p_data->pack[ch_id].ty  = PKT_TYPE_CHANGE;
                }
                else if(p_data->data_type[i] == DATA_CALI_TATOL_FRAME_TYPE)
                {
                    p_data->pack[ch_id].ty  = PKT_TYPE_CHANGE;
                }
                else
                {
                      break;
                }
                p_data->pack[ch_id].adc = p_buf[j].bit.adc; 
                p_data->dev_data_num[i]++;
                p_data->all_key_num++;
            }
        } 
    }
    
    mg_spi_receive_timeout();
    p_dev_sta->rev_all_ok = true;
    if (p_data->all_key_num > 0)
    {
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        if (pdFAIL != xTaskNotifyFromISR(m_key_thread, (uint32_t)p_data, eSetValueWithoutOverwrite, &higherPriorityTaskWoken))
        {
            portYIELD_FROM_ISR(higherPriorityTaskWoken);
        }
    }
}


uint32_t data_real_timeout_cnt = 0;

ATTR_RAMFUNC void spi0_data_rx_handle(spi_msg_t *p_msg)
{
    sm_dev_state_t *p_dev_sta = &sm_dev_sta;
    sys_run_info_s *sys_run_info = &sys_run;
    uint8_t cur_dev_num = p_dev_sta->dev_cur_num[p_msg->bus_num];
    uint8_t start_num = (p_msg->bus_num == SM_BUS_0) ? SM_DEV_NUM_2 : SM_DEV_NUM_0;
    uint8_t end_num = (p_msg->bus_num == SM_BUS_0) ? SM_DEV_NUM_MAX : SM_DEV_NUM_2;

    do
    {
        if (p_dev_sta->rev_all_ok)
        {
            return;
        }
        if (p_msg->is_crc_err)
        {
           p_msg->is_crc_err = false;
           p_dev_sta->bus_act[p_msg->bus_num] = false;
           break;
        }

        if ((cur_dev_num >= start_num) && (cur_dev_num < end_num))
        {
            spi_data_rx_handle(p_msg,&p_dev_sta->dev_data[cur_dev_num],sys_run_info->sys_frame_mode);
            if ((p_dev_sta->dev_data[cur_dev_num].is_complete == SPI_REV_FIRST_OK) && (p_dev_sta->dev_data[cur_dev_num].remain_size > 0))
            {
                hal_spi_receive(cur_dev_num, p_dev_sta->dev_data[cur_dev_num].remain_size + SPI_CRC_LEN);
                return;
            }
            /*else if(p_dev_sta->dev_data[cur_dev_num].is_complete == SPI_REV_FIRST_MODE_ERROR)
            {
                hal_spi_receive(cur_dev_num, 14);
                return;
            }*/
        }
        p_dev_sta->bus_act[p_msg->bus_num] = false;
    }while(0);

    if (hpm_csr_get_core_cycle() < p_dev_sta->dev_time)
    {
        for (uint8_t i = 0; i< 3; i++)
        {
            spi_remain_state_s spi_remain = mg_get_remain_dev_num();

            if(spi_remain.remain_all_num == 0)
            {
                if ((p_dev_sta->bus_act[SM_BUS_0] == false) && (p_dev_sta->bus_act[SM_BUS_1] == false))
                {
                    mg_data_handle();
                    return;
                }
            }

            if ((p_dev_sta->bus_act[SM_BUS_0] == false) && (spi_remain.ntf_rdy_num[SM_BUS_0] > 0))
            {
                mg_spi_receive_data(SM_BUS_0);
            }

            if ((p_dev_sta->bus_act[SM_BUS_1] == false) && (spi_remain.ntf_rdy_num[SM_BUS_1] > 0))
            {
                mg_spi_receive_data(SM_BUS_1);
            }

            /*if ((p_dev_sta->bus_act[SM_BUS_0] == false) && (spi_remain.ntf_not_rdy_num[SM_BUS_0] > 0))
            {
                board_delay_us(3);
                continue;
            }*/

            if ((p_dev_sta->bus_act[SM_BUS_0] == false) && (p_dev_sta->bus_act[SM_BUS_1] == false))
            {
                if((spi_remain.ntf_not_rdy_num[SM_BUS_0] > 0) || (spi_remain.ntf_not_rdy_num[SM_BUS_1] > 0))
                {
                    board_delay_us(3);
                    continue;
                }
            }
        }

        if ((p_dev_sta->bus_act[SM_BUS_0] == false) && (p_dev_sta->bus_act[SM_BUS_1] == false))
        {
            mg_data_handle();
        }
    }
    else
    {
        data_real_timeout_cnt++;
        p_dev_sta->timeout_flag = true;
        mg_data_handle();
    }
}

ATTR_RAMFUNC void spi1_data_rx_handle(spi_msg_t *p_msg)
{
    sm_dev_state_t *p_dev_sta = &sm_dev_sta;
    sys_run_info_s *sys_run_info = &sys_run;
    uint8_t cur_dev_num = p_dev_sta->dev_cur_num[p_msg->bus_num];
    uint8_t start_num = (p_msg->bus_num == SM_BUS_0) ? SM_DEV_NUM_2 : SM_DEV_NUM_0;
    uint8_t end_num = (p_msg->bus_num == SM_BUS_0) ? SM_DEV_NUM_MAX : SM_DEV_NUM_2;

    do
    {
        if (p_dev_sta->rev_all_ok)
        {
            return;
        }

        if (p_msg->is_crc_err)
        {
           p_msg->is_crc_err = false;
           p_dev_sta->bus_act[SM_BUS_1] = false;
           break;
        }

        if ((cur_dev_num >= start_num) && (cur_dev_num < end_num))
        {
            spi_data_rx_handle(p_msg,&p_dev_sta->dev_data[cur_dev_num],sys_run_info->sys_frame_mode);
            if ((p_dev_sta->dev_data[cur_dev_num].is_complete == SPI_REV_FIRST_OK) && (p_dev_sta->dev_data[cur_dev_num].remain_size > 0))
            {
                hal_spi_receive(cur_dev_num, p_dev_sta->dev_data[cur_dev_num].remain_size + SPI_CRC_LEN);
                return;
            }
            /*else if (p_dev_sta->dev_data[cur_dev_num].is_complete == SPI_REV_FIRST_MODE_ERROR)
            {
                hal_spi_receive(cur_dev_num, 14);
                return;
            }*/

            
        } 
        p_dev_sta->bus_act[p_msg->bus_num] = false;
    }while(0);
    
    if (hpm_csr_get_core_cycle() < p_dev_sta->dev_time)
    {
        for (uint8_t i = 0; i< 3; i++)
        {
            spi_remain_state_s spi_remain = mg_get_remain_dev_num();

            if(spi_remain.remain_all_num == 0)
            {
                if ((p_dev_sta->bus_act[SM_BUS_0] == false) && (p_dev_sta->bus_act[SM_BUS_1] == false))
                {
                    mg_data_handle();
                    return;
                }
            }

            if ((p_dev_sta->bus_act[SM_BUS_1] == false) && (spi_remain.ntf_rdy_num[SM_BUS_1] > 0))
            {
                mg_spi_receive_data(SM_BUS_1);
            }

            if ((p_dev_sta->bus_act[SM_BUS_0] == false) && (spi_remain.ntf_rdy_num[SM_BUS_0] > 0))
            {
                mg_spi_receive_data(SM_BUS_0);
            }

            /*if ((p_dev_sta->bus_act[SM_BUS_1] == false) && (spi_remain.ntf_not_rdy_num[SM_BUS_1] > 0))
            {
                board_delay_us(3);
                continue;
            }*/

            if ((p_dev_sta->bus_act[SM_BUS_0] == false) && (p_dev_sta->bus_act[SM_BUS_1] == false))
            {
                if((spi_remain.ntf_not_rdy_num[SM_BUS_0] > 0) || (spi_remain.ntf_not_rdy_num[SM_BUS_1] > 0))
                {
                    board_delay_us(3);
                    continue;
                }
            }
        }
        if ((p_dev_sta->bus_act[SM_BUS_0] == false) && (p_dev_sta->bus_act[SM_BUS_1] == false))
        {
            mg_data_handle();
        }
    }
    else
    {
        data_real_timeout_cnt++;
        p_dev_sta->timeout_flag = true;
        mg_data_handle();
    }
    
}
uint32_t ntf_cnt = 0;
static void sm_thread(void *pvParameters)
{
    (void)pvParameters;
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    hal_spi_set_rx_callback(spi1_rx_callback, spi2_rx_callback);
    hal_gpioa_register_notify_callback(sm_notify_callback);
    uint32_t ticks_per_us = clock_get_core_clock_ticks_per_us();
    uint32_t ticks_per_ms = clock_get_core_clock_ticks_per_ms();
    const uint64_t WAITE_40US = ticks_per_us * 50UL;
    uint64_t sys_start_time = hpm_csr_get_core_cycle();
    vTaskDelay(400);
    mg_nation_cfg_init();
    DBG_PRINTF("sm_thread init\n"); 

    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint64_t cur_timestap = hpm_csr_get_core_cycle();
        if(cur_timestap < (sys_start_time + (ticks_per_ms * 1000)))
        {
            continue;
        }
        ntf_cnt++;
        sm_dev_state_t *p_dev_sta = &sm_dev_sta;
        spi_clear();
        sm_state_clear();
        p_dev_sta->dev_time = cur_timestap + WAITE_40US;
        spi_remain_state_s spi_remain = mg_get_remain_dev_num();
        if (spi_remain.ntf_rdy_num[SM_BUS_0] > 0)
        {
            mg_spi_receive_data(SM_BUS_0);
        }
        if (spi_remain.ntf_rdy_num[SM_BUS_1] > 0)
        {
            mg_spi_receive_data(SM_BUS_1);
        }
    }
}
ATTR_RAMFUNC void spi1_rx_callback(spi_msg_t *p_msg)
{
    spi0_data_rx_handle(p_msg);
}
ATTR_RAMFUNC void spi2_rx_callback(spi_msg_t *p_msg)
{
    spi1_data_rx_handle(p_msg);
}

ATTR_RAMFUNC void sm_notify_callback(void)
{
    BaseType_t result;
    uint32_t value = 0;
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    result = xTaskNotifyFromISR(m_sm_thread, value, eSetValueWithoutOverwrite, &higherPriorityTaskWoken);
    if (pdFAIL != result)
    {
        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }
}

void mg_sm_init(void)
{
    DBG_PRINTF("sm_thread init!\n");

    if (xTaskCreate(sm_thread, "sm_thread", STACK_SIZE_SM, NULL, PRIORITY_SM, &m_sm_thread) != pdPASS)
    {
        DBG_PRINTF("sm_thread creation failed!\n");
        for (;;)
        {
            ;
        }
    }
}

void mg_sm_uninit(void)
{
    hal_notify_enable(false);
    if (m_sm_thread)
    {
        vTaskSuspend(m_sm_thread);
    }
}


void mg_nation_cfg_init(void)
{
    uint8_t send_buf[UART_SM_CMD_BUF_SIZE] = {0};
    uint16_t indx = 0;
    uint8_t retry = 3; // 
    while (retry--)
    {
        do
        {
            uint8_t i = 0;
            for (i = 0; i < MCU_NUM_MAX_SIZE; i++)
            {
                memset(send_buf,0,UART_SM_CMD_BUF_SIZE);
                indx = 0;
                send_buf[indx++] = (uint8_t)dev_channel_map[i].channel;
                send_buf[indx++] = (uint8_t)(dev_channel_map[i].channel >> 8);
                send_buf[indx++] = (uint8_t)(dev_channel_map[i].channel >> 16);
                send_buf[indx++] = (uint8_t)(dev_channel_map[i].channel >> 24);
                if ( UART_SUCCESS != mg_uart_send_to_single_nation_request_cmd(dev_channel_map[i].uart_id, SM_SET_ADC_CH_INFO_CMD, indx, send_buf,UART_RETRY_TIMES))
                {
                    break;
                }

                DBG_PRINTF(" SM_SET_ADC_CH_INFO_CMD send uart_%d success\r\n",i);
            }
            if (i != MCU_NUM_MAX_SIZE)
            {
                break;
            }

            memset(send_buf,0,UART_SM_CMD_BUF_SIZE);
            indx = 0;
#if ((defined GET_KEY_IDX) && (GET_KEY_IDX == 1))
            uint16_t tmp_th = 500;
            send_buf[indx++] = (uint8_t)tmp_th;
            send_buf[indx++] = (uint8_t)(tmp_th >> 8);
#else
            send_buf[indx++] = (uint8_t)KEY_UPLOAD_THRESHOLD;
            send_buf[indx++] = (uint8_t)(KEY_UPLOAD_THRESHOLD >> 8);
#endif
            if (UART_SUCCESS != mg_uart_send_to_all_nation_request_cmd(SM_SET_THRESHOLD_CMD, indx, send_buf)) //设置阈值
            {
                break;
            }
            DBG_PRINTF(" SM_SET_THRESHOLD_CMD send success\r\n");

            memset(send_buf,0,UART_SM_CMD_BUF_SIZE);
            indx = 0;
            send_buf[indx++] = (uint8_t)KEY_LOW_PWR_MODE_TIME;
            send_buf[indx++] = (uint8_t)(KEY_LOW_PWR_MODE_TIME >> 8);

            if (UART_SUCCESS != mg_uart_send_to_all_nation_request_cmd(SM_SET_LOWPWR_TIME_CMD, indx, send_buf)) //设置进入节能模式时间
            {
                break;
            }
            DBG_PRINTF(" SM_SET_LOWPWR_TIME_CMD send success\r\n");

            memset(send_buf,0,UART_SM_CMD_BUF_SIZE);
            indx = 0;
            send_buf[indx++] = (uint8_t)KEY_LOW_PWR_SCAN_FREQ;
            send_buf[indx++] = (uint8_t)(KEY_LOW_PWR_SCAN_FREQ >> 8);

            if (UART_SUCCESS != mg_uart_send_to_all_nation_request_cmd(SM_SET_LOWPWR_FREQ_CMD, indx, send_buf)) //设置低功耗扫描频率
            {
                break;
            }
            DBG_PRINTF(" SM_SET_LOWPWR_FREQ_CMD send success\r\n");

            memset(send_buf,0,UART_SM_CMD_BUF_SIZE);
            indx = 0;
            send_buf[indx++] = (uint8_t)KEY_HALL_STABLE_TIME;

            if (UART_SUCCESS != mg_uart_send_to_all_nation_request_cmd(SM_SET_HALL_STABLE_TIME_CMD, indx, send_buf)) //设置霍尔稳定时间
            {
                break;
            }
            DBG_PRINTF(" SM_SET_HALL_STABLE_TIME_CMD send success\r\n");

            memset(send_buf,0,UART_SM_CMD_BUF_SIZE);
            indx = 0;
            send_buf[indx++] = (uint8_t)SPI_RX_KEY_MAX;

            if (UART_SUCCESS != mg_uart_send_to_all_nation_request_cmd(SM_SET_CH_MAX_CMD, indx, send_buf))
            {
                break;
            }
            DBG_PRINTF(" SM_SET_CH_MAX_CMD send success\r\n");

            

            return;

        }while(0);
        hal_wdg_manual_feed();
        vTaskDelay(200); // 重试间隔
    }

    while(1)
    {
        DBG_PRINTF("mg_nation_cfg_init error!.\n");
        vTaskDelay(1000);
    }
}
