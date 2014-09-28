/* linux/arch/arm/mach-msm/board-bcm4330-wifi.c
*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/skbuff.h>
#include <linux/wifi_tiwlan.h>
#include <mach/TCA6418_ioextender.h>
#include <linux/module.h>
#include "board-bcm4330-wifi.h"

int bcm4330_wifi_set_carddetect(int val);
int bcm4330_wifi_get_mac_addr(unsigned char *buf);

#define PREALLOC_WLAN_NUMBER_OF_SECTIONS	4
#define PREALLOC_WLAN_NUMBER_OF_BUFFERS		160
#define PREALLOC_WLAN_SECTION_HEADER		24

#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 128)
#define WLAN_SECTION_SIZE_1	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 128)
#define WLAN_SECTION_SIZE_2	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 512)
#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 1024)

#define WLAN_SKB_BUF_NUM	16

/*#define HW_OOB 1*/

static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];

typedef struct wifi_mem_prealloc_struct {
	void *mem_ptr;
	unsigned long size;
} wifi_mem_prealloc_t;

static wifi_mem_prealloc_t wifi_mem_array[PREALLOC_WLAN_NUMBER_OF_SECTIONS] = {
	{ NULL, (WLAN_SECTION_SIZE_0 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_1 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_2 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_3 + PREALLOC_WLAN_SECTION_HEADER) }
};

static void *bcm4330_wifi_mem_prealloc(int section, unsigned long size)
{
	printk("wifi: prealloc buffer index: %d\n", section);
	if (section == PREALLOC_WLAN_NUMBER_OF_SECTIONS)
		return wlan_static_skb;
	if ((section < 0) || (section > PREALLOC_WLAN_NUMBER_OF_SECTIONS))
		return NULL;
	if (wifi_mem_array[section].size < size)
		return NULL;
	return wifi_mem_array[section].mem_ptr;
}

int __init bcm4330_init_wifi_mem(void)
{
	int i;

	for (i = 0 ; (i < WLAN_SKB_BUF_NUM) ; i++) {
		if (i < (WLAN_SKB_BUF_NUM/2))
			wlan_static_skb[i] = dev_alloc_skb(PAGE_SIZE*2);
		else
			wlan_static_skb[i] = dev_alloc_skb(PAGE_SIZE*4);
	}
	for (i = 0 ; (i < PREALLOC_WLAN_NUMBER_OF_SECTIONS) ; i++) {
		wifi_mem_array[i].mem_ptr = kmalloc(wifi_mem_array[i].size, GFP_KERNEL);
		if (wifi_mem_array[i].mem_ptr == NULL)
			return -ENOMEM;
	}
	return 0;
}

static uint32_t wifi_on_gpio_table[] = {
	GPIO_CFG(64, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA), /* DAT3 */
	GPIO_CFG(65, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA), /* DAT2 */
	GPIO_CFG(66, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA), /* DAT1 */
	GPIO_CFG(67, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA), /* DAT0 */
	GPIO_CFG(63, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA), /* CMD */
	GPIO_CFG(62, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), /* CLK */
	GPIO_CFG(29, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA), /* WLAN IRQ */
};

static uint32_t wifi_off_gpio_table[] = {
	GPIO_CFG(64, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), /* DAT3 */
	GPIO_CFG(65, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), /* DAT2 */
	GPIO_CFG(66, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), /* DAT1 */
	GPIO_CFG(67, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), /* DAT0 */
	GPIO_CFG(63, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), /* CMD */
	GPIO_CFG(62, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* CLK */
	GPIO_CFG(29, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_4MA), /* WLAN IRQ */
};

static void config_gpio_table(uint32_t *table, int len)
{
		int n, rc;
		for (n = 0; n < len; n++) {
				rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
				if (rc) {
						pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
								__func__, table[n], rc);
						break;
				}
		}
}

int bcm4330_wifi_power(int on)
{
	printk(KERN_INFO "[WLAN]%s: %d\n", __func__, on);

	if (on) {
		config_gpio_table(wifi_on_gpio_table,
				ARRAY_SIZE(wifi_on_gpio_table));
	} else {
		config_gpio_table(wifi_off_gpio_table,
				ARRAY_SIZE(wifi_off_gpio_table));
	}

	if(machine_is_magnids()) {
		/* magnids_wifi_bt_sleep_clk_ctl(on, ID_WIFI); */
		ioext_gpio_set_value(8, on); /* WIFI_SHUTDOWN */
	} else {
		gpio_set_value(13, on);
	}
	mdelay(120);
	return 0;
}
EXPORT_SYMBOL(bcm4330_wifi_power);

int bcm4330_wifi_reset(int on)
{
	printk(KERN_INFO "%s: do nothing\n", __func__);
	return 0;
}

static struct resource bcm4330_wifi_resources[] = {
	[0] = {
		.name		= "bcm4329_wlan_irq",
		.start		= MSM_GPIO_TO_INT(BCM4330_GPIO_WIFI_IRQ),
		.end		= MSM_GPIO_TO_INT(BCM4330_GPIO_WIFI_IRQ),
#ifdef HW_OOB
		.flags          = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE,
#else
		.flags          = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
#endif
	},
};

static struct wifi_platform_data bcm4330_wifi_control = {
	.set_power      = bcm4330_wifi_power,
	.set_reset      = bcm4330_wifi_reset,
	.set_carddetect = bcm4330_wifi_set_carddetect,
	.mem_prealloc   = bcm4330_wifi_mem_prealloc,
	.get_mac_addr	= bcm4330_wifi_get_mac_addr,

};

