/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
#ifndef _MATRIX_CONFG_H_ 
#define _MATRIX_CONFG_H_ 
#include <stdint.h>

#define KB_MOJO68RT                             ( 1 )
#define KB_MADE68PRO                            ( 0 )
#define KB_USE                                  ( KB_MOJO68RT )


#define KEY_SCAN_SWITCH_CH_COUNT (6)//需要切换多少次
#define KEY_SCAN_ADC_ONCE_COUNT (6)//每个ADC 要转换几个通道
#define KEY_SCAN_ADC_BUFF_COUNT (1)
#define KEY_SCAN_ADC_ONCE_TIMES (2)

#define BOARD_KEY_NUM                           ( 79 )
#define KEY_FRAME_NUM                           ( 5 )
#define KEY_FRAME_SIZE                          ( 16 )
#define KEY_FRAME_TOTAL_SIZE                    ( KEY_FRAME_NUM * KEY_FRAME_SIZE )

#define KEYBOARD_CFG_MAX                        ( 4 )
#define KEYBOARD_CFG_NAME_SIZE                  ( 20 )

#define KEYBOARD_NUMBER                         ( BOARD_KEY_NUM )


#define KEY_SROUND_NUM              ( 3 )
#define KEY_SCOUNT_COM_NUM          ( 6 )

#define KEY_SCOUNT_NUM              ( 32 )

/* ********** key default para ********** */

#define KEY_DFT_TOTAL_VAL_SHAFT_TYPE_JW_NMF             ( 10350 )
#define KEY_DFT_TOTAL_VAL_SHAFT_TYPE_G_JADE             ( 15000 )
#define KEY_DFT_TOTAL_VAL_SHAFT_TYPE_G_WHITE            ( 11700 )//1000*0.75
#define KEY_DFT_TOTAL_VAL_SHAFT_TYPE_K_GREEN            ( 10500 )//880*0.75=
#define KEY_DFT_TOTAL_VAL_SHAFT_TYPE_T_WCW              ( 15000 )
#define KEY_DFT_TOTAL_VAL_SHAFT_TYPE_T_JIN              ( 15000 )



#define KEY_DFT_TOP_ADC_VALUE                           ( 2100 )

#define KEY_ADC_VALUE_SWITCH_OUT                        ( 53500 )

#define KEY_DFT_SHAFT                                   ( KEY_SHAFT_TYPE_TTC_FD )

#define KEY_DFT_SHAFT_MAX_LEVEL(shaft)                  ( sw_config_table[shaft].maxLevel )
#define KEY_DFT_TRAVEL_VALUE(shaft)                     ( sw_thd_table[shaft].def_diff )

/* Deadband */
#define KEY_DFT_DEAD_TOP_LEVEL(shaft)                   ( DIST_TO_LEVEL(0.0f,  sw_config_table[shaft].deadAcc) )
#define KEY_DFT_DEAD_BOT_LEVEL(shaft)                   ( DIST_TO_LEVEL(0.04f,  sw_config_table[shaft].deadAcc) )
/* General */
#define KEY_DFT_GENERAL_TRG_PRESS_LEVEL(shaft)          ( DIST_TO_LEVEL(1.5f,  sw_config_table[shaft].generalTrgAcc) )
#define KEY_DFT_GENERAL_TRG_RAISE_OFFSET_LEVEL(shaft)   ( DIST_TO_LEVEL(-0.1f, sw_config_table[shaft].generalTrgAcc) )
/* RT */
#define KEY_DFT_RT_IS_ENABLE                            ( false )
#define KEY_DFT_RT_IS_CONTINUE                          ( false )
#define KEY_DFT_RT_PRESS_SENS_LEVEL(shaft)              ( DIST_TO_LEVEL(0.5f, sw_config_table[shaft].rtSensAcc) )
#define KEY_DFT_RT_RAISE_SENS_LEVEL(shaft)              ( DIST_TO_LEVEL(0.5f, sw_config_table[shaft].rtSensAcc) )
#define KEY_DFT_RT_MODE                                 ( RT_MODE_DEFAULT ) //0: default 1:adaptive
#define KEY_DFT_RT_ADAPTIVE_EXTREME_EN                  ( false )
#define KEY_DFT_RT_ADAPTIVE_MODE                        ( RT_ADA_MODE_DEFAULT )
#define KEY_DFT_RT_ADAPTIVE_SENS                        ( RT_ADA_SENS_STABLE )
/* DKS */
#define KEY_DFT_DKS_PARA_TABLE_MAX                      ( 8 )
#define KEY_DFT_DKS_VIRTUAL_KEY_NUMBER                  ( 8 )
#define KEY_DFT_DKS_VIRTUAL_KEY_TICK_TYPE               ( TICK_STATIC )
#define KEY_DFT_DKS_VIRTUAL_KEY_TICK_MAX                ( 0 )
#define KEY_DFT_DKS_VIRTUAL_KEY_TICK_MIN                ( 0 )
#define KEY_DFT_DKS_OFFSET_DIST                         ( 0.4f )
#define KEY_DFT_DKS_TO_TOP_TRG_LEVEL(shaft)             ( DIST_TO_LEVEL(0.5f,  sw_config_table[shaft].dksAcc) )
#define KEY_DFT_DKS_TO_BOT_TRG_LEVEL(shaft)             ( DIST_TO_LEVEL(KEY_DFT_DKS_OFFSET_DIST,  sw_config_table[shaft].dksAcc) )
#define KEY_DFT_DKS_TRG_RAISE_OFFSET_LEVEL(shaft)       ( DIST_TO_LEVEL(-0.1f, sw_config_table[shaft].dksAcc) )
#define KEY_DFT_DKS_BOT_TRG_LEVEL(shaft)                ( KEY_DFT_SHAFT_MAX_LEVEL(shaft) - KEY_DFT_DKS_TO_BOT_TRG_LEVEL(shaft) )

