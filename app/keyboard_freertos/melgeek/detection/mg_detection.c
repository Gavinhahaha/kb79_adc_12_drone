#include "FreeRTOS.h"
#include "task.h"
#include "hal_adc.h"
#include "mg_detection.h"
#include "app_debug.h"
#include "app_config.h"
#include "mg_hive.h"
#include "mg_uart.h"
#include "mg_sm.h"
#include "rgb_led_action.h"
#include "mg_matrix.h"
#include "mg_hid.h"

#if DEBUG_EN == 1
    #define   MODULE_LOG_ENABLED            (1)
#if MODULE_LOG_ENABLED == 1
    #define LOG_TAG          "mg_detecte"
    #define   DBG_PRINTF(fmt, ...)     DEBUG_RTT(LOG_TAG,fmt, ##__VA_ARGS__)
#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#else
    #define   DBG_PRINTF(fmt, ...)
#endif

#define   ADC_RES_VOLT_DIV                   (75.0f/195.0f)
#define   ADC_SAMPLE_PRECISION               (65535.0f)
#define	  ADC_SAMPLE_VOLT                    (3.3f)
#define   ADC_COEF                           (ADC_SAMPLE_PRECISION/ADC_SAMPLE_VOLT*ADC_RES_VOLT_DIV)
#define   VOLT_MAX                           (7000)
#define   VOLT_WARN_LEVEL1_MIN               (4300)
#define   VOLT_WARN_LEVEL2_MIN               (4150)
#define   VOLT_WARN_LEVEL3_MIN               (3950)
#define   VOLT_WAVE                          (130) //WAVE 150MV
#define   WATER_WAVE                         (100) //WAVE 100
#define   WATER_MIN                          (2500)
#define   VOLT_IS_VALID(VOLT)                (((VOLT > 7000) || (VOLT < 3000)) ? 0XFFFF : VOLT)

#define VOLT_SAMPLE_TIMES                    (60)
#define VOLT_SAMPLE_AVG_TIMES                (5)

static detect_data_s detect_data;
static TaskHandle_t m_detecete_thread;
static uint16_t volt_buf[VOLT_SAMPLE_TIMES];
static uint16_t last_volt_buf[VOLT_SAMPLE_AVG_TIMES];
static uint32_t volt_cnt = 0;
static uint32_t volt_avg_cnt = 0;
static bool volt_warn_enable_state = true;
extern bool is_timer_key_unchange_flag;

void mg_detected_suspend(void)
{
    if (m_detecete_thread)
    {
        vTaskSuspend(m_detecete_thread);
    }
}

void mg_detecte_task_notify(void)
{
    if (m_detecete_thread != NULL)
    {
        xTaskNotifyGive(m_detecete_thread);
    }
}

uint8_t mg_detecte_get_err_status(void)
{
    return sys_run.sys_err_status;
}

void mg_detecte_set_volt_warn_check_state(bool state)
{
    volt_warn_enable_state = state;
}

bool mg_detecte_get_volt_warn_check_state(void)
{
    return volt_warn_enable_state;
}

bool mg_detecte_get_volt_low_state(void)
{
    return (sys_run.volt_err_level > VOLT_WARN_LEVEL_1) ? true : false;
}

static void detecte_ch_init(void)
{
    memset(detect_data.detecte_water_pos_buf,0xff,sizeof(detect_data.detecte_water_pos_buf));
    detect_data.valid_water_pos_num = 0;
    for(uint8_t i = 0; i < MCU_NUM_MAX_SIZE; i++)
    {
        uint8_t temp_ch_num = 0;
        for(uint8_t j = 0; j < SPI_RX_KEY_MAX ; j++)
        {
            if(((dev_channel_map[i].channel >> (j*2)) & (0x03)) == 0)
            {
                continue;
            }

            if(((dev_channel_map[i].channel >> (j*2)) & (0x03)) == 0x03)
            {
                detect_data.detecte_volt_pos = i * SPI_RX_KEY_MAX + temp_ch_num;
                DBG_PRINTF("detecte_volt_pos :%d\n",detect_data.detecte_volt_pos);
            }
            else if(((dev_channel_map[i].channel >> (j*2)) & (0x02)) == 0x02)
            {
                if(detect_data.valid_water_pos_num < DETECTE_NUM_MAX)
                {
                    detect_data.detecte_water_pos_buf[detect_data.valid_water_pos_num++] =  i * SPI_RX_KEY_MAX + temp_ch_num;
                    DBG_PRINTF("detecte_water_pos_buf[%d] :%d\n",detect_data.valid_water_pos_num-1,detect_data.detecte_water_pos_buf[detect_data.valid_water_pos_num-1]);
                }
                
            }
            temp_ch_num++;
        }
    }
}

