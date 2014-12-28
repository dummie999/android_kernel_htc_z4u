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
#ifndef __ARCH_ARM_MACH_MSM_BOARD_CP3DCG__
#define __ARCH_ARM_MACH_MSM_BOARD_CP3DCG__

#include "board-cp3.h"

#define CP3DCG_GPIO_PS_HOLD        (25)
#define CP3DCG_GPIO_LCM_ID0        (34)
/* Compass  */
#define CP3DCG_GPIO_GSENSORS_INT         (83)
#define CP3DCG_LAYOUTS			{ \
		{ {  0, -1, 0}, { -1,  0, 0}, {0, 0, -1} }, \
		{ {  0, -1, 0}, { -1,  0, 0}, {0, 0,  1} }, \
		{ { -1,  0, 0}, {  0,  1, 0}, {0, 0, -1} }, \
		{ {  1,  0, 0}, {  0,  0, 1}, {0, 1,  0} }  \
					}

#define CP3DCG_LAYOUTS_EVT                  { \
                { {  0, -1, 0}, { -1,  0, 0}, {0, 0, -1} }, \
                { {  0, -1, 0}, { -1,  0, 0}, {0, 0,  1} }, \
                { { -1,  0, 0}, {  0,  1, 0}, {0, 0, -1} }, \
                { { -1,  0, 0}, {  0,  0,-1}, {0, 1,  0} }  \
                                        }

#define CP3DCG_LAYOUTS_XC                  { \
                { {  0, -1, 0}, { -1,  0, 0}, {0, 0, -1} }, \
                { {  0, -1, 0}, { -1,  0, 0}, {0, 0,  1} }, \
                { { -1,  0, 0}, {  0,  1, 0}, {0, 0, -1} }, \
                { {  0, -1, 0}, {  0,  0, 1}, {1, 0,  0} }  \
                                        }


#define CP3DCG_AUD_UART_OEz			(30)
#define CP3DCG_AUD_REMO_PRESz		(40)
#define CP3DCG_AUD_UART_RX		(122)
#define CP3DCG_AUD_UART_TX		(123)
#define CP3DCG_AUD_UART_SEL			(69)


/* Camera */
#define CP3DCG_GPIO_CAM_I2C_SDA    (61)
#define CP3DCG_GPIO_CAM_I2C_SCL    (60)
#define CP3DCG_GPIO_CAM_MCLK       (15)
#define CP3DCG_GPIO_CAM_ID         (84)
/* Rawchip */
#define	CP3DCG_GPIO_RAW_INTR0      (23)
#define	CP3DCG_GPIO_RAW_INTR1      (26)

/* CPLD */
#define	CP3DCG_GPIO_CPLD_TMS       (13)
#define	CP3DCG_GPIO_CPLD_TCK       (11)
#define	CP3DCG_GPIO_CPLD_RST       (49)
#define	CP3DCG_GPIO_CPLD_CLK       (4)
#define	CP3DCG_GPIO_CPLD_INT       (39)
#define	CP3DCG_GPIO_CPLD_I2CEN     (116)

#define	CP3DCG_GPIO_ADDR_0         (130)
#define	CP3DCG_GPIO_ADDR_1         (128)
#define	CP3DCG_GPIO_ADDR_2         (115)
#define	CP3DCG_GPIO_DATA_0         (120)
#define	CP3DCG_GPIO_DATA_1         (117)
#define	CP3DCG_GPIO_DATA_2         (111)
#define	CP3DCG_GPIO_DATA_3         (121)
#define	CP3DCG_GPIO_DATA_4         (119)
#define	CP3DCG_GPIO_DATA_5         (129)
#define	CP3DCG_GPIO_DATA_6         (124)
#define	CP3DCG_GPIO_DATA_7         (125)
#define	CP3DCG_GPIO_OE             (126)
#define	CP3DCG_GPIO_WE             (127)

#define CP3DCG_GPIO_MBAT_IN	(85)

void __init cp3dcg_camera_init(void);

int __init cp3dcg_init_panel(void);

extern int panel_type;
#endif