#define KEY_IS_BIG(ki)                                  ((ki == 28) || (ki == 29) || (ki == 42) || (ki == 43) || (ki == 55) || (ki == 56) || (ki == 67) || (ki == 72))



/* ********** para msg ********** */

#define KEY_ACC_01MM                            ( 0.1f )
#define KEY_ACC_001MM                           ( 0.01f )
#define KEY_ACC_002MM                           ( 0.02f )
#define KEY_ACC_004MM                           ( 0.04f )
#define KEY_ACC_005MM                           ( 0.05f )
#define KEY_ACC_SUPPORT_MIN                     ( KEY_ACC_001MM )

//hive
#define HIVE_TU_F01MM                           ( (1.0f / KEY_ACC_01MM) )
#define HIVE_TU_F001MM                          ( (1.0f / KEY_ACC_001MM) )
#define HIVE_TXDA_01MM(mm)                      ( (uint16_t)(((float)(mm) * HIVE_TU_F01MM) + (KEY_ACC_SUPPORT_MIN * 0.5f)) )
#define HIVE_RXDA_01MM(mm)                      ( (float)(mm) / HIVE_TU_F01MM )
#define HIVE_TXDA_001MM(mm)                     ( (uint16_t)(((float)(mm) * HIVE_TU_F001MM) + (KEY_ACC_SUPPORT_MIN * 0.5f)) )
#define HIVE_RXDA_001MM(mm)                     ( (float)(mm) / HIVE_TU_F001MM )

// conv
#define SYMBOL(x)                               ( (x) < 0 ? -1 : 1 )
#define LEVEL_TO_DIST(level, acc)               ( (float)((float)(level) * (float)(acc)) )
#define DIST_TO_LEVEL(dist, acc)                ( (int16_t)(((float)(dist) / (float)(acc)) + ((float)SYMBOL(dist) * KEY_ACC_SUPPORT_MIN * 0.5f)) )
#define ACC_BETW_CONV(inlevel, inacc, outacc)   ( DIST_TO_LEVEL( LEVEL_TO_DIST(inlevel, inacc), outacc) )

// jade shaft
#define KEY_SHAFT_TYPE_G_JADE_550GS             ( 0 )
#define KEY_SHAFT_TYPE_G_JADE_450GS             ( 1 )

#if (defined CYBER01)
#define KEY_SHAFT_G_JADE_HIGH_ACC_MODE          ( 1 )  //Enable/disable high sensitivity  mode
#elif (defined MADE68)
#define KEY_SHAFT_G_JADE_HIGH_ACC_MODE          ( 1 )
#else
#define KEY_SHAFT_G_JADE_HIGH_ACC_MODE          ( 1 )
#endif


