/*
 * Copyright (c) 2024-2024 Melgeek, All Rights Reserved. 
 * 
 * Description: 
 * 
 */
 
#include "mg_matrix_config.h"
#include "mg_matrix_type.h"




const key_sw_para_t sw_config_table[KEY_SHAFT_TYPE_MAX] = 
{
    /* BASE: hall-MH481, switch-74HC4067(nexperia) */
    
    // KEY_SHAFT_TYPE_UNKNOWN 
    {
        .realTravel        = 0,
        .nacc              = 0,
        .nacc2dist         = 0,
        .nacc2lvl          = 0,
        .hacc              = 0,
        .hacc2dist         = 0,
        .hacc2lvl          = 0,
        .generalTrgAcc     = 0,
        .rtSensAcc         = 0,
        .deadAcc           = 0,
        .dksAcc            = 0,
        .maxLevel          = 0,
        .isRtHighAcc       = 0,
        .rtSensMin         = 0,
        .rtSensMax         = 0,
        .deadMin           = 0,
        .deadMax           = 0,
        .dksToTopMin       = 0,
        .dksToTopMax       = 0,
        .dksToBotMin       = 0,
        .dksToBotMax       = 0,
        .voltAscend        = false,
        .curvTab = {
            0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 
            0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 
            0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 
            0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 
        },
    },
    // KEY_SHAFT_TYPE_GATERON_WHITE 
    {
        .voltAscend        = false,
        .isRtHighAcc       = false,
        
        .nacc              = 0,
        .nacc2dist         = 0,
        .nacc2lvl          = 0,
        .hacc              = 0,
        .hacc2dist         = 0,
        .hacc2lvl          = 0,
        .realTravel        = 4.0f,
        .maxLevel          = DIST_TO_LEVEL(4.0f, KEY_ACC_01MM),
        
        .generalTrgAcc     = KEY_ACC_01MM,
        
        .deadAcc           = KEY_ACC_01MM,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_ACC_01MM),

        .rtSensAcc         = KEY_ACC_01MM,
        .rtSensMin         = KEY_ACC_01MM,
        .rtSensMax         = 2.5f,
        
        .dksAcc            = KEY_ACC_01MM,
        .dksToTopMin       = KEY_ACC_01MM,
        .dksToTopMax       = 4.0f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_ACC_01MM,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.005749f, 0.005749f, 0.005749f, 0.006288f, 0.006288f, 0.006288f, 0.008085f, 0.008085f, 0.008085f, 0.008983f,
                0.008983f, 0.008983f, 0.010780f, 0.010780f, 0.010780f, 0.012576f, 0.012576f, 0.013475f, 0.015271f, 0.016170f,
                0.017068f, 0.017966f, 0.019763f, 0.019763f, 0.023356f, 0.025153f, 0.027848f, 0.028746f, 0.032339f, 0.033238f,
                0.035932f, 0.040424f, 0.040424f, 0.045814f, 0.046712f, 0.051204f, 0.056594f, 0.064678f, 0.069170f, 0.070068f,
        },
    },
    // KEY_SHAFT_TYPE_GATERON_JADE
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_G_JADE_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_G_JADE_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_G_JADE_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_G_JADE_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_G_JADE_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_G_JADE_HIGH_ACC_TO_LEVEL,
        .realTravel        = 3.4f,
        .maxLevel          = KEY_SHAFT_G_JADE_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_G_JADE_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_G_JADE_NORMAL_ACC,
        
        .deadAcc           = KEY_SHAFT_G_JADE_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_G_JADE_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_G_JADE_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_G_JADE_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_G_JADE_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_G_JADE_NORMAL_ACC,
        .dksToTopMax       = 3.4f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_G_JADE_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.005488837f, 0.007753774f, 0.008457170f, 0.008954237f, 0.009446615f, 0.009997608f, 0.010635355f, 0.011322338f, 0.012225031f, 0.012888568f,
                0.014021036f, 0.014618923f, 0.015793595f, 0.016937787f, 0.018241415f, 0.019364505f, 0.021073758f, 0.022740808f, 0.024145256f, 0.026135868f,
                0.027960010f, 0.030567266f, 0.033315201f, 0.035493386f, 0.038044370f, 0.041807541f, 0.045544921f, 0.049533179f, 0.054360824f, 0.059108750f,
                0.063730065f, 0.070487360f, 0.075727664f,
                0.008203947f, 0.008030443f, 0.008178156f, 0.008696325f, 0.008482961f, 0.008337593f, 0.008738529f, 0.009270765f, 0.008614262f, 0.007523998f,  
        },
    },
    // KEY_SHAFT_TYPE_KAILH_GREEN
    {
        .voltAscend        = false,
        .isRtHighAcc       = false,

        .nacc              = 0,
        .nacc2dist         = 0,
        .nacc2lvl          = 0,
        .hacc              = 0,
        .hacc2dist         = 0,
        .hacc2lvl          = 0,
        .realTravel        = 3.6f,
        .maxLevel          = DIST_TO_LEVEL(3.6f, KEY_ACC_01MM),

        .generalTrgAcc     = KEY_ACC_01MM,
        
        .deadAcc           = KEY_ACC_01MM,
        .deadMin           = 0.0f,
        .deadMax           = LEVEL_TO_DIST(7, KEY_ACC_01MM),

        .rtSensAcc         = KEY_ACC_01MM,
        .rtSensMin         = KEY_ACC_01MM,
        .rtSensMax         = 2.5f,

        .dksAcc            = KEY_ACC_01MM,
        .dksToTopMin       = KEY_ACC_01MM,
        .dksToTopMax       = 3.6f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_ACC_01MM,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.005992f, 0.007050f, 0.007050f, 0.008248f, 0.008399f, 0.009437f, 0.009517f, 0.009517f, 0.009870f, 0.010575f, 
                0.011280f, 0.012337f, 0.013394f, 0.014099f, 0.014804f, 0.016567f, 0.017977f, 0.019034f, 0.020797f, 0.022207f, 
                0.024641f, 0.026276f, 0.029449f, 0.030254f, 0.032136f, 0.037011f, 0.037011f, 0.039891f, 0.044001f, 0.048836f, 
                0.051975f, 0.056398f, 0.063447f, 0.066972f, 0.074022f, 0.077194f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 
        },
    },
    // KEY_SHAFT_TYPE_JW_NMF
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_JW_NMF_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_JW_NMF_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_JW_NMF_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_JW_NMF_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_JW_NMF_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_JW_NMF_HIGH_ACC_TO_LEVEL,
        .realTravel        = 3.7f,
        .maxLevel          = KEY_SHAFT_JW_NMF_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_JW_NMF_HIGH_ACC_TO_LEVEL,

        .generalTrgAcc     = KEY_SHAFT_JW_NMF_NORMAL_ACC,

        .deadAcc           = KEY_SHAFT_JW_NMF_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(5, KEY_SHAFT_JW_NMF_HIGH_ACC),

        .rtSensAcc         = KEY_SHAFT_JW_NMF_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_JW_NMF_HIGH_ACC,
        .rtSensMax         = 2.4f,

        .dksAcc            = KEY_SHAFT_JW_NMF_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_JW_NMF_NORMAL_ACC,
        .dksToTopMax       = 3.7f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_JW_NMF_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.005688f, 0.007417f, 0.007753f, 0.008769f, 0.008826f, 0.009003f, 0.010110f, 0.010137f, 0.010188f, 0.011017f, 
                0.012189f, 0.012422f, 0.013526f, 0.013645f, 0.014807f, 0.015816f, 0.015999f, 0.018972f, 0.020260f, 0.022003f, 
                0.023967f, 0.024615f, 0.026952f, 0.029108f, 0.033125f, 0.034042f, 0.037028f, 0.039350f, 0.040933f, 0.044343f, 
                0.047026f, 0.053566f, 0.055389f, 0.059343f, 0.065635f, 
                0.025778f, 0.026573f, 0.028084f, 0.027851f, 0.028745f, 
        },
    },
    // KEY_SHAFT_TYPE_TTC_WCW
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_TTC_WCW_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_TTC_WCW_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_TTC_WCW_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_TTC_WCW_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_TTC_WCW_HIGH_ACC_TO_LEVEL,
        .realTravel        = 3.3f,
        .maxLevel          = KEY_SHAFT_TTC_WCW_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_TTC_WCW_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
            
        .deadAcc           = KEY_SHAFT_TTC_WCW_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_TTC_WCW_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_TTC_WCW_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_TTC_WCW_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
        .dksToTopMax       = 3.3f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.00683431f, 0.00871670f, 0.00918238f, 0.00945129f, 0.01097294f, 0.01159603f, 0.01170097f, 0.01331444f, 0.01379980f, 0.01490168f, 
                0.01583304f, 0.01692836f, 0.01762360f, 0.01944040f, 0.02001758f, 0.02096205f, 0.02372988f, 0.02557947f, 0.02592709f, 0.02844569f, 
                0.03110858f, 0.03346320f, 0.03400758f, 0.03629006f, 0.03970721f, 0.04254063f, 0.04658088f, 0.05290360f, 0.05252974f, 0.06358795f, 
                0.06101688f, 0.07162253f, 
                0.00665066f, 0.00771320f, 0.00714913f, 0.00721472f, 0.00673593f, 0.00771975f, 0.00786405f, 0.00819199f, 0.00821167f, 0.00841499f, 
                0.00915614f, 0.00956279f, 0.00966117f, 
        },
    },
    // KEY_SHAFT_TYPE_GATERON_JPRO
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_G_JADE_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_G_JADE_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_G_JADE_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_G_JADE_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_G_JADE_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_G_JADE_HIGH_ACC_TO_LEVEL,
        .realTravel        = 3.4f,
        .maxLevel          = KEY_SHAFT_G_JADE_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_G_JADE_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_G_JADE_NORMAL_ACC,
        
        .deadAcc           = KEY_SHAFT_G_JADE_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_G_JADE_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_G_JADE_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_G_JADE_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_G_JADE_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_G_JADE_NORMAL_ACC,
        .dksToTopMax       = 3.4f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_G_JADE_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.005488837f, 0.007753774f, 0.008457170f, 0.008954237f, 0.009446615f, 0.009997608f, 0.010635355f, 0.011322338f, 0.012225031f, 0.012888568f,
                0.014021036f, 0.014618923f, 0.015793595f, 0.016937787f, 0.018241415f, 0.019364505f, 0.021073758f, 0.022740808f, 0.024145256f, 0.026135868f,
                0.027960010f, 0.030567266f, 0.033315201f, 0.035493386f, 0.038044370f, 0.041807541f, 0.045544921f, 0.049533179f, 0.054360824f, 0.059108750f,
                0.063730065f, 0.070487360f, 0.075727664f,
                0.008203947f, 0.008030443f, 0.008178156f, 0.008696325f, 0.008482961f, 0.008337593f, 0.008738529f, 0.009270765f, 0.008614262f, 0.007523998f,  
        },
    },
    // KEY_SHAFT_TYPE_GATERON_JMAX
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_G_JADE_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_G_JADE_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_G_JADE_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_G_JADE_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_G_JADE_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_G_JADE_HIGH_ACC_TO_LEVEL,
        .realTravel        = 3.4f,
        .maxLevel          = KEY_SHAFT_G_JADE_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_G_JADE_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_G_JADE_NORMAL_ACC,
        
        .deadAcc           = KEY_SHAFT_G_JADE_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_G_JADE_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_G_JADE_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_G_JADE_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_G_JADE_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_G_JADE_NORMAL_ACC,
        .dksToTopMax       = 3.4f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_G_JADE_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.005488837f, 0.007753774f, 0.008457170f, 0.008954237f, 0.009446615f, 0.009997608f, 0.010635355f, 0.011322338f, 0.012225031f, 0.012888568f,
                0.014021036f, 0.014618923f, 0.015793595f, 0.016937787f, 0.018241415f, 0.019364505f, 0.021073758f, 0.022740808f, 0.024145256f, 0.026135868f,
                0.027960010f, 0.030567266f, 0.033315201f, 0.035493386f, 0.038044370f, 0.041807541f, 0.045544921f, 0.049533179f, 0.054360824f, 0.059108750f,
                0.063730065f, 0.070487360f, 0.075727664f,
                0.008203947f, 0.008030443f, 0.008178156f, 0.008696325f, 0.008482961f, 0.008337593f, 0.008738529f, 0.009270765f, 0.008614262f, 0.007523998f, 
        },
    },
    // KEY_SHAFT_TYPE_GATERON_JGAMING
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_G_JADE_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_G_JADE_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_G_JADE_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_G_JADE_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_G_JADE_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_G_JADE_HIGH_ACC_TO_LEVEL,
        .realTravel        = 3.4f,
        .maxLevel          = KEY_SHAFT_G_JADE_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_G_JADE_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_G_JADE_NORMAL_ACC,
        
        .deadAcc           = KEY_SHAFT_G_JADE_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_G_JADE_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_G_JADE_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_G_JADE_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_G_JADE_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_G_JADE_NORMAL_ACC,
        .dksToTopMax       = 3.4f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_G_JADE_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.005488837f, 0.007753774f, 0.008457170f, 0.008954237f, 0.009446615f, 0.009997608f, 0.010635355f, 0.011322338f, 0.012225031f, 0.012888568f,
                0.014021036f, 0.014618923f, 0.015793595f, 0.016937787f, 0.018241415f, 0.019364505f, 0.021073758f, 0.022740808f, 0.024145256f, 0.026135868f,
                0.027960010f, 0.030567266f, 0.033315201f, 0.035493386f, 0.038044370f, 0.041807541f, 0.045544921f, 0.049533179f, 0.054360824f, 0.059108750f,
                0.063730065f, 0.070487360f, 0.075727664f,
                0.008203947f, 0.008030443f, 0.008178156f, 0.008696325f, 0.008482961f, 0.008337593f, 0.008738529f, 0.009270765f, 0.008614262f, 0.007523998f,  
        },
    },
    // KEY_SHAFT_TYPE_TTC_WMIN,
    {
        .realTravel        = 0,
    },
    // KEY_SHAFT_TYPE_TTC_WRGB,
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_TTC_WCW_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_TTC_WCW_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_TTC_WCW_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_TTC_WCW_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_TTC_WCW_HIGH_ACC_TO_LEVEL,
        .realTravel        = 3.3f,
        .maxLevel          = KEY_SHAFT_TTC_WCW_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_TTC_WCW_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
            
        .deadAcc           = KEY_SHAFT_TTC_WCW_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_TTC_WCW_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_TTC_WCW_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_TTC_WCW_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
        .dksToTopMax       = 3.3f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.00683431f, 0.00871670f, 0.00918238f, 0.00945129f, 0.01097294f, 0.01159603f, 0.01170097f, 0.01331444f, 0.01379980f, 0.01490168f, 
                0.01583304f, 0.01692836f, 0.01762360f, 0.01944040f, 0.02001758f, 0.02096205f, 0.02372988f, 0.02557947f, 0.02592709f, 0.02844569f, 
                0.03110858f, 0.03346320f, 0.03400758f, 0.03629006f, 0.03970721f, 0.04254063f, 0.04658088f, 0.05290360f, 0.05252974f, 0.06358795f, 
                0.06101688f, 0.07162253f, 
                0.00665066f, 0.00771320f, 0.00714913f, 0.00721472f, 0.00673593f, 0.00771975f, 0.00786405f, 0.00819199f, 0.00821167f, 0.00841499f, 
                0.00915614f, 0.00956279f, 0.00966117f, 
        },
    },
    // KEY_SHAFT_TYPE_TTC_TSTD,
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_TTC_TW_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_TTC_TW_HIGH_ACC_TO_LEVEL,
        .realTravel        = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_DIST + KEY_SHAFT_TTC_TW_HIGH_ACC_TO_DIST,
        .maxLevel          = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_TTC_TW_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_TTC_TW_NORMAL_ACC,
            
        .deadAcc           = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_TTC_TW_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToTopMax       = (KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_DIST + KEY_SHAFT_TTC_TW_HIGH_ACC_TO_DIST) - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.00605842f, 0.00823728f, 0.00867735f, 0.00893147f, 0.01036943f, 0.01095825f, 0.01105742f, 0.01258215f, 0.01304081f, 0.01408209f, 
                0.01496222f, 0.01599730f, 0.01665430f, 0.01837118f, 0.01891661f, 0.01980914f, 0.02242474f, 0.02417260f, 0.02450110f, 0.02688118f, 
                0.02939761f, 0.03162272f, 0.03213716f, 0.03429411f, 0.03752331f, 0.04020090f, 0.04401893f, 0.04999390f, 0.04964060f, 0.06009061f, 
                0.05766095f, 0.06768329f,
                0.00628487f, 0.00728897f, 0.00675593f, 0.00681791f, 0.00636545f, 0.00729516f, 0.00743153f, 0.00774143f, 0.00785453f, 0.00795217f, 
                0.00808555f, 0.00809184f, 0.00813278f, 0.00813278f, 0.00822728f, 0.00822728f, 0.00832178f, 0.00841161f, 0.00846831f, 0.00856281f, 
        },
    },
    // KEY_SHAFT_TYPE_TTC_TSE,
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_TTC_TW_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_TTC_TW_HIGH_ACC_TO_LEVEL,
        .realTravel        = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_DIST + KEY_SHAFT_TTC_TW_HIGH_ACC_TO_DIST,
        .maxLevel          = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_TTC_TW_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_TTC_TW_NORMAL_ACC,
            
        .deadAcc           = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_TTC_TW_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToTopMax       = (KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_DIST + KEY_SHAFT_TTC_TW_HIGH_ACC_TO_DIST) - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.00605842f, 0.00823728f, 0.00867735f, 0.00893147f, 0.01036943f, 0.01095825f, 0.01105742f, 0.01258215f, 0.01304081f, 0.01408209f, 
                0.01496222f, 0.01599730f, 0.01665430f, 0.01837118f, 0.01891661f, 0.01980914f, 0.02242474f, 0.02417260f, 0.02450110f, 0.02688118f, 
                0.02939761f, 0.03162272f, 0.03213716f, 0.03429411f, 0.03752331f, 0.04020090f, 0.04401893f, 0.04999390f, 0.04964060f, 0.06009061f, 
                0.05766095f, 0.06768329f,
                0.00628487f, 0.00728897f, 0.00675593f, 0.00681791f, 0.00636545f, 0.00729516f, 0.00743153f, 0.00774143f, 0.00785453f, 0.00795217f, 
                0.00808555f, 0.00809184f, 0.00813278f, 0.00813278f, 0.00822728f, 0.00822728f, 0.00832178f, 0.00841161f, 0.00846831f, 0.00856281f, 
        },
    },
    // KEY_SHAFT_TYPE_TTC_TGAMING,
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_TTC_TW_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_TTC_TW_HIGH_ACC_TO_LEVEL,
        .realTravel        = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_DIST + KEY_SHAFT_TTC_TW_HIGH_ACC_TO_DIST,
        .maxLevel          = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_TTC_TW_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_TTC_TW_NORMAL_ACC,
            
        .deadAcc           = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_TTC_TW_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToTopMax       = (KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_DIST + KEY_SHAFT_TTC_TW_HIGH_ACC_TO_DIST) - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.00605842f, 0.00823728f, 0.00867735f, 0.00893147f, 0.01036943f, 0.01095825f, 0.01105742f, 0.01258215f, 0.01304081f, 0.01408209f, 
                0.01496222f, 0.01599730f, 0.01665430f, 0.01837118f, 0.01891661f, 0.01980914f, 0.02242474f, 0.02417260f, 0.02450110f, 0.02688118f, 
                0.02939761f, 0.03162272f, 0.03213716f, 0.03429411f, 0.03752331f, 0.04020090f, 0.04401893f, 0.04999390f, 0.04964060f, 0.06009061f, 
                0.05766095f, 0.06768329f,
                0.00628487f, 0.00728897f, 0.00675593f, 0.00681791f, 0.00636545f, 0.00729516f, 0.00743153f, 0.00774143f, 0.00785453f, 0.00795217f, 
                0.00808555f, 0.00809184f, 0.00813278f, 0.00813278f, 0.00822728f, 0.00822728f, 0.00832178f, 0.00841161f, 0.00846831f, 0.00856281f, 
        },
    },
    // KEY_SHAFT_TYPE_TTC_SNAKE
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_TTC_WCW_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_TTC_WCW_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_TTC_WCW_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_TTC_WCW_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_TTC_WCW_HIGH_ACC_TO_LEVEL,
        .realTravel        = 3.3f,
        .maxLevel          = KEY_SHAFT_TTC_WCW_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_TTC_WCW_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
            
        .deadAcc           = KEY_SHAFT_TTC_WCW_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_TTC_WCW_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_TTC_WCW_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_TTC_WCW_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
        .dksToTopMax       = 3.3f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.00683431f, 0.00871670f, 0.00918238f, 0.00945129f, 0.01097294f, 0.01159603f, 0.01170097f, 0.01331444f, 0.01379980f, 0.01490168f, 
                0.01583304f, 0.01692836f, 0.01762360f, 0.01944040f, 0.02001758f, 0.02096205f, 0.02372988f, 0.02557947f, 0.02592709f, 0.02844569f, 
                0.03110858f, 0.03346320f, 0.03400758f, 0.03629006f, 0.03970721f, 0.04254063f, 0.04658088f, 0.05290360f, 0.05252974f, 0.06358795f, 
                0.06101688f, 0.07162253f, 
                0.00665066f, 0.00771320f, 0.00714913f, 0.00721472f, 0.00673593f, 0.00771975f, 0.00786405f, 0.00819199f, 0.00821167f, 0.00841499f, 
                0.00915614f, 0.00956279f, 0.00966117f, 
        },
    },
    // KEY_SHAFT_TYPE_TTC_TA,
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_TTC_TA_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_TTC_TA_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_TTC_TA_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_TTC_TA_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_TTC_TA_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_TTC_TA_HIGH_ACC_TO_LEVEL,
        .realTravel        = KEY_SHAFT_TTC_TA_NORMAL_ACC_TO_DIST + KEY_SHAFT_TTC_TA_HIGH_ACC_TO_DIST,
        .maxLevel          = KEY_SHAFT_TTC_TA_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_TTC_TA_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_TTC_TA_NORMAL_ACC,
            
        .deadAcc           = KEY_SHAFT_TTC_TA_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_TTC_TA_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_TTC_TA_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_TTC_TA_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_TTC_TA_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_TTC_TA_NORMAL_ACC,
        .dksToTopMax       = (KEY_SHAFT_TTC_TA_NORMAL_ACC_TO_DIST + KEY_SHAFT_TTC_TA_HIGH_ACC_TO_DIST) - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_TTC_TA_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.00718780f, 0.00913153f, 0.01006027f, 0.01071371f, 0.01097574f, 0.01211677f, 0.01216652f, 0.01333740f, 0.01413678f, 0.01507548f,
                0.01586491f, 0.01681023f, 0.01797116f, 0.01918184f, 0.02045223f, 0.02194485f, 0.02169940f, 0.02475430f, 0.02648906f, 0.02819728f,
                0.03068498f, 0.03223067f, 0.03533533f, 0.03784293f, 0.04102387f, 0.04392287f, 0.04547852f, 0.05085527f, 0.05630168f, 0.05949921f,
                0.06331700f, 0.07041193f,
                0.00658744f, 0.00760574f, 0.00743657f, 0.00781470f, 0.00789431f, 0.00794738f, 0.00806016f, 0.00815635f, 0.00826249f, 0.00841839f,
                0.00860082f, 0.00885622f, 0.00918791f, 
        },
    },
    // KEY_SHAFT_TYPE_TTC_SX,
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_TTC_TA_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_TTC_TA_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_TTC_TA_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_TTC_TA_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_TTC_TA_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_TTC_TA_HIGH_ACC_TO_LEVEL,
        .realTravel        = KEY_SHAFT_TTC_TA_NORMAL_ACC_TO_DIST + KEY_SHAFT_TTC_TA_HIGH_ACC_TO_DIST,
        .maxLevel          = KEY_SHAFT_TTC_TA_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_TTC_TA_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_TTC_TA_NORMAL_ACC,
            
        .deadAcc           = KEY_SHAFT_TTC_TA_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_TTC_TA_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_TTC_TA_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_TTC_TA_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_TTC_TA_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_TTC_TA_NORMAL_ACC,
        .dksToTopMax       = (KEY_SHAFT_TTC_TA_NORMAL_ACC_TO_DIST + KEY_SHAFT_TTC_TA_HIGH_ACC_TO_DIST) - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_TTC_TA_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.00718780f, 0.00913153f, 0.01006027f, 0.01071371f, 0.01097574f, 0.01211677f, 0.01216652f, 0.01333740f, 0.01413678f, 0.01507548f,
                0.01586491f, 0.01681023f, 0.01797116f, 0.01918184f, 0.02045223f, 0.02194485f, 0.02169940f, 0.02475430f, 0.02648906f, 0.02819728f,
                0.03068498f, 0.03223067f, 0.03533533f, 0.03784293f, 0.04102387f, 0.04392287f, 0.04547852f, 0.05085527f, 0.05630168f, 0.05949921f,
                0.06331700f, 0.07041193f,
                0.00658744f, 0.00760574f, 0.00743657f, 0.00781470f, 0.00789431f, 0.00794738f, 0.00806016f, 0.00815635f, 0.00826249f, 0.00841839f,
                0.00860082f, 0.00885622f, 0.00918791f, 
        },
    },
    // KEY_SHAFT_TYPE_TTC_PURPLE
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_TTC_WCW_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_TTC_WCW_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_TTC_WCW_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_TTC_WCW_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_TTC_WCW_HIGH_ACC_TO_LEVEL,
        .realTravel        = 3.3f,
        .maxLevel          = KEY_SHAFT_TTC_WCW_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_TTC_WCW_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
            
        .deadAcc           = KEY_SHAFT_TTC_WCW_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_TTC_WCW_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_TTC_WCW_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_TTC_WCW_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
        .dksToTopMax       = 3.3f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_TTC_WCW_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.00620975f, 0.00804968f, 0.00919963f, 0.00988960f, 0.01011960f, 0.01103956f, 0.01149954f, 0.01218951f, 0.01241950f, 0.01356946f, 
                0.01379945f, 0.01494940f, 0.01632935f, 0.01678933f, 0.01839926f, 0.01954922f, 0.02092916f, 0.02230911f, 0.02391904f, 0.02552898f, 
                0.02782889f, 0.03012879f, 0.03219871f, 0.03472861f, 0.03725851f, 0.04070837f, 0.04415823f, 0.04806808f, 0.05243790f, 0.05772769f, 
                0.06255750f, 0.06853726f, 
                0.00896964f, 0.00965961f, 0.00965961f, 0.00965961f, 0.01011960f, 0.01034959f, 0.01034959f, 0.01034959f, 0.01034959f, 0.01057958f, 
                0.01057958f, 0.01057958f, 0.01057958f, 
        },
    },
    // KEY_SHAFT_TYPE_TTC_FD
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_TTC_LG_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_TTC_LG_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_TTC_LG_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_TTC_LG_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_TTC_LG_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_TTC_LG_HIGH_ACC_TO_LEVEL,
        .realTravel        = 3.4f,
        .maxLevel          = KEY_SHAFT_TTC_LG_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_TTC_LG_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_TTC_LG_NORMAL_ACC,
            
        .deadAcc           = KEY_SHAFT_TTC_LG_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_TTC_LG_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_TTC_LG_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_TTC_LG_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_TTC_LG_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_TTC_LG_NORMAL_ACC,
        .dksToTopMax       = 3.4f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_TTC_LG_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.006321918f, 0.007919403f, 0.008255613f, 0.009153469f, 0.009343924f, 0.009705398f, 0.010813143f, 0.011075503f, 0.011784849f, 0.012606912f, 
                0.013320144f, 0.014280189f, 0.015522030f, 0.016268300f, 0.017360497f, 0.018551808f, 0.019818913f, 0.020951922f, 0.022236517f, 0.024267382f, 
                0.026086416f, 0.027950148f, 0.030387186f, 0.033006905f, 0.035096073f, 0.038116134f, 0.041602615f, 0.043483838f, 0.048684407f, 0.052856912f, 
                0.057620215f, 0.063532073f, 0.069655763f, 
                0.007107057f, 0.007462701f, 0.007406342f, 0.007590966f, 0.006522090f, 0.008207028f, 0.007637608f, 0.007637608f, 0.008086536f, 0.007933007f, 
                0.007919403f, 0.008245896f, 0.007857214f, 0.008006856f, 0.008275047f, 
        },
    },
    // KEY_SHAFT_TYPE_TTC_035
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_TTC_TW_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_TTC_TW_HIGH_ACC_TO_LEVEL,
        .realTravel        = 3.4f,
        .maxLevel          = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_TTC_TW_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_TTC_TW_NORMAL_ACC,
            
        .deadAcc           = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_TTC_TW_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToTopMax       = 3.4f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.005677887f, 0.007795522f, 0.008276956f, 0.008778589f, 0.009413206f, 0.009987223f, 0.010709374f, 0.011185758f, 0.011985342f, 0.012747892f,
                0.013722543f, 0.014596193f, 0.015781261f, 0.017008412f, 0.017764229f, 0.019444197f, 0.020821164f, 0.022408549f, 0.024113766f, 0.025876218f,
                0.025792051f, 0.030358937f, 0.032794722f, 0.035243974f, 0.038479343f, 0.041576678f, 0.045680648f, 0.050057318f, 0.054565287f, 0.059519341f,
                0.064921161f, 0.071294265f,
                0.007285472f, 0.007507672f, 0.007519455f, 0.007563222f, 0.007672639f, 0.008002572f, 0.007755122f, 0.008255072f, 0.007814039f, 0.008261806f, 
                0.008253389f, 0.008026139f, 0.008329139f, 0.008174272f, 0.008547973f, 0.008433506f, 0.008394789f, 0.008679273f, 0.008618673f, 0.008527772f, 
        },
    },
    // KEY_SHAFT_TYPE_TTC_FENGMI
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_TTC_TW_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_TTC_TW_HIGH_ACC_TO_LEVEL,
        .realTravel        = 3.4f,
        .maxLevel          = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_TTC_TW_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_TTC_TW_NORMAL_ACC,
            
        .deadAcc           = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .deadMin           = KEY_ACC_001MM,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_TTC_TW_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToTopMax       = 3.4f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.006001760f, 0.008374592f, 0.008946712f, 0.009438149f, 0.009707705f, 0.010708916f, 0.011429567f, 0.012042029f, 0.012715003f, 0.013556680f,
                0.014510214f, 0.015293212f, 0.016551142f, 0.017381817f, 0.018582902f, 0.019756482f, 0.021564529f, 0.021307808f, 0.024498478f, 0.026211171f,
                0.028077896f, 0.030318700f, 0.032884072f, 0.034961675f, 0.037631569f, 0.041280669f, 0.043679173f, 0.048582536f, 0.051747534f, 0.057725456f,
                0.062515128f, 0.067739392f,
                0.006295155f, 0.007153336f, 0.007333040f, 0.007153336f, 0.007639271f, 0.007908828f, 0.007224851f, 0.007567756f, 0.007701617f, 0.008251733f,
                0.007450398f, 0.007732791f, 0.008325082f, 0.007897825f, 0.007884989f, 0.008433271f, 0.008411266f, 0.008216892f, 0.008579968f, 0.008398430f,
        },
    },
    // KEY_SHAFT_TYPE_MIX_BABAO
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_WS_BABAO_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_WS_BABAO_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_WS_BABAO_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_WS_BABAO_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_WS_BABAO_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_WS_BABAO_HIGH_ACC_TO_LEVEL,
        .realTravel        = 3.6f,
        .maxLevel          = KEY_SHAFT_WS_BABAO_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_WS_BABAO_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_WS_BABAO_NORMAL_ACC,
            
        .deadAcc           = KEY_SHAFT_WS_BABAO_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_WS_BABAO_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_WS_BABAO_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_WS_BABAO_HIGH_ACC,
        .rtSensMax         = 2.5f,
        
        .dksAcc            = KEY_SHAFT_WS_BABAO_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_WS_BABAO_NORMAL_ACC,
        .dksToTopMax       = 3.6f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_WS_BABAO_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                 0.003741546F, 0.008058713F, 0.008490430F, 0.008922147F, 0.009785581F, 0.010217297F, 0.010936825F, 0.011656353F, 0.012231976F, 0.012663693F, 
                 0.013239315F, 0.014390560F, 0.015541805F, 0.016261333F, 0.017556483F, 0.017844294F, 0.019571161F, 0.021154123F, 0.022305368F, 0.023888329F, 
                 0.026190819F, 0.027629875F, 0.029932364F, 0.031803137F, 0.034681249F, 0.037271550F, 0.039861851F, 0.043171679F, 0.045905886F, 0.050079148F, 
                 0.053245071F, 0.057850050F, 0.061879407F, 0.068211253F, 
                 0.007339185F, 0.006619658F, 0.007626997F, 0.006907469F, 0.007914808F, 0.007339185F, 0.007483091F, 0.007914808F, 0.007339185F, 0.007626997F, 
                 0.008058713F, 0.008202619F, 0.007914808F, 0.007626997F, 0.007914808F,
        },
    },
    // KEY_SHAFT_TYPE_MIX_G_HUAHUO
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_G_HUAHUO_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_G_HUAHUO_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_G_HUAHUO_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_G_HUAHUO_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_G_HUAHUO_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_G_HUAHUO_HIGH_ACC_TO_LEVEL,
        .realTravel        = 3.2f,
        .maxLevel          = KEY_SHAFT_G_HUAHUO_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_G_HUAHUO_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_G_HUAHUO_NORMAL_ACC,
            
        .deadAcc           = KEY_SHAFT_G_HUAHUO_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_G_HUAHUO_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_G_HUAHUO_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_G_HUAHUO_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_G_HUAHUO_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_G_HUAHUO_NORMAL_ACC,
        .dksToTopMax       = 3.2f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_G_HUAHUO_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.006882989f, 0.010324484f, 0.010570305f, 0.011799410f, 0.012045231f, 0.013028515f, 0.013520157f, 0.014503441f, 0.015732547f, 0.016224189f, 
                0.017453294f, 0.018682399f, 0.019911504f, 0.021140610f, 0.023107178f, 0.024090462f, 0.024090462f, 0.027531957f, 0.030235988f, 0.032448378f, 
                0.034169125f, 0.038102262f, 0.039577188f, 0.043756146f, 0.047197640f, 0.051622419f, 0.054572271f, 0.059980334f, 0.064896755f, 0.070796460f, 
                0.075467060f, 
                0.007620452f, 0.008112094f, 0.008112094f, 0.008357915f, 0.008112094f, 0.008112094f, 0.008112094f,
        },
    },
    // KEY_SHAFT_TYPE_MIX_TAMSHAN
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_TTC_TW_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_TTC_TW_HIGH_ACC_TO_LEVEL,
        .realTravel        = 3.4f,
        .maxLevel          = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_TTC_TW_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_TTC_TW_NORMAL_ACC,
            
        .deadAcc           = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_TTC_TW_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToTopMax       = 3.4f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.005730659f, 0.008416905f, 0.008595989f, 0.009312321f, 0.010028653f, 0.009491404f, 0.011282235f, 0.011640401f, 0.012177650f, 0.013073066f, 
                0.014505731f, 0.014863897f, 0.015222063f, 0.017371060f, 0.018445559f, 0.020057307f, 0.021131805f, 0.023280802f, 0.023818052f, 0.026146132f,
                0.028474212f, 0.029906877f, 0.033130372f, 0.035995702f, 0.037786533f, 0.042442693f, 0.045845272f, 0.049964183f, 0.053366762f, 0.059097421f,
                0.064111748f, 0.068409742f, 
                0.007521490f, 0.006984241f, 0.007342407f, 0.007521490f, 0.008058739f, 0.007879656f, 0.007342407f, 0.007521490f, 0.007879656f, 0.008416905f, 
                0.008237822f, 0.008237822f, 0.007879656f, 0.007700573f, 0.008416905f, 0.008416905f, 0.007879656f, 0.007879656f, 0.007879656f, 0.007879656f,
        },
    },
    // KEY_SHAFT_TYPE_TTC_HOURSE
    {
        .voltAscend        = false,
        .isRtHighAcc       = true,
        
        .nacc              = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .nacc2dist         = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_DIST,
        .nacc2lvl          = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_LEVEL,
        .hacc              = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .hacc2dist         = KEY_SHAFT_TTC_TW_HIGH_ACC_TO_DIST,
        .hacc2lvl          = KEY_SHAFT_TTC_TW_HIGH_ACC_TO_LEVEL,
        .realTravel        = 3.4f,
        .maxLevel          = KEY_SHAFT_TTC_TW_NORMAL_ACC_TO_LEVEL + KEY_SHAFT_TTC_TW_HIGH_ACC_TO_LEVEL,
        
        .generalTrgAcc     = KEY_SHAFT_TTC_TW_NORMAL_ACC,
            
        .deadAcc           = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .deadMin           = 0,
        .deadMax           = LEVEL_TO_DIST(7, KEY_SHAFT_TTC_TW_HIGH_ACC),
        
        .rtSensAcc         = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .rtSensMin         = KEY_SHAFT_TTC_TW_HIGH_ACC,
        .rtSensMax         = 2.4f,
        
        .dksAcc            = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToTopMin       = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToTopMax       = 3.4f - (KEY_DFT_DKS_OFFSET_DIST * 2),
        .dksToBotMin       = KEY_SHAFT_TTC_TW_NORMAL_ACC,
        .dksToBotMax       = KEY_DFT_DKS_OFFSET_DIST,

        .curvTab = {
                0.005820722f, 0.008537059f, 0.008925107f, 0.009119131f, 0.010283275f, 0.010283275f, 0.011447419f, 0.011641444f, 0.012611564f, 0.013387660f,
                0.013581684f, 0.014939853f, 0.015715949f, 0.017074117f, 0.018432286f, 0.018820334f, 0.020954598f, 0.022118743f, 0.023864959f, 0.025611176f,
                0.027551416f, 0.029491657f, 0.032596042f, 0.035118355f, 0.037252619f, 0.041133101f, 0.044237485f, 0.046953822f, 0.052968568f, 0.055878929f,
                0.060535506f, 0.065774156f, 
                0.007178890f, 0.006596818f, 0.007372914f, 0.007178890f, 0.007372914f, 0.007178890f, 0.007372914f, 0.007372914f, 0.007178890f, 0.007372914f,
                0.007954986f, 0.007760962f, 0.007954986f, 0.007954986f, 0.007954986f, 0.007954986f, 0.008343035f, 0.008343035f, 0.008149010f, 0.008149010f,
        },
    },
};

