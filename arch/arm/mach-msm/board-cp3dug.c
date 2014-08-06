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
#include <asm/htc_version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio_event.h>
#ifdef CONFIG_USB_G_ANDROID
#include <linux/usb/android_composite.h>
#include <mach/usbdiag.h>
#endif
#include <linux/usb/android.h>
#include <linux/platform_device.h>
#include <mach/htc_usb.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/i2c.h>
#include <linux/android_pmem.h>
#include <linux/bootmem.h>
#include <linux/mfd/marimba.h>
#include <linux/power_supply.h>
#include <linux/input/rmi_platformdata.h>
#include <linux/input/rmi_i2c.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/regulator/consumer.h>
#include <linux/memblock.h>
#include <linux/input/ft5x06_ts.h>
#include <mach/tfa9887.h>
#include <mach/rt5501.h>
#include <linux/msm_adc.h>
#include <linux/fmem.h>
#include <linux/regulator/msm-gpio-regulator.h>
#include <linux/ion.h>
#include <linux/i2c-gpio.h>
#include <linux/regulator/onsemi-ncp6335d.h>
#include <linux/i2c/cpld.h>
#include <asm/mach/mmc.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/hardware/gic.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/msm_hsusb.h>
#include <mach/rpc_hsusb.h>
#include <mach/rpc_pmapp.h>
#include <mach/usbdiag.h>
#include <mach/msm_memtypes.h>
#include <mach/msm_serial_hs.h>
#include <mach/system.h>
#include <mach/pmic.h>
#include <mach/socinfo.h>
#include <mach/vreg.h>
#include <mach/rpc_pmapp.h>
#include <linux/max17050_battery.h>
#include <linux/max17050_gauge.h>
#include <linux/tps65200.h>
#include <mach/htc_battery.h>
#include <mach/rpc_server_handset.h>
#include <mach/htc_headset_mgr.h>
#include <mach/htc_headset_gpio.h>
#include <mach/htc_headset_pmic.h>
#include <mach/htc_headset_one_wire.h>
#include <mach/socinfo.h>
#include "devices.h"
#include "devices-msm7x2xa.h"
#include "pm.h"
#include "timer.h"
#include "pm-boot.h"
#include <linux/proc_fs.h>
#include "board-msm7x27a-regulator.h"
#include <mach/board_htc.h>
#include <asm/setup.h>
#include "board-cp3dug.h"
#include <linux/leds-pm8029.h>
#include <linux/cm3629.h>
#include <linux/htc_flashlight.h>
#include <linux/i2c/cpld.h>
#include <linux/pn544.h>

#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_3K
#include <linux/synaptics_i2c_rmi.h>
#endif
#include <linux/bma250.h>

#ifdef CONFIG_CODEC_AIC3254
#include <linux/i2c/aic3254.h>
#endif

#ifdef CONFIG_PERFLOCK
#include <mach/perflock.h>
#endif

#ifdef CONFIG_BT
#include <mach/htc_bdaddress.h>
#endif

#include <mach/htc_util.h>

#define PMEM_KERNEL_EBI1_SIZE	0x3A000
#define MSM_PMEM_AUDIO_SIZE	0x1F4000
#define BAHAMA_SLAVE_ID_FM_REG 0x02
#define FM_GPIO	42
#define BT_PCM_BCLK_MODE  0x88
#define BT_PCM_DIN_MODE   0x89
#define BT_PCM_DOUT_MODE  0x8A
#define BT_PCM_SYNC_MODE  0x8B
#define FM_I2S_SD_MODE    0x8E
#define FM_I2S_WS_MODE    0x8F
#define FM_I2S_SCK_MODE   0x90
#define I2C_PIN_CTL       0x15
#define I2C_NORMAL        0x40

#define TFA9887_I2C_SLAVE_ADDR  (0x68 >> 1)
#define TFA9887L_I2C_SLAVE_ADDR (0x6A >> 1)
#define RT5501_I2C_SLAVE_ADDR	(0xF0 >> 1)

#define CP3DUG_GPIO_NFC_INT     (114)
int htc_get_usb_accessory_adc_level(uint32_t *buffer);
#include <mach/cable_detect.h>

static DEFINE_MUTEX(nfc_lock);
static int pn544_set_ven_gpio(int enable)
{
	int ret = 0;

	mutex_lock(&nfc_lock);
	pr_info("[NFC][pn544] %s: enable:%d\n", __func__, enable);
	ret = cpld_gpio_write(CPLD_EXT_GPIO_NFC_VEN, enable);
	    if(ret < 0)  printk(KERN_ERR "write error NFC_VEN");
	mutex_unlock(&nfc_lock);
	pr_info("[NFC][pn544] %s: ret=0x%x\n", __func__, ret);
	return ret;
}

static int pn544_get_ven_gpio(void)
{
	int ret = 0;

	mutex_lock(&nfc_lock);
	pr_info("[NFC][pn544] %s:\n", __func__);
	ret = cpld_gpio_read(CPLD_EXT_GPIO_NFC_VEN);
	    if(ret < 0)  printk(KERN_ERR "read error NFC_VEN");
	mutex_unlock(&nfc_lock);
	pr_info("[NFC][pn544] %s: ret=0x%x\n", __func__, ret);
	return ret;
}

static int pn544_set_firm_gpio(int enable)
{
	int ret = 0;

	mutex_lock(&nfc_lock);
	pr_info("[NFC][pn544] %s: enable:%d\n", __func__, enable);
	ret = cpld_gpio_write(CPLD_EXT_GPIO_NFC_DL_MODE, enable);
	    if(ret < 0)  printk(KERN_ERR "write error NFC_VEN");
	mutex_unlock(&nfc_lock);
	pr_info("[NFC][pn544] %s: ret=0x%x\n", __func__, ret);
	return ret;
}

static int pn544_get_firm_gpio(void)
{
	int ret = 0;

	mutex_lock(&nfc_lock);
	pr_info("[NFC][pn544] %s:\n", __func__);
	ret = cpld_gpio_read(CPLD_EXT_GPIO_NFC_DL_MODE);
	    if(ret < 0)  printk(KERN_ERR "read error NFC_VEN");
	mutex_unlock(&nfc_lock);
	pr_info("[NFC][pn544] %s: ret=0x%x\n", __func__, ret);
	return ret;
}

static struct pn544_i2c_platform_data nfc_platform_data = {
	.irq_gpio = CP3DUG_GPIO_NFC_INT,
	.set_ven_gpio = pn544_set_ven_gpio,
	.get_ven_gpio = pn544_get_ven_gpio,
	.set_firm_gpio = pn544_set_firm_gpio,
	.get_firm_gpio = pn544_get_firm_gpio,
	.ven_isinvert = 1,
};

static struct i2c_board_info pn544_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO(PN544_I2C_NAME, 0x50 >> 1),
		.platform_data = &nfc_platform_data,
		.irq = MSM_GPIO_TO_INT(CP3DUG_GPIO_NFC_INT),
	},
};

static struct bma250_platform_data gsensor_bma250_platform_data = {
	.intr = CP3DUG_GPIO_GSENSORS_INT,
	.chip_layout = 0,
	.layouts = CP3DUG_LAYOUTS,
};

static struct i2c_board_info i2c_bma250_devices[] = {
	{
		I2C_BOARD_INFO(BMA250_I2C_NAME_REMOVE_ECOMPASS, \
				0x32 >> 1),
		.platform_data = &gsensor_bma250_platform_data,
		.irq = MSM_GPIO_TO_INT(CP3DUG_GPIO_GSENSORS_INT),
	},
};

static struct bma250_platform_data gsensor_bma250_platform_data_evt = {
        .intr = CP3DUG_GPIO_GSENSORS_INT,
        .chip_layout = 1,
        .layouts = CP3DUG_LAYOUTS_EVT,
};

static struct i2c_board_info i2c_bma250_devices_evt[] = {
        {
                I2C_BOARD_INFO(BMA250_I2C_NAME_REMOVE_ECOMPASS, \
                                0x32 >> 1),
                .platform_data = &gsensor_bma250_platform_data_evt,
                .irq = MSM_GPIO_TO_INT(CP3DUG_GPIO_GSENSORS_INT),
        },
};


struct rt5501_platform_data rt5501_data = {
         .gpio_rt5501_spk_en = CPLD_EXT_GPIO_AUD_HP_EN,
};

static struct i2c_board_info msm_i2c_gsbi1_rt5501_info[] = {
	{
		I2C_BOARD_INFO( RT5501_I2C_NAME, RT5501_I2C_SLAVE_ADDR),
		.platform_data = &rt5501_data,
	},
};

static struct i2c_board_info msm_i2c_gsbi1_tfa9887_info[] = {
	{
		I2C_BOARD_INFO(TFA9887_I2C_NAME, TFA9887_I2C_SLAVE_ADDR)
	},
	{
		I2C_BOARD_INFO(TFA9887L_I2C_NAME, TFA9887L_I2C_SLAVE_ADDR)
	},
};

#if defined(CONFIG_I2C_CPLD)

static struct i2c_board_info i2c_cpld_devices[] = {
        {
                I2C_BOARD_INFO("cpld",0x70),
               
               .irq = MSM_GPIO_TO_INT(CP3DUG_GPIO_CPLD_INT),
        },
};
#endif

extern int emmc_partition_read_proc(char *page, char **start, off_t off,
					int count, int *eof, void *data);

static struct platform_device msm_wlan_ar6000_pm_device = {
	.name           = "wlan_ar6000_pm_dev",
	.id             = -1,
};
#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_3K

static ssize_t syn_vkeys_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 200,
	__stringify(EV_KEY) ":" __stringify(KEY_BACK) ":102:1024:97:97"
	
	":" __stringify(EV_KEY) ":" __stringify(KEY_HOME) ":447:1024:97:97"
	
	"\n");
}

static struct kobj_attribute syn_vkeys_attr = {
	.attr = {
		.name = "virtualkeys.synaptics-rmi-touchscreen",
		.mode = S_IRUGO,
	},
	.show = &syn_vkeys_show,
};

static struct attribute *syn_properties_attrs[] = {
	&syn_vkeys_attr.attr,
	NULL
};

static struct attribute_group syn_properties_attr_group = {
	.attrs = syn_properties_attrs,
};
static int synaptic_rmi_tp_power(int on)
{
	return 0;
}