#if (KEY_SHAFT_G_JADE_HIGH_ACC_MODE == 1)
#define KEY_SHAFT_TYPE_G_JADE                   ( KEY_SHAFT_TYPE_G_JADE_450GS )
#else
#define KEY_SHAFT_TYPE_G_JADE                   ( KEY_SHAFT_TYPE_G_JADE_450GS )
#endif


/** jade acc(mm): 
  *         [0.04]: nor:0.1(3.0), high:0.04(0.4)
  *         [0.02]: nor:0.1(3.3), high:0.02(0.1)
  *         [0.01]: nor:0.1(3.3), high:0.01(0.07)
  */
#define KEY_SHAFT_G_JADE_NORMAL_ACC             ( KEY_ACC_01MM )
#define KEY_SHAFT_G_JADE_NORMAL_ACC_TO_DIST     ( 3.3f )
#define KEY_SHAFT_G_JADE_NORMAL_ACC_TO_LEVEL    ( DIST_TO_LEVEL(KEY_SHAFT_G_JADE_NORMAL_ACC_TO_DIST, KEY_SHAFT_G_JADE_NORMAL_ACC) )
#define KEY_SHAFT_G_JADE_HIGH_ACC               ( KEY_ACC_001MM )
#define KEY_SHAFT_G_JADE_HIGH_ACC_TO_DIST       ( 0.1f )
#define KEY_SHAFT_G_JADE_HIGH_ACC_TO_LEVEL      ( DIST_TO_LEVEL(KEY_SHAFT_G_JADE_HIGH_ACC_TO_DIST, KEY_SHAFT_G_JADE_HIGH_ACC) )

/** nmf acc(mm): 
  *         [0.04]: nor:0.1(3.5), high:0.04(0.2)
  *         [0.02]: nor:0.1(3.6), high:0.02(0.1)
  *         [0.01]: nor:0.1(3.6), high:0.01(0.07)
  */
#define KEY_SHAFT_JW_NMF_NORMAL_ACC             ( KEY_ACC_01MM )
#define KEY_SHAFT_JW_NMF_NORMAL_ACC_TO_DIST     ( 3.5f )
#define KEY_SHAFT_JW_NMF_NORMAL_ACC_TO_LEVEL    ( DIST_TO_LEVEL(KEY_SHAFT_JW_NMF_NORMAL_ACC_TO_DIST, KEY_SHAFT_JW_NMF_NORMAL_ACC) )
#define KEY_SHAFT_JW_NMF_HIGH_ACC               ( KEY_ACC_004MM )
#define KEY_SHAFT_JW_NMF_HIGH_ACC_TO_DIST       ( 0.2f )
#define KEY_SHAFT_JW_NMF_HIGH_ACC_TO_LEVEL      ( DIST_TO_LEVEL(KEY_SHAFT_JW_NMF_HIGH_ACC_TO_DIST, KEY_SHAFT_JW_NMF_HIGH_ACC) )


/** ttc wcw(mm): 
  *         [0.04]: nor:0.1(3.0), high:0.04(0.4)
  *         [0.02]: nor:0.1(3.3), high:0.02(0.1)
  *         [0.01]: nor:0.1(3.3), high:0.01(0.07)
  */
#define KEY_SHAFT_TTC_WCW_NORMAL_ACC             ( KEY_ACC_01MM )
#define KEY_SHAFT_TTC_WCW_NORMAL_ACC_TO_DIST     ( 3.2f )
#define KEY_SHAFT_TTC_WCW_NORMAL_ACC_TO_LEVEL    ( DIST_TO_LEVEL(KEY_SHAFT_TTC_WCW_NORMAL_ACC_TO_DIST, KEY_SHAFT_TTC_WCW_NORMAL_ACC) )
#define KEY_SHAFT_TTC_WCW_HIGH_ACC               ( KEY_ACC_001MM )
#define KEY_SHAFT_TTC_WCW_HIGH_ACC_TO_DIST       ( 0.13f )
#define KEY_SHAFT_TTC_WCW_HIGH_ACC_TO_LEVEL      ( DIST_TO_LEVEL(KEY_SHAFT_TTC_WCW_HIGH_ACC_TO_DIST, KEY_SHAFT_TTC_WCW_HIGH_ACC) )

/** ttc tw(mm): 
  *         [0.04]: nor:0.1(3.0), high:0.04(0.4)
  *         [0.02]: nor:0.1(3.3), high:0.02(0.1)
  *         [0.01]: nor:0.1(3.3), high:0.01(0.07)
  */
