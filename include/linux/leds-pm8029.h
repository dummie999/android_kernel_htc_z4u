/* include/linux/leds-pm8029.h
 *
 * Copyright (C) 2009 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_LEDS_PM8029_H
#define _LINUX_LEDS_PM8029_H

#include <linux/leds.h>
#include <linux/workqueue.h>
#include <linux/android_alarm.h>

enum pmic8029_leds {
	PMIC8029_DRV1 = 0,
	PMIC8029_DRV2,
	PMIC8029_DRV3,
	PMIC8029_DRV4,
	PMIC8029_GPIO1,
	PMIC8029_GPIO5,
	PMIC8029_GPIO6,
	PMIC8029_GPIO8,
	PMIC8029_GPIO2,
};
#define BLINK_DISABLE		0
#define BLINK_64MS_PER_2S	1
#define BLINK_1S_PER_2S		2

#define ALWAYS_ON		0
#define DUTY_64MS		0xF8
#define DUTY_1S			0x80
#define TURN_OFF 		0xFF

#define FIX_BRIGHTNESS		(1 << 0)
#define DYNAMIC_BRIGHTNESS	(1 << 1)
struct pm8029_led_config {
	const char *name;
	uint32_t bank;
	uint32_t init_pwm_brightness;
	uint32_t out_current;
	uint32_t flag;
};

struct pm8029_led_platform_data {
	struct pm8029_led_config *led_config;
	uint32_t num_leds;
};

struct pm8029_led_data {
	struct led_classdev ldev;
	struct pm8029_led_config *led_config;
	struct delayed_work blink_work;
	struct work_struct off_timer_work;
	struct alarm off_timer_alarm;
	atomic_t brightness;
	atomic_t blink;
	atomic_t off_timer;
	uint8_t bank;
	uint8_t init_pwm_brightness;
	uint8_t out_current;
	uint32_t flag;
};

#endif 
