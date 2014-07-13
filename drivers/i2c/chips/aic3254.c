/* drivers/i2c/chips/aic3254.c - aic3254 Codec driver
 *
 * Copyright (C) 2008-2012 HTC Corporation.
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
 * This program is modified according to drivers/spi/spi_aic3254.c
 * Some labels are change:
 * 	spi_aic3254.c				this_program
 *
 *	codec_dev		-->		aic3254_client
 *	codec_drv->idlelock	-->		aic3254_idlelock
 *	codec_drv->wakelock	-->		aic3254_wakelock
 *	pr_aud_info		-->		AUD_INF
 *	pr_aud_err		-->		AUD_ERR
 *	_CODEC_SPI_CMD		-->		aic3254_cmd
 *	CODEC_SPI_CMD		-->		struct aic3254_cmd
 *	AIC3254_PARAM		-->		aic3254_param
 *
 * basic function change:
 *	spi_aic3254.c				this_program
 *
 *	codec_spi_read		-->		aic3254_i2c_read_1byte
 *	codec_spi_write		-->		aic3254_i2c_write_1byte
 *
 * Owner: Octagram Sun (octagram_sun@htc.com) HTCC_SSD3 Audio_Team
 * Version: alpha 1
 */

#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/input.h>
#include <asm/mach-types.h>
#include <linux/i2c/aic3254.h>
#include <linux/i2c/aic3254_reg.h>
#include <asm/gpio.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <mach/vreg.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/module.h>
#if defined(CONFIG_CPLD) && (defined(CONFIG_MACH_CP3DUG) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY))
#include <linux/i2c/cpld.h>
#endif

#define AUD_DBG(x...) 
#define AUD_INF(x...) printk(KERN_INFO "[AUD][aic3254] " x)
#define AUD_ERR(x...) printk(KERN_ERR "[AUD][aic3254 ERR] " x)

static struct aic3254_cmd **aic3254_uplink;
static struct aic3254_cmd **aic3254_downlink;
static struct aic3254_cmd **aic3254_minidsp;

static int suspend_flag;
static int aic3254_existed = false;
static int aic3254_opend = false;

static int aic3254_tx_mode;
#if 0
static int aic3254_rx_mode;
#endif

static struct i2c_msg aic3254_i2c_msg[2];
static struct mutex aic3254_i2c_mutex;

static struct i2c_client *aic3254_client = NULL;
static struct mutex aic3254_mutex;
static struct wake_lock aic3254_wakelock;

#if 0

static unsigned short mic_enabled_count = 0;
static struct aic3254_ctl_ops default_ctl_ops;
static struct aic3254_ctl_ops *ctl_ops = &default_ctl_ops;

int route_tx_enable(int, int);
int route_rx_enable(int, int);
static void spi_aic3254_prevent_sleep(void);
static void spi_aic3254_allow_sleep(void);

#endif

static int aic3254_i2c_write(uint8_t *buf, uint8_t num)
{
	int ret;

	mutex_lock(&aic3254_i2c_mutex);

	
	aic3254_i2c_msg[0].len = num;
	aic3254_i2c_msg[0].flags = 0;
	aic3254_i2c_msg[0].buf = buf;
	ret = i2c_transfer(aic3254_client->adapter, aic3254_i2c_msg, 1);
	mutex_unlock(&aic3254_i2c_mutex);

	
	if (ret >= 0)
		return 0;
	else {
		AUD_DBG("%s: Failed! (ret = %d)\n", __func__, ret);
		return -EIO;
	}
}

static int aic3254_i2c_write_1byte(uint8_t reg, uint8_t data)
{
	uint8_t buf[2];
	buf[0] = reg;
	buf[1] = data;

	return aic3254_i2c_write(buf, 2);
}

static int aic3254_i2c_read(uint8_t reg, uint8_t *buf, uint8_t num)
{
	int ret;
	uint8_t index;

	mutex_lock(&(aic3254_i2c_mutex));
	
	aic3254_i2c_msg[0].len = 1;
	aic3254_i2c_msg[0].flags = 0; 
	index = reg;
	aic3254_i2c_msg[0].buf = &index;
	
	aic3254_i2c_msg[1].flags = I2C_M_RD;  
	aic3254_i2c_msg[1].len = num;   
	aic3254_i2c_msg[1].buf = buf;
	ret = i2c_transfer(aic3254_client->adapter, aic3254_i2c_msg, 2);
	mutex_unlock(&(aic3254_i2c_mutex));

	
	if (ret >= 0)
		return 0;
	else {
		AUD_DBG("%s: Failed! (ret = %d)\n", __func__, ret);
		return -EIO;
	}
}

#define aic3254_i2c_read_1byte(r, b)  aic3254_i2c_read(r, b, 1)


#ifdef	CONFIG_PROC_FS
#define AIC3254_PROC_FILE		"driver/aic3254"
static struct proc_dir_entry *aic3254_proc_dbg_file;

static enum {
	AUD_DBG_REG_DUMP = 1,
	AUD_DBG_ARRAY_DUMP,
} aic3254_dbg_mode = AUD_DBG_REG_DUMP;

static uint8_t aic3254_dbg_reg_index = 0x0;
static uint8_t aic3254_dbg_reg_val[AIC3254_REG_NUM] = {0};

static struct aic3254_cmd **aic3254_dbg_array = NULL;
static char aic3254_dbg_array_name[70];
static int aic3254_dbg_array_row = 0;
static int aic3254_dbg_array_col = 0;


