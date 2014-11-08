/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio_event.h>
#include <linux/memblock.h>
#include <asm/mach-types.h>
#include <linux/memblock.h>
#include <asm/mach/arch.h>
#include <asm/hardware/gic.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#ifdef CONFIG_USB_MSM_OTG_72K
#include <mach/msm_hsusb.h>
#else
#include <linux/usb/msm_hsusb.h>
#endif
#include <mach/htc_usb.h>
#include <mach/rpc_hsusb.h>
#include <mach/rpc_pmapp.h>
#ifdef CONFIG_USB_G_ANDROID
#include <linux/usb/android_composite.h>
#include <mach/usbdiag.h>
#endif
#include <mach/msm_memtypes.h>
#include <mach/msm_serial_hs.h>
#include <linux/usb/android.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <mach/vreg.h>
#include <linux/leds-pm8029.h>
#include <mach/pmic.h>
#include <mach/socinfo.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <asm/mach/mmc.h>
#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>
#include <linux/gpio.h>
#include <linux/android_pmem.h>
#include <linux/bootmem.h>
#include <linux/mfd/marimba.h>
#include <mach/vreg.h>
#include <linux/power_supply.h>
#include <linux/regulator/consumer.h>
#include <mach/rpc_pmapp.h>
#ifdef CONFIG_BATTERY_MSM
#include <mach/msm_battery.h>
#endif
#include <linux/max17050_battery.h>
#include <linux/tps65200.h>
#include <mach/system.h>
#include <mach/htc_battery.h>
#include <linux/msm_adc.h>
#include <linux/himax8526a.h>
#include <mach/htc_headset_mgr.h>
#include <mach/htc_headset_gpio.h>
#include <mach/htc_headset_pmic.h>
#include <mach/htc_headset_one_wire.h>
#include <asm/setup.h>
#include "devices.h"
#include "timer.h"
#include "board-msm7x27a-regulator.h"
#include "devices-msm7x2xa.h"
#include <mach/board_htc.h>
#include "pm.h"
#include <mach/rpc_server_handset.h>
#include <mach/socinfo.h>
#include <mach/msm_rtb.h>
#include <linux/bma250.h>
#include "pm-boot.h"
#include <linux/proc_fs.h>
#include <linux/cm3629.h>
#include <linux/pn544.h>
#include <linux/htc_flashlight.h>
#ifdef CONFIG_PERFLOCK
#include <mach/perflock.h>
#endif
#ifdef CONFIG_BT
#include <mach/htc_bdaddress.h>
#endif
#include "board-magnids.h"
#include "board-bcm4330-wifi.h"
#include <mach/htc_util.h>
#include <mach/TCA6418_ioextender.h>
#define CPU_FOOT_PRINT (MSM_HTC_DEBUG_INFO_BASE + 0x0)

#define MSM8625_SECONDARY_PHYS		0x0FE00000

int htc_get_usb_accessory_adc_level(uint32_t *buffer);
#include <mach/cable_detect.h>

extern int emmc_partition_read_proc(char *page, char **start, off_t off,
					int count, int *eof, void *data);

#if defined(CONFIG_GPIO_SX150X)
enum {
	SX150X_CORE,
};

static struct sx150x_platform_data sx150x_data[] __initdata = {
	[SX150X_CORE]	= {
		.gpio_base		= GPIO_CORE_EXPANDER_BASE,
		.oscio_is_gpo		= false,
		.io_pullup_ena		= 0,
		.io_pulldn_ena		= 0x02,
		.io_open_drain_ena	= 0xfef8,
		.irq_summary		= -1,
	},
};
#endif


#if 0
static struct platform_device msm_wlan_ar6000_pm_device = {
	.name           = "wlan_ar6000_pm_dev",
	.id             = -1,
};
#endif


#if defined(CONFIG_I2C) && defined(CONFIG_GPIO_SX150X)
static struct i2c_board_info core_exp_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("sx1509q", 0x3e),
	},
};

static void __init register_i2c_devices(void)
{
	if (machine_is_msm7x27a_surf() || machine_is_msm7625a_surf() || machine_is_msm8625_surf() || machine_is_magnids())
		sx150x_data[SX150X_CORE].io_open_drain_ena = 0xe0f0;

	core_exp_i2c_info[0].platform_data =
			&sx150x_data[SX150X_CORE];

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				core_exp_i2c_info,
				ARRAY_SIZE(core_exp_i2c_info));
}
#endif

#if defined(CONFIG_INPUT_CAPELLA_CM3629)
#define MAGNIDS_GPIO_PROXIMITY_INT 36
static struct cm3629_platform_data cm36282_pdata = {
        .model = CAPELLA_CM36282,
        .ps_select = CM3629_PS1_ONLY,
        .intr = MAGNIDS_GPIO_PROXIMITY_INT,
        .levels = {12, 14, 16, 476, 1130, 3922, 6795, 9795, 12795, 65535},
        .golden_adc = 0x131B,
        .power = NULL,
        .cm3629_slave_address = 0xC0>>1,
        .ps_calibration_rule = 1,
        .ps1_thd_set = 0x08,
        .ps1_thh_diff = 2,
        .ps1_thd_no_cal = 0xF1,
        .ps1_thd_with_cal = 0x08,
        .ps_conf1_val = CM3629_PS_DR_1_320 | CM3629_PS_IT_1_6T |
                        CM3629_PS1_PERS_1,
        .ps_conf2_val = CM3629_PS_ITB_1 | CM3629_PS_ITR_1 |
                        CM3629_PS2_INT_DIS | CM3629_PS1_INT_DIS,
        .ps_conf3_val = CM3629_PS2_PROL_32,

};

static struct i2c_board_info i2c_cm36282_devices[] = {
	{
		I2C_BOARD_INFO(CM3629_I2C_NAME,0xC0 >> 1),
        .platform_data = &cm36282_pdata,
        .irq = MSM_GPIO_TO_INT(MAGNIDS_GPIO_PROXIMITY_INT),
	},
};
#endif

