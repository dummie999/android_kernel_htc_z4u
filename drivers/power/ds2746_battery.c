/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 2010 High Tech Computer Corporation

Module Name:

		ds2746_battery.c

Abstract:

		This module implements the power algorithm, including below concepts:
		1. Charging function control.
		2. Charging full condition.
		3. Recharge control.
		4. Battery capacity maintainance.
		5. Battery full capacity calibration.

Original Auther:

		Andy.YS Wang  June-01-2010
---------------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/android_alarm.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/wakelock.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <linux/ds2746_battery.h>
#include <linux/ds2746_battery_config.h>
#include <linux/ds2746_param.h>
#include <linux/wrapper_types.h>
#include <linux/tps65200.h>
#include <mach/htc_battery.h>
#include <asm/mach-types.h>
#include <mach/proc_comm.h>
#include <linux/i2c.h>  					
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/slab.h>
#include <mach/board.h>
#define MSPERIOD(end, start)	ktime_to_ms(ktime_sub(end, start))

struct ds2746_device_info {

		struct device *dev;
		struct device *w1_dev;
		struct workqueue_struct *monitor_wqueue;
		
		struct delayed_work monitor_work;
		
		struct mutex lock;
		
		unsigned long update_time;	
		struct alarm alarm;
		struct wake_lock work_wake_lock;
		spinlock_t spin_lock;
		u8 slow_poll;
		ktime_t last_poll;
};
static struct wake_lock vbus_wake_lock;


static struct poweralg_type poweralg = {0};
static struct poweralg_config_type config = {0};
static struct poweralg_config_type debug_config = {0};
#if (defined(CONFIG_MACH_PRIMODS) || defined(CONFIG_MACH_PROTOU) || defined(CONFIG_MACH_PROTODUG) || defined(CONFIG_MACH_MAGNIDS))
BOOL is_need_battery_id_detection = FALSE;
#else
BOOL is_need_battery_id_detection = TRUE;
#endif
static struct ds2746_device_info *g_di_ptr = NULL;

static int g_first_update_charger_ctl = 1;

static int charger_control;
static int force_update_batt_info = 1;
static int force_set_chg = 0;
static int reverse_protecion_counter;
static int set_phone_call_in_poll;


#define FAST_POLL	(1 * 30)
#define SLOW_POLL	(60 * 60)
#define PHONE_CALL_POLL	(5 * 60)
#define PREDIC_POLL	20

#define HTC_BATTERY_I2C_DEBUG_ENABLE		0
#define HTC_BATTERY_DS2746_DEBUG_ENABLE 	1

#if defined(CONFIG_MACH_SPADE)
#define DS2746_I2C_BUS_ID   	0
#else
#define DS2746_I2C_BUS_ID   	1
#endif
#define DS2746_I2C_SLAVE_ADDR   0x26

static UINT32 delta_time_sec = 0;
static UINT32 chg_en_time_sec = 0;
static UINT32 chg_kick_time_sec = 0;
static UINT32 super_chg_on_time_sec = 0;
static ktime_t cable_remove_ktime;
static ktime_t last_poll_ktime;



#define DS2746_STATUS_REG	0x01
#define DS2746_AUX0_MSB		0x08
#define DS2746_AUX0_LSB 	0x09
#define DS2746_AUX1_MSB 	0x0A
#define DS2746_AUX1_LSB 	0x0B
#define DS2746_VOLT_MSB 	0x0C
#define DS2746_VOLT_LSB 	0x0D
#define DS2746_CURRENT_MSB	0x0E
#define DS2746_CURRENT_LSB	0x0F
#define DS2746_ACR_MSB  	0x10
#define DS2746_ACR_LSB  	0x11

static struct i2c_adapter *i2c2 = NULL;
static struct i2c_client *ds2746_i2c = NULL;

int ds2746_i2c_write_u8(u8 value, u8 reg)
{
	int ret;
	u8 buf[2];
	struct i2c_msg *msg;
	struct i2c_msg xfer_msg[1];

	
	msg = &xfer_msg[0];
	msg->addr = ds2746_i2c->addr;
	msg->len = 2;
	msg->flags = 0; 		
	msg->buf = buf;

	buf[0] = reg;
	buf[1] = value;

	ret = i2c_transfer(ds2746_i2c->adapter, xfer_msg, 1);
	if (ret <= 0){
		printk(DRIVER_ZONE "[%s] fail.\n", __func__);
	}

#if HTC_BATTERY_I2C_DEBUG_ENABLE
	printk(DRIVER_ZONE "[%s] ds2746[0x%x]<-0x%x.\n", __func__, reg, value);
#endif

	return ret;
}

int ds2746_i2c_read_u8(u8 *value, u8 reg)
{
	int ret;
	struct i2c_msg *msg;
	struct i2c_msg xfer_msg[2];

	
	msg = &xfer_msg[0];
	msg->addr = ds2746_i2c->addr;
	msg->len = 1;
	msg->flags = 0; 		
	msg->buf = &reg;
	
	msg = &xfer_msg[1];
	msg->addr = ds2746_i2c->addr;
	msg->len = 1;
	msg->flags = I2C_M_RD;  
	msg->buf = value;

	ret = i2c_transfer(ds2746_i2c->adapter, xfer_msg, 2);
	if (ret <= 0){
		printk(DRIVER_ZONE "[%s] fail.\n", __func__);
	}

#if HTC_BATTERY_I2C_DEBUG_ENABLE
	printk(DRIVER_ZONE "[%s] ds2746[0x%x]=0x%x.\n", __func__, reg, *value);
#endif

	return ret;
}

static void ds2746_i2c_exit(void)
{
	if (ds2746_i2c != NULL){
		kfree(ds2746_i2c);
		ds2746_i2c = NULL;
	}

	if (i2c2 != NULL){
		i2c_put_adapter(i2c2);
		i2c2 = NULL;
	}
}

static int ds2746_i2c_init(void)
{
	i2c2 = i2c_get_adapter(DS2746_I2C_BUS_ID);
	ds2746_i2c = kzalloc(sizeof(*ds2746_i2c), GFP_KERNEL);

	if (i2c2 == NULL || ds2746_i2c == NULL){
		printk(DRIVER_ZONE "[%s] fail (0x%x, 0x%x).\n",
			__func__,
			(int) i2c2,
			(int) ds2746_i2c);
		ds2746_i2c_exit();
		return -ENOMEM;
	}

	ds2746_i2c->adapter = i2c2;
	ds2746_i2c->addr = DS2746_I2C_SLAVE_ADDR;

	return 0;
}


static BOOL b_is_charge_off_by_bounding = FALSE;
static void bounding_fullly_charged_level(int upperbd)
{
	static int pingpong = 1;
	int lowerbd;
	int current_level;
	b_is_charge_off_by_bounding = FALSE;
	if (upperbd <= 0)
		return; 
	lowerbd = upperbd - 5; 

	if (lowerbd < 0)
		lowerbd = 0;
	current_level = CEILING(poweralg.capacity_01p, 10);

	if (pingpong == 1 && upperbd <= current_level) {
		printk(DRIVER_ZONE "MFG: lowerbd=%d, upperbd=%d, current=%d, pingpong:1->0 turn off\n", lowerbd, upperbd, current_level);
		b_is_charge_off_by_bounding = TRUE;
		pingpong = 0;
	} else if (pingpong == 0 && lowerbd < current_level) {
		printk(DRIVER_ZONE "MFG: lowerbd=%d, upperbd=%d, current=%d, toward 0, turn off\n", lowerbd, upperbd, current_level);
		b_is_charge_off_by_bounding = TRUE;
	} else if (pingpong == 0 && current_level <= lowerbd) {
		printk(DRIVER_ZONE "MFG: lowerbd=%d, upperbd=%d, current=%d, pingpong:0->1 turn on\n", lowerbd, upperbd, current_level);
		pingpong = 1;
	} else {
		printk(DRIVER_ZONE "MFG: lowerbd=%d, upperbd=%d, current=%d, toward %d, turn on\n", lowerbd, upperbd, current_level, pingpong);
	}

}

