/* arch/arm/mach-msm/board-primods-keypad.c
 * Copyright (C) 2010 HTC Corporation.
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

#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/gpio_event.h>
#include <linux/gpio.h>
#include <linux/keyreset.h>
#include <asm/mach-types.h>

#include <mach/board_htc.h>

#include "board-cp3dug.h"
#include <linux/module.h>
#include <mach/proc_comm.h>


static char *keycaps = "--qwerty";
#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "board_cp3dug."

module_param_named(keycaps, keycaps, charp, 0);

static struct gpio_event_direct_entry msm8625q_keypad_nav_map[] = {
	{
		.gpio = CP3DUG_POWER_KEY,
		.code = KEY_POWER,
	},
	{
		.gpio = CPLD_GPIO_PM_TO_SYS(CP3_VOL_UP),
		.code = KEY_VOLUMEUP,
	},
	{
		.gpio = CPLD_GPIO_PM_TO_SYS(CP3_VOL_DN),
		.code = KEY_VOLUMEDOWN,
	},
};

static void msm8625q_direct_inputs_gpio(void)
{
	static uint32_t matirx_inputs_gpio_table[] = {
		GPIO_CFG(CP3DUG_POWER_KEY, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,	GPIO_CFG_4MA),
	};
	gpio_tlmm_config(matirx_inputs_gpio_table[0], GPIO_CFG_ENABLE);
}

static struct gpio_event_input_info msm8625q_keypad_power_info = {
	.info.func = gpio_event_input_func,
	.flags = GPIOEDF_PRINT_KEYS,
	.type = EV_KEY,
#if BITS_PER_LONG != 64 && !defined(CONFIG_KTIME_SCALAR)
	.debounce_time.tv.nsec = 20 * NSEC_PER_MSEC,
#else
	.debounce_time.tv64 = 20 * NSEC_PER_MSEC,
#endif
	.keymap = msm8625q_keypad_nav_map,
	.keymap_size = ARRAY_SIZE(msm8625q_keypad_nav_map),
	.setup_input_gpio = msm8625q_direct_inputs_gpio,
};

static struct gpio_event_info *msm8625q_keypad_info[] = {
	&msm8625q_keypad_power_info.info,
};

static struct gpio_event_platform_data msm8625q_keypad_data = {
	.name = "device-keypad",
	.info = msm8625q_keypad_info,
	.info_count = ARRAY_SIZE(msm8625q_keypad_info),
};

static struct platform_device msm8625q_keypad_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev		= {
		.platform_data	= &msm8625q_keypad_data,
	},
};

static struct keyreset_platform_data msm8625q_reset_keys_pdata = {
	.keys_down = {
		KEY_POWER,
		KEY_VOLUMEDOWN,
		KEY_VOLUMEUP,
		0
	},
};

static struct platform_device msm8625q_reset_keys_device = {
	.name = KEYRESET_NAME,
	.dev.platform_data = &msm8625q_reset_keys_pdata,
};

int __init msm8625Q_init_keypad(void)
{
	if (platform_device_register(&msm8625q_reset_keys_device))
		printk(KERN_WARNING "%s: register reset key fail\n", __func__);

	return platform_device_register(&msm8625q_keypad_device);
}

