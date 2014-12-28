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
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/mach/mmc.h>
#include <linux/regulator/consumer.h>
#include <mach/gpio.h>
#include <asm/gpio.h>
#include <mach/gpiomux.h>
#include <mach/board.h>
#include "devices.h"
#include "pm.h"
#include "board-msm7627a.h"
#include <linux/mmc/card.h>

extern int msm_add_sdcc(unsigned int controller, struct mmc_platform_data *plat);

#if (defined(CONFIG_MMC_MSM_SDC1_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC2_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC3_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC4_SUPPORT))

#define MAX_SDCC_CONTROLLER 4
static unsigned long vreg_sts, gpio_sts;

struct sdcc_gpio {
	struct msm_gpio *cfg_data;
	uint32_t size;
	struct msm_gpio *sleep_cfg_data;
};

static struct msm_gpio sdc1_cfg_data[] = {
	{GPIO_CFG(51, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc1_dat_3"},
	{GPIO_CFG(52, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc1_dat_2"},
	{GPIO_CFG(53, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc1_dat_1"},
	{GPIO_CFG(54, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc1_dat_0"},
	{GPIO_CFG(55, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc1_cmd"},
	{GPIO_CFG(56, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
								"sdc1_clk"},
};

static struct msm_gpio sdc1_sleep_cfg_data[] = {
	{GPIO_CFG(51, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc1_dat_3"},
	{GPIO_CFG(52, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc1_dat_2"},
	{GPIO_CFG(53, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc1_dat_1"},
	{GPIO_CFG(54, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc1_dat_0"},
	{GPIO_CFG(55, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc1_cmd"},
	{GPIO_CFG(56, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
								"sdc1_clk"},
};

static struct msm_gpio sdc2_cfg_data[] = {
	{GPIO_CFG(62, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
								"sdc2_clk"},
	{GPIO_CFG(63, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc2_cmd"},
	{GPIO_CFG(64, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc2_dat_3"},
	{GPIO_CFG(65, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc2_dat_2"},
	{GPIO_CFG(66, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc2_dat_1"},
	{GPIO_CFG(67, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc2_dat_0"},
};

static struct msm_gpio sdc2_sleep_cfg_data[] = {
	{GPIO_CFG(62, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
								"sdc2_clk"},
	{GPIO_CFG(63, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
								"sdc2_cmd"},
	{GPIO_CFG(64, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
								"sdc2_dat_3"},
	{GPIO_CFG(65, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
								"sdc2_dat_2"},
	{GPIO_CFG(66, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
								"sdc2_dat_1"},
	{GPIO_CFG(67, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
								"sdc2_dat_0"},
};

static struct msm_gpio sdc3_cfg_data[] = {
	{GPIO_CFG(88, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_12MA),
								"sdc3_clk"},
	{GPIO_CFG(89, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc3_cmd"},
	{GPIO_CFG(90, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc3_dat_3"},
	{GPIO_CFG(91, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc3_dat_2"},
	{GPIO_CFG(92, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc3_dat_1"},
	{GPIO_CFG(93, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
								"sdc3_dat_0"},
#ifdef CONFIG_MMC_MSM_SDC3_8_BIT_SUPPORT
	{GPIO_CFG(19, 3, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_12MA),
								"sdc3_dat_7"},
	{GPIO_CFG(20, 3, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_12MA),
								"sdc3_dat_6"},
	{GPIO_CFG(21, 3, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_12MA),
								"sdc3_dat_5"},
	{GPIO_CFG(108, 3, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_12MA),
								"sdc3_dat_4"},
#endif
};

static struct msm_gpio sdc4_cfg_data[] = {
	{GPIO_CFG(19, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc4_dat_3"},
	{GPIO_CFG(20, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc4_dat_2"},
	{GPIO_CFG(21, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc4_dat_1"},
	{GPIO_CFG(107, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc4_cmd"},
	{GPIO_CFG(108, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA),
								"sdc4_dat_0"},
	{GPIO_CFG(109, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
								"sdc4_clk"},
};

static struct sdcc_gpio sdcc_cfg_data[] = {
	{
		.cfg_data = sdc1_cfg_data,
		.size = ARRAY_SIZE(sdc1_cfg_data),
		.sleep_cfg_data = sdc1_sleep_cfg_data,
	},
	{
		.cfg_data = sdc2_cfg_data,
		.size = ARRAY_SIZE(sdc2_cfg_data),
		.sleep_cfg_data = sdc2_sleep_cfg_data,
	},
	{
		.cfg_data = sdc3_cfg_data,
		.size = ARRAY_SIZE(sdc3_cfg_data),
	},
	{
		.cfg_data = sdc4_cfg_data,
		.size = ARRAY_SIZE(sdc4_cfg_data),
	},
};

static int gpio_sdc1_hw_det = 94;
static void gpio_sdc1_config(void)
{
	if (machine_is_msm7627a_qrd1() || machine_is_msm7627a_evb()
					|| machine_is_msm8625_evb()
					|| machine_is_msm7627a_qrd3()
					|| machine_is_msm8625_qrd7())
		gpio_sdc1_hw_det = 94;
}

static struct regulator *sdcc_vreg_data[MAX_SDCC_CONTROLLER];
static int msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct sdcc_gpio *curr;

	curr = &sdcc_cfg_data[dev_id - 1];
	if (!(test_bit(dev_id, &gpio_sts)^enable))
		return rc;

	if (enable) {
		set_bit(dev_id, &gpio_sts);
		rc = msm_gpios_request_enable(curr->cfg_data, curr->size);
		if (rc)
			pr_err("%s: Failed to turn on GPIOs for slot %d\n",
					__func__,  dev_id);
	} else {
		clear_bit(dev_id, &gpio_sts);
		if (curr->sleep_cfg_data) {
			rc = msm_gpios_enable(curr->sleep_cfg_data, curr->size);
			msm_gpios_free(curr->sleep_cfg_data, curr->size);
			return rc;
		}
		msm_gpios_disable_free(curr->cfg_data, curr->size);
	}
	return rc;
}

static int msm_sdcc_setup_vreg(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct regulator *curr = sdcc_vreg_data[dev_id - 1];

	if (test_bit(dev_id, &vreg_sts) == enable)
		return 0;

	if (!curr)
		return -ENODEV;

	if (IS_ERR(curr))
		return PTR_ERR(curr);

	if (enable) {
		if (dev_id == 1) {
			mdelay(5);
			pr_info("%s: mmc1 Enabling SD slot power\n", __func__);
		}
		set_bit(dev_id, &vreg_sts);

		rc = regulator_enable(curr);
		if (rc)
			pr_err("%s: could not enable regulator: %d\n",
						__func__, rc);
	} else {
		if (dev_id == 1) {
			mdelay(5);
			pr_info("%s: mmc1 Disabling SD slot power\n", __func__);
		}
		clear_bit(dev_id, &vreg_sts);

		rc = regulator_disable(curr);
		if (rc)
			pr_err("%s: could not disable regulator: %d\n",
						__func__, rc);
	}
	return rc;
}

static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);

	rc = msm_sdcc_setup_gpio(pdev->id, !!vdd);
	if (rc)
		goto out;

	rc = msm_sdcc_setup_vreg(pdev->id, !!vdd);
out:
	return rc;
}

#if defined(CONFIG_MMC_MSM_SDC1_SUPPORT) \
	&& defined(CONFIG_MMC_MSM_CARD_HW_DETECTION)
static unsigned int msm7627a_sdcc_slot_status(struct device *dev)
{
	int status;

	status = gpio_tlmm_config(GPIO_CFG(gpio_sdc1_hw_det, 2, GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_6MA),
				GPIO_CFG_ENABLE);
	if (status)
		pr_err("%s:Failed to configure tlmm for GPIO %d\n", __func__,
				gpio_sdc1_hw_det);

	status = gpio_request(gpio_sdc1_hw_det, "SD_HW_Detect");
	if (status) {
		pr_err("%s:Failed to request GPIO %d\n", __func__,
				gpio_sdc1_hw_det);
	} else {
		status = gpio_direction_input(gpio_sdc1_hw_det);
		if (!status) {
			if (machine_is_msm7627a_qrd1() ||
					machine_is_msm7627a_evb() ||
					machine_is_msm8625_evb()  ||
					machine_is_msm7627a_qrd3() ||
					machine_is_msm8625_qrd7())
				status = !gpio_get_value(gpio_sdc1_hw_det);
			else
				status = !gpio_get_value(gpio_sdc1_hw_det);
		}
		gpio_free(gpio_sdc1_hw_det);
	}
	return status;
}
#endif

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
static unsigned int msm7627a_sdslot_type = MMC_TYPE_SD;
static struct mmc_platform_data sdc1_plat_data = {
	.ocr_mask       = MMC_VDD_28_29,
	.translate_vdd  = msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin   = 144000,
	.msmsdcc_fmid   = 25000000,
	.msmsdcc_fmax   = 50000000,
	.slot_type      = &msm7627a_sdslot_type,
	.mmc_dma_ch    = 10,
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	.status      = msm7627a_sdcc_slot_status,
	.irq_flags   = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
#endif
};
#endif


#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
#if (defined(CONFIG_MACH_PROTODCG) || defined(CONFIG_MACH_MAGNIDS) \
    || defined(CONFIG_MACH_PROTODUG) || defined(CONFIG_MACH_PROTOU))
static struct embedded_sdio_data bcm4330_wifi_emb_data = {
	.cccr	= {
		.sdio_vsn	= 2,
		.multi_block	= 1,
		.low_speed	= 0,
		.wide_bus	= 0,
		.high_power	= 1,
		.high_speed	= 1,
	}
};

static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;

static int
bcm4330_wifi_status_register(void (*callback)(int card_present, void *dev_id),
				void *dev_id)
{
	if (wifi_status_cb)
		return -EAGAIN;
	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
	return 0;
}

static int bcm4330_wifi_cd;	

static unsigned int bcm4330_wifi_status(struct device *dev)
{
	return bcm4330_wifi_cd;
}

static unsigned int bcm4330_wifislot_type = MMC_TYPE_SDIO_WIFI;
static struct mmc_platform_data bcm4330_wifi_data = {
		.ocr_mask               = MMC_VDD_28_29,
		.status                 = bcm4330_wifi_status,
		.register_status_notify = bcm4330_wifi_status_register,
		.embedded_sdio          = &bcm4330_wifi_emb_data,
		.slot_type	= &bcm4330_wifislot_type,
		.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
		.msmsdcc_fmin   = 144000,
		.msmsdcc_fmid   = 25000000,
		.msmsdcc_fmax   = 50000000,
		.nonremovable   = 0,
};

int bcm4330_wifi_set_carddetect(int val)
{
        printk(KERN_INFO "%s: %d\n", __func__, val);
        bcm4330_wifi_cd = val;
        if (wifi_status_cb)
                wifi_status_cb(val, wifi_status_cb_devid);
        else
                printk(KERN_WARNING "%s: Nobody to notify\n", __func__);
        return 0;
}
#endif

#if (defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) \
    || defined(CONFIG_MACH_CP3DUG) \
    || defined(CONFIG_MACH_DUMMY))
static unsigned int atheros_wifislot_type = MMC_TYPE_SDIO_WIFI;
static struct mmc_platform_data sdc2_plat_data = {
	.ocr_mask       = MMC_VDD_28_29 | MMC_VDD_165_195,
	
	.slot_type	= &atheros_wifislot_type,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.sdiowakeup_irq = MSM_GPIO_TO_INT(66),
	.msmsdcc_fmin   = 144000,
	.msmsdcc_fmid   = 24000000,
	.msmsdcc_fmax   = 50000000,
	.nonremovable   = 0,
#ifdef CONFIG_MMC_MSM_SDC2_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
};
#endif
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static unsigned int msm7627a_emmcslot_type = MMC_TYPE_MMC;
static struct mmc_platform_data sdc3_plat_data = {
	.ocr_mask       = MMC_VDD_28_29,
	
#ifdef CONFIG_MMC_MSM_SDC3_8_BIT_SUPPORT
	.mmc_bus_width  = MMC_CAP_8_BIT_DATA,
#else
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#endif
	.msmsdcc_fmin   = 144000,
	.msmsdcc_fmid   = 25000000,
	.msmsdcc_fmax   = 50000000,
	.nonremovable   = 1,
	.mmc_dma_ch    = 7,
	.slot_type      = &msm7627a_emmcslot_type,
};
#endif

#if (defined(CONFIG_MMC_MSM_SDC4_SUPPORT)\
		&& !defined(CONFIG_MMC_MSM_SDC3_8_BIT_SUPPORT))
static unsigned int msm7627a_sprdslot_type = MMC_TYPE_SDIO_SPRD;
static struct mmc_platform_data sdc4_plat_data = {
	.ocr_mask       = MMC_VDD_165_195,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin   = 144000,
	.msmsdcc_fmid   = 25000000,
	.msmsdcc_fmax   = 50000000,
	.slot_type	= &msm7627a_sprdslot_type,
	.nonremovable	= 0,
};
#endif

static int __init mmc_regulator_init(int sdcc_no, const char *supply, int uV)
{
	int rc;

	BUG_ON(sdcc_no < 1 || sdcc_no > 4);

	sdcc_no--;

	sdcc_vreg_data[sdcc_no] = regulator_get(NULL, supply);

	if (IS_ERR(sdcc_vreg_data[sdcc_no])) {
		rc = PTR_ERR(sdcc_vreg_data[sdcc_no]);
		pr_err("%s: could not get regulator \"%s\": %d\n",
				__func__, supply, rc);
		goto out;
	}

	rc = regulator_set_voltage(sdcc_vreg_data[sdcc_no], uV, uV);

	if (rc) {
		pr_err("%s: could not set voltage for \"%s\" to %d uV: %d\n",
				__func__, supply, uV, rc);
		goto reg_free;
	}

	return rc;

reg_free:
	regulator_put(sdcc_vreg_data[sdcc_no]);
out:
	sdcc_vreg_data[sdcc_no] = NULL;
	return rc;
}
#ifdef QCT_original
void __init cp3dug_init_mmc(void)
{
	
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT

	
	if (!(machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7())) {
		if (mmc_regulator_init(3, "emmc", 3000000))
			return;
		msm_add_sdcc(3, &sdc3_plat_data);
	}
#endif
	
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	gpio_sdc1_config();
	if (mmc_regulator_init(1, "mmc", 2850000))
		return;
	sdc1_plat_data.status_irq = MSM_GPIO_TO_INT(gpio_sdc1_hw_det);
	msm_add_sdcc(1, &sdc1_plat_data);
#endif
	
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	if (mmc_regulator_init(2, "smps3", 1800000))
		return;
	msm_add_sdcc(2, &sdc2_plat_data);
#endif
	
#if (defined(CONFIG_MMC_MSM_SDC4_SUPPORT)\
		&& !defined(CONFIG_MMC_MSM_SDC3_8_BIT_SUPPORT))
	
	if (!(machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7())) {
		if (mmc_regulator_init(4, "smps3", 1800000))
			return;
		msm_add_sdcc(4, &sdc4_plat_data);
	}
#endif
}
#else
void __init cp3dug_init_mmc(void)
{
	printk(KERN_ERR "%s: HTC 0\n", __func__);
	
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT

	if (mmc_regulator_init(3, "emmc", 3000000))
		return;
	msm_add_sdcc(3, &sdc3_plat_data);
#endif
	printk(KERN_ERR "%s: HTC 1\n", __func__);
	
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	gpio_sdc1_config();
	if (mmc_regulator_init(1, "mmc", 2850000))
		return;
	sdc1_plat_data.status_irq = MSM_GPIO_TO_INT(gpio_sdc1_hw_det);
	msm_add_sdcc(1, &sdc1_plat_data);
#endif
	
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	if (mmc_regulator_init(2, "smps3", 1800000))
		return;

#if (defined(CONFIG_MACH_PROTODCG) || defined(CONFIG_MACH_MAGNIDS) \
    || defined(CONFIG_MACH_PROTODUG) || defined(CONFIG_MACH_PROTOU))
	msm_add_sdcc(2, &bcm4330_wifi_data); 
#endif
#if (defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) \
    || defined(CONFIG_MACH_CP3DUG) \
    || defined(CONFIG_MACH_DUMMY))
	msm_add_sdcc(2, &sdc2_plat_data);  
#endif
#endif
	
#if (defined(CONFIG_MMC_MSM_SDC4_SUPPORT)\
		&& !defined(CONFIG_MMC_MSM_SDC3_8_BIT_SUPPORT))
	
	if (!(machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7())) {
		if (mmc_regulator_init(4, "smps3", 1800000))
			return;
		msm_add_sdcc(4, &sdc4_plat_data);
	}
	printk(KERN_ERR "%s: HTC 2\n", __func__);
#endif
}
#endif

#endif
