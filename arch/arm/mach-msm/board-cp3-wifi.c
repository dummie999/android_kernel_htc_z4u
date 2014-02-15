#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/skbuff.h>
#include <linux/gpio.h>
#include "linux/i2c/cpld.h"


#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>

#define ATH6KL_BOARD_DATA_FILE_SIZE   (4 * 1024)
#define ATH6K_FW_FILE_SIZE 	(124 * 1024)
#define ATH6K_TEST_FW_FILE_SIZE (48 * 1024)

#define ATH6K_FW_NUM 3

struct ath6kl_prealloc_mem_struct {
	void *mem_ptr;
	unsigned long size;
};

static struct ath6kl_prealloc_mem_struct ath6kl_fw_buf[ATH6K_FW_NUM] = {
	{ NULL, (ATH6KL_BOARD_DATA_FILE_SIZE) },
	{ NULL, (ATH6K_FW_FILE_SIZE) },
	{ NULL, (ATH6K_TEST_FW_FILE_SIZE) },
};

void *ath6kl_get_prealloc_fw_buf(u8 fw_type)
{
	memset(ath6kl_fw_buf[fw_type].mem_ptr, 0, ath6kl_fw_buf[fw_type].size);
	return ath6kl_fw_buf[fw_type].mem_ptr;
}
EXPORT_SYMBOL(ath6kl_get_prealloc_fw_buf);

static int ath6kl_prealloc_mem(void)
{
	int i;
	printk("%s\n",__func__);
	for(i = 0 ; i < ATH6K_FW_NUM; i++) {
		ath6kl_fw_buf[i].mem_ptr = kmalloc(ath6kl_fw_buf[i].size,
				GFP_KERNEL);
		if (ath6kl_fw_buf[i].mem_ptr == NULL)
			return -ENOMEM;
	}
	return 0;
}

static uint32_t wifi_on_gpio_table[] = {
	GPIO_CFG(64, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA), 
	GPIO_CFG(65, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA), 
	GPIO_CFG(66, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA), 
	GPIO_CFG(67, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA), 
	GPIO_CFG(63, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA), 
	GPIO_CFG(62, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_10MA), 
	GPIO_CFG(38, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 
	
	
};

static uint32_t wifi_off_gpio_table[] = {
	GPIO_CFG(64, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 
	GPIO_CFG(65, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 
	GPIO_CFG(66, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 
	GPIO_CFG(67, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 
	GPIO_CFG(63, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 
	GPIO_CFG(62, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 
	GPIO_CFG(38, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 
	
	
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("[WLAN] %s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

int atheros_wifi_power(bool on)
{
	int ret = 0;
	printk(KERN_INFO "[WLAN] %s: %d\n", __func__, on);

	if (on) {
		config_gpio_table(wifi_on_gpio_table, ARRAY_SIZE(wifi_on_gpio_table));
		mdelay(200);
		
		ret = cpld_gpio_write(CPLD_EXT_GPIO_WIFI_SHUTDOWN, 1);
		if (ret < 0)
			printk(KERN_INFO "[WLAN] %s write error CPLD_EXT_GPIO_WIFI_SHUTDOWN\n", __func__);
	} else {
		
		ret = cpld_gpio_write(CPLD_EXT_GPIO_WIFI_SHUTDOWN, 0);
		if (ret < 0)
			printk(KERN_INFO "[WLAN] %s write error CPLD_EXT_GPIO_WIFI_SHUTDOWN\n", __func__);
		mdelay(1);
		config_gpio_table(wifi_off_gpio_table, ARRAY_SIZE(wifi_off_gpio_table));
		mdelay(10);
	}
	mdelay(250);
	return 0;
}
EXPORT_SYMBOL(atheros_wifi_power);

int __init cp3_wifi_init(void)
{
	int ret = 0;

	printk(KERN_INFO "[WLAN] %s\n", __func__);

	ath6kl_prealloc_mem();

	config_gpio_table(wifi_off_gpio_table, ARRAY_SIZE(wifi_off_gpio_table));

#ifdef ATH_OOB_IRQ
	ret= gpio_request(GPIO_WIFI_IRQ, "WLAN_IRQ");
	if (ret) {
		printk(KERN_INFO "[WLAN] %s: gpio_request is failed!!\n", __func__);
		return ret;
	}
	ret = gpio_direction_output(GPIO_WIFI_IRQ, 0);
	if (ret) {
		printk(KERN_INFO "[WLAN] %s: gpio_direction_output is failed\n", __func__);
	}
	gpio_free(GPIO_WIFI_IRQ);
#endif
	return ret;
}
