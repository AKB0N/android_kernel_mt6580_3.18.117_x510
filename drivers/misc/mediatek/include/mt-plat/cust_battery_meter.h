
#ifndef _CUST_BATTERY_METER_H
#define _CUST_BATTERY_METER_H

#include <mt_typedefs.h>

// ============================================================
// define
// ============================================================
//#define SOC_BY_AUXADC
#define SOC_BY_HW_FG
//#define HW_FG_FORCE_USE_SW_OCV
//#define SOC_BY_SW_FG

//#define CONFIG_DIS_CHECK_BATTERY
//#define FIXED_TBAT_25

/* ADC Channel Number */
#if 0
#define CUST_TABT_NUMBER 17
#define VBAT_CHANNEL_NUMBER      7
#define ISENSE_CHANNEL_NUMBER	 6
#define VCHARGER_CHANNEL_NUMBER  4
#define VBATTEMP_CHANNEL_NUMBER  5
#endif
/* ADC resistor  */
#define R_BAT_SENSE 4					
#define R_I_SENSE 4						
#define R_CHARGER_1 330
#define R_CHARGER_2 39

#define TEMPERATURE_T0             110
#define TEMPERATURE_T1             0
#define TEMPERATURE_T2             25
#define TEMPERATURE_T3             50
#define TEMPERATURE_T              255 // This should be fixed, never change the value

#define FG_METER_RESISTANCE 	0

#if 1//lisong 2015-2-28 [cust battery capacity for D5116(35M) project: ZCV of 2200mAh]start
/* Qmax for battery  */   
/*--------------------
fenghua_2200mAh_daigangpian:
Q_MAX_POS_50:2260
Q_MAX_POS_25:2272
Q_MAX_POS_0:2239
Q_MAX_NEG_10:2140
Q_MAX_POS_50_H_CURRENT:2248
Q_MAX_POS_25_H_CURRENT:2255
Q_MAX_POS_0_H_CURRENT:1960
Q_MAX_NEG_10_H_CURRENT:1380
fenghua_3000mAh_daigangpian:
Q_MAX_POS_50:3174
Q_MAX_POS_25:3181
Q_MAX_POS_0:3139
Q_MAX_NEG_10:2999
Q_MAX_POS_50_H_CURRENT:3159
Q_MAX_POS_25_H_CURRENT:3160
Q_MAX_POS_0_H_CURRENT:2891
Q_MAX_NEG_10_H_CURRENT:1947
---------------------*/
/* Qmax for battery  */
#define Q_MAX_POS_50	2260
#define Q_MAX_POS_25	2272
#define Q_MAX_POS_0	2239
#define Q_MAX_NEG_10	2140
    
#define Q_MAX_POS_50_H_CURRENT	2248
#define Q_MAX_POS_25_H_CURRENT	2255
#define Q_MAX_POS_0_H_CURRENT	1960
#define Q_MAX_NEG_10_H_CURRENT	1380
#else
/* Qmax for battery  */
#define Q_MAX_POS_50	1523
#define Q_MAX_POS_25	1489
#define Q_MAX_POS_0		1272
#define Q_MAX_NEG_10	1189
#define Q_MAX_POS_50_H_CURRENT	1511
#define Q_MAX_POS_25_H_CURRENT	1462
#define Q_MAX_POS_0_H_CURRENT	  818
#define Q_MAX_NEG_10_H_CURRENT	149
#endif//lisong 2015-2-28 [cust battery capacity for D5116(35M) project: ZCV of 2200mAh]end

/* Discharge Percentage */
#define OAM_D5		 0		//  1 : D5,   0: D2


/* battery meter parameter */
#define CHANGE_TRACKING_POINT
#define CUST_TRACKING_POINT  1
#define CUST_R_SENSE         56
#define CUST_HW_CC 		    0
#define AGING_TUNING_VALUE   103
#define CUST_R_FG_OFFSET    0

#define OCV_BOARD_COMPESATE	0 //mV 
#define R_FG_BOARD_BASE		1000
#define R_FG_BOARD_SLOPE	1000 //slope
#define CAR_TUNE_VALUE		191 //1.00


/* HW Fuel gague  */
#define CURRENT_DETECT_R_FG	10  //1mA
#define MinErrorOffset       1000
#define FG_VBAT_AVERAGE_SIZE 18
#define R_FG_VALUE 			10 // mOhm, base is 20

#define CUST_POWERON_DELTA_CAPACITY_TOLRANCE	30
#define CUST_POWERON_LOW_CAPACITY_TOLRANCE		5
#define CUST_POWERON_MAX_VBAT_TOLRANCE			90
#define CUST_POWERON_DELTA_VBAT_TOLRANCE		30
#define CUST_POWERON_DELTA_HW_SW_OCV_CAPACITY_TOLRANCE	10


/* Disable Battery check for HQA */
#ifdef MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
#define FIXED_TBAT_25
#endif

/* Dynamic change wake up period of battery thread when suspend*/
#define VBAT_NORMAL_WAKEUP		3600		//3.6V
#define VBAT_LOW_POWER_WAKEUP		3500		//3.5v
#define NORMAL_WAKEUP_PERIOD		5400 		//90 * 60 = 90 min
#define LOW_POWER_WAKEUP_PERIOD		300		//5 * 60 = 5 min
#define CLOSE_POWEROFF_WAKEUP_PERIOD	30	//30 s

#define FG_BAT_INT
#define IS_BATTERY_REMOVE_BY_PMIC


#endif	//#ifndef _CUST_BATTERY_METER_H