static ssize_t aic3254_proc_read(struct file *filp,
	char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len;
	char buf[512];

	switch (aic3254_dbg_mode) {
	case AUD_DBG_ARRAY_DUMP:
		if (aic3254_dbg_array == NULL) {
			len = sprintf(buf, "Error! Please appoint a valid array to be dumped.\n");
			aic3254_dbg_mode = AUD_DBG_REG_DUMP;
		}
		else {
			len = sprintf(buf, "%s[%d][%d]: act = %c; reg = %d (0x%X); data = 0x%X\n",
					aic3254_dbg_array_name, aic3254_dbg_array_row, aic3254_dbg_array_col,
					aic3254_dbg_array[aic3254_dbg_array_row][aic3254_dbg_array_col].act,
					aic3254_dbg_array[aic3254_dbg_array_row][aic3254_dbg_array_col].reg,
					aic3254_dbg_array[aic3254_dbg_array_row][aic3254_dbg_array_col].reg,
					aic3254_dbg_array[aic3254_dbg_array_row][aic3254_dbg_array_col].data);	
		}
		break;
	case AUD_DBG_REG_DUMP:
	default:
		if (aic3254_dbg_reg_index < 0 || aic3254_dbg_reg_index > AIC3254_REG_NUM) {
			len = sprintf(buf, "Error! Range of aic3254 reg index: 0x00 ~ 0x0xFF.\n");
		} else {
			aic3254_i2c_read(aic3254_dbg_reg_index, &aic3254_dbg_reg_val[aic3254_dbg_reg_index], 1);
			len = sprintf(buf, "register 0x%02x: 0x%02x\n", 
					aic3254_dbg_reg_index, aic3254_dbg_reg_val[aic3254_dbg_reg_index]);
		}
		break;
	}

	return simple_read_from_buffer(buffer, count, offset, buf, len);
}


struct vreg {
	const char *name;
	unsigned id;
	int status;
	unsigned refcnt;
};

#define VREG(_name, _id, _status, _refcnt) \
	{ .name = _name, .id = _id, .status = _status, .refcnt = _refcnt }

static struct vreg vregs[] = {
	VREG("msma",	0, 0, 0),
	VREG("msmp",	1, 0, 0),
	VREG("msme1",	2, 0, 0),
	VREG("msmc1",	3, 0, 0),
	VREG("msmc2",	4, 0, 0),
	VREG("gp3",	5, 0, 0),
	VREG("msme2",	6, 0, 0),
	VREG("gp4",	7, 0, 0),
	VREG("gp1",	8, 0, 0),
	VREG("tcxo",	9, 0, 0),
	VREG("pa",	10, 0, 0),
	VREG("rftx",	11, 0, 0),
	VREG("rfrx1",	12, 0, 0),
	VREG("rfrx2",	13, 0, 0),
	VREG("synt",	14, 0, 0),
	VREG("wlan",	15, 0, 0),
	VREG("usb",	16, 0, 0),
	VREG("boost",	17, 0, 0),
	VREG("mmc",	18, 0, 0),
	VREG("ruim",	19, 0, 0),
	VREG("msmc0",	20, 0, 0),
	VREG("gp2",	21, 0, 0),
	VREG("gp5",	22, 0, 0),
	VREG("gp6",	23, 0, 0),
	VREG("rf",	24, 0, 0),
	VREG("rf_vco",	26, 0, 0),
	VREG("mpll",	27, 0, 0),
	VREG("s2",	28, 0, 0),
	VREG("s3",	29, 0, 0),
	VREG("rfubm",	30, 0, 0),
	VREG("ncp",	31, 0, 0),
	VREG("gp7",	32, 0, 0),
	VREG("gp8",	33, 0, 0),
	VREG("gp9",	34, 0, 0),
	VREG("gp10",	35, 0, 0),
	VREG("gp11",	36, 0, 0),
	VREG("gp12",	37, 0, 0),
	VREG("gp13",	38, 0, 0),
	VREG("gp14",	39, 0, 0),
	VREG("gp15",	40, 0, 0),
	VREG("gp16",	41, 0, 0),
	VREG("gp17",	42, 0, 0),
	VREG("s4",	43, 0, 0),
	VREG("usb2",	44, 0, 0),
	VREG("wlan2",	45, 0, 0),
	VREG("xo_out",	46, 0, 0),
	VREG("lvsw0",	47, 0, 0),
	VREG("lvsw1",	48, 0, 0),
	VREG("mddi",	49, 0, 0),
	VREG("pllx",	50, 0, 0),
	VREG("wlan3",	51, 0, 0),
	VREG("emmc",	52, 0,	0),
	VREG("wlan_tcx0", 53, 0, 0),
	VREG("usim2",	54, 0, 0),
	VREG("usim",	55, 0, 0),
	VREG("bt",	56, 0, 0),
	VREG("wlan4",	57, 0, 0),
};

