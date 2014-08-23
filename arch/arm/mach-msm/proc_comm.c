/* arch/arm/mach-msm/proc_comm.c
 *
 * Copyright (C) 2007-2008 Google, Inc.
 * Copyright (c) 2009-2012, Code Aurora Forum. All rights reserved.
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
 */

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <mach/msm_iomap.h>
#include <mach/system.h>
#include <mach/proc_comm.h>

#include "smd_private.h"

static inline void notify_other_proc_comm(void)
{
	
	wmb();
#if defined(CONFIG_ARCH_MSM7X30)
	__raw_writel(1 << 6, MSM_APCS_GCC_BASE + 0x8);
#elif defined(CONFIG_ARCH_MSM8X60)
	__raw_writel(1 << 5, MSM_GCC_BASE + 0x8);
#else
	__raw_writel(1, MSM_CSR_BASE + 0x400 + (6) * 4);
#endif
}

#define APP_COMMAND 0x00
#define APP_STATUS  0x04
#define APP_DATA1   0x08
#define APP_DATA2   0x0C

#define MDM_COMMAND 0x10
#define MDM_STATUS  0x14
#define MDM_DATA1   0x18
#define MDM_DATA2   0x1C

#ifdef CONFIG_HTC_ACPU_DEBUG
#define BACKUP_APP_COMMAND	0xf8010
#define BACKUP_APP_DATA1	0xf8014
#define BACKUP_APP_DATA2	0xf8018

#define BACKUP_NCP_VOTAGE	0xf8020
#define BACKUP_CPU_RATE		0xf8024
#define BACKUP_AXI_RATE		0xf8028
#define BACKUP_CPU_STATUS	0xf802c

#define BACKUP_PLL0_STATUS	0xf8030
#define BACKUP_PLL1_STATUS	0xf8034
#define BACKUP_PLL2_STATUS	0xf8038
#define BACKUP_PLL4_STATUS	0xf803c
#define BACKUP_PLL5_STATUS	0xf8040

#define BACKUP_PLL_CLK_ENABLE	0xf8044
#define BACKUP_PLL_CLK_DISABLE	0xf8048

#define BACKUP_CLK_DISABLE_NAME	0xf8050
#define BACKUP_CLK_DISABLE	0xf805c

#define BACKUP_CLK_ENABLE_NAME	0xf8060
#define BACKUP_CLK_ENABLE	0xf806c
#endif


static DEFINE_SPINLOCK(proc_comm_lock);
static int msm_proc_comm_disable;

/* Poll for a state change, checking for possible
 * modem crashes along the way (so we don't wait
 * forever while the ARM9 is blowing up).
 *
 * Return an error in the event of a modem crash and
 * restart so the msm_proc_comm() routine can restart
 * the operation from the beginning.
 */
static int proc_comm_wait_for(unsigned addr, unsigned value)
{
	while (1) {
		
		mb();
		if (readl_relaxed(addr) == value)
			return 0;

		if (smsm_check_for_modem_crash())
			return -EAGAIN;

		udelay(5);
	}
}

void msm_proc_comm_reset_modem_now(void)
{
	unsigned base = (unsigned)MSM_SHARED_RAM_BASE;
	unsigned long flags;

	spin_lock_irqsave(&proc_comm_lock, flags);

again:
	if (proc_comm_wait_for(base + MDM_STATUS, PCOM_READY))
		goto again;

	writel_relaxed(PCOM_RESET_MODEM, base + APP_COMMAND);
	writel_relaxed(0, base + APP_DATA1);
	writel_relaxed(0, base + APP_DATA2);

	spin_unlock_irqrestore(&proc_comm_lock, flags);

	
	wmb();
	notify_other_proc_comm();

	return;
}
EXPORT_SYMBOL(msm_proc_comm_reset_modem_now);