static BOOL is_charge_off_by_bounding_condition(void)
{
	return b_is_charge_off_by_bounding;
}

void calibrate_id_ohm(struct battery_type *battery)
{
	if (!poweralg.charging_source || !poweralg.charging_enable){
		battery->id_ohm += 500; 		
	}
	else if (poweralg.charging_source == 2 && battery->current_mA >= 400 && battery->id_ohm >= 1500){
		battery->id_ohm -= 1500;		
	}
	else if (battery->id_ohm >= 700){
		battery->id_ohm -= 700; 		
	}
}

static BOOL is_charging_avaiable(void)
{
	BOOL chg_avalible = TRUE;
	if (poweralg.is_superchg_software_charger_timeout) chg_avalible = FALSE;
	if (poweralg.is_software_charger_timeout) chg_avalible = FALSE;
	if (!poweralg.protect_flags.is_charging_enable_available &&
		!poweralg.protect_flags.is_fake_room_temp)chg_avalible = FALSE;
	if (poweralg.protect_flags.is_charging_reverse_protect) {
		printk(DRIVER_ZONE "Disable charger due to reverse protection\n");
		chg_avalible = FALSE;
	}
	if (!poweralg.is_cable_in) chg_avalible = FALSE;
	if (poweralg.charge_state == CHARGE_STATE_PENDING) chg_avalible = FALSE;
	if (poweralg.charge_state == CHARGE_STATE_FULL_PENDING)	chg_avalible = FALSE;
	if (poweralg.charge_state == CHARGE_STATE_PREDICTION) chg_avalible = FALSE;
	if (is_charge_off_by_bounding_condition()) chg_avalible = FALSE;
	if (poweralg.battery.id_index == BATTERY_ID_UNKNOWN) chg_avalible = FALSE;
	if (charger_control)
		chg_avalible = FALSE;

#if (defined(CONFIG_MACH_PRIMODS) || defined(CONFIG_MACH_PROTOU) || defined(CONFIG_MACH_PROTODUG) || defined(CONFIG_MACH_MAGNIDS))
	set_smem_chg_avalible(chg_avalible);
#endif
	return chg_avalible; 
}

static BOOL is_high_current_charging_avaialable(void)
{
	if (!poweralg.protect_flags.is_charging_high_current_avaialble &&
		!poweralg.protect_flags.is_fake_room_temp)	return FALSE;
	if (!poweralg.is_china_ac_in) return FALSE;
	if (poweralg.charge_state == CHARGE_STATE_UNKNOWN) return FALSE;
	return TRUE;
}

static BOOL is_super_current_charging_avaialable(void)
{
	if (!poweralg.is_super_ac) return FALSE;
	return TRUE;
}
static BOOL is_set_min_taper_current(void)
{
	if ((config.min_taper_current_ma > 0) &&
		(config.min_taper_current_mv > 0) &&
		(poweralg.battery.current_mA < config.min_taper_current_ma) &&
		(config.min_taper_current_mv < poweralg.battery.voltage_mV))
		return TRUE;

	return FALSE;
}