static ssize_t aic3254_proc_write(struct file *filp,
		const char *buff, size_t len, loff_t *off)
{
	uint8_t buf[2];
	char messages[80], val[80], sep[] = ",";
	char *sz1 = NULL, *sz2 = NULL, *sz3 = NULL;
	char **psz;
	struct vreg *vreg = NULL;
	int id;
	uint8_t i, rdbuf;

	if (len > 80)
		len = 80;

	if (copy_from_user(messages, buff, len))
		return -EFAULT;

	if ('*' == messages[0]) {
		aic3254_dbg_mode = AUD_DBG_REG_DUMP;
		
		memcpy(val, messages+1, len-1);
		aic3254_dbg_reg_index = (int) simple_strtoul(val, NULL, 16);
		AUD_DBG("aic3254_proc_write:register # was set = 0x%x\n",
				aic3254_dbg_reg_index);
	} else if ('-' == messages[0]) {
		memcpy(val, messages+1, len-1);
		id = (int) simple_strtoul(val, NULL, 16);
		vreg = vreg_get(NULL, vregs[id].name);
		vreg_disable(vreg);
		AUD_DBG("Close power %d : %s\n", id, vregs[id].name);
	} else if ('+' == messages[0]) {
		memcpy(val, messages+1, len-1);
		id = (int) simple_strtoul(val, NULL, 16);
		vreg = vreg_get(NULL, vregs[id].name);
		vreg_enable(vreg);
		AUD_DBG("Open power %d : %s\n", id, vregs[id].name);
	} else if (('d' == messages[0]) && ('+' == messages[1]) && (',' == messages[2])) {
		aic3254_dbg_mode = AUD_DBG_ARRAY_DUMP;
		sz3 = &messages[3];
		psz = &sz3;
		sz1 = strsep(psz, sep);
		sz2 = strsep(psz, sep);
		strcpy(aic3254_dbg_array_name, sz1);
		if (strcmp(sz1, "uplink") == 0)
			aic3254_dbg_array = aic3254_uplink;
		else if (strcmp(sz1, "downlink") == 0)
			aic3254_dbg_array = aic3254_downlink;
		else if (strcmp(sz1, "minidsp") == 0)
			aic3254_dbg_array = aic3254_minidsp;
		else
			aic3254_dbg_array = NULL;
		if (sz2 != NULL)
			aic3254_dbg_array_row = simple_strtoul(sz2, NULL, 10);
		if (sz3 != NULL)
			aic3254_dbg_array_col = simple_strtoul(sz3, NULL, 10);
	} else if (('d' == messages[0]) && ('-' == messages[1])) {
		aic3254_dbg_mode = AUD_DBG_REG_DUMP;
	} else if ('#' == messages[0]) {
		memcpy(val, messages+1, len-1);
		buf[1] = (uint8_t) simple_strtoul(val, NULL, 16);
		buf[0] = 0x0;
		AUD_DBG("dump reg page = %d\n", buf[1]);
		aic3254_i2c_write(buf, 2);
		for(i = 0; i < 128; i ++) {
			aic3254_i2c_read(i, &rdbuf, 1);
			AUD_INF("reg[0x%X (%d)] = 0x%X (%d)\n", i, i, rdbuf, rdbuf);
		}

	} else if (isdigit(messages[0]) && aic3254_dbg_mode == AUD_DBG_REG_DUMP) {
		
		buf[0] = aic3254_dbg_reg_index;
		buf[1] = (uint8_t)(simple_strtoul(messages, NULL, 16) & 0xFF);
		aic3254_i2c_write(buf, 2);
		AUD_DBG("aic3254_proc_write:register write 0x%x <- %x\n",
				aic3254_dbg_reg_index, buf[1]);
	} else {
		AUD_DBG("Bad command or mode!\n");
	}
	return len;
}

static const struct file_operations aic3254_proc_ops = {
	.read = aic3254_proc_read,
	.write = aic3254_proc_write,
};

static void aic3254_create_proc_file(void)
{
	aic3254_proc_dbg_file = create_proc_entry(AIC3254_PROC_FILE, 0644, NULL);
	if (aic3254_proc_dbg_file)
		aic3254_proc_dbg_file->proc_fops = &aic3254_proc_ops;
	else
		AUD_ERR("proc file create failed!\n");
}

#define aic3254_remove_proc_file()	remove_proc_entry(AIC3254_PROC_FILE, NULL)

#endif


static void aic3254_prevent_sleep(void)
{
	wake_lock(&aic3254_wakelock);
	
}

static void aic3254_allow_sleep(void)
{
	
	wake_unlock(&aic3254_wakelock);
}

static int32_t aic3254_write_table(struct aic3254_cmd *cmds, int num)
{
	int i;
	uint8_t *tx;
	int ret = 0;
	tx = kcalloc(num<<1, sizeof(uint8_t), GFP_KERNEL);

	if (!tx) {
		AUD_ERR("%s: Error allocating memory\n", __func__);
		return -1;
	}

	for (i = 0; i < num ; i++) {
		tx[i * 2] = cmds[i].reg;
		tx[i * 2 + 1] = cmds[i].data;
	}

	if (aic3254_client == NULL)
		ret = -ESHUTDOWN;
	else
		ret = aic3254_i2c_write(tx, num<<1);

	kfree(tx);
	return ret;
}

static int aic3254_config(struct aic3254_cmd *cmds, int size)
{
	int i, retry, ret;
	unsigned char data;

	if (aic3254_client == NULL) {
		AUD_ERR("%s: no i2c device\n", __func__);
		return -EFAULT;
	}

	if (cmds == NULL) {
		AUD_ERR("%s: invalid reg parameters\n", __func__);
		return -EINVAL;
	}

	
	
	if (size < 1024) {
		for (i = 0; i < size; i++) {
			switch (cmds[i].act) {
			case 'w':
				aic3254_i2c_write_1byte(cmds[i].reg, cmds[i].data);
				break;
			case 'r':
				for (retry = AIC3254_MAX_RETRY; retry > 0; retry--) {
					ret = aic3254_i2c_read_1byte(cmds[i].reg, &data);
					if (ret < 0)
						AUD_ERR("%s: read fail %d, retry\n",
							__func__, ret);
					else if (data == cmds[i].data)
						break;
					msleep(1);
				}
				if (retry <= 0)
					AUD_ERR("aic3254 power down procedure"
						" ,flag 0x%02X=0x%02X(0x%02X)\n",
						cmds[i].reg,
						ret, cmds[i].data);
				break;
			case 'd':
				msleep(cmds[i].data);
				break;
			default:
				break;
			}
		}
	} else {
		
		aic3254_write_table(cmds, size);
	}
	return 0;
}

static void aic3254_tx_config(int mode)
{
	
	if (aic3254_uplink == NULL) {
		AUD_ERR("%s: aic3254_uplink is NULL\n", __func__);
#if 0
		if (mode == UPLINK_OFF)
			route_tx_enable(mode, 0);
		else
			route_tx_enable(mode, 1);
#endif
		return;
	}

	if (mode != UPLINK_OFF && mode != POWER_OFF) {
		
		AUD_INF("uplink wakeup len(%d)\n",
			(aic3254_uplink[UPLINK_WAKEUP][0].data - 1));
		aic3254_config(
			&aic3254_uplink[UPLINK_WAKEUP][1],
			aic3254_uplink[UPLINK_WAKEUP][0].data - 1);
	}

	
	AUD_INF("uplink TX %d len(%d)\n", mode,
		(aic3254_uplink[mode][0].data - 1));
	aic3254_config(&aic3254_uplink[mode][1],
			aic3254_uplink[mode][0].data - 1);
}

