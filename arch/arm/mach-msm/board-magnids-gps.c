#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h> 
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h> 
#include <linux/io.h>     
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <asm/bitops.h>
#include <linux/semaphore.h> 
#include <linux/spinlock.h>
#include <linux/wait.h>           
#include <linux/sched.h>  
#include <linux/kfifo.h>  
#include <linux/timer.h>         
#include <linux/input.h>      
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <mach/vreg.h>
//for GPS_32K_IN
#include <linux/mfd/pm8xxx/gpio.h>

#include <mach/TCA6418_ioextender.h>   

#include <linux/miscdevice.h>                                                                                              
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <asm/mach-types.h>
#include <linux/io.h>

#include <linux/tty.h>
#include <mach/msm_serial_hs.h>

                                                                                

#ifdef PRIMOU_REWORK
#undef PRIMOU_REWORK
#endif
#define DEBUG_ON 1

#if DEBUG_ON
#define GPSD(fmt, arg...) printk(KERN_DEBUG "[GPS].(DEBUG) "fmt"", ##arg)
#define GPSE(fmt, arg...) printk(KERN_ERR "[GPS].(ERROR) "fmt"", ##arg)
#else
#define GPSD(...) 
#define GPSE(fmt, arg...) printk(KERN_ERR "[GPS].(ERROR) "fmt"", ##arg)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
#define LINUX_3
#endif

#define GPS_ONOFF_VERSION "1.0.0"
#define DEVICE_NAME "gps_onoff"

#ifndef PRIMOU_REWORK

#define PRIMOTD_GPIO_GPS_UART3_RX     (53)
#define PRIMOTD_GPIO_GPS_UART3_TX     (54)
#define PRIMOTD_GPIO_GPS_UART3_CTS    (55)
#define PRIMOTD_GPIO_GPS_UART3_RTS    (57)

#define GPS_CLK_32K_IN				(39-1)

#define GPS_POWER					(125)
#define GPS_ONOFF					(17)
#define GPS_RESET					(16)

#else

#define GPS_ONOFF					  (44)

#endif

#if (defined(CONFIG_MACH_MAGNIDS))
#define IOEXT_GPS_RESET		(16)
#define IOEXT_GPS_ONOFF		(17)
#endif

int gps_onoff_major = -1;
static struct class  *gps_onoff_class = NULL;   /* GPS class during class_create */
static struct device *gps_onoff_dev   = NULL;   /* GPS dev during device_create */

char status = '0';

static int config_gps_uart(void)
{
	return 0;
}


static int deconfig_gps_uart(void)
{
	return 0;
}

static void gps_onoff(int onoff)
{
	ioext_gpio_set_value(IOEXT_GPS_ONOFF, onoff);
}

static long gps_onoff_ioctl(
#ifndef LINUX_3
		struct inode *inode,
#endif
		struct file *filp , unsigned int cmd, unsigned long arg)
{
	switch(cmd){
		case 1:
			gps_onoff(1);
			status = '1';
			GPSD("set the gps_engine on\n");
			break;
		case 0:
			gps_onoff(0);
			status = '0';
			GPSD("set the gps_engine off\n");
			break;
		default:
			GPSE("the cmd is not correct, please check \n");
			return -1;
	}
	return 0;
}

static int gps_onoff_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_ops)
{
	char data[2];
	
	if( copy_from_user(data, buf, count) ) {
		GPSE("write data error\n");
		return -1;
	}
	
	if ( data[0] == '1' ){
		gps_onoff(1);
		status = '1';
		GPSD("set GPIO OK!\n");
	}
	else{
		gps_onoff(0);
		status = '0';
		GPSD("reset GPIO OK!\n");
	}
	
	return 1;
}

static int gps_onoff_read(struct file *filp, char *buf, size_t count, loff_t *f_ops)
{
	if( copy_to_user(buf, &status, 1)){
		GPSE("read data wrong\n");
		return -1;
	}
	
	return 1;
}