static uint32_t qup_i2c_gpio_table_io[] = {
	GPIO_CFG(60, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(61, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(131, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(132, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	};

static uint32_t qup_i2c_gpio_table_hw[] = {
	GPIO_CFG(60, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(61, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(131, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(132, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	};

static void gsbi_qup_i2c_gpio_config(int adap_id, int config_type)
{
	if (adap_id < 0 || adap_id > 1)
		return;

	pr_info("%s: adap_id = %d, config_type = %d\n",
		__func__, adap_id, config_type);

	if (config_type){
		gpio_tlmm_config(qup_i2c_gpio_table_hw[adap_id*2], GPIO_CFG_ENABLE);
		gpio_tlmm_config(qup_i2c_gpio_table_hw[adap_id*2 + 1], GPIO_CFG_ENABLE);
	} else {
		gpio_tlmm_config(qup_i2c_gpio_table_io[adap_id*2], GPIO_CFG_ENABLE);
		gpio_tlmm_config(qup_i2c_gpio_table_io[adap_id*2 + 1], GPIO_CFG_ENABLE);
	}
}

static struct msm_i2c_platform_data msm_gsbi0_qup_i2c_pdata = {
	.clk_freq		= 400000,
	.msm_i2c_config_gpio	= gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi1_qup_i2c_pdata = {
	.clk_freq		= 400000,
	.msm_i2c_config_gpio	= gsbi_qup_i2c_gpio_config,
};

static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id              = 0x0bb4,
	.product_id             = 0x0d05,
	.version                = 0x0100,
	.product_name           = "Android Phone",
	.manufacturer_name      = "HTC",
	.num_products           = ARRAY_SIZE(usb_products),
	.products               = usb_products,
	.num_functions          = ARRAY_SIZE(usb_functions_all),
	.functions              = usb_functions_all,
	.usb_id_pin_gpio	= MAGNIDS_GPIO_USB_ID_PIN,
	.usb_diag_interface	= "diag",
	.fserial_init_string = "tty:modem,tty:,tty:serial,tty:",
	.nluns                  = 2,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};

#ifdef CONFIG_USB_EHCI_MSM_72K
static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{
	int rc = 0;
	unsigned gpio;

	gpio = GPIO_HOST_VBUS_EN;

	rc = gpio_request(gpio, "i2c_host_vbus_en");
	if (rc < 0) {
		pr_err("failed to request %d GPIO\n", gpio);
		return;
	}
	gpio_direction_output(gpio, !!on);
	gpio_set_value_cansleep(gpio, !!on);
	gpio_free(gpio);
}

static struct msm_usb_host_platform_data msm_usb_host_pdata = {
	.phy_info       = (USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM),
};

static void __init msm7x2x_init_host(void)
{
	msm_add_host(0, &msm_usb_host_pdata);
}
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static int hsusb_rpc_connect(int connect)
{
	if (connect)
		return msm_hsusb_rpc_connect();
	else
		return msm_hsusb_rpc_close();
}

static struct regulator *reg_hsusb;
static int msm_hsusb_ldo_init(int init)
{
	int rc = 0;

	if (init) {
		reg_hsusb = regulator_get(NULL, "usb");
		if (IS_ERR(reg_hsusb)) {
			rc = PTR_ERR(reg_hsusb);
			pr_err("%s: could not get regulator: %d\n",
					__func__, rc);
			goto out;
		}

		rc = regulator_set_voltage(reg_hsusb, 3300000, 3300000);
		if (rc) {
			pr_err("%s: could not set voltage: %d\n",
					__func__, rc);
			goto reg_free;
		}

		return 0;
	}
	/* else fall through */
reg_free:
	regulator_put(reg_hsusb);
out:
	reg_hsusb = NULL;
	return rc;
}

static int msm_hsusb_ldo_enable(int enable)
{
	static int ldo_status;

	if (IS_ERR_OR_NULL(reg_hsusb))
		return reg_hsusb ? PTR_ERR(reg_hsusb) : -ENODEV;

	if (ldo_status == enable)
		return 0;

	ldo_status = enable;

	return enable ?
		regulator_enable(reg_hsusb) :
		regulator_disable(reg_hsusb);
}

#ifndef CONFIG_USB_EHCI_MSM_72K
static int msm_hsusb_pmic_notif_init(void (*callback)(int online), int init)
{
	int ret = 0;

	if (init)
		ret = msm_pm_app_rpc_init(callback);
	else
		msm_pm_app_rpc_deinit(callback);

	return ret;
}
#endif
#endif

#ifdef CONFIG_USB_GADGET_MSM_72K
static int phy_init_settings[] =
{	0x0c, 0x31,
	0x28, 0x31,
        -1
};
#endif

static struct msm_otg_platform_data msm_otg_pdata = {
        .vbus_power              = msm_hsusb_vbus_power,
        .rpc_connect             = hsusb_rpc_connect,
        .pemp_level              = PRE_EMPHASIS_WITH_20_PERCENT,
        .cdr_autoreset           = CDR_AUTO_RESET_DISABLE,
        .drv_ampl                = HS_DRV_AMPLITUDE_DEFAULT,
        .se1_gating              = SE1_GATING_DISABLE,
        .ldo_init                = msm_hsusb_ldo_init,
        .ldo_enable              = msm_hsusb_ldo_enable,
        .chg_init                = hsusb_chg_init,
        .chg_connected           = hsusb_chg_connected,
        .chg_vbus_draw           = hsusb_chg_vbus_draw,
};

#ifdef CONFIG_USB_GADGET_MSM_72K
static struct msm_hsusb_gadget_platform_data msm_gadget_pdata = {
	.phy_init_seq		= phy_init_settings,
	.is_phy_status_timer_on = 1,
};
#endif

void magnids_add_usb_devices(void)
{
	printk(KERN_INFO "%s rev: %d\n", __func__, system_rev);
	android_usb_pdata.products[0].product_id =
		android_usb_pdata.product_id;

	/* diag bit set */
	if (get_radio_flag() & 0x20000)
		android_usb_pdata.diag_init = 1;
	/* add cdrom support in normal mode */
	if (board_mfg_mode() == 0) {
		android_usb_pdata.nluns = 3;
		android_usb_pdata.cdrom_lun = 0x4;
	}

	msm8625_device_gadget_peripheral.dev.parent = &msm8625_device_otg.dev;
	platform_device_register(&msm8625_device_gadget_peripheral);
	platform_device_register(&android_usb_device);
}

static int get_thermal_id(void)
{
	return THERMAL_470_100_4360;
}
static int get_battery_id(void)
{
	return BATTERY_ID_FORMOSA_SANYO;
}

static void magnids_poweralg_config_init(struct poweralg_config_type *config)
{
	INT32 batt_id = 0;

	batt_id = get_batt_id();
	if (batt_id == BATTERY_ID_FORMOSA_SANYO) {
		pr_info("[BATT] %s() is used, batt_id -> %d\n",__func__, batt_id);
		config->full_charging_mv = 4250;
		config->voltage_recharge_mv = 4290;
		config->voltage_exit_full_mv = 4200;
	} else {
		pr_info("[BATT] %s() is used, batt_id -> %d\n",__func__, batt_id);
		config->full_charging_mv = 4100;
		config->voltage_recharge_mv = 4150;
		config->voltage_exit_full_mv = 4000;
	}

	config->full_charging_ma = 50;
	config->full_pending_ma = 0;		/* disabled*/
	config->full_charging_timeout_sec = 60 * 60;
	config->min_taper_current_mv = 0;	/* disabled */
	config->min_taper_current_ma = 0;	/* disabled */
	config->wait_votlage_statble_sec = 1 * 60;
	config->predict_timeout_sec = 10;
	config->polling_time_in_charging_sec = 30;

	config->enable_full_calibration = TRUE;
	config->enable_weight_percentage = TRUE;
	config->software_charger_timeout_sec = 57600; /* 16hr */
	config->superchg_software_charger_timeout_sec = 0;	/* disabled */
	config->charger_hw_safety_timer_watchdog_sec =  0;	/* disabled */

	config->debug_disable_shutdown = FALSE;
	config->debug_fake_room_temp = FALSE;

	config->debug_disable_hw_timer = FALSE;
	config->debug_always_predict = FALSE;
	config->full_level = 0;
}

static int magnids_update_charging_protect_flag(int ibat_ma, int vbat_mv, int temp_01c, BOOL* chg_allowed, BOOL* hchg_allowed, BOOL* temp_fault)
{
	static int pState = 0;
	int old_pState = pState;
	/* pStates:
		0: initial (temp detection)
		1: temp < 0 degree c
		2: 0 <= temp <= 45 degree c
		3: 45 < temp <= 48 degree c
		4: 48 < temp <= 60 degree c
		5: 60 < temp
	*/
	enum {
		PSTAT_DETECT = 0,
		PSTAT_LOW_STOP,
		PSTAT_NORMAL,
		PSTAT_SLOW,
		PSTAT_LIMITED,
		PSTAT_HIGH_STOP
	};
	/* generally we assumed that pState implies last temp.
		it won't hold if temp changes faster than sample rate */

	// step 1. check if change state condition is hit
	pr_debug("[BATT] %s(i=%d, v=%d, t=%d, %d, %d)\n",__func__, ibat_ma, vbat_mv, temp_01c, *chg_allowed, *hchg_allowed);
	switch(pState) {
		default:
			pr_info("[BATT] error: unexpected pState\n");
		case PSTAT_DETECT:
			if (temp_01c < 0)
				pState = PSTAT_LOW_STOP;
			if ((0 <= temp_01c) && (temp_01c <= 450))
				pState = PSTAT_NORMAL;
			if ((450 < temp_01c) && (temp_01c <= 480))
				pState = PSTAT_SLOW;
			if ((480 < temp_01c) && (temp_01c <= 600))
				pState = PSTAT_LIMITED;
			if (600 < temp_01c)
				pState = PSTAT_HIGH_STOP;
			break;
		case PSTAT_LOW_STOP:
			if (30 <= temp_01c)
				pState = PSTAT_NORMAL;
			/* suppose never jump to LIMITED/HIGH_STOP from here */
			break;
		case PSTAT_NORMAL:
			if (temp_01c < 0)
				pState = PSTAT_LOW_STOP;
			else if (600 < temp_01c)
				pState = PSTAT_HIGH_STOP;
			else if (480 < temp_01c) /* also implies t <= 550 */
				pState = PSTAT_LIMITED;
			else if (450 < temp_01c)
				pState = PSTAT_SLOW;
			break;
		case PSTAT_SLOW:
			if (temp_01c < 0)
				pState = PSTAT_LOW_STOP;
			else if (600 < temp_01c)
				pState = PSTAT_HIGH_STOP;
			else if (480 < temp_01c)
				pState = PSTAT_LIMITED;
			else if (temp_01c < 420)
				pState = PSTAT_NORMAL;
			break;
		case PSTAT_LIMITED:
			if (temp_01c < 0)
				pState = PSTAT_LOW_STOP;
			else if (600 < temp_01c)
				pState = PSTAT_HIGH_STOP;
			else if (temp_01c < 450)
				pState = PSTAT_NORMAL;
			else if (temp_01c < 470)
				pState = PSTAT_SLOW;
			break;
		case PSTAT_HIGH_STOP:
			if (temp_01c < 420)
				pState = PSTAT_NORMAL;
			else if (temp_01c < 470)
				pState = PSTAT_SLOW;
			else if ((temp_01c <= 570) && (vbat_mv <= 3800))
				pState = PSTAT_LIMITED;
			/* suppose never jump to LOW_STOP from here */
			break;
	}
	if (old_pState != pState)
		pr_info("[BATT] Protect pState changed from %d to %d\n", old_pState, pState);

	/* step 2. check state protect condition */
	/* chg_allowed = TRUE:only means it's allowed no matter it has charger.
		same as hchg_allowed. */
	switch(pState) {
		default:
		case PSTAT_DETECT:
			pr_info("[BATT] error: unexpected pState\n");
			break;
		case PSTAT_LOW_STOP:
			*chg_allowed = FALSE;
			*hchg_allowed = FALSE;
			break;
		case PSTAT_NORMAL:
			*chg_allowed = TRUE;
			*hchg_allowed = TRUE;
			break;
		case PSTAT_SLOW:	/* 4.2V Charge Full, 4.15V recharge */
			if (4200 < vbat_mv)
				*chg_allowed = FALSE;
			else if (vbat_mv <= 4150)
				*chg_allowed = TRUE;
			*hchg_allowed = TRUE;
			break;
		case PSTAT_LIMITED:	/* 4.1V Charge Full, 3.8V recharge */
			if (PSTAT_LIMITED != old_pState)
				*chg_allowed = TRUE;
			if (4100 < vbat_mv)
				*chg_allowed = FALSE;
			else if (vbat_mv <= 3800)
				*chg_allowed = TRUE;
			*hchg_allowed = TRUE;
			break;
		case PSTAT_HIGH_STOP:
			*chg_allowed = FALSE;
			*hchg_allowed = FALSE;
			break;
	}

	/* update temp_fault */
	if (PSTAT_NORMAL == pState)
		*temp_fault = FALSE;
	else
		*temp_fault = TRUE;

	return pState;
}

static struct htc_battery_platform_data htc_battery_pdev_data = {
	.chg_limit_active_mask = HTC_BATT_CHG_LIMIT_BIT_TALK,
	.func_show_batt_attr = htc_battery_show_attr,
	.suspend_highfreq_check_reason = SUSPEND_HIGHFREQ_CHECK_BIT_TALK,
	.guage_driver = GAUGE_MAX17050,
	.charger = SWITCH_CHARGER_TPS65200,
	.m2a_cable_detect = 1,
	.enable_bootup_voltage = 3400,
};

static struct platform_device htc_battery_pdev = {
	.name = "htc_battery",
	.id = -1,
	.dev	= {
		.platform_data = &htc_battery_pdev_data,
	},
};

/* battery parameters */
UINT32 m_parameter_TWS_SDI_1650mah[] = {
	/* capacity (in 0.01%) -> voltage (in mV)*/
	10000, 4250, 8100, 4101, 5100, 3846,
	2000, 3711, 900, 3641, 0, 3411,
};
UINT32 m_parameter_Formosa_Sanyo_1650mah[] = {
	/* capacity (in 0.01%) -> voltage (in mV)*/
	10000, 4250, 8100, 4145, 5100, 3845,
	2000, 3735, 900, 3625, 0, 3405,
};
UINT32 m_parameter_PydTD_1520mah[] = {
	10000, 4100, 5500, 3826, 2000, 3739,
	500, 3665, 0, 3397,
};
UINT32 m_parameter_unknown_1650mah[] = {
	10000, 4250, 8100, 4145, 5100, 3845,
	2000, 3735, 900, 3625, 0, 3405,
};

static UINT32* m_param_tbl[] = {
	m_parameter_unknown_1650mah,
	m_parameter_TWS_SDI_1650mah,
	m_parameter_Formosa_Sanyo_1650mah,
	m_parameter_PydTD_1520mah
};

static UINT32 capacity[] = {1620, 1620, 1620, 1620};
static UINT32 fl_25[] = {1650, 1650, 1650, 1520};
static UINT32 pd_m_coef[] = {22, 26, 26, 26};
static UINT32 pd_m_resl[] = {100, 220, 220, 220};
static UINT32 pd_t_coef[] = {100, 220, 220, 220};
static INT32 padc[] = {200, 200, 200, 200};
static INT32 pw[] = {5, 5, 5, 5};

static UINT32* pd_m_coef_tbl[] = {pd_m_coef,};
static UINT32* pd_m_resl_tbl[] = {pd_m_resl,};
static UINT32 capacity_deduction_tbl_01p[] = {0,};

static struct battery_parameter magnids_battery_parameter = {
	.fl_25 = fl_25,
	.pd_m_coef_tbl = pd_m_coef_tbl,
	.pd_m_coef_tbl_boot = pd_m_coef_tbl,
	.pd_m_resl_tbl = pd_m_resl_tbl,
	.pd_m_resl_tbl_boot = pd_m_resl_tbl,
	.pd_t_coef = pd_t_coef,
	.padc = padc,
	.pw = pw,
	.capacity_deduction_tbl_01p = capacity_deduction_tbl_01p,
	.id_tbl = NULL,
	.temp_index_tbl = NULL,
	.m_param_tbl = m_param_tbl,
	.m_param_tbl_size = sizeof(m_param_tbl)/sizeof(UINT32*),
	.capacity = capacity,
};

static max17050_platform_data max17050_pdev_data = {
	.func_get_thermal_id = get_thermal_id,
	.func_get_battery_id = get_battery_id,
	.func_poweralg_config_init = magnids_poweralg_config_init,
	.func_update_charging_protect_flag = magnids_update_charging_protect_flag,
	.r2_kohm = 0,	/* use get_battery_id, doesn't need this */
	.batt_param = &magnids_battery_parameter,
#ifdef CONFIG_TPS65200
	.func_kick_charger_ic = tps65200_kick_charger_ic,
#endif
};

static struct platform_device max17050_battery_pdev = {
	.name = "max17050-battery",
	.id = -1,
	.dev = {
		.platform_data = &max17050_pdev_data,
	},
};

static struct tps65200_platform_data tps65200_data = {
	.gpio_chg_int = MSM_GPIO_TO_INT(40),
	.gpio_chg_stat = MSM_GPIO_TO_INT(57),
};

static struct i2c_board_info i2c_tps65200_devices[] = {
	{
		I2C_BOARD_INFO("tps65200", 0xD4 >> 1),
		.platform_data = &tps65200_data,
	},
};

#ifdef CONFIG_SUPPORT_DQ_BATTERY
static int __init check_dq_setup(char *str)
{
        if (!strcmp(str, "PASS"))
                tps65200_data.dq_result = 1;
        else
                tps65200_data.dq_result = 0;

        return 1;
}
__setup("androidboot.dq=", check_dq_setup);
#endif

static int __init board_serialno_setup(char *serialno)
{
	android_usb_pdata.serial_number = serialno;
	return 1;
}
__setup("androidboot.serialno=", board_serialno_setup);

#ifdef CONFIG_PERFLOCK
static unsigned msm7x27a_perf_acpu_table[] = {
	245760000, /* PERF_LOCK_LOWEST */
	320000000, /* PERF_LOCK_LOW */
	480000000, /* PERF_LOCK_MEDIUM */
	600000000, /* PERF_LOCK_HIGH */
	1008000000,/* PERF_LOCK_HIGHEST */
};

static struct perflock_data msm7x27a_perflock_data = {
	.perf_acpu_table = msm7x27a_perf_acpu_table,
	.table_size = ARRAY_SIZE(msm7x27a_perf_acpu_table),
};

static unsigned msm7x27a_cpufreq_ceiling_acpu_table[] = {
	320000000, /* CEILING_LEVEL_MEDIUM :4,5 */
	480000000, /* PERF_LOCK_HIGH   :2,3 */
	600000000, /* PERF_LOCK_HIGHEST:0,1 */
};

static struct perflock_data msm7x27a_cpufreq_ceiling_data = {
	.perf_acpu_table = msm7x27a_cpufreq_ceiling_acpu_table,
	.table_size = ARRAY_SIZE(msm7x27a_cpufreq_ceiling_acpu_table),
};

static struct perflock_pdata perflock_pdata = {
	.perf_floor = &msm7x27a_perflock_data,
	.perf_ceiling = &msm7x27a_cpufreq_ceiling_data,
};

struct platform_device msm7x27a_device_perf_lock = {
	.name = "perf_lock",
	.id = -1,
	.dev = {
		.platform_data = &perflock_pdata,
	},
};

#ifdef CONFIG_PERFLOCK_SCREEN_POLICY
//static struct perf_lock screen_on_perf_lock;
static struct perf_lock screen_off_ceiling_lock;
static struct perflock_screen_policy msm7x27a_screen_off_policy = {
	.on_min  = NULL,
		   /*  245760 = PERF_LOCK_LOWEST */
	.off_max = &screen_off_ceiling_lock,
		   /*  600000 = PERF_LOCK_HIGHEST */
};
#endif /* CONFIG_PERFLOCK_SCREEN_POLICY */
#endif

#ifdef CONFIG_SERIAL_MSM_HS
static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
	.inject_rx_on_wakeup = 0,
	.cpu_lock_supported = 1,

	/* for bcm BT */
	.bt_wakeup_pin_supported = 1,
	.bt_wakeup_pin = MAGNIDS_GPIO_BT_WAKE,
	.host_wakeup_pin = MAGNIDS_GPIO_BT_HOST_WAKE,
};
#endif

#ifdef CONFIG_BT
static struct platform_device magnids_rfkill = {
	.name = "magnids_rfkill",
	.id = -1,
};
#endif

static struct msm_pm_platform_data msm7x27a_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 16000,
					.residency = 20000,
	},
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 12000,
					.residency = 20000,
	},
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 0,
					.suspend_enabled = 1,
					.latency = 2000,
					.residency = 0,
	},
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 2,
					.residency = 0,
	},
};

static struct bma250_platform_data gsensor_bma250_platform_data = {
	.intr = MAGNIDS_GPIO_GSENSORS_INT,
	.chip_layout = 1,
	.layouts = MAGNIDS_LAYOUTS,
};

static struct i2c_board_info i2c_bma250_devices[] = {
	{
		I2C_BOARD_INFO(BMA250_I2C_NAME_REMOVE_ECOMPASS, \
				0x30 >> 1),
		.platform_data = &gsensor_bma250_platform_data,
		.irq = MSM_GPIO_TO_INT(MAGNIDS_GPIO_GSENSORS_INT),
	},
};

static struct msm_pm_boot_platform_data msm_pm_boot_pdata __initdata = {
	.mode = MSM_PM_BOOT_CONFIG_RESET_VECTOR_PHYS,
	.p_addr = 0,
};

/* 8625 PM platform data */
static struct msm_pm_platform_data msm8625_pm_data[MSM_PM_SLEEP_MODE_NR * 2] = {
	/* CORE0 entries */
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 16000,
					.residency = 20000,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 0,
					.suspend_enabled = 0,
					.latency = 12000,
					.residency = 20000,
	},

	/* picked latency & redisdency values from 7x30 */
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
					.idle_supported = 0,
					.suspend_supported = 0,
					.idle_enabled = 0,
					.suspend_enabled = 0,
					.latency = 500,
					.residency = 6000,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 2,
					.residency = 10,
	},

	/* picked latency & redisdency values from 7x30 */
	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 500,
					.residency = 6000,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 2,
					.residency = 10,
	},

};