static void detect_volt_handle(void)
{
    static uint16_t volt_err_cnt = 0;
    static uint16_t volt_recovery_cnt = 0;

    uint16_t cur_volt_err_level = VOLT_WARN_LEVEL_3;  // 默认最严重等级
    sys_run_info_s *sys_run_info = &sys_run;
    uint16_t volt = sys_run_info->volt;

    // 定义每个等级的最低电压门槛（从高到低）
    const uint16_t level_thresholds[4] = {
        VOLT_WARN_LEVEL1_MIN,  // Level 0: 正常
        VOLT_WARN_LEVEL2_MIN,  // Level 1
        VOLT_WARN_LEVEL3_MIN,  // Level 2
        0                      // Level 3: 极低
    };

    // 查找第一个满足 volt >= threshold 的 level（即最高优先级）
    for (int i = 0; i < 4; i++) 
    {
        if (volt >= level_thresholds[i]) 
        {
            cur_volt_err_level = i;
            break;
        }
    }
    //DBG_PRINTF("sys_run.volt_err_levelqqq:%d,volt:%d,sys_run.volt_err_level:%d\r\n",cur_volt_err_level,volt,sys_run_info->volt_err_level);
    if(sys_run_info->volt_err_level > VOLT_WARN_LEVEL_MAX)
    {
        DBG_PRINTF("sys_run.volt_err_level err:%d\r\n",sys_run_info->volt_err_level);
        sys_run_info->volt_err_level = VOLT_WARN_LEVEL_0;
    }
    //DBG_PRINTF("addr: buf=%p, last_buf=%p, sys_run=%p\n", 
    //       volt_buf, last_volt_buf, &sys_run);
    
    // === 状态迁移处理 ===

    if (cur_volt_err_level > sys_run_info->volt_err_level) 
    {
        // 恶化：进入更严重的等级
        if (volt_err_cnt >= 3) 
        {
            sys_run_info->volt_err_level = cur_volt_err_level;
        } 
        else 
        {
            volt_err_cnt++;
        }
        volt_recovery_cnt = 0;  // 重置恢复计数
    }
    else if (cur_volt_err_level < sys_run_info->volt_err_level) 
    {
        // 改善：尝试降级
        // 设置迟滞阈值：必须高于目标等级的基准 + 100
        uint16_t hysteresis_threshold = level_thresholds[sys_run_info->volt_err_level-1] + VOLT_WAVE;
        if (volt > hysteresis_threshold) 
        {
            if (volt_recovery_cnt >= 3) 
            {
                sys_run_info->volt_err_level = sys_run_info->volt_err_level-1;
            } 
            else 
            {
                volt_recovery_cnt++;
            }
            volt_err_cnt = 0;
        } 
        else 
        {
            // 未真正脱离风险
            volt_recovery_cnt = 0;
            volt_err_cnt = 0;
        }
    }
    else 
    {
        // 等级一致
        volt_err_cnt = 0;
        volt_recovery_cnt = 0;
    }

    // === 统一更新故障标志 ===
    if (sys_run_info->volt_err_level > VOLT_WARN_LEVEL_0) 
    {
        sys_run_info->sys_err_status |= SM_VOLT_LOW_ERROR;
    } 
    else 
    {
        sys_run_info->sys_err_status &= ~SM_VOLT_LOW_ERROR;
    }
}

