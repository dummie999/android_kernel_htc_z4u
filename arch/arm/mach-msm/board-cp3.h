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
#ifndef __ARCH_ARM_MACH_MSM_BOARD_CP3__
#define __ARCH_ARM_MACH_MSM_BOARD_CP3__

#include "board-msm7627a.h"
#include <linux/i2c/cpld.h>

void __init msm7627a_init_mmc(void);

#define MSM_RAM_CONSOLE_BASE    0x03A00000 /* MSM_HTC_RAM_CONSOLE_PHYS must be the same */
#define MSM_RAM_CONSOLE_SIZE    MSM_HTC_RAM_CONSOLE_SIZE

#define MSM_FB_BASE             0x3FA00000
#define MSM_FB_SIZE             0x00600000
#define MSM_PMEM_MDP_SIZE       0x2F00000
#define MSM_PMEM_ADSP_SIZE      0x5000000
#define MSM_PMEM_ADSP2_SIZE     0x2C0000
#define PMEM_KERNEL_EBI1_SIZE	0x3A000
#define MSM_PMEM_AUDIO_SIZE		0x1F4000

#define CP3_POWER_KEY           (37)

#define CPLD_GPIO_BASE						(NR_MSM_GPIOS+0x100)
#define CPLD_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio + CPLD_GPIO_BASE)
#define CPLD_GPIO_SYS_TO_PM(sys_gpio)	(sys_gpio - CPLD_GPIO_BASE)

#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_3K
#define MSM_TP_ATTz					(18)
#define MSM_V_TP_3V3_EN				(0)
#define MSM_TP_RSTz					(0)
#endif

#define CPLDGPIO(x) (x)
#define CP3_VOL_UP						CPLD_EXT_GPIO_KEY_UP_INPUT_LEVEL
#define CP3_VOL_DN						CPLD_EXT_GPIO_KEY_DW_INPUT_LEVEL

int __init cp3_wifi_init(void);

int  msm8625Q_init_keypad(void);

extern int panel_type;
#endif