static struct msm_pm_boot_platform_data msm_pm_8625_boot_pdata __initdata = {
	.mode = MSM_PM_BOOT_CONFIG_REMAP_BOOT_ADDR,
	.v_addr = MSM_CFG_CTL_BASE,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
};
static struct android_pmem_platform_data android_pmem_adsp2_pdata = {
	.name = "pmem_adsp2",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};


static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};
static struct platform_device android_pmem_adsp2_device = {
	.name = "android_pmem",
	.id = 3,
	.dev = { .platform_data = &android_pmem_adsp2_pdata },
};


static unsigned pmem_mdp_size = MSM_PMEM_MDP_SIZE;
static int __init pmem_mdp_size_setup(char *p)
{
	pmem_mdp_size = memparse(p, NULL);
	return 0;
}

early_param("pmem_mdp_size", pmem_mdp_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
static int __init pmem_adsp_size_setup(char *p)
{
	pmem_adsp_size = memparse(p, NULL);
	return 0;
}

early_param("pmem_adsp_size", pmem_adsp_size_setup);

static unsigned pmem_adsp2_size = MSM_PMEM_ADSP2_SIZE;
static int __init pmem_adsp2_size_setup(char *p)
{
	pmem_adsp2_size = memparse(p, NULL);
	return 0;
}

early_param("pmem_adsp2_size", pmem_adsp2_size_setup);


#define SND(desc, num) { .name = #desc, .id = num }
static struct snd_endpoint snd_endpoints_list[] = {
	SND(HANDSET, 0),
	SND(MONO_HEADSET, 2),
	SND(HEADSET, 3),
	SND(SPEAKER, 6),
	SND(TTY_HEADSET, 8),
	SND(TTY_VCO, 9),
	SND(TTY_HCO, 10),
	SND(BT, 12),
	SND(IN_S_SADC_OUT_HANDSET, 16),
	SND(IN_S_SADC_OUT_SPEAKER_PHONE, 25),
	SND(FM_DIGITAL_STEREO_HEADSET, 26),
	SND(FM_DIGITAL_SPEAKER_PHONE, 27),
	SND(FM_DIGITAL_BT_A2DP_HEADSET, 28),
	SND(STEREO_HEADSET_AND_SPEAKER, 31),
	SND(CURRENT, 0x7FFFFFFE),
	SND(FM_ANALOG_STEREO_HEADSET, 35),
	SND(FM_ANALOG_STEREO_HEADSET_CODEC, 36),
};
#undef SND

static struct msm_snd_endpoints msm_device_snd_endpoints = {
	.endpoints = snd_endpoints_list,
	.num = sizeof(snd_endpoints_list) / sizeof(struct snd_endpoint)
};

static struct platform_device msm_device_snd = {
	.name = "msm_snd",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_snd_endpoints
	},
};