static void update_next_charge_state(BOOL bFirstEntry)
{
	static UINT32 count_charging_full_condition;
	static UINT32 count_charge_over_load;
	int next_charge_state;
	int i;
	ktime_t end_ktime = ktime_get_real();


	if (MSPERIOD(poweralg.start_ktime, end_ktime) > 0) {
		poweralg.start_ktime = end_ktime;
		printk(DRIVER_ZONE "Time changed, reassigned start time [%lld]\n",ktime_to_ms(poweralg.start_ktime));
	}

	for (i = 0; i < 25; i++) 
	{
		next_charge_state = poweralg.charge_state;

		
		if (poweralg.charge_state == CHARGE_STATE_UNKNOWN){
			if (bFirstEntry && config.por_reset_fail && abs(poweralg.battery.KADC_01p - poweralg.battery.RARC_01p) >= 50){
				printk(DRIVER_ZONE " |KADC[%d] - RARC[%d]| > 5%%"
						" Set POR as TRUE\n",
						poweralg.battery.KADC_01p / 10,
						poweralg.battery.RARC_01p / 10);
				poweralg.battery.is_power_on_reset = TRUE;
			}
			if (poweralg.battery.is_power_on_reset || config.debug_always_predict){
				if (poweralg.protect_flags.is_battery_dead){
					
					printk(DRIVER_ZONE " dead battery, \
						p=0%%\n");
					poweralg.capacity_01p = 0;
					battery_capacity_update(&poweralg.battery, poweralg.capacity_01p);

					poweralg.fst_discharge_capacity_01p = poweralg.capacity_01p;
					poweralg.fst_discharge_acr_mAh = poweralg.battery.charge_counter_mAh;
				}
				else{
					
					printk(DRIVER_ZONE " start predict discharge...\n");
					next_charge_state = CHARGE_STATE_PREDICTION;
				}

				config.debug_always_predict = FALSE;
			}
		}
		
		if (((poweralg.charge_state == CHARGE_STATE_UNKNOWN) ||
		     (poweralg.charge_state == CHARGE_STATE_DISCHARGE))
		    && (200 < (poweralg.battery.KADC_01p - poweralg.battery.RARC_01p))
		    && (ktime_equal(ktime_set(0, 0), cable_remove_ktime) ||
			MSPERIOD(ktime_get_real(), cable_remove_ktime) >= 15 * 60 * 1000)
		    && (!poweralg.is_cable_in)) {
			printk(DRIVER_ZONE " KADC[%d] - RARC[%d] > 20%%"
				" => start prediction discharge...\n",
				poweralg.battery.KADC_01p / 10,
				poweralg.battery.RARC_01p / 10);
			next_charge_state = CHARGE_STATE_PREDICTION;
			poweralg.battery.is_power_on_reset = TRUE;
			cable_remove_ktime = ktime_set(0, 0);
		}

		if (next_charge_state == poweralg.charge_state){
			
			
			if (poweralg.charge_state == CHARGE_STATE_UNKNOWN ||
				poweralg.charge_state == CHARGE_STATE_CHARGING ||
				poweralg.charge_state == CHARGE_STATE_PENDING ||
				poweralg.charge_state == CHARGE_STATE_FULL_WAIT_STABLE ||
				poweralg.charge_state == CHARGE_STATE_FULL_CHARGING ||
				poweralg.charge_state == CHARGE_STATE_FULL_RECHARGING ||
				poweralg.charge_state == CHARGE_STATE_FULL_PENDING){
				if (!poweralg.is_cable_in){
					next_charge_state = CHARGE_STATE_DISCHARGE;
				}
				else if (!poweralg.protect_flags.is_charging_enable_available){
					next_charge_state = CHARGE_STATE_PENDING;
				}
			}

			
			
			if (poweralg.charge_state == CHARGE_STATE_UNKNOWN ||
				poweralg.charge_state == CHARGE_STATE_DISCHARGE){
				if (poweralg.is_cable_in){
					next_charge_state = CHARGE_STATE_CHARGING;
				}
			}
		}

		

		
		if (next_charge_state == poweralg.charge_state){
			switch (poweralg.charge_state){
				case CHARGE_STATE_PREDICTION:
						end_ktime = ktime_get_real();
						if (MSPERIOD(end_ktime, poweralg.start_ktime) >= 50 * 1000) {
							poweralg.start_ktime = end_ktime;
							printk(DRIVER_ZONE "reassign prediction start timestamp as [%lld]\n", ktime_to_ms(end_ktime));
						} else if (MSPERIOD(end_ktime, poweralg.start_ktime) >= config.predict_timeout_sec * 1000) {
							printk(DRIVER_ZONE "predict done [%lld->%lld]\n", ktime_to_ms(poweralg.start_ktime), ktime_to_ms(end_ktime));
							next_charge_state = CHARGE_STATE_UNKNOWN;
						}
					break;
				case CHARGE_STATE_CHARGING:
					if (!poweralg.battery.is_power_on_reset){
						
						if (poweralg.capacity_01p > 990){
							
							next_charge_state = CHARGE_STATE_FULL_CHARGING; 
						}
						else if (poweralg.battery.voltage_mV >= config.full_charging_mv &&
							poweralg.battery.current_mA >= 0 &&
							poweralg.battery.current_mA <= config.full_charging_ma){
							
							next_charge_state = CHARGE_STATE_FULL_WAIT_STABLE;
						}
					}

					if (poweralg.battery.current_mA <= 0){
						
						if (count_charge_over_load < 5)
							count_charge_over_load++;
						else
							poweralg.is_charge_over_load = TRUE;
					}
					else{
						count_charge_over_load = 0;
						poweralg.is_charge_over_load = FALSE;
					}

					
					
					if (!poweralg.protect_flags.is_fake_room_temp && config.software_charger_timeout_sec &&
						config.software_charger_timeout_sec <=
						chg_en_time_sec) {
								printk(DRIVER_ZONE "Disable charger due"
								" to charging time lasts %u s > 16hr\n",
								chg_en_time_sec);
								poweralg.is_software_charger_timeout = TRUE;
					}
#if 0
					
					if (config.software_charger_timeout_sec && poweralg.is_china_ac_in){
						
						UINT32 end_time_ms = BAHW_MyGetMSecs();

						if (end_time_ms - poweralg.state_start_time_ms >=
							config.software_charger_timeout_sec * 1000){

							printk(DRIVER_ZONE "software charger timer timeout [%d->%d]\n",
								poweralg.state_start_time_ms,
								end_time_ms);
							poweralg.is_software_charger_timeout = TRUE;
						}
					}
#endif
					
#if 0
					if (config.superchg_software_charger_timeout_sec && poweralg.is_super_ac
						&& FALSE==poweralg.is_superchg_software_charger_timeout){
						
						UINT32 end_time_ms = BAHW_MyGetMSecs();

						if (end_time_ms - poweralg.state_start_time_ms >=
							config.superchg_software_charger_timeout_sec * 1000){

							printk(DRIVER_ZONE "superchg software charger timer timeout [%d->%d]\n",
								poweralg.state_start_time_ms,
								end_time_ms);
							poweralg.is_superchg_software_charger_timeout = TRUE;
						}
					}
#endif
#if 0
					if (config.charger_hw_safety_timer_watchdog_sec && poweralg.is_cable_in){
						UINT32 end_time_ms = BAHW_MyGetMSecs();
						if (end_time_ms - poweralg.last_charger_enable_toggled_time_ms >=
							config.charger_hw_safety_timer_watchdog_sec * 1000){
							poweralg.last_charger_enable_toggled_time_ms = BAHW_MyGetMSecs();
							poweralg.is_need_toggle_charger = TRUE;
							printk(DRIVER_ZONE "need software toggle charger [%d->%d]\n",
								poweralg.last_charger_enable_toggled_time_ms,
								end_time_ms);

						}
					}
#endif
					break;
				case CHARGE_STATE_FULL_WAIT_STABLE:
					{
						
						if (poweralg.battery.voltage_mV >= config.full_charging_mv &&
							poweralg.battery.current_mA >= 0 &&
							poweralg.battery.current_mA <= config.full_charging_ma){

							count_charging_full_condition++;
						}
						else{
							count_charging_full_condition = 0;
							next_charge_state = CHARGE_STATE_CHARGING;
						}

						if (count_charging_full_condition >= 3){
							poweralg.capacity_01p = 1000;
							battery_capacity_update(&poweralg.battery, poweralg.capacity_01p);

							next_charge_state = CHARGE_STATE_FULL_CHARGING;
						}
					}
					break;
				case CHARGE_STATE_FULL_CHARGING:
					{
						
						end_ktime = ktime_get_real();

						if (poweralg.battery.voltage_mV < config.voltage_exit_full_mv){
							if (poweralg.capacity_01p > 990)
								poweralg.capacity_01p = 990;
							next_charge_state = CHARGE_STATE_CHARGING;
						}
						else if (config.full_pending_ma != 0 &&
							poweralg.battery.current_mA >= 0 &&
							poweralg.battery.current_mA <= config.full_pending_ma){

							printk(DRIVER_ZONE " charge-full pending(%dmA)(%lld:%lld)\n",
								poweralg.battery.current_mA,
								ktime_to_ms(poweralg.start_ktime),
								ktime_to_ms(end_ktime));

							next_charge_state = CHARGE_STATE_FULL_PENDING;
						}
						else if (MSPERIOD(end_ktime, poweralg.start_ktime) >=
							config.full_charging_timeout_sec * 1000){

							printk(DRIVER_ZONE " charge-full (expect:%dsec)(%lld:%lld)\n",
								config.full_charging_timeout_sec,
								ktime_to_ms(poweralg.start_ktime),
								ktime_to_ms(end_ktime));
							next_charge_state = CHARGE_STATE_FULL_PENDING;
						}
					}
					break;
				case CHARGE_STATE_FULL_PENDING:
					if ((poweralg.battery.voltage_mV >= 0 &&
						poweralg.battery.voltage_mV < config.voltage_recharge_mv) ||
						(poweralg.battery.RARC_01p >= 0 &&
						poweralg.battery.RARC_01p <= config.capacity_recharge_p * 10)){
						
						next_charge_state = CHARGE_STATE_FULL_RECHARGING;
					}
					break;
				case CHARGE_STATE_FULL_RECHARGING:
					{
						if (poweralg.battery.voltage_mV < config.voltage_exit_full_mv){
							if (poweralg.capacity_01p > 990)
								poweralg.capacity_01p = 990;
							next_charge_state = CHARGE_STATE_CHARGING;
						}
						else if (poweralg.battery.voltage_mV >= config.full_charging_mv &&
							poweralg.battery.current_mA >= 0 &&
							poweralg.battery.current_mA <= config.full_charging_ma){
							
							next_charge_state = CHARGE_STATE_FULL_CHARGING;
						}
					}
					break;
				case CHARGE_STATE_PENDING:
				case CHARGE_STATE_DISCHARGE:
					{
						end_ktime = ktime_get_real();

						if (!poweralg.is_voltage_stable){
							if (MSPERIOD(end_ktime, poweralg.start_ktime) >=
								config.wait_votlage_statble_sec * 1000){

								printk(DRIVER_ZONE " voltage stable\n");
								poweralg.is_voltage_stable = TRUE;
							}
						}
					}

					if (poweralg.is_cable_in &&
						poweralg.protect_flags.is_charging_enable_available){
						
						next_charge_state = CHARGE_STATE_CHARGING;
					}
					break;
			}
		}

		
		
		if (next_charge_state != poweralg.charge_state){
			
			switch (poweralg.charge_state){
				case CHARGE_STATE_UNKNOWN:
					if (bFirstEntry || (poweralg.capacity_01p > poweralg.battery.RARC_01p))
						poweralg.capacity_01p = poweralg.battery.RARC_01p;
					if (poweralg.capacity_01p > 990)
						poweralg.capacity_01p = 990;
					if (poweralg.capacity_01p < 0)
						poweralg.capacity_01p = 0;
					poweralg.fst_discharge_capacity_01p = poweralg.capacity_01p;
					poweralg.fst_discharge_acr_mAh = poweralg.battery.charge_counter_mAh;
					break;
				case CHARGE_STATE_PREDICTION:
					battery_param_update(&poweralg.battery,
						&poweralg.protect_flags);
					if (bFirstEntry || (poweralg.capacity_01p > poweralg.battery.KADC_01p))
						poweralg.capacity_01p = poweralg.battery.KADC_01p;
					if (poweralg.capacity_01p > 1000)
						poweralg.capacity_01p = 1000;
					if (poweralg.capacity_01p < 0)
						poweralg.capacity_01p = 0;
					battery_capacity_update(&poweralg.battery,
						poweralg.capacity_01p);

					poweralg.fst_discharge_capacity_01p = poweralg.capacity_01p;
					poweralg.fst_discharge_acr_mAh = poweralg.battery.charge_counter_mAh;
					break;
			}

			
			poweralg.start_ktime = ktime_get_real();

			switch (next_charge_state){
				case CHARGE_STATE_DISCHARGE:
				case CHARGE_STATE_PENDING:
					
					if (poweralg.battery.RARC_01p > 1000)
						battery_capacity_update(&poweralg.battery, 1000);

					poweralg.is_need_calibrate_at_49p = TRUE;
					poweralg.is_need_calibrate_at_14p = TRUE;
					poweralg.fst_discharge_capacity_01p = poweralg.capacity_01p;
					poweralg.fst_discharge_acr_mAh = poweralg.battery.charge_counter_mAh;
					poweralg.is_voltage_stable = FALSE;

					break;
				case CHARGE_STATE_CHARGING:
					poweralg.is_need_toggle_charger = FALSE;
					poweralg.last_charger_enable_toggled_time_ms = BAHW_MyGetMSecs();
					poweralg.is_software_charger_timeout = FALSE;   
					poweralg.is_charge_over_load = FALSE;
					count_charge_over_load = 0;
					poweralg.battery.charge_full_real_mAh = poweralg.battery.charge_full_design_mAh;
					battery_capacity_update(&poweralg.battery, poweralg.capacity_01p);
					break;
				case CHARGE_STATE_FULL_WAIT_STABLE:
					
					count_charging_full_condition = 0;
					break;
			}

			printk(DRIVER_ZONE " state change(%d->%d), full count=%d, over load count=%d [%lld]\n",
				poweralg.charge_state,
				next_charge_state,
				count_charging_full_condition,
				count_charge_over_load,
				ktime_to_ms(poweralg.start_ktime));

			poweralg.charge_state = next_charge_state;
			continue;
		}

		break;
	}
}