const key_sw_thd_t sw_thd_table[KEY_SHAFT_TYPE_MAX] = 
{
    /* BASE: hall-MH481, switch-74HC4067(nexperia) */
    
    // KEY_SHAFT_TYPE_UNKNOWN 
    {
            .top_max        = 0,
            .top_min        = 0,
            .bottom_max     = 0,
            .bottom_min     = 0,
            .diff_max       = 0,
            .diff_min       = 0,   
    },
    // KEY_SHAFT_TYPE_GATERON_WHITE 
    {
            .top_max        = 3550*1, //3550
            .top_min        = 2850*1, //3250
            .bottom_max     = 2450*1, //2150
            .bottom_min     = 1500*1, //1850
            .diff_max       = 1900*1, //1700
            .diff_min       = 900*1, //1100 
            .def_diff       = 800*1,
    },
    // KEY_SHAFT_TYPE_GATERON_JADE
    {
            .top_max        = (3250+200)*1,
            .top_min        = (3250-200)*1,
            .bottom_max     = (1950+200)*1,
            .bottom_min     = (1650-200)*1,
            .diff_max       = (1600+300)*1,
            .diff_min       = (1600-300)*1,
            .def_diff       = (1600)*1,
    },
    // KEY_SHAFT_TYPE_KAILH_GREEN
    {
            .top_max        = 3450*1, //3450
            .top_min        = 3000*1, //3150
            .bottom_max     = 2150*1, //2150
            .bottom_min     = 1600*1, //1850
            .diff_max       = 1600*1, //1600
            .diff_min       = 1000*1, //1000
            .def_diff       = 700*1,
    },
    // KEY_SHAFT_TYPE_JW_NMF
    {
            .top_max        = 3550*1, //3550
            .top_min        = 3150*1, //3150
            .bottom_max     = 2550*1, //2550
            .bottom_min     = 2150*1, //2150
            .diff_max       = 1600*1, //1600
            .diff_min       = 600*1, //600
            .def_diff       = 700*1,
    },
     // KEY_SHAFT_TYPE_TTC_WCW//1.6mm pcb
    {
            .top_max        = (3250+200)*1,
            .top_min        = (3250-200)*1,
            .bottom_max     = (1650+200)*1,
            .bottom_min     = (1650-200)*1,
            .diff_max       = (1600+300)*1,
            .diff_min       = (1600-300)*1,
            .def_diff       = (1600)*1,
    },
    // KEY_SHAFT_TYPE_GATERON_JPRO,
    {
            .top_max        = (3250+200)*1,
            .top_min        = (3250-200)*1,
            .bottom_max     = (1950+200)*1,
            .bottom_min     = (1650-200)*1,
            .diff_max       = (1600+300)*1,
            .diff_min       = (1600-300)*1,
            .def_diff       = (1600)*1,
    },
    // KEY_SHAFT_TYPE_GATERON_JMAX,
    {
            .top_max        = (3250+200)*1,
            .top_min        = (3250-200)*1,
            .bottom_max     = (1950+200)*1,
            .bottom_min     = (1650-200)*1,
            .diff_max       = (1600+300)*1,
            .diff_min       = (1600-300)*1,
            .def_diff       = (1600)*1,
    },
    // KEY_SHAFT_TYPE_GATERON_JGAMING,
    {
            .top_max        = (3250+200)*1,
            .top_min        = (3250-200)*1,
            .bottom_max     = (1950+200)*1,
            .bottom_min     = (1650-200)*1,
            .diff_max       = (1600+300)*1,
            .diff_min       = (1600-300)*1,
            .def_diff       = (1600)*1,
    },
    // KEY_SHAFT_TYPE_TTC_WMIN,
    {
            .top_max        = 0*15,
            .top_min        = 0*15,
            .bottom_max     = 0*15,
            .bottom_min     = 0*15,
            .diff_max       = 0*15,
            .diff_min       = 0*15,
            .def_diff       = 0*15,
    },
    // KEY_SHAFT_TYPE_TTC_WRGB,
    {
            .top_max        = (3250+200)*1,
            .top_min        = (3250-200)*1,
            .bottom_max     = (1650+200)*1,
            .bottom_min     = (1650-200)*1,
            .diff_max       = (1600+300)*1,
            .diff_min       = (1600-300)*1,
            .def_diff       = (1600)*1,
    },
    // KEY_SHAFT_TYPE_TTC_TSTD,
    {
            .top_max        = (3250+200)*1,
            .top_min        = (3250-200)*1,
            .bottom_max     = (1650+200)*1,
            .bottom_min     = (1650-200)*1,
            .diff_max       = (1600+300)*1,
            .diff_min       = (1600-300)*1,
            .def_diff       = (1600)*1,
    },
    // KEY_SHAFT_TYPE_TTC_TSE,
    {
            .top_max        = (3250+200)*1,
            .top_min        = (3250-200)*1,
            .bottom_max     = (1650+200)*1,
            .bottom_min     = (1650-200)*1,
            .diff_max       = (1600+300)*1,
            .diff_min       = (1600-300)*1,
            .def_diff       = (1600)*1,
    },
    // KEY_SHAFT_TYPE_TTC_TGAMING,
    {
            .top_max        = (3250+200)*1,
            .top_min        = (3250-200)*1,
            .bottom_max     = (1650+200)*1,
            .bottom_min     = (1650-200)*1,
            .diff_max       = (1600+300)*1,
            .diff_min       = (1600-300)*1,
            .def_diff       = (1600)*1,
    },
    // KEY_SHAFT_TYPE_TTC_SNAKE
    {
            .top_max        = (3250+200)*1,
            .top_min        = (3250-200)*1,
            .bottom_max     = (1650+200)*1,
            .bottom_min     = (1650-200)*1,
            .diff_max       = (1600+300)*1,
            .diff_min       = (1600-300)*1,
            .def_diff       = (1600)*1,
    },
    // KEY_SHAFT_TYPE_TTC_TA
    {
            .top_max        = (3250+200)*1,
            .top_min        = (3250-200)*1,
            .bottom_max     = (1650+200)*1,
            .bottom_min     = (1650-200)*1,
            .diff_max       = (1600+300)*1,
            .diff_min       = (1600-300)*1,
            .def_diff       = (1600)*1,
    },
    // KEY_SHAFT_TYPE_TTC_SX
    {
            .top_max        = (3250+200)*1,
            .top_min        = (3250-200)*1,
            .bottom_max     = (1650+200)*1,
            .bottom_min     = (1650-200)*1,
            .diff_max       = (1600+300)*1,
            .diff_min       = (1600-300)*1,
            .def_diff       = (1600)*1,
    },
    // KEY_SHAFT_TYPE_TTC_PURPLE
    {
            .top_max        = (3350+200)*1,
            .top_min        = (3350-200)*1,
            .bottom_max     = (1900+200)*1,
            .bottom_min     = (1900-200)*1,
            .diff_max       = (1400+300)*1,
            .diff_min       = (1400-300)*1,
            .def_diff       = (1400)*1,
    },
    // KEY_SHAFT_TYPE_TTC_FD
    {
            .top_max        = (3350+300)*1,
            .top_min        = (3350-300)*1,
            .bottom_max     = (2200+400)*1,
            .bottom_min     = (1950-400)*1,
            .diff_max       = (1350+400)*1,
            .diff_min       = (1200-400)*1,
            .def_diff       = (1200)*1,
    },
    // KEY_SHAFT_TYPE_TTC_035
    {
            .top_max        = (3350+300)*1,
            .top_min        = (3350-300)*1,
            .bottom_max     = (1950+300)*1,
            .bottom_min     = (1950-500)*1,
            .diff_max       = (1450+450)*1,
            .diff_min       = (1450-350)*1,
            .def_diff       = (1300)*1,
    },
    // KEY_SHAFT_TYPE_TTC_FENGMI
    {
            .top_max        = (3350+300)*1,
            .top_min        = (3350-300)*1,
            .bottom_max     = (1950+300)*1,
            .bottom_min     = (1950-500)*1,
            .diff_max       = (1450+450)*1,
            .diff_min       = (1450-350)*1,
            .def_diff       = (1300)*1,
    },
    // KEY_SHAFT_TYPE_MIX_BABAO
    {
            .top_max        = (3350+300)*1,
            .top_min        = (3350-300)*1,
            .bottom_max     = (1550+300)*1,
            .bottom_min     = (1550-500)*1,
            .diff_max       = (1750+450)*1,
            .diff_min       = (1450-350)*1,
            .def_diff       = (1600)*1,
    },
    // KEY_SHAFT_TYPE_MIX_G_HUAHUO
    {
            .top_max        = (3350+300)*1,
            .top_min        = (3350-300)*1,
            .bottom_max     = (2400+300)*1,
            .bottom_min     = (2400-500)*1,
            .diff_max       = (1450+450)*1,
            .diff_min       = (1150-350)*1,
            .def_diff       = (1300)*1,
    },
    // KEY_SHAFT_TYPE_MIX_TAMSHAN
    {
            .top_max        = (3350+300)*1,
            .top_min        = (3350-300)*1,
            .bottom_max     = (1950+300)*1,
            .bottom_min     = (1950-500)*1,
            .diff_max       = (1450+450)*1,
            .diff_min       = (1450-350)*1,
            .def_diff       = (1300)*1,
    },
    // KEY_SHAFT_TYPE_TTC_HOURSE
    {
            .top_max        = (3350+300)*1,
            .top_min        = (3350-300)*1,
            .bottom_max     = (1950+300)*1,
            .bottom_min     = (1950-500)*1,
            .diff_max       = (1450+450)*1,
            .diff_min       = (1450-350)*1,
            .def_diff       = (1300)*1, 
    }
};