#define KEY_SHAFT_TTC_TW_NORMAL_ACC             ( KEY_ACC_01MM )
#define KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_DIST     ( 3.2f )
#define KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_LEVEL    ( DIST_TO_LEVEL(KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_DIST, KEY_SHAFT_TTC_TW_NORMAL_ACC) )
#define KEY_SHAFT_TTC_TW_HIGH_ACC               ( KEY_ACC_001MM )
#define KEY_SHAFT_TTC_TW_HIGH_ACC_TO_DIST       ( 0.2f )
#define KEY_SHAFT_TTC_TW_HIGH_ACC_TO_LEVEL      ( DIST_TO_LEVEL(KEY_SHAFT_TTC_TW_HIGH_ACC_TO_DIST, KEY_SHAFT_TTC_TW_HIGH_ACC) )

/** ttc tw(mm): 
  *         [0.04]: nor:0.1(3.0), high:0.04(0.4)
  *         [0.02]: nor:0.1(3.3), high:0.02(0.1)
  *         [0.01]: nor:0.1(3.3), high:0.01(0.07)
  */
#define KEY_SHAFT_TTC_TA_NORMAL_ACC             ( KEY_ACC_01MM )
#define KEY_SHAFT_TTC_TA_NORMAL_ACC_TO_DIST     ( 3.2f )
#define KEY_SHAFT_TTC_TA_NORMAL_ACC_TO_LEVEL    ( DIST_TO_LEVEL(KEY_SHAFT_TTC_TA_NORMAL_ACC_TO_DIST, KEY_SHAFT_TTC_TA_NORMAL_ACC) )
#define KEY_SHAFT_TTC_TA_HIGH_ACC               ( KEY_ACC_001MM )
#define KEY_SHAFT_TTC_TA_HIGH_ACC_TO_DIST       ( 0.13f )
#define KEY_SHAFT_TTC_TA_HIGH_ACC_TO_LEVEL      ( DIST_TO_LEVEL(KEY_SHAFT_TTC_TA_HIGH_ACC_TO_DIST, KEY_SHAFT_TTC_TA_HIGH_ACC) )
/** ttc lg(mm): 
  *         //[0.04]: nor:0.1(3.0), high:0.04(0.4)
  *         //[0.02]: nor:0.1(3.3), high:0.02(0.1)
  *         //[0.01]: nor:0.1(3.3), high:0.01(0.07)
  */
#define KEY_SHAFT_TTC_LG_NORMAL_ACC             ( KEY_ACC_01MM )
#define KEY_SHAFT_TTC_LG_NORMAL_ACC_TO_DIST     ( 3.3f )
#define KEY_SHAFT_TTC_LG_NORMAL_ACC_TO_LEVEL    ( DIST_TO_LEVEL(KEY_SHAFT_TTC_LG_NORMAL_ACC_TO_DIST, KEY_SHAFT_TTC_LG_NORMAL_ACC) )
#define KEY_SHAFT_TTC_LG_HIGH_ACC               ( KEY_ACC_001MM )
#define KEY_SHAFT_TTC_LG_HIGH_ACC_TO_DIST       ( 0.15f )
#define KEY_SHAFT_TTC_LG_HIGH_ACC_TO_LEVEL      ( DIST_TO_LEVEL(KEY_SHAFT_TTC_LG_HIGH_ACC_TO_DIST, KEY_SHAFT_TTC_LG_HIGH_ACC) )

/** WuqueStudio babao(mm): 
  *         [0.04]: nor:0.1(3.0), high:0.04(0.4)
  *         [0.02]: nor:0.1(3.3), high:0.02(0.1)
  *         [0.01]: nor:0.1(3.3), high:0.01(0.07)
  */
#define KEY_SHAFT_WS_BABAO_NORMAL_ACC             ( KEY_ACC_01MM )
#define KEY_SHAFT_WS_BABAO_NORMAL_ACC_TO_DIST     ( 3.4f )
#define KEY_SHAFT_WS_BABAO_NORMAL_ACC_TO_LEVEL    ( DIST_TO_LEVEL(KEY_SHAFT_WS_BABAO_NORMAL_ACC_TO_DIST, KEY_SHAFT_WS_BABAO_NORMAL_ACC) )
#define KEY_SHAFT_WS_BABAO_HIGH_ACC               ( KEY_ACC_001MM )
#define KEY_SHAFT_WS_BABAO_HIGH_ACC_TO_DIST       ( 0.15f )
#define KEY_SHAFT_WS_BABAO_HIGH_ACC_TO_LEVEL      ( DIST_TO_LEVEL(KEY_SHAFT_WS_BABAO_HIGH_ACC_TO_DIST, KEY_SHAFT_WS_BABAO_HIGH_ACC) )

