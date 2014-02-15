/*
 * RTC subsystem, initialize system time on startup
 *
 * Copyright (C) 2005 Tower Technologies
 * Author: Alessandro Zummo <a.zummo@towertech.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/rtc.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>


#define HTC_RTC_SYNC_ENABLE 1

#if HTC_RTC_SYNC_ENABLE
static void htc_rtc_sync_work(struct work_struct *work);
static struct delayed_work sync_work;

static void htc_rtc_sync_work(struct work_struct *work)
{
        int err = -ENODEV;
        struct rtc_time tm;
        struct timespec tv = {
                .tv_nsec = NSEC_PER_SEC >> 1,
        };
	struct timeval utc_tv;

        struct rtc_device *rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);
        if (rtc == NULL) {
                pr_err("%s: unable to open rtc device (%s)\n",
                        __FILE__, CONFIG_RTC_HCTOSYS_DEVICE);
                goto err_open;
        }
        err = rtc_read_time(rtc, &tm);
        if (err) {
                dev_err(rtc->dev.parent,
                        "hctosys: unable to read the hardware clock\n");
                goto err_read;

        }

        err = rtc_valid_tm(&tm);
        if (err) {
                dev_err(rtc->dev.parent,
                        "hctosys: invalid date/time\n");
                goto err_invalid;
        }

        rtc_tm_to_time(&tm, &tv.tv_sec);

	do_gettimeofday(&utc_tv);
	
	printk(KERN_INFO "[TIME] %s: UTC.tv_sec:%ld, RTC.tv_sec:%ld\n", __func__, utc_tv.tv_sec, tv.tv_sec);
	if(((utc_tv.tv_sec - tv.tv_sec) > 60) || ((tv.tv_sec - utc_tv.tv_sec) > 60)){
		printk(KERN_INFO "[TIME] %s: go to sync time.\n", __func__);
		
        	do_settimeofday(&tv);
	
        	dev_info(rtc->dev.parent,
                	"HTC_RTC_SYNC: setting system clock to "
                	"%d-%02d-%02d %02d:%02d:%02d UTC (%u)\n",
                	tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                	tm.tm_hour, tm.tm_min, tm.tm_sec,
                	(unsigned int) tv.tv_sec);
	}

err_invalid:
err_read:
        rtc_class_close(rtc);

err_open:

        
        schedule_delayed_work(&sync_work, 120 * HZ); 

}
#endif

int rtc_hctosys_ret = -ENODEV;

int rtc_hctosys(void)
{
	int err = -ENODEV;
	struct rtc_time tm;
	struct timespec tv = {
		.tv_nsec = NSEC_PER_SEC >> 1,
	};
	struct rtc_device *rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);
#if HTC_RTC_SYNC_ENABLE
	static int rtc_sync_enable = 0; 
#endif

	printk(KERN_INFO "[TIME] %s ++\n", __func__);

	if (rtc == NULL) {
		pr_err("%s: unable to open rtc device (%s)\n",
			__FILE__, CONFIG_RTC_HCTOSYS_DEVICE);
		goto err_open;
	}

	err = rtc_read_time(rtc, &tm);
	if (err) {
		dev_err(rtc->dev.parent,
			"hctosys: unable to read the hardware clock\n");
		goto err_read;

	}

	err = rtc_valid_tm(&tm);
	if (err) {
		dev_err(rtc->dev.parent,
			"hctosys: invalid date/time\n");
		goto err_invalid;
	}

	rtc_tm_to_time(&tm, &tv.tv_sec);

	do_settimeofday(&tv);

	dev_info(rtc->dev.parent,
		"setting system clock to "
		"%d-%02d-%02d %02d:%02d:%02d UTC (%u)\n",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec,
		(unsigned int) tv.tv_sec);

err_invalid:
err_read:
	rtc_class_close(rtc);

err_open:
	rtc_hctosys_ret = err;

        
#if HTC_RTC_SYNC_ENABLE
	if(!rtc_sync_enable){
        	INIT_DELAYED_WORK_DEFERRABLE(&sync_work, htc_rtc_sync_work);

        	
        	schedule_delayed_work(&sync_work, 60 * HZ);

		rtc_sync_enable = 1;
	}
#endif
        

	printk(KERN_INFO "[TIME] %s --\n", __func__);

	return err;
}

late_initcall(rtc_hctosys);