//const static int8_t key_frame1[]=
//{
// 00, 01, 02, 03, 04, 05, 06, 07, 08, 09, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
   //37, 36, 34, -1, -1, 30,  8,  6,  5,  3,  1,  0, 23, 61, 35,  4, 60, 59,  9,  7, 20, 33, 31, 58, 50, 49, 48, 47, 45, 44, 22, 21, 19, 18, 16, 15,
//};

//const static int8_t key_frame2[]=
//{
// 00, 01, 02, 03, 04, 05, 06, 07, 08, 09, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
   //43, 42, 41, 40, 38, -1, 14, 13, 12, 11, 10,  2, 67, 56, 64, 62, 39, -1, 66, 65, 63, 26, 52, 32, 57, 55, 54, 53, 51, 46, 29, 28, 27, 25, 24, 17
//};

const int8_t key_frame1[]=
{
// 00, 01, 02, 03, 04, 05, 06, 07, 08, 09, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
   37, 36, 34, -1, -1, 30,  8,  6,  5,  3,  1,  0, 23, 61, 35,  4, 60, 59,  9,  7, 20, 33, 31, 58, 50, 49, 48, 47, 45, 44, 22, 21, 19, 18, 16, 15,
};

const int8_t key_frame2[]=
{
// 00, 01, 02, 03, 04, 05, 06, 07, 08, 09, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
   43, 42, 41, 40, 38, -1, 14, 13, 12, 11, 10,  2, 67, 56, 64, 62, 39, -1, 66, 65, 63, 26, 52, 32, 57, 55, 54, 53, 51, 46, 29, 28, 27, 25, 24, 17
};