/** huahuo acc(mm):   
  *         [0.04]: nor:0.1(3.0), high:0.04(0.4)
  *         [0.02]: nor:0.1(3.3), high:0.02(0.1)
  *         [0.01]: nor:0.1(3.3), high:0.01(0.07)
  */
#define KEY_SHAFT_G_HUAHUO_NORMAL_ACC             ( KEY_ACC_01MM )
#define KEY_SHAFT_G_HUAHUO_NORMAL_ACC_TO_DIST     ( 3.1f )
#define KEY_SHAFT_G_HUAHUO_NORMAL_ACC_TO_LEVEL    ( DIST_TO_LEVEL(KEY_SHAFT_G_HUAHUO_NORMAL_ACC_TO_DIST, KEY_SHAFT_G_HUAHUO_NORMAL_ACC) )
#define KEY_SHAFT_G_HUAHUO_HIGH_ACC               ( KEY_ACC_001MM )
#define KEY_SHAFT_G_HUAHUO_HIGH_ACC_TO_DIST       ( 0.07f )
#define KEY_SHAFT_G_HUAHUO_HIGH_ACC_TO_LEVEL      ( DIST_TO_LEVEL(KEY_SHAFT_G_HUAHUO_HIGH_ACC_TO_DIST, KEY_SHAFT_G_HUAHUO_HIGH_ACC) )

#define KEY_LVL_MODE_BUFF                       (0)
#define KEY_LVL_MODE_REALTIME                   (1)
#define USE_KEY_LVL_MODE                        KEY_LVL_MODE_BUFF

/* level boundary value buffer size */
#if (USE_KEY_LVL_MODE == KEY_LVL_MODE_BUFF)
#define KEY_LEVEL_BOUNDARY_BUFF_SIZE            ( 54 )
#define KEY_LVL_GET_VAL(kid, lvl)               ( kh.p_st[kid].levelBoundaryValue[lvl] )
#elif (USE_KEY_LVL_MODE == KEY_LVL_MODE_REALTIME)
#define KEY_LEVEL_BOUNDARY_BUFF_SIZE            ( 200 )
#define KEY_LVL_GET_VAL(kid, lvl)               ( (uint16_t)((float)key_st[kid].max - ((float)key_st[kid].diff * kcr_table[key_cali.sw[kid]][lvl])) )
#endif
/* acc */
#define KEY_SUPT_HACC(kid)                      ( sw_config_table[key_cali.sw[kid]].isRtHighAcc )
#define KEY_NACC(kid)                           ( sw_config_table[key_cali.sw[kid]].nacc )
#define KEY_NACC_TO_DIST(kid)                   ( sw_config_table[key_cali.sw[kid]].nacc2dist )
#define KEY_NACC_TO_LVL(kid)                    ( sw_config_table[key_cali.sw[kid]].nacc2lvl )
#define KEY_HACC(kid)                           ( sw_config_table[key_cali.sw[kid]].hacc )
#define KEY_HACC_TO_DIST(kid)                   ( sw_config_table[key_cali.sw[kid]].hacc2dist )
#define KEY_HACC_TO_LVL(kid)                    ( sw_config_table[key_cali.sw[kid]].hacc2lvl )
/* Travel */
#define KEY_REAL_TRAVEL(kid)                    ( sw_config_table[key_cali.sw[kid]].realTravel )
/* Level */
#define KEY_MAX_LEVEL(kid)                      ( sw_config_table[key_cali.sw[kid]].maxLevel )
/* Deadband */
#define KEY_DEADBAND_ACC(kid)                   ( sw_config_table[key_cali.sw[kid]].deadAcc )
#define KEY_DEADBAND_MIN(kid)                   ( sw_config_table[key_cali.sw[kid]].deadMin )
#define KEY_DEADBAND_MAX(kid)                   ( sw_config_table[key_cali.sw[kid]].deadMax )
/* General */
#define KEY_GENERAL_TRG_ACC(kid)                ( sw_config_table[key_cali.sw[kid]].generalTrgAcc )
// #define KEY_GENERAL_TRG_MIN(kid)                ( sw_config_table[key_cali.sw[kid]].generalTrgMin )
// #define KEY_GENERAL_TRG_MAX(kid)                ( sw_config_table[key_cali.sw[kid]].generalTrgMax )
#define KEY_GENERAL_TRG_MIN(kid)                ( KEY_GENERAL_TRG_ACC(kid) )
#define KEY_GENERAL_TRG_MAX(kid)                ( KEY_REAL_TRAVEL(kid) )
/* RT */
#define KEY_RT_ACC(kid)                         ( sw_config_table[key_cali.sw[kid]].rtSensAcc )
#define KEY_RT_SENS_MIN(kid)                    ( sw_config_table[key_cali.sw[kid]].rtSensMin )
#define KEY_RT_SENS_MAX(kid)                    ( sw_config_table[key_cali.sw[kid]].rtSensMax )
/* DKS */
#define KEY_DKS_TRG_ACC(kid)                    ( sw_config_table[key_cali.sw[kid]].dksAcc )
#define KEY_DKS_TO_TOP_MIN(kid)                 ( sw_config_table[key_cali.sw[kid]].dksToTopMin )
#define KEY_DKS_TO_TOP_MAX(kid)                 ( sw_config_table[key_cali.sw[kid]].dksToTopMax )
#define KEY_DKS_TO_BOT_MIN(kid)                 ( sw_config_table[key_cali.sw[kid]].dksToBotMin )
#define KEY_DKS_TO_BOT_MAX(kid)                 ( sw_config_table[key_cali.sw[kid]].dksToBotMax )