static void detect_water_handle(sm_data_t *p_sm_data)
{
    static uint16_t water_err_cnt = 0;
    static uint16_t water_normal_cnt = 0;
    static uint8_t cur_water_status[DETECTE_NUM_MAX] = {0,0,0,0,0,0};
    bool err_valid_flag = false;
    bool normal_valid_flag = false;
    TickType_t curr_sys_ticks  = xTaskGetTickCount();

    if (is_timer_key_unchange_flag || (NULL == p_sm_data) || \
       (curr_sys_ticks < pdMS_TO_TICKS(1000)))
    {
        return;
    }

    for(uint8_t i = 0; i < detect_data.valid_water_pos_num; i++)
    {
        uint8_t water_pos = detect_data.detecte_water_pos_buf[i];
        if ((p_sm_data->pack[water_pos].ty == PKT_TYPE_CHANGE) || (p_sm_data->pack[water_pos].ty == PKT_TYPE_TATOL_CALI))
        {
            uint16_t tmp_adc = p_sm_data->pack[water_pos].adc;
            if ((tmp_adc < ADC_VAL_MIN) || (tmp_adc > ADC_VAL_MAX))
            {
                    continue;
            }
            if (tmp_adc < WATER_MIN)
            {
                cur_water_status[i] = SM_WATER_ERROR;
                err_valid_flag = true;
            }
            else  if(tmp_adc > (WATER_MIN + WATER_WAVE))//防止反复波动
            {
                cur_water_status[i] = 0;
                normal_valid_flag = true;
            }
        }
    }

    if(err_valid_flag || normal_valid_flag)
    {
        if(err_valid_flag)
        {
            if((!(sys_run.sys_err_status &SM_WATER_ERROR)) && (water_err_cnt >= 10))
            {
                sys_run.sys_err_status |= SM_WATER_ERROR;
            }
            water_err_cnt++;
            water_normal_cnt = 0;
        }
        else
        {
            bool all_normal_flag = true;
            for(uint8_t i = 0; i < detect_data.valid_water_pos_num; i++)
            {
                if(cur_water_status[i] == SM_WATER_ERROR)
                {
                    all_normal_flag = false;
                    break;
                }
            }

            if(all_normal_flag)
            {
                if((sys_run.sys_err_status &SM_WATER_ERROR) && (water_normal_cnt > 2))
                {
                    sys_run.sys_err_status &= ~SM_WATER_ERROR;
                }
                water_normal_cnt++;
                water_err_cnt = 0;
            }
        }
    }
    
    return;
}
void detecte_data_handle(sm_data_t *p_sm_data)
{
    static uint64_t curr_sys_run_time = 0;
    static uint8_t last_err_status = 0;
    static uint16_t abandon_volt_cnt = 0;
    static uint8_t last_volt_err_level = VOLT_WARN_LEVEL_0;
    static bool first_volt_warn_flag = false;
    const uint64_t now = xTaskGetTickCount();

    detect_water_handle(p_sm_data);

    if ((now - curr_sys_run_time) <= pdMS_TO_TICKS(30)) 
    {
        return; 
    }
    curr_sys_run_time = now;

    uint16_t adc_value = hal_adc_volt_value_get();

    // ADC 转换为 mV
    uint16_t volt_mV = (uint16_t)(((float)(adc_value)/ADC_COEF)*1000);  // 示例：12bit ADC

    // 电压有效性检查
    if (INVAIL_UINT16 == VOLT_IS_VALID(volt_mV))
    {
        return;
    }
        
    if(abandon_volt_cnt < 20)
    {
        abandon_volt_cnt++;
        return;
    }

    // 限幅
    volt_mV = (volt_mV > 5000) ? 5000 : volt_mV;
    

    // 环形缓冲存储原始采样
    volt_buf[volt_cnt++ % VOLT_SAMPLE_TIMES] = volt_mV;

    // 计算当前窗口平均值（去零）
    uint32_t sum = 0;
    uint8_t valid_count = 0;
    uint16_t min_val= 0xffff, max_val = 0,diff = 0,avrg_val = 0;
    uint16_t filtered_volt = 0;
    for (int i = 0; i < VOLT_SAMPLE_TIMES; i++) 
    {
        uint16_t val = volt_buf[i];
        if (volt_buf[i] == 0) 
        {
            continue;
        }
        min_val = (min_val > volt_buf[i]) ? volt_buf[i] : min_val;
        max_val = (max_val < volt_buf[i]) ? volt_buf[i] : max_val;
        sum += val;
        valid_count++;
    }
    diff = max_val - min_val;
    avrg_val = (max_val + min_val)/2;
    if (diff > 300)
    {
        filtered_volt = valid_count ? (sum / valid_count) : 5000;
    }
    else
    {
        filtered_volt = ((avrg_val - (diff/2)) < min_val) ? min_val : (avrg_val - (diff/2));
    }
    filtered_volt = (filtered_volt > 5000) ? 5000 : filtered_volt;
    

    // 更新平均历史缓冲
    last_volt_buf[volt_avg_cnt % VOLT_SAMPLE_AVG_TIMES] = filtered_volt;
    volt_avg_cnt++;

    // 再次平均最后 N 次结果
    sum = 0; valid_count = 0;
    for (int i = 0; i < VOLT_SAMPLE_AVG_TIMES; i++) 
    {
        uint16_t val = last_volt_buf[i];
        if (val == 0) 
        {
            continue;
        }
        sum += val;
        valid_count++;
    }
    sys_run.volt = valid_count ? (sum / valid_count) : 5000;

    // 只在电压较高时才进行电压保护判断（防止启动阶段误判）
    if (sys_run.volt > 2500) 
    {
        detect_volt_handle();  // 更新 volt_err_level 和 sys_err_status
    }

    //DBG_PRINTF("sys_run.volt_err_level err:%d\r\n",sys_run.volt_err_level);

    if ((sys_run.sys_err_status != last_err_status) || (last_volt_err_level != sys_run.volt_err_level)) 
    {
        bool is_cmd_update_flag = false;

        // 水浸报警
        if (!(last_err_status & SM_WATER_ERROR) && (sys_run.sys_err_status & SM_WATER_ERROR)) 
        {
            rgb_led_set_prompt(PROMPT_KB_WARNING_WATER);
            is_cmd_update_flag = true;
        }

        // 低压报警
        if (sys_run.sys_err_status & SM_VOLT_LOW_ERROR) 
        {
            if ((!first_volt_warn_flag) || (sys_run.volt_err_level > VOLT_WARN_LEVEL_2)) 
            {
                rgb_led_set_prompt(PROMPT_KB_WARNING_USB_VOLT);
            }

            if (sys_run.volt_err_level >= VOLT_WARN_LEVEL_2) 
            {
                matrix_para_reset();
                interface_release_report();
                matrix_para_reload();
            } 
            else 
            {
                db_sys_set_cfg(common_info.cfg_info.cfg_idx);
                matrix_para_reload();
            }
            if(!first_volt_warn_flag)
            {
                first_volt_warn_flag = true;
                is_cmd_update_flag = true;
            }
        }
        else
        {
            if (last_err_status & SM_VOLT_LOW_ERROR)
            {
                db_sys_set_cfg(common_info.cfg_info.cfg_idx);
                matrix_para_reload();
            }
        }

        if (is_cmd_update_flag || (sys_run.sys_err_status == 0)) 
        {
            cmd_do_response(CMD_NOTIFY, NTF_ERR_INFO, 1, 0, &sys_run.sys_err_status);
            uart_notify_screen_cmd(CMD_UART_KB_WARNING, SCMD_SET_KB_WARNING, &sys_run.sys_err_status, sizeof(sys_run.sys_err_status));
        }

        last_err_status = sys_run.sys_err_status;
        last_volt_err_level = sys_run.volt_err_level;
    }
    return;
}

