/*
 * Copyright (C) 2007 HTC Incorporated
 * Author: Jay Tu (jay_tu@htc.com)
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
#ifndef _HTC_BATTERY_H_
#define _HTC_BATTERY_H_
#include <linux/notifier.h>
#include <linux/device.h>
#include <linux/power_supply.h>
#include <mach/htc_battery_common.h>

extern unsigned int system_rev;

enum batt_ctl_t {
	DISABLE = 0,
	ENABLE_SLOW_CHG,
	ENABLE_FAST_CHG,
	ENABLE_SUPER_CHG,
	ENABLE_WIRELESS_CHG,
	CHARGER_CHK,
	TOGGLE_CHARGER,
	ENABLE_MIN_TAPER,
	DISABLE_MIN_TAPER
};

enum {
	GUAGE_NONE,
	GUAGE_MODEM,
	GUAGE_DS2784,
	GUAGE_DS2746,
	GAUGE_MAX17050,
};

enum {
	LINEAR_CHARGER,
	SWITCH_CHARGER,
	SWITCH_CHARGER_TPS65200,
};

enum {
	MBAT_IN_NO_IRQ = 0,
	MBAT_IN_LOW_TRIGGER,
	MBAT_IN_HIGH_TRIGGER,
};

enum {
	OPTION_FLAG_BT_DOCK_CHARGE_CTL = 1,
};

enum htc_batt_rt_attr {
	HTC_BATT_RT_VOLTAGE = 0,
	HTC_BATT_RT_CURRENT,
	HTC_BATT_RT_TEMPERATURE,
};

struct battery_info_reply {
	u32 batt_id;		
	u32 batt_vol;		
	s32 batt_temp;		
	s32 batt_current;	
	u32 level;		
	u32 charging_source;	
	u32 charging_enabled;	
	u32 full_bat;		
	u32 full_level;		
	u32 over_vchg;		
	s32 eval_current;	
	u32 temp_fault;		
	u32 overloading_charge; 
	u32 thermal_temp; 
	u32 batt_state;
};

struct htc_battery_platform_data {
	int (*func_get_batt_rt_attr)(enum htc_batt_rt_attr attr, int* val);
	int (*func_show_batt_attr)(struct device_attribute *attr,
					 char *buf);
	int (*func_show_htc_extension_attr)(struct device_attribute *attr,
					 char *buf);
	int gpio_mbat_in;
	int gpio_mbat_in_trigger_level;
	int mbat_in_keep_charging;
	int mbat_in_unreg_rmt;

	int gpio_usb_id;
	int gpio_mchg_en_n;
	int gpio_iset;
	int gpio_adp_9v;
	int guage_driver;
	int m2a_cable_detect;
	int charger;
	unsigned int option_flag;
	int (*func_is_support_super_charger)(void);
	int (*func_battery_charging_ctrl)(enum batt_ctl_t ctl);
	int (*func_battery_gpio_init)(void);
	int charger_re_enable;
	int chg_limit_active_mask;
	int suspend_highfreq_check_reason;
	int enable_bootup_voltage;
#ifdef CONFIG_CPLD
	int cpld_hw_chg_led;
#endif
};

extern int register_notifier_cable_status(struct notifier_block *nb);
extern int unregister_notifier_cable_status(struct notifier_block *nb);

extern int register_notifier_wireless_charger(struct notifier_block *nb);
extern int unregister_notifier_wireless_charger(struct notifier_block *nb);

extern int register_notifier_cable_rpc(struct notifier_block *nb);
extern int unregister_notifier_cable_rpc(struct notifier_block *nb);

extern int htc_is_wireless_charger(void);

#ifdef CONFIG_BATTERY_DS2784
extern int battery_charging_ctrl(enum batt_ctl_t ctl);
#endif
extern int get_cable_status(void);

extern unsigned int batt_get_status(enum power_supply_property psp);

#if (defined(CONFIG_BATTERY_DS2746) || defined(CONFIG_BATTERY_MAX17050))
int htc_battery_update_change(int force_update);
#if (defined(CONFIG_MACH_PRIMODS) || defined(CONFIG_MACH_PROTOU) || defined(CONFIG_MACH_PROTODUG) || defined(CONFIG_MACH_PROTODCG) || defined(CONFIG_MACH_MAGNIDS) || defined(CONFIG_MACH_CP3DUG) || defined(CONFIG_MACH_CP3DTG) || defined(CONFIG_MACH_CP3DCG) || defined(CONFIG_MACH_CP3U) || defined(CONFIG_MACH_Z4DUG) || defined(CONFIG_MACH_Z4DCG) || defined(CONFIG_MACH_Z4U))
extern int get_batt_id(void); 
extern void set_smem_chg_avalible(int chg_avalible);
#endif
extern int get_cable_type(void); 
#endif
#ifdef CONFIG_THERMAL_TEMPERATURE_READ
extern int htc_get_thermal_adc_level(uint32_t *buffer);
#endif
extern int unregister_rmt_reboot_notifier(void);
#endif