/*
const int8_t key_frame_table[KEY_FRAME_NUM][KEY_FRAME_SIZE] = {
  //  00, 01, 02, 03, 04, 05,  06, 07, 08, 09, 10, 11, 
    { 37, 36, 34, -1, -1, 30,  43, 42, 41, 40, 38, -1,  }, //r1
    {  8,  6,  5,  3,  1,  0,  14, 13, 12, 11, 10,  2,  }, //r2
    { 23, 61, 35,  4, 60, 59,  67, 56, 64, 62, 39, -1,  }, //r3
    {  9,  7, 20, 33, 31, 58,  66, 65, 63, 26, 52, 32,  }, //r4
    { 50, 49, 48, 47, 45, 44,  57, 55, 54, 53, 51, 46,  }, //r5
    { 22, 21, 19, 18, 16, 15,  29, 28, 27, 25, 24, 17,  }, //r6
};
*/  
const int8_t key_idx_table[KEY_FRAME_NUM*KEY_FRAME_SIZE] = 
{
    56, 70, 69, 43, 0 , 15, 29, 16, 1 , 17, 31, 30, 44, 45, 
    46, 57, 32, 18, 2 , 33, 19, 3 , 20, 21, 34, 61, 48, 60, 
    47, 59, 58, 71, 35, 22, 4 , 36, 5 , 6 , 23, 24, 37, 64, 
    51, 63, 50, 62, 72, 49, 38, 7 , 8 , 39, 25, 26, 9 , 40, 
    54, 75, 74, 66, 53, 65, 73, 52, 41, 27, 10, 11, 28, 12, 
    42, 13, 14, 55, -1, 78, 68, 77, 76, 67, 
};