/* top and bottom stroke reserver */
#define KEY_SPARE_STROKE(kid)                   ( 0 )
#define KEY_TOP_RESERVER(kid)                   ( (uint16_t)((float)key_st[kid].diff * sw_config_table[key_cali.sw[kid]].curvTab[0] * 0.0f) )
#define KEY_BOT_RESERVER(kid)                   ( (uint16_t)((float)key_st[kid].diff * sw_config_table[key_cali.sw[kid]].curvTab[sw_config_table[key_cali.sw[kid]].maxLevel-1]) )
/* top and bottom wave range */
#define KEY_TOP_WAVE(kid)                       ( (uint16_t)((float)key_st[kid].diff * sw_config_table[p_calib->sw[kid]].curvTab[0] * 0.5f) )
//#if (defined CYBER01)
#define KEY_BOT_WAVE(kid)                       ( (uint16_t)((float)key_st[kid].diff * sw_config_table[p_calib->sw[kid]].curvTab[0] * 1.2f) )
//#elif (defined MADE68)
//#define KEY_BOT_WAVE(kid)                       ( (uint16_t)((float)key_cali.travel[kid] * sw_config_table[key_cali.sw[kid]].curvTab[0] * 1.1f) )
//#endif

/*  */
#define KEY_SHAPE_CHANGE_VAL      ( 3 )
#define KEY_BIG_SHAPE_CHANGE_VAL  ( 3 )
#define IMPACT_BETW_KEY           ( 30 )

#define KEY_MS_TO_TICK(ms)          ((uint32_t)((ms) * 8))

#define KEY_DRT_TICK_EN             (1)
#define KEY_DRT_OVER_TICK           KEY_MS_TO_TICK(180) //识别长按时间
#define KEY_DRT_PRESS_TICK          KEY_MS_TO_TICK(0.5)
#define KEY_DRT_RAISE_TICK          (KEY_DRT_PRESS_TICK + KEY_MS_TO_TICK(5)) //短按反向输出
#define KEY_DRT_RAND_TICK           KEY_MS_TO_TICK(5)
#define KEY_DRT_LONG_RAISE_TICK     (KEY_DRT_PRESS_TICK + KEY_MS_TO_TICK(55)) //长按反向输出
#define KEY_DRT_LONG_RAND_TICK      KEY_MS_TO_TICK(30)

#define KEY_SOCD_TICK_EN            (0)
#define KEY_SOCD_TICK               KEY_MS_TO_TICK(20)
#define KEY_SOCD_RAND_TICK          KEY_MS_TO_TICK(10)