#if 0
static void aic3254_rx_config(int mode)
{
	
	if (aic3254_downlink == NULL) {
		if (mode == DOWNLINK_OFF)
			route_rx_enable(mode, 0);
		else
			route_rx_enable(mode, 1);
		return;
	}

	if (mode != DOWNLINK_OFF && mode != POWER_OFF) {
		
		AUD_INF("downlink wakeup len(%d)\n",
			(aic3254_downlink[DOWNLINK_WAKEUP][0].data-1));
		aic3254_config(
			&aic3254_downlink[DOWNLINK_WAKEUP][1],
			aic3254_downlink[DOWNLINK_WAKEUP][0].data);
	}

	
	AUD_INF("downlink RX %d len(%d)\n", mode,
		(aic3254_downlink[mode][0].data-1));
	aic3254_config(&aic3254_downlink[mode][1],
				aic3254_downlink[mode][0].data);
}
#endif

static void aic3254_powerdown(void)
{
	int64_t t1, t2;

	if (aic3254_tx_mode != UPLINK_OFF)
		return;

	t1 = ktime_to_ms(ktime_get());

	aic3254_prevent_sleep();
	if (aic3254_uplink != NULL) {
		AUD_INF("power off AIC3254 len(%d)++\n",
			(aic3254_uplink[POWER_OFF][0].data-1));
		aic3254_config(&aic3254_uplink[POWER_OFF][1],
				aic3254_uplink[POWER_OFF][0].data);
	} else {
		AUD_INF("power off AIC3254 len(%d)++\n",
			(ARRAY_SIZE(CODEC_POWER_OFF)));
		aic3254_config(CODEC_POWER_OFF, ARRAY_SIZE(CODEC_POWER_OFF));
	}

	aic3254_allow_sleep();

	t2 = ktime_to_ms(ktime_get())-t1;
	AUD_INF("power off AIC3254 %lldms --\n", t2);
	return;
}

static void aic3254_loopback(int mode)
{
#if 0
	if (!(ctl_ops->lb_dsp_init &&
		ctl_ops->lb_receiver_imic &&
		ctl_ops->lb_speaker_imic &&
		ctl_ops->lb_headset_emic)) {
		AUD_INF("%s: AIC3254 LOOPBACK not supported\n", __func__);
		return;
	}

	
	aic3254_config(ctl_ops->lb_dsp_init->data, ctl_ops->lb_dsp_init->len);

	AUD_INF("%s: set AIC3254 in LOOPBACK mode\n", __func__);
	switch (mode) {
	case 0:
		
		aic3254_config(ctl_ops->lb_receiver_imic->data,
				ctl_ops->lb_receiver_imic->len);
		break;
	case 1:
		
		aic3254_config(ctl_ops->lb_speaker_imic->data,
				ctl_ops->lb_speaker_imic->len);
		break;
	case 2:
		
		aic3254_config(ctl_ops->lb_headset_emic->data,
				ctl_ops->lb_headset_emic->len);
		break;
	case 13:
		
		if (ctl_ops->lb_receiver_bmic)
			aic3254_config(ctl_ops->lb_receiver_bmic->data,
				ctl_ops->lb_receiver_bmic->len);
		else
			AUD_INF("%s: receiver v.s. 2nd mic loopback not supported\n", __func__);
		break;

	case 14:
		
		if (ctl_ops->lb_speaker_bmic)
			aic3254_config(ctl_ops->lb_speaker_bmic->data,
				ctl_ops->lb_speaker_bmic->len);
		else
			AUD_INF("%s: speaker v.s. 2nd mic loopback not supported\n", __func__);
		break;

	case 15:
		
		if (ctl_ops->lb_headset_bmic)
			aic3254_config(ctl_ops->lb_headset_bmic->data,
				ctl_ops->lb_headset_bmic->len);
		else
			AUD_INF("%s: headset v.s. 2nd mic loopback not supported\n", __func__);
		break;
	default:
		break;
	}
#endif
}

#if 0
int route_rx_enable(int path, int en)
{
	AUD_INF("%s: (%d,%d) uses 3254 default setting\n", __func__, path, en);
	if (en) {
		
		aic3254_config(CODEC_DOWNLINK_ON,
				ARRAY_SIZE(CODEC_DOWNLINK_ON));
		
		switch (path) {
		case FM_OUT_HEADSET:
			
			aic3254_config(FM_In_Headphone,
					ARRAY_SIZE(FM_In_Headphone));
			aic3254_config(FM_Out_Headphone,
					ARRAY_SIZE(FM_Out_Headphone));
			break;
		case FM_OUT_SPEAKER:
			
			aic3254_config(FM_In_SPK,
					ARRAY_SIZE(FM_In_SPK));
			aic3254_config(FM_Out_SPK,
					ARRAY_SIZE(FM_Out_SPK));
			break;
		default:
			
			aic3254_config(Downlink_IMIC_Receiver,
					ARRAY_SIZE(Downlink_IMIC_Receiver));
			break;
		}
	} else {
		
		aic3254_config(CODEC_DOWNLINK_OFF,
				ARRAY_SIZE(CODEC_DOWNLINK_OFF));
	}

	return 0;
}
#endif

#if 0
int route_tx_enable(int path, int en)
{
	AUD_INF("%s: (%d,%d) uses 3254 default setting\n", __func__, path, en);
	if (en) {
		
		aic3254_config(CODEC_UPLINK_ON, ARRAY_SIZE(CODEC_UPLINK_ON));
		
		switch (path) {
		case CALL_UPLINK_IMIC_RECEIVER:
		case CALL_UPLINK_IMIC_HEADSET:
		case CALL_UPLINK_IMIC_SPEAKER:
		case VOICERECORD_IMIC:
			
			aic3254_config(MECHA_Uplink_IMIC,
					ARRAY_SIZE(MECHA_Uplink_IMIC));
			break;
		case CALL_UPLINK_EMIC_HEADSET:
		case VOICERECORD_EMIC:
			aic3254_config(Uplink_EMIC,
					ARRAY_SIZE(Uplink_EMIC));
			break;
		}
	} else {
		
		aic3254_config(CODEC_UPLINK_OFF, ARRAY_SIZE(CODEC_UPLINK_OFF));
	}

	return 0;
}
#endif