static struct platform_device bcm4330_wifi_device = {
				.name           = "bcm4329_wlan",
				.id             = 1,
				.num_resources  = ARRAY_SIZE(bcm4330_wifi_resources),
				.resource       = bcm4330_wifi_resources,
				.dev            = {
				.platform_data = &bcm4330_wifi_control,
				},
};

unsigned char *get_wifi_nvs_ram(void);

static unsigned bcm4330_wifi_update_nvs(char *str)
{
#define NVS_LEN_OFFSET		0x0C
#define NVS_DATA_OFFSET		0x40
	unsigned char *ptr;
	unsigned len;

	if (!str)
		return -EINVAL;
	ptr = get_wifi_nvs_ram();
	/* Size in format LE assumed */
	memcpy(&len, ptr + NVS_LEN_OFFSET, sizeof(len));

	/* the last bye in NVRAM is 0, trim it */
	if (ptr[NVS_DATA_OFFSET + len - 1] == 0)
		len -= 1;

	if (ptr[NVS_DATA_OFFSET + len - 1] != '\n') {
		len += 1;
		ptr[NVS_DATA_OFFSET + len - 1] = '\n';
	}

	strcpy(ptr + NVS_DATA_OFFSET + len, str);
	len += strlen(str);
	memcpy(ptr + NVS_LEN_OFFSET, &len, sizeof(len));
	return 0;
}

#ifdef HW_OOB
static unsigned strip_nvs_param(char *param)
{
	unsigned char *nvs_data;

	unsigned param_len;
	int start_idx, end_idx;

	unsigned char *ptr;
	unsigned len;

	if (!param)
		return -EINVAL;
	ptr = get_wifi_nvs_ram();
	/* Size in format LE assumed */
	memcpy(&len, ptr + NVS_LEN_OFFSET, sizeof(len));

	/* the last bye in NVRAM is 0, trim it */
	if (ptr[NVS_DATA_OFFSET + len - 1] == 0)
		len -= 1;

	nvs_data = ptr + NVS_DATA_OFFSET;

	param_len = strlen(param);

	/* search param */
	for (start_idx = 0; start_idx < len - param_len; start_idx++) {
		if (memcmp(&nvs_data[start_idx], param, param_len) == 0)
			break;
	}

	end_idx = 0;
	if (start_idx < len - param_len) {
		/* search end-of-line */
		for (end_idx = start_idx + param_len; end_idx < len; end_idx++) {
			if (nvs_data[end_idx] == '\n' || nvs_data[end_idx] == 0)
				break;
		}
	}

	if (start_idx < end_idx) {
		/* move the remain data forward */
		for (; end_idx + 1 < len; start_idx++, end_idx++)
			nvs_data[start_idx] = nvs_data[end_idx+1];

		len = len - (end_idx - start_idx + 1);
		memcpy(ptr + NVS_LEN_OFFSET, &len, sizeof(len));
	}
	return 0;
}
#endif

#define WIFI_MAC_PARAM_STR     "macaddr="
#define WIFI_MAX_MAC_LEN       17 /* XX:XX:XX:XX:XX:XX */

static uint
get_mac_from_wifi_nvs_ram(char *buf, unsigned int buf_len)
{
	unsigned char *nvs_ptr;
	unsigned char *mac_ptr;
	uint len = 0;

	if (!buf || !buf_len)
		return 0;

	nvs_ptr = get_wifi_nvs_ram();
	if (nvs_ptr)
		nvs_ptr += NVS_DATA_OFFSET;

	mac_ptr = strstr(nvs_ptr, WIFI_MAC_PARAM_STR);
	if (mac_ptr) {
		mac_ptr += strlen(WIFI_MAC_PARAM_STR);

		/* skip leading space */
		while (mac_ptr[0] == ' ')
			mac_ptr++;

		/* locate end-of-line */
		len = 0;
		while (mac_ptr[len] != '\r' && mac_ptr[len] != '\n' &&
			mac_ptr[len] != '\0') {
			len++;
		}

		if (len > buf_len)
			len = buf_len;

		memcpy(buf, mac_ptr, len);
	}

	return len;
}

#define ETHER_ADDR_LEN 6
int bcm4330_wifi_get_mac_addr(unsigned char *buf)
{
	static u8 ether_mac_addr[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0xFF};
	char mac[WIFI_MAX_MAC_LEN];
	unsigned mac_len;
	unsigned int macpattern[ETHER_ADDR_LEN];
	int i;

	mac_len = get_mac_from_wifi_nvs_ram(mac, WIFI_MAX_MAC_LEN);
	if (mac_len > 0) {
		/*Mac address to pattern*/
		sscanf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
		&macpattern[0], &macpattern[1], &macpattern[2],
		&macpattern[3], &macpattern[4], &macpattern[5]
		);

		for (i = 0; i < ETHER_ADDR_LEN; i++)
			ether_mac_addr[i] = (u8)macpattern[i];
	}

	memcpy(buf, ether_mac_addr, sizeof(ether_mac_addr));

	printk(KERN_INFO"bcm4330_wifi_get_mac_addr = %02x %02x %02x %02x %02x %02x \n",
		ether_mac_addr[0], ether_mac_addr[1], ether_mac_addr[2], ether_mac_addr[3], ether_mac_addr[4], ether_mac_addr[5]);

	return 0;
}

int __init bcm4330_wifi_init(void)
{
	int ret;

	printk(KERN_ERR "%s: start\n", __func__);
#ifdef HW_OOB
	strip_nvs_param("sd_oobonly");
#else
	bcm4330_wifi_update_nvs("sd_oobonly=1\n");
#endif
	bcm4330_wifi_update_nvs("btc_params80=0\n");
	bcm4330_wifi_update_nvs("btc_params6=30\n");
	bcm4330_init_wifi_mem();
	ret = platform_device_register(&bcm4330_wifi_device);
	return ret;
}