static void __update_capacity(BOOL bFirstEntry)
{
	INT32 next_capacity_01p;

	if (poweralg.charge_state == CHARGE_STATE_PREDICTION ||
		poweralg.charge_state == CHARGE_STATE_UNKNOWN){
		if (bFirstEntry) {
			
			poweralg.capacity_01p = max(min(990, poweralg.battery.KADC_01p), 250);
			printk(DRIVER_ZONE "fake percentage (%d) during prediction.\n",
				poweralg.capacity_01p);
		}
	}
	else if (poweralg.charge_state == CHARGE_STATE_FULL_CHARGING ||
		poweralg.charge_state == CHARGE_STATE_FULL_RECHARGING ||
		poweralg.charge_state == CHARGE_STATE_FULL_PENDING){
			poweralg.capacity_01p = 1000;
	}
	else if (!is_charging_avaiable() && poweralg.is_voltage_stable){
		
		if (poweralg.battery.voltage_mV < 3300) {
			poweralg.capacity_01p -= 30; 
			if (poweralg.capacity_01p < 0)
				poweralg.capacity_01p = 0;
			battery_capacity_update(&poweralg.battery, poweralg.capacity_01p);
		}
		
		else if (poweralg.battery.KADC_01p <= 0){
			if (poweralg.capacity_01p > 0)
				poweralg.capacity_01p -= 10;
			if (poweralg.capacity_01p > 0){
				
				battery_capacity_update(&poweralg.battery, poweralg.capacity_01p);
			}
		}
		else{
			if ((config.enable_weight_percentage) && (poweralg.capacity_01p <150 ||
				poweralg.battery.RARC_01p> poweralg.battery.KADC_01p)){
				INT32 w_kadc;
				INT32 w_rarc;
				INT32 Padc;
				INT32 Pw;
				if (0 <= poweralg.pdata->batt_param->padc)
					Padc = poweralg.pdata->batt_param->padc[poweralg.battery.id_index];
				else
					Padc = 200; 
				if (0 <= poweralg.pdata->batt_param->pw)
					Pw = poweralg.pdata->batt_param->pw[poweralg.battery.id_index];
				else
					Pw = 5;	
#define W_KADC(RARC, Percentage) Padc+(INT32)abs(RARC-Percentage)*Pw
				
				w_kadc = min(max(W_KADC(poweralg.battery.RARC_01p, poweralg.battery.KADC_01p), 0), 1000);
				w_rarc = 1000 - w_kadc;
				next_capacity_01p = (w_kadc * poweralg.battery.KADC_01p + w_rarc * poweralg.battery.RARC_01p)/1000;
			}
			else{
				next_capacity_01p = poweralg.battery.RARC_01p;
			}

			if (next_capacity_01p > 1000)
				next_capacity_01p = 1000;
			if (next_capacity_01p < 0)
				next_capacity_01p = 0;

			if (next_capacity_01p < poweralg.capacity_01p){
				poweralg.capacity_01p -= min(10, poweralg.capacity_01p-next_capacity_01p);
			}
		}

		if (config.enable_full_calibration){
			if (poweralg.is_need_calibrate_at_49p &&
				poweralg.capacity_01p <= 500 &&
				poweralg.fst_discharge_capacity_01p >= 600){

				poweralg.battery.charge_full_real_mAh = (poweralg.fst_discharge_acr_mAh-poweralg.battery.charge_counter_mAh)*1000/
					(poweralg.fst_discharge_capacity_01p-poweralg.capacity_01p);

				battery_capacity_update(&poweralg.battery, poweralg.capacity_01p);

				poweralg.is_need_calibrate_at_49p = FALSE;
				poweralg.fst_discharge_capacity_01p = poweralg.capacity_01p;
				poweralg.fst_discharge_acr_mAh = poweralg.battery.charge_counter_mAh;

				printk(DRIVER_ZONE " 1.full calibrate: full=%d\n",
					poweralg.battery.charge_full_real_mAh);
			}
			else if (poweralg.is_need_calibrate_at_14p &&
				poweralg.capacity_01p <= 150 &&
				poweralg.fst_discharge_capacity_01p >= 250){
				poweralg.battery.charge_full_real_mAh = (poweralg.fst_discharge_acr_mAh-poweralg.battery.charge_counter_mAh)*1000/
					(poweralg.fst_discharge_capacity_01p - poweralg.capacity_01p);

				battery_capacity_update(&poweralg.battery, poweralg.capacity_01p);

				poweralg.is_need_calibrate_at_14p = FALSE;
				poweralg.fst_discharge_capacity_01p = poweralg.capacity_01p;
				poweralg.fst_discharge_acr_mAh = poweralg.battery.charge_counter_mAh;

				printk(DRIVER_ZONE " 2.full calibrate: full=%d\n",
					poweralg.battery.charge_full_real_mAh);
			}
		}
	}
	else{
		next_capacity_01p = poweralg.battery.RARC_01p;

		if (next_capacity_01p > 1000)
			next_capacity_01p = 1000;
		if (next_capacity_01p < 0)
			next_capacity_01p = 0;

		if (next_capacity_01p > poweralg.capacity_01p){
			
			next_capacity_01p = poweralg.capacity_01p + min(next_capacity_01p - poweralg.capacity_01p, 10);
			if (poweralg.capacity_01p > 990)
				poweralg.capacity_01p = next_capacity_01p;
			else
				poweralg.capacity_01p = min(next_capacity_01p, 990);
		}
		else if (next_capacity_01p < poweralg.capacity_01p){
			
			poweralg.capacity_01p -= min(poweralg.capacity_01p - next_capacity_01p, 10);
			if (poweralg.capacity_01p < 0)
				poweralg.capacity_01p = 0;
		}
	}
}