#if 0
void aic3254_set_mic_bias(int en)
{
	if (en)
		aic3254_config(CODEC_MICBIAS_ON, ARRAY_SIZE(CODEC_MICBIAS_ON));
	else
		aic3254_config(CODEC_MICBIAS_OFF, ARRAY_SIZE(CODEC_MICBIAS_OFF));
}
#endif

static int aic3254_set_config(int config_tbl, int idx, int en)
{
	int rc = 0;

	mutex_lock(&aic3254_mutex);
	aic3254_prevent_sleep();

	switch (config_tbl) {
	case AIC3254_CONFIG_TX:
		
		AUD_INF("%s: enable tx\n", __func__);
		if (en) {
#if 0
			if (ctl_ops->tx_amp_enable)
				ctl_ops->tx_amp_enable(0);
#endif
			aic3254_tx_config(idx);
			aic3254_tx_mode = idx;
#if 0
			if (ctl_ops->tx_amp_enable)
				ctl_ops->tx_amp_enable(1);
#endif
		} else {
			aic3254_tx_config(UPLINK_OFF);
			aic3254_tx_mode = UPLINK_OFF;
		}
		break;

#if 0
	case AIC3254_CONFIG_RX:
		
		AUD_INF("%s: enable rx\n", __func__);
		if (en) {
#if 0
			if (ctl_ops->rx_amp_enable)
				ctl_ops->rx_amp_enable(0);
#endif
			aic3254_rx_config(idx);
			aic3254_rx_mode = idx;
#if 0
			if (ctl_ops->rx_amp_enable)
				ctl_ops->rx_amp_enable(1);
#endif
		} else {
			aic3254_rx_config(DOWNLINK_OFF);
			aic3254_rx_mode = DOWNLINK_OFF;
		}
		break;
#endif
#if 0
	case AIC3254_CONFIG_MEDIA:
		if (aic3254_minidsp == NULL) {
			rc = -EFAULT;
			break;
		}

		len = (aic3254_minidsp[idx][0].reg << 8)
			| aic3254_minidsp[idx][0].data;

		AUD_INF("%s: configure miniDSP index(%d) len = %d ++\n",
			__func__, idx, len);
		AUD_INF("%s: rx mode %d, tx mode %d\n",
			__func__, aic3254_rx_mode, aic3254_tx_mode);

		t1 = ktime_to_ms(ktime_get());

		if (ctl_ops->rx_amp_enable)
			ctl_ops->rx_amp_enable(0);

		
		if (aic3254_rx_mode != DOWNLINK_OFF)
			aic3254_rx_config(DOWNLINK_OFF);

		
		aic3254_config(&aic3254_minidsp[idx][1], len);

		
		if (aic3254_rx_mode != DOWNLINK_OFF)
			aic3254_rx_config(aic3254_rx_mode);
		if (aic3254_tx_mode != UPLINK_OFF)
			aic3254_tx_config(aic3254_tx_mode);

		t2 = ktime_to_ms(ktime_get())-t1;

		if (ctl_ops->rx_amp_enable)
			ctl_ops->rx_amp_enable(1);

		AUD_INF("%s: configure miniDSP index(%d) time: %lldms --\n",
			__func__, idx, (t2));
		break;
#endif
	}

	aic3254_allow_sleep();
	mutex_unlock(&aic3254_mutex);
	return rc;
}

static int aic3254_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	AUD_DBG("aic3254 driver: %s -- %s\n", __FILE__ ,__func__);

	mutex_lock(&aic3254_mutex);
	if (aic3254_opend) {
		AUD_ERR("%s: busy\n", __func__);
		ret = -EBUSY;
	} else
		aic3254_opend = 1;
	mutex_unlock(&aic3254_mutex);

	return ret;

}

static int aic3254_release(struct inode *inode, struct file *file)
{
	AUD_DBG("aic3254 driver: %s -- %s\n", __FILE__ ,__func__);

	mutex_lock(&aic3254_mutex);
	aic3254_opend = 0;
	mutex_unlock(&aic3254_mutex);

	return 0;
}

#if 0
void aic3254_set_mode(int config, int mode)
{
	mutex_lock(&aic3254_mutex);
	spi_aic3254_prevent_sleep();

	if (mode == UPLINK_OFF)	{
		if (mic_enabled_count > 0)
			mic_enabled_count--;
		else
			AUD_ERR("%s: mic_enabled_count already zero", __func__);

		if (mic_enabled_count > 0) {
			spi_aic3254_allow_sleep();
			mutex_unlock(&aic3254_mutex);
			return;
		}
		AUD_INF("%s: turn off 3254", __func__);
	}

	if (mode == VOICERECORD_IMIC || mode == VOICERECORD_EMIC)
		mic_enabled_count++;

	AUD_INF("%s: count = %d", __func__, mic_enabled_count);

#if defined(CONFIG_ARCH_U8500)
	struct ecodec_aic3254_state *drv = &codec_clk;
	AUD_INF("%s: try to enable MCLK: mclk enabled = %d\n", __func__, drv->enabled);
		if (drv->enabled == 0) {
			
			clk_enable(drv->tx_mclk);
			AUD_INF("%s: enable MCLK and delay 5ms\n", __func__);
			drv->enabled = 1;
			mdelay(5);
		}
#endif

	switch (config) {
	case AIC3254_CONFIG_TX:
		
		AUD_INF("%s: AIC3254_CONFIG_TX mode = %d\n",
			__func__, mode);
		aic3254_tx_config(mode);
		aic3254_tx_mode = mode;
		break;
	case AIC3254_CONFIG_RX:
		
		AUD_INF("%s: AIC3254_CONFIG_RX mode = %d\n",
			__func__, mode);
		aic3254_rx_config(mode);
		if (mode == FM_OUT_SPEAKER)
			aic3254_tx_config(FM_IN_SPEAKER);
		else if (mode == FM_OUT_HEADSET)
			aic3254_tx_config(FM_IN_HEADSET);
		aic3254_rx_mode = mode;
		break;
	}

	aic3254_powerdown();

	spi_aic3254_allow_sleep();
	mutex_unlock(&aic3254_mutex);
}

