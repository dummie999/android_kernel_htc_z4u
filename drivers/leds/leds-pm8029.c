/* driver/leds/leds-pm8029.c
 *
 * Copyright (C) 2012 HTC Corporation.
 * Author: azhe_liu@htc.com
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/rtc.h>
#include <linux/wakelock.h>
#include <asm/atomic.h>
#include <linux/leds.h>
#include "../../../arch/arm/mach-msm/proc_comm.h"
#include <linux/leds-pm8029.h>

#ifdef DEBUG
#define LED_DBG_LOG(fmt, ...) \
		printk(KERN_DEBUG "[LED]" fmt, ##__VA_ARGS__)
#else
#define LED_DBG_LOG(fmt, ...)
#endif
#define LED_INFO_LOG(fmt, ...) \
		printk(KERN_INFO "[LED]" fmt, ##__VA_ARGS__)
#define LED_ERR_LOG(fmt, ...) \
		printk(KERN_ERR "[LED][ERR]" fmt, ##__VA_ARGS__)

#define LED_ALM(x...) do { \
struct timespec ts; \
struct rtc_time tm; \
getnstimeofday(&ts); \
rtc_time_to_tm(ts.tv_sec, &tm); \
printk(KERN_INFO "[LED-ALM] " x); \
printk(" at %lld (%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n", \
ktime_to_ns(ktime_get()), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, \
tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec); \
} while (0)


static struct workqueue_struct *led_wq;
static void pm8029_led_control(struct pm8029_led_data *ldata)
{
	uint32_t data1, data2;
	int rc;
	int blink = atomic_read(&ldata->blink);
	int brightness = atomic_read(&ldata->brightness);

	if (ldata->out_current < 0 || ldata->out_current > 255 ||
			brightness < 0 || brightness >255 ||
			blink < 0 || blink > 3)
		LED_ERR_LOG("%s: Wrong LED data. brightness = %d, blink mode = %d\n",
				__func__, brightness, blink);

	
	data1 = 0x2;
	data2 = 0x0;
	
	data2 |= ldata->out_current;
	
	data2 |= (brightness << 8);
	
	data2 |= (ldata->bank << 16);
	LED_INFO_LOG("%s: %s brightness=%d, data1=0x%x, data2=0x%x\n",
			__func__, ldata->ldev.name, brightness, data1, data2);
	rc = msm_proc_comm(PCOM_CUSTOMER_CMD2, &data1, &data2);
	if (rc)
		LED_ERR_LOG("%s: data2 0x%x, rc=%d\n", __func__, data2, rc);

	
	if (strcmp(ldata->ldev.name, "button-backlight")) {
		data1 = 0x4;
		data2 = 0X00;
		switch (blink) {
			case 0:
				if (brightness == 0)
					LED_INFO_LOG("%s: %s turn off, data1=0x%x, data2=0x%x\n",
						__func__, ldata->ldev.name, data1, data2);
				break;
			case 1:
				data2 |= (DUTY_64MS << 8);
				data2 |= (ldata->bank << 16);
				LED_INFO_LOG("%s: %s blink 64ms in 2s, data1=0x%x, data2=0x%x\n",
						__func__, ldata->ldev.name, data1, data2);
				rc = msm_proc_comm(PCOM_CUSTOMER_CMD2, &data1, &data2);
				if (rc)
					LED_ERR_LOG("%s: data2 0x%x, rc=%d\n", __func__, data2, rc);
				break;
			case 2:
				data2 |= (DUTY_1S << 8);
				data2 |= (ldata->bank << 16);
				LED_INFO_LOG("%s: %s blink 1s in 2s, data1=0x%x, data2=0x%x\n",
						__func__, ldata->ldev.name, data1, data2);
				rc = msm_proc_comm(PCOM_CUSTOMER_CMD2, &data1, &data2);
				if (rc)
					LED_ERR_LOG("%s: data2 0x%x, rc=%d\n", __func__, data2, rc);
				break;
		}
	}
}

static void led_do_blink(struct work_struct *work)
{
	struct pm8029_led_data *ldata;
	LED_DBG_LOG("%s()\n", __func__);
	ldata = container_of(work, struct pm8029_led_data, blink_work.work);

	pm8029_led_control(ldata);
}

static void pm8029_led_brightness_set(struct led_classdev *led_cdev,
					   enum led_brightness brightness)
{
	struct pm8029_led_data *ldata;

	ldata = container_of(led_cdev, struct pm8029_led_data, ldev);
	if (brightness > 0 && (ldata->flag & FIX_BRIGHTNESS))
		brightness = ldata->init_pwm_brightness;
	atomic_set(&ldata->brightness, brightness);
	
	atomic_set(&ldata->blink, BLINK_DISABLE);
	
	pm8029_led_control(ldata);

	LED_INFO_LOG("%s: brightness = %d\n",__func__, brightness);
}

static ssize_t pm8029_led_blink_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct pm8029_led_data *ldata;
	struct led_classdev *led_cdev;
	uint32_t val;

	sscanf(buf, "%u", &val);

	if (val < 0 || val > 4){
		LED_ERR_LOG("%s: Wrong blink mode %u", __func__, val);
		return -EINVAL;
	}

	led_cdev = (struct led_classdev *) dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct pm8029_led_data, ldev);
	atomic_set(&ldata->blink, val);

	switch(atomic_read(&ldata->blink)){
		case 0:
			atomic_set(&ldata->blink, BLINK_DISABLE);
			pm8029_led_control(ldata);
			break;
		case 1:
			atomic_set(&ldata->blink, BLINK_64MS_PER_2S);
			pm8029_led_control(ldata);
			break;
		case 2:
			cancel_delayed_work_sync(&ldata->blink_work);
                        atomic_set(&ldata->blink, BLINK_64MS_PER_2S);
                        queue_delayed_work(led_wq, &ldata->blink_work, msecs_to_jiffies(314));
                        break;
		case 3:
			cancel_delayed_work_sync(&ldata->blink_work);
			atomic_set(&ldata->blink, BLINK_64MS_PER_2S);
                        queue_delayed_work(led_wq, &ldata->blink_work, msecs_to_jiffies(1000));
                        break;
		case 4:
			atomic_set(&ldata->blink, BLINK_1S_PER_2S);
                        pm8029_led_control(ldata);
                        break;
		default:
			return -EINVAL;
	}

	return count;
}

static ssize_t pm8029_led_blink_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct pm8029_led_data *ldata;
        struct led_classdev *led_cdev;

        led_cdev = (struct led_classdev *) dev_get_drvdata(dev);
        ldata = container_of(led_cdev, struct pm8029_led_data, ldev);

	return sprintf(buf, "%d\n", atomic_read(&ldata->blink));
}
static DEVICE_ATTR(blink, 0664, pm8029_led_blink_show, pm8029_led_blink_store);

static ssize_t pm8029_led_currents_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct led_classdev *led_cdev;
	struct pm8029_led_data *ldata;

	led_cdev = (struct led_classdev *) dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct pm8029_led_data, ldev);

	return sprintf(buf, "%d\n", ldata->out_current);
}

static ssize_t pm8029_led_currents_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	int currents = 0;
	struct led_classdev *led_cdev;
	struct pm8029_led_data *ldata;

	sscanf(buf, "%d", &currents);
	if (currents < 0)
		return -EINVAL;

	led_cdev = (struct led_classdev *)dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct pm8029_led_data, ldev);

	LED_INFO_LOG("%s: bank %d currents %d +\n", __func__, ldata->bank, currents);

	ldata->out_current = currents;

	ldata->ldev.brightness_set(led_cdev, 0);
	if (currents)
		ldata->ldev.brightness_set(led_cdev, 255);

	LED_INFO_LOG("%s: bank %d currents %d -\n", __func__, ldata->bank, currents);
	return count;
}

static DEVICE_ATTR(currents, 0664, pm8029_led_currents_show,
		   pm8029_led_currents_store);

static ssize_t pm8029_led_off_timer_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev;
	struct pm8029_led_data *ldata;

	led_cdev = (struct led_classdev *)dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct pm8029_led_data, ldev);
	return sprintf(buf, "%d s\n", atomic_read(&ldata->off_timer));
}

static ssize_t pm8029_led_off_timer_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct led_classdev *led_cdev;
	struct pm8029_led_data *ldata;
	int min, sec;
	ktime_t interval;
	ktime_t next_alarm;

	min = -1;
	sec = -1;
	sscanf(buf, "%d %d", &min, &sec);

	if (min < 0 || min > 255 || sec < 0 || sec > 255) {
		LED_ERR_LOG("%s: min=%d, sec=%d,Invalid off_timer!\n",
				__func__, min, sec);
		return -EINVAL;
	}

	led_cdev = (struct led_classdev *)dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct pm8029_led_data, ldev);

	LED_INFO_LOG("Setting %s off_timer to %d min %d sec\n",
					   led_cdev->name, min, sec);

	atomic_set(&ldata->off_timer, min * 60 + sec);
	alarm_cancel(&ldata->off_timer_alarm);
	cancel_work_sync(&ldata->off_timer_work);
	if (atomic_read(&ldata->off_timer)) {
		interval = ktime_set(atomic_read(&ldata->off_timer), 0);
		next_alarm = ktime_add(alarm_get_elapsed_realtime(), interval);
		alarm_start_range(&ldata->off_timer_alarm, next_alarm, next_alarm);
		LED_ALM("led alarm start -");
	}
	return count;
}

static DEVICE_ATTR(off_timer, 0664, pm8029_led_off_timer_show,
				      pm8029_led_off_timer_store);

static void led_work_func(struct work_struct *work)
{
	struct pm8029_led_data *ldata;

	ldata = container_of(work, struct pm8029_led_data, off_timer_work);
	LED_ALM("%s led alarm led work +" , ldata->ldev.name);
	
	atomic_set(&ldata->brightness, 0);
	atomic_set(&ldata->blink, 0);
	atomic_set(&ldata->off_timer, 0);
	pm8029_led_control(ldata);
	
}

static void led_alarm_handler(struct alarm *alarm)
{
	struct pm8029_led_data *ldata;

	ldata = container_of(alarm, struct pm8029_led_data, off_timer_alarm);
	LED_ALM("%s led alarm trigger +", ldata->ldev.name);
	queue_work(led_wq, &ldata->off_timer_work);
	
}

static int pm8029_led_probe(struct platform_device *pdev)
{
	struct pm8029_led_platform_data *pdata;
	struct pm8029_led_data *ldata;
	int i = 0, ret = -ENOMEM;

	pdata = pdev->dev.platform_data;
	if (pdata == NULL) {
		LED_ERR_LOG("%s: platform data is NULL\n", __func__);
		return -ENODEV;
	}

	ldata = kzalloc(sizeof(struct pm8029_led_data)
			* pdata->num_leds, GFP_KERNEL);
	if (!ldata && pdata->num_leds) {
		ret = -ENOMEM;
		LED_ERR_LOG("%s: failed on allocate ldata\n", __func__);
		goto err_exit;
	}

	dev_set_drvdata(&pdev->dev, ldata);
	led_wq = create_singlethread_workqueue("led_blink");
	if (!led_wq){
		LED_ERR_LOG("%s: failed on create workqueue!\n", __func__);
		goto err_create_work_queue;
	}


	
	for (i = 0; i < pdata->num_leds; i++) {
		ldata[i].ldev.name = pdata->led_config[i].name;
		ldata[i].bank = pdata->led_config[i].bank;
		ldata[i].init_pwm_brightness =  pdata->led_config[i].init_pwm_brightness;
		ldata[i].out_current =  pdata->led_config[i].out_current;
		ldata[i].flag = pdata->led_config[i].flag;
		atomic_set(&ldata[i].off_timer, 0);
		atomic_set(&ldata[i].brightness, 0);
		atomic_set(&ldata[i].blink, 0);

		ldata[i].ldev.brightness_set = pm8029_led_brightness_set;

		ret = led_classdev_register(&pdev->dev, &ldata[i].ldev);
		if (ret < 0) {
			LED_ERR_LOG("%s: failed on led_classdev_register [%s]\n",
				__func__, ldata[i].ldev.name);
			goto err_register_led_cdev;
		}

		if (ldata[i].bank <= PMIC8029_DRV4) {
			ret = device_create_file(ldata[i].ldev.dev, &dev_attr_currents);
				if (ret < 0) {
					LED_ERR_LOG("%s: Failed to create attr currents [%d]\n", __func__, i);
					goto err_register_attr_currents;
			}
		}
	}

	
	for (i = 0; i < pdata->num_leds; i++) {
		ret = device_create_file(ldata[i].ldev.dev, &dev_attr_blink);
		if (ret < 0){
			LED_ERR_LOG("%s: Failed to vreate attr blink [%d]\n",
					__func__, i);
			goto err_register_attr_blink;
		}
	}

	
        for (i = 0; i < pdata->num_leds; i++) {
                ret = device_create_file(ldata[i].ldev.dev, &dev_attr_off_timer);
                if (ret < 0){
                        LED_ERR_LOG("%s: Failed to vreate attr off_timer [%d]\n",
                                        __func__, i);
                        goto err_register_attr_off_timer;
                }
		INIT_DELAYED_WORK(&ldata[i].blink_work, led_do_blink);
		INIT_WORK(&ldata[i].off_timer_work, led_work_func);
		alarm_init(&ldata[i].off_timer_alarm,
			ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP,
			led_alarm_handler);
        }

	LED_INFO_LOG("%s: probe ok!\n",__func__);
	return 0;

err_register_attr_off_timer:
	for (i--; i >= 0; i--) {
                device_remove_file(ldata[i].ldev.dev, &dev_attr_off_timer);
        }

err_register_attr_blink:
	i = pdata->num_leds;
	for (i--; i >= 0; i--) {
		device_remove_file(ldata[i].ldev.dev, &dev_attr_blink);
	}
err_register_attr_currents:
	i = pdata->num_leds;
	for (i--; i >= 0; i--) {
		if (ldata[i].bank > PMIC8029_DRV4)
			continue;
		device_remove_file(ldata[i].ldev.dev, &dev_attr_currents);
	}
	i = pdata->num_leds;

err_exit:
err_register_led_cdev:
	for (i--; i >= 0; i--) {
		led_classdev_unregister(&ldata[i].ldev);
	}
	destroy_workqueue(led_wq);
err_create_work_queue:
	kfree(ldata);

	LED_ERR_LOG("%s: probe failed!\n",__func__);
	return ret;
}

static int __devexit pm8029_led_remove(struct platform_device *pdev)
{
	struct pm8029_led_platform_data *pdata;
	struct pm8029_led_data *ldata;
	int i;

	pdata = pdev->dev.platform_data;
	ldata = platform_get_drvdata(pdev);

	for (i = 0; i < pdata->num_leds; i++) {
		if (ldata[i].bank <= PMIC8029_DRV4)
			device_remove_file(ldata[i].ldev.dev,
					   &dev_attr_currents);
	}

	kfree(ldata);

	return 0;
}

static struct platform_driver pm8029_led_driver = {
	.probe = pm8029_led_probe,
	.remove = __devexit_p(pm8029_led_remove),
	.driver = {
		   .name = "leds-pm8029",
		   .owner = THIS_MODULE,
		   },
};

int __init pm8029_led_init(void)
{
	return platform_driver_register(&pm8029_led_driver);
}

void pm8029_led_exit(void)
{
	platform_driver_unregister(&pm8029_led_driver);
}

module_init(pm8029_led_init);
module_exit(pm8029_led_exit);

MODULE_DESCRIPTION("pm8029 led driver");
MODULE_LICENSE("GPL");
