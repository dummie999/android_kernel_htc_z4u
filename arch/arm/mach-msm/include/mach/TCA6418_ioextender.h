/* include/asm/mach-msm/TCA6418_ioextender.h
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


#ifndef _LINUX_ATMEGA_MICROP_H
#define _LINUX_ATMEGA_MICROP_H

#include <linux/leds.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/wakelock.h>
#include <linux/input.h>
#include <linux/list.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>


#define IOEXTENDER_I2C_NAME "TI-IOExtender"

#define IOEXTENDER_I2C_RCMD_VERSION		0x00

#define IOEXTENDER_I2C_GPO_DATA_OUT		0x23
#define IOEXTENDER_I2C_GPIO_DATA_OUT_L 0x23;
#define IOEXTENDER_I2C_GPIO_DATA_OUT_H 0x24;

/* IO expander chip TCA6418 */
#define TCA6418E_Device               0x68
#define TCA6418E_Reg_GPIO_DAT_STAT1  0x14  //(H)GPIO0~GPIO7(L)
#define TCA6418E_Reg_GPIO_DAT_STAT2  0x15  //(H)GPIO15~GPIO8(L)
#define TCA6418E_Reg_GPIO_DAT_STAT3  0x16  //(H)GPIO17~GPIO16(L)
#define TCA6418E_Reg_GPIO_DAT_OUT1    0x17  //(H)GPIO0~GPIO7(L)
#define TCA6418E_Reg_GPIO_DAT_OUT2    0x18  //(H)GPIO15~GPIO8(L)
#define TCA6418E_Reg_GPIO_DAT_OUT3    0x19  //(H)GPIO17~GPIO16(L)
#define TCA6418E_Reg_GPIO_DIR1        0x23  //(H)GPIO0~GPIO7(L)      0/1: input/output
#define TCA6418E_Reg_GPIO_DIR2        0x24  //(H)GPIO15~GPIO8(L)     0/1: input/output
#define TCA6418E_Reg_GPIO_DIR3        0x25  //(H)GPIO17~GPIO16(L)    0/1: input/output
#define TCA6418E_GPIO_NUM             18 
//---------------------//

struct ioext_i2c_platform_data {
	struct platform_device *ioext_devices;
	int			num_devices;
	uint32_t		gpio_reset;
	void 			*dev_id;
	void (*setup_gpio)(void);
	void (*reset_chip)(void);
};

struct ioext_i2c_client_data {
	struct mutex ioext_i2c_rw_mutex;
	struct mutex ioext_set_gpio_mutex;
	uint16_t version;
	struct early_suspend early_suspend;

	atomic_t ioext_is_suspend;
};

struct ioext_ops {
	int (*init_ioext_func)(struct i2c_client *);
};

int ioext_i2c_read(uint8_t addr, uint8_t *data, int length);
int ioext_i2c_write(uint8_t addr, uint8_t *data, int length);
int ioext_gpio_set_value(uint8_t gpio, uint8_t value);
int ioext_gpio_get_value(uint8_t gpio);
int ioext_gpio_get_direction(uint8_t gpio);
int ioext_read_gpio_status(uint8_t *data);
void ioext_print_gpio_status(void);
void ioext_register_ops(struct ioext_ops *ops);

#endif /* _LINUX_ATMEGA_MICROP_H */