#define DEC0_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC1_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC2_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC3_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC4_FORMAT (1<<MSM_ADSP_CODEC_MIDI)

static unsigned int dec_concurrency_table[] = {
	/* Audio LP */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DMA)), 0,
	0, 0, 0,

	/* Concurrency 1 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	 /* Concurrency 2 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 3 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 4 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 5 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 6 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|
			(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	0, 0, 0, 0,

	/* Concurrency 7 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),
};

#define DEC_INFO(name, queueid, decid, nr_codec) { .module_name = name, \
	.module_queueid = queueid, .module_decid = decid, \
	.nr_codec_support = nr_codec}

static struct msm_adspdec_info dec_info_list[] = {
	DEC_INFO("AUDPLAY0TASK", 13, 0, 11), /* AudPlay0BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY1TASK", 14, 1, 11),  /* AudPlay1BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY2TASK", 15, 2, 11),  /* AudPlay2BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY3TASK", 16, 3, 11),  /* AudPlay3BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY4TASK", 17, 4, 1),  /* AudPlay4BitStreamCtrlQueue */
};

static struct msm_adspdec_database msm_device_adspdec_database = {
	.num_dec = ARRAY_SIZE(dec_info_list),
	.num_concurrency_support = (ARRAY_SIZE(dec_concurrency_table) / \
					ARRAY_SIZE(dec_info_list)),
	.dec_concurrency_table = dec_concurrency_table,
	.dec_info_list = dec_info_list,
};

static struct platform_device msm_device_adspdec = {
	.name = "msm_adspdec",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_adspdec_database
	},
};

static struct android_pmem_platform_data android_pmem_audio_pdata = {
	.name = "pmem_audio",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_audio_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_audio_pdata },
};

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
};
static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};
#ifdef CONFIG_BATTERY_MSM
static u32 msm_calculate_batt_capacity(u32 current_voltage);

static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.voltage_min_design     = 2800,
	.voltage_max_design     = 4300,
	.avail_chg_sources      = AC_CHG | USB_CHG ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
	.calculate_capacity     = &msm_calculate_batt_capacity,
};

static u32 msm_calculate_batt_capacity(u32 current_voltage)
{
	u32 low_voltage	 = msm_psy_batt_data.voltage_min_design;
	u32 high_voltage = msm_psy_batt_data.voltage_max_design;

	return (current_voltage - low_voltage) * 100
			/ (high_voltage - low_voltage);
}

static struct platform_device msm_batt_device = {
	.name               = "msm-battery",
	.id                 = -1,
	.dev.platform_data  = &msm_psy_batt_data,
};
#endif
static char *msm_adc_surf_device_names[] = {
	"XO_ADC",
};

static struct msm_adc_platform_data msm_adc_pdata = {
	.dev_names = msm_adc_surf_device_names,
	.num_adc = ARRAY_SIZE(msm_adc_surf_device_names),
	.target_hw = MSM_8x25,
};

static struct platform_device msm_adc_device = {
	.name   = "msm_adc",
	.id = -1,
	.dev = {
		.platform_data = &msm_adc_pdata,
	},
};

#if defined(CONFIG_MSM_RTB)
static struct msm_rtb_platform_data msm_rtb_pdata = {
	.size = SZ_128K,
};

static int __init msm_rtb_set_buffer_size(char *p)
{
	int s;
	s = memparse(p, NULL);
	msm_rtb_pdata.size = ALIGN(s, SZ_4K);
	return 0;
}
early_param("msm_rtb_size", msm_rtb_set_buffer_size);
static struct platform_device msm_rtb_device = {
	.name = "msm_rtb",
	.id = -1,
	.dev = {
		.platform_data = &msm_rtb_pdata,
	},
};
#endif

#if 0

#if defined(CONFIG_SERIAL_MSM_HSL_CONSOLE) \
		&& defined(CONFIG_MSM_SHARED_GPIO_FOR_UART2DM)