int get_state_check_interval_min_sec(void)
{
	return 0;
}

int check_charging_function(void)
{
	if (is_charging_avaiable()) {
		chg_en_time_sec += delta_time_sec;
		chg_kick_time_sec += delta_time_sec;
		
		if (poweralg.pdata->func_kick_charger_ic &&
			600 <= chg_kick_time_sec) {
			chg_kick_time_sec = 0;
			poweralg.pdata->func_kick_charger_ic(poweralg.charging_enable);
		}
		if (config.charger_hw_safety_timer_watchdog_sec) {
			if (config.charger_hw_safety_timer_watchdog_sec
				<= chg_en_time_sec) {
				printk(DRIVER_ZONE "need software toggle "
					"charger: lasts %d sec\n",
					chg_en_time_sec);
				chg_en_time_sec = 0;
				chg_kick_time_sec = 0;
				poweralg.is_need_toggle_charger = FALSE;
				poweralg.protect_flags.is_charging_reverse_protect = FALSE;
				ds2746_charger_control(DISABLE);
				udelay(200);
			}
		}

		if (is_high_current_charging_avaialable()) {
			if (is_super_current_charging_avaialable())
				ds2746_charger_control(ENABLE_SUPER_CHG);
			else
				ds2746_charger_control(ENABLE_FAST_CHG);
		} else
			ds2746_charger_control(ENABLE_SLOW_CHG);

		
		
		if ((config.min_taper_current_ma > 0)) {
			if (is_set_min_taper_current())
				ds2746_charger_control(ENABLE_MIN_TAPER);
			else
				ds2746_charger_control(DISABLE_MIN_TAPER);
		}
	} else {
		ds2746_charger_control(DISABLE);
		chg_en_time_sec = 0;
		chg_kick_time_sec = 0;
		super_chg_on_time_sec = 0;
		poweralg.is_need_toggle_charger = FALSE;
		poweralg.protect_flags.is_charging_reverse_protect = FALSE;
	}

	if (config.debug_disable_hw_timer && poweralg.is_charge_over_load) {
		ds2746_charger_control(DISABLE);
		printk(DRIVER_ZONE "Toggle charger due to HW disable charger.\n");
	}

	return 0;
}

BOOL do_power_alg(BOOL is_event_triggered)
{
	
	static BOOL s_bFirstEntry = TRUE;
	static ktime_t s_pre_time_ktime, pre_param_update_ktime;
	ktime_t now_time_ktime = ktime_get_real();
#if !HTC_BATTERY_DS2746_DEBUG_ENABLE
	BOOL show_debug_message = FALSE;
#endif

#ifdef CONFIG_THERMAL_TEMPERATURE_READ
	
	htc_get_thermal_adc_level(&poweralg.battery.thermal_temp);
#endif
	
	if (MSPERIOD(pre_param_update_ktime, now_time_ktime) > 0 || MSPERIOD(s_pre_time_ktime, now_time_ktime) > 0) {
		printk(DRIVER_ZONE "Time changed, update to the current time [%lld]\n",ktime_to_ms(now_time_ktime));
		pre_param_update_ktime = now_time_ktime;
		s_pre_time_ktime = now_time_ktime;
	}
	if (s_bFirstEntry || MSPERIOD(now_time_ktime, pre_param_update_ktime) >= 3 * 1000) {
		pre_param_update_ktime = now_time_ktime;
		if (!battery_param_update(&poweralg.battery, &poweralg.protect_flags)){
			printk(DRIVER_ZONE "battery_param_update fail, please retry next time.\n");
			return FALSE;
		}
	}

	update_next_charge_state(s_bFirstEntry);

	if (poweralg.charge_state != CHARGE_STATE_UNKNOWN && poweralg.charge_state != CHARGE_STATE_PREDICTION)
		poweralg.is_gauge_driver_ready = TRUE;

	if (s_bFirstEntry || MSPERIOD(now_time_ktime, s_pre_time_ktime) >= 10 * 1000 || !is_event_triggered){
		
		__update_capacity(s_bFirstEntry);

		s_bFirstEntry = FALSE;
		s_pre_time_ktime = now_time_ktime;
	}

	if (config.debug_disable_shutdown){
		if (poweralg.capacity_01p <= 0){
			poweralg.capacity_01p = 1;
		}
	}

	bounding_fullly_charged_level(config.full_level);

	
	if (config.superchg_software_charger_timeout_sec && poweralg.is_super_ac
		&& FALSE==poweralg.is_superchg_software_charger_timeout){
		super_chg_on_time_sec += delta_time_sec;
		if (config.superchg_software_charger_timeout_sec <= super_chg_on_time_sec){
			printk(DRIVER_ZONE "superchg charger on timer timeout: %u sec\n",
				super_chg_on_time_sec);
			poweralg.is_superchg_software_charger_timeout = TRUE;
		}
	}

	check_charging_function();


	htc_battery_update_change(force_update_batt_info);
	if (force_update_batt_info)
		force_update_batt_info = 0;

#if HTC_BATTERY_DS2746_DEBUG_ENABLE
	printk(DRIVER_ZONE "S=%d P=%d chg=%d cable=%d%d%d flg=%d%d%d%d dbg=%d%d%d%d fst_dischg=%d/%d [%u]\n",
		poweralg.charge_state,
		poweralg.capacity_01p,
		poweralg.charging_enable,
		poweralg.is_cable_in,
		poweralg.is_china_ac_in,
		poweralg.is_super_ac,
		poweralg.protect_flags.is_charging_enable_available,
		poweralg.protect_flags.is_charging_high_current_avaialble,
		poweralg.protect_flags.is_battery_dead,
		poweralg.protect_flags.is_charging_reverse_protect,
		config.debug_disable_shutdown,
		config.debug_fake_room_temp,
		config.debug_disable_hw_timer,
		config.debug_always_predict,
		poweralg.fst_discharge_capacity_01p,
		poweralg.fst_discharge_acr_mAh,
		BAHW_MyGetMSecs());
#else
	if (show_debug_message == TRUE)
		printk(DRIVER_ZONE "P=%d V=%d T=%d I=%d ACR=%d/%d KADC=%d charger=%d%d \n",
			poweralg.capacity_01p,
			poweralg.battery.voltage_mV,
			poweralg.battery.temp_01c,
			poweralg.battery.current_mA,
			poweralg.battery.charge_counter_mAh,
			poweralg.battery.charge_full_real_mAh,
			poweralg.battery.KADC_01p,
			poweralg.charging_source,
			poweralg.charging_enable);
#endif

	
	return TRUE;
}