#ifdef CONFIG_HTC_ACPU_DEBUG
static int msm_proc_htc_cmd_handler(unsigned cmd, unsigned *data1, unsigned *data2) {
	int ret = 0;
	unsigned base = (unsigned)MSM_SHARED_RAM_BASE;

	if ( base == 0 ) {
		return 0;
	}

	switch(cmd) {
	case PCOM_BACKUP_NCP_VOLTAGE:
		writel_relaxed(data1 ? *data1 : 0, base + BACKUP_NCP_VOTAGE);
		ret = 0;
		break;
	case PCOM_READ_CPU_RATE:
		if (data1) {
			*data1 = readl_relaxed(base + BACKUP_CPU_RATE);
		}
		break;
        case PCOM_BACKUP_CPU_AXI_RATE:
		writel_relaxed(data1 ? *data1 : 0, base + BACKUP_CPU_RATE);
		writel_relaxed(data2 ? *data2 : 0, base + BACKUP_AXI_RATE);
		break;
		ret = 0;
		break;
	case PCOM_BACKUP_CPU_STATUS:
		if (data1 && data2) {
			unsigned int cpu_status = readl_relaxed(base +  BACKUP_CPU_STATUS);
			switch(*data1) {
			case 0:
				writel_relaxed((cpu_status & 0xFFFFFF00) | (*data2?0x00000001:0x0), base + BACKUP_CPU_STATUS);
				break;
			case 1:
				writel_relaxed((cpu_status & 0xFFFF00FF) | (*data2?0x00000100:0x0), base + BACKUP_CPU_STATUS);
				break;
			case 2:
				writel_relaxed((cpu_status & 0xFF00FFFF) | (*data2?0x00010000:0x0), base + BACKUP_CPU_STATUS);
				break;
			case 3:
				writel_relaxed((cpu_status & 0x00FFFFFF) | (*data2?0x01000000:0x0), base + BACKUP_CPU_STATUS);
				break;
			}
		}
		ret = 0;
		break;
	case PCOM_BACKUP_PLL_STATUS:
		if (data1 && data2) {
			switch( *data1 ) {
			case 0:
				writel_relaxed( *data2, base + BACKUP_PLL0_STATUS);
				break;
			case 1:
				writel_relaxed( *data2, base + BACKUP_PLL1_STATUS);
				break;
			case 2:
				writel_relaxed( *data2, base + BACKUP_PLL2_STATUS);
				break;
			case 4:
				writel_relaxed( *data2, base + BACKUP_PLL4_STATUS);
				break;
			case 5:
				writel_relaxed( *data2, base + BACKUP_PLL5_STATUS);
				break;
			}
		}
		ret = 0;
		break;
	case PCOM_BACKUP_PLL_CLK_ENABLE_ENTER:
		writel_relaxed( 0x1, base + BACKUP_PLL_CLK_ENABLE);
		break;
	case PCOM_BACKUP_PLL_CLK_ENABLE_LEAVE:
		writel_relaxed( 0x0, base + BACKUP_PLL_CLK_ENABLE);
		break;
	case PCOM_BACKUP_PLL_CLK_DISABLE_ENTER:
		writel_relaxed( 0x1, base + BACKUP_PLL_CLK_DISABLE);
		break;
	case PCOM_BACKUP_PLL_CLK_DISABLE_LEAVE:
		writel_relaxed( 0x0, base + BACKUP_PLL_CLK_DISABLE);
		break;
	case PCOM_BACKUP_CLK_ENABLE:
		if (data1 && data2){
			strcpy((char*)(base + BACKUP_CLK_ENABLE_NAME), (char*) data2);
			writel_relaxed( *data1, base + BACKUP_CLK_ENABLE);
		}
		break;
	case PCOM_BACKUP_CLK_DISABLE:
		if (data1 && data2){
			strcpy((char*)(base + BACKUP_CLK_DISABLE_NAME), (char*) data2);
			writel_relaxed( *data1, base + BACKUP_CLK_DISABLE);
		}
		break;
	default:
		return -1;
	}

	return ret;
}
#endif

int msm_proc_comm(unsigned cmd, unsigned *data1, unsigned *data2)
{
	unsigned base = (unsigned)MSM_SHARED_RAM_BASE;
	unsigned long flags;
	int ret;

#ifdef CONFIG_HTC_ACPU_DEBUG
	if ( 0 == msm_proc_htc_cmd_handler( cmd, data1, data2)) {
		return 0;
	}
#endif

        
        if(cmd == PCOM_FINAL_EFS_SYNC)
        {
                printk(KERN_INFO "[HTC][EFS] update efs  magic number:%x\n", (data1 ? *data1 : 0));
                writel_relaxed(data1 ? *data1 : 0, base + APP_EFS_MAGIC);
                return 0;
        }
        


	spin_lock_irqsave(&proc_comm_lock, flags);

	if (msm_proc_comm_disable) {
		ret = -EIO;
		goto end;
	}


again:
	if (proc_comm_wait_for(base + MDM_STATUS, PCOM_READY))
		goto again;

#if (defined(CONFIG_MACH_PRIMODS) || defined(CONFIG_MACH_PROTOU) || defined(CONFIG_MACH_PROTODUG) || defined(CONFIG_MACH_PROTODCG) || defined(CONFIG_MACH_MAGNIDS) || defined(CONFIG_MACH_CP3DCG) || defined(CONFIG_MACH_CP3DTG) || defined(CONFIG_MACH_CP3DUG))
       if ((cmd == PCOM_CLKCTL_RPC_DISABLE) && (*data1 == 0x1F)) {
               
               ret = 0;
               goto end;
       }
#endif

#ifdef CONFIG_HTC_ACPU_DEBUG
        writel_relaxed(cmd, base + BACKUP_APP_COMMAND);
        writel_relaxed(data1 ? *data1 : 0, base + BACKUP_APP_DATA1);
        writel_relaxed(data2 ? *data2 : 0, base + BACKUP_APP_DATA2);
#endif
	writel_relaxed(cmd, base + APP_COMMAND);
	writel_relaxed(data1 ? *data1 : 0, base + APP_DATA1);
	writel_relaxed(data2 ? *data2 : 0, base + APP_DATA2);

	
	wmb();
	notify_other_proc_comm();

	if (proc_comm_wait_for(base + APP_COMMAND, PCOM_CMD_DONE))
		goto again;

	if (readl_relaxed(base + APP_STATUS) == PCOM_CMD_SUCCESS) {
		if (data1)
			*data1 = readl_relaxed(base + APP_DATA1);
		if (data2)
			*data2 = readl_relaxed(base + APP_DATA2);
		ret = 0;
	} else {
		ret = -EIO;
	}

	writel_relaxed(PCOM_CMD_IDLE, base + APP_COMMAND);

	switch (cmd) {
	case PCOM_RESET_CHIP:
	case PCOM_RESET_CHIP_IMM:
	case PCOM_RESET_APPS:
#if 1
		
#else
		msm_proc_comm_disable = 1;
		printk(KERN_ERR "msm: proc_comm: proc comm disabled\n");
#endif
		break;
	}
end:
	
	wmb();
	spin_unlock_irqrestore(&proc_comm_lock, flags);
	return ret;
}
EXPORT_SYMBOL(msm_proc_comm);