//kseq_t kseq = {
// .addr = {   1,  5,  7,  0,  2,  3,  },
// .seq  = {   0,  1,  2,  3,  4,  5,  },
// .ki   = {
//           { 00, 59, 58, 44, 15, 30, }, //A
//           { 01, 60, -1, 45, 16, 31, }, //B
//           { 02, -1, -1, 46, 17, 32, }, //C
//           { 03, 04, -1, 47, 18, 33, }, //D
//           { 05, 35, 20, 48, 19, 34, }, //E
//           { 06, 61, 07, 49, 21, 36, }, //F
//           { 08, 23, 09, 50, 22, 37, }, //G
//           { 10, 39, 52, 51, 24, 38, }, //H
//           { 11, 62, 26, 53, 25, 40, }, //J
//           { 12, 64, 63, 54, 27, 41, }, //K
//           { 13, 56, 65, 55, 28, 42, }, //L
//           { 14, 67, 66, 57, 29, 43, }, //M
//    },
//};  

#if ((KB_USE==KB_MOJO68RT) || (KB_USE==KB_MADE68PRO))
const int8_t key_amend_table[KEY_FRAME_NUM*KEY_FRAME_SIZE] = 
{
//  00,   01,   02,   03,   04,   05,   06,   07,   08,   09,   10,   11,   12,   13,   14,   15,   16,   17,   18,   19,   20,   21,   22,   23,   
    59,   60,  -127,  04,  -127,  35,   61,   49,   23,   50,   39,   62,   64,   56,   67,   31,   30,   32,   33,   34,   48,   36,   37,   9,

//  24,   25,   26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,   40,   41,   42,   43,   44,   45,   46,   47,
    38,   40,   53,   41,   42,   43,  -30,  -31,  -32,  -33,  -34,   20,  -36,  -37,  -38,   52,  -40,  -41,  -42,  -43,   15,   16,   17,   18,

//  48,   49,   50,   51,   52,   53,   54,   55,   56,   57,   58,   59,   60,   61,   62,   63,   64,   65,   66,   67,   68,   69,   
    19,   21,   22,   24,   51,   25,   27,   28,   65,   29,   44,   58,  -127,  7,    26,   54,   63,   55,   57,   66,  -127, -127, 
};
#endif
const int16_t kc_to_ki[256] = 
{
    -1, -1, -1, -1, 44, 61, 59, 46, 32, 47, 
    48, 49, 37, 50, 51, 52, 63, 62, 38, 39, 
    30, 33, 45, 34, 36, 60, 31, 58, 35, 57, 
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 
    55,  0, 28, 29, 72, 26, 27, 40, 41, 42, 
    -1, 53, 54, 15, 64, 65, 66, 43,  1,  2, 
     3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 
    -1, -1, -1, 13, -1, -1, 14, -1, -1, 78, 
    76, 77, 68, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, 69, 56, 71, 70, 75, 67, 
    73, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, 
};

#if (USE_KEY_LVL_MODE == KEY_LVL_MODE_REALTIME)

float kcr_table[KEY_SHAFT_TYPE_MAX][KEY_LEVEL_BOUNDARY_BUFF_SIZE];
void kcr_table_update(void)
{
    for (uint8_t i = 0; i < KEY_SHAFT_TYPE_MAX; i++)
    {
        for (uint16_t j = 0; j < KEY_LEVEL_BOUNDARY_BUFF_SIZE; j++)
        {
            kcr_table[i][j] = sw_config_table[i].curvTab[0][j];
            if (j != 0)
            {
                kcr_table[i][j] += kcr_table[i][j-1];
            }
        }
    }
}

#endif