int aic3254_get_state(void)
{
	return aic3254_existed;
}
#endif

static long aic3254_ioctl(struct file *file, unsigned int cmd,
				unsigned long argc)
{

	struct aic3254_param para;
	void *table;
	int ret = 0, i = 0, mem_size, volume = 0;
	struct aic3254_cmd reg[2];
	unsigned char data;
	struct aic3254_cmd *cmd_ptr;
	char ctable[4];

	int table_num;

	
	AUD_DBG("aic3254 driver: %s -- %s\n", __FILE__ ,__func__);
	AUD_DBG("%s: cmd %d\n", __func__, cmd);

	if (aic3254_uplink == NULL ||
		aic3254_downlink == NULL ||
		aic3254_minidsp == NULL) {
		AUD_ERR("%s: cmd 0x%x, invalid pointers\n", __func__, cmd);
		return -EFAULT;
	}

	switch (cmd) {
	case AIC3254_SET_TX_PARAM:
	case AIC3254_SET_RX_PARAM:
		if (copy_from_user(&para, (void *)argc, sizeof(para))) {
			AUD_ERR("%s: failed on copy_from_user\n", __func__);
			return -EFAULT;
		}

		AUD_DBG("%s: parameters(%d, %d, %p)\n", __func__,
				para.row_num, para.col_num, para.cmd_data);
		cmd_ptr = (struct aic3254_cmd *)para.cmd_data;
		AUD_DBG("%s: cmd[0]{act, reg, data} = {%c, %d, %x}\n", __func__,
				cmd_ptr[0].act, cmd_ptr[0].reg, cmd_ptr[0].data);
		if (cmd == AIC3254_SET_TX_PARAM)
			table = aic3254_uplink[0];
		else
			table = aic3254_downlink[0];

		
		if (para.row_num > IO_CTL_ROW_MAX
				|| para.col_num != IO_CTL_COL_MAX) {
			AUD_ERR("%s: data size mismatch with allocated"
					" memory (%d,%d)\n", __func__,
					IO_CTL_ROW_MAX, IO_CTL_COL_MAX);
			return -EFAULT;
		}

		mem_size = para.row_num * para.col_num * sizeof(struct aic3254_cmd);
		if (copy_from_user(table, para.cmd_data, mem_size)) {
			AUD_ERR("%s: failed on copy_from_user\n", __func__);
			return -EFAULT;
		}

#if 0
		
		if (cmd == AIC3254_SET_TX_PARAM)
			aic3254_tx_config(INITIAL);
#endif

		AUD_DBG("%s: update table(%d,%d) successfully\n",
				__func__, para.row_num, para.col_num);
		break;

	case AIC3254_WRITE_ARRAY:
		memset(&ctable,'\0',sizeof(ctable));
		AUD_DBG("%s: cmd %d\n", __func__, cmd);
		if(copy_from_user(&ctable,(void*)argc,sizeof(ctable))) {
			AUD_ERR("%s: failed on copy_from_user\n", __func__);
			return -EFAULT;
		}
		if(ctable[0] == 'A') {
			table_num = (int) simple_strtoul(&ctable[1], NULL, 10);
			AUD_DBG("write uplink[%d]\n", table_num);
			aic3254_config(&aic3254_uplink[table_num][1], aic3254_uplink[table_num][0].data - 1);
		} else if (ctable[0] == 'B') {
			table_num = (int) simple_strtoul(&ctable[1], NULL, 10);
			AUD_DBG("write downlink[%d]\n", table_num);
			aic3254_config(&aic3254_downlink[table_num][1], aic3254_downlink[table_num][0].data - 1);
		} else {
			AUD_DBG("bad table num : %s", (char *)argc);
			return -EFAULT;
		}

		break;
#if 0

	case AIC3254_SET_DSP_PARAM:
		if (copy_from_user(&para, (void *)argc, sizeof(para))) {
			AUD_ERR("%s: failed on copy_from_user\n", __func__);
			return -EFAULT;
		}

		AUD_INF("%s: parameters(%d, %d, %p)\n", __func__,
				para.row_num, para.col_num, para.cmd_data);

		table = aic3254_minidsp[0];

		
		if (para.row_num > MINIDSP_ROW_MAX
				|| para.col_num != MINIDSP_COL_MAX) {
			AUD_ERR("%s: data size mismatch with allocated"
					" memory (%d,%d)\n", __func__,
					MINIDSP_ROW_MAX, MINIDSP_COL_MAX);
			return -EFAULT;
			}

		mem_size = para.row_num * para.col_num * sizeof(struct aic3254_cmd);
		if (copy_from_user(table, para.cmd_data, mem_size)) {
			AUD_ERR("%s: failed on copy_from_user\n", __func__);
			return -EFAULT;
		}

		AUD_INF("%s: update table(%d,%d) successfully\n",
				__func__, para.row_num, para.col_num);
		break;
#endif

	case AIC3254_CONFIG_TX:
		if (copy_from_user(&i, (void *)argc, sizeof(int))) {
			AUD_ERR("%s: failed on copy_from_user\n", __func__);
			return -EFAULT;
		}
		ret = aic3254_set_config(cmd, i, 1);
		if (ret < 0)
			AUD_ERR("%s: configure(%d) error %d\n",
				__func__, i, ret);
		break;

	case AIC3254_CONFIG_VOLUME_L:
		if (copy_from_user(&volume, (void *)argc, sizeof(int))) {
			AUD_ERR("%s: failed on copy_from_user\n", __func__);
			return -EFAULT;
		}

		if (volume < -127 || volume > 48) {
			AUD_ERR("%s: volume out of range\n", __func__);
			return -EFAULT;
		}

		AUD_INF("%s: AIC3254 config left volume %d\n",
				__func__, volume);

		CODEC_SET_VOLUME_L[1].data = volume;
		aic3254_config(CODEC_SET_VOLUME_L, ARRAY_SIZE(CODEC_SET_VOLUME_L));
		break;

	case AIC3254_CONFIG_VOLUME_R:
		if (copy_from_user(&volume, (void *)argc, sizeof(int))) {
			AUD_ERR("%s: failed on copy_from_user\n", __func__);
			return -EFAULT;
		}

		if (volume < -127 || volume > 48) {
			AUD_ERR("%s: volume out of range\n", __func__);
			return -EFAULT;
		}

		AUD_INF("%s: AIC3254 config right volume %d\n",
				__func__, volume);

		CODEC_SET_VOLUME_R[1].data = volume;
		aic3254_config(CODEC_SET_VOLUME_R, ARRAY_SIZE(CODEC_SET_VOLUME_R));
		break;

	case AIC3254_DUMP_PAGES:
		if (copy_from_user(&i, (void *)argc, sizeof(int))) {
			AUD_ERR("%s: failed on copy_from_user\n", __func__);
			return -EFAULT;
		}
		if (i > AIC3254_MAX_PAGES) {
			AUD_ERR("%s: invalid page number %d\n", __func__, i);
			return -EINVAL;
		}

		AUD_INF("========== %s: dump page %d ==========\n",
				__func__, i);
		
#if 0
		if (ctl_ops->rx_amp_enable)
			ctl_ops->rx_amp_enable(1);
#endif
		aic3254_i2c_write_1byte(0x00, i);
		for (i = 0; i < AIC3254_MAX_REGS; i++) {
			ret = aic3254_i2c_read_1byte(i, &data);
			if (ret < 0)
				AUD_ERR("read fail on register 0x%X\n", i);
			else
				AUD_INF("(0x%02X, 0x%02X)\n", i, data);
		}
#if 0
		if (ctl_ops->rx_amp_enable)
			ctl_ops->rx_amp_enable(0);
#endif
		AUD_INF("=============================================\n");
		break;

	case AIC3254_WRITE_REG:
		if (copy_from_user(&reg, (void *)argc,
					sizeof(struct aic3254_cmd)*2)) {
			AUD_ERR("%s: failed on copy_from_user\n", __func__);
			return -EFAULT;
		}
		AUD_INF("%s: command list (%c,%02X,%02X) (%c,%02X,%02X)\n",
				__func__, reg[0].act, reg[0].reg, reg[0].data,
				reg[1].act, reg[1].reg, reg[1].data);
		aic3254_config(reg, 2);
		break;

	case AIC3254_READ_REG:
		if (copy_from_user(&reg, (void *)argc,
					sizeof(struct aic3254_cmd)*2)) {
			AUD_ERR("%s: failed on copy_from_user\n", __func__);
			return -EFAULT;
		}
#if 0
		if (ctl_ops->spibus_enable)
			ctl_ops->spibus_enable(1);
#endif
		for (i = 0; i < 2; i++) {
			if (reg[i].act == 'r' || reg[i].act == 'R')
				aic3254_i2c_read_1byte(reg[i].reg, &reg[i].data);
			else if (reg[i].act == 'w' || reg[i].act == 'W')
				aic3254_i2c_write_1byte(reg[i].reg, reg[i].data);
			else
				return -EINVAL;
		}
#if 0
		if (ctl_ops->spibus_enable)
			ctl_ops->spibus_enable(0);
#endif
		if (copy_to_user((void *)argc, &reg, sizeof(struct aic3254_cmd)*2)) {
			AUD_ERR("%s: failed on copy_to_user\n", __func__);
			return -EFAULT;
		}
		break;

	case AIC3254_POWERDOWN:
		mutex_lock(&aic3254_mutex);
		aic3254_powerdown();
		mutex_unlock(&aic3254_mutex);
		break;

	case AIC3254_LOOPBACK:
		if (copy_from_user(&i, (void *)argc, sizeof(int))) {
			AUD_ERR("%s: failed on copy_from_user\n", __func__);
			return -EFAULT;
		}
		AUD_INF("%s: index %d for LOOPBACK\n", __func__, i);
		aic3254_loopback(i);
		break;

	default:
		AUD_ERR("%s: invalid command %d\n", __func__, _IOC_NR(cmd));
		ret = -EINVAL;


	}

	return ret;
}

