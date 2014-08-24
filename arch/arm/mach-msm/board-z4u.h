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
#ifndef __ARCH_ARM_MACH_MSM_BOARD_Z4U__
#define __ARCH_ARM_MACH_MSM_BOARD_Z4U__

#include "board-cp3.h"

#define Z4U_GPIO_BATID_USBID_SEL		(14)
#define Z4U_GPIO_PS_HOLD         		(25)
/* Compass  */
#define Z4U_GPIO_GSENSORS_INT         (83)
#define Z4U_LAYOUTS_EVM               { \
                { {  0,  1, 0}, { -1,  0, 0}, {0, 0,  1} }, \
                { { -1,  0, 0}, {  0, -1, 0}, {0, 0, -1} }, \
                { { -1,  0, 0}, {  0, -1, 0}, {0, 0,  1} }, \
                { {  0,  1, 0}, {  0,  0,-1}, {1, 0,  0} }  \
                                        }
#define Z4U_LAYOUTS_EVT               { \
                { {  0,  1, 0}, { -1,  0, 0}, {0, 0,  1} }, \
                { {  0, -1, 0}, { -1,  0, 0}, {0, 0,  1} }, \
                { { -1,  0, 0}, {  0, -1, 0}, {0, 0,  1} }, \
                { {  1,  0, 0}, {  0,  0, 1}, {0, 1,  0} }  \
                                        }

#define Z4U_GPIO_LCM_ID0       (34)
#define Z4U_AUD_UART_OEz		(30)
#define Z4U_AUD_REMO_PRESz	(40)
#define Z4U_AUD_UART_RX		(122)
#define Z4U_AUD_UART_TX		(123)
#define Z4U_AUD_UART_SEL		(118)


/* Camera */
#define Z4U_GPIO_CAM_I2C_SDA    (61)
#define Z4U_GPIO_CAM_I2C_SCL    (60)
#define Z4U_GPIO_CAM_MCLK       (15)
#define Z4U_GPIO_CAM_ID         (6)
/* Rawchip */
#define	Z4U_GPIO_RAW_INTR0      (23)
#define	Z4U_GPIO_RAW_INTR1      (26)

#define CPLDGPIO(x) (x)
#define CP3_VOL_UP						CPLD_EXT_GPIO_KEY_UP_INPUT_LEVEL
#define CP3_VOL_DN						CPLD_EXT_GPIO_KEY_DW_INPUT_LEVEL
/* CPLD */
#define	Z4U_GPIO_CPLD_TMS       (13)
#define	Z4U_GPIO_CPLD_TCK       (11)
#define	Z4U_GPIO_CPLD_RST       (49)
#define	Z4U_GPIO_CPLD_CLK       (4)
#define	Z4U_GPIO_CPLD_INT       (39)
#define	Z4U_GPIO_CPLD_I2CEN     (116)

#define	Z4U_GPIO_ADDR_0         (130)
#define	Z4U_GPIO_ADDR_1         (128)
#define	Z4U_GPIO_ADDR_2         (115)
#define	Z4U_GPIO_DATA_0         (120)
#define	Z4U_GPIO_DATA_1         (117)
#define	Z4U_GPIO_DATA_2         (111)
#define	Z4U_GPIO_DATA_3         (121)
#define	Z4U_GPIO_DATA_4         (119)
#define	Z4U_GPIO_DATA_5         (129)
#define	Z4U_GPIO_DATA_6         (124)
#define	Z4U_GPIO_DATA_7         (125)
#define	Z4U_GPIO_OE             (126)
#define	Z4U_GPIO_WE             (127)

#define Z4U_GPIO_MBAT_IN	(28)


void __init z4u_camera_init(void);

int __init z4_init_panel(void);
extern int __init cp3dcg_init_panel(void);
#endif
