#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/module.h>

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/cdev.h>

#include <linux/regulator/consumer.h>
#include <linux/regulator/onsemi-ncp6335d.h>


static int rild_value;

static struct class *z4mode_change_class;
struct z4mode_change_device {
	const char *name;
	struct device *device;
	struct cdev cdev;
};
static struct z4mode_change_device *pdev;


static int set_value(int setmode)
{
	int ret = 0;

	struct regulator *ncp_regulator;

	ncp_regulator = regulator_get(NULL, "ncp6335d");

	ret = regulator_get_mode(ncp_regulator);

	if(ret == REGULATOR_MODE_FAST)
		pr_info("Z4 mode is PWM.\n");
	else if(ret == REGULATOR_MODE_NORMAL)	
		pr_info("Z4 mode is PFM.\n");
	else
		pr_info("Z4 mode is unknown.\n");

	if(rild_value == 1){ 
		if(setmode == 2){
			pr_info("Z4 mode: set back to PFM\n");
			rild_value = 2;
			ret = regulator_set_mode(ncp_regulator, REGULATOR_MODE_NORMAL);
		}else{
			pr_info("Z4 mode: no need to change.\n");
		}
	}else{
		if(setmode == 1){
			pr_info("Z4 mode: set to PWM\n");
			rild_value = 1;
			ret = regulator_set_mode(ncp_regulator, REGULATOR_MODE_FAST);
		}else if(setmode == 3){
			pr_info("Z4 mode: set PWM\n");
			rild_value = 3;
			ret = regulator_set_mode(ncp_regulator, REGULATOR_MODE_FAST);
		}else if(setmode == 4){
			pr_info("Z4 mode: set PFM\n");
			rild_value = 4;
			ret = regulator_set_mode(ncp_regulator, REGULATOR_MODE_NORMAL);
		}else{
			pr_info("Z4 mode: change unknown stage.\n");
		}
	}

	ret = regulator_get_mode(ncp_regulator);

	if(ret == REGULATOR_MODE_FAST)
		pr_info("Z4 mode: PWM.\n");
	else if(ret == REGULATOR_MODE_NORMAL)	
		pr_info("Z4 mode: PFM.\n");
	else
		pr_info("[Z4 mode: unknown.\n");


	pr_info("Z4 mode set done.\n");

	return ret;
}


static int z4mode_change_release(struct inode *ip, struct file *fp)
{
	return 0;
}

static int z4mode_change_open(struct inode *ip, struct file *fp)
{
	return 0;
}

static const struct file_operations z4mode_change_fops = {
	.owner = THIS_MODULE,
	.open = z4mode_change_open,
	.release = z4mode_change_release,
};

static ssize_t z4mode_change_read(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int ret = 0;

	ret = sprintf(buf, "%d\n", rild_value);

	return ret;
}

static ssize_t z4mode_change_write(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	if (!strncmp(buf, "1", 1)){ 
		set_value(1);
	}else if (!strncmp(buf, "2", 1)){ 
		set_value(2);
	}else if (!strncmp(buf, "3", 1)){ 
		set_value(3);
	}else if (!strncmp(buf, "4", 1)){ 
		set_value(4);
	}
	return count;
}

static DEVICE_ATTR(z4mode_change, 0777, z4mode_change_read, z4mode_change_write);

static void __exit z4mode_change_exit(void)
{
	class_unregister(z4mode_change_class);
	kfree(pdev);
}

static int __init z4mode_change_init(void)
{
	int ret;

	pdev = kzalloc(sizeof(struct z4mode_change_device), GFP_KERNEL);
	if (!pdev)
		return -ENOMEM;

	z4mode_change_class = class_create(THIS_MODULE, "z4dev");
	if (IS_ERR(z4mode_change_class)) {
		pr_err("create z4mode class fail\n");
		goto fail_create_class;
	}

	pdev->device = device_create(z4mode_change_class, NULL, 0, "%s", "z4mode");
	if (unlikely(IS_ERR(pdev->device))) {
		pr_err("create z4mode device fail\n");
		goto fail_create_device;
	}

	ret = register_chrdev(0, "z4mode", &z4mode_change_fops);
	if (ret < 0) {
		pr_err("create z4mode device fail\n");
		goto fail_create_device;
	}

	ret = device_create_file(pdev->device, &dev_attr_z4mode_change);
	if (ret) {
		pr_err("create device_create_file fail\n");
		goto fail_create_file;
	}

	rild_value = 0;
	return 0;


fail_create_class:
	class_unregister(z4mode_change_class);
fail_create_device:
fail_create_file:
	device_unregister(pdev->device);

	return -1;
}

module_init(z4mode_change_init);
module_exit(z4mode_change_exit);

MODULE_DESCRIPTION("z4 mode change Driver");
MODULE_LICENSE("GPL v2");