static struct msm_gpio uart2dm_gpios[] = {
	{GPIO_CFG(19, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
							"uart2dm_rfr_n" },
	{GPIO_CFG(20, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
							"uart2dm_cts_n" },
	{GPIO_CFG(21, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
							"uart2dm_rx"    },
	{GPIO_CFG(108, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
							"uart2dm_tx"    },
};

static void msm7x27a_cfg_uart2dm_serial(void)
{
	int ret;
	ret = msm_gpios_request_enable(uart2dm_gpios,
					ARRAY_SIZE(uart2dm_gpios));
	if (ret)
		pr_err("%s: unable to enable gpios for uart2dm\n", __func__);
}
#else
static void msm7x27a_cfg_uart2dm_serial(void) { }
#endif

#endif
static struct gpio_led gpio_exp_leds_config[] = {
	{
		.name = "button-backlight",
		.gpio = 124,
		.active_low = 0,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
};

static struct gpio_led_platform_data gpio_leds_pdata = {
	.num_leds = ARRAY_SIZE(gpio_exp_leds_config),
	.leds = gpio_exp_leds_config,
};

static struct platform_device gpio_leds = {
	.name		  = "leds-gpio",
	.id			= -1,
	.dev		   = {
		.platform_data = &gpio_leds_pdata,
	},
};

static struct pm8029_led_config pm_led_config[] = {
	{
		.name = "green",
		.bank = 0,
		.init_pwm_brightness = 200,
		.flag = FIX_BRIGHTNESS,
	},
	{
		.name = "amber",
		.bank = 4,
		.init_pwm_brightness = 200,
		.flag = FIX_BRIGHTNESS,
	},

};

static struct pm8029_led_platform_data pm8029_leds_data = {
	.led_config = pm_led_config,
	.num_leds = ARRAY_SIZE(pm_led_config),
};

static struct platform_device pm8029_leds = {
	.name   = "leds-pm8029",
	.id     = -1,
	.dev    = {
		.platform_data  = &pm8029_leds_data,
	},
};

/* HTC_HEADSET_GPIO Driver */
static struct htc_headset_gpio_platform_data htc_headset_gpio_data = {
	.hpin_gpio		= MAGNIDS_AUD_HP_INz,
	.key_enable_gpio	= 0,
	.mic_select_gpio	= 0,
};

static struct platform_device htc_headset_gpio = {
	.name	= "HTC_HEADSET_GPIO",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_gpio_data,
	},
};

/* HTC_HEADSET_PMIC Driver */
static struct htc_headset_pmic_platform_data htc_headset_pmic_data = {
	.driver_flag		= DRIVER_HS_PMIC_RPC_KEY,
	.hpin_gpio		= 0,
	.hpin_irq		= 0,
	.key_gpio		= MAGNIDS_AUD_REMO_PRESz,
	.key_irq		= 0,
	.key_enable_gpio	= 0,
	.adc_mic		= 0,
	.adc_remote		= {0, 1833, 1854, 6037, 6059, 12936},
	.hs_controller		= 0,
	.hs_switch		= 0,
};

static struct platform_device htc_headset_pmic = {
	.name	= "HTC_HEADSET_PMIC",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_pmic_data,
	},
};

static struct htc_headset_1wire_platform_data htc_headset_1wire_data = {
	.tx_level_shift_en	= MAGNIDS_AUD_UART_OEz,
	.uart_sw		= MAGNIDS_IOEXT_AUD_UART_SEL,
	.remote_press		= 0,
	.one_wire_remote	={0x7E, 0x7F, 0x7D, 0x7F, 0x7B, 0x7F},
	.onewire_tty_dev	= "/dev/ttyMSM0",
};

static struct platform_device htc_headset_one_wire = {
       .name   = "HTC_HEADSET_1WIRE",
       .id     = -1,
       .dev    = {
               .platform_data  = &htc_headset_1wire_data,
       },
};

/* HTC_HEADSET_MGR Driver */
static struct platform_device *headset_devices[] = {
	&htc_headset_pmic,
	&htc_headset_gpio,
	&htc_headset_one_wire,
	/* Please put the headset detection driver on the last */
};

static struct headset_adc_config htc_headset_mgr_config[] = {
	{
		.type = HEADSET_MIC,
		.adc_max = 59508,
		.adc_min = 47222,
	},
	{
		.type = HEADSET_BEATS,
		.adc_max = 47221,
		.adc_min = 34950,
	},
	{
		.type = HEADSET_BEATS_SOLO,
		.adc_max = 34949,
		.adc_min = 21582,
	},
	{
		.type = HEADSET_NO_MIC, /* HEADSET_INDICATOR */
		.adc_max = 21581,
		.adc_min = 9045,
	},
	{
		.type = HEADSET_NO_MIC,
		.adc_max = 9044,
		.adc_min = 0,
	},
};

static uint32_t headset_cpu_gpio[] = {
	GPIO_CFG(MAGNIDS_AUD_UART_OEz, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(MAGNIDS_AUD_HP_INz, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(MAGNIDS_AUD_REMO_PRESz, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(MAGNIDS_AUD_UART_RX, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(MAGNIDS_AUD_UART_TX, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t headset_1wire_gpio[] = {
	GPIO_CFG(MAGNIDS_AUD_UART_RX, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(MAGNIDS_AUD_UART_TX, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(MAGNIDS_AUD_UART_RX, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(MAGNIDS_AUD_UART_TX, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static void uart_tx_gpo(int mode)
{
	switch (mode) {
		case 0:
			gpio_tlmm_config(headset_1wire_gpio[1], GPIO_CFG_ENABLE);
			gpio_set_value_cansleep(MAGNIDS_AUD_UART_TX, 0);
			pr_info("[HS_BOARD]UART TX GPO 0\n");
			break;
		case 1:
			gpio_tlmm_config(headset_1wire_gpio[1], GPIO_CFG_ENABLE);
			gpio_set_value_cansleep(MAGNIDS_AUD_UART_TX, 1);
			pr_info("[HS_BOARD]UART TX GPO 1\n");
			break;
		case 2:
			gpio_tlmm_config(headset_1wire_gpio[3], GPIO_CFG_ENABLE);
			pr_info("[HS_BOARD]UART TX alternative\n");
			break;
	}
}

static void uart_lv_shift_en(int enable)
{
	gpio_set_value_cansleep(MAGNIDS_AUD_UART_OEz, 0);
	pr_info("[HS_BOARD]level shift %d\n", enable);
}

static void headset_init(void)
{
	int i,ret;
	for (i = 0; i < ARRAY_SIZE(headset_cpu_gpio); i++) {
		ret = gpio_tlmm_config(headset_cpu_gpio[i], GPIO_CFG_ENABLE);
		pr_info("[HS_BOARD]Config gpio[%d], ret = %d\n", (headset_cpu_gpio[i] & 0x3FF0) >> 4, ret);
	}
	gpio_set_value(MAGNIDS_AUD_UART_OEz, 1); /*Disable 1-wire level shift by default*/

}

static void headset_power(int hs_enable)
{
	gpio_set_value(MAGNIDS_AUD_UART_OEz, 1);
}

static struct htc_headset_mgr_platform_data htc_headset_mgr_data = {
	.driver_flag		= DRIVER_HS_MGR_FLOAT_DET,
	.headset_devices_num	= ARRAY_SIZE(headset_devices),
	.headset_devices	= headset_devices,
	.headset_config_num	= ARRAY_SIZE(htc_headset_mgr_config),
	.headset_config		= htc_headset_mgr_config,
	.headset_power		= headset_power,
	.headset_init		= headset_init,
	.uart_tx_gpo		= uart_tx_gpo,
	.uart_lv_shift_en	= uart_lv_shift_en,
};

static struct platform_device htc_headset_mgr = {
	.name	= "HTC_HEADSET_MGR",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_mgr_data,
	},
};

/* HEADSET DRIVER END */


#define PM8058ADC_16BIT(adc) ((adc * 1800) / 65535) /* vref=2.2v, 16-bits resolution */
int64_t magnids_get_usbid_adc(void)
{
	uint32_t adc_value = 0xffffffff;
	htc_get_usb_accessory_adc_level(&adc_value);
	adc_value = PM8058ADC_16BIT(adc_value);
	return adc_value;
}

static uint32_t usb_ID_PIN_input_table[] = {
	GPIO_CFG(MAGNIDS_GPIO_USB_ID, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA),
};

static uint32_t usb_ID_PIN_ouput_table[] = {
	GPIO_CFG(MAGNIDS_GPIO_USB_ID, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA),
};


static void config_usb_id_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("[CAM] %s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

void config_magnids_usb_id_gpios(bool output)
{
	if (output) {
		config_usb_id_table(usb_ID_PIN_ouput_table, ARRAY_SIZE(usb_ID_PIN_ouput_table));
		gpio_set_value(42, 1);
		printk(KERN_INFO "%s %d output high\n",  __func__, 42);
	} else {
		config_usb_id_table(usb_ID_PIN_input_table, ARRAY_SIZE(usb_ID_PIN_input_table));
		printk(KERN_INFO "%s %d input none pull\n",  __func__, 42);
	}
}

static struct cable_detect_platform_data cable_detect_pdata = {
	.detect_type = CABLE_TYPE_PMIC_ADC,
	.usb_id_pin_gpio = MAGNIDS_GPIO_USB_ID_PIN,
	.config_usb_id_gpios = config_magnids_usb_id_gpios,
	.get_adc_cb		= magnids_get_usbid_adc,
};

static struct platform_device cable_detect_device = {
	.name	= "cable_detect",
	.id	= -1,
	.dev	= {
		.platform_data = &cable_detect_pdata,
	},
};

static struct resource ram_console_resources[] = {
	{
		.start  = MSM_RAM_CONSOLE_BASE,
		.end    = MSM_RAM_CONSOLE_BASE + MSM_RAM_CONSOLE_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
};

static struct platform_device ram_console_device = {
	.name           = "ram_console",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(ram_console_resources),
	.resource       = ram_console_resources,
};

static struct platform_device *msm7627a_surf_ffa_devices[] __initdata = {
	&msm_device_dmov,
	&msm_device_smd,
	&msm_device_uart1,
#if 0//config by BT
	&msm_device_uart_dm1,
#endif
	&msm_device_uart_dm2,
	&msm_gsbi0_qup_i2c_device,
	&msm_gsbi1_qup_i2c_device,
	&msm_device_otg,
	&msm_device_gadget_peripheral,
	&msm_kgsl_3d0,
};

static struct platform_device *common_devices[] __initdata = {
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
	&android_pmem_adsp2_device,
	&msm_device_nand,
	&msm_device_snd,
	&msm_device_adspdec,
	&asoc_msm_pcm,
	&asoc_msm_dai0,
	&asoc_msm_dai1,
	&msm_adc_device,
#ifdef CONFIG_BATTERY_MSM
	&msm_batt_device,
#endif
	&htc_battery_pdev,
#ifdef CONFIG_PERFLOCK
	&msm7x27a_device_perf_lock,
#endif
	&max17050_battery_pdev,
	&pm8029_leds,
	&gpio_leds,
	&htc_headset_mgr,
	&cable_detect_device,
#if defined(CONFIG_MSM_RTB)
	&msm_rtb_device,
#endif
};

static struct platform_device *msm8625_surf_devices[] __initdata = {
	&ram_console_device,
	&msm8625_device_dmov,
	&msm8625_device_uart1,
#if 0//config by BT
	&msm8625_device_uart_dm1,
#endif
	&msm8625_device_uart_dm2,
	&msm8625_gsbi0_qup_i2c_device,
	&msm8625_gsbi1_qup_i2c_device,
	&msm8625_device_smd,
	&msm8625_device_otg,
	&msm8625_kgsl_3d0,
};

static unsigned pmem_kernel_ebi1_size = PMEM_KERNEL_EBI1_SIZE;
static int __init pmem_kernel_ebi1_size_setup(char *p)
{
	pmem_kernel_ebi1_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_kernel_ebi1_size", pmem_kernel_ebi1_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;
static int __init pmem_audio_size_setup(char *p)
{
	pmem_audio_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_audio_size", pmem_audio_size_setup);

static struct memtype_reserve msm7x27a_reserve_table[] __initdata = {
	[MEMTYPE_SMI] = {
	},
	[MEMTYPE_EBI0] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
	[MEMTYPE_EBI1] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
};

static void __init size_pmem_devices(void)
{
	pmem_mdp_size = MSM_PMEM_MDP_SIZE;
	pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
	pmem_adsp2_size = MSM_PMEM_ADSP2_SIZE;

#ifdef CONFIG_ANDROID_PMEM
	android_pmem_adsp_pdata.size = pmem_adsp_size;
	android_pmem_adsp2_pdata.size = pmem_adsp2_size;
	android_pmem_pdata.size = pmem_mdp_size;
	android_pmem_audio_pdata.size = pmem_audio_size;
#endif
}

static void __init reserve_rtb_memory(void)
{
#if defined(CONFIG_MSM_RTB)
	// need to add align the with rtb size
	unsigned int size;
	unsigned int rtb_align_size;
	size = SZ_128K;
	rtb_align_size = ALIGN(size, SZ_4K);
	//msm7627a_reserve_table[MEMTYPE_EBI1].size += msm_rtb_pdata.size;
	msm7x27a_reserve_table[MEMTYPE_EBI1].size += rtb_align_size;
#endif
}

static void __init reserve_memory_for(struct android_pmem_platform_data *p)
{
	msm7x27a_reserve_table[p->memory_type].size += p->size;
}

static void __init reserve_pmem_memory(void)
{
#ifdef CONFIG_ANDROID_PMEM
	reserve_memory_for(&android_pmem_adsp_pdata);
	reserve_memory_for(&android_pmem_adsp2_pdata);
	reserve_memory_for(&android_pmem_pdata);
	reserve_memory_for(&android_pmem_audio_pdata);
	msm7x27a_reserve_table[MEMTYPE_EBI1].size += pmem_kernel_ebi1_size;
#endif
}

static void __init msm7x27a_calculate_reserve_sizes(void)
{
	size_pmem_devices();
	reserve_pmem_memory();
	reserve_rtb_memory();
}

static int msm7x27a_paddr_to_memtype(unsigned int paddr)
{
	return MEMTYPE_EBI1;
}

static struct reserve_info msm7x27a_reserve_info __initdata = {
	.memtype_reserve_table = msm7x27a_reserve_table,
	.calculate_reserve_sizes = msm7x27a_calculate_reserve_sizes,
	.paddr_to_memtype = msm7x27a_paddr_to_memtype,
};

static void __init msm7x27a_reserve(void)
{
	reserve_info = &msm7x27a_reserve_info;
	msm_reserve();
}

static void __init msm8625_reserve(void)
{
	msm7x27a_reserve();
	memblock_remove(MSM8625_SECONDARY_PHYS, SZ_8);
	memblock_remove(MSM8625_WARM_BOOT_PHYS, SZ_32);
}

static void __init msm8625_device_i2c_init(void)
{
	msm8625_gsbi0_qup_i2c_device.dev.platform_data =
		&msm_gsbi0_qup_i2c_pdata;
	msm8625_gsbi1_qup_i2c_device.dev.platform_data =
		&msm_gsbi1_qup_i2c_pdata;
}

/* Touch Panel */
#define MAGNIDS_GPIO_TP_ATT_N            (18)
#define MAGNIDS_GPIO_TP_RST_N            (85)

static int msm8625_ts_himax_power(int on)
{
	printk(KERN_INFO "%s():\n", __func__);
	//gpio_set_value(MAGNIDS_V_TP_3V3_EN, on);

	return 0;
}

static void msm8625_ts_himax_reset(void)
{
	printk(KERN_INFO "%s():\n", __func__);
	gpio_direction_output(MAGNIDS_GPIO_TP_RST_N, 0);
	mdelay(20);
	gpio_direction_output(MAGNIDS_GPIO_TP_RST_N, 1);
	mdelay(50);
}

struct himax_i2c_platform_data_config_type_3 config_type3[] = {
	{
		.version = 0x10,
		.common = 1,
		.z_fuzz = 6,

		.c1 = { 0x62, 0x43, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00 },
		.c2 = { 0x63, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00 },
		.c3 = { 0x64, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00 },
		.c4 = { 0x65, 0x13, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00 },
		.c5 = { 0x66, 0x23, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00 },
		.c6 = { 0x67, 0x13, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00 },
		.c7 = { 0x68, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00 },
		.c8 = { 0x69, 0x43, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00 },
		.c9 = { 0x6A, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00 },
		.c10 = { 0x6B, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00 },
		.c11 = { 0x6C, 0x23, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00 },
		.c12 = { 0x6D, 0x30, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.c13 = { 0xC9, 0x00, 0x00, 0x00, 0x04, 0x2C, 0x06, 0x26, 0x29, 0x01, 0x30,
			 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
			 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.c14 = { 0x8A, 0x00, 0x04, 0x00, 0x04, 0x00, 0x06, 0x3E, 0x0B, 0xB4, 0x6D,
			 0x27, 0x0F, 0x00, 0x41, 0x00, 0xDA, 0x38, 0x81, 0x0F, 0x00, 0x10,
			 0xFF, 0x0A, 0x11, 0xFF, 0x1C, 0x09, 0xFF, 0xFF, 0x1B, 0x08, 0xFF,
			 0x0B, 0x1A, 0x07, 0xFF, 0xFF, 0x19, 0x06, 0x0C, 0x0D, 0x18, 0x05,
			 0xFF, 0xFF, 0x17, 0x04, 0xFF, 0xFF, 0x16, 0x03, 0x0F, 0xFF, 0x15,
			 0x02, 0xFF, 0xFF, 0x14, 0x01, 0xFF, 0xFF, 0x13, 0x00, 0x0E, 0xFF,
			 0x12, 0x1D, 0xFF, 0x04, 0x2C, 0x06, 0x26, 0x29, 0x01, 0x30, 0x0D,
			 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x24,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.c15 = { 0xC5, 0x0D, 0x1D, 0x04, 0x10, 0x1B, 0x1F, 0x0B },
		.c16 = { 0xC6, 0x11, 0x10, 0x17 },
		.c17 = { 0x7D, 0x00, 0x04, 0x0A, 0x0A, 0x04 },
		.c18 = { 0x7F, 0x08, 0x01, 0x01, 0x01, 0x01, 0x07, 0x08, 0x07, 0x0F, 0x07,
			 0x0F, 0x07, 0x0F, 0x00 },
		.c19 = { 0xD5, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.c20 = { 0xE9, 0x00, 0x00 },
		.c21 = { 0xEA, 0x0B, 0x13, 0x00, 0x13 },
		.c22 = { 0xEB, 0x32, 0x29, 0xFA, 0x83 },
		.c23 = { 0xEC, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.c24 = { 0xED, 0x0F, 0xFF },
		.c25 = { 0xEE, 0x00 },
		.c26 = { 0xEF, 0x11, 0x00 },
		.c27 = { 0xF0, 0x40 },
		.c28 = { 0xF1, 0x06, 0x04, 0x06, 0x03 },
		.c29 = { 0xF2, 0x0A, 0x06, 0x14, 0x3C },
		.c30 = { 0xF3, 0x5F },
		.c31 = { 0xF4, 0x7D, 0xB9, 0x2D, 0x3A },
		.c32 = { 0xF6, 0x00, 0x00, 0x1B, 0x76, 0x08 },
		.c33 = { 0xF7, 0x30, 0x78, 0x7F, 0x0D, 0x00 },
		.c34 = { 0x35, 0x02, 0x01 },
		.c35 = { 0x36, 0x0F, 0x53, 0x01 },
		.c36 = { 0x37, 0xFF, 0x08, 0xFF, 0x08 },
		.c37 = { 0x39, 0x03 },
		.c38 = { 0x3A, 0x00 },
		.c39 = { 0x50, 0xAB },
		.c40 = { 0x6E, 0x04 },
		.c41 = { 0x76, 0x01, 0x36 },
		.c42 = { 0x78, 0x03 },
		.c43 = { 0x7A, 0x00, 0x18, 0x0D },
		.c44 = { 0x8B, 0x10, 0x20 },
		.c45 = { 0x8C, 0x30, 0x0C, 0x0A, 0x0C, 0x0A, 0x0A, 0x0A, 0x32, 0x24, 0x40,
			 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x0D, 0x00, 0x00, 0x00 },
		.c46 = { 0x8D, 0xA0, 0x5A, 0x14, 0x0A, 0x32, 0x0A, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.c47 = { 0xC2, 0x11, 0x00, 0x00, 0x00 },
		.c48 = { 0xCB, 0x01, 0xF5, 0xFF, 0xFF, 0x01, 0x00, 0x05, 0x00, 0x05, 0x00 },
		.c49 = { 0xD4, 0x01, 0x04, 0x07 },
		.c50 = { 0xDD, 0x05, 0x02 },
		.checksum = { 0xAC, 0x72, 0x4E },
	},
};

struct himax_i2c_platform_data msm7x27a_ts_himax_data = {
	.slave_addr = 0x90,
	.abs_x_min = 0,
	.abs_x_max = 1023,
	.abs_y_min = 0,
	.abs_y_max = 935,
	.abs_pressure_min = 0,
	.abs_pressure_max = 200,
	.abs_width_min = 0,
	.abs_width_max = 200,
	.gpio_irq = MAGNIDS_GPIO_TP_ATT_N,
	.gpio_reset = MAGNIDS_GPIO_TP_RST_N,
	.version = 0x00,
	.tw_id = 0,
	.event_htc_enable = 0,
	.cable_config = { 0x90, 0x00},
	.power = msm8625_ts_himax_power,
	.powerOff3V3 = 0,
	.reset = msm8625_ts_himax_reset,
	.protocol_type = PROTOCOL_TYPE_B,
	.screenWidth = 480,
	.screenHeight = 800,
	.ID0 = "ALPS",
	.ID1 = "J-Touch",
	.ID2 = "N/A",
	.ID3 = "N/A",
	.type1 = 0,
	.type1_size = 0,
	.type2 = 0,
	.type2_size = 0,
	.type3 = config_type3,
	.type3_size = sizeof(config_type3),
};

#ifdef CONFIG_FLASHLIGHT_AAT1290
#ifdef CONFIG_MSM_CAMERA_FLASH
int flashlight_control(int mode)
{
	int	rc;
	/* Andrew_Cheng Turn off backlight when flash on */
	static int	backlight_off = 0;

	if (mode != FL_MODE_PRE_FLASH && mode != FL_MODE_OFF) {
		led_brightness_switch("lcd-backlight", FALSE);
		backlight_off = 1;
	}

	rc = aat1290_flashlight_control(mode);

	if(mode == FL_MODE_PRE_FLASH || mode == FL_MODE_OFF) {
		if(backlight_off) {
			led_brightness_switch("lcd-backlight", TRUE);
			backlight_off = 0;
		}
	}

	return rc;
}
#endif

static void config_flashlight_gpios(void)
{
	if (system_rev > XA) {
		static uint32_t flashlight_gpio_table[] = {
			GPIO_CFG(MAGNIDS_GPIO_FLASH_ENABLE_XB, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG(MAGNIDS_GPIO_FLASH_SWITCH, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		};
		gpio_tlmm_config(flashlight_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(flashlight_gpio_table[1], GPIO_CFG_ENABLE);
	} else {
		static uint32_t flashlight_gpio_table[] = {
			GPIO_CFG(MAGNIDS_GPIO_FLASH_ENABLE_XA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG(MAGNIDS_GPIO_FLASH_SWITCH, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		};
		gpio_tlmm_config(flashlight_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(flashlight_gpio_table[1], GPIO_CFG_ENABLE);
	}
}

static struct flashlight_platform_data flashlight_data = {
		.gpio_init = config_flashlight_gpios,
		.flash = MAGNIDS_GPIO_FLASH_ENABLE_XB,
		.torch = MAGNIDS_GPIO_FLASH_SWITCH,
		.led_count = 1,
		.flash_duration_ms = 600,
	};

static struct platform_device flashlight_device = {
	.name = "FLASHLIGHT_AAT1290",
	.dev = {
		.platform_data	= &flashlight_data,
	},
};

static struct flashlight_platform_data flashlight_data_XA = {
		.gpio_init = config_flashlight_gpios,
		.flash = MAGNIDS_GPIO_FLASH_ENABLE_XA,
		.torch = MAGNIDS_GPIO_FLASH_SWITCH,
		.led_count = 1,
		.flash_duration_ms = 600,
	};

static struct platform_device flashlight_device_XA = {
	.name = "FLASHLIGHT_AAT1290",
	.dev = {
		.platform_data	= &flashlight_data_XA,
	},
};
#endif

/*define gpio expander here*/
static void ioext_reset_chip(void)
{
	printk(KERN_INFO "%s\n", __func__);
}

static struct platform_device ioext_devices[] = {
};

static struct ioext_i2c_platform_data ioext_data = {
	.num_devices = ARRAY_SIZE(ioext_devices),
	.ioext_devices = ioext_devices,
	.reset_chip = ioext_reset_chip,
};

static struct i2c_board_info i2c_ioext_devices[] = {
	{
		I2C_BOARD_INFO(IOEXTENDER_I2C_NAME, 0x68 >> 1),
		.platform_data = &ioext_data,
	},
};

static struct i2c_board_info i2c_touch_device[] = {
	{
		I2C_BOARD_INFO(HIMAX8526A_NAME, 0x90>>1),
		.platform_data  = &msm7x27a_ts_himax_data,
		.irq = MSM_GPIO_TO_INT(MAGNIDS_GPIO_TP_ATT_N),
	},
};


static ssize_t msm8625_virtual_keys_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
		__stringify(EV_KEY) ":" __stringify(KEY_BACK)	    ":74:858:87:73"
		":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":245:860:89:71"
		":" __stringify(EV_KEY) ":" __stringify(KEY_APP_SWITCH) ":411:860:92:75"
		"\n");
}
static struct kobj_attribute msm8625_himax_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.himax-touchscreen",
		.mode = S_IRUGO,
	},
	.show = &msm8625_virtual_keys_show,
};

static struct attribute *msm8625_properties_attrs[] = {
	&msm8625_himax_virtual_keys_attr.attr,
	NULL
};

static struct attribute_group msm8625_properties_attr_group = {
	.attrs = msm8625_properties_attrs,
};

int msm8625_init_keypad(void);

#define MSM_EBI2_PHYS			0xa0d00000
#define MSM_EBI2_XMEM_CS2_CFG1		0xa0d10030

static void __init msm7x27a_init_ebi2(void)
{
	uint32_t ebi2_cfg;
	void __iomem *ebi2_cfg_ptr;

	ebi2_cfg_ptr = ioremap_nocache(MSM_EBI2_PHYS, sizeof(uint32_t));
	if (!ebi2_cfg_ptr)
		return;

	ebi2_cfg = readl(ebi2_cfg_ptr);
	if (machine_is_msm7x27a_rumi3() || machine_is_msm7x27a_surf() ||
		machine_is_msm7625a_surf() || machine_is_msm8625_surf() || machine_is_magnids())
		ebi2_cfg |= (1 << 4); /* CS2 */

	writel(ebi2_cfg, ebi2_cfg_ptr);
	iounmap(ebi2_cfg_ptr);

	/* Enable A/D MUX[bit 31] from EBI2_XMEM_CS2_CFG1 */
	ebi2_cfg_ptr = ioremap_nocache(MSM_EBI2_XMEM_CS2_CFG1,
							 sizeof(uint32_t));
	if (!ebi2_cfg_ptr)
		return;

	ebi2_cfg = readl(ebi2_cfg_ptr);
	if (machine_is_msm7x27a_surf() || machine_is_msm7625a_surf())
		ebi2_cfg |= (1 << 31);

	writel(ebi2_cfg, ebi2_cfg_ptr);
	iounmap(ebi2_cfg_ptr);
}

static struct platform_device msm_proccomm_regulator_dev = {
	.name   = PROCCOMM_REGULATOR_DEV_NAME,
	.id     = -1,
	.dev    = {
		.platform_data = &msm7x27a_proccomm_regulator_data
	}
};

static void msm_adsp_add_pdev(void)
{
	int rc = 0;
	struct rpc_board_dev *rpc_adsp_pdev;

	rpc_adsp_pdev = kzalloc(sizeof(struct rpc_board_dev), GFP_KERNEL);
	if (rpc_adsp_pdev == NULL) {
		pr_err("%s: Memory Allocation failure\n", __func__);
		return;
	}
	rpc_adsp_pdev->prog = ADSP_RPC_PROG;

	if (cpu_is_msm8625())
		rpc_adsp_pdev->pdev = msm8625_device_adsp;
	else
		rpc_adsp_pdev->pdev = msm_adsp_device;
	rc = msm_rpc_add_board_dev(rpc_adsp_pdev, 1);
	if (rc < 0) {
		pr_err("%s: return val: %d\n",	__func__, rc);
		kfree(rpc_adsp_pdev);
	}
}

#if 0//defined(CONFIG_BT) && defined(CONFIG_MARIMBA_CORE)
static int __init msm7x27a_init_ar6000pm(void)
{
	msm_wlan_ar6000_pm_device.dev.platform_data = &ar600x_wlan_power;
	return platform_device_register(&msm_wlan_ar6000_pm_device);
}
#else
static int __init msm7x27a_init_ar6000pm(void) { return 0; }
#endif

static void __init msm7x27a_init_regulators(void)
{
	int rc = platform_device_register(&msm_proccomm_regulator_dev);
	if (rc)
		pr_err("%s: could not register regulator device: %d\n",
				__func__, rc);
}

static void __init msm7x27a_add_footswitch_devices(void)
{
	platform_add_devices(msm_footswitch_devices,
			msm_num_footswitch_devices);
}

static void __init msm7x27a_add_platform_devices(void)
{
	if (machine_is_msm8625_surf() || machine_is_msm8625_ffa() || machine_is_magnids()) {
		platform_add_devices(msm8625_surf_devices,
			ARRAY_SIZE(msm8625_surf_devices));
	} else {
		platform_add_devices(msm7627a_surf_ffa_devices,
			ARRAY_SIZE(msm7627a_surf_ffa_devices));
	}

	platform_add_devices(common_devices,
			ARRAY_SIZE(common_devices));
}
#if 0
static void __init msm7x27a_uartdm_config(void)
{
	msm7x27a_cfg_uart2dm_serial();
#if 0//UART1DM config by BT
	msm_uart_dm1_pdata.wakeup_irq = gpio_to_irq(UART1DM_RX_GPIO);
	if (cpu_is_msm8625())
		msm8625_device_uart_dm1.dev.platform_data =
			&msm_uart_dm1_pdata;
	else
		msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
#endif
}

#endif
static void __init msm7x27a_otg_gadget(void)
{
	if (cpu_is_msm8625()) {
#if 0
		msm_otg_pdata.swfi_latency =
		msm8625_pm_data[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].latency;
#endif
		msm8625_device_otg.dev.platform_data = &msm_otg_pdata;
		android_usb_pdata.swfi_latency =
		msm8625_pm_data[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].latency;
#ifdef CONFIG_USB_GADGET_MSM_72K
		msm8625_device_gadget_peripheral.dev.platform_data =
			&msm_gadget_pdata;
#endif
	} else {
#if 0
		msm_otg_pdata.swfi_latency =
		msm7x27a_pm_data[
		MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
#endif
		msm_device_otg.dev.platform_data = &msm_otg_pdata;
#ifdef CONFIG_USB_GADGET_MSM_72K
		msm_device_gadget_peripheral.dev.platform_data =
			&msm_gadget_pdata;
#endif
	}
}

static void __init msm7x27a_pm_init(void)
{
	if (machine_is_msm8625_surf() || machine_is_msm8625_ffa() || machine_is_magnids()) {
		msm_pm_set_platform_data(msm8625_pm_data,
				ARRAY_SIZE(msm8625_pm_data));
		BUG_ON(msm_pm_boot_init(&msm_pm_8625_boot_pdata));
		msm8x25_spm_device_init();
	} else {
		msm_pm_set_platform_data(msm7x27a_pm_data,
				ARRAY_SIZE(msm7x27a_pm_data));
		BUG_ON(msm_pm_boot_init(&msm_pm_boot_pdata));
	}

	msm_pm_register_irqs();
}

static void magnids_reset(void)
{
	gpio_set_value(MAGNIDS_GPIO_PS_HOLD, 0);
}

unsigned int *cpu_foot_print = CPU_FOOT_PRINT;


extern int msm_proc_comm(unsigned, unsigned*, unsigned*);

static void __init msm7x2x_init(void)
{
	struct proc_dir_entry *entry = NULL;
	int rc = 0;
	struct kobject *properties_kobj;

	msm_hw_reset_hook = magnids_reset;

	msm7x2x_misc_init();

#ifdef CONFIG_PERFLOCK
#ifdef CONFIG_PERFLOCK_SCREEN_POLICY
	/* Init screen off policy perflocks */
	perf_lock_init(&screen_off_ceiling_lock, TYPE_CPUFREQ_CEILING,
					PERF_LOCK_HIGH, "screen_off_scaling_max");
	perflock_screen_policy_init(&msm7x27a_screen_off_policy);
#endif /* CONFIG_PERFLOCK_SCREEN_POLICY */
#endif
	/* Initialize regulators first so that other devices can use them */
	msm7x27a_init_regulators();
	msm_adsp_add_pdev();
	msm8625_device_i2c_init();
	msm7x27a_init_ebi2();
	/*msm7x27a_uartdm_config();*/

	msm7x27a_otg_gadget();

	msm7x27a_add_footswitch_devices();
	if (get_kernel_flag() & 0x02) {
		htc_headset_mgr_data.headset_devices_num--;
		htc_headset_mgr_data.headset_devices[2] = NULL;
	}
	msm7x27a_add_platform_devices();

#ifdef CONFIG_SERIAL_MSM_HS
	msm_uart_dm1_pdata.wakeup_irq = gpio_to_irq(MAGNIDS_GPIO_BT_HOST_WAKE);
	//msm_device_uart_dm1.name = "msm_serial_hs_brcm"; /* for brcm */
	//msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
	msm8625_device_uart_dm1.name = "msm_serial_hs_brcm"; /* for brcm */
	msm8625_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
#endif
#ifdef CONFIG_BT
	bt_export_bd_address();
	platform_device_register(&magnids_rfkill);
	//platform_device_register(&msm_device_uart_dm1);
	platform_device_register(&msm8625_device_uart_dm1);
#endif

	/* Ensure ar6000pm device is registered before MMC/SDC */
	msm7x27a_init_ar6000pm();

#ifdef CONFIG_MMC_MSM
	printk(KERN_ERR "%s: start init mmc\n", __func__);
	msm7627a_init_mmc();
	printk(KERN_ERR "%s: msm7627a_init_mmc()\n", __func__);
	entry = create_proc_read_entry("emmc", 0, NULL, emmc_partition_read_proc, NULL);
	printk(KERN_ERR "%s: create_proc_read_entry()\n", __func__);
	if (!entry)
		printk(KERN_ERR"Create /proc/emmc failed!\n");
#endif
//	msm_fb_add_devices();
//	printk(KERN_ERR "%s: msm_fb_add_devices\n", __func__);
	/*Magni#DS display initializations*/
	magnids_init_panel();
	printk(KERN_ERR "%s: magnids_init_panel\n", __func__);
#ifdef CONFIG_USB_EHCI_MSM_72K
	msm7x2x_init_host();
	printk(KERN_ERR "%s: msm7x2x_init_host\n", __func__);
#endif
	msm7x27a_pm_init();
	printk(KERN_ERR "%s: msm7x27a_pm_init\n", __func__);
	register_i2c_devices();
	printk(KERN_ERR "%s: register_i2c_devices\n", __func__);

	//msm7627a_bt_power_init();
	magnids_camera_init();
	//msm7627a_add_io_devices();
	/*7x25a kgsl initializations*/
	//msm7x25a_kgsl_3d0_init();
	/*8x25 kgsl initializations*/
	if(board_mfg_mode() != 4)
		msm8x25_kgsl_3d0_init();
	/* FIXME: board_mfg_mode is not ready yet */
#if 0
        /*usb driver won't be loaded in MFG 58 station and gift mode*/
        if (!(board_mfg_mode() == 6 || board_mfg_mode() == 7))
#endif
		magnids_add_usb_devices();


#if defined(CONFIG_INPUT_CAPELLA_CM3629)
#define GPIO_PS_2V8 58
	{
    int status;
	status = gpio_request(GPIO_PS_2V8, "PS_2V85_EN");
    if (status) {
            pr_err("%s:Failed to request GPIO %d\n", __func__,
                            GPIO_PS_2V8);
    } else {
            status = gpio_direction_output(GPIO_PS_2V8,1);
            pr_info("PS_2V85 status = %d\n",status);
            if (!status)
                    status = gpio_get_value(GPIO_PS_2V8);
            gpio_free(GPIO_PS_2V8);
            pr_info("PS_2V85 status = %d\n",status);
    }

	status = gpio_request(MAGNIDS_GPIO_PROXIMITY_INT, "PS_INT");
	if (status) {
		pr_err("%s: Failed to request GPIO %d\n", __func__,MAGNIDS_GPIO_PROXIMITY_INT);
	} else {
		status = gpio_tlmm_config(GPIO_CFG(MAGNIDS_GPIO_PROXIMITY_INT, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		pr_info("MAGNIDS_GPIO_PROXIMITY_INT status = %d\n",status);
        gpio_free(MAGNIDS_GPIO_PROXIMITY_INT);
	}

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_cm36282_devices, ARRAY_SIZE(i2c_cm36282_devices));
	}
#endif
	printk(KERN_ERR "%s: end\n", __func__);
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				i2c_touch_device,
				ARRAY_SIZE(i2c_touch_device));

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_bma250_devices, ARRAY_SIZE(i2c_bma250_devices));

	/*register gpio expander here*/
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				i2c_ioext_devices,
				ARRAY_SIZE(i2c_ioext_devices));

	properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (properties_kobj)
		rc = sysfs_create_group(properties_kobj,
						&msm8625_properties_attr_group);
	if (!properties_kobj || rc)
		printk(KERN_ERR "[TP]failed to create board_properties\n");
	else {
		msm7x27a_ts_himax_data.vk_obj = properties_kobj;
		msm7x27a_ts_himax_data.vk2Use = &msm8625_himax_virtual_keys_attr;
	}
	msm_init_pmic_vibrator();

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_tps65200_devices, ARRAY_SIZE(i2c_tps65200_devices));

	msm8625_init_keypad();
	bcm4330_wifi_init();
	magnids_audio_init();
	if (get_kernel_flag() & KERNEL_FLAG_PM_MONITOR) {
		htc_monitor_init();
		htc_PM_monitor_init();
	}
#ifdef CONFIG_FLASHLIGHT_AAT1290
	if (system_rev > XA) {
		platform_device_register(&flashlight_device);
	} else {
		platform_device_register(&flashlight_device_XA);
	}
#endif
	*(cpu_foot_print + 0) = 0xaabbccaa;
	*(cpu_foot_print + 1) = 0xaabbccaa;
	*(cpu_foot_print + 2) = 0xaabbccaa;
	*(cpu_foot_print + 3) = 0xaabbccaa;
}

static unsigned int radio_sm_log_enable = 0;

static void __init magnids_fixup(struct tag *tags, char **cmdline, struct meminfo *mi)
{
	radio_sm_log_enable = parse_tag_security((const struct tag *)tags);
	printk(KERN_INFO "%s: radio_sm_log_enable=0x%x\n", __func__, radio_sm_log_enable);

	mi->nr_banks = 2;
	mi->bank[0].start = 0x03200000;
	mi->bank[0].size = 0x0CE00000;
	mi->bank[1].start = 0x10000000;

	/*reserve 20MB for QXDM log if radio sm log enable is ture*/
	if (radio_sm_log_enable & 0x1)
		mi->bank[1].size = 0x1E700000;
	else
		mi->bank[1].size = 0x1FB00000;
}

static void __init msm7x2x_init_early(void)
{
	//msm_msm7627a_allocate_memory_regions();
}

MACHINE_START(MAGNIDS, "magnids")
	.atag_offset	= PHYS_OFFSET + 0x100,
	.fixup = magnids_fixup,
	.map_io		= msm8625_map_io,
	.reserve	= msm8625_reserve,
	.init_irq	= msm8625_init_irq,
	.init_machine	= msm7x2x_init,
	.timer		= &msm_timer,
	.init_early     = msm7x2x_init_early,
	.handle_irq	= gic_handle_irq,
MACHINE_END
