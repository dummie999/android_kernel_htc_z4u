/* linux/arch/arm/mach-msm/board-protou.h
 * Copyright (C) 2007-2009 HTC Corporation.
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

#ifndef __ARCH_ARM_MACH_MSM_BOARD_PROTOU_H
#define __ARCH_ARM_MACH_MSM_BOARD_PROTOU_H

#include <mach/board.h>



#define PROTOU_GPIO_WIFI_IRQ             29
#define PROTOU_GPIO_WIFI_SHUTDOWN_N       13


int protou_init_mmc(unsigned int sys_rev);
int __init protou_wifi_init(void);


#endif 