struct file_operations gps_onoff_ops={
	.owner	= THIS_MODULE,
	.write	= gps_onoff_write,
	.read	= gps_onoff_read,
#ifdef LINUX_3
	.unlocked_ioctl = 
#else
	.ioctl	= 
#endif
	gps_onoff_ioctl,
};

static int gps_onoff_init(struct platform_device *platdev)
{
	
	GPSD("In gps_onoff_init\n");

	ioext_gpio_set_value(IOEXT_GPS_RESET, 1);
	ioext_gpio_set_value(IOEXT_GPS_ONOFF, 0);

	gps_onoff_major = register_chrdev(0, DEVICE_NAME, &gps_onoff_ops);
	if(gps_onoff_major < 0){
		GPSE("register gps_onoff device err\n");
		goto err4;
	}
	GPSD(KERN_INFO "%s , major %d",DEVICE_NAME, gps_onoff_major);
	
	gps_onoff_class = class_create(THIS_MODULE, DEVICE_NAME);
	if(gps_onoff_class == NULL){
		GPSE("gps_onoff class create err\n");
		goto err5;
	}
	gps_onoff_dev = device_create(gps_onoff_class, NULL, MKDEV(gps_onoff_major, 0), NULL, DEVICE_NAME);
	if(gps_onoff_dev == NULL){
		GPSE("gps_onoff dev create err\n");
		goto err6;
	}

	GPSD("init OK\n");

	return 0;

//	device_destroy(gpsdrv_class, MKDEV(gpsdrv_major, 0));
err6:
	class_destroy(gps_onoff_class);
err5:
	unregister_chrdev(gps_onoff_major, DEVICE_NAME);
err4:	
	return -1;
}

static int  gps_onoff_exit(struct platform_device *platdev )
{
	GPSD("free up\n");
	device_destroy(gps_onoff_class, MKDEV(gps_onoff_major, 0));
	class_destroy(gps_onoff_class);
	unregister_chrdev(gps_onoff_major, DEVICE_NAME);
	return 0;
}

static int gps_suspend(struct platform_device *platdev, pm_message_t state)
{
	GPSD("In gps_suspend function\n");
	
	if(deconfig_gps_uart() < 0){
		GPSE("error occured when deinit gps uart");
		return -1;
	}

	return 0;
}

static int gps_resume(struct platform_device *platdev)
{
	GPSD("In gps_resume function\n");
	
	if(config_gps_uart() < 0){
		GPSE("resume to config to GPS uart err!\n ");
		return -1;
	}

	return 0;
}


static struct platform_driver gps_ctl_drv={
	.probe = gps_onoff_init,
	.remove = gps_onoff_exit,
	.suspend = gps_suspend,
	.resume = gps_resume,
	.driver = {
		.name = "gps_ctl",
		.owner = THIS_MODULE,
	},

};


static struct platform_device gps_ctl_dev={
	    .name="gps_ctl",
		.id=0,
};

static int __init gps_ctl_init(void)
{
        int ret;
	ret = platform_device_register(&gps_ctl_dev);
	if(ret < 0){
		GPSE("platform_device register error!\n");
		return ret;
	}

	ret = platform_driver_register(&gps_ctl_drv);
	if(ret < 0)
	{
		GPSE("platform_driver register error!\n");
		platform_device_unregister(&gps_ctl_dev);
		return ret;
	}
	
	return 0;
}

static void __exit gps_ctl_exit(void)
{
	platform_device_unregister(&gps_ctl_dev);
	platform_driver_unregister(&gps_ctl_drv);
}

module_init(gps_ctl_init);
module_exit(gps_ctl_exit);

MODULE_AUTHOR("Zhang Kai<kai_zhang@htc.com>");
MODULE_DESCRIPTION("GPS gpio control");
MODULE_LICENSE("GPL");
MODULE_VERSION(GPS_ONOFF_VERSION);