void power_alg_init(struct poweralg_config_type *debug_config)
{
	poweralg.charge_state = CHARGE_STATE_UNKNOWN;
	poweralg.capacity_01p = 990;
	poweralg.last_capacity_01p = poweralg.capacity_01p;
	poweralg.fst_discharge_capacity_01p = 0;
	poweralg.fst_discharge_acr_mAh = 0;
	poweralg.is_need_calibrate_at_49p = TRUE;
	poweralg.is_need_calibrate_at_14p = TRUE;
	poweralg.is_charge_over_load = FALSE;
	poweralg.is_cable_in = FALSE;
	poweralg.is_china_ac_in = FALSE;
	poweralg.is_super_ac = FALSE;
	poweralg.is_voltage_stable = FALSE;
	poweralg.is_software_charger_timeout = FALSE;
	poweralg.is_superchg_software_charger_timeout = FALSE;
	poweralg.is_need_toggle_charger = FALSE;
	poweralg.last_charger_enable_toggled_time_ms = 0;
	poweralg.start_ktime = ktime_get_real();
	cable_remove_ktime = ktime_set(0, 0);

	if(get_cable_status() == CONNECT_TYPE_USB) {
		poweralg.is_cable_in = TRUE;
		poweralg.charging_source = CONNECT_TYPE_USB;
		ds2746_charger_control(ENABLE_SLOW_CHG);
	}
	else if (get_cable_status() == CONNECT_TYPE_AC) {
		poweralg.is_cable_in = TRUE;
		poweralg.is_china_ac_in = TRUE;
		poweralg.charging_source = CONNECT_TYPE_AC;
		ds2746_charger_control(ENABLE_FAST_CHG);
	}
	else if (get_cable_status() == CONNECT_TYPE_9V_AC) {
		poweralg.is_cable_in = TRUE;
		poweralg.is_china_ac_in = TRUE;
		poweralg.is_super_ac = TRUE;
		poweralg.charging_source = CONNECT_TYPE_9V_AC;
		ds2746_charger_control(ENABLE_SUPER_CHG);
	} else {
		poweralg.charging_source = CONNECT_TYPE_NONE;
		ds2746_charger_control(DISABLE);
	}
	if (poweralg.pdata && poweralg.pdata->func_poweralg_config_init)
		poweralg.pdata->func_poweralg_config_init(&config);
	else
		poweralg_config_init(&config);

#if (defined(CONFIG_MACH_PRIMODS) || defined(CONFIG_MACH_PROTOU) || defined(CONFIG_MACH_PROTODUG) || defined(CONFIG_MACH_MAGNIDS))
	 
	if (poweralg.battery.id_index!=BATTERY_ID_TWS_SDI_1650MAH &&
		poweralg.battery.id_index!=BATTERY_ID_FORMOSA_SANYO) {
			config.full_charging_mv = 4110;
			config.voltage_recharge_mv = 4150;
			config.voltage_exit_full_mv = 4000;
		}
#endif

	if (debug_config->debug_disable_shutdown)
		config.debug_disable_shutdown = debug_config->debug_disable_shutdown;
	if (debug_config->debug_fake_room_temp)
		config.debug_fake_room_temp = debug_config->debug_fake_room_temp;
	if (debug_config->debug_disable_hw_timer)
		config.debug_disable_hw_timer = debug_config->debug_disable_hw_timer;
	if (debug_config->debug_always_predict)
		config.debug_always_predict = debug_config->debug_always_predict;



	poweralg.protect_flags.is_charging_enable_available = TRUE;
	poweralg.protect_flags.is_battery_dead = FALSE;
	poweralg.protect_flags.is_charging_high_current_avaialble = FALSE;
	poweralg.protect_flags.is_fake_room_temp = config.debug_fake_room_temp;
	poweralg.protect_flags.is_charging_reverse_protect = FALSE;
	poweralg.protect_flags.func_update_charging_protect_flag = NULL;

	battery_param_init(&poweralg.battery);

	
}

void power_alg_preinit(void)
{
	
}

static BLOCKING_NOTIFIER_HEAD(ds2746_notifier_list);
int ds2746_register_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&ds2746_notifier_list, nb);
}

int ds2746_unregister_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&ds2746_notifier_list, nb);
}


int ds2746_blocking_notify(unsigned long val, void *v)
{
	int chg_ctl;

	if (val == DS2746_CHARGING_CONTROL){
		chg_ctl = *(int *) v;
		 if (poweralg.battery.id_index != BATTERY_ID_UNKNOWN && (TOGGLE_CHARGER == chg_ctl || ENABLE_MIN_TAPER == chg_ctl || DISABLE_MIN_TAPER == chg_ctl)) {
			if (0 == poweralg.charging_enable)
				return 0;
		} else if (poweralg.battery.id_index != BATTERY_ID_UNKNOWN && poweralg.charge_state != CHARGE_STATE_PREDICTION) {
			
			if (g_first_update_charger_ctl == 1) {
				printk(DRIVER_ZONE "first update charger control forcely.\n");
				g_first_update_charger_ctl = 0;
				poweralg.charging_enable = chg_ctl;
			} else if (poweralg.charging_enable == chg_ctl && force_set_chg == 0) {
				return 0;
			} else if (force_set_chg == 1) {
				force_set_chg = 0;
				poweralg.charging_enable = chg_ctl;
			} else
				poweralg.charging_enable = chg_ctl;
		} else {
			if (poweralg.charging_enable == DISABLE) {
				
			} else {
				poweralg.charging_enable = DISABLE;
				v = DISABLE;
			}
			printk(DRIVER_ZONE "Charging disable due to Unknown battery\n");
		}
	}
	return blocking_notifier_call_chain(&ds2746_notifier_list, val, v);
}


int ds2746_get_battery_info(struct battery_info_reply *batt_info)
{
	batt_info->batt_id = poweralg.battery.id_index; 
	batt_info->batt_vol = poweralg.battery.voltage_mV; 
	batt_info->batt_temp = poweralg.battery.temp_01c; 
	batt_info->batt_current = poweralg.battery.current_mA; 
	batt_info->level = CEILING(poweralg.capacity_01p, 10); 
	batt_info->charging_source = poweralg.charging_source;
	batt_info->charging_enabled = poweralg.charging_enable;
	batt_info->full_bat = poweralg.battery.charge_full_capacity_mAh;
	batt_info->temp_fault = poweralg.protect_flags.is_temperature_fault;
	batt_info->thermal_temp = poweralg.battery.thermal_temp;
	batt_info->batt_state = poweralg.is_gauge_driver_ready;
	
	if (config.debug_fake_room_temp && (680 < poweralg.battery.temp_01c))
		batt_info->batt_temp = 680; 

	return 0;
}
ssize_t htc_battery_show_attr(struct device_attribute *attr, char *buf)
{
	int len = 0;

	if (!strcmp(attr->attr.name, "batt_attr_text")){
		len += scnprintf(buf +
				len,
				PAGE_SIZE -
				len,
				"Percentage(%%): %d;\n"
				"KADC(%%): %d;\n"
				"RARC(%%): %d;\n"
				"V_MBAT(mV): %d;\n"
				"Battery_ID: %d;\n"
				"pd_M: %d;\n"
				"Current(mA): %d;\n"
				"Temp: %d;\n"
				"Thermal_temp: %d;\n"
				"Charging_source: %d;\n"
				"ACR(mAh): %d;\n"
				"FULL(mAh): %d;\n"
				"1st_dis_percentage(%%): %d;\n"
				"1st_dis_ACR: %d;\n"
				"config_dbg: %d%d%d%d;\n",
				CEILING(poweralg.capacity_01p, 10),
				CEILING(poweralg.battery.KADC_01p, 10),
				CEILING(poweralg.battery.RARC_01p, 10),
				poweralg.battery.voltage_mV,
				poweralg.battery.id_index,
				poweralg.battery.pd_m,
				poweralg.battery.current_mA,
				CEILING(poweralg.battery.temp_01c, 10),
				poweralg.battery.thermal_temp,
				poweralg.charging_source,
				poweralg.battery.charge_counter_mAh,
				poweralg.battery.charge_full_real_mAh,
				CEILING(poweralg.fst_discharge_capacity_01p, 10),
				poweralg.fst_discharge_acr_mAh,
				config.debug_disable_shutdown,
				config.debug_fake_room_temp,
				config.debug_disable_hw_timer,
				config.debug_always_predict
		);
	}
	return len;
}