static struct synaptics_i2c_rmi_platform_data syn_ts_3k_data[] = {
	{ 														
		.version = 0x3332,
		.packrat_number = 1473052,
		.abs_x_min = 11,
		.abs_x_max = 799,
		.abs_y_min = 11,
		.abs_y_max = 1338,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		#if defined(CONFIG_CPLD)
		.cpld_gpio_reset = CPLD_EXT_GPIO_TP_RST,
		#endif
		.tw_pin_mask = 0x0088,
		.sensor_id = 0x00 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.reduce_report_level = {0, 0, 0, 0, 0},
		.default_config = 1,
		.i2c_err_handler_en = 1,
		.config = {
			0x43, 0x50, 0x30, 0x37, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x46,
			0x80, 0x18, 0xF9, 0x01, 0x01, 0x3C, 0x1E, 0x01,
			0x1A, 0x02, 0x0A, 0x57, 0x00, 0x54, 0x7A, 0xAA,
			0x40, 0xB2, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x14, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF4, 0xC8,
			0xDC, 0x43, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x1E, 0x05, 0x00, 0x02, 0x4A,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x12, 0x3A, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x10, 0x0A, 0x60, 0x60,
			0x60, 0x60, 0x68, 0x60, 0x68, 0x60, 0x30, 0x2F,
			0x2E, 0x2D, 0x2C, 0x2B, 0x2A, 0x29, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xE8,
			0xFD, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x00, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x5A, 0x5D,
			0x5F, 0x61, 0x63, 0x66, 0x69, 0x6C, 0x00, 0x28,
			0x00, 0x64, 0xC8, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x06, 0x08, 0x0B, 0x0E, 0x0F, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x51, 0x51, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x12, 0x13, 0x16, 0x15, 0x17, 0x18,
			0x19, 0x1B, 0x1A, 0xFF, 0xFF, 0xFF, 0xFF, 0x02,
			0x07, 0x0B, 0x05, 0x00, 0x0F, 0x0A, 0x11, 0x10,
			0x0C, 0x03, 0x06, 0x0E, 0x12, 0x04, 0x09, 0x08,
			0x0D, 0x13, 0x01, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x19, 0x01,
			0x20, 0x01, 0x3C, 0x08, 0x84, 0x10, 0x20, 0x02,
			0x78, 0x14, 0x34, 0x03, 0x08
		},
	},
	{ 														
		.version = 0x3332,
		.packrat_number = 1473052,
		.abs_x_min = 11,
		.abs_x_max = 799,
		.abs_y_min = 11,
		.abs_y_max = 1338,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		#if defined(CONFIG_CPLD)
		.cpld_gpio_reset = CPLD_EXT_GPIO_TP_RST,
		#endif
		.tw_pin_mask = 0x0088,
		.sensor_id = 0x80 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.reduce_report_level = {0, 0, 0, 0, 0},
		.default_config = 1,
		.i2c_err_handler_en = 1,
		.config = {
			0x43, 0x50, 0x30, 0x37, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x46,
			0x80, 0x18, 0xF9, 0x01, 0x01, 0x3C, 0x1E, 0x01,
			0x1A, 0x02, 0x0A, 0x57, 0x00, 0x54, 0x7A, 0xAA,
			0x40, 0xB2, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x14, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF4, 0xC8,
			0xDC, 0x43, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x1E, 0x05, 0x00, 0x02, 0x4A,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x12, 0x3A, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x10, 0x0A, 0x60, 0x60,
			0x60, 0x60, 0x68, 0x60, 0x68, 0x60, 0x30, 0x2F,
			0x2E, 0x2D, 0x2C, 0x2B, 0x2A, 0x29, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xE8,
			0xFD, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x00, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x5A, 0x5D,
			0x5F, 0x61, 0x63, 0x66, 0x69, 0x6C, 0x00, 0x28,
			0x00, 0x64, 0xC8, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x06, 0x08, 0x0B, 0x0E, 0x0F, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x51, 0x51, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x12, 0x13, 0x16, 0x15, 0x17, 0x18,
			0x19, 0x1B, 0x1A, 0xFF, 0xFF, 0xFF, 0xFF, 0x02,
			0x07, 0x0B, 0x05, 0x00, 0x0F, 0x0A, 0x11, 0x10,
			0x0C, 0x03, 0x06, 0x0E, 0x12, 0x04, 0x09, 0x08,
			0x0D, 0x13, 0x01, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x19, 0x01,
			0x20, 0x01, 0x3C, 0x08, 0x84, 0x10, 0x20, 0x02,
			0x78, 0x14, 0x34, 0x03, 0x08
		},
	},
	{ 														
		.version = 0x3332,
		.packrat_number = 1460308,
		.abs_x_min = 8,
		.abs_x_max = 792,
		.abs_y_min = 8,
		.abs_y_max = 1340,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		#if defined(CONFIG_CPLD)
		.cpld_gpio_reset = CPLD_EXT_GPIO_TP_RST,
		#endif
		.tw_pin_mask = 0x0088,
		.sensor_id = 0x00 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.reduce_report_level = {0, 0, 0, 0, 0},
		.default_config = 1,
		.i2c_err_handler_en = 1,
		.config = {
			0x43, 0x50, 0x30, 0x38, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x3C,
			0x98, 0x13, 0x2F, 0x02, 0x01, 0x3C, 0x1D, 0x01,
			0x16, 0x02, 0x0A, 0x57, 0x00, 0x54, 0x7A, 0xAA,
			0x40, 0xB2, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x14, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF3, 0xC8,
			0xDC, 0x43, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x1E, 0x05, 0x00, 0x02, 0x4A,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x12, 0x3A, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x10, 0x0A, 0x60, 0x60,
			0x60, 0x60, 0x68, 0x60, 0x68, 0x60, 0x30, 0x2F,
			0x2E, 0x2D, 0x2C, 0x2B, 0x2A, 0x29, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x88,
			0x13, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x66, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x5A, 0x5D,
			0x5F, 0x61, 0x63, 0x66, 0x69, 0x6C, 0x00, 0xB4,
			0x00, 0x64, 0xC8, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x06, 0x08, 0x0B, 0x0E, 0x0F, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x51, 0x51, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x12, 0x13, 0x16, 0x15, 0x17, 0x18,
			0x19, 0x1B, 0x1A, 0xFF, 0xFF, 0xFF, 0xFF, 0x02,
			0x07, 0x0B, 0x05, 0x00, 0x0F, 0x0A, 0x11, 0x10,
			0x0C, 0x03, 0x06, 0x0E, 0x12, 0x04, 0x09, 0x08,
			0x0D, 0x13, 0x01, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x19, 0x01,
			0x20, 0x01, 0x02
		},
	},
	{ 														
		.version = 0x3332,
		.packrat_number = 1460308,
		.abs_x_min = 8,
		.abs_x_max = 792,
		.abs_y_min = 8,
		.abs_y_max = 1340,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		#if defined(CONFIG_CPLD)
		.cpld_gpio_reset = CPLD_EXT_GPIO_TP_RST,
		#endif
		.tw_pin_mask = 0x0088,
		.sensor_id = 0x80 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.reduce_report_level = {0, 0, 0, 0, 0},
		.default_config = 1,
		.i2c_err_handler_en = 1,
		.config = {
			0x43, 0x50, 0x30, 0x38, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x3C,
			0x98, 0x13, 0x2F, 0x02, 0x01, 0x3C, 0x1D, 0x01,
			0x16, 0x02, 0x0A, 0x57, 0x00, 0x54, 0x7A, 0xAA,
			0x40, 0xB2, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x14, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF3, 0xC8,
			0xDC, 0x43, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x1E, 0x05, 0x00, 0x02, 0x4A,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x12, 0x3A, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x10, 0x0A, 0x60, 0x60,
			0x60, 0x60, 0x68, 0x60, 0x68, 0x60, 0x30, 0x2F,
			0x2E, 0x2D, 0x2C, 0x2B, 0x2A, 0x29, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x88,
			0x13, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x66, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x5A, 0x5D,
			0x5F, 0x61, 0x63, 0x66, 0x69, 0x6C, 0x00, 0xB4,
			0x00, 0x64, 0xC8, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x06, 0x08, 0x0B, 0x0E, 0x0F, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x51, 0x51, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x12, 0x13, 0x16, 0x15, 0x17, 0x18,
			0x19, 0x1B, 0x1A, 0xFF, 0xFF, 0xFF, 0xFF, 0x02,
			0x07, 0x0B, 0x05, 0x00, 0x0F, 0x0A, 0x11, 0x10,
			0x0C, 0x03, 0x06, 0x0E, 0x12, 0x04, 0x09, 0x08,
			0x0D, 0x13, 0x01, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x19, 0x01,
			0x20, 0x01, 0x02
		},
	},
	{ 														
		.version = 0x3332,
		.packrat_number = 1457068,
		.abs_x_min = 0,
		.abs_x_max = 800,
		.abs_y_min = 0,
		.abs_y_max = 1320,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		#if defined(CONFIG_CPLD)
		.cpld_gpio_reset = CPLD_EXT_GPIO_TP_RST,
		#endif
		.tw_pin_mask = 0x0088,
		.sensor_id = 0x00 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.reduce_report_level = {0, 0, 0, 0, 0},
		.default_config = 1,
		.i2c_err_handler_en = 1,
		.config = {
			0x43, 0x50, 0x30, 0x36, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x3C,
			0x98, 0x13, 0x2F, 0x02, 0x01, 0x3C, 0x1D, 0x01,
			0x16, 0x02, 0x0A, 0x57, 0x00, 0x54, 0x7A, 0xAA,
			0x40, 0xB2, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x14, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF4, 0xC8,
			0xDC, 0x43, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x1E, 0x05, 0x00, 0x02, 0x4A,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x12, 0x3A, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x10, 0x0A, 0x60, 0x60,
			0x60, 0x60, 0x68, 0x60, 0x68, 0x60, 0x30, 0x2F,
			0x2E, 0x2D, 0x2C, 0x2B, 0x2A, 0x29, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x88,
			0x13, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x66, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x5A, 0x5D,
			0x5F, 0x61, 0x63, 0x66, 0x69, 0x6C, 0x00, 0xB4,
			0x00, 0x64, 0xC8, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x06, 0x08, 0x0B, 0x0E, 0x0F, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x51, 0x51, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x12, 0x13, 0x16, 0x15, 0x17, 0x18,
			0x19, 0x1B, 0x1A, 0xFF, 0xFF, 0xFF, 0xFF, 0x02,
			0x07, 0x0B, 0x05, 0x00, 0x0F, 0x0A, 0x11, 0x10,
			0x0C, 0x03, 0x06, 0x0E, 0x12, 0x04, 0x09, 0x08,
			0x0D, 0x13, 0x01, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x19, 0x01,
			0x20, 0x01
		},
	},
	{ 														
		.version = 0x3332,
		.packrat_number = 1457068,
		.abs_x_min = 0,
		.abs_x_max = 800,
		.abs_y_min = 0,
		.abs_y_max = 1320,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		#if defined(CONFIG_CPLD)
		.cpld_gpio_reset = CPLD_EXT_GPIO_TP_RST,
		#endif
		.tw_pin_mask = 0x0088,
		.sensor_id = 0x80 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.reduce_report_level = {0, 0, 0, 0, 0},
		.default_config = 1,
		.i2c_err_handler_en = 1,
		.config = {
			0x43, 0x50, 0x30, 0x36, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x3C,
			0x98, 0x13, 0x2F, 0x02, 0x01, 0x3C, 0x1D, 0x01,
			0x16, 0x02, 0x0A, 0x57, 0x00, 0x54, 0x7A, 0xAA,
			0x40, 0xB2, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x14, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF4, 0xC8,
			0xDC, 0x43, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x1E, 0x05, 0x00, 0x02, 0x4A,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x12, 0x3A, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x10, 0x0A, 0x60, 0x60,
			0x60, 0x60, 0x68, 0x60, 0x68, 0x60, 0x30, 0x2F,
			0x2E, 0x2D, 0x2C, 0x2B, 0x2A, 0x29, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x88,
			0x13, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x66, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x5A, 0x5D,
			0x5F, 0x61, 0x63, 0x66, 0x69, 0x6C, 0x00, 0xB4,
			0x00, 0x64, 0xC8, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x06, 0x08, 0x0B, 0x0E, 0x0F, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x51, 0x51, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x12, 0x13, 0x16, 0x15, 0x17, 0x18,
			0x19, 0x1B, 0x1A, 0xFF, 0xFF, 0xFF, 0xFF, 0x02,
			0x07, 0x0B, 0x05, 0x00, 0x0F, 0x0A, 0x11, 0x10,
			0x0C, 0x03, 0x06, 0x0E, 0x12, 0x04, 0x09, 0x08,
			0x0D, 0x13, 0x01, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x19, 0x01,
			0x20, 0x01
		},
	},
	{ 														
		.version = 0x3332,
		.packrat_number = 1423922,
		.abs_x_min = 0,
		.abs_x_max = 800,
		.abs_y_min = 0,
		.abs_y_max = 1320,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		#if defined(CONFIG_CPLD)
		.cpld_gpio_reset = CPLD_EXT_GPIO_TP_RST,
		#endif
		.tw_pin_mask = 0x0088,
		.sensor_id = 0x00 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.reduce_report_level = {65, 65, 50, 0, 0},
		.default_config = 1,
		.i2c_err_handler_en = 1,
		.config = {
			0x43, 0x50, 0x30, 0x34, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x3C,
			0x98, 0x13, 0x2F, 0x02, 0x01, 0x3C, 0x1D, 0x01,
			0x16, 0x02, 0x0A, 0x57, 0x00, 0x54, 0x7A, 0xAA,
			0x40, 0xB2, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x14, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF6, 0xC8,
			0xC0, 0x43, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x1E, 0x05, 0x00, 0x02, 0x4A,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x12, 0x3A, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x10, 0x0A, 0x60, 0x60,
			0x60, 0x60, 0x68, 0x60, 0x68, 0x60, 0x30, 0x2F,
			0x2E, 0x2D, 0x2C, 0x2B, 0x2A, 0x29, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x88,
			0x13, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x66, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x5A, 0x5D,
			0x5F, 0x61, 0x63, 0x66, 0x69, 0x6C, 0x00, 0xB4,
			0x00, 0x64, 0xC8, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x06, 0x08, 0x0B, 0x0E, 0x0F, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x51, 0x51, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x12, 0x13, 0x16, 0x15, 0x17, 0x18,
			0x19, 0x1B, 0x1A, 0xFF, 0xFF, 0xFF, 0xFF, 0x02,
			0x07, 0x0B, 0x05, 0x00, 0x0F, 0x0A, 0x11, 0x10,
			0x0C, 0x03, 0x06, 0x0E, 0x12, 0x04, 0x09, 0x08,
			0x0D, 0x13, 0x01, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x0F, 0x01,
			0x20, 0x01
		},
	},
	{ 														
		.version = 0x3332,
		.packrat_number = 1423922,
		.abs_x_min = 0,
		.abs_x_max = 800,
		.abs_y_min = 0,
		.abs_y_max = 1320,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		#if defined(CONFIG_CPLD)
		.cpld_gpio_reset = CPLD_EXT_GPIO_TP_RST,
		#endif
		.tw_pin_mask = 0x0088,
		.sensor_id = 0x80 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.reduce_report_level = {65, 65, 50, 0, 0},
		.default_config = 1,
		.i2c_err_handler_en = 1,
		.config = {
			0x43, 0x50, 0x30, 0x31, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x50,
			0x4D, 0x07, 0x74, 0x01, 0x01, 0x3C, 0x34, 0x00,
			0x2A, 0x01, 0x0A, 0x57, 0x00, 0x54, 0x25, 0xBC,
			0xEB, 0xA0, 0x01, 0xE0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x14, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x64, 0x07, 0xF4, 0x96,
			0xD2, 0x43, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0xE8, 0xD0, 0x73, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x1E, 0x05, 0x00, 0x02, 0x5B,
			0x01, 0x80, 0x01, 0x0E, 0x1F, 0x13, 0x43, 0x00,
			0x19, 0x04, 0x1B, 0x00, 0x64, 0xC8, 0x60, 0x60,
			0x40, 0x40, 0x48, 0x48, 0x48, 0x28, 0x29, 0x28,
			0x27, 0x26, 0x24, 0x23, 0x22, 0x21, 0x00, 0x00,
			0x00, 0x00, 0x01, 0x04, 0x06, 0x09, 0x00, 0x88,
			0x13, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x80, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x04, 0x03, 0x20, 0x20,
			0x10, 0x10, 0x10, 0x10, 0x30, 0x20, 0x69, 0x6D,
			0x38, 0x3A, 0x3C, 0x3E, 0x60, 0x58, 0x00, 0x8C,
			0x00, 0x64, 0xC8, 0x00, 0x00, 0x00, 0x03, 0x06,
			0x0A, 0x0D, 0x0E, 0x10, 0x11, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x51, 0x51, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x12, 0x13, 0x16, 0x15, 0x17, 0x18,
			0x19, 0x1B, 0x1A, 0xFF, 0xFF, 0xFF, 0xFF, 0x02,
			0x07, 0x0B, 0x05, 0x00, 0x0F, 0x0A, 0x11, 0x10,
			0x0C, 0x03, 0x06, 0x0E, 0x12, 0x04, 0x09, 0x08,
			0x0D, 0x13, 0x01, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x0A, 0x00,
			0x4F, 0x53
		},
	},
	{ 														
		.version = 0x3332,
		.packrat_number = 1293984,
		.abs_x_min = 0,
		.abs_x_max = 800,
		.abs_y_min = 0,
		.abs_y_max = 1320,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		#if defined(CONFIG_CPLD)
		.cpld_gpio_reset = CPLD_EXT_GPIO_TP_RST,
		#endif
		.tw_pin_mask = 0x0088,
		.sensor_id = 0x00 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.reduce_report_level = {65, 65, 50, 0, 0},
		.default_config = 1,
		.i2c_err_handler_en = 1,
		.config = {
			0x43, 0x50, 0x30, 0x31, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x50,
			0x4D, 0x07, 0x74, 0x01, 0x01, 0x3C, 0x34, 0x00,
			0x2A, 0x01, 0x0A, 0x57, 0x00, 0x54, 0x25, 0xBC,
			0xEB, 0xA0, 0x01, 0xE0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x14, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x64, 0x07, 0xF4, 0x96,
			0xD2, 0x43, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0xE8, 0xD0, 0x73, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x1E, 0x05, 0x00, 0x02, 0x5B,
			0x01, 0x80, 0x01, 0x0E, 0x1F, 0x13, 0x43, 0x00,
			0x19, 0x04, 0x1B, 0x00, 0x64, 0xC8, 0x60, 0x60,
			0x40, 0x40, 0x48, 0x48, 0x48, 0x28, 0x29, 0x28,
			0x27, 0x26, 0x24, 0x23, 0x22, 0x21, 0x00, 0x00,
			0x00, 0x00, 0x01, 0x04, 0x06, 0x09, 0x00, 0x88,
			0x13, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x80, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x04, 0x03, 0x20, 0x20,
			0x10, 0x10, 0x10, 0x10, 0x30, 0x20, 0x69, 0x6D,
			0x38, 0x3A, 0x3C, 0x3E, 0x60, 0x58, 0x00, 0x8C,
			0x00, 0x64, 0xC8, 0x00, 0x00, 0x00, 0x03, 0x06,
			0x0A, 0x0D, 0x0E, 0x10, 0x11, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x51, 0x51, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x12, 0x13, 0x16, 0x15, 0x17, 0x18,
			0x19, 0x1B, 0x1A, 0xFF, 0xFF, 0xFF, 0xFF, 0x02,
			0x07, 0x0B, 0x05, 0x00, 0x0F, 0x0A, 0x11, 0x10,
			0x0C, 0x03, 0x06, 0x0E, 0x12, 0x04, 0x09, 0x08,
			0x0D, 0x13, 0x01, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x0A, 0x00,
			0x4F, 0x53
		},
	},
	{ 														
		.version = 0x3332,
		.packrat_number = 1293984,
		.abs_x_min = 0,
		.abs_x_max = 800,
		.abs_y_min = 0,
		.abs_y_max = 1320,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		#if defined(CONFIG_CPLD)
		.cpld_gpio_reset = CPLD_EXT_GPIO_TP_RST,
		#endif
		.tw_pin_mask = 0x0088,
		.sensor_id = 0x80 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.reduce_report_level = {65, 65, 50, 0, 0},
		.default_config = 1,
		.i2c_err_handler_en = 1,
		.config = {
			0x43, 0x50, 0x30, 0x31, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x50,
			0x4D, 0x07, 0x74, 0x01, 0x01, 0x3C, 0x34, 0x00,
			0x2A, 0x01, 0x0A, 0x57, 0x00, 0x54, 0x25, 0xBC,
			0xEB, 0xA0, 0x01, 0xE0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x14, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x64, 0x07, 0xF4, 0x96,
			0xD2, 0x43, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0xE8, 0xD0, 0x73, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x1E, 0x05, 0x00, 0x02, 0x5B,
			0x01, 0x80, 0x01, 0x0E, 0x1F, 0x13, 0x43, 0x00,
			0x19, 0x04, 0x1B, 0x00, 0x64, 0xC8, 0x60, 0x60,
			0x40, 0x40, 0x48, 0x48, 0x48, 0x28, 0x29, 0x28,
			0x27, 0x26, 0x24, 0x23, 0x22, 0x21, 0x00, 0x00,
			0x00, 0x00, 0x01, 0x04, 0x06, 0x09, 0x00, 0x88,
			0x13, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x80, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x04, 0x03, 0x20, 0x20,
			0x10, 0x10, 0x10, 0x10, 0x30, 0x20, 0x69, 0x6D,
			0x38, 0x3A, 0x3C, 0x3E, 0x60, 0x58, 0x00, 0x8C,
			0x00, 0x64, 0xC8, 0x00, 0x00, 0x00, 0x03, 0x06,
			0x0A, 0x0D, 0x0E, 0x10, 0x11, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x51, 0x51, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x12, 0x13, 0x16, 0x15, 0x17, 0x18,
			0x19, 0x1B, 0x1A, 0xFF, 0xFF, 0xFF, 0xFF, 0x02,
			0x07, 0x0B, 0x05, 0x00, 0x0F, 0x0A, 0x11, 0x10,
			0x0C, 0x03, 0x06, 0x0E, 0x12, 0x04, 0x09, 0x08,
			0x0D, 0x13, 0x01, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x0A, 0x00,
			0x4F, 0x53
		},
	},
	{ 														
		.version = 0x3332,
		.packrat_number = 1293984,
		.abs_x_min = 0,
		.abs_x_max = 800,
		.abs_y_min = 0,
		.abs_y_max = 1320,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		#if defined(CONFIG_CPLD)
		.cpld_gpio_reset = CPLD_EXT_GPIO_TP_RST,
		#endif
		.tw_pin_mask = 0x0088,
		.sensor_id = 0x08 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.reduce_report_level = {65, 65, 50, 0, 0},
		.default_config = 1,
		.i2c_err_handler_en = 1,
		.config = {
			0x43, 0x50, 0x33, 0x30, 0x04, 0x7F, 0x03, 0x1E,
			0x05, 0x08, 0x00, 0x19, 0x19, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x50,
			0x4D, 0x17, 0xAC, 0x02, 0x01, 0x3C, 0x1D, 0x01,
			0x22, 0x01, 0x66, 0x52, 0x5C, 0x5B, 0xD4, 0xC5,
			0x05, 0xC1, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x00,
			0x0A, 0x04, 0xAD, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0B, 0x13, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF6, 0xBE,
			0xC8, 0x42, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x1E, 0x05, 0x20, 0x02, 0x2C,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x12, 0x3E, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x10, 0xFF, 0x60, 0x68,
			0x60, 0x68, 0x60, 0x68, 0x48, 0x48, 0x2E, 0x2D,
			0x2C, 0x2B, 0x2A, 0x29, 0x27, 0x26, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x88,
			0x13, 0x00, 0x64, 0x00, 0xC8, 0x00, 0xCD, 0x14,
			0xC0, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x5E, 0x61,
			0x63, 0x66, 0x69, 0x6C, 0x6F, 0x39, 0x00, 0x78,
			0x00, 0x10, 0x28, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x07, 0x0A, 0x0D, 0x10, 0x11, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x51, 0x51, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x17, 0x15, 0x18, 0x16, 0x19, 0x13, 0x1B,
			0x12, 0x1A, 0x14, 0x11, 0xFF, 0xFF, 0xFF, 0x09,
			0x0F, 0x08, 0x0A, 0x0D, 0x11, 0x13, 0x10, 0x01,
			0x0C, 0x04, 0x05, 0x12, 0x0B, 0x0E, 0x07, 0x06,
			0x02, 0x03, 0xFF, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x23, 0x01,
			0x00, 0x00
		},
	},
	{ 														
		.version = 0x3332,
		.packrat_number = 1293984,
		.abs_x_min = 0,
		.abs_x_max = 800,
		.abs_y_min = 0,
		.abs_y_max = 1320,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		#if defined(CONFIG_CPLD)
		.cpld_gpio_reset = CPLD_EXT_GPIO_TP_RST,
		#endif
		.tw_pin_mask = 0x0088,
		.sensor_id = 0x88 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.reduce_report_level = {65, 65, 50, 0, 0},
		.default_config = 1,
		.i2c_err_handler_en = 1,
		.config = {
			0x43, 0x50, 0x33, 0x30, 0x04, 0x7F, 0x03, 0x1E,
			0x05, 0x08, 0x00, 0x19, 0x19, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x50,
			0x4D, 0x17, 0xAC, 0x02, 0x01, 0x3C, 0x1D, 0x01,
			0x22, 0x01, 0x66, 0x52, 0x5C, 0x5B, 0xD4, 0xC5,
			0x05, 0xC1, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x00,
			0x0A, 0x04, 0xAD, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0B, 0x13, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF6, 0xBE,
			0xC8, 0x42, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x2A, 0xA0, 0x53, 0x1E, 0x05, 0x20, 0x02, 0x2C,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x12, 0x3E, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x10, 0xFF, 0x60, 0x68,
			0x60, 0x68, 0x60, 0x68, 0x48, 0x48, 0x2E, 0x2D,
			0x2C, 0x2B, 0x2A, 0x29, 0x27, 0x26, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x88,
			0x13, 0x00, 0x64, 0x00, 0xC8, 0x00, 0xCD, 0x14,
			0xC0, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x5E, 0x61,
			0x63, 0x66, 0x69, 0x6C, 0x6F, 0x39, 0x00, 0x78,
			0x00, 0x10, 0x28, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x07, 0x0A, 0x0D, 0x10, 0x11, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x51, 0x51, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x17, 0x15, 0x18, 0x16, 0x19, 0x13, 0x1B,
			0x12, 0x1A, 0x14, 0x11, 0xFF, 0xFF, 0xFF, 0x09,
			0x0F, 0x08, 0x0A, 0x0D, 0x11, 0x13, 0x10, 0x01,
			0x0C, 0x04, 0x05, 0x12, 0x0B, 0x0E, 0x07, 0x06,
			0x02, 0x03, 0xFF, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x23, 0x01,
			0x00, 0x00
		},
	},
	{
		.version = 0x3330,
		.packrat_number = 974353,
		.abs_x_min = 0,
		.abs_x_max = 1100,
		.abs_y_min = 0,
		.abs_y_max = 1770,
		.display_width = 544,
		.display_height = 960,
		.flags = SYNAPTICS_FLIP_X,
		.gpio_irq = MSM_TP_ATTz,
		.large_obj_check = 1,
		.tw_pin_mask = 0x0080,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.segmentation_bef_unlock = 0x50,
		.config = {
			0x32, 0x30, 0x30, 0x31, 0x04, 0x0F, 0x03, 0x1E,
			0x05, 0x20, 0xB1, 0x00, 0x0B, 0x19, 0x19, 0x00,
			0x00, 0x4C, 0x04, 0x6C, 0x07, 0x1E, 0x05, 0x28,
			0xF5, 0x28, 0x1E, 0x05, 0x01, 0x30, 0x00, 0x30,
			0x00, 0x00, 0x48, 0x00, 0x48, 0x44, 0xA0, 0xD3,
			0xA1, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x0A,
			0x04, 0xC0, 0x00, 0x02, 0xA1, 0x01, 0x80, 0x02,
			0x0D, 0x1E, 0x00, 0x8C, 0x00, 0x19, 0x04, 0x1E,
			0x00, 0x10, 0x0A, 0x01, 0x11, 0x14, 0x1A, 0x12,
			0x1B, 0x13, 0x19, 0x16, 0x18, 0x15, 0x17, 0xFF,
			0xFF, 0xFF, 0x09, 0x0F, 0x08, 0x0A, 0x0D, 0x11,
			0x13, 0x10, 0x01, 0x0C, 0x04, 0x05, 0x12, 0x0B,
			0x0E, 0x07, 0x06, 0x02, 0x03, 0xFF, 0x40, 0x40,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x24, 0x23,
			0x21, 0x20, 0x1F, 0x1D, 0x1C, 0x1A, 0x00, 0x07,
			0x0F, 0x18, 0x21, 0x2B, 0x37, 0x43, 0x00, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0x00, 0xFF, 0xFF, 0x00, 0xC0, 0x80, 0x00, 0x10,
			0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x02, 0x02, 0x02, 0x07, 0x02, 0x03,
			0x09, 0x03, 0x10, 0x10, 0x10, 0x40, 0x10, 0x10,
			0x40, 0x10, 0x59, 0x5D, 0x61, 0x74, 0x6A, 0x4A,
			0x68, 0x52, 0x30, 0x30, 0x00, 0x1E, 0x19, 0x05,
			0x00, 0x00, 0x3D, 0x08
		}
	},
	{
		.version = 0x3330,
		.packrat_number = 7788,
		.abs_x_min = 0,
		.abs_x_max = 1100,
		.abs_y_min = 0,
		.abs_y_max = 1770,
		.display_width = 544,
		.display_height = 960,
		.flags = SYNAPTICS_FLIP_X,
		.gpio_irq = MSM_TP_ATTz,
		.large_obj_check = 1,
		.tw_pin_mask = 0x0080,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.segmentation_bef_unlock = 0x50,
		.power = synaptic_rmi_tp_power,
	},
};

