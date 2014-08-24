/* linux/arch/arm/mach-msm/board-magnids-panel.c
 *
 * Copyright (c) 2011 HTC.
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

#include <asm/mach-types.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/msm_mdp.h>
#include <mach/msm_iomap.h>
#include <mach/panel_id.h>
#include <mach/board.h>
#include <mach/debug_display.h>
#include <mach/htc_battery_common.h>

#include "../devices.h"
#include "../board-magnids.h"
#include "../../../../drivers/video/msm/msm_fb.h"
#include "../proc_comm.h"

#define LCM_GPIO_CFG(gpio, func) \
PCOM_GPIO_CFG(gpio, func, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_4MA)

#define LCM_GPIO_CFG_OFF(gpio, func) \
PCOM_GPIO_CFG(gpio, func, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_4MA)


static int lcdc_power_save_on = 1;
static int lcdc_status = 1;
static int bl_level_prevset = 0;

static char ptype[60] = "Panel Type = ";

#define DEFAULT_BRIGHTNESS 143
static unsigned int last_brightness = DEFAULT_BRIGHTNESS;

/*common init command*/
static u16 spi_sleep_out[] = {
	0x0011,
};

static u16 spi_display_on[] = {
	0x0029,
};

static u16 spi_sleep_in[] = {
	0x0010,
};

static u16 spi_display_off[] = {
	0x0028,
};

static u16 spi_set_brightness[] = {
	0x0051,	0x01FF,
};

static u16 spi_enable_cabc[] = {
	0x0055,	0x0101,
	0x0000,
};


/*sharp init command*/
static u16 sharp_spi_disp_on_setting[] = {
	0x00b9, 0x01ff, 0x0183, 0x0163,
	0x003a, 0x0170,
	0x00db, 0x0180,
	0x00c9, 0x010f, 0x013e, 0x0101,	// PWM frequency will be 10.7kHz
	0x00C1, 0x0101, 0x0100, 0x010C, 0x0113, 0x011C, 0x0125, 0x012E, 0x0136,
		0x013B, 0x0143, 0x014B, 0x0153, 0x015B, 0x0163, 0x016C, 0x0177, 0x017E,
		0x0185, 0x018D, 0x0194, 0x019D, 0x01A4, 0x01AB, 0x01B4, 0x01BC, 0x01C5,
		0x01CC, 0x01D3, 0x01DA, 0x01E1, 0x01E5, 0x01ED, 0x01F3, 0x01F8, 0x0100,
		0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
		0x010C, 0x0113, 0x011C, 0x0124, 0x012D, 0x0135, 0x0139, 0x0141, 0x0149,
		0x0151, 0x0159, 0x0160, 0x0169, 0x0174, 0x017B, 0x0182, 0x018A, 0x0191,
		0x019A, 0x01A1, 0x01A8, 0x01B1, 0x01B9, 0x01C2, 0x01CB, 0x01D2, 0x01DA,
		0x01E2, 0x01E9, 0x01F1, 0x01F8, 0x01FF, 0x0100, 0x0100, 0x0100, 0x0100,
		0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0106, 0x010D, 0x0117,
		0x011D, 0x0125, 0x012E, 0x0133, 0x0137, 0x013C, 0x0143, 0x014A, 0x0150,
		0x0157, 0x015F, 0x0164, 0x016B, 0x0174, 0x017B, 0x0182, 0x0188, 0x018E,
		0x0196, 0x019D, 0x01A3, 0x01AC, 0x01B3, 0x01BB, 0x01C6, 0x01CD, 0x01D6,
		0x01E0, 0x01EB, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
		0x0100, 0x0100,
};
static u16 sharp_spi_backlight_setting[] = {
	0x005e, 0x0100,
	0x00ca, 0x012c, 0x012a, 0x0128, 0x0127, 0x0126, 0x0125, 0x0124, 0x0122, 0x0120,
	0x0053, 0x0124,
};

static int gpio_spi_send_cmd(u16 * dat, u32 size, u32 bit_len)
{
	u32 bit_cnt = bit_len;
	u16 *val = dat;

	gpio_set_value(MAGNIDS_LCD_SPI_CS1, 1);
	udelay(1);

	for (; size; size--) {
		bit_cnt = bit_len;
		gpio_set_value(MAGNIDS_LCD_SPI_CS1, 0);
		udelay(1);
		while (bit_cnt) {
			bit_cnt--;
			if ((*val >> bit_cnt) & 1) {
				  gpio_set_value(MAGNIDS_LCD_SPI_DOUT, 1);
			} else {
				  gpio_set_value(MAGNIDS_LCD_SPI_DOUT, 0);
			}

			udelay(1);
			gpio_set_value(MAGNIDS_LCD_SPI_CLK, 0);

			udelay(1);
			gpio_set_value(MAGNIDS_LCD_SPI_CLK, 1);
			udelay(1);
		}
		val++;
		udelay(1);

		gpio_set_value(MAGNIDS_LCD_SPI_CS1, 1);
	}

	gpio_set_value(MAGNIDS_LCD_SPI_DOUT,0);
	gpio_set_value(MAGNIDS_LCD_SPI_CLK, 0);
	gpio_set_value(MAGNIDS_LCD_SPI_CS1, 0);
	return 0;
}

