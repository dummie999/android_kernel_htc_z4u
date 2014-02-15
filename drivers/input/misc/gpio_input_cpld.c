/* drivers/input/misc/gpio_input.c
 *
 * Copyright (C) 2007 Google, Inc.
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
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <mach/board.h>

#ifdef CONFIG_POWER_KEY_LED
#include <linux/leds-pm8058.h>
#include <linux/leds-max8957-lpg.h>

#define PWRKEYLEDON_DELAY 3*HZ
#define PWRKEYLEDOFF_DELAY 0
#define KTIME_TO_JIFFIES(kt) (((kt.tv64 * NSEC_CONVERSION) >> (NSEC_JIFFIE_SC - SEC_JIFFIE_SC)) >> SEC_JIFFIE_SC)

static int power_key_led_requested;
static int pre_power_key_status;
static int pre_power_key_led_status;
#endif


#ifdef CONFIG_CPLD_GPIO_KEY
#include <linux/i2c/cpld.h>
#define CPLD_GPIO_BASE						(NR_MSM_GPIOS+0x100)
#define KTIME_TO_JIFFIES(kt) (((kt.tv64 * NSEC_CONVERSION) >> (NSEC_JIFFIE_SC - SEC_JIFFIE_SC)) >> SEC_JIFFIE_SC)
static struct workqueue_struct *ki_queue;
#endif

static DEFINE_MUTEX(wakeup_mutex);
static unsigned char wakeup_bitmask;
static unsigned char set_wakeup;
static unsigned int vol_up_irq;
static unsigned int vol_down_irq;
static ssize_t vol_wakeup_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	unsigned char bitmask = 0;
	bitmask = simple_strtoull(buf, NULL, 10);
	mutex_lock(&wakeup_mutex);
	if (bitmask) {
		if (bitmask == 127)
			wakeup_bitmask &= bitmask;
		else if (bitmask > 128)
			wakeup_bitmask &= bitmask;
		else
			wakeup_bitmask |= bitmask;
	}

	if (wakeup_bitmask && (!set_wakeup)) {
		#ifdef CONFIG_CPLD_GPIO_KEY
		enable_cpld_keyirq_wake(vol_up_irq);
		#else
		enable_irq_wake(vol_up_irq);
		enable_irq_wake(vol_down_irq);
		#endif
		set_wakeup = 1;
		KEY_LOGI("%s:change to wake up function(%d, %d)\n", __func__, vol_up_irq, vol_down_irq);
	} else if ((!wakeup_bitmask) && set_wakeup) {
		#ifdef CONFIG_CPLD_GPIO_KEY
		disable_cpld_keyirq_wake(vol_up_irq);
		#else
		disable_irq_wake(vol_up_irq);
		disable_irq_wake(vol_down_irq);
		#endif
		set_wakeup = 0;
		KEY_LOGI("%s:change to non-wake up function(%d, %d)\n", __func__, vol_up_irq, vol_down_irq);
	}
	mutex_unlock(&wakeup_mutex);
	return count;
}
static ssize_t vol_wakeup_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%x\n", wakeup_bitmask);
}

static DEVICE_ATTR(vol_wakeup, 0664, vol_wakeup_show, vol_wakeup_store);

enum {
	DEBOUNCE_UNSTABLE     = BIT(0),	
	DEBOUNCE_PRESSED      = BIT(1),
	DEBOUNCE_NOTPRESSED   = BIT(2),
	DEBOUNCE_WAIT_IRQ     = BIT(3),	
	DEBOUNCE_POLL         = BIT(4),	

	DEBOUNCE_UNKNOWN =
		DEBOUNCE_PRESSED | DEBOUNCE_NOTPRESSED,
};

struct gpio_key_state {
	struct gpio_input_state *ds;
	uint8_t debounce;
#ifdef CONFIG_CPLD_GPIO_KEY
	uint8_t need_queue;
	uint8_t keyValue;
	uint8_t irq_enable_needed;
	struct delayed_work key_work;
#endif
};

struct gpio_input_state {
	int debug_log;
	struct gpio_event_input_devs *input_devs;
	const struct gpio_event_input_info *info;
#ifndef CONFIG_CPLD_GPIO_KEY
	struct hrtimer timer;
#endif
	int use_irq;
	int debounce_count;
	spinlock_t irq_lock;
	struct wake_lock wake_lock;
	struct gpio_key_state key_state[0];
};

#ifdef CONFIG_POWER_KEY_LED
static void power_key_led_on_work_func(struct work_struct *dummy)
{
	KEY_LOGI("[PWR] %s in (%x)\n", __func__, power_key_led_requested);
	if (power_key_led_requested == 1) {
		pre_power_key_led_status = 1;
		KEY_LOGI("[PWR] change power key led on\n");
		button_backlight_flash(1);
	}
}
static DECLARE_DELAYED_WORK(power_key_led_on_work, power_key_led_on_work_func);

static void power_key_led_off_work_func(struct work_struct *dummy)
{
	if (power_key_led_requested) {
		if (cancel_delayed_work_sync(&power_key_led_on_work)) {
			KEY_LOGI("[PWR] cancel power key led work successfully(%x)\n", power_key_led_requested);
		} else
			KEY_LOGI("[PWR] cancel power key led work unsuccessfully (%x)\n", power_key_led_requested);

		power_key_led_requested = 0;
	}
	if (pre_power_key_led_status == 1) {
		KEY_LOGI("[PWR] change power key led off\n");
		button_backlight_flash(0);
		pre_power_key_led_status = 0;
	}
}
static DECLARE_DELAYED_WORK(power_key_led_off_work, power_key_led_off_work_func);

static void handle_power_key_led(unsigned int code, int value)
{
	if (code == KEY_POWER) {
		if (pre_power_key_status == value)
			return;
		pre_power_key_status = value;
		if (value) {
			KEY_LOGI("[PWR] start count for power key led on\n");
			schedule_delayed_work(&power_key_led_on_work, PWRKEYLEDON_DELAY);
			power_key_led_requested = 1;
		} else {
			KEY_LOGI("[PWR] start count for power key led off\n");
			schedule_delayed_work(&power_key_led_off_work, PWRKEYLEDOFF_DELAY);
		}
	}
}
#endif

static inline void print_key_ds(struct gpio_input_state *ds)
{
	int i;
	int nkeys = ds->info->keymap_size;
	const struct gpio_event_direct_entry *key_entry;
	struct gpio_key_state *key_state;

	key_entry = ds->info->keymap;
	key_state = ds->key_state;
	for (i = 0; i < nkeys; i++, key_entry++, key_state++) {
		KEY_LOGD("0x%X key state: ", key_entry->code);
		if (key_state->debounce & DEBOUNCE_UNSTABLE)
			KEY_LOGD(" | DEBOUNCE_UNSTABLE");
		if (key_state->debounce & DEBOUNCE_PRESSED)
			KEY_LOGD(" | DEBOUNCE_PRESSED");
		if (key_state->debounce & DEBOUNCE_NOTPRESSED)
			KEY_LOGD(" | DEBOUNCE_NOTPRESSED");
		if (key_state->debounce & DEBOUNCE_WAIT_IRQ)
			KEY_LOGD(" | DEBOUNCE_WAIT_IRQ");
		if (key_state->debounce & DEBOUNCE_POLL)
			KEY_LOGD(" | DEBOUNCE_POLL");
	}
}

#ifndef CONFIG_CPLD_GPIO_KEY
static enum hrtimer_restart gpio_event_input_timer_func(struct hrtimer *timer)
{
	int i;
	int pressed;
	struct gpio_input_state *ds =
		container_of(timer, struct gpio_input_state, timer);
	unsigned gpio_flags = ds->info->flags;
	unsigned npolarity;
	int nkeys = ds->info->keymap_size;
	const struct gpio_event_direct_entry *key_entry;
	struct gpio_key_state *key_state;
	unsigned long irqflags;
	uint8_t debounce;
	bool sync_needed;

#if 0
	key_entry = kp->keys_info->keymap;
	key_state = kp->key_state;
	for (i = 0; i < nkeys; i++, key_entry++, key_state++)
		pr_info("gpio_read_detect_status %d %d\n", key_entry->gpio,
			gpio_read_detect_status(key_entry->gpio));
#endif
#ifdef KEYPAD_DEBUG
	KEY_LOGD("Before process: \n");
	print_key_ds(ds);
#endif

	key_entry = ds->info->keymap;
	key_state = ds->key_state;
	sync_needed = false;
	spin_lock_irqsave(&ds->irq_lock, irqflags);
	for (i = 0; i < nkeys; i++, key_entry++, key_state++) {
		debounce = key_state->debounce;
		if (debounce & DEBOUNCE_WAIT_IRQ)
			continue;
		if (key_state->debounce & DEBOUNCE_UNSTABLE) {
			debounce = key_state->debounce = DEBOUNCE_UNKNOWN;
			enable_irq(gpio_to_irq(key_entry->gpio));
			if (gpio_flags & GPIOEDF_PRINT_KEY_UNSTABLE)
				KEY_LOGI("gpio_keys_scan_keys: key %x-%x, %d "
					"(%d) continue debounce,"
#ifdef KEYPAD_DEBUG
					" enable irq#%d\n"
#endif
					, ds->info->type, key_entry->code,
					i, key_entry->gpio
#ifdef KEYPAD_DEBUG
					, gpio_to_irq(key_entry->gpio)
#endif
					);
		}
		npolarity = !(gpio_flags & GPIOEDF_ACTIVE_HIGH);
		pressed = gpio_get_value(key_entry->gpio) ^ npolarity;
		if (debounce & DEBOUNCE_POLL) {
			if (pressed == !(debounce & DEBOUNCE_PRESSED)) {
				ds->debounce_count++;
				key_state->debounce = DEBOUNCE_UNKNOWN;
				if (gpio_flags & GPIOEDF_PRINT_KEY_DEBOUNCE)
					KEY_LOGI("gpio_keys_scan_keys: key %x-"
						"%x, %d (%d) start debounce\n",
						ds->info->type, key_entry->code,
						i, key_entry->gpio);
			}
			continue;
		}
		if (pressed && (debounce & DEBOUNCE_NOTPRESSED)) {
			if (gpio_flags & GPIOEDF_PRINT_KEY_DEBOUNCE)
				KEY_LOGI("gpio_keys_scan_keys: key %x-%x, %d "
					"(%d) debounce pressed 1\n",
					ds->info->type, key_entry->code,
					i, key_entry->gpio);
			key_state->debounce = DEBOUNCE_PRESSED;
			continue;
		}
		if (!pressed && (debounce & DEBOUNCE_PRESSED)) {
			if (gpio_flags & GPIOEDF_PRINT_KEY_DEBOUNCE)
				KEY_LOGI("gpio_keys_scan_keys: key %x-%x, %d "
					"(%d) debounce pressed 0\n",
					ds->info->type, key_entry->code,
					i, key_entry->gpio);
			key_state->debounce = DEBOUNCE_NOTPRESSED;
			continue;
		}
		
		ds->debounce_count--;
		if (ds->use_irq)
			key_state->debounce |= DEBOUNCE_WAIT_IRQ;
		else
			key_state->debounce |= DEBOUNCE_POLL;
		if (gpio_flags & GPIOEDF_PRINT_KEYS)
			KEY_LOGI("gpio_keys_scan_keys: key %x-%x, %d (%d) "
				"changed to %d\n", ds->info->type,
				key_entry->code, i, key_entry->gpio, pressed);
#ifdef CONFIG_POWER_KEY_LED
		handle_power_key_led(key_entry->code, pressed);
#endif
		input_event(ds->input_devs->dev[key_entry->dev], ds->info->type,
			    key_entry->code, pressed);
		sync_needed = true;
	}
	if (sync_needed) {
		for (i = 0; i < ds->input_devs->count; i++)
			input_sync(ds->input_devs->dev[i]);
	}

#if 0
	key_entry = kp->keys_info->keymap;
	key_state = kp->key_state;
	for (i = 0; i < nkeys; i++, key_entry++, key_state++) {
		pr_info("gpio_read_detect_status %d %d\n", key_entry->gpio,
			gpio_read_detect_status(key_entry->gpio));
	}
#endif

	if (ds->debounce_count)
		hrtimer_start(timer, ds->info->debounce_time, HRTIMER_MODE_REL);
	else if (!ds->use_irq)
		hrtimer_start(timer, ds->info->poll_time, HRTIMER_MODE_REL);
	else
		wake_unlock(&ds->wake_lock);

	spin_unlock_irqrestore(&ds->irq_lock, irqflags);
#ifdef KEYPAD_DEBUG
	KEY_LOGD("After process:\n");
	print_key_ds(ds);
#endif
	return HRTIMER_NORESTART;
}
#else
static void gpio_event_input_delayed_func(struct work_struct *key_work)
{
	int i;
	int pressed;
	struct gpio_input_state *ds =
		container_of((struct delayed_work *)key_work, struct gpio_key_state, key_work)->ds;
	unsigned npolarity;
	unsigned gpio_flags = ds->info->flags;
	int nkeys = ds->info->keymap_size;
	const struct gpio_event_direct_entry *key_entry;
	struct gpio_key_state *key_state;
	unsigned long irqflags;
	uint8_t debounce;
	bool sync_needed;

	key_entry = ds->info->keymap;
	key_state = ds->key_state;
	npolarity = !(gpio_flags & GPIOEDF_ACTIVE_HIGH);
	for (i = 0; i < nkeys; i++, key_entry++, key_state++) {
		#ifdef CONFIG_CPLD_GPIO_KEY
		if(key_entry->gpio > CPLD_GPIO_BASE)
			key_state->keyValue = cpld_gpio_read((key_entry->gpio)-CPLD_GPIO_BASE) ^ npolarity;
		else
			key_state->keyValue = gpio_get_value(key_entry->gpio) ^ npolarity;
		#else
		key_state->keyValue = gpio_get_value(key_entry->gpio) ^ npolarity;
		#endif
		key_state->irq_enable_needed = 0;
		key_state->need_queue = 0;
	}
#ifdef KEYPAD_DEBUG
	KEY_LOGD("Before process: \n");
	print_key_ds(ds);
#endif
	key_entry = ds->info->keymap;
	key_state = ds->key_state;
	sync_needed = false;
	spin_lock_irqsave(&ds->irq_lock, irqflags);
	for (i = 0; i < nkeys; i++, key_entry++, key_state++) {
		debounce = key_state->debounce;
		if (debounce & DEBOUNCE_WAIT_IRQ)
			continue;
		if (key_state->debounce & DEBOUNCE_UNSTABLE) {
			debounce = key_state->debounce = DEBOUNCE_UNKNOWN;
			key_state->irq_enable_needed = 1;
			if (gpio_flags & GPIOEDF_PRINT_KEY_UNSTABLE)
				KEY_LOGD("gpio_keys_scan_keys: key %x-%x, %d "
					"(%d) continue debounce,"
#ifdef KEYPAD_DEBUG
					" enable irq#%d\n"
#endif
					, ds->info->type, key_entry->code,
					i, key_entry->gpio
#ifdef KEYPAD_DEBUG
					, gpio_to_irq(key_entry->gpio)
#endif
					);
		}
		pressed = key_state->keyValue;

		if (debounce & DEBOUNCE_POLL) {
			if (pressed == !(debounce & DEBOUNCE_PRESSED)) {
				ds->debounce_count++;
				key_state->need_queue = 1;
				key_state->debounce = DEBOUNCE_UNKNOWN;
				if (gpio_flags & GPIOEDF_PRINT_KEY_DEBOUNCE)
					KEY_LOGD("gpio_keys_scan_keys: key %x-"
						"%x, %d (%d) start debounce\n",
						ds->info->type, key_entry->code,
						i, key_entry->gpio);
			}
			continue;
		}
		if (pressed && (debounce & DEBOUNCE_NOTPRESSED)) {
			if (gpio_flags & GPIOEDF_PRINT_KEY_DEBOUNCE)
				KEY_LOGD("gpio_keys_scan_keys: key %x-%x, %d "
					"(%d) debounce pressed 1\n",
					ds->info->type, key_entry->code,
					i, key_entry->gpio);
			key_state->debounce = DEBOUNCE_PRESSED;
			key_state->need_queue = 1;
			continue;
		}
		if (!pressed && (debounce & DEBOUNCE_PRESSED)) {
			if (gpio_flags & GPIOEDF_PRINT_KEY_DEBOUNCE)
				KEY_LOGD("gpio_keys_scan_keys: key %x-%x, %d "
					"(%d) debounce pressed 0\n",
					ds->info->type, key_entry->code,
					i, key_entry->gpio);
			key_state->debounce = DEBOUNCE_NOTPRESSED;
			key_state->need_queue = 1;
			continue;
		}
		
		ds->debounce_count--;
		if (ds->use_irq)
			key_state->debounce |= DEBOUNCE_WAIT_IRQ;
		else
			key_state->debounce |= DEBOUNCE_POLL;
		if (gpio_flags & GPIOEDF_PRINT_KEYS)
			KEY_LOGD("gpio_keys_scan_keys: key %x-%x, %d (%d) "
				"changed to %d\n", ds->info->type,
				key_entry->code, i, key_entry->gpio, pressed);
#ifdef CONFIG_POWER_KEY_LED
		handle_power_key_led(key_entry->code, pressed);
#endif
		input_event(ds->input_devs->dev[key_entry->dev], ds->info->type,
			    key_entry->code, pressed);
		sync_needed = true;
	}
	if (sync_needed) {
		for (i = 0; i < ds->input_devs->count; i++)
			input_sync(ds->input_devs->dev[i]);
	}

#if 0
	key_entry = kp->keys_info->keymap;
	key_state = kp->key_state;
	for (i = 0; i < nkeys; i++, key_entry++, key_state++) {
		pr_info("gpio_read_detect_status %d %d\n", key_entry->gpio,
			gpio_read_detect_status(key_entry->gpio));
	}
#endif
	if (ds->debounce_count) {
		key_entry = ds->info->keymap;
		key_state = ds->key_state;
		for (i = 0; i < nkeys; i++, key_entry++, key_state++) {
			if (key_state->need_queue)
				if (!queue_delayed_work(ki_queue, &(key_state->key_work), KTIME_TO_JIFFIES(ds->info->debounce_time)))
					KEY_LOGI("Keycode: 0x%X has been queued already.\n", key_entry->code);
		}
	} else if (!ds->use_irq) {
		key_entry = ds->info->keymap;
		key_state = ds->key_state;
		for (i = 0; i < nkeys; i++, key_entry++, key_state++) {
			if (key_state->need_queue)
				if (!queue_delayed_work(ki_queue, &(key_state->key_work), KTIME_TO_JIFFIES(ds->info->poll_time)))
					KEY_LOGI("Keycode: 0x%X has been queued already.\n", key_entry->code);
		}
	} else
		wake_unlock(&ds->wake_lock);

	spin_unlock_irqrestore(&ds->irq_lock, irqflags);

#if 0
	key_entry = ds->info->keymap;
	key_state = ds->key_state;
	for (i = 0; i < nkeys; i++, key_entry++, key_state++)
		if (key_state->irq_enable_needed)
			enable_irq(gpio_to_irq(key_entry->gpio));
#endif

#ifdef KEYPAD_DEBUG
	KEY_LOGD("After process:\n");
	print_key_ds(ds);
#endif
}
#endif

static irqreturn_t gpio_event_input_irq_handler(int irq, void *dev_id)
{
	struct gpio_key_state *ks = dev_id;
	struct gpio_input_state *ds = ks->ds;
	int keymap_index = ks - ds->key_state;
	const struct gpio_event_direct_entry *key_entry;
	unsigned long irqflags;
	int pressed;

	KEY_LOGI("%s, irq=%d, use_irq=%d, keycode=0x%X\n", __func__, irq, ds->use_irq, ds->info->keymap[keymap_index].code);

	if (!ds->use_irq)
		return IRQ_HANDLED;

	key_entry = &ds->info->keymap[keymap_index];

	if (ds->info->debounce_time.tv64) {
		spin_lock_irqsave(&ds->irq_lock, irqflags);
		if (ks->debounce & DEBOUNCE_WAIT_IRQ) {
			ks->debounce = DEBOUNCE_UNKNOWN;
			if (ds->debounce_count++ == 0) {
				wake_lock(&ds->wake_lock);
#ifndef CONFIG_CPLD_GPIO_KEY
				hrtimer_start(
					&ds->timer, ds->info->debounce_time,
					HRTIMER_MODE_REL);
#else
				if (!queue_delayed_work(ki_queue, &(ks->key_work), KTIME_TO_JIFFIES(ds->info->debounce_time)))
					KEY_LOGI("Keycode: 0x%X has been queued already.\n", key_entry->code);
#endif
			}
			if (ds->info->flags & GPIOEDF_PRINT_KEY_DEBOUNCE)
				KEY_LOGD("gpio_event_input_irq_handler: "
					"key %x-%x, %d (%d) start debounce\n",
					ds->info->type, key_entry->code,
					keymap_index, key_entry->gpio);
		} else {
#ifndef CONFIG_CPLD_GPIO_KEY
			disable_irq_nosync(irq);
#endif
#ifdef KEYPAD_DEBUG
			if (ds->info->flags & GPIOEDF_PRINT_KEY_UNSTABLE)
				KEY_LOGD("Disable IRQ because of bouncing.");
#endif
			ks->debounce = DEBOUNCE_UNSTABLE;
		}
		spin_unlock_irqrestore(&ds->irq_lock, irqflags);
	} else {
		#ifdef CONFIG_CPLD_GPIO_KEY
		if(key_entry->gpio > CPLD_GPIO_BASE)
			pressed = cpld_gpio_read((key_entry->gpio)-CPLD_GPIO_BASE) ^
			!(ds->info->flags & GPIOEDF_ACTIVE_HIGH);
		else
			pressed = gpio_get_value(key_entry->gpio) ^
			!(ds->info->flags & GPIOEDF_ACTIVE_HIGH);
		#else
		pressed = gpio_get_value(key_entry->gpio) ^
			!(ds->info->flags & GPIOEDF_ACTIVE_HIGH);
		#endif
		if (ds->info->flags & GPIOEDF_PRINT_KEYS)
			KEY_LOGI("gpio_event_input_irq_handler: key %x-%x, %d "
				"(%d) changed to %d\n",
				ds->info->type, key_entry->code, keymap_index,
				key_entry->gpio, pressed);
		input_event(ds->input_devs->dev[key_entry->dev], ds->info->type,
			    key_entry->code, pressed);
		input_sync(ds->input_devs->dev[key_entry->dev]);
	}
	return IRQ_HANDLED;
}
#ifdef CONFIG_CPLD_GPIO_KEY

static void cpld_gpio_event_input_irq_handler(unsigned long dev_id)
{
	int irq=0xA00;
	void *dev;
	dev=(void*)dev_id;
	gpio_event_input_irq_handler(irq,dev);
}


static int gpio_event_input_request_irqs(struct gpio_input_state *ds)
{
	int i;
	int err;
	unsigned int irq;
	unsigned long req_flags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;

	for (i = 0; i < ds->info->keymap_size; i++) {
		if (ds->info->keymap[i].code == KEY_POWER)
			{
				err = irq = gpio_to_irq(ds->info->keymap[i].gpio);
				if (err < 0)
					goto err_gpio_get_irq_num_failed;
				err = request_any_context_irq(irq, gpio_event_input_irq_handler,
						  req_flags, "gpio_keys", &ds->key_state[i]);
				if (err < 0) {
					KEY_LOGE("KEY_ERR: %s: request_irq "
						"failed for input %d, irq %d, err %d\n", __func__,
						ds->info->keymap[i].gpio, irq, err);
					goto err_request_irq_failed;
				}
			}
		else
			{
				if (ds->info->keymap[i].code == KEY_VOLUMEUP)
					irq=CPLD_IRQ_VOL_UP;
				else
					irq=CPLD_IRQ_VOL_DW;
				err = cpld_request_irq(irq, &cpld_gpio_event_input_irq_handler,
						  &ds->key_state[i]);
				if (err < 0) {
					KEY_LOGE("KEY_ERR: %s: cpld_request_irq "
						"failed for input %d, irq %d, err %d\n", __func__,
						ds->info->keymap[i].gpio, irq, err);
					goto err_request_irq_failed;
				}
			}
		if (ds->info->keymap[i].code == KEY_VOLUMEUP ||
			ds->info->keymap[i].code == KEY_VOLUMEDOWN) {
			KEY_LOGI("keycode = %d, gpio = %d, irq = %d", ds->info->keymap[i].code, ds->info->keymap[i].gpio, irq);
			if (ds->info->keymap[i].code == KEY_VOLUMEUP)
				vol_up_irq = irq;
			else
				vol_down_irq = irq;
		} else
			enable_irq_wake(irq);
	}
	return 0;

	for (i = ds->info->keymap_size - 1; i >= 0; i--) {
		free_irq(gpio_to_irq(ds->info->keymap[i].gpio),
			 &ds->key_state[i]);
err_request_irq_failed:
err_gpio_get_irq_num_failed:
		;
	}
	return err;
}
#else
static int gpio_event_input_request_irqs(struct gpio_input_state *ds)
{
	int i;
	int err;
	unsigned int irq;
	unsigned long req_flags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;

	for (i = 0; i < ds->info->keymap_size; i++) {
		err = irq = gpio_to_irq(ds->info->keymap[i].gpio);
		if (err < 0)
			goto err_gpio_get_irq_num_failed;
		err = request_any_context_irq(irq, gpio_event_input_irq_handler,
				  req_flags, "gpio_keys", &ds->key_state[i]);
		if (err < 0) {
			KEY_LOGE("KEY_ERR: %s: request_irq "
				"failed for input %d, irq %d, err %d\n", __func__,
				ds->info->keymap[i].gpio, irq, err);
			goto err_request_irq_failed;
		}
		if (ds->info->keymap[i].code == KEY_VOLUMEUP ||
			ds->info->keymap[i].code == KEY_VOLUMEDOWN) {
			KEY_LOGI("keycode = %d, gpio = %d, irq = %d", ds->info->keymap[i].code, ds->info->keymap[i].gpio, irq);
			if (ds->info->keymap[i].code == KEY_VOLUMEUP)
				vol_up_irq = irq;
			else
				vol_down_irq = irq;
		} else
			enable_irq_wake(irq);
	}
	return 0;

	for (i = ds->info->keymap_size - 1; i >= 0; i--) {
		free_irq(gpio_to_irq(ds->info->keymap[i].gpio),
			 &ds->key_state[i]);
err_request_irq_failed:
err_gpio_get_irq_num_failed:
		;
	}
	return err;
}
#endif
int gpio_event_input_func(struct gpio_event_input_devs *input_devs,
			struct gpio_event_info *info, void **data, int func)
{
	int ret;
	int i;
	unsigned long irqflags;
	struct gpio_event_input_info *di;
	struct gpio_input_state *ds = *data;
	struct kobject *keyboard_kobj;

	di = container_of(info, struct gpio_event_input_info, info);

	if (func == GPIO_EVENT_FUNC_SUSPEND) {
		if (ds->use_irq)
			for (i = 0; i < di->keymap_size; i++)
				disable_irq(gpio_to_irq(di->keymap[i].gpio));
#ifndef CONFIG_CPLD_GPIO_KEY
		hrtimer_cancel(&ds->timer);
#else
		for (i = 0; i < di->keymap_size; i++) {
			if (cancel_delayed_work_sync(&(ds->key_state[i].key_work)))
				KEY_LOGI("Cancel delayed debounce function of keycode:0x%X success.\n", ds->info->keymap[i].code);
			else
				KEY_LOGI("Cancel delayed debounce function of keycode:0x%X fail.\n", ds->info->keymap[i].code);
		}
#endif
		return 0;
	}
	if (func == GPIO_EVENT_FUNC_RESUME) {
		spin_lock_irqsave(&ds->irq_lock, irqflags);
		if (ds->use_irq)
			for (i = 0; i < di->keymap_size; i++)
				enable_irq(gpio_to_irq(di->keymap[i].gpio));
#ifndef CONFIG_CPLD_GPIO_KEY
		hrtimer_start(&ds->timer, ktime_set(0, 0), HRTIMER_MODE_REL);
#else
		queue_delayed_work(ki_queue, &(ds->key_state[0].key_work), 0);
#endif
		spin_unlock_irqrestore(&ds->irq_lock, irqflags);
		return 0;
	}

	if (func == GPIO_EVENT_FUNC_INIT) {
		if (ktime_to_ns(di->poll_time) <= 0)
			di->poll_time = ktime_set(0, 20 * NSEC_PER_MSEC);

		*data = ds = kzalloc(sizeof(*ds) + sizeof(ds->key_state[0]) *
					di->keymap_size, GFP_KERNEL);
		if (ds == NULL) {
			ret = -ENOMEM;
			KEY_LOGE("KEY_ERR: %s: "
				"Failed to allocate private data\n", __func__);
			goto err_ds_alloc_failed;
		}
		ds->debounce_count = di->keymap_size;
		ds->input_devs = input_devs;
		ds->info = di;
		wake_lock_init(&ds->wake_lock, WAKE_LOCK_SUSPEND, "gpio_input");
		spin_lock_init(&ds->irq_lock);
			ds->debug_log = 1;

		for (i = 0; i < di->keymap_size; i++) {
			int dev = di->keymap[i].dev;
			if (dev >= input_devs->count) {
				KEY_LOGE("KEY_ERR: %s: bad device "
					"index %d >= %d for key code %d\n",
					__func__, dev, input_devs->count,
					di->keymap[i].code);
				ret = -EINVAL;
				goto err_bad_keymap;
			}
			input_set_capability(input_devs->dev[dev], di->type,
					     di->keymap[i].code);
			ds->key_state[i].ds = ds;
			ds->key_state[i].debounce = DEBOUNCE_UNKNOWN;
		}

		for (i = 0; i < di->keymap_size; i++) {
			#ifdef CONFIG_CPLD_GPIO_KEY
			KEY_LOGI("di->keymap[%d].gpio=%d\n",i,di->keymap[i].gpio);
			if(di->keymap[i].gpio < CPLD_GPIO_BASE)
				{
			#endif
					ret = gpio_request(di->keymap[i].gpio, "gpio_kp_in");
					if (ret) {
						KEY_LOGE("KEY_ERR: %s: gpio_request "
							"failed for %d\n", __func__, di->keymap[i].gpio);
						goto err_gpio_request_failed;
					}
					ret = gpio_direction_input(di->keymap[i].gpio);
					if (ret) {
						KEY_LOGE("KEY_ERR: %s: "
							"gpio_direction_input failed for %d\n",
							__func__, di->keymap[i].gpio);
						goto err_gpio_configure_failed;
					}
			#ifdef CONFIG_CPLD_GPIO_KEY
				}
			#endif
		}

		keyboard_kobj = kobject_create_and_add("keyboard", NULL);
		if (keyboard_kobj == NULL) {
			KEY_LOGE("KEY_ERR: %s: subsystem_register failed\n", __func__);
			ret = -ENOMEM;
			return ret;
		}
		if (sysfs_create_file(keyboard_kobj, &dev_attr_vol_wakeup.attr))
			KEY_LOGE("KEY_ERR: %s: sysfs_create_file "
					"return %d\n", __func__, ret);
		wakeup_bitmask = 0;
		set_wakeup = 0;

		if (di->setup_input_gpio)
			di->setup_input_gpio();
#ifdef CONFIG_CPLD_GPIO_KEY
		ki_queue = create_singlethread_workqueue("ki_queue");
#endif

		ret = gpio_event_input_request_irqs(ds);

		spin_lock_irqsave(&ds->irq_lock, irqflags);
		ds->use_irq = ret == 0;

		KEY_LOGI("GPIO Input Driver: Start gpio inputs for %s%s in %s "
			"mode\n", input_devs->dev[0]->name,
			(input_devs->count > 1) ? "..." : "",
			ret == 0 ? "interrupt" : "polling");

#ifndef CONFIG_CPLD_GPIO_KEY
		hrtimer_init(&ds->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ds->timer.function = gpio_event_input_timer_func;
		hrtimer_start(&ds->timer, ktime_set(0, 0), HRTIMER_MODE_REL);
#else
		for (i = 0; i < di->keymap_size; i++)
			INIT_DELAYED_WORK(&(ds->key_state[i].key_work), gpio_event_input_delayed_func);

		queue_delayed_work(ki_queue, &(ds->key_state[0].key_work), 0);
#endif
		spin_unlock_irqrestore(&ds->irq_lock, irqflags);
		return 0;
	}

	ret = 0;
	spin_lock_irqsave(&ds->irq_lock, irqflags);
#ifndef CONFIG_CPLD_GPIO_KEY
	hrtimer_cancel(&ds->timer);
#else
	for (i = 0; i < di->keymap_size; i++) {
		if (cancel_delayed_work_sync(&(ds->key_state[i].key_work)))
			KEY_LOGI("Cancel delayed debounce function of keycode:0x%X success.\n", ds->info->keymap[i].code);
		else
			KEY_LOGI("Cancel delayed debounce function of keycode:0x%X fail.\n", ds->info->keymap[i].code);
	}
#endif
	if (ds->use_irq) {
		for (i = di->keymap_size - 1; i >= 0; i--) {
			free_irq(gpio_to_irq(di->keymap[i].gpio),
				 &ds->key_state[i]);
		}
	}
	spin_unlock_irqrestore(&ds->irq_lock, irqflags);

	for (i = di->keymap_size - 1; i >= 0; i--) {
err_gpio_configure_failed:
		gpio_free(di->keymap[i].gpio);
err_gpio_request_failed:
		;
	}
err_bad_keymap:
	wake_lock_destroy(&ds->wake_lock);
	kfree(ds);
err_ds_alloc_failed:
	return ret;
}