static struct i2c_board_info i2c_touch_device[] = {
	{
		I2C_BOARD_INFO(SYNAPTICS_3200_NAME, 0x40 >> 1),
		.platform_data = &syn_ts_3k_data,
		.irq = MSM_GPIO_TO_INT(MSM_TP_ATTz)
	},
};

static void syn_init_vkeys_cp3(void)
{
	int rc = 0;
	static struct kobject *syn_properties_kobj;

	syn_properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (syn_properties_kobj)
		rc = sysfs_create_group(syn_properties_kobj, &syn_properties_attr_group);
	if (!syn_properties_kobj || rc)
		pr_err("%s: failed to create board_properties\n", __func__);
	else {
		for (rc = 0; rc < ARRAY_SIZE(syn_ts_3k_data); rc++) {
			syn_ts_3k_data[rc].vk_obj = syn_properties_kobj;
			syn_ts_3k_data[rc].vk2Use = &syn_vkeys_attr;
		}
	}

	return;
}

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

static struct msm_gpio msm8625q_i2c_gpio_config[] = {
	{ GPIO_CFG(31, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_10MA),
		"qup_scl" },
	{ GPIO_CFG(32, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_sda" },
};

static struct i2c_gpio_platform_data msm8625q_i2c_gpio_pdata = {
	.scl_pin = 31,
	.sda_pin = 32,
	.udelay = 5, 
};

static struct platform_device msm8625q_i2c_gpio = {
	.name	= "i2c-gpio",
	.id	= 2,
	.dev	= {
		.platform_data = &msm8625q_i2c_gpio_pdata,
	}
};

#ifdef CONFIG_I2C_CPLD
static struct msm_gpio msm8625q_i2c_cpld_config[] = {
	
	
	
	

	
#if 0
	
        { GPIO_CFG(4, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),   "CPLD_CLK"},
#endif
	{ GPIO_CFG(39, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   "CPLD_INT"},
	{ GPIO_CFG(49, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),  "CPLD_RST"},
        { GPIO_CFG(116, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), "CPLD_I2C_EN"},
	

};

static struct i2c_gpio_platform_data msm8625q_i2c_cpld_pdata = {
	.scl_pin = 33,
	.sda_pin = 82,
	
        .udelay = 1,
        .timeout = 100,
        .sda_is_open_drain = 1,
        .scl_is_open_drain = 1,
};

static struct platform_device msm8625q_i2c_cpld = {
	.name	= "i2c-cpld",
	.id	= 3,
	.dev	= {
		.platform_data = &msm8625q_i2c_cpld_pdata,
	}
};
#endif

#ifdef CONFIG_SDIO_TTY_SP6502COM
static struct platform_device sprd_notify_device = {
	.name	= "mdm_sprd",
	.id	= -1,
};

static void __init sprd_init(void)
{
	printk(KERN_INFO "[SPRD]device init");
	platform_device_register(&sprd_notify_device);
}
#endif 

#ifdef CONFIG_ARCH_MSM7X27A
#define CAMERA_ZSL_SIZE     (SZ_1M * 111)  

#ifdef CONFIG_ION_MSM
#define MSM_ION_HEAP_NUM	4
static struct platform_device ion_dev;
static int msm_ion_camera_size;
static int msm_ion_audio_size;
static int msm_ion_sf_size;
#endif
#endif

#define PM8058ADC_16BIT(adc) ((adc * 1800) / 65535) 
int64_t cp3dug_get_usbid_adc(void)
{
	uint32_t adc_value = 0xffffffff;
#if 1
	htc_get_usb_accessory_adc_level(&adc_value);
#endif
	adc_value = PM8058ADC_16BIT(adc_value);
	return adc_value;
}

void config_cp3dug_usb_id_gpios(bool output)
{
	int ret = 0;
	if (output) {
		ret =cpld_irq_mask(CPLD_IRQ_USB_ID);
		if (ret < 0)
			printk(KERN_ERR "[CABLE]%s CPLD_IRQ_USB_IRQ mask fail\n", __func__);
		cpld_gpio_config(CPLD_EXT_GPIO_EXT_USB_ID_OUTPUT,  CPLD_GPIO_OUT);
		cpld_gpio_write(CPLD_EXT_GPIO_EXT_USB_ID_OUTPUT, 1);
		printk(KERN_INFO "[CABLE]%s CPLD_EXT_GPIO_EXT_USB_ID_OUTPUT, output high\n",  __func__);
	} else {
		cpld_gpio_config(CPLD_EXT_GPIO_EXT_USB_ID_OUTPUT,  CPLD_GPIO_IN);
		ret = cpld_irq_unmask(CPLD_IRQ_USB_ID);
		if (ret < 0)
			printk(KERN_ERR "[CABLE]%s CPLD_IRQ_USB_IRQ unmask fail\n",__func__);
		printk(KERN_INFO "[CABLE]%s CPLD_EXT_GPIO_EXT_USB_ID_OUTPUT, input_no_pull\n",  __func__);
	}
}

static struct cable_detect_platform_data cable_detect_pdata = {
	.detect_type = CABLE_TYPE_PMIC_ADC,
	.usb_id_pin_gpio = ID_PIN_CPLD,
	.config_usb_id_gpios = config_cp3dug_usb_id_gpios,
	.get_adc_cb             = cp3dug_get_usbid_adc,
};

static struct platform_device cable_detect_device = {
	.name   = "cable_detect",
	.id     = -1,
	.dev    = {
		.platform_data = &cable_detect_pdata,
	},
};

static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id              = 0x0bb4,
#ifdef CONFIG_MACH_PROTOU
	.product_id             = 0x0dd5,
#else
	.product_id             = 0x0dcf,
#endif
	.version                = 0x0100,
	.product_name           = "Android Phone",
	.manufacturer_name      = "HTC",
	.num_products           = ARRAY_SIZE(usb_products),
	.products               = usb_products,
	.num_functions          = ARRAY_SIZE(usb_functions_all),
	.functions              = usb_functions_all,
	.usb_id_pin_gpio	= ID_PIN_CPLD,
	.usb_diag_interface	= "diag",
	.fserial_init_string = "tty:modem,tty:autobot,tty:serial,tty:autobot",
	.nluns                  = 2,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};

static int __init board_serialno_setup(char *serialno)
{
        android_usb_pdata.serial_number = serialno;
        return 1;
}
__setup("androidboot.serialno=", board_serialno_setup);

#ifdef CONFIG_USB_EHCI_MSM_72K
static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{
	int rc = 0;
	unsigned gpio;

	gpio = QRD_GPIO_HOST_VBUS_EN;

	rc = gpio_request(gpio,	"i2c_host_vbus_en");
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

static void __init msm7627a_init_host(void)
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

static struct msm_otg_platform_data msm_otg_pdata = {
#ifndef CONFIG_USB_EHCI_MSM_72K
	.pmic_vbus_notif_init	 = msm_hsusb_pmic_notif_init,
#else
	.vbus_power		 = msm_hsusb_vbus_power,
#endif
	.rpc_connect		 = hsusb_rpc_connect,
	.pemp_level		 = PRE_EMPHASIS_WITH_20_PERCENT,
	.cdr_autoreset		 = CDR_AUTO_RESET_DISABLE,
	.drv_ampl		 = HS_DRV_AMPLITUDE_DEFAULT,
	.se1_gating		 = SE1_GATING_DISABLE,
	.ldo_init		 = msm_hsusb_ldo_init,
	.ldo_enable		 = msm_hsusb_ldo_enable,
	.chg_init		 = hsusb_chg_init,
	.chg_connected		 = hsusb_chg_connected,
	.chg_vbus_draw		 = hsusb_chg_vbus_draw,
};
#endif

#define USB_PHY_ID 16
static uint32_t usb_phy_id_gpio[] = {
	GPIO_CFG(USB_PHY_ID, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

void usb_phy_id_gpio_setting(void)
{
	gpio_tlmm_config(usb_phy_id_gpio[0], GPIO_CFG_ENABLE);
	gpio_set_value_cansleep(USB_PHY_ID, 1);
}

void cp3_add_usb_devices(void)
{
#if 0
FIXME
	printk(KERN_INFO "%s rev: %d\n", __func__, system_rev);
#endif
	usb_phy_id_gpio_setting();
	android_usb_pdata.products[0].product_id =
		android_usb_pdata.product_id;

	
	if (get_radio_flag() & 0x20000)
		android_usb_pdata.diag_init = 1;
	
	if (board_mfg_mode() == 0) {
		android_usb_pdata.nluns = 1;
		android_usb_pdata.cdrom_lun = 0x1;
	}

	msm8625_device_gadget_peripheral.dev.parent = &msm8625_device_otg.dev;
	platform_device_register(&msm8625_device_gadget_peripheral);
	platform_device_register(&android_usb_device);
}

#ifdef CONFIG_PERFLOCK
static unsigned msm8x25q_perf_acpu_table[] = {
	700800000, 
	700800000, 
	1008000000,
	1008000000,
	1209600000,
};

static struct perflock_data msm8x25q_floor_data = {
	.perf_acpu_table = msm8x25q_perf_acpu_table,
	.table_size = ARRAY_SIZE(msm8x25q_perf_acpu_table),
};

static struct perflock_data msm8x25q_cpufreq_ceiling_data = {
	.perf_acpu_table = msm8x25q_perf_acpu_table,
	.table_size = ARRAY_SIZE(msm8x25q_perf_acpu_table),
};

static struct perflock_pdata perflock_pdata = {
	.perf_floor = &msm8x25q_floor_data,
	.perf_ceiling = &msm8x25q_cpufreq_ceiling_data,
};

struct platform_device msm8x25q_device_perf_lock = {
	.name = "perf_lock",
	.id = -1,
	.dev = {
		.platform_data = &perflock_pdata,
	},
};

#ifdef CONFIG_PERFLOCK_SCREEN_POLICY
static struct perf_lock screen_off_ceiling_lock;
static struct perflock_screen_policy msm8x25q_screen_off_policy = {
	.on_min  = NULL,
	.off_max = &screen_off_ceiling_lock,
};
#endif 
#endif

static int get_thermal_id(void)
{
	return THERMAL_470_100_4360;
}
static int get_battery_id(void)
{
	return BATTERY_ID_FORMOSA_SANYO;
}

static int dq_result = 0;
static void cp3dug_poweralg_config_init(struct poweralg_config_type *config)
{
	INT32 batt_id = 0;

	batt_id = get_batt_id();

	pr_info("[BATT] dq_result -> %d, batt_id -> %d\n",dq_result, batt_id);
	if (dq_result == 1) {
		config->full_charging_mv = 4250;
		config->voltage_exit_full_mv = 4200;
		config->capacity_recharge_p = 98;
	} else {
		config->full_charging_mv = 4100;
		config->voltage_exit_full_mv = 4000;
		config->capacity_recharge_p = 98;
	}

	config->full_charging_ma = 180;
	config->full_pending_ma = 50;
	config->full_charging_timeout_sec = 60 * 60;
	config->min_taper_current_mv = 0;	
	config->min_taper_current_ma = 0;	
	config->wait_votlage_statble_sec = 1 * 60;
	config->predict_timeout_sec = 10;
	config->polling_time_in_charging_sec = 30;

	config->enable_full_calibration = TRUE;
	config->enable_weight_percentage = TRUE;
	config->software_charger_timeout_sec = 57600; 
	config->superchg_software_charger_timeout_sec = 0;	
	config->charger_hw_safety_timer_watchdog_sec =  0;	

	config->debug_disable_shutdown = FALSE;
	config->debug_fake_room_temp = FALSE;

	config->debug_disable_hw_timer = FALSE;
	config->debug_always_predict = FALSE;
	config->full_level = 0;
}

static int cp3dug_update_charging_protect_flag(int ibat_ma, int vbat_mv, int temp_01c, BOOL* chg_allowed, BOOL* hchg_allowed, BOOL* temp_fault)
{
	static int pState = 0;
	int old_pState = pState;
	enum {
		PSTAT_DETECT = 0,
		PSTAT_LOW_STOP,
		PSTAT_NORMAL,
		PSTAT_SLOW,
		PSTAT_LIMITED,
		PSTAT_HIGH_STOP
	};

	
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
			
			break;
		case PSTAT_NORMAL:
			if (temp_01c < 0)
				pState = PSTAT_LOW_STOP;
			else if (600 < temp_01c)
				pState = PSTAT_HIGH_STOP;
			else if (480 < temp_01c) 
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
			else if (temp_01c < 420)
				pState = PSTAT_NORMAL;
			else if (temp_01c < 450)
				pState = PSTAT_SLOW;
			break;
		case PSTAT_HIGH_STOP:
			if (temp_01c < 420)
				pState = PSTAT_NORMAL;
			else if (temp_01c < 450)
				pState = PSTAT_SLOW;
			else if (temp_01c <= 570)
				pState = PSTAT_LIMITED;
			
			break;
	}
	if (old_pState != pState)
		pr_info("[BATT] Protect pState changed from %d to %d\n", old_pState, pState);

	
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
		case PSTAT_SLOW:
			*chg_allowed = TRUE;
			*hchg_allowed = TRUE;
			break;
#if 0 
		case PSTAT_SLOW:	
			if (4200 < vbat_mv)
				*chg_allowed = FALSE;
			else if (vbat_mv <= 4150)
				*chg_allowed = TRUE;
			*hchg_allowed = TRUE;
			break;
#endif
		case PSTAT_LIMITED:	
			if (PSTAT_LIMITED != old_pState)
				*chg_allowed = TRUE;
			if (4100 < vbat_mv)
				*chg_allowed = FALSE;
			else
				*chg_allowed = TRUE;
			*hchg_allowed = TRUE;
			break;
		case PSTAT_HIGH_STOP:
			*chg_allowed = FALSE;
			*hchg_allowed = FALSE;
			break;
	}

	
	if (PSTAT_NORMAL == pState || PSTAT_SLOW == pState)
		*temp_fault = FALSE;
	else
		*temp_fault = TRUE;

	return pState;
}

static struct htc_battery_platform_data htc_battery_pdev_data = {
	.gpio_mbat_in = CP3DUG_GPIO_MBAT_IN,
	.gpio_mbat_in_trigger_level = MBAT_IN_HIGH_TRIGGER,
	.mbat_in_keep_charging = 1,
	.mbat_in_unreg_rmt = 1,
	.chg_limit_active_mask = HTC_BATT_CHG_LIMIT_BIT_TALK,
	.func_show_batt_attr = htc_battery_show_attr,
	.func_show_htc_extension_attr = htc_battery_show_htc_extension_attr,
	.func_get_batt_rt_attr = htc_battery_get_rt_attr,
	.suspend_highfreq_check_reason = SUSPEND_HIGHFREQ_CHECK_BIT_TALK,
	.guage_driver = GAUGE_MAX17050,
	.charger = SWITCH_CHARGER_TPS65200,
	.m2a_cable_detect = 1,
	.enable_bootup_voltage = 3300,
#ifdef CONFIG_CPLD
	.cpld_hw_chg_led = CPLD_EXT_GPIO_HW_CHG_LED_OFF,
#endif
};

static struct platform_device htc_battery_pdev = {
	.name = "htc_battery",
	.id = -1,
	.dev	= {
		.platform_data = &htc_battery_pdev_data,
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



UINT32 m_parameter_TWS_SDI_1650mah[] = {
	
	10000, 4250, 8100, 4101, 5100, 3846,
	2000, 3711, 900, 3641, 0, 3411,
};
UINT32 m_parameter_Formosa_Sanyo_1650mah[] = {
	
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

static UINT16 m_maxim17050_param_id1_1860mah[] = {
	0x0001,
	0x420A, 0x2486,
	0x2946, 0x0030,
};

static UINT16 m_maxim17050_param_id2_1860mah[] = {
	0x0001,
	0x420A, 0x2486,
	0x2946, 0x0030,
};

static UINT16 m_maxim17050_param_id3_1800mah[] = {
	0x0001,
	0x2C02, 0x1280,
	0x2423, 0x0900,
};

static UINT16 m_maxim17050_param_id4_1860mah[] = {
	0x0001,
	0x2C00, 0x1400,
	0x4000, 0x0000,
};

static UINT16* m_maxim17050_qrtable_tbl[] = {
	m_maxim17050_param_id1_1860mah,
	m_maxim17050_param_id2_1860mah,
	m_maxim17050_param_id3_1800mah,
	m_maxim17050_param_id4_1860mah
};

static UINT32 capacity[] = {1860, 1860, 1860, 1800, 1860};
static UINT32 pd_m_coef[] = {22, 26, 26, 26};
static UINT32 pd_m_resl[] = {100, 220, 220, 220};
static UINT32 pd_t_coef[] = {100, 220, 220, 220};
static INT32 padc[] = {200, 200, 200, 200};
static INT32 pw[] = {5, 5, 5, 5};

static UINT32* pd_m_coef_tbl[] = {pd_m_coef,};
static UINT32* pd_m_resl_tbl[] = {pd_m_resl,};
static UINT32 capacity_deduction_tbl_01p[] = {0,};

static struct battery_parameter cp3dug_battery_parameter = {
	.fl_25 = NULL,
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
	.capacity_size = sizeof(capacity)/sizeof(UINT32),
	.m_qrtable_tbl = m_maxim17050_qrtable_tbl,
	.m_qrtable_tbl_size = sizeof(m_maxim17050_qrtable_tbl)/sizeof(UINT16*),
};

static max17050_platform_data max17050_pdev_data = {
	.func_get_thermal_id = get_thermal_id,
	.func_get_battery_id = get_battery_id,
	.func_poweralg_config_init = cp3dug_poweralg_config_init,
	.func_update_charging_protect_flag = cp3dug_update_charging_protect_flag,
	.r2_kohm = 0,	
	.batt_param = &cp3dug_battery_parameter,
#ifdef CONFIG_TPS65200
	.func_kick_charger_ic = tps65200_kick_charger_ic,
#endif
	.func_adjust_qrtable_by_temp = max17050_adjust_qrtable_by_temp,
};

static struct platform_device max17050_battery_pdev = {
	.name = "max17050-battery",
	.id = -1,
	.dev = {
		.platform_data = &max17050_pdev_data,
	},
};

static struct tps65200_platform_data tps65200_data = {
#ifdef CONFIG_CPLD
	.cpld_chg_int = CPLD_IRQ_CHARGE_INT,
	.cpld_chg_stat = CPLD_IRQ_CHARGE_STATE,
#endif
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
	if (!strcmp(str, "PASS")) {
		tps65200_data.dq_result = 1;
		dq_result = 1;
	} else {
		tps65200_data.dq_result = 0;
		dq_result = 0;
	}

	return 1;
}
__setup("androidboot.dq=", check_dq_setup);
#endif

static struct msm_hsusb_gadget_platform_data msm_gadget_pdata = {
	.is_phy_status_timer_on = 1,
	.prop_chg = 0,
};

#ifdef CONFIG_SERIAL_MSM_HS
static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
	.inject_rx_on_wakeup	= 1,
	.rx_to_inject		= 0xFD,
};
#endif

static struct msm_pm_platform_data msm7627a_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 16000,
					.residency = 20000,
	},
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 12000,
					.residency = 20000,
	},
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 0,
					.suspend_enabled = 1,
					.latency = 2000,
					.residency = 0,
	},
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 2,
					.residency = 0,
	},
};