#define LCDC_MUX_CTL_CMD 2

static void magnids_panel_power(int on)
{
	u32 id = LCDC_MUX_CTL_CMD;
	PR_DISP_INFO("%s: power %s.\n", __func__, on ? "on" : "off");

	if (on) {
		msm_proc_comm(PCOM_CUSTOMER_CMD3, &id, &on);
		gpio_set_value(MAGNIDS_LCD_RST_ID0, 1);
		hr_msleep(10);
		gpio_set_value(MAGNIDS_LCM_3V_EN, 1);
		hr_msleep(10);
		gpio_set_value(MAGNIDS_LCD_RST_ID0, 0);
		hr_msleep(5);
		gpio_set_value(MAGNIDS_LCD_RST_ID0, 1);
		hr_msleep(50);
	} else {
		msm_proc_comm(PCOM_CUSTOMER_CMD3, &id, &on);
		hr_msleep(50);
		gpio_set_value(MAGNIDS_LCD_RST_ID0, 0);
		gpio_set_value(MAGNIDS_LCM_3V_EN, 0);
	}
}

static int lcdc_panel_power(int on)
{
	int flag_on = !!on;

	if (lcdc_power_save_on == flag_on)
		return 0;

	lcdc_power_save_on = flag_on;

	magnids_panel_power(on);

	return 0;
}

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("[DISP]%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}

}