static const struct file_operations aic3254_fops = {
	.owner = THIS_MODULE,
	.open = aic3254_open,
	.release = aic3254_release,
	.unlocked_ioctl = aic3254_ioctl,
};

static struct miscdevice aic3254_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "codec_aic3254",
	.fops = &aic3254_fops,
};

static  struct aic3254_cmd **init_2d_array(int row_sz, int col_sz)
{
	struct aic3254_cmd *table = NULL;
	struct aic3254_cmd **table_ptr = NULL;
	int i = 0;

	table_ptr = kzalloc(row_sz * sizeof(struct aic3254_cmd *), GFP_KERNEL);
	table = kzalloc(row_sz * col_sz * sizeof(struct aic3254_cmd), GFP_KERNEL);
	if (table_ptr == NULL || table == NULL) {
		AUD_ERR("%s: out of memory\n", __func__);
		kfree(table);
		kfree(table_ptr);
	} else
		for (i = 0; i < row_sz; i++)
			table_ptr[i] = (struct aic3254_cmd *)table + i * col_sz;

	return table_ptr;
}

static int aic3254_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	uint8_t reg_data = 0xFF;
	int retry = 0;
	
	AUD_DBG("aic3254 driver: %s -- %s\n", __FILE__ ,__func__);

	aic3254_client = client;
	aic3254_i2c_msg[0].addr = aic3254_i2c_msg[1].addr = aic3254_client->addr;
