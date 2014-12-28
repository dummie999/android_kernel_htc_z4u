/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 2010 High Tech Computer Corporation

Module Name:

		ds2746_param_config.c

Abstract:

		This module tells batt_param.c module what the battery characteristic is.
		And also which charger/gauge component hardware uses, to decide if software/hardware function is available.

Original Auther:

		Andy.ys Wang June-01-2010

---------------------------------------------------------------------------------*/


#ifndef __BATT_PARAM_CONFIG_H__
#define __BATT_PARAM_CONFIG_H__
#if defined(CONFIG_MACH_SPADE)
#define HTC_BATT_BOARD_NAME "ACE"
#endif

#if (defined(CONFIG_MACH_PRIMODS) || defined(CONFIG_MACH_PROTOU) || defined(CONFIG_MACH_PROTODUG) || defined(CONFIG_MACH_PROTODCG) || defined(CONFIG_MACH_MAGNIDS))
#define HTC_BATT_BOARD_NAME "PRIMODS"
#endif


static BOOL support_ds2746_gauge_ic = TRUE;

UINT32 TEMP_MAP_300K_100_4360[] =
{
0, 96, 100, 103, 107, 111, 115, 119, 123, 127,
133, 137, 143, 148, 154, 159, 165, 172, 178, 185,
192, 199, 207, 215, 223, 232, 241, 250, 260, 270,
280, 291, 301, 314, 326, 339, 352, 366, 380, 394,
410, 425, 442, 458, 476, 494, 512, 531, 551, 571,
592, 614, 636, 659, 682, 706, 730, 755, 780, 806,
833, 860, 887, 942, 943, 971, 1000, 1028, 1058, 1087,
1117, 1146, 1176, 1205, 1234, 1264, 1293, 1321, 1350, 1378,
1406, 1433, 2047,
};

UINT32 TEMP_MAP_300K_47_3440[] =
{
0, 68, 70, 72, 74, 76, 78, 81, 83, 85,
88, 91, 93, 96, 99, 102, 105, 109, 112, 116,
119, 123, 127, 131, 135, 139, 144, 148, 153, 158,
163, 169, 174, 180, 186, 192, 199, 205, 212, 219,
227, 234, 242, 250, 259, 268, 277, 286, 296, 306,
317, 327, 339, 350, 362, 374, 387, 401, 414, 428,
443, 458, 474, 490, 506, 523, 540, 558, 577, 596,
615, 656, 677, 698, 720, 742, 765, 789, 812, 837,
861, 886, 2047,
};

UINT32 TEMP_MAP_1000K_100_4360[] =
{
0, 30, 31, 32, 34, 35, 36, 38, 39, 40,
42, 44, 45, 47, 49, 51, 53, 55, 57, 60,
62, 64, 67, 70, 73, 76, 79, 82, 86, 89,
93, 97, 101, 106, 111, 115, 120, 126, 131, 137,
143, 150, 156, 163, 171, 178, 187, 195, 204, 213,
223, 233, 244, 255, 267, 279, 292, 306, 320, 334,
350, 365, 382, 399, 418, 436, 456, 476, 497, 519,
542, 566, 590, 615, 641, 668, 695, 723, 752, 782,
812, 843, 2047,
};

UINT32 TEMP_MAP_470K_100_4360[] =
{
62, 64, 67, 69, 72, 74, 77, 80, 83, 86,
90, 93, 97, 100, 104, 108, 113, 117, 122, 126,
131, 137, 142, 148, 154, 160, 167, 173, 180, 188,
196, 204, 212, 221, 230, 239, 249, 260, 270, 282,
293, 305, 318, 331, 345, 359, 374, 389, 405, 422,
439, 457, 475, 494, 514, 535, 556, 577, 600, 623,
647, 671, 696, 722, 748, 775, 802, 830, 859, 887,
917, 947, 977, 1007, 1038, 1069, 1100, 1131, 1162,
1193, 1224, 1255,
};

UINT32 *TEMP_MAP = TEMP_MAP_300K_100_4360;

#define PD_M_COEF_DEFAULT	(30)
#define PD_M_RESL_DEFAULT	(100)
#define PD_T_COEF_DEFAULT	(250)
#define CAPACITY_DEDUCTION_DEFAULT	(0)

UINT32 M_PARAMETER_DEFAULT[] =
{
  
  10000, 4135, 7500, 3960, 4700, 3800, 1700, 3727, 900, 3674, 300, 3640, 0, 3420,
};



static INT32 voltage_adc_to_mv_coef = 244;
static INT32 voltage_adc_to_mv_resl = 100;
static INT32 current_adc_to_mv_coef = 625;
static INT32 current_adc_to_mv_resl = 1450;
static INT32 discharge_adc_to_mv_coef = 625;
static INT32 discharge_adc_to_mv_resl = 1450;
static INT32 acr_adc_to_mv_coef = 625;
static INT32 acr_adc_to_mv_resl = 1450;
static INT32 charge_counter_zero_base_mAh = 500;

static INT32 id_adc_overflow = 3067; 
static INT32 id_adc_resl = 2047;
static INT32 temp_adc_resl = 2047;


static INT32 pd_m_bias_mA;    


static INT32 over_high_temp_lock_01c = 600;
static INT32 over_high_temp_release_01c = 570;
static INT32 over_low_temp_lock_01c = 0;
static INT32 over_low_temp_release_01c = 30;


static BOOL is_allow_batt_id_change = TRUE;	
extern BOOL is_need_battery_id_detection;


#define BATTERY_DEAD_VOLTAGE_LEVEL  	3420
#define BATTERY_DEAD_VOLTAGE_RELEASE	3450

#define TEMP_MAX 70
#define TEMP_MIN -11
#define TEMP_NUM 83

#endif
