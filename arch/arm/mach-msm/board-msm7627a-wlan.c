/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
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
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <asm/mach-types.h>
#include <mach/rpc_pmapp.h>
#include "board-msm7627a.h"
#include "devices-msm7x2xa.h"
#include "timer.h"

#if (defined(CONFIG_MACH_CP3DTG) || defined(CONFIG_MACH_CP3DCG) \
    || defined(CONFIG_MACH_CP3DUG) ||  defined(CONFIG_MACH_CP3U) \
    || defined(CONFIG_MACH_Z4DUG) || defined(CONFIG_MACH_Z4DCG) || defined(CONFIG_MACH_Z4U))
#else
#define QCA_ORIGINAL
#endif

#define GPIO_WLAN_3V3_EN 119
static const char *id = "WLAN";
static bool wlan_powered_up;

enum {
#ifdef QCA_ORIGINAL
	WLAN_VREG_S3 = 0,
	WLAN_VREG_L17,
#else
	WLAN_VREG_L17 = 0,
#endif
	WLAN_VREG_L19
};

struct wlan_vreg_info {
	const char *vreg_id;
	unsigned int level_min;
	unsigned int level_max;
	unsigned int pmapp_id;
	unsigned int is_vreg_pin_controlled;
	struct regulator *reg;
};

static struct wlan_vreg_info vreg_info[] = {
#ifdef QCA_ORIGINAL
	{"msme1",     1800000, 1800000, 2,  0, NULL},
#endif
	{"bt",        3300000, 3300000, 21, 1, NULL},
	{"wlan4",     1800000, 1800000, 23, 1, NULL}
};

int gpio_wlan_sys_rest_en = 134;
static void gpio_wlan_config(void)
{
	if (machine_is_msm7627a_qrd1() || machine_is_msm7627a_evb()
					|| machine_is_msm8625_evb()
					|| machine_is_msm8625_evt()
					|| machine_is_msm7627a_qrd3()
					|| machine_is_msm8625_qrd7())
		gpio_wlan_sys_rest_en = 124;
	else if  (machine_is_qrd_skud_prime() || machine_is_msm8625q_evbd()
				|| machine_is_msm8625q_skud())
		gpio_wlan_sys_rest_en = 38;
}

static unsigned int qrf6285_init_regs(void)
{
	struct regulator_bulk_data regs[ARRAY_SIZE(vreg_info)];
	int i = 0, rc = 0;

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		regs[i].supply = vreg_info[i].vreg_id;
		regs[i].min_uV = vreg_info[i].level_min;
		regs[i].max_uV = vreg_info[i].level_max;
	}

	rc = regulator_bulk_get(NULL, ARRAY_SIZE(regs), regs);
	if (rc) {
		pr_err("%s: could not get regulators: %d\n", __func__, rc);
		goto out;
	}

	for (i = 0; i < ARRAY_SIZE(regs); i++)
		vreg_info[i].reg = regs[i].consumer;

out:
	return rc;
}

#ifdef QCA_ORIGINAL
static unsigned int setup_wlan_gpio(bool on)
{
	int rc = 0;

	if (on) {
		rc = gpio_direction_output(gpio_wlan_sys_rest_en, 1);
		msleep(100);
	} else {
		gpio_set_value_cansleep(gpio_wlan_sys_rest_en, 0);
		rc = gpio_direction_input(gpio_wlan_sys_rest_en);
		msleep(100);
	}

	if (rc)
		pr_err("%s: WLAN sys_reset_en GPIO: Error", __func__);

	return rc;
}

static unsigned int setup_wlan_clock(bool on)
{
	int rc = 0;

	if (on) {
		/* Vote for A0 clock */
		rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_A0,
					PMAPP_CLOCK_VOTE_ON);
	} else {
		/* Vote against A0 clock */
		rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_A0,
					 PMAPP_CLOCK_VOTE_OFF);
	}

	if (rc)
		pr_err("%s: Configuring A0 clock for WLAN: Error", __func__);

	return rc;
}
#endif