static struct msm_pm_boot_platform_data msm_pm_boot_pdata __initdata = {
	.mode = MSM_PM_BOOT_CONFIG_RESET_VECTOR_PHYS,
	.p_addr = 0,
};

static struct msm_pm_platform_data
		msm8625_pm_data[MSM_PM_SLEEP_MODE_NR * CONFIG_NR_CPUS] = {
	
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

	
	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 500,
					.residency = 500,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 2,
					.residency = 10,
	},

	
	[MSM_PM_MODE(2, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 500,
					.residency = 500,
	},

	[MSM_PM_MODE(2, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 2,
					.residency = 10,
	},

	
	[MSM_PM_MODE(3, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 500,
					.residency = 500,
	},

	[MSM_PM_MODE(3, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
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

#ifdef CONFIG_CODEC_AIC3254
static int aic3254_lowlevel_init(void)
{
    return 0;
}

static struct i2c_board_info i2c_aic3254_devices[] = {
    {
        I2C_BOARD_INFO(AIC3254_I2C_NAME, \
                AIC3254_I2C_ADDR),
    },
};
#endif

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
	.request_region = request_fmem_c_region,
	.release_region = release_fmem_c_region,
	.reusable = 1,
};

static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
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



static struct fmem_platform_data fmem_pdata;

static struct platform_device fmem_device = {
	.name = "fmem",
	.id = 1,
	.dev = { .platform_data = &fmem_pdata },
};

struct regulator_consumer_supply ncp6335d_consumer_supplies[] = {
	REGULATOR_SUPPLY("ncp6335d", NULL),
	REGULATOR_SUPPLY("vddx_cx", NULL),
};

static struct regulator_init_data ncp6335d_init_data = {
	.constraints	= {
		.name		= "ncp6335d_sw",
		.min_uV		= 600000,
		.max_uV		= 1400000,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				REGULATOR_CHANGE_STATUS |
				REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_NORMAL |
				REGULATOR_MODE_FAST,
		.initial_mode	= REGULATOR_MODE_NORMAL,
		.always_on	= 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(ncp6335d_consumer_supplies),
	.consumer_supplies = ncp6335d_consumer_supplies,
};

static struct ncp6335d_platform_data ncp6335d_pdata = {
	.init_data = &ncp6335d_init_data,
	.default_vsel = NCP6335D_VSEL0,
	.slew_rate_ns = 333,
	.rearm_disable = 1,
};

static struct i2c_board_info i2c2_info[] __initdata = {
	{
		I2C_BOARD_INFO("ncp6335d", 0x38 >> 1),
		.platform_data = &ncp6335d_pdata,
	},
};
#ifdef CONFIG_LEDS_PM8029
static struct pm8029_led_config pm_led_config[] = {
        {
                .name = "button-backlight",
                .bank = PMIC8029_GPIO6,
                .init_pwm_brightness = 120,
                .flag = FIX_BRIGHTNESS,
        },
        {
                .name = "green",
                .bank = PMIC8029_GPIO5,
                .init_pwm_brightness = 255,
                .flag = FIX_BRIGHTNESS,
        },
        {
                .name = "amber",
                .bank = PMIC8029_GPIO8,
                .init_pwm_brightness = 240,
                .flag = FIX_BRIGHTNESS,
        }
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
#endif 


static struct htc_headset_gpio_platform_data htc_headset_gpio_data = {
	.hpin_gpio		= 0,
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

static struct htc_headset_pmic_platform_data htc_headset_pmic_data = {
	.driver_flag		= DRIVER_HS_PMIC_RPC_KEY,
	.hpin_gpio		= 0,
	.hpin_irq		= 0,
	.hpin_cpld		= CPLD_IRQ_HP,
	.key_gpio		= CP3DUG_AUD_REMO_PRESz,
	.key_irq		= 0,
	.key_enable_gpio	= 0,
	.adc_mic		= 0,
	.adc_remote		= {0, 2057, 2058, 5332, 5333, 12360},
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
	.tx_level_shift_en      = CP3DUG_AUD_UART_OEz,
	.uart_sw                = CP3DUG_AUD_UART_SEL,
	.remote_press	       = 0,
	.one_wire_remote        ={0x7E, 0x7F, 0x7D, 0x7F, 0x7B, 0x7F},
	.onewire_tty_dev	= "/dev/ttyMSM0",
};

static struct platform_device htc_headset_one_wire = {
       .name   = "HTC_HEADSET_1WIRE",
       .id     = -1,
       .dev    = {
               .platform_data  = &htc_headset_1wire_data,
       },
};

static struct platform_device *headset_devices[] = {
	&htc_headset_pmic,
	&htc_headset_gpio,
	&htc_headset_one_wire,
	
};

static struct headset_adc_config htc_headset_mgr_config[] = {
	{
		.type = HEADSET_MIC,
		.adc_max = 55723,
		.adc_min = 44510,
	},
	{
		.type = HEADSET_BEATS,
		.adc_max = 44509,
		.adc_min = 33333,
	},
	{
		.type = HEADSET_BEATS_SOLO,
		.adc_max = 33332,
		.adc_min = 20590,
	},
	{
		.type = HEADSET_NO_MIC,
		.adc_max = 20589,
		.adc_min = 0,
	},
};

static uint32_t headset_cpu_gpio[] = {
	GPIO_CFG(CP3DUG_AUD_UART_OEz, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_AUD_REMO_PRESz, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_AUD_UART_SEL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_AUD_UART_RX, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_AUD_UART_TX, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t headset_1wire_gpio[] = {
	GPIO_CFG(CP3DUG_AUD_UART_RX, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_AUD_UART_TX, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_AUD_UART_RX, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_AUD_UART_TX, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
 };

static void uart_tx_gpo(int mode)
{
	switch (mode) {
		case 0:
			gpio_tlmm_config(headset_1wire_gpio[1], GPIO_CFG_ENABLE);
			gpio_set_value_cansleep(CP3DUG_AUD_UART_TX, 0);
			pr_info("[HS_BOARD]UART TX GPO 0\n");
			break;
		case 1:
			gpio_tlmm_config(headset_1wire_gpio[1], GPIO_CFG_ENABLE);
			gpio_set_value_cansleep(CP3DUG_AUD_UART_TX, 1);
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
	gpio_set_value_cansleep(CP3DUG_AUD_UART_OEz, enable);
	pr_info("[HS_BOARD]level shift %d\n", enable);
}

static void headset_init(void)
{
	int i,ret,gpio_array;

	gpio_array = ARRAY_SIZE(headset_cpu_gpio);
	if (board_mfg_mode() != 0) {
		gpio_array -= 2;
	}

	for (i = 0; i < gpio_array; i++) {
		ret = gpio_tlmm_config(headset_cpu_gpio[i], GPIO_CFG_ENABLE);
		pr_info("[HS_BOARD]Config gpio[%d], ret = %d\n", (headset_cpu_gpio[i] & 0x3FF0) >> 4, ret);
	}
	gpio_set_value(CP3DUG_AUD_UART_OEz, 1); 
	gpio_set_value(CP3DUG_AUD_UART_SEL, 0);

}

static void headset_power(int hs_enable)
{
	int insert = 0;
	gpio_tlmm_config(headset_cpu_gpio[2], GPIO_CFG_ENABLE);
	pr_info("[HS_BOARD] AUD_UART_SEL Redefine_workaround\n");
	gpio_set_value(CP3DUG_AUD_UART_OEz, 1);
	insert = cpld_gpio_read(CPLD_EXT_GPIO_MIC_2V85_EN);
	pr_info("[HS_BOARD] pwr cpld_gpio_read %d\n", insert);
	if (insert < 0) {
		pr_info("Failed to read cpld gpio");
	}
	insert = cpld_gpio_write(CPLD_EXT_GPIO_MIC_2V85_EN, hs_enable);
	pr_info("[HS_BOARD]  pwr cpld_gpio_write result %d\n", insert);
	if (insert < 0) {
		pr_info("Failed to write cpld gpio");
		return;
	}
	insert = cpld_gpio_read(CPLD_EXT_GPIO_MIC_2V85_EN);
	pr_info("[HS_BOARD] pwr cpld_gpio_read %d\n", insert);
	if (insert < 0) {
		pr_info("Failed to read cpld gpio");
	}
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



static struct platform_device *common_devices[] __initdata = {
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
	&msm_device_adspdec,
	&msm_device_snd,
	&asoc_msm_pcm,
	&asoc_msm_dai0,
	&asoc_msm_dai1,
	&msm_adc_device,
	&htc_headset_mgr,
	&cable_detect_device,
	&fmem_device,
#ifdef CONFIG_LEDS_PM8029
        &pm8029_leds,
#endif 
#ifdef CONFIG_ION_MSM
	&ion_dev,
#endif
#ifdef CONFIG_PERFLOCK
	&msm8x25q_device_perf_lock,
#endif
	&htc_battery_pdev,
	&max17050_battery_pdev,
};

static struct platform_device *qrd7627a_devices[] __initdata = {
	&msm_device_dmov,
	&msm_device_smd,
	&msm_device_uart1,
	&msm_device_uart_dm1,
	&msm8625_gsbi0_qup_i2c_device,
	&msm8625_gsbi1_qup_i2c_device,
	&msm_device_otg,
	&msm_device_gadget_peripheral,
	&msm_kgsl_3d0,
};

#if 0
static struct platform_device *qrd3_devices[] __initdata = {
	&msm_device_nand,
};
#endif

static struct platform_device *msm8625_evb_devices[] __initdata = {
	&ram_console_device,
	&msm8625_device_dmov,
	&msm8625_device_smd,
	&msm8625_gsbi0_qup_i2c_device,
	&msm8625_gsbi1_qup_i2c_device,
	&msm8625_device_uart1,
#ifdef CONFIG_GPS_1530
	&msm8625_device_uart3,
#endif
	&msm8625_device_uart_dm1,
	&msm8625_device_uart_dm2,
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

static void fix_sizes(void)
{
	if (get_ddr_size() > SZ_512M)
		pmem_adsp_size = CAMERA_ZSL_SIZE;
#ifdef CONFIG_ION_MSM
	msm_ion_camera_size = pmem_adsp_size;
	msm_ion_audio_size = (MSM_PMEM_AUDIO_SIZE + PMEM_KERNEL_EBI1_SIZE);
	msm_ion_sf_size = pmem_mdp_size;
#endif
}

#ifdef CONFIG_ION_MSM
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct ion_co_heap_pdata co_ion_pdata = {
	.adjacent_mem_id = INVALID_HEAP_ID,
	.align = PAGE_SIZE,
};
#endif

struct ion_platform_heap msm7627a_heaps[] = {
		{
			.id	= ION_SYSTEM_HEAP_ID,
			.type	= ION_HEAP_TYPE_SYSTEM,
			.name	= ION_VMALLOC_HEAP_NAME,
		},
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
		
		{
			.id	= ION_CAMERA_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_CAMERA_HEAP_NAME,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *)&co_ion_pdata,
		},
		
		{
			.id	= ION_AUDIO_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_AUDIO_HEAP_NAME,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *)&co_ion_pdata,
		},
		
		{
			.id	= ION_SF_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_SF_HEAP_NAME,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *)&co_ion_pdata,
		},
#endif
};

static struct ion_platform_data ion_pdata = {
	.nr = MSM_ION_HEAP_NUM,
	.has_outer_cache = 1,
	.heaps = msm7627a_heaps,
};

static struct platform_device ion_dev = {
	.name = "ion-msm",
	.id = 1,
	.dev = { .platform_data = &ion_pdata },
};
#endif

static struct memtype_reserve msm7627a_reserve_table[] __initdata = {
	[MEMTYPE_SMI] = {
	},
	[MEMTYPE_EBI0] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
	[MEMTYPE_EBI1] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
};

#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct android_pmem_platform_data *pmem_pdata_array[] __initdata = {
		&android_pmem_adsp_pdata,
		&android_pmem_audio_pdata,
		&android_pmem_pdata,
};
#endif
#endif

static void __init size_pmem_devices(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	unsigned int i;
	unsigned int reusable_count = 0;

	android_pmem_adsp_pdata.size = pmem_adsp_size;
	android_pmem_pdata.size = pmem_mdp_size;
	android_pmem_audio_pdata.size = pmem_audio_size;

	fmem_pdata.size = 0;
	fmem_pdata.align = PAGE_SIZE;

	for (i = 0; i < ARRAY_SIZE(pmem_pdata_array); ++i) {
		struct android_pmem_platform_data *pdata = pmem_pdata_array[i];

		if (!reusable_count && pdata->reusable)
			fmem_pdata.size += pdata->size;

		reusable_count += (pdata->reusable) ? 1 : 0;

		if (pdata->reusable && reusable_count > 1) {
			pr_err("%s: Too many PMEM devices specified as reusable. PMEM device %s was not configured as reusable.\n",
				__func__, pdata->name);
			pdata->reusable = 0;
		}
	}
#endif
#endif
}

#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static void __init reserve_memory_for(struct android_pmem_platform_data *p)
{
	msm7627a_reserve_table[p->memory_type].size += p->size;
}
#endif
#endif

static void __init reserve_pmem_memory(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(pmem_pdata_array); ++i)
		reserve_memory_for(pmem_pdata_array[i]);

	msm7627a_reserve_table[MEMTYPE_EBI1].size += pmem_kernel_ebi1_size;
#endif
#endif
}

static void __init size_ion_devices(void)
{
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	ion_pdata.heaps[1].size = msm_ion_camera_size;
	ion_pdata.heaps[2].size = msm_ion_audio_size;
	ion_pdata.heaps[3].size = msm_ion_sf_size;
#endif
}

static void __init reserve_ion_memory(void)
{
#if defined(CONFIG_ION_MSM) && defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	msm7627a_reserve_table[MEMTYPE_EBI1].size += msm_ion_camera_size;
	msm7627a_reserve_table[MEMTYPE_EBI1].size += msm_ion_audio_size;
	msm7627a_reserve_table[MEMTYPE_EBI1].size += msm_ion_sf_size;
#endif
}

static void __init msm7627a_calculate_reserve_sizes(void)
{
	fix_sizes();
	size_pmem_devices();
	reserve_pmem_memory();
	size_ion_devices();
	reserve_ion_memory();
}

static int msm7627a_paddr_to_memtype(unsigned int paddr)
{
	return MEMTYPE_EBI1;
}

static struct reserve_info msm7627a_reserve_info __initdata = {
	.memtype_reserve_table = msm7627a_reserve_table,
	.calculate_reserve_sizes = msm7627a_calculate_reserve_sizes,
	.paddr_to_memtype = msm7627a_paddr_to_memtype,
};

static void __init msm7627a_reserve(void)
{
	reserve_info = &msm7627a_reserve_info;
	msm_reserve();
}

static void __init msm8625_reserve(void)
{
	memblock_remove(MSM8625_NON_CACHE_MEM, SZ_2K);
	memblock_remove(MSM8625_CPU_PHYS, SZ_8);
	memblock_remove(MSM8625_WARM_BOOT_PHYS, SZ_32);
	msm7627a_reserve();
}

static void msmqrd_adsp_add_pdev(void)
{
	int rc = 0;
	struct rpc_board_dev *rpc_adsp_pdev;

	rpc_adsp_pdev = kzalloc(sizeof(struct rpc_board_dev), GFP_KERNEL);
	if (rpc_adsp_pdev == NULL) {
		pr_err("%s: Memory Allocation failure\n", __func__);
		return;
	}
	rpc_adsp_pdev->prog = ADSP_RPC_PROG;

	if (cpu_is_msm8625() || cpu_is_msm8625q())
		rpc_adsp_pdev->pdev = msm8625_device_adsp;
	else
		rpc_adsp_pdev->pdev = msm_adsp_device;
	rc = msm_rpc_add_board_dev(rpc_adsp_pdev, 1);
	if (rc < 0) {
		pr_err("%s: return val: %d\n",	__func__, rc);
		kfree(rpc_adsp_pdev);
	}
}

static void __init msm7627a_device_i2c_init(void)
{
	msm_gsbi0_qup_i2c_device.dev.platform_data = &msm_gsbi0_qup_i2c_pdata;
	msm_gsbi1_qup_i2c_device.dev.platform_data = &msm_gsbi1_qup_i2c_pdata;
}

static void __init msm8625_device_i2c_init(void)
{
	int i, rc;

	msm8625_gsbi0_qup_i2c_device.dev.platform_data
					= &msm_gsbi0_qup_i2c_pdata;
	msm8625_gsbi1_qup_i2c_device.dev.platform_data
					= &msm_gsbi1_qup_i2c_pdata;
	if (0 || cpu_is_msm8625q()) {
		for (i = 0 ; i < ARRAY_SIZE(msm8625q_i2c_gpio_config); i++) {
			rc = gpio_tlmm_config(
					msm8625q_i2c_gpio_config[i].gpio_cfg,
					GPIO_CFG_ENABLE);
			if (rc)
				pr_err("I2C-gpio tlmm config failed\n");
		}
		rc = platform_device_register(&msm8625q_i2c_gpio);
		if (rc)
			pr_err("%s: could not register i2c-gpio device: %d\n",
						__func__, rc);
	}

#ifdef CONFIG_I2C_CPLD
		for (i = 0 ; i < ARRAY_SIZE(msm8625q_i2c_cpld_config); i++) {
			rc = gpio_tlmm_config(
					msm8625q_i2c_cpld_config[i].gpio_cfg,
					GPIO_CFG_ENABLE);
			if (rc)
				pr_err("I2C-cpld tlmm config failed\n");
		}
		rc = platform_device_register(&msm8625q_i2c_cpld);
		if (rc)
			pr_err("%s: could not register i2c-cpld device: %d\n",
						__func__, rc);
#endif
}

#ifdef CONFIG_FLASHLIGHT_TPS61310
#ifdef CONFIG_MSM_CAMERA_FLASH
int flashlight_control(int mode)
{
	int	rc;
	
	static int	backlight_off = 0;

	if (mode != FL_MODE_PRE_FLASH && mode != FL_MODE_OFF) {
		led_brightness_switch("lcd-backlight", FALSE);
		backlight_off = 1;
	}

	rc = tps61310_flashlight_control(mode);

	if(mode == FL_MODE_PRE_FLASH || mode == FL_MODE_OFF) {
		if(backlight_off) {
			led_brightness_switch("lcd-backlight", TRUE);
			backlight_off = 0;
		}
	}

	return rc;
}
#endif

static struct TPS61310_flashlight_platform_data tps61310_pdata = {
        .tps61310_strb0 = CPLD_EXT_GPIO_FLASH_EN,
        .tps61310_strb1 = CPLD_EXT_GPIO_TORCH_FLASH,
        .led_count = 1,
        .flash_duration_ms = 600,
	.cpld = 1,
	.mode_pin_suspend_state_low = 1,
};
static struct i2c_board_info tps61310_i2c_info[] = {
        {
                I2C_BOARD_INFO("TPS61310_FLASHLIGHT", 0x66 >> 1),
                .platform_data = &tps61310_pdata,
        },
};
#endif

#if defined(CONFIG_INPUT_CAPELLA_CM3629)
#define CP3DUG_GPIO_PROXIMITY_INT       (36)
static uint8_t cm3629_mapping_table[] = {0x0, 0x3, 0x6, 0x9, 0xC,
                        0xF, 0x12, 0x15, 0x18, 0x1B,
                        0x1E, 0x21, 0x24, 0x27, 0x2A,
                        0x2D, 0x30, 0x33, 0x36, 0x39,
                        0x3C, 0x3F, 0x43, 0x47, 0x4B,
                        0x4F, 0x53, 0x57, 0x5B, 0x5F,
                        0x63, 0x67, 0x6B, 0x70, 0x75,
                        0x7A, 0x7F, 0x84, 0x89, 0x8E,
                        0x93, 0x98, 0x9D, 0xA2, 0xA8,
                        0xAE, 0xB4, 0xBA, 0xC0, 0xC6,
                        0xCC, 0xD3, 0xDA, 0xE1, 0xE8,
                        0xEF, 0xF6, 0xFF};

static struct cm3629_platform_data cm36282_pdata = {
        .model = CAPELLA_CM36282,
        .ps_select = CM3629_PS1_ONLY,
        .intr = CP3DUG_GPIO_PROXIMITY_INT,
        .levels = {4, 13, 18, 147, 274, 1556, 2160, 4178, 7430, 65535},
        .golden_adc = 2196,
        .power = NULL,
        .cm3629_slave_address = 0xC0>>1,
        .ps1_thd_set = 0x07,
        .ps1_thd_no_cal = 0xF1,
        .ps1_thd_with_cal = 0x07,
        .ps_calibration_rule = 1,
        .ps_conf1_val = CM3629_PS_DR_1_320 | CM3629_PS_IT_1_6T |
                        CM3629_PS1_PERS_3,
        .ps_conf2_val = CM3629_PS_ITB_1 | CM3629_PS_ITR_1 |
                        CM3629_PS2_INT_DIS | CM3629_PS1_INT_DIS,
        .ps_conf3_val = CM3629_PS2_PROL_32,
        .dynamical_threshold = 1,
        .mapping_table = cm3629_mapping_table,
        .mapping_size = ARRAY_SIZE(cm3629_mapping_table),
};

static struct i2c_board_info i2c_cm36282_devices[] = {
        {
                I2C_BOARD_INFO(CM3629_I2C_NAME,0xC0 >> 1),
        .platform_data = &cm36282_pdata,
        .irq = MSM_GPIO_TO_INT(CP3DUG_GPIO_PROXIMITY_INT),
        },
};

#endif 


static struct platform_device msm_proccomm_regulator_dev = {
	.name   = PROCCOMM_REGULATOR_DEV_NAME,
	.id     = -1,
	.dev    = {
		.platform_data = &msm7x27a_proccomm_regulator_data
	}
};

static void __init msm7627a_init_regulators(void)
{
	int rc = platform_device_register(&msm_proccomm_regulator_dev);
	if (rc)
		pr_err("%s: could not register regulator device: %d\n",
				__func__, rc);
}

static int __init msm_qrd_init_ar6000pm(void)
{
	msm_wlan_ar6000_pm_device.dev.platform_data = &ar600x_wlan_power;
	return platform_device_register(&msm_wlan_ar6000_pm_device);
}

static void __init msm_add_footswitch_devices(void)
{
	platform_add_devices(msm_footswitch_devices,
				msm_num_footswitch_devices);
}

static void __init add_platform_devices(void)
{
	if (machine_is_msm8625_evb() || machine_is_msm8625_qrd7()
				|| machine_is_msm8625_evt()
				|| machine_is_msm8625q_evbd()
				|| machine_is_msm8625q_skud()
				|| 0
				|| 0
				|| 0
				|| machine_is_cp3dug()
	) {
		platform_add_devices(msm8625_evb_devices,
				ARRAY_SIZE(msm8625_evb_devices));
#if 0
		platform_add_devices(qrd3_devices,
				ARRAY_SIZE(qrd3_devices));
#endif
	} else {
		platform_add_devices(qrd7627a_devices,
				ARRAY_SIZE(qrd7627a_devices));
	}

#if 0
	if (machine_is_msm7627a_qrd3() || machine_is_msm7627a_evb())
		platform_add_devices(qrd3_devices,
				ARRAY_SIZE(qrd3_devices));
#endif
	platform_add_devices(common_devices,
			ARRAY_SIZE(common_devices));
}

#define UART1DM_RX_GPIO		45
static void __init qrd7627a_uart1dm_config(void)
{
	msm_uart_dm1_pdata.wakeup_irq = gpio_to_irq(UART1DM_RX_GPIO);
	if (cpu_is_msm8625() || cpu_is_msm8625q()){
		msm8625_device_uart_dm1.dev.platform_data =
			&msm_uart_dm1_pdata;
    msm8625_device_uart_dm1.name = "msm_serial_hs_qct";
	}
	else{
		msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
		msm_device_uart_dm1.name = "msm_serial_hs_qct";
	}

}

static void __init qrd7627a_otg_gadget(void)
{
	if (cpu_is_msm8625() || cpu_is_msm8625q()) {
		msm_otg_pdata.swfi_latency = msm8625_pm_data
		[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].latency;
		msm8625_device_otg.dev.platform_data = &msm_otg_pdata;
		msm8625_device_gadget_peripheral.dev.platform_data =
					&msm_gadget_pdata;

	} else {
	msm_otg_pdata.swfi_latency = msm7627a_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
		msm_device_otg.dev.platform_data = &msm_otg_pdata;
		msm_device_gadget_peripheral.dev.platform_data =
					&msm_gadget_pdata;
	}
}

static void __init msm_pm_init(void)
{

	if (!cpu_is_msm8625() && !cpu_is_msm8625q()) {
		msm_pm_set_platform_data(msm7627a_pm_data,
				ARRAY_SIZE(msm7627a_pm_data));
		BUG_ON(msm_pm_boot_init(&msm_pm_boot_pdata));
	} else {
		msm_pm_set_platform_data(msm8625_pm_data,
				ARRAY_SIZE(msm8625_pm_data));
		BUG_ON(msm_pm_boot_init(&msm_pm_8625_boot_pdata));
		msm8x25_spm_device_init();
		msm_pm_register_cpr_ops();
	}
}

static void cp3_reset(void)
{
	gpio_set_value(CP3DUG_GPIO_PS_HOLD, 0);
}

#ifdef CONFIG_GPS_1530
#define UART3_RTS       GPIO_CFG(84, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)
#define UART3_CTS       GPIO_CFG(85, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)
#define UART3_RX        GPIO_CFG(86, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA)
#define UART3_TX        GPIO_CFG(87, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA)
#define UART3_RTS_SLEEP GPIO_CFG(84, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA)
#define UART3_CTS_SLEEP GPIO_CFG(85, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)
#define UART3_RX_SLEEP  GPIO_CFG(86, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)
#define UART3_TX_SLEEP  GPIO_CFG(87, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA)

void msm8625Q_init_uart3(void)
{
        gpio_tlmm_config(UART3_RX, GPIO_CFG_ENABLE);
        gpio_tlmm_config(UART3_TX, GPIO_CFG_ENABLE);
        gpio_tlmm_config(UART3_RTS, GPIO_CFG_ENABLE);
        gpio_tlmm_config(UART3_CTS, GPIO_CFG_ENABLE);
}

struct gps_vreg_info {
        const char *vreg_id;
        unsigned int level_min;
        unsigned int level_max;
        unsigned int pmapp_id;
        unsigned int is_vreg_pin_controlled;
        struct regulator *reg;
};

static struct gps_vreg_info vreg_info[] = {
        {"rftx",     2700000, 2700000, 22, 1, NULL}
};

int gps_1530_lna_power_enable(int on)
{
        struct regulator_bulk_data regs[ARRAY_SIZE(vreg_info)];
        int i = 0, rc = 0, index;

	printk(KERN_INFO "gps_1530_lna_power_enable");

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

        for ( index=0 ; index < ARRAY_SIZE(vreg_info); index++) {
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
		}else {

                        rc = regulator_disable(vreg_info[index].reg);
                        if (rc) {
                                pr_err("%s:%s vreg disable failed %d\n",
                                        __func__,
                                        vreg_info[index].vreg_id, rc);
                                goto reg_disable;
                        }
		}
	}

	return rc;
reg_disable:
        while (index) {
	        index--;
                regulator_disable(vreg_info[index].reg);
                regulator_put(vreg_info[index].reg);
        }
out:
        return rc;
}
EXPORT_SYMBOL(gps_1530_lna_power_enable);

void gps_1530_1v8_on(void)
{
        int ret;

       	cpld_gpio_config(CPLD_EXT_GPIO_GPS_1V8_EN,  CPLD_GPIO_OUT);
       	ret = cpld_gpio_write(CPLD_EXT_GPIO_GPS_1V8_EN, 1);
       	if(ret < 0)  printk(KERN_ERR "write error GPS_1V8_EN");
}
EXPORT_SYMBOL(gps_1530_1v8_on);

void gps_1530_uart_on(int on)
{
        if (on)
        {
                gpio_tlmm_config(UART3_RX, GPIO_CFG_ENABLE);
                gpio_tlmm_config(UART3_TX, GPIO_CFG_ENABLE);
                gpio_tlmm_config(UART3_RTS, GPIO_CFG_ENABLE);
                gpio_tlmm_config(UART3_CTS, GPIO_CFG_ENABLE);

        }else {
                gpio_tlmm_config(UART3_RX_SLEEP, GPIO_CFG_DISABLE);
                gpio_tlmm_config(UART3_TX_SLEEP, GPIO_CFG_DISABLE);
                gpio_tlmm_config(UART3_RTS_SLEEP, GPIO_CFG_DISABLE);
                gpio_tlmm_config(UART3_CTS_SLEEP, GPIO_CFG_DISABLE);
        }
}
EXPORT_SYMBOL(gps_1530_uart_on);
#endif

static void __init msm_cp3dug_init(void)
{
#ifdef CONFIG_MMC_MSM
	struct proc_dir_entry *entry = NULL;
#endif

	msm_hw_reset_hook = cp3_reset;
	gpio_set_value(CP3DUG_GPIO_PS_HOLD, 1);

	msm7x2x_misc_init();
	msm7627a_init_regulators();
	msmqrd_adsp_add_pdev();

	if (cpu_is_msm8625() || cpu_is_msm8625q())
		msm8625_device_i2c_init();
	else
		msm7627a_device_i2c_init();

#if defined(CONFIG_I2C_CPLD)
        i2c_register_board_info(3,
                                i2c_cpld_devices,
                                ARRAY_SIZE(i2c_cpld_devices));
#endif
	
	qrd7627a_uart1dm_config();
	
	qrd7627a_otg_gadget();

	msm_add_footswitch_devices();
	if (get_kernel_flag() & BIT(1) || (board_mfg_mode() != 0)) {
		printk("[HS_BOARD]Debug UART enabled, remove 1wire driver\n");
		htc_headset_mgr_data.headset_devices_num--;
		htc_headset_mgr_data.headset_devices[2] = NULL;
	}
	add_platform_devices();

	
	msm_qrd_init_ar6000pm();
	cp3_add_usb_devices();
	cp3_wifi_init();

#ifdef CONFIG_MMC_MSM
	printk(KERN_ERR "%s: start init mmc\n", __func__);
	cp3dug_init_mmc();
	printk(KERN_ERR "%s: msm7627a_init_mmc()\n", __func__);
	entry = create_proc_read_entry("emmc", 0, NULL, emmc_partition_read_proc, NULL);
	printk(KERN_ERR "%s: create_proc_read_entry()\n", __func__);
	if (!entry)
		printk(KERN_ERR"Create /proc/emmc failed!\n");

	entry = create_proc_read_entry("dying_processes", 0, NULL, dying_processors_read_proc, NULL);
	if (!entry)
		printk(KERN_ERR"Create /proc/dying_processes FAILED!\n");

#endif

#ifdef CONFIG_SDIO_TTY_SP6502COM
	sprd_init();
#endif

#ifdef CONFIG_USB_EHCI_MSM_72K
	msm7627a_init_host();
#endif
	msm_pm_init();

	msm_pm_register_irqs();
	
	cp3dcg_init_panel();
	#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_3K
	syn_init_vkeys_cp3();
	#endif

	if (0 || machine_is_msm8625q_evbd()
					|| machine_is_msm8625q_skud()
					|| 0
					|| 0
					|| machine_is_cp3dug()
	)
		i2c_register_board_info(2, i2c2_info,
				ARRAY_SIZE(i2c2_info));

#if 0
#if defined(CONFIG_I2C_CPLD)
        i2c_register_board_info(3,
				i2c_cpld_devices,
                             	ARRAY_SIZE(i2c_cpld_devices));
#endif
#endif


#if defined(CONFIG_BT) && defined(CONFIG_MARIMBA_CORE)
	bt_export_bd_address();
	msm7627a_bt_power_init();
#endif

#ifdef CONFIG_PERFLOCK_SCREEN_POLICY
	
	perf_lock_init(&screen_off_ceiling_lock, TYPE_CPUFREQ_CEILING,
					PERF_LOCK_HIGH, "screen_off_scaling_max");
	perflock_screen_policy_init(&msm8x25q_screen_off_policy);
#endif

	#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_3K
	
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				i2c_touch_device,
				ARRAY_SIZE(i2c_touch_device));
	#endif

#ifdef CONFIG_FLASHLIGHT_TPS61310
        i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
                                tps61310_i2c_info,
                                ARRAY_SIZE(tps61310_i2c_info));
#endif
	
	printk(KERN_ERR"NFC i2c registration\n");
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
		pn544_i2c_boardinfo, ARRAY_SIZE(pn544_i2c_boardinfo));

	i2c_register_board_info(1,         
		msm_i2c_gsbi1_rt5501_info,
		ARRAY_SIZE(msm_i2c_gsbi1_rt5501_info));

	i2c_register_board_info(1,         
		msm_i2c_gsbi1_tfa9887_info,
		ARRAY_SIZE(msm_i2c_gsbi1_tfa9887_info));

#ifdef CONFIG_CODEC_AIC3254
	aic3254_lowlevel_init();
	i2c_register_board_info(1,
		i2c_aic3254_devices, ARRAY_SIZE(i2c_aic3254_devices));
#endif

	
	if(htc_get_board_revision()){
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_bma250_devices_evt, ARRAY_SIZE(i2c_bma250_devices_evt));
	}
	else{
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_bma250_devices, ARRAY_SIZE(i2c_bma250_devices));
	}
        
        i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
                i2c_cm36282_devices, ARRAY_SIZE(i2c_cm36282_devices));
	cp3dug_camera_init();
	
	msm7x25a_kgsl_3d0_init();
	msm8x25_kgsl_3d0_init();
#ifdef CONFIG_MSM_RPC_VIBRATOR
	msm_init_pmic_vibrator(3000);
#endif


	
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			i2c_tps65200_devices, ARRAY_SIZE(i2c_tps65200_devices));

	msm8625Q_init_keypad();

	
	if (~get_kernel_flag() & KERNEL_FLAG_TEST_PWR_SUPPLY) {
		htc_monitor_init();
		htc_PM_monitor_init();
	}

#ifdef CONFIG_GPS_1530
	msm8625Q_init_uart3();
	gps_1530_lna_power_enable(1);
#endif
}

static unsigned int radio_security = 0;

static void __init cp3dug_fixup(struct tag *tags, char **cmdline, struct meminfo *mi)
{
    radio_security = parse_tag_security((const struct tag *)tags);
    printk(KERN_INFO "%s: security_atag=0x%x\n", __func__, radio_security);

    mi->nr_banks = 2;
    mi->bank[0].start = 0x03B00000;
    mi->bank[0].size = 0x0C500000;
    mi->bank[1].start = 0x10000000;

    if(radio_security && 0x1){
      
      mi->bank[1].size = 0x2E600000;
    }
    else{
      mi->bank[1].size = 0x2FA00000;
    }
}

static void __init qrd7627a_init_early(void)
{
	
}

MACHINE_START(CP3DUG, "cp3dug")
        .atag_offset    = PHYS_OFFSET + 0x100,
        .fixup          = cp3dug_fixup,
        .map_io         = msm8625_map_io,
        .reserve        = msm8625_reserve,
        .init_irq       = msm8625_init_irq,
        .init_machine   = msm_cp3dug_init,
        .timer          = &msm_timer,
        .init_early     = qrd7627a_init_early,
        .handle_irq     = gic_handle_irq,
MACHINE_END
