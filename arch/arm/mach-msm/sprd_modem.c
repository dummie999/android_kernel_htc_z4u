#include <linux/resource.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wakelock.h>
#include <linux/io.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/i2c/cpld.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/htc_version.h>
#include <mach/gpio.h>
#include <mach/hardware.h>
#include <mach/board_htc.h>
#include <mach/gpiomux.h>
#include <mach/board.h>
#include <mach/proc_comm.h>
#include "devices.h"
#include "pm.h"
#include "board-msm7627a.h"

#define SPRD_LOG_TAG "[MDM_SPRD] "

#define SPRD_ERR(fmt, args...) \
	printk(KERN_ERR SPRD_LOG_TAG fmt, ## args)
#define SPRD_WARN(fmt, args...) \
	printk(KERN_WARNING SPRD_LOG_TAG fmt, ## args)
#define SPRD_INFO(fmt, args...) \
	printk(KERN_INFO SPRD_LOG_TAG fmt, ## args)

#define SIM_DETECT 1
#define SDIO_SLAVE_RESET 1

#define SPRD_AP_RTS           6
#define SPRD_AP_RDY           5
#define SPRD_AP_RESEND        8
#define SPRD_MDM_RTS          27
#define SPRD_MDM_RDY          17
#define SPRD_MDM_RESEND       96
#define SPRD_SD_D0            108
#define SPRD_SD_D1            21
#define SPRD_SD_D2            20
#define SPRD_SD_D3            19
#define SPRD_SD_CMD           107
#define SPRD_SD_CLK           109
#define SPRD_MDM_NBOOT        14
#define SPRD_AP_TO_MDM1       CPLD_EXT_GPIO_AP2MDM_MODE1
#define SPRD_AP_TO_MDM2       CPLD_EXT_GPIO_AP2MDM_MODE2
#define SPRD_AP_TO_MDM3       CPLD_EXT_GPIO_AP2MDM_MODE3
#define SPRD_MDM_DEBUG_SEL	CPLD_EXT_GPIO_MDM_DEBUG_SEL
#define SPRD_MDM_TO_AP1       28
#define SPRD_MDM_TO_AP2       48
#define SPRD_MDM_TO_AP3       12
#define SPRD_MDM_ALIVE        29
#define SPRD_BB_VDD_EN        118
#define SPRD_MSM_SIM_DETECT 	41
#define SPRD_SIM_DETECT		112
#define SPRD_SIM_SWITCH		87
#define SPRD_SIM_SWITCH_EVM	CPLD_EXT_GPIO_CDMA_RUIM_GSM_SIM_SW

#define SPRD_IRQ_TYPE	(IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING)

#define SPRD_STATUS_IDLE 		0x00
#define SPRD_STATUS_POWER_SET		0x01

#define SPRD_MODEM_POWER_OFF_TIME_OUT 6000
#define SPRD_MODEM_POWER_ON_TIME_OUT 10000

#define SPRD_MODEM_POWER_OFF     0
#define SPRD_MODEM_POWER_ON      1
#define SPRD_MODEM_POWER_OFF_ONGOING     2
#define SPRD_MODEM_POWER_ON_ONGOING      3

static int set_modem_power(int value);
#ifdef CONFIG_SDIO_TTY_SP6502COM
extern void modem_detect(bool,unsigned int);
extern int modem_detect_status(void);
#endif
#ifdef CONFIG_SPRD_FLASHLESS
#define SPRD_MDM_FLASHLESS SPRD_MDM_TO_AP2
#define SPRD_SDIO_RESET SPRD_MDM_TO_AP2
#define SPRD_SDIO_RESET_INIT SPRD_MDM_RESEND
#define SPRD_MUX_FLOW_CONTROL	SPRD_MDM_TO_AP3

struct workqueue_struct *sprd_alive_wq;
struct workqueue_struct *sprd_detect_wq;
struct workqueue_struct *sprd_uart_wq;
extern int flashless_mode_get(void);
extern void flashless_mode_set(int);
extern int sprd_uart_write(void);
int flashless_detect = 0;  
int bootmode = 0;
ktime_t rx_q;
#endif
#ifdef SIM_DETECT
static int sim_status = 0;
#if (defined(CONFIG_MACH_DUMMY))
static int sim_switch = 0;
static int msm_sim_status = 0;
#endif
#endif
enum irq_state {
	irq_mdm_alive = 0x0001,
	irq_mdm_to_ap = 0x0002,
	irq_flashlessd = 0x0004,
	irq_sim_detect = 0x0008,
	irq_msm_sim_detect = 0x0010,
	irq_slave_reset = 0x0020,
};

enum sprd_boot_mode {
	mdm_default = 0x0000,
	mdm_radio_router = 0x0002,
	mdm_offline_nsim = 0x0003,
	mdm_offline = 0x0004,
	mdm_krouter_adc = 0x0005,
	mdm_krouter1_cft = 0x0006,
	mdm_ramdump = 0x0007,
	mdm_online = 0x0008,
	mdm_krouter2 = 0x0009,
};

static struct sprd_notify_info {
	struct kobject *sprd_notify_kobj;
	unsigned int modem_alive_irq;
#ifdef CONFIG_SPRD_FLASHLESS
	unsigned int reset_irq;
	unsigned int flashless_irq;
	unsigned int log_throttle;
#endif
	unsigned int modem_to_ap_irq;
	unsigned int sim_detect_irq;
#if (defined(CONFIG_MACH_DUMMY))
	unsigned int msm_sim_detect_irq;
#endif
	unsigned char cpoff;
	unsigned int power_status;
	unsigned int  state;
	unsigned long irq_state;
	wait_queue_head_t wait_power_off_wq;
	wait_queue_head_t wait_power_on_wq;
	struct workqueue_struct *sprd_notify_wq;
} sprd_notify_info;

static struct msm_gpio sprd_pin_init[] = {
	{GPIO_CFG(SPRD_AP_RTS,	0, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "ap_rts"},
	{GPIO_CFG(SPRD_AP_RDY,	0, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "ap_rdy"},
	{GPIO_CFG(SPRD_AP_RESEND, 0, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "ap_resend"},
	{GPIO_CFG(SPRD_MDM_RTS,	0, GPIO_CFG_INPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "mdm_rts"},
	{GPIO_CFG(SPRD_MDM_RDY,	0, GPIO_CFG_INPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "mdm_rdy"},
	{GPIO_CFG(SPRD_MDM_RESEND,0, GPIO_CFG_INPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "mdm_resend"},
#if (defined(CONFIG_MACH_DUMMY))
	{GPIO_CFG(SPRD_SD_D0,	1, GPIO_CFG_INPUT,	GPIO_CFG_PULL_UP, GPIO_CFG_4MA), "mdm_sdio_d0" },
	{GPIO_CFG(SPRD_SD_D1,	1, GPIO_CFG_INPUT,	GPIO_CFG_PULL_UP, GPIO_CFG_4MA), "mdm_sdio_d1" },
	{GPIO_CFG(SPRD_SD_D2,	1, GPIO_CFG_INPUT,	GPIO_CFG_PULL_UP, GPIO_CFG_4MA), "mdm_sdio_d2" },
	{GPIO_CFG(SPRD_SD_D3,	1, GPIO_CFG_INPUT,	GPIO_CFG_PULL_UP, GPIO_CFG_4MA), "mdm_sdio_d3" },
	{GPIO_CFG(SPRD_SD_CMD,	1, GPIO_CFG_INPUT,	GPIO_CFG_PULL_UP, GPIO_CFG_4MA), "mdm_sdio_cmd" },
	{GPIO_CFG(SPRD_SD_CLK,	1, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_12MA), "mdm_sdio_clk" },
#else
	{GPIO_CFG(SPRD_SD_D0,	1, GPIO_CFG_INPUT,	GPIO_CFG_PULL_UP, GPIO_CFG_6MA), "mdm_sdio_d0" },
	{GPIO_CFG(SPRD_SD_D1,	1, GPIO_CFG_INPUT,	GPIO_CFG_PULL_UP, GPIO_CFG_6MA), "mdm_sdio_d1" },
	{GPIO_CFG(SPRD_SD_D2,	1, GPIO_CFG_INPUT,	GPIO_CFG_PULL_UP, GPIO_CFG_6MA), "mdm_sdio_d2" },
	{GPIO_CFG(SPRD_SD_D3,	1, GPIO_CFG_INPUT,	GPIO_CFG_PULL_UP, GPIO_CFG_6MA), "mdm_sdio_d3" },
	{GPIO_CFG(SPRD_SD_CMD,	1, GPIO_CFG_INPUT,	GPIO_CFG_PULL_UP, GPIO_CFG_6MA), "mdm_sdio_cmd" },
	{GPIO_CFG(SPRD_SD_CLK,	1, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_12MA), "mdm_sdio_clk" },
#endif
	{GPIO_CFG(SPRD_MDM_TO_AP1,0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),"mdm_to_ap1"},
	{GPIO_CFG(SPRD_MDM_TO_AP2,0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),"mdm_to_ap2"},
	{GPIO_CFG(SPRD_MDM_TO_AP3,0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),"mdm_to_ap3"},

	{GPIO_CFG(SPRD_MDM_ALIVE,0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),"mdm_alive"},
	{GPIO_CFG(SPRD_SIM_DETECT,0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_UP, GPIO_CFG_2MA),"mdm_sim_detect"},
};

static struct msm_gpio sprd_pin_input_set[] = {
	{GPIO_CFG(SPRD_AP_RTS,	0, GPIO_CFG_INPUT,	GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ap_rts"},
	{GPIO_CFG(SPRD_AP_RDY,	0, GPIO_CFG_INPUT,	GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ap_rdy"},
	{GPIO_CFG(SPRD_AP_RESEND, 0, GPIO_CFG_INPUT,	GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "ap_resend"},

	{GPIO_CFG(SPRD_MDM_NBOOT,0, GPIO_CFG_INPUT,   GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "mdm_nboot"},
};

static struct msm_gpio sprd_pin_flymode_cfg[] = {
	{GPIO_CFG(SPRD_AP_RTS,	0, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "ap_rts"},
	{GPIO_CFG(SPRD_AP_RDY,	0, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "ap_rdy"},
	{GPIO_CFG(SPRD_AP_RESEND, 0, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "ap_resend"},
	{GPIO_CFG(SPRD_MDM_RTS,	0, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "mdm_rts"},
	{GPIO_CFG(SPRD_MDM_RDY,	0, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "mdm_rdy"},
	{GPIO_CFG(SPRD_MDM_RESEND,0, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "mdm_resend"},

	{GPIO_CFG(SPRD_SD_D0,	0, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "mdm_sdio_d0"},
	{GPIO_CFG(SPRD_SD_D1,	0, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "mdm_sdio_d1"},
	{GPIO_CFG(SPRD_SD_D2,	0, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "mdm_sdio_d2"},
	{GPIO_CFG(SPRD_SD_D3,	0, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "mdm_sdio_d3"},
	{GPIO_CFG(SPRD_SD_CMD,	0, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "mdm_sdio_cmd"},
	{GPIO_CFG(SPRD_SD_CLK,	0, GPIO_CFG_OUTPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "mdm_sdio_clk"},

	{GPIO_CFG(SPRD_MDM_NBOOT,0, GPIO_CFG_OUTPUT,   GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "mdm_nboot"},
	{GPIO_CFG(SPRD_MDM_TO_AP1,0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),"mdm_to_ap1"},
	{GPIO_CFG(SPRD_MDM_TO_AP2,0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),"mdm_to_ap2"},
	{GPIO_CFG(SPRD_MDM_TO_AP3,0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),"mdm_to_ap3"},

	{GPIO_CFG(SPRD_MDM_ALIVE,0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),"mdm_alive"},
};

#ifndef CONFIG_SPRD_FLASHLESS
static int init_variables(void)
{
	sprd_notify_info.power_status = SPRD_MODEM_POWER_OFF;
	sprd_notify_info.state = SPRD_STATUS_IDLE;
	sprd_notify_info.cpoff = 0;
	return 0;
}
#endif

static inline void enable_uart2dm(int enable)
{
	unsigned write_cmd = 3;
	unsigned read_cmd = 4;
	unsigned value = 0;
	msm_proc_comm(0x5c, &read_cmd, &value);
	SPRD_INFO("before, value=0x%x\n", value);
	value = 0x10;
	msm_proc_comm(0x5c, &write_cmd, &value);
	msm_proc_comm(0x5c, &read_cmd, &value);
	SPRD_INFO("after, value=0x%x\n", value);
}

static void sprd_cpld_pin_init(void)
{
	int ret = 0;
	ret = cpld_gpio_write(SPRD_AP_TO_MDM1,0);
	if (ret < 0)
		SPRD_ERR("%s: Fail to set AP_TO_MDM1\n", __func__);

	ret = cpld_gpio_write(SPRD_AP_TO_MDM2,0);
	if (ret < 0)
		SPRD_ERR("%s: Fail to set AP_TO_MDM2\n", __func__);

	ret = cpld_gpio_write(SPRD_AP_TO_MDM3,0);
	if (ret < 0)
		SPRD_ERR("%s: Fail to set AP_TO_MDM3\n", __func__);
}

static void sprd_cpld_mdpin_set(int mode)
{
	int ret = 0;
	int board_ver = htc_get_board_revision();

	bootmode = mode;
	SPRD_INFO("%s: cpld mode 0x%x\n", __func__,mode);
	ret = cpld_gpio_write(SPRD_AP_TO_MDM1,mode & 0x1);
	if (ret < 0)
		SPRD_ERR("%s: Fail to set AP_TO_MDM1\n", __func__);

	ret = cpld_gpio_write(SPRD_AP_TO_MDM2,mode & 0x2);
	if (ret < 0)
		SPRD_ERR("%s: Fail to set AP_TO_MDM2\n", __func__);

	ret = cpld_gpio_write(SPRD_AP_TO_MDM3,mode & 0x4);
	if (ret < 0)
		SPRD_ERR("%s: Fail to set AP_TO_MDM3\n", __func__);

	if (mode & 0x8) {
		SPRD_INFO("SWITCH USB to SPRD modem board_ver:%d\n",board_ver);
		if (!board_ver) 
			ret = cpld_gpio_write(SPRD_MDM_DEBUG_SEL,0);
		else
			ret = cpld_gpio_write(SPRD_MDM_DEBUG_SEL,1);
		if (ret < 0)
			SPRD_ERR("%s: Fail to set MDM_DEBUG_SEL\n", __func__);
	}
	return;
}

static void sprd_bootmode_select(void)
{
	int mode = board_mfg_mode();
	int radio_flag = get_radio2_flag();
	SPRD_INFO("%s: kernel boot mode %d\n", __func__,mode);

	if (mode == 9) {	
		sprd_cpld_mdpin_set(mdm_radio_router);
	} else if (mode == 10 || mode == 11) {	
		sprd_cpld_mdpin_set(mdm_krouter1_cft);
	} else if (mode == 12) {	
		sprd_cpld_mdpin_set(mdm_krouter_adc);
	} else if (mode == 13) {	
		sprd_cpld_mdpin_set(mdm_krouter2);
	} else if (radio_flag & 0x200) { 
		sprd_cpld_mdpin_set(mdm_online);
	} else if (radio_flag & 0x400) { 
		if (radio_flag & 0x4) {	
			sprd_cpld_mdpin_set(mdm_offline_nsim);
		} else {
			sprd_cpld_mdpin_set(mdm_offline);
		}
	} else {	
		sprd_cpld_mdpin_set(mdm_default);
	}

	return;
}

static int sprd_input_pin_config(void)
{
	int rc = 0;

	SPRD_INFO("%s\n", __func__);

	rc = msm_gpios_enable(sprd_pin_input_set,
		ARRAY_SIZE(sprd_pin_input_set));

	if (rc) {
		SPRD_WARN("%s: rc =%d\n", __func__, rc);
	}
	return rc;
}

static int sprd_flymode_pin_config(void)
{
	int ret = 0;
	int array_size = 0;
	int i = 0;

	SPRD_INFO("%s\n", __func__);

	ret = msm_gpios_enable(sprd_pin_flymode_cfg,
		ARRAY_SIZE(sprd_pin_flymode_cfg));

	if (ret) {
		SPRD_WARN("%s:pin cfg ret =%d\n", __func__, ret);
	}

	array_size = sizeof(sprd_pin_flymode_cfg) / sizeof(sprd_pin_flymode_cfg[0]);

	for(i = 0; i< array_size; i++ ){
		gpio_set_value(GPIO_PIN(sprd_pin_flymode_cfg[i].gpio_cfg), 0);
	}

	return ret;
}

static void sprd_init_pin_pre(void)
{
	sprd_cpld_pin_init();
	
	gpio_tlmm_config(GPIO_CFG(SPRD_MDM_NBOOT, 0, GPIO_CFG_INPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
}

static int sprd_init_pin_config(void)
{
	int rc = 0;
	int radio_flag = get_radio2_flag();

	SPRD_INFO("%s, radio2_flag: 0x%x\n", __func__, radio_flag);

	rc = msm_gpios_enable(sprd_pin_init,
		ARRAY_SIZE(sprd_pin_init));

	if (rc) {
		SPRD_WARN("%s: rc =%d\n", __func__, rc);
	}

	gpio_set_value(SPRD_MUX_FLOW_CONTROL, 0);
	gpio_set_value(SPRD_AP_RDY, 1);	

	sprd_bootmode_select();

	return rc;
}

static void sprd_irq_set(int irq,int mode,int on)
{
	if (on && !test_bit(mode, &sprd_notify_info.irq_state)) {
		enable_irq(irq);
		irq_set_irq_wake(irq,1);
		set_bit(mode, &sprd_notify_info.irq_state);
	} else if (!on && test_bit(mode, &sprd_notify_info.irq_state)) {
		irq_set_irq_wake(irq,0);
		disable_irq(irq);
		clear_bit(mode, &sprd_notify_info.irq_state);
	} else {
		SPRD_WARN("Duplicated irq mode set,mode %d,on %d\n",mode,on);
	}
}

static int sprd_set_modem_power(int is_on)
{
	if (is_on) {
		gpio_tlmm_config(GPIO_CFG(SPRD_BB_VDD_EN, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
			GPIO_CFG_ENABLE);
		gpio_set_value(SPRD_BB_VDD_EN, 1);
	} else {
		gpio_tlmm_config(GPIO_CFG(SPRD_BB_VDD_EN, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
			GPIO_CFG_ENABLE);
		gpio_set_value(SPRD_BB_VDD_EN, 0);
	}
	return 0;
}

static ssize_t modem_state_power_get(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	int power;
	int ctl_pin = 0;

	ctl_pin = gpio_get_value(SPRD_BB_VDD_EN);
	power = sprd_notify_info.power_status;
#ifndef CONFIG_SPRD_FLASHLESS
	if (power != ctl_pin) {
		SPRD_WARN("%s: please check modem power control pin,power = %d,ctl = %d\n", __func__,power,ctl_pin);
	}
#endif
	if (power == SPRD_MODEM_POWER_ON) {
		SPRD_INFO("%s: modem power on\n", __func__);
	} else if (power == SPRD_MODEM_POWER_ON_ONGOING) {
		SPRD_INFO("%s: modem power ongoing ,alive = %d\n", __func__, gpio_get_value(SPRD_MDM_ALIVE));
	} else {
		SPRD_INFO("%s: modem power off\n", __func__);
	}
	return sprintf(buf, "%d\n", power);
}

static ssize_t modem_state_power_set(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	int ret = count;
	if (sprd_notify_info.state != SPRD_STATUS_IDLE) {
		SPRD_INFO("%s: is setting modem power!\n", __func__);
#ifndef CONFIG_SPRD_FLASHLESS
		return 0;
#endif
	}

	if (count > 0) {
		switch (buf[0]) {
		case '0':
			set_modem_power(0);
			msleep(100);
			SPRD_INFO("%s: modem power off successful!\n", __func__);
			break;
		case '1':
#ifdef CONFIG_SPRD_FLASHLESS
			set_modem_power(1);
#else
			if (set_modem_power(1) == 0) {
				SPRD_INFO("%s: modem power on fail!\n", __func__);
				return 0;
			}
			SPRD_INFO("%s: modem power on successful!\n", __func__);
#endif
			break;

		default:
			break;
		}
	}
	return ret;
}

static DEVICE_ATTR(modempower, S_IRUSR | S_IWUSR,
		modem_state_power_get, modem_state_power_set);

static ssize_t cpoff_set(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	int ret = count;
	if (count > 0) {
		switch (buf[0]) {
			case '1':
				sprd_notify_info.cpoff = 1;
				SPRD_INFO("%s: CP off is set\n", __func__);
				break;
			default:
				break;
		}
	}
	return ret;
}
static DEVICE_ATTR(cpoff, S_IRUSR | S_IWUSR, NULL, cpoff_set);

static ssize_t mdm_assert(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	int ret = count;
	if (count > 0) {
		switch (buf[0]) {
			case '1':
				if (get_radio2_flag() & 0x8) {
					sprd_cpld_pin_init();
					msleep(30);
					sprd_cpld_mdpin_set(mdm_ramdump);
					SPRD_INFO("%s: Mdm assert set\n", __func__);
				} else {
					SPRD_INFO("%s: Radio mode not allow trigger mdm ramdump,0x%x\n",
						__func__,get_radio2_flag());
				}
				break;
			default:
				break;
		}
	}
	return ret;
}
static DEVICE_ATTR(mdm_assert, S_IRUSR | S_IWUSR, NULL, mdm_assert);

#ifdef CONFIG_SPRD_FLASHLESS
static ssize_t flashless_get(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", flashless_mode_get());
}

static ssize_t flashless_set(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	int ret = count;
	if (count > 0) {
		switch (buf[0]) {
			case '0':
				flashless_mode_set(0);
				SPRD_INFO("%s: flashless_mode disabled\n", __func__);
				break;

			case '1':
				flashless_mode_set(1);
				SPRD_INFO("%s: flashless_mode enabled\n", __func__);
				break;
			default:
				break;
		}
	}
	return ret;
}
static DEVICE_ATTR(fl_mode, S_IRUSR | S_IWUSR, flashless_get, flashless_set);
#endif

#ifdef SIM_DETECT
static void mdm_sim_detect_uevent(struct work_struct *work)
{
	char *envp_sim_on[] = { "SIM_PLUG_IN", NULL };
	char *envp_sim_off[] = { "SIM_PLUG_OUT", NULL };
	int value = 0;
	int board_ver = htc_get_board_revision();

#if (defined(CONFIG_MACH_DUMMY))
	if (sim_switch) {
		value = gpio_get_value(SPRD_MSM_SIM_DETECT);
		if (msm_sim_status != value) {
			msm_sim_status = value;
		} else {
			SPRD_WARN("%s:MSM sim find same status %d, filter it\n", __func__, value);
			return;
		}
	} else {
		value = gpio_get_value(SPRD_SIM_DETECT);
		if (sim_status != value) {
			sim_status = value;
		} else {
			SPRD_WARN("%s:SPRD sim find same status %d, filter it\n", __func__, value);
			return;
		}
	}
#else
	value = gpio_get_value(SPRD_SIM_DETECT);
	if (sim_status != value) {
		sim_status = value;
	} else {
		SPRD_WARN("%s:find same status %d, filter it\n", __func__, value);
		return;
	}
#endif
	if (value == !board_ver) { 
		SPRD_INFO("uevent: SIM_PLUG_OUT\n");
		kobject_uevent_env(sprd_notify_info.sprd_notify_kobj, KOBJ_CHANGE, envp_sim_off);
	} else {
		SPRD_INFO("uevent: SIM_PLUG_IN\n");
		kobject_uevent_env(sprd_notify_info.sprd_notify_kobj, KOBJ_CHANGE, envp_sim_on);
	}
}

static DECLARE_DELAYED_WORK(mdm_sim_detect_uevent_work, mdm_sim_detect_uevent);

static irqreturn_t sprd_sim_detect_irq_handler(int irq, void *dev)
{
	unsigned int value;

	value = gpio_get_value(SPRD_SIM_DETECT);
	SPRD_INFO("SPRD SIM detect pin = %d\n", value);
	queue_delayed_work(sprd_notify_info.sprd_notify_wq,
					&mdm_sim_detect_uevent_work, msecs_to_jiffies(500));
	return IRQ_HANDLED;
}

#if (defined(CONFIG_MACH_DUMMY))
static irqreturn_t msm_sim_detect_irq_handler(int irq, void *dev)
{
	unsigned int value;

	value = gpio_get_value(SPRD_MSM_SIM_DETECT);
	SPRD_INFO("MSM SIM detect pin = %d\n", value);
	queue_delayed_work(sprd_notify_info.sprd_notify_wq,
					&mdm_sim_detect_uevent_work, msecs_to_jiffies(500));
	return IRQ_HANDLED;
}

static ssize_t sim_switch_get(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", sim_switch);
}

static ssize_t sim_switch_set(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	char *envp_switch_on[] = { "SIM_SWITCH_ON", NULL };
	char *envp_switch_off[] = { "SIM_SWITCH_OFF", NULL };
	int ret = 0;
	int board_ver = htc_get_board_revision();
	if (count > 0) {
		switch (buf[0]) {
			case '0':
				if (sprd_notify_info.msm_sim_detect_irq) {
					sprd_irq_set(sprd_notify_info.msm_sim_detect_irq,irq_msm_sim_detect,0);
					SPRD_INFO("%s: MSM sim irq disabled\n", __func__);
					if (!board_ver) 
						cpld_gpio_write(SPRD_SIM_SWITCH_EVM,1);
					else
						gpio_set_value(SPRD_SIM_SWITCH,0);
					sprd_irq_set(sprd_notify_info.sim_detect_irq,irq_sim_detect,1);
					SPRD_INFO("%s: SPRD sim irq enabled & sim_switch disabled\n", __func__);
					sim_switch = 0;
					kobject_uevent_env(sprd_notify_info.sprd_notify_kobj, KOBJ_CHANGE, envp_switch_off);
				} else {
					SPRD_WARN("%s: MSM sim irq disable FAILED\n", __func__);
				}
				break;
			case '1':
				sprd_irq_set(sprd_notify_info.sim_detect_irq,irq_sim_detect,0);
				SPRD_INFO("%s: SPRD sim irq disabled\n", __func__);
				if (!board_ver) 
					cpld_gpio_write(SPRD_SIM_SWITCH_EVM,0);
				else
					gpio_set_value(SPRD_SIM_SWITCH,1);
				if (!sprd_notify_info.msm_sim_detect_irq) {
					sprd_notify_info.msm_sim_detect_irq = gpio_to_irq(SPRD_MSM_SIM_DETECT);
					ret = request_irq(sprd_notify_info.msm_sim_detect_irq, msm_sim_detect_irq_handler,
								 IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
								 "msm_sim_detect_irq", &sprd_notify_info);
					if (ret) {
						SPRD_ERR("%s: Failed to request msm_sim_detect_irq(0x%08X)\n", __func__, ret);
						break;
					}
					set_bit(irq_msm_sim_detect,&sprd_notify_info.irq_state);
					irq_set_irq_wake(sprd_notify_info.msm_sim_detect_irq,1);
				}
				sprd_irq_set(sprd_notify_info.msm_sim_detect_irq,irq_msm_sim_detect,1);
				msm_sim_status = gpio_get_value(SPRD_MSM_SIM_DETECT);
				SPRD_INFO("%s: MSM sim irq enabled & sim_switch enabled,%d\n", __func__, msm_sim_status);
				sim_switch = 1;
				kobject_uevent_env(sprd_notify_info.sprd_notify_kobj, KOBJ_CHANGE, envp_switch_on);
				break;
			default:
				break;
		}
	}
	return count;
}
static DEVICE_ATTR(sim_switch, S_IRUSR | S_IWUSR, sim_switch_get, sim_switch_set);
#endif
#endif

#ifdef CONFIG_SDIO_TTY_SP6502COM
static ssize_t sdio_detect_status(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", modem_detect_status());
}

static ssize_t sdio_detect(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	int ret = count;
	if (count > 0) {
		switch (buf[0]) {
			case '0':
				modem_detect(0,500);
				SPRD_INFO("%s: SDIO_detect off\n", __func__);
				break;

			case '1':
				modem_detect(1,500);
				SPRD_INFO("%s: SDIO_detect on\n", __func__);
				break;
			default:
				break;
		}
	}
	return ret;
}
static DEVICE_ATTR(sdio_detect, S_IRUSR | S_IWUSR, sdio_detect_status, sdio_detect);
#endif

#ifdef SIM_DETECT
static ssize_t sim_detect_get(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	int value;
	int board_ver = htc_get_board_revision();

#if (defined(CONFIG_MACH_DUMMY))
	
	if (sim_switch)
		if (!board_ver) 
			value = !msm_sim_status;
		else
			value = msm_sim_status;
	else {
		if (!board_ver) 
			value = !sim_status;
		else
			value = sim_status;
	}
#else
	
	if (!board_ver) 
		value = !sim_status;
	else
		value = sim_status;
#endif
	return sprintf(buf, "%d\n", value);
}
static DEVICE_ATTR(sim_detect, S_IRUSR, sim_detect_get, NULL);
#endif
#ifdef UART_MUX_LOG
void mux_log_throttle(int on)
{
	if (on != sprd_notify_info.log_throttle) {
		gpio_set_value(SPRD_MUX_FLOW_CONTROL,on);
		sprd_notify_info.log_throttle = on;
	}
}
EXPORT_SYMBOL(mux_log_throttle);
#endif

#ifdef CONFIG_SPRD_FLASHLESS
#ifdef SDIO_SLAVE_RESET
void modem_sdio_reset(int on)
{
	if (on) {
		sprd_irq_set(sprd_notify_info.reset_irq,irq_slave_reset,1);
		gpio_set_value(SPRD_SDIO_RESET,1);
		mdelay(250);
		gpio_set_value(SPRD_SDIO_RESET,0);
	} else {
		sprd_irq_set(sprd_notify_info.reset_irq,irq_slave_reset,0);
	}
}
EXPORT_SYMBOL(modem_sdio_reset);
#endif

static void modem_alive_enable(struct work_struct *work)
{
	msleep(500);
	sprd_irq_set(sprd_notify_info.flashless_irq,irq_flashlessd,1);
	msleep(3000);
	sprd_irq_set(sprd_notify_info.modem_alive_irq,irq_mdm_alive,1);

}
static DECLARE_DELAYED_WORK(sprd_alive_work, modem_alive_enable);

static void sdio_detect_work(struct work_struct *work)
{
	modem_detect(1,0);
}
static DECLARE_DELAYED_WORK(sprd_detect_work, sdio_detect_work);

static void cpld_disable_work(struct work_struct *work)
{
	sprd_cpld_pin_init();
}
static DECLARE_DELAYED_WORK(sprd_cpld_work, cpld_disable_work);

static void sprd_uart_tx_work(struct work_struct *work)
{
	int i = 7;
	int ret = 0;
	ktime_t rx_s;
	ktime_t tx_s;
	rx_s = ktime_get();

	while (i--) {
		ret = sprd_uart_write();
		if (ret < 0) {
			SPRD_WARN("unexpected handshake %d,%d\n",ret,i);
			break;
		}
		mdelay(50);
	}

	tx_s = ktime_sub(ktime_get(),rx_s);

	if (ktime_to_ms(tx_s) < 300) {
		i = 5;
		while (i--) {
			ret = sprd_uart_write();
			if (ret < 0) {
				SPRD_WARN("unexpected handshake %d,%d\n",ret,i);
				break;
			}
			mdelay(40);
		}
		SPRD_WARN("send time %lld ms,another 5 times\n",ktime_to_ms(tx_s));
		tx_s = ktime_sub(ktime_get(),rx_s);
	}

	SPRD_WARN("queue time = %lld us,total time = %lld us\n",
		ktime_to_us(ktime_sub(rx_s,rx_q)),ktime_to_us(tx_s));

}
static DECLARE_DELAYED_WORK(sprd_uart_work, sprd_uart_tx_work);

void sprd_queue_tx(void)
{
	rx_q = ktime_get();
	queue_delayed_work(sprd_uart_wq,
		&sprd_uart_work, msecs_to_jiffies(720));
}
#endif

#define SDIO_ENABLE_BITS (0x400 | 0x800)
static int set_modem_power(int value)
{
	int get_status = 0;
	sprd_notify_info.state = SPRD_STATUS_POWER_SET;

	SPRD_INFO("%s:MODEM POWER set state, value = %d\n", __func__, value);
	SPRD_INFO("Process %s,Current PID %u\n",current->comm,current->pid);

	if (1 == value) {
		if (SPRD_MODEM_POWER_ON == sprd_notify_info.power_status) {
			SPRD_WARN("%s:MODEM POWER on state error, please check! \n", __func__);
		}
		sprd_notify_info.power_status = SPRD_MODEM_POWER_ON_ONGOING;

		sprd_init_pin_pre();

		sprd_set_modem_power(1);
#ifdef CONFIG_SPRD_FLASHLESS
		sprd_queue_tx();
#endif
		mdelay(230);

		enable_uart2dm(1);
		sprd_init_pin_config();

#ifdef CONFIG_SPRD_FLASHLESS
		flashless_detect = 0;
		queue_delayed_work(sprd_alive_wq,
						&sprd_alive_work, 0);
#else
		msleep(1000);
		sprd_irq_set(sprd_notify_info.modem_alive_irq,irq_mdm_alive,1);
#endif
#ifndef CONFIG_SPRD_FLASHLESS
		if (wait_event_interruptible_timeout(sprd_notify_info.wait_power_on_wq,
			 (sprd_notify_info.power_status == SPRD_MODEM_POWER_ON),
					 msecs_to_jiffies(SPRD_MODEM_POWER_ON_TIME_OUT)) == 0) {
				SPRD_INFO("power status = %d\n", sprd_notify_info.power_status);
				init_variables();
				SPRD_ERR("%s: modem power up timeout!\n", __func__);
				return 0;
		}

		
		gpio_tlmm_config(GPIO_CFG(SPRD_MDM_TO_AP2, 0, GPIO_CFG_INPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);

#ifdef CONFIG_SDIO_TTY_SP6502COM
		modem_detect(1,0);
#endif
		sprd_notify_info.state = SPRD_STATUS_IDLE;
		sprd_notify_info.cpoff = 0 ;
#endif
	} else {
		if (sprd_notify_info.power_status != SPRD_MODEM_POWER_OFF) {
			sprd_notify_info.power_status = SPRD_MODEM_POWER_OFF_ONGOING;

			
			sprd_input_pin_config();

			if (sprd_notify_info.cpoff == 1) {
				if (wait_event_interruptible_timeout(sprd_notify_info.wait_power_off_wq,
					 (sprd_notify_info.power_status == SPRD_MODEM_POWER_OFF), msecs_to_jiffies(SPRD_MODEM_POWER_OFF_TIME_OUT))==0) {
					get_status = gpio_get_value(SPRD_MDM_TO_AP1);
					SPRD_INFO("%s: wait modem power off timeout, force power off\n", __func__);
					SPRD_INFO("%s: gpio_get_value SPRD_MDM_TO_AP=[%d]\n", __func__, get_status);
				}
			}
		} else {
			
			sprd_input_pin_config();
		}

		sprd_set_modem_power(0);
#ifdef CONFIG_SDIO_TTY_SP6502COM
		modem_detect(0,2000);
#endif
		
		sprd_cpld_pin_init();
		sprd_flymode_pin_config();
#ifdef CONFIG_SPRD_FLASHLESS
		sprd_irq_set(sprd_notify_info.flashless_irq,irq_flashlessd,0);
		sprd_notify_info.log_throttle = 0;
#ifdef SDIO_SLAVE_RESET
		sprd_irq_set(sprd_notify_info.reset_irq,irq_slave_reset,0);
#endif
#endif
		sprd_irq_set(sprd_notify_info.modem_alive_irq,irq_mdm_alive,0);
		sprd_notify_info.state = SPRD_STATUS_IDLE;
		sprd_notify_info.cpoff = 0 ;
	}

	return 1;
}

#ifdef SPRD_MDM_POWER_ON_UEVENT
static void mdm_power_on_uevent(struct work_struct *work)
{
	char *envp[] = { "MDM_POWER_ON", NULL };

	SPRD_INFO("MDM_POWER_ON: kobject_uevent_env\n");

	kobject_uevent_env(sprd_notify_info.sprd_notify_kobj, KOBJ_CHANGE, envp);
}
#endif

static void mdm_assert_uevent(struct work_struct *work)
{
	char *envp[] = { "MDM_ASSERT", NULL };

	SPRD_INFO("MDM_ASSERT: kobject_uevent_env\n");

	kobject_uevent_env(sprd_notify_info.sprd_notify_kobj, KOBJ_CHANGE, envp);
}

static DECLARE_DELAYED_WORK(mdm_assert_uevent_work, mdm_assert_uevent);

#ifdef CONFIG_SPRD_FLASHLESS
static irqreturn_t sprd_mdm_flashless_irq_handler(int irq, void *dev)
{
	unsigned int value;
	struct sprd_notify_info *info = (struct sprd_notify_info*) dev;

	value = gpio_get_value(SPRD_MDM_FLASHLESS);
	SPRD_INFO("Others MDM_FLASHLESS status = %d,value = %d\n", info->power_status,value);
	if (SPRD_MODEM_POWER_ON_ONGOING == info->power_status) {
		if (value == 1) {
			SPRD_INFO("mdm_flashless pin is high\n");
			if (flashless_detect == 1)	
				queue_delayed_work(sprd_detect_wq,
						&sprd_detect_work, 0);
			flashless_detect++;
		} else {
			SPRD_INFO("mdm_flashless pin is low\n");
		}
	} else {
		SPRD_INFO("MDM_FLASHLESS status invalid = %d,value = %d\n", info->power_status,value);
	}

	return IRQ_HANDLED;
}
#ifdef SDIO_SLAVE_RESET
static irqreturn_t sprd_mdm_reset_irq_handler(int irq, void *dev)
{
	unsigned int value;
	struct sprd_notify_info *info = (struct sprd_notify_info*) dev;

	value = gpio_get_value(SPRD_SDIO_RESET_INIT);
	SPRD_INFO("Others MDM_RESET status = %d,value = %d\n", info->power_status,value);
	if (SPRD_MODEM_POWER_ON == info->power_status) {
		if (value == 1) {
			SPRD_INFO("mdm_reset pin is high\n");
			queue_delayed_work(sprd_detect_wq,
					&sprd_detect_work, 0);
		} else {
			SPRD_INFO("mdm_reset pin is low\n");
		}
	} else {
		SPRD_INFO("MDM_RESET status invalid = %d,value = %d\n", info->power_status,value);
	}

	return IRQ_HANDLED;
}
#endif
#endif

static void mdm_alive_debounce(struct work_struct *work)
{
	struct sprd_notify_info *info = &sprd_notify_info;
	unsigned int value;

	value = gpio_get_value(SPRD_MDM_ALIVE);
	if (SPRD_MODEM_POWER_ON_ONGOING == info->power_status) {
		if (value == 1) {
			SPRD_INFO("alive pin is high\n");
			info->power_status = SPRD_MODEM_POWER_ON;
#ifndef CONFIG_SPRD_FLASHLESS
			if (list_empty(&(info->wait_power_on_wq.task_list))) {
				return;
			}
			wake_up_interruptible(&(info->wait_power_on_wq));
#else
			sprd_irq_set(sprd_notify_info.flashless_irq,irq_flashlessd,0);
			queue_delayed_work(sprd_detect_wq,
							&sprd_detect_work, 0);

			queue_delayed_work(sprd_detect_wq,
							&sprd_cpld_work, 0);

			
			gpio_tlmm_config(GPIO_CFG(SPRD_MDM_ALIVE, 0, GPIO_CFG_INPUT,
						GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
						GPIO_CFG_ENABLE);

			gpio_tlmm_config(GPIO_CFG(SPRD_SDIO_RESET, 0, GPIO_CFG_OUTPUT,
						GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
						GPIO_CFG_ENABLE);
			gpio_set_value(SPRD_SDIO_RESET, 0);

			sprd_notify_info.state = SPRD_STATUS_IDLE;
			sprd_notify_info.log_throttle = 0;
			sprd_notify_info.cpoff = 0 ;
#endif
			return;
		} else {
			SPRD_INFO("alive pin is low\n");
		}
	} else if (SPRD_MODEM_POWER_ON == info->power_status) {
		
		if ((value == 0) && (sprd_notify_info.cpoff == 0)) {
			SPRD_INFO("assert, alive pin = %d\n", value);
			queue_delayed_work(sprd_notify_info.sprd_notify_wq,
						&mdm_assert_uevent_work, 0);
		} else {
			SPRD_INFO("others, alive pin = %d\n", value);
		}
	} else {
		SPRD_INFO("power off alive pin = %d\n", value);
	}
}

static DECLARE_DELAYED_WORK(mdm_alive_debounce_work, mdm_alive_debounce);

static irqreturn_t sprd_mdm_alive_irq_handler(int irq, void *dev)
{
	unsigned int value;
	value = gpio_get_value(SPRD_MDM_ALIVE);
	SPRD_INFO("SPRD alive pin = %d\n", value);
	queue_delayed_work(sprd_notify_info.sprd_notify_wq,
					&mdm_alive_debounce_work, msecs_to_jiffies(50));
	return IRQ_HANDLED;
}

static irqreturn_t sprd_mdm_to_ap_irq_handler(int irq, void *dev)
{
	unsigned int value;
	struct sprd_notify_info *info = (struct sprd_notify_info*) dev;

	value = gpio_get_value(SPRD_MDM_TO_AP1);
	if ((SPRD_MODEM_POWER_OFF_ONGOING == info->power_status) || (sprd_notify_info.cpoff == 1)) {
		if (value == 0) {
			info->power_status = SPRD_MODEM_POWER_OFF;
			sprd_set_modem_power(0); 
			SPRD_INFO("mdm_to_ap pin is low\n");
			if (list_empty(&(info->wait_power_off_wq.task_list))) {
			return IRQ_HANDLED;
			}
			wake_up_interruptible(&(info->wait_power_off_wq));
				return IRQ_HANDLED;
		} else {
			SPRD_INFO("mdm_to_ap pin is high\n");
		}
	} else {
		SPRD_INFO("Others mdm_to_ap pin = %d\n", value);
	}

	return IRQ_HANDLED;
}

static int sprd_gpio_free_irq(unsigned int *irq, void *arg)
{
	free_irq(*irq, arg);
	return 0;
}

static int sprd_notify_driver_probe(struct platform_device *pdev)
{
	int ret = 0;
#if (defined(CONFIG_MACH_DUMMY))
	int board_ver = htc_get_board_revision();
#endif
	ret = sysfs_create_file(&pdev->dev.kobj, &dev_attr_modempower.attr);
	if (ret) {
		SPRD_ERR("%s: modempower sysfs create fail!\n", __func__);
		goto err_file_modempower;
	}

	ret = sysfs_create_file(&pdev->dev.kobj, &dev_attr_cpoff.attr);
	if (ret) {
		SPRD_ERR("%s: cpoff sysfs create fail!\n", __func__);
		goto err_file_dev_attr_cpoff;
	}
#ifdef CONFIG_SDIO_TTY_SP6502COM
	ret = sysfs_create_file(&pdev->dev.kobj, &dev_attr_sdio_detect.attr);
	if (ret) {
		SPRD_ERR("%s: sdio_detect sysfs create fail!\n", __func__);
		goto err_file_dev_attr_sdio_detect;
	}
#endif
#ifdef CONFIG_SPRD_FLASHLESS
	ret = sysfs_create_file(&pdev->dev.kobj, &dev_attr_fl_mode.attr);
	if (ret) {
		SPRD_ERR("%s: fl_mode sysfs create fail!\n", __func__);
		goto err_file_dev_attr_fl_mode;
	}
#endif
	sprd_notify_info.sprd_notify_kobj = &pdev->dev.kobj;
	sprd_notify_info.sprd_notify_wq = create_workqueue("sprd_notify");

	if (sprd_notify_info.sprd_notify_wq == NULL) {
		ret = -ENOMEM;
		SPRD_ERR("%s: Failed to create ap bp notify workqueue\n", __func__);
		goto err_create_notify_work;
	}

	sprd_notify_info.modem_alive_irq = gpio_to_irq(SPRD_MDM_ALIVE);
	ret = request_irq(sprd_notify_info.modem_alive_irq, sprd_mdm_alive_irq_handler,
				 IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
				 "alive irq", &sprd_notify_info);

	if (ret) {
		SPRD_ERR("%s: Failed to config MDM_ALVE\n", __func__);
		goto err_mdm_alive_request_irq;
	}
	set_bit(irq_mdm_alive,&sprd_notify_info.irq_state);
	irq_set_irq_wake(sprd_notify_info.modem_alive_irq,1);
	sprd_irq_set(sprd_notify_info.modem_alive_irq,irq_mdm_alive,0);
	
	sprd_notify_info.modem_to_ap_irq = gpio_to_irq(SPRD_MDM_TO_AP1);
	ret = request_irq(sprd_notify_info.modem_to_ap_irq, sprd_mdm_to_ap_irq_handler,
				 IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
				 "mdm_to_ap_irq", &sprd_notify_info);

	if (ret) {
		SPRD_ERR("%s: Failed to request MDM_AP IRQ(0x%08X)\n", __func__, ret);
		goto err_mdm_ap1_request_irq;
	}
	set_bit(irq_mdm_to_ap,&sprd_notify_info.irq_state);
	irq_set_irq_wake(sprd_notify_info.modem_to_ap_irq,1);

#ifdef SIM_DETECT
	
	sprd_notify_info.sim_detect_irq = gpio_to_irq(SPRD_SIM_DETECT);
	ret = request_irq(sprd_notify_info.sim_detect_irq, sprd_sim_detect_irq_handler,
				 IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
				 "sim_detect_irq", &sprd_notify_info);

	if (ret) {
		SPRD_ERR("%s: Failed to request sim_detect_irq(0x%08X)\n", __func__, ret);
		goto err_sim_detect_request_irq;
	}
	set_bit(irq_sim_detect,&sprd_notify_info.irq_state);
	irq_set_irq_wake(sprd_notify_info.sim_detect_irq,1);

	ret = sysfs_create_file(&pdev->dev.kobj, &dev_attr_sim_detect.attr);
	if (ret) {
		SPRD_ERR("%s: sim_detect sysfs create fail!\n", __func__);
		goto err_file_dev_attr_sim_detect;
	}
	sim_status = gpio_get_value(SPRD_SIM_DETECT);
#if (defined(CONFIG_MACH_DUMMY))
	ret = sysfs_create_file(&pdev->dev.kobj, &dev_attr_sim_switch.attr);
	if (ret) {
		SPRD_ERR("%s: sim_switch sysfs create fail!\n", __func__);
		goto err_file_dev_attr_sim_switch;
	}

	if (!board_ver) 
		sim_switch = !cpld_gpio_read(SPRD_SIM_SWITCH_EVM);
	else
		sim_switch = gpio_get_value(SPRD_SIM_SWITCH);
#endif
#endif
	ret = sysfs_create_file(&pdev->dev.kobj, &dev_attr_mdm_assert.attr);
	if (ret) {
		SPRD_ERR("%s: mdm_assert sysfs create fail!\n", __func__);
		goto err_file_dev_attr_mdm_assert;
	}

#ifdef CONFIG_SPRD_FLASHLESS
	sprd_notify_info.flashless_irq = gpio_to_irq(SPRD_MDM_FLASHLESS);
	ret = request_irq(sprd_notify_info.flashless_irq, sprd_mdm_flashless_irq_handler,
				 IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
				 "flashless_irq", &sprd_notify_info);

	if (ret) {
		SPRD_ERR("%s: Failed to config MDM_FLASHLESS\n", __func__);
		goto err_enable_mdm_flashless_irq;
	}
	set_bit(irq_flashlessd,&sprd_notify_info.irq_state);
	irq_set_irq_wake(sprd_notify_info.flashless_irq,1);
	sprd_irq_set(sprd_notify_info.flashless_irq,irq_flashlessd,0);
#ifdef SDIO_SLAVE_RESET
	sprd_notify_info.reset_irq = gpio_to_irq(SPRD_SDIO_RESET_INIT);
	ret = request_irq(sprd_notify_info.reset_irq, sprd_mdm_reset_irq_handler,
				 IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
				 "salve_reset_irq", &sprd_notify_info);
	if (ret) {
		SPRD_ERR("%s: Failed to config MDM_RESET\n", __func__);
		goto err_enable_mdm_reset_irq;
	}
	set_bit(irq_slave_reset,&sprd_notify_info.irq_state);
	irq_set_irq_wake(sprd_notify_info.reset_irq,1);
	sprd_irq_set(sprd_notify_info.reset_irq,irq_slave_reset,0);
#endif
#endif
	init_waitqueue_head(&sprd_notify_info.wait_power_off_wq);
	init_waitqueue_head(&sprd_notify_info.wait_power_on_wq);
#ifdef CONFIG_SPRD_FLASHLESS
	sprd_alive_wq = create_workqueue("sprd_alive");
	sprd_detect_wq = create_workqueue("sprd_detect");
	sprd_uart_wq = create_workqueue("sprd_uart_tx");
#endif
	goto out_ok;

#ifdef CONFIG_SPRD_FLASHLESS
#ifdef SDIO_SLAVE_RESET
err_enable_mdm_reset_irq:
	sprd_gpio_free_irq(&(sprd_notify_info.flashless_irq), NULL);
#endif
err_enable_mdm_flashless_irq:
#endif
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_mdm_assert.attr);
err_file_dev_attr_mdm_assert:
#ifdef SIM_DETECT
#if (defined(CONFIG_MACH_DUMMY))
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_sim_switch.attr);
err_file_dev_attr_sim_switch:
#endif
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_sim_detect.attr);
err_file_dev_attr_sim_detect:
	sprd_gpio_free_irq(&(sprd_notify_info.sim_detect_irq), NULL);
err_sim_detect_request_irq:
	sprd_gpio_free_irq(&(sprd_notify_info.modem_to_ap_irq), NULL);
#endif

err_mdm_ap1_request_irq:

	sprd_gpio_free_irq(&(sprd_notify_info.modem_alive_irq), NULL);

err_mdm_alive_request_irq:

	destroy_workqueue(sprd_notify_info.sprd_notify_wq);

err_create_notify_work:
#ifdef CONFIG_SPRD_FLASHLESS
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_fl_mode.attr);
err_file_dev_attr_fl_mode:
#endif
#ifdef CONFIG_SDIO_TTY_SP6502COM
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_sdio_detect.attr);
err_file_dev_attr_sdio_detect:
#endif
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_cpoff.attr);

err_file_dev_attr_cpoff:
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_modempower.attr);

err_file_modempower:
out_ok:

	sprd_notify_info.power_status = SPRD_MODEM_POWER_OFF;
	sprd_notify_info.state = SPRD_STATUS_IDLE;
	sprd_notify_info.cpoff = 0;

	return ret;
}

#ifdef CONFIG_PM
static int sprd_notify_suspend(struct platform_device *pdev, pm_message_t state)
{
	if (sprd_notify_info.power_status == SPRD_MODEM_POWER_ON) {
		enable_irq_wake(gpio_to_irq(SPRD_MDM_ALIVE));
	}
	return 0;
}
static int sprd_notify_resume(struct platform_device *pdev)
{
	if (sprd_notify_info.power_status == SPRD_MODEM_POWER_ON) {
		disable_irq_wake(gpio_to_irq(SPRD_MDM_ALIVE));
	}
	return 0;
}
#else
static int sprd_notify_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int sprd_notify_resume(struct platform_device *pdev)
{
	return 0;
}
#endif

static int sprd_notify_driver_remove(struct platform_device *pdev)
{
#ifdef CONFIG_SPRD_FLASHLESS
#ifdef SDIO_SLAVE_RESET
	sprd_gpio_free_irq(&sprd_notify_info.reset_irq, NULL);
#endif
	sprd_gpio_free_irq(&sprd_notify_info.flashless_irq, NULL);
#endif
	sprd_gpio_free_irq(&sprd_notify_info.modem_alive_irq, NULL);
	sprd_gpio_free_irq(&sprd_notify_info.modem_to_ap_irq, NULL);
#ifdef SIM_DETECT
	sprd_gpio_free_irq(&sprd_notify_info.sim_detect_irq, NULL);
#endif
	destroy_workqueue(sprd_notify_info.sprd_notify_wq);
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_mdm_assert.attr);
#ifdef SIM_DETECT
#if (defined(CONFIG_MACH_DUMMY))
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_sim_switch.attr);
#endif
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_sim_detect.attr);
#endif
#ifdef CONFIG_SPRD_FLASHLESS
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_fl_mode.attr);
#endif
#ifdef CONFIG_SDIO_TTY_SP6502COM
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_sdio_detect.attr);
#endif
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_cpoff.attr);
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_modempower.attr);
	return 0;
}

static struct platform_driver sprd_notify_driver = {
	.probe = sprd_notify_driver_probe,
	.remove = sprd_notify_driver_remove,
	.suspend = sprd_notify_suspend,
	.resume = sprd_notify_resume,
	.driver = {
		.name	= "mdm_sprd",
		.owner	= THIS_MODULE,
	},
};

static int __init sprd_notify_init(void)
{
	int ret;

	ret = platform_driver_register(&sprd_notify_driver);

	sprd_flymode_pin_config();
	
	gpio_tlmm_config(GPIO_CFG(SPRD_BB_VDD_EN, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
	gpio_set_value(SPRD_BB_VDD_EN, 0);

	return ret;
}

static void __exit sprd_notify_exit(void)
{
	platform_driver_unregister(&sprd_notify_driver);
}

module_init(sprd_notify_init);
module_exit(sprd_notify_exit);

MODULE_DESCRIPTION("sprd modem notify");
MODULE_LICENSE("GPL");