static void mg_detectiopn(void *pvParameters)
{
    (void)pvParameters;
    sys_run_info_s *sys_run_info = &sys_run;
    static uint32_t volt_up_wait_cnt = 0;
    memset(volt_buf,0,sizeof(volt_buf));
    memset(last_volt_buf,0,sizeof(last_volt_buf));
    const uint64_t now = xTaskGetTickCount();

    while ((xTaskGetTickCount() - now) < pdMS_TO_TICKS(1500)) 
    {
        detecte_data_handle(NULL);
        vTaskDelay(10);
    }

    while(1)
    {
        if (sys_run_info->volt_up_en)
    	{
            if ((volt_up_wait_cnt++) % 5 == 0)
            {
                uint8_t data[3];
                data[0] = (uint8_t)(sys_run_info->volt);
                data[1] = (uint8_t)(sys_run_info->volt >> 8);
                data[2] = sys_run_info->volt_err_level;
                cmd_do_response(CMD_NOTIFY, NTF_VOLT, 3, 0, data);
                DBG_PRINTF("volt :%d,err:%d\r\n",sys_run_info->volt,sys_run_info->sys_err_status);
            }
    	}
        else
        {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }

        vTaskDelay(1000);
    }
}

void mg_detectiopn_init(void)
{
    detecte_ch_init();
    hal_adc_init();
    DBG_PRINTF("mg_detectiopn init!\n");

    if (xTaskCreate(mg_detectiopn, "detecte_thread", STACK_SIZE_DETECTE, NULL, PRIORITY_DETECTE, &m_detecete_thread) != pdPASS)
    {
        DBG_PRINTF("mg_detectiopn creation failed!\n");
    }
}