#define KEY_DEBOUNCE_TICK_MAX       KEY_MS_TO_TICK(100)
#define KEY_FIX_DEBOUNCE_TICK       KEY_MS_TO_TICK(20)
#define KEY_RT_UP_DEBOUNCE_MS       (0)
#define KEY_RT_DOWN_DEBOUNCE_MS     (0)//(480000 * 5)
#define KEY_RT_BIG_DEBOUNCE_MS      (480000 * 60)
#define KEY_RT_D2U_STABBLE_US       (30000)
#define KEY_RT_BIG_D2U_STABBLE_US   (100000)

#define KEY_RT_FIRST_D2U_US         (30000)
#define KEY_RT_BIG_FIRST_D2U_US     (60000)

#define KEY_RT_B2T_US               (30000)
#define KEY_RT_BIG_B2T_US           (100000)

#define KEY_DEBOUNCE_VAL            2621 //100mV
#define KEY_CALI_NOISE_TIME         KEY_MS_TO_TICK(3000)

#define KEY_SPD_BUF_SIZE            (1000)

/* top and bottom stroke reserver */
#define KC_HALL_POLAR_P 0//按键往下按,adc值减小
#define KC_HALL_POLAR_N 1//按键往下按,adc值增大

#define KC_HALL_POLAR_TYPE KC_HALL_POLAR_P

//tuning para
#define KEY_TUNING_EN           (0)
#define KEY_TUNING_ZERO_EN      (1)

#define KS_TUNING_BUFFER_SIZE   (5)
#define KS_TUNING_THD_STATIC    (6)
#define KS_TUNING_STATIC_TIMES  (5)
#define KS_TUNING_BUFFER_MIN_SIZE (5)

#define KS_TUNING_THD_CALI      (3)
#define KS_TUNING_THD_MAX       (10)


#define KEY_TUNING_DEBUG        (0)

//底部值最大容差
#define KS_TUNING_TOR_MAX       (0.07)
#define KS_SCAN_RATE            (8000)
#define KS_TUNING_TMIE_PER_SEC  (55)
#define KS_TUNING_INTER         (8000/KS_TUNING_TMIE_PER_SEC)//333/6=55次
#define KS_TUNING_RELOAD_TMIE_SEC(sec) ((sec) * KS_TUNING_TMIE_PER_SEC)


/* hardware ctrl default */
#define KEYSCAN_DEFAULT_HW_UART_BAUD_RATE           ( KEYSCAN_HW_CTRL_UART_BAUDRATE_115200 )
#define KEYSCAN_DEFAULT_HW_UART_DATA_TARANSMIT_MODE ( KEYSCAN_HW_CTRL_UART_DATA_TRANSMIT_MODE_LSB )
#define KEYSCAN_DEFAULT_HW_UART_IS_OPEN_TX_DMA      ( true )

#define KEYSCAN_DEFAULT_HW_SPI_TRANSMIT_RATE        ( KEYSCAN_HW_CTRL_SPI_TRANSMIT_RATE_7500K )
#define KEYSCAN_DEFAULT_HW_SPI_DATA_TARANSMIT_MODE  ( KEYSCAN_HW_CTRL_SPI_DATA_TRANSMIT_MODE_LSB )
#define KEYSCAN_DEFAULT_HW_SPI_DATA_TARANSMIT_SIZE  ( KEYSCAN_HW_CTRL_SPI_DATA_TRANSMIT_SIZE_8BIT )
#define KEYSCAN_DEFAULT_HW_SPI_CLK_POLARITY_PHASE   ( KEYSCAN_HW_CTRL_SPI_PL_LOW_PH_2EDGE )
#define KEYSCAN_DEFAULT_HW_SPI_IS_OPEN_TX_DMA       ( true )





//extern const uint8_t keyboardTableMcuToHostRound[KEY_SROUND_NUM][KEY_SCOUNT_NUM];
extern const uint8_t keyboardTableMcuToHost[];
extern const int8_t key_idx_table[KEY_FRAME_NUM*KEY_FRAME_SIZE];
extern const int8_t key_frame1[36];
extern const int8_t key_frame2[36];
extern const int8_t key_frame_table_full[];
extern const int8_t key_amend_table[KEY_FRAME_NUM*KEY_FRAME_SIZE];
extern const int16_t kc_to_ki[256];


void kcr_table_update(void);





#endif

