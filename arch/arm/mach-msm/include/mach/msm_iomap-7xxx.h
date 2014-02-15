/*
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2012, Code Aurora Forum. All rights reserved.
 * Author: Brian Swetland <swetland@google.com>
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
 *
 * The MSM peripherals are spread all over across 768MB of physical
 * space, which makes just having a simple IO_ADDRESS macro to slide
 * them into the right virtual location rough.  Instead, we will
 * provide a master phys->virt mapping for peripherals here.
 *
 */

#ifndef __ASM_ARCH_MSM_IOMAP_7XXX_H
#define __ASM_ARCH_MSM_IOMAP_7XXX_H


#define MSM_SHARED_RAM_PHYS   0x00200000

#define MSM7XXX_VIC_PHYS          0xC0000000
#define MSM7XXX_VIC_SIZE          SZ_4K

#define MSM7XXX_CSR_PHYS          0xC0100000
#define MSM7XXX_CSR_SIZE          SZ_4K

#define MSM7XXX_TMR_PHYS          MSM7XXX_CSR_PHYS
#define MSM7XXX_TMR_SIZE          SZ_4K

#define MSM7XXX_GPIO1_PHYS        0xA9200000
#define MSM7XXX_GPIO1_SIZE        SZ_4K

#define MSM7XXX_GPIO2_PHYS        0xA9300000
#define MSM7XXX_GPIO2_SIZE        SZ_4K

#define MSM7XXX_CLK_CTL_PHYS      0xA8600000
#define MSM7XXX_CLK_CTL_SIZE      SZ_4K

#define MSM7XXX_L2CC_PHYS         0xC0400000
#define MSM7XXX_L2CC_SIZE         SZ_4K

#define MSM7XXX_UART1_PHYS        0xA9A00000
#define MSM7XXX_UART1_SIZE        SZ_4K

#define MSM7XXX_UART2_PHYS        0xA9B00000
#define MSM7XXX_UART2_SIZE        SZ_4K

#define MSM7XXX_UART3_PHYS        0xA9C00000
#define MSM7XXX_UART3_SIZE        SZ_4K

#define MSM7XXX_MDC_PHYS	  0xAA500000
#define MSM7XXX_MDC_SIZE          SZ_1M

#define MSM7XXX_AD5_PHYS          0xAC000000
#define MSM7XXX_AD5_SIZE          (SZ_1M*13)

#define MSM7XXX_PBUS_PHYS         0xA0E00000
#define MSM7XXX_PBUS_SIZE         SZ_4K

#define MSM_UART3_PHYS        0xA9C00000
#define MSM_UART3_SIZE        SZ_4K

#ifdef CONFIG_MSM_DEBUG_UART
#define MSM_DEBUG_UART_BASE   0xFB000000
#if CONFIG_MSM_DEBUG_UART == 1
#define MSM_DEBUG_UART_PHYS   MSM_UART1_PHYS
#elif CONFIG_MSM_DEBUG_UART == 2
#define MSM_DEBUG_UART_PHYS   MSM_UART2_PHYS
#elif CONFIG_MSM_DEBUG_UART == 3
#define MSM_DEBUG_UART_PHYS   MSM_UART3_PHYS
#endif
#define MSM_DEBUG_UART_SIZE   SZ_4K
#endif

#if defined(CONFIG_ARCH_MSM7X27A)
#if (defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY))
#define MSM_HTC_RAM_CONSOLE_PHYS        0x03A00000  
#else
#define MSM_HTC_RAM_CONSOLE_PHYS        0x03100000  
#endif
#define MSM_HTC_RAM_CONSOLE_SIZE	(SZ_1M - SZ_128K)  
#endif

#define MSM_HTC_DEBUG_INFO_BASE 	IOMEM(0xFB700000)
#define MSM_HTC_DEBUG_INFO_PHYS 	(MSM_HTC_RAM_CONSOLE_PHYS + MSM_HTC_RAM_CONSOLE_SIZE)
#define MSM_HTC_DEBUG_INFO_SIZE 	SZ_128K

#endif
