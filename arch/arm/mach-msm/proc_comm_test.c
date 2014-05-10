/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
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


#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <mach/proc_comm.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/cdev.h>

static int proc_comm_test_res;

static struct class *proc_comm_class;
struct proc_comm_device {
	const char *name;
	struct device *device;
	struct cdev cdev;
};
static struct proc_comm_device *pdev;

static int proc_comm_reverse_test(void)
{
	uint32_t data1, data2;
	int rc;

	data1 = 5;
	data2 = 0;

	rc = msm_proc_comm(PCOM_CUSTOMER_CMD3, &data1, &data2);
	pr_info("read adc return data1: %d, data2: %d\n", data1, data2);
	if (rc < 0)
		return rc;

	return data2;
}

static ssize_t proc_comm_read(struct file *fp, char __user *buf,
			  size_t count, loff_t *pos)
{
	char _buf[16];

	snprintf(_buf, sizeof(_buf), "%i\n", proc_comm_test_res);

	return simple_read_from_buffer(buf, count, pos, _buf, strlen(_buf));
}

static ssize_t proc_comm_write(struct file *fp, const char __user *buf,
			   size_t count, loff_t *pos)
{

	unsigned char cmd[64];
	int len;

	if (count < 1)
		return 0;

	len = count > 63 ? 63 : count;

	if (copy_from_user(cmd, buf, len))
		return -EFAULT;

	cmd[len] = 0;

	if (cmd[len-1] == '\n') {
		cmd[len-1] = 0;
		len--;
	}

	if (!strncmp(cmd, "reverse_test", 64))
		proc_comm_test_res = proc_comm_reverse_test();
	else
		proc_comm_test_res = -EINVAL;

	return count;
}

static int proc_comm_release(struct inode *ip, struct file *fp)
{
	return 0;
}

static int proc_comm_open(struct inode *ip, struct file *fp)
{
	return 0;
}

static const struct file_operations proc_comm_fops = {
	.owner = THIS_MODULE,
	.open = proc_comm_open,
	.release = proc_comm_release,
	.read = proc_comm_read,
	.write = proc_comm_write,
};

static ssize_t adc_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int ret = 0;

	ret = sprintf(buf, "%d\n", proc_comm_test_res);
	return ret;
}

static ssize_t adc_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	proc_comm_test_res = proc_comm_reverse_test();

	return count;
}
static DEVICE_ATTR(adc, 0664, adc_show, adc_store);

static void __exit proc_comm_test_mod_exit(void)
{
	class_unregister(proc_comm_class);
	kfree(pdev);
}

static int __init proc_comm_test_mod_init(void)
{
	int ret;

	pdev = kzalloc(sizeof(struct proc_comm_device), GFP_KERNEL);
	if (!pdev)
		return -ENOMEM;

	proc_comm_class = class_create(THIS_MODULE, "thermal");
	if (IS_ERR(proc_comm_class)) {
		pr_err("create proc_comm class fail\n");
		goto fail_create_class;
	}

	pdev->device = device_create(proc_comm_class, NULL, 0, "%s", "proc_comm");
	if (unlikely(IS_ERR(pdev->device))) {
		pr_err("create proc_comm device fail\n");
		goto fail_create_device;
	}

	ret = register_chrdev(0, "proc_comm", &proc_comm_fops);
	if (ret < 0) {
		pr_err("create proc_comm device fail\n");
		goto fail_create_device;
	}

	ret = device_create_file(pdev->device, &dev_attr_adc);
	if (ret) {
		pr_err("create device_create_file fail\n");
		goto fail_create_file;
	}

	proc_comm_test_res = -1;
	proc_comm_reverse_test();

	return 0;

fail_create_class:
	class_unregister(proc_comm_class);
fail_create_device:
fail_create_file:
	device_unregister(pdev->device);

	return -1;
}

module_init(proc_comm_test_mod_init);
module_exit(proc_comm_test_mod_exit);

MODULE_DESCRIPTION("PROC COMM TEST Driver");
MODULE_LICENSE("GPL v2");
