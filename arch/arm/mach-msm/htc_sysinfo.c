/*
 * Copyright (C) 2010 HTC, Inc.
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <mach/board.h>

#define CHECK_PROC_ENTRY(name, entry) do { \
				if (entry) { \
					pr_info("Create /proc/%s OK.\n", name); \
				} else { \
					pr_err("Create /proc/%s FAILED.\n", name); \
				} \
			} while (0);

extern int board_get_boot_powerkey_debounce_time(void);
int sys_boot_powerkey_debounce_ms(char *page, char **start, off_t off,
			   int count, int *eof, void *data)
{
	char *p = page;

	p += sprintf(p, "%d\n", board_get_boot_powerkey_debounce_time());

	return p - page;
}

static int __init sysinfo_proc_init(void)
{
	struct proc_dir_entry *entry = NULL;

	pr_info("%s: Init HTC system info proc interface.\r\n", __func__);

	

	entry = create_proc_read_entry("powerkey_debounce_ms", 0, NULL, sys_boot_powerkey_debounce_ms, NULL);
	CHECK_PROC_ENTRY("powerkey_debounce_ms", entry);

	return 0;
}

module_init(sysinfo_proc_init);
MODULE_AUTHOR("Jimmy.CM Chen <jimmy.cm_chen@htc.com>");
MODULE_DESCRIPTION("HTC System Info Interface");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL v2");