#if 0

#if 0
	aic3254_config(CODEC_INIT_REG, ARRAY_SIZE(CODEC_INIT_REG));
	aic3254_config(CODEC_DOWNLINK_OFF, ARRAY_SIZE(CODEC_DOWNLINK_OFF));
	aic3254_config(CODEC_UPLINK_OFF, ARRAY_SIZE(CODEC_UPLINK_OFF));
	aic3254_config(CODEC_POWER_OFF, ARRAY_SIZE(CODEC_POWER_OFF));
#endif

	aic3254_tx_mode = UPLINK_OFF;
	aic3254_rx_mode = DOWNLINK_OFF;

#endif

	
	aic3254_uplink = init_2d_array(IO_CTL_ROW_MAX, IO_CTL_COL_MAX);
	aic3254_downlink = init_2d_array(IO_CTL_ROW_MAX, IO_CTL_COL_MAX);
	aic3254_minidsp = init_2d_array(MINIDSP_ROW_MAX, MINIDSP_COL_MAX);

	aic3254_uplink[0][0].act = 'w';
	aic3254_uplink[0][0].reg = 8;
	aic3254_uplink[0][0].data = 8;

	do {
#if defined(CONFIG_CPLD) && (defined(CONFIG_MACH_CP3DUG) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY))
		
		cpld_gpio_write(CPLD_EXT_GPIO_AUD_A3254_RST, 0);
		
		mdelay(1); 
		cpld_gpio_write(CPLD_EXT_GPIO_AUD_A3254_RST, 1);
		
		mdelay(12); 
#endif

		aic3254_i2c_read_1byte(0x00, &reg_data);
		if (reg_data == 0x00) {
			aic3254_existed = true;
			AUD_INF("%s: AIC3254 exists\n", __func__);
			break;
		} else {
			aic3254_existed = false;
			AUD_ERR("%s: AIC3254 doesn't exist. Retry %d\n", __func__, retry);
			usleep(10000); 
			retry++;
		}
	} while (retry < 10);
#if 0
	AUD_INF("Initialize IMIC & EMIC for loopback test.\n");
	aic3254_config(CODEC_INIT_REG, ARRAY_SIZE(CODEC_INIT_REG));
	aic3254_config(CODEC_2MIC_ON, ARRAY_SIZE(CODEC_2MIC_ON));
#endif



#ifdef	CONFIG_PROC_FS
	aic3254_create_proc_file();
#endif

	return 0;
}

static int aic3254_remove(struct i2c_client *client)
{
	AUD_DBG("aic3254 driver: %s -- %s\n", __FILE__ ,__func__);

	if (aic3254_uplink != NULL) {
		kfree(*aic3254_uplink);
		kfree(aic3254_uplink);
		aic3254_uplink = NULL;
	}
	if (aic3254_downlink != NULL) {
		kfree(*aic3254_downlink);
		kfree(aic3254_downlink);
		aic3254_downlink = NULL;
	}
	if (aic3254_minidsp != NULL) {
		kfree(*aic3254_minidsp);
		kfree(aic3254_minidsp);
		aic3254_minidsp = NULL;
	}

	return 0;

#ifdef	CONFIG_PROC_FS
	aic3254_remove_proc_file();
#endif
	return 0;
}

static int aic3254_suspend(struct i2c_client *client, pm_message_t mesg)
{
	AUD_DBG("aic3254 driver: %s -- %s\n", __FILE__ ,__func__);
	suspend_flag = 1;
	return 0;
}

static int aic3254_resume(struct i2c_client *client)
{
	AUD_DBG("aic3254 driver: %s -- %s\n", __FILE__ ,__func__);
	return 0;
}

static const struct i2c_device_id aic3254_id[] = {
	{ AIC3254_I2C_NAME, 0 },
	{ }
};

static struct i2c_driver aic3254_i2c_driver = {
	.probe = aic3254_probe,
	.remove = aic3254_remove,
	.suspend = aic3254_suspend,
	.resume = aic3254_resume,	
	.id_table = aic3254_id,
	.driver = {
		   .name = AIC3254_I2C_NAME,
		  },
};

static int __init aic3254_init(void)
{
	int ret = 0;
	AUD_DBG("aic3254 driver: %s -- %s\n", __FILE__ ,__func__);
	
	mutex_init(&(aic3254_mutex));
	mutex_init(&(aic3254_i2c_mutex));

	ret = i2c_add_driver(&aic3254_i2c_driver);
	if (ret < 0) {
		AUD_ERR("%s: Failed to register i2c driver (ret is %d)\n", __func__, ret);
		return ret;
	}

	if (aic3254_existed == false) {
		return -1;
	}

	AUD_DBG("%s: Register misc\n", __func__);
	ret = misc_register(&aic3254_misc);
	if (ret < 0) {
		AUD_ERR("%s: Failed to register misc device\n", __func__);
		i2c_del_driver(&aic3254_i2c_driver);
		return ret;
	}

	wake_lock_init(&(aic3254_wakelock), WAKE_LOCK_SUSPEND,
			"aic3254_suspend_lock");

	return 0;
}

static void __exit aic3254_exit(void)
{
	AUD_DBG("aic3254 driver: %s -- %s\n", __FILE__ ,__func__);

	i2c_del_driver(&aic3254_i2c_driver);
	misc_deregister(&aic3254_misc);

	wake_lock_destroy(&(aic3254_wakelock));
	

	return;
}

module_init(aic3254_init);
module_exit(aic3254_exit);

MODULE_DESCRIPTION("Codec aic3254 driver");
MODULE_LICENSE("GPLv2");