static void ds2746_program_alarm(struct ds2746_device_info *di, int seconds)
{
	ktime_t low_interval = ktime_set(seconds, 0);
	ktime_t slack = ktime_set(1, 0);
	ktime_t next;

	di->last_poll = alarm_get_elapsed_realtime();
	next = ktime_add(di->last_poll, low_interval);

	delta_time_sec = seconds;
	printk(DRIVER_ZONE "%s:last_poll = %lld + %d s = %lld\n",
		__func__,ktime_to_ns(di->last_poll),seconds,ktime_to_ns(next));
	alarm_start_range(&di->alarm, next, ktime_add(next, slack));
}

static int cable_status_handler_func(struct notifier_block *nfb,
	unsigned long action, void *param)
{
	u32 cable_type = (u32) action;
	u32 smem_cable_type = (u32)get_cable_type();

	printk(DRIVER_ZONE "%s(%d)\n",__func__, cable_type);

        if (cable_type != smem_cable_type && cable_type < CONNECT_TYPE_MAX) {
                printk(DRIVER_ZONE "update to smem_cable_type(%d)\n", smem_cable_type);
                cable_type = smem_cable_type;
        }

	if (cable_type == CONNECT_TYPE_NONE) {
		poweralg.is_cable_in = 0;
		poweralg.is_china_ac_in = 0;
		poweralg.is_super_ac = 0;
		poweralg.charging_source = cable_type;
		cable_remove_ktime = ktime_get_real();
		chg_en_time_sec = super_chg_on_time_sec = delta_time_sec = chg_kick_time_sec = 0;
		force_update_batt_info = 1;
		if (TRUE == poweralg.is_superchg_software_charger_timeout) {
			poweralg.is_superchg_software_charger_timeout = FALSE;	
			printk(DRIVER_ZONE "reset superchg software timer\n");
		}
		if (!is_charging_avaiable()) {
			poweralg.protect_flags.is_charging_reverse_protect = FALSE;
		}
		
		if (g_di_ptr) {
			alarm_try_to_cancel(&g_di_ptr->alarm);
			ds2746_program_alarm(g_di_ptr, 0);
		}
		else {
			printk(DRIVER_ZONE "charger out but no di ptr.\n");
		}
	} else if (cable_type == CONNECT_TYPE_USB) {
		poweralg.is_cable_in = 1;
		poweralg.is_china_ac_in = 0;
		poweralg.is_super_ac = 0;
		poweralg.charging_source = cable_type;
		cable_remove_ktime = ktime_get_real();
		chg_en_time_sec = super_chg_on_time_sec = delta_time_sec = chg_kick_time_sec = 0;
		force_update_batt_info = 1;
		force_set_chg = 1;
		
		if (g_di_ptr) {
			alarm_try_to_cancel(&g_di_ptr->alarm);
			ds2746_program_alarm(g_di_ptr, 0);
		}
		else {
			printk(DRIVER_ZONE "charger in but no di ptr.\n");
		}
	} else if (cable_type == CONNECT_TYPE_AC) {
		poweralg.is_cable_in = 1;
		poweralg.is_china_ac_in = 1;
		poweralg.is_super_ac = 0;
		poweralg.charging_source = cable_type;
		cable_remove_ktime = ktime_get_real();
		chg_en_time_sec = super_chg_on_time_sec = delta_time_sec = chg_kick_time_sec = 0;
		force_update_batt_info = 1;
		force_set_chg = 1;
		
		if (g_di_ptr) {
			alarm_try_to_cancel(&g_di_ptr->alarm);
			ds2746_program_alarm(g_di_ptr, 0);
		}
		else {
			printk(DRIVER_ZONE "charger in but no di ptr.\n");
		}
	} else if (cable_type == CONNECT_TYPE_9V_AC) {
		poweralg.is_cable_in = 1;
		poweralg.is_china_ac_in = 1;
		poweralg.is_super_ac = 1;
		poweralg.charging_source = cable_type;
		chg_en_time_sec = super_chg_on_time_sec = delta_time_sec = chg_kick_time_sec = 0;
		force_update_batt_info = 1;
		force_set_chg = 1;
		
		if (g_di_ptr) {
			alarm_try_to_cancel(&g_di_ptr->alarm);
			ds2746_program_alarm(g_di_ptr, 0);
		}
		else {
			printk(DRIVER_ZONE "charger in but no di ptr.\n");
		}
	} else if (cable_type == 0xff) {
		if (param)
			config.full_level = *(INT32 *)param;
		printk(DRIVER_ZONE "Set the full level to %d\n", config.full_level);
		return NOTIFY_OK;
	} else if (cable_type == 0x10) {
		poweralg.protect_flags.is_fake_room_temp = TRUE;
		printk(DRIVER_ZONE "enable fake temp mode\n");
		return NOTIFY_OK;
	}

	return NOTIFY_OK;
}

void reverse_protection_handler(int status)
{
	if (status == REVERSE_PROTECTION_HAPPEND) {
		if (poweralg.charging_source != CONNECT_TYPE_NONE) {
			poweralg.protect_flags.is_charging_reverse_protect = TRUE;
			reverse_protecion_counter++;
			printk(DRIVER_ZONE "%s: reverse protection is happened: %d\n",__func__,reverse_protecion_counter);
		}
	}
	else if (status == REVERSE_PROTECTION_CONTER_CLEAR) {
		reverse_protecion_counter = 0;
	}
}
EXPORT_SYMBOL(reverse_protection_handler);

static struct notifier_block cable_status_handler =
{
  .notifier_call = cable_status_handler_func,
};

void ds2746_charger_control(int type)
{
	int charge_type = type;
	printk(DRIVER_ZONE "%s(%d)\n",__func__, type);

	switch (charge_type){
		case DISABLE:
			
			ds2746_blocking_notify(DS2746_CHARGING_CONTROL, &charge_type);
			break;
		case ENABLE_SLOW_CHG:
			ds2746_blocking_notify(DS2746_CHARGING_CONTROL, &charge_type);
			break;
		case ENABLE_FAST_CHG:
			ds2746_blocking_notify(DS2746_CHARGING_CONTROL, &charge_type);
			break;
		case ENABLE_SUPER_CHG:
			ds2746_blocking_notify(DS2746_CHARGING_CONTROL, &charge_type);
			break;
		case TOGGLE_CHARGER:
			ds2746_blocking_notify(DS2746_CHARGING_CONTROL, &charge_type);
			break;
		case ENABLE_MIN_TAPER:
			ds2746_blocking_notify(DS2746_CHARGING_CONTROL, &charge_type);
			break;
		case DISABLE_MIN_TAPER:
			ds2746_blocking_notify(DS2746_CHARGING_CONTROL, &charge_type);
			break;
	}
}


static void ds2746_battery_work(struct work_struct *work)
{
	struct ds2746_device_info *di = container_of(work,
				struct ds2746_device_info, monitor_work.work); 
	static int alarm_delta_ready = 0;
	unsigned long flags;

	if (!alarm_delta_ready && !alarm_delta_is_ready()) {
		printk(DRIVER_ZONE "alarm delta isn't ready so delay 500ms\n");
		cancel_delayed_work(&di->monitor_work);
		queue_delayed_work(di->monitor_wqueue, &di->monitor_work, msecs_to_jiffies(500));
		return;
	} else
		alarm_delta_ready = 1;

	do_power_alg(0);
	last_poll_ktime = ktime_get_real();
	get_state_check_interval_min_sec();
	di->last_poll = alarm_get_elapsed_realtime();

	
	spin_lock_irqsave(&di->spin_lock, flags);

	wake_unlock(&di->work_wake_lock);
	if (poweralg.battery.is_power_on_reset)
		ds2746_program_alarm(di, PREDIC_POLL);
	else
		ds2746_program_alarm(di, FAST_POLL);

	spin_unlock_irqrestore(&di->spin_lock, flags);
}