static unsigned int wlan_switch_regulators(int on)
{
	int rc = 0, index = 0;

	if (machine_is_msm7627a_qrd1())
		index = 2;

	for ( ; index < ARRAY_SIZE(vreg_info); index++) {
		if (on) {
			rc = regulator_set_voltage(vreg_info[index].reg,
						vreg_info[index].level_min,
						vreg_info[index].level_max);
			if (rc) {
				pr_err("%s:%s set voltage failed %d\n",
					__func__, vreg_info[index].vreg_id, rc);
				goto reg_disable;
			}

			rc = regulator_enable(vreg_info[index].reg);
			if (rc) {
				pr_err("%s:%s vreg enable failed %d\n",
					__func__, vreg_info[index].vreg_id, rc);
				goto reg_disable;
			}

			if (vreg_info[index].is_vreg_pin_controlled) {
				rc = pmapp_vreg_lpm_pincntrl_vote(id,
						vreg_info[index].pmapp_id,
						PMAPP_CLOCK_ID_A0, 1);
				if (rc) {
					pr_err("%s:%s pincntrl failed %d\n",
						__func__,
						vreg_info[index].vreg_id, rc);
					goto pin_cnt_fail;
				}
			}
#ifndef QCA_ORIGINAL
			if (index == WLAN_VREG_L17)
				udelay(5);
			else if (index == WLAN_VREG_L19)
				udelay(10);
#endif
		} else {
			if (vreg_info[index].is_vreg_pin_controlled) {
				rc = pmapp_vreg_lpm_pincntrl_vote(id,
						vreg_info[index].pmapp_id,
						PMAPP_CLOCK_ID_A0, 0);
				if (rc) {
					pr_err("%s:%s pincntrl failed %d\n",
						__func__,
						vreg_info[index].vreg_id, rc);
					goto pin_cnt_fail;
				}
			}

			rc = regulator_disable(vreg_info[index].reg);
			if (rc) {
				pr_err("%s:%s vreg disable failed %d\n",
					__func__,
					vreg_info[index].vreg_id, rc);
				goto reg_disable;
			}
		}
	}
	return 0;
pin_cnt_fail:
	if (on)
		regulator_disable(vreg_info[index].reg);
reg_disable:
	if (!machine_is_msm7627a_qrd1()) {
		while (index) {
			if (on) {
				index--;
				regulator_disable(vreg_info[index].reg);
				regulator_put(vreg_info[index].reg);
			}
		}
	}
	return rc;
}

#ifndef QCA_ORIGINAL
int atheros_wifi_power(int on);  
#endif

static unsigned int msm_AR600X_setup_power(bool on)
{
	int rc = 0;
	static bool init_done;

	if (wlan_powered_up) {
		pr_info("WLAN already powered up\n");
		return 0;
	}

	if (unlikely(!init_done)) {
		gpio_wlan_config();
		rc = qrf6285_init_regs();
		if (rc) {
			pr_err("%s: qrf6285 init failed = %d\n", __func__, rc);
			return rc;
		} else {
			init_done = true;
		}
	}

	rc = wlan_switch_regulators(on);
	if (rc) {
		pr_err("%s: wlan_switch_regulators error = %d\n", __func__, rc);
		goto out;
	}
#ifndef QCA_ORIGINAL
	udelay(20);
	atheros_wifi_power(on);
#endif

#ifdef QCA_ORIGINAL
	/* GPIO_WLAN_3V3_EN is only required for the QRD7627a */
	if (machine_is_msm7627a_qrd1()) {
		rc = gpio_tlmm_config(GPIO_CFG(GPIO_WLAN_3V3_EN, 0,
					GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
					GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s gpio_tlmm_config 119 failed,error = %d\n",
				__func__, rc);
			goto reg_disable;
		}
		gpio_set_value(GPIO_WLAN_3V3_EN, 1);
	}

	/*
	 * gpio_wlan_sys_rest_en is not from the GPIO expander for QRD7627a,
	 * EVB1.0 and QRD8625,so the below step is required for those devices.
	 */
	if (machine_is_msm7627a_qrd1() || machine_is_msm7627a_evb()
					|| machine_is_msm8625_evb()
					|| machine_is_msm8625_evt()
					|| machine_is_msm7627a_qrd3()
					|| machine_is_msm8625_qrd7()
					|| machine_is_msm8625q_evbd()
					|| machine_is_qrd_skud_prime()) {
		rc = gpio_tlmm_config(GPIO_CFG(gpio_wlan_sys_rest_en, 0,
					GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
					GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s gpio_tlmm_config 119 failed,error = %d\n",
				__func__, rc);
			goto qrd_gpio_fail;
		}
		gpio_set_value(gpio_wlan_sys_rest_en, 1);
	} else {
		rc = gpio_request(gpio_wlan_sys_rest_en, "WLAN_DEEP_SLEEP_N");
		if (rc) {
			pr_err("%s: WLAN sys_rest_en GPIO %d request failed %d\n",
				__func__,
				gpio_wlan_sys_rest_en, rc);
			goto qrd_gpio_fail;
		}
		rc = setup_wlan_gpio(on);
		if (rc) {
			pr_err("%s: wlan_set_gpio = %d\n", __func__, rc);
			goto gpio_fail;
		}
	}

	/* Enable the A0 clock */
	rc = setup_wlan_clock(on);
	if (rc) {
		pr_err("%s: setup_wlan_clock = %d\n", __func__, rc);
		goto set_gpio_fail;
	}

	/* Configure A0 clock to be slave to WLAN_CLK_PWR_REQ */
	rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_A0,
				 PMAPP_CLOCK_VOTE_PIN_CTRL);
	if (rc) {
		pr_err("%s: Configuring A0 to Pin controllable failed %d\n",
				__func__, rc);
		goto set_clock_fail;
	}