/* ----------------------------DISP GPIO-------------------------------- */
static unsigned int  display_on_gpio_table[] = {
	LCM_GPIO_CFG(MAGNIDS_LCD_PCLK, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_DE, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_VSYNC, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_HSYNC, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_G0, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_G1, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_G2, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_G3, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_G4, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_G5, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_G6, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_G7, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_B0_ID1, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_B1, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_B2, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_B3, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_B4, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_B5, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_B6, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_B7, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_R0, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_R1, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_R2, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_R3, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_R4, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_R5, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_R6, 1),
	LCM_GPIO_CFG(MAGNIDS_LCD_R7, 1),
	PCOM_GPIO_CFG(MAGNIDS_LCD_RST_ID0, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	PCOM_GPIO_CFG(MAGNIDS_LCD_SPI_DIN, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	PCOM_GPIO_CFG(MAGNIDS_LCD_SPI_CS1, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	PCOM_GPIO_CFG(MAGNIDS_LCD_SPI_CLK, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	PCOM_GPIO_CFG(MAGNIDS_LCD_SPI_DOUT, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
};

static unsigned int display_off_gpio_table[] = {
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_PCLK, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_DE, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_VSYNC, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_HSYNC, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_G0, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_G1, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_G2, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_G3, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_G4, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_G5, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_G6, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_G7, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_B0_ID1, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_B1, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_B2, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_B3, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_B4, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_B5, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_B6, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_B7, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_R0, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_R1, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_R2, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_R3, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_R4, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_R5, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_R6, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_R7, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_RST_ID0, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_SPI_DIN, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_SPI_CS1, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_SPI_CLK, 0),
	LCM_GPIO_CFG_OFF(MAGNIDS_LCD_SPI_DOUT, 0),
};

static int lcdc_gpio_config(int on)
{
	PR_DISP_INFO("%s: gpio %s.\n", __func__, on ? "on" : "off");
	config_gpio_table(
		!!on ? display_on_gpio_table : display_off_gpio_table,
		!!on ? ARRAY_SIZE(display_on_gpio_table) : ARRAY_SIZE(display_off_gpio_table));

	return 0;
}

static struct lcdc_platform_data lcdc_pdata = {
	.lcdc_gpio_config = lcdc_gpio_config,
	.lcdc_power_save = lcdc_panel_power,
};

#define BRI_SETTING_MIN                 1
#define BRI_SETTING_DEF                 143
#define BRI_SETTING_MAX                 255

static int magnids_shrink_pwm(int val)
{
	unsigned int pwm_min, pwm_default, pwm_max;
	unsigned int shrink_br = BRI_SETTING_MAX;

	if (panel_type == PANEL_ID_MAGNIDS_SHARP) {
		pwm_min = 9;
		pwm_default = 126;
		pwm_max = 255;
	} else {
		pwm_min = 10;
		pwm_default = BRI_SETTING_DEF;
		pwm_max = 255;
	}

	if (val <= 0) {
		shrink_br = 0;
	} else if (val > 0 && (val < BRI_SETTING_MIN)) {
			shrink_br = pwm_min;
	} else if ((val >= BRI_SETTING_MIN) && (val <= BRI_SETTING_DEF)) {
			shrink_br = (val - BRI_SETTING_MIN) * (pwm_default - pwm_min) /
		(BRI_SETTING_DEF - BRI_SETTING_MIN) + pwm_min;
	} else if (val > BRI_SETTING_DEF && val <= BRI_SETTING_MAX) {
			shrink_br = (val - BRI_SETTING_DEF) * (pwm_max - pwm_default) /
		(BRI_SETTING_MAX - BRI_SETTING_DEF) + pwm_default;
	} else if (val > BRI_SETTING_MAX)
			shrink_br = pwm_max;

	/*PR_DISP_INFO("brightness orig=%d, transformed=%d\n", val, shrink_br);*/

	return shrink_br;
}

static int magnids_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct msm_fb_panel_data *pdata = NULL;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	pdata = (struct msm_fb_panel_data *)mfd->pdev->dev.platform_data;

	if (mfd->key != MFD_KEY)
		return -EINVAL;


	if (mfd->first_init_lcd != 0) {
		PR_DISP_INFO("Display On - 1st time\n");

		mfd->first_init_lcd = 0;

	} else {
		PR_DISP_INFO("Display On \n");
		if (panel_type != PANEL_ID_NONE) {
			PR_DISP_INFO("%s\n", ptype);
			gpio_spi_send_cmd(spi_sleep_out, ARRAY_SIZE(spi_sleep_out), 9);
			hr_msleep(120);
			gpio_spi_send_cmd(sharp_spi_disp_on_setting,ARRAY_SIZE(sharp_spi_disp_on_setting), 9);
			gpio_spi_send_cmd(spi_enable_cabc,ARRAY_SIZE(spi_enable_cabc), 9);
			hr_msleep(5);
			gpio_spi_send_cmd(sharp_spi_backlight_setting,ARRAY_SIZE(sharp_spi_backlight_setting), 9);
			gpio_spi_send_cmd(spi_display_on,ARRAY_SIZE(spi_display_on), 9);
		} else {
			printk(KERN_ERR "panel_type=0x%x not support at power on\n", panel_type);
			return -EINVAL;
		}
	}
	PR_DISP_DEBUG("Init done!\n");

	return 0;
}

static int magnids_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	PR_DISP_INFO("%s\n", __func__);

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (panel_type != PANEL_ID_NONE) {
		PR_DISP_INFO("%s\n", ptype);
		gpio_spi_send_cmd(spi_display_off,ARRAY_SIZE(spi_display_off),9);
		gpio_spi_send_cmd(spi_sleep_in,ARRAY_SIZE(spi_sleep_in),9);
		msleep(100);
	} else
		printk(KERN_ERR "panel_type=0x%x not support at power off\n",
			panel_type);

	return 0;
}

static int lcdc_set_backlight(struct msm_fb_data_type *mfd)
{
	unsigned int shrink_br;

	if (lcdc_status == 0 || bl_level_prevset == mfd->bl_level) {
		PR_DISP_DEBUG("Skip the backlight setting > lcdc_status : %d, bl_level_prevset : %d, bl_level : %d\n",
			lcdc_status, bl_level_prevset, mfd->bl_level);
		goto end;
	}

	shrink_br = magnids_shrink_pwm(mfd->bl_level);

	if (mfd->bl_level == 0 || board_mfg_mode() == 4 ||
	    (board_mfg_mode() == 5 && !(htc_battery_get_zcharge_mode() % 2))) {
		shrink_br = 0;
	}

	spi_set_brightness[1] = 0x0100 | shrink_br;

	gpio_spi_send_cmd(spi_set_brightness, ARRAY_SIZE(spi_set_brightness), 9);

	bl_level_prevset = mfd->bl_level;

	/* Record the last value which was not zero for resume use */
	if (mfd->bl_level >= BRI_SETTING_MIN)
		last_brightness = mfd->bl_level;

	PR_DISP_INFO("lcdc_set_backlight > set brightness to %d(%d)\n", shrink_br, mfd->bl_level);
end:
	return 0;
}

static void magnids_set_backlight(struct msm_fb_data_type *mfd)
{
	if (!mfd->panel_power_on)
		return;

	lcdc_set_backlight(mfd);
}


static void magnids_display_on(struct msm_fb_data_type *mfd)
{
	PR_DISP_INFO("%s+\n", __func__);
#if 0 /* Skip the display_on cmd transfer for LG panel only */
	htc_mdp_sem_down(current, &mfd->dma->mutex);
	if (mfd->panel_info.type == MIPI_CMD_PANEL) {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
	}
	mipi_dsi_cmds_tx(mfd, &magnids_panel_tx_buf, lg_display_on_cmds,
		ARRAY_SIZE(lg_display_on_cmds));
	htc_mdp_sem_up(&mfd->dma->mutex);
#endif
}

static void magnids_bkl_switch(struct msm_fb_data_type *mfd, bool on)
{
	PR_DISP_INFO("%s > %d\n", __func__, on);
	if (on) {
		lcdc_status = 1;
		if (mfd->bl_level == 0)
			/* Assign the backlight by last brightness before suspend */
			mfd->bl_level = last_brightness;

		lcdc_set_backlight(mfd);
	} else {
		if (bl_level_prevset != 0) {
			if (bl_level_prevset >= BRI_SETTING_MIN)
				last_brightness = bl_level_prevset;
			mfd->bl_level = 0;
			lcdc_set_backlight(mfd);
		}
		lcdc_status = 0;
	}
}

static int __devinit magnids_lcd_probe(struct platform_device *pdev)
{
	msm_fb_add_device(pdev);

	return 0;
}

static void magnids_lcd_shutdown(struct platform_device *pdev)
{
	lcdc_panel_power(0);
}

static struct platform_driver this_driver = {
	.probe = magnids_lcd_probe,
	.shutdown = magnids_lcd_shutdown,
	.driver = {
		.name   = "lcdc_magnids",
	},
};

static struct msm_fb_panel_data magnids_panel_data = {
	.on		= magnids_lcd_on,
	.off		= magnids_lcd_off,
	.set_backlight  = magnids_set_backlight,
	.display_on  = magnids_display_on,
	.bklswitch	= magnids_bkl_switch,
};

static struct platform_device this_device = {
	.name   = "lcdc_magnids",
	.id	= 1,
	.dev	= {
		.platform_data = &magnids_panel_data,
	}
};

static int lcdc_sharp_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

	pinfo = &magnids_panel_data.panel_info;
	pinfo->xres = 480;
	pinfo->yres = 800;
	MSM_FB_SINGLE_MODE_PANEL(pinfo);
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 24;
	pinfo->fb_num = 2;
	pinfo->clk_rate = 24576000;
	pinfo->bl_max = 255;
	pinfo->bl_min = 1;
	pinfo->camera_backlight = 227;

	pinfo->lcdc.h_back_porch = 6;
	pinfo->lcdc.h_front_porch = 15;
	pinfo->lcdc.h_pulse_width = 6;
	pinfo->lcdc.v_back_porch = 3;
	pinfo->lcdc.v_front_porch = 3;
	pinfo->lcdc.v_pulse_width = 3;
	pinfo->lcdc.border_clr = 0;     /* blk */
	pinfo->lcdc.underflow_clr = 0xff;       /* blue */
	pinfo->lcdc.hsync_skew = 0;

	ret = platform_device_register(&this_device);
	if (ret)
		pr_err("%s: failed to register device!\n", __func__);

	strcat(ptype, "PANEL_ID_MAGNIDS_SHARP");
	PR_DISP_INFO("%s: assign initial setting for SHARP, %s\n", __func__, ptype);

	return ret;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.width = 56,
	.height = 94,
};

static struct resource msm_fb_resources[] = {
	{
		.start = MSM_FB_BASE,
		.end = MSM_FB_BASE + MSM_FB_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

static struct msm_panel_common_pdata mdp_pdata = {
	.mdp_rev = MDP_REV_303,
};

int __init magnids_init_panel(void)
{
	platform_device_register(&msm_fb_device);

	if (panel_type != PANEL_ID_NONE) {
		msm_fb_register_device("mdp", &mdp_pdata);
		msm_fb_register_device("lcdc", &lcdc_pdata);
	}

	return 0;
}

static int __init magnids_panel_late_init(void)
{
	PR_DISP_INFO("%s: enter 0x%x\n", __func__, panel_type);

	if (panel_type == PANEL_ID_MAGNIDS_SHARP) {
		lcdc_sharp_panel_init();
		PR_DISP_INFO("match PANEL_ID_MAGNIDS_SHARP panel_type\n");
	} else
		PR_DISP_INFO("Mis-match panel_type\n");

	return platform_driver_register(&this_driver);
}

late_initcall(magnids_panel_late_init);