static void ds2746_battery_alarm(struct alarm *alarm)
{
	struct ds2746_device_info *di = container_of(alarm, struct ds2746_device_info, alarm);
	wake_lock(&di->work_wake_lock);
	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, 0);
	
}

static int ds2746_battery_probe(struct platform_device *pdev)
{
	int rc;
	struct ds2746_device_info *di;
	ds2746_platform_data *pdata = pdev->dev.platform_data; 
	poweralg.pdata = pdev->dev.platform_data;
	poweralg.battery.thermal_id = pdata->func_get_thermal_id();
	if ((pdata->func_get_battery_id != NULL) && (0 < pdata->func_get_battery_id())) {
#if (defined(CONFIG_MACH_PRIMODS) || defined(CONFIG_MACH_PROTOU) || defined(CONFIG_MACH_PROTODUG) || defined(CONFIG_MACH_MAGNIDS))
		poweralg.battery.id_index = get_batt_id();
#else
		poweralg.battery.id_index = pdata->func_get_battery_id();
#endif
		if (NULL != poweralg.pdata->batt_param->fl_25) {
			poweralg.battery.charge_full_design_mAh =
				poweralg.pdata->batt_param->fl_25[poweralg.battery.id_index];
		} else
			poweralg.battery.charge_full_design_mAh = DS2746_FULL_CAPACITY_DEFAULT;
		poweralg.battery.charge_full_real_mAh = poweralg.battery.charge_full_design_mAh;
		is_need_battery_id_detection = FALSE;

		if (NULL != poweralg.pdata->batt_param->capacity) {
			poweralg.battery.charge_full_capacity_mAh =
				poweralg.pdata->batt_param->capacity[poweralg.battery.id_index];
		} else
			poweralg.battery.charge_full_capacity_mAh = poweralg.battery.charge_full_design_mAh;
	}
	else {
		poweralg.battery.id_index = BATTERY_ID_UNKNOWN;
		is_need_battery_id_detection = TRUE;
	}

	power_alg_preinit();
	power_alg_init(&debug_config);
	
	poweralg.protect_flags.func_update_charging_protect_flag = pdata->func_update_charging_protect_flag;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di){
		rc = -ENOMEM;
		goto fail_register;
	}

	di->update_time = jiffies;
	platform_set_drvdata(pdev, di);

	di->dev = &pdev->dev;

	
	INIT_DELAYED_WORK(&di->monitor_work, ds2746_battery_work);
	di->monitor_wqueue = create_singlethread_workqueue(dev_name(&pdev->dev));

	
	di->last_poll = alarm_get_elapsed_realtime();
	spin_lock_init(&di->spin_lock);

	if (!di->monitor_wqueue){
		rc = -ESRCH;
		goto fail_workqueue;
	}
	wake_lock_init(&di->work_wake_lock, WAKE_LOCK_SUSPEND, "ds2746-battery");
	alarm_init(&di->alarm,
		ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP,
		ds2746_battery_alarm);
	wake_lock(&di->work_wake_lock);
	if (alarm_delta_is_ready()) {
		printk(DRIVER_ZONE "alarm delta is ready\n");
		queue_delayed_work(di->monitor_wqueue, &di->monitor_work, 0);
		
	} else {
		printk(DRIVER_ZONE "[probe] alarm delta isn't ready so delay 500ms\n");
		queue_delayed_work(di->monitor_wqueue, &di->monitor_work, msecs_to_jiffies(500));
	}

	g_di_ptr = di; 
	return 0;

	fail_workqueue : fail_register : kfree(di);
	return rc;
}

int ds2746_charger_switch(int charger_switch)
{
	printk(DRIVER_ZONE "%s: charger_switch=%d\n",
		__func__, charger_switch);

	if (charger_switch == 0) {
		chg_en_time_sec = 0;
		chg_kick_time_sec = 0;
		super_chg_on_time_sec = 0;
		poweralg.is_need_toggle_charger = FALSE;
		poweralg.protect_flags.is_charging_reverse_protect = FALSE;
		charger_control = 1;
	} else {
		charger_control = 0;
	}
	if (g_di_ptr) {
		alarm_try_to_cancel(&g_di_ptr->alarm);
		ds2746_program_alarm(g_di_ptr, 0);
	}

	return 0;

}
EXPORT_SYMBOL(ds2746_charger_switch);

static int ds2746_battery_remove(struct platform_device *pdev)
{
	struct ds2746_device_info *di = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&di->monitor_work);
	
	destroy_workqueue(di->monitor_wqueue);

	return 0;
}

void ds2746_phone_call_in(int phone_call_in)
{
	set_phone_call_in_poll = phone_call_in;
}
static int ds2746_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ds2746_device_info *di = platform_get_drvdata(pdev);
	unsigned long flags;

	if (poweralg.charging_source == CONNECT_TYPE_NONE) {
		spin_lock_irqsave(&di->spin_lock, flags);
		if (set_phone_call_in_poll) {
			ds2746_program_alarm(di, PHONE_CALL_POLL);
			di->slow_poll = 1;
		} else {
			ds2746_program_alarm(di, SLOW_POLL);
			di->slow_poll = 1;
		}
		spin_unlock_irqrestore(&di->spin_lock, flags);
	}
	
	return 0;
}
static void ds2746_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ds2746_device_info *di = platform_get_drvdata(pdev);
	unsigned long flags;
	ktime_t now_time_ktime;

	
	
	if (di->slow_poll){
		spin_lock_irqsave(&di->spin_lock, flags);
		if ((MSPERIOD(ktime_get_real(), last_poll_ktime) >=
		     FAST_POLL * 1000)) {
		     ds2746_program_alarm(di, 0);
		} else if ((MSPERIOD(ktime_get_real(), last_poll_ktime) < 0)) {
			 now_time_ktime = ktime_get_real();
			 ds2746_program_alarm(di, 0);
			 printk(DRIVER_ZONE "current time[%lld] - last_poll_time [%lld] is negative\n", ktime_to_ms(now_time_ktime), ktime_to_ms(last_poll_ktime));
		} else
			ds2746_program_alarm(di, FAST_POLL);
		di->slow_poll = 0;
		spin_unlock_irqrestore(&di->spin_lock, flags);
	}
}

static struct dev_pm_ops ds2746_pm_ops = {
       .prepare = ds2746_suspend,
       .complete  = ds2746_resume,
};

MODULE_ALIAS("platform:ds2746-battery");
static struct platform_driver ds2746_battery_driver =
{
	.driver = {
	.name = "ds2746-battery",
	.pm = &ds2746_pm_ops,
	},
	.probe = ds2746_battery_probe,
	.remove = ds2746_battery_remove,
};

static int __init ds2746_fake_temp_setup(char *str)
{
	if(!strcmp(str,"true"))
		debug_config.debug_fake_room_temp = TRUE;
	else
		debug_config.debug_fake_room_temp = FALSE;
	return 1;
}
__setup("battery_fake_temp=", ds2746_fake_temp_setup);

static int __init ds2746_battery_init(void)
{
	int ret;

	charger_control = 0;

	wake_lock_init(&vbus_wake_lock, WAKE_LOCK_SUSPEND, "vbus_present");
	register_notifier_cable_status(&cable_status_handler);

	ret = ds2746_i2c_init();
	if (ret < 0){
		return ret;
	}

	
	return platform_driver_register(&ds2746_battery_driver);
}

static void __exit ds2746_battery_exit(void)
{
	ds2746_i2c_exit();
	platform_driver_unregister(&ds2746_battery_driver);
}

module_init(ds2746_battery_init);
module_exit(ds2746_battery_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andy.YS Wang  <Andy.ys_wang@htc.com>");
MODULE_DESCRIPTION("ds2746 battery driver");

