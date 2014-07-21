/*
 * Copyright (C) 2007 HTC Incorporated
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef _DS2746_BATTERY_H_
#define _DS2746_BATTERY_H_
#include <linux/notifier.h>
#include <linux/device.h>
#include <mach/htc_battery.h>
#include <linux/wrapper_types.h>
#include <linux/ds2746_param.h>


enum ds2746_notify_evt_t {
	DS2746_CHARGING_CONTROL = 0,
	DS2746_LEVEL_UPDATE,
};


enum {
	CHARGE_STATE_UNKNOWN,   		
	CHARGE_STATE_PREDICTION,		
	CHARGE_STATE_DISCHARGE, 		
	CHARGE_STATE_CHARGING,  		
	CHARGE_STATE_PENDING,   		
	CHARGE_STATE_FULL_WAIT_STABLE,  
	CHARGE_STATE_FULL_CHARGING, 	
	CHARGE_STATE_FULL_PENDING,  	
	CHARGE_STATE_FULL_RECHARGING,  	
};

enum {
	THERMAL_300_100_4360,
	THERMAL_300_47_3440,
	THERMAL_1000_100_4360,
	THERMAL_470_100_4360,
};

enum BATTERY_ID_ENUM {
	BATTERY_ID_UNKNOWN = 0,
	BATTERY_ID_TWS_SDI_1650MAH,
	BATTERY_ID_FORMOSA_SANYO,
	BATTERY_ID_SAMSUNG_1230MAH = BATTERY_ID_TWS_SDI_1650MAH,
	BATTERY_ID_LG_3260MAH,
	BATTERY_ID_ATL_4000MAH,
	BATTERY_ID_NUM, 
};


typedef struct _ds2746_platform_data ds2746_platform_data;

struct poweralg_type
{
	int charge_state;
	int capacity_01p;
	int last_capacity_01p;
	int fst_discharge_capacity_01p;
	int fst_discharge_acr_mAh;
	int charging_source;
	int charging_enable;
	BOOL is_gauge_driver_ready;
	BOOL is_need_calibrate_at_49p;
	BOOL is_need_calibrate_at_14p;
	BOOL is_charge_over_load;
	struct battery_type battery;
	struct protect_flags_type protect_flags;
	BOOL is_china_ac_in;
	BOOL is_super_ac;
	BOOL is_cable_in;
	BOOL is_voltage_stable;
	BOOL is_software_charger_timeout;
	BOOL is_superchg_software_charger_timeout;
	ktime_t start_ktime;
	UINT32 last_charger_enable_toggled_time_ms;
	BOOL is_need_toggle_charger;
	ds2746_platform_data* pdata;
};

struct poweralg_config_type
{
	INT32 full_charging_mv;
	INT32 full_charging_ma;
	INT32 full_pending_ma;			
	INT32 full_charging_timeout_sec;	 
	INT32 voltage_recharge_mv;  		 
	INT32 capacity_recharge_p;  		 
	INT32 voltage_exit_full_mv; 		 
	INT32 min_taper_current_mv;		 
	INT32 min_taper_current_ma; 		 
	INT32 wait_votlage_statble_sec;
	INT32 predict_timeout_sec;
	INT32 polling_time_in_charging_sec;
	INT32 polling_time_in_discharging_sec;

	BOOL enable_full_calibration;
	BOOL enable_weight_percentage;
	INT32 software_charger_timeout_sec;   
	INT32 superchg_software_charger_timeout_sec;   
	INT32 charger_hw_safety_timer_watchdog_sec;  

	BOOL debug_disable_shutdown;
	BOOL debug_fake_room_temp;
	BOOL debug_disable_hw_timer;
	BOOL debug_always_predict;
	BOOL por_reset_fail;
	INT32 full_level;                  
};

struct battery_parameter {
	UINT32* fl_25;
	UINT32** pd_m_coef_tbl_boot;	
	UINT32** pd_m_coef_tbl;		
	UINT32** pd_m_resl_tbl_boot;	
	UINT32** pd_m_resl_tbl;		
	UINT32* capacity_deduction_tbl_01p;	
	UINT32* pd_t_coef;
	INT32* padc;	
	INT32* pw;	
	UINT32* id_tbl;
	INT32* temp_index_tbl;	
	UINT32** m_param_tbl;
	int m_param_tbl_size;
	UINT32* capacity;
};

struct _ds2746_platform_data {
	struct battery_parameter* batt_param;
	int (*func_get_thermal_id)(void);
	int (*func_get_battery_id)(void);
	void (*func_poweralg_config_init)(struct poweralg_config_type*);
	int (*func_update_charging_protect_flag)(int, int, int, BOOL*, BOOL*, BOOL*);
	int r2_kohm;
	void (*func_kick_charger_ic)(int);
	
};


#define DS2746_FULL_CAPACITY_DEFAULT	(999)


#define BATTERY_PERCENTAGE_UNKNOWN  0xFF
#define BATTERY_LOW_PERCENTAGE	    10  	
#define BATTERY_CRITICAL_PERCENTAGE 5   	
#define BATTERY_EMPTY_PERCENTAGE    0   	

#define REVERSE_PROTECTION_HAPPEND 1
#define REVERSE_PROTECTION_CONTER_CLEAR 0


int get_state_check_interval_min_sec( void);
BOOL do_power_alg( BOOL is_event_triggered);
void power_alg_init( struct poweralg_config_type *debug_config);
void power_alg_preinit( void);
int ds2746_blocking_notify( unsigned long val, void *v);
void ds2746_charger_control( int type);
int ds2746_i2c_write_u8( u8 value, u8 reg);
int ds2746_i2c_read_u8( u8* value, u8 reg);
int ds2746_battery_id_adc_2_ohm(int id_adc, int r2_kohm);
void calibrate_id_ohm( struct battery_type *battery);
extern int ds2746_charger_switch(int charger_switch);


extern int ds2746_register_notifier( struct notifier_block *nb);
extern int ds2746_unregister_notifier( struct notifier_block *nb);
extern int ds2746_get_battery_info( struct battery_info_reply *batt_info);
extern ssize_t htc_battery_show_attr( struct device_attribute *attr, char *buf);
extern void reverse_protection_handler( int status);
extern void ds2746_phone_call_in(int phone_call_in);

#endif