#endif

	pr_info("WLAN power-up success\n");
	wlan_powered_up = true;
	return 0;
#ifdef QCA_ORIGINAL
set_clock_fail:
	setup_wlan_clock(0);
set_gpio_fail:
	setup_wlan_gpio(0);
gpio_fail:
	if (!(machine_is_msm7627a_qrd1() || machine_is_msm7627a_evb() ||
	    machine_is_msm8625_evb() || machine_is_msm8625_evt() ||
	    machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7()))
			gpio_free(gpio_wlan_sys_rest_en);
qrd_gpio_fail:
	/* GPIO_WLAN_3V3_EN is only required for the QRD7627a */
	if (machine_is_msm7627a_qrd1())
		gpio_free(GPIO_WLAN_3V3_EN);
reg_disable:
	wlan_switch_regulators(0);
#endif
out:
	pr_info("WLAN power-up failed\n");
	wlan_powered_up = false;
	return rc;
}

static unsigned int msm_AR600X_shutdown_power(bool on)
{
	int rc = 0;

	if (!wlan_powered_up) {
		pr_info("WLAN is not powered up, returning success\n");
		return 0;
	}

#ifndef QCA_ORIGINAL
	atheros_wifi_power(on);
	udelay(20);
#endif
#ifdef QCA_ORIGINAL
	/* Disable the A0 clock */
	rc = setup_wlan_clock(on);
	if (rc) {
		pr_err("%s: setup_wlan_clock = %d\n", __func__, rc);
		goto set_clock_fail;
	}

	/*
	 * gpio_wlan_sys_rest_en is not from the GPIO expander for QRD7627a,
	 * EVB1.0 and QRD8625,so the below step is required for those devices.
	 */
	if (machine_is_msm7627a_qrd1() || machine_is_msm7627a_evb()
					|| machine_is_msm8625_evb()
					|| machine_is_msm8625_evt()
					|| machine_is_msm7627a_qrd3()
					|| machine_is_msm8625_qrd7()
					|| machine_is_msm8625q_evbd()
					|| machine_is_qrd_skud_prime()) {
		rc = gpio_tlmm_config(GPIO_CFG(gpio_wlan_sys_rest_en, 0,
					GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
					GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s gpio_tlmm_config 119 failed,error = %d\n",
				__func__, rc);
			goto gpio_fail;
		}
		gpio_set_value(gpio_wlan_sys_rest_en, 0);
	} else {
		rc = setup_wlan_gpio(on);
		if (rc) {
			pr_err("%s: setup_wlan_gpio = %d\n", __func__, rc);
			goto set_gpio_fail;
		}
		gpio_free(gpio_wlan_sys_rest_en);
	}

	/* GPIO_WLAN_3V3_EN is only required for the QRD7627a */
	if (machine_is_msm7627a_qrd1()) {
		rc = gpio_tlmm_config(GPIO_CFG(GPIO_WLAN_3V3_EN, 0,
					GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
					GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s gpio_tlmm_config 119 failed,error = %d\n",
				__func__, rc);
			goto qrd_gpio_fail;
		}
		gpio_set_value(GPIO_WLAN_3V3_EN, 0);
	}
#endif

	rc = wlan_switch_regulators(on);
	if (rc) {
		pr_err("%s: wlan_switch_regulators error = %d\n",
			__func__, rc);
		goto reg_disable;
	}
	wlan_powered_up = false;
	pr_info("WLAN power-down success\n");
	return 0;
#ifdef QCA_ORIGINAL
set_clock_fail:
	setup_wlan_clock(0);
set_gpio_fail:
	setup_wlan_gpio(0);
gpio_fail:
	if (!(machine_is_msm7627a_qrd1() || machine_is_msm7627a_evb() ||
	    machine_is_msm8625_evb() || machine_is_msm8625_evt() ||
	    machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7()))
			gpio_free(gpio_wlan_sys_rest_en);
qrd_gpio_fail:
	/* GPIO_WLAN_3V3_EN is only required for the QRD7627a */
	if (machine_is_msm7627a_qrd1())
		gpio_free(GPIO_WLAN_3V3_EN);
#endif
reg_disable:
	wlan_switch_regulators(0);
	pr_info("WLAN power-down failed\n");
	return rc;
}

int wifi_is_powering_onoff = 0;
DEFINE_MUTEX(wifi_power_mtx);

int  ar600x_wlan_power(bool on)
{
	mutex_lock(&wifi_power_mtx);
	wifi_is_powering_onoff = 1;
	if (on)
		msm_AR600X_setup_power(on);
	else
		msm_AR600X_shutdown_power(on);

	wifi_is_powering_onoff = 0;
	mutex_unlock(&wifi_power_mtx);
	return 0;
}
