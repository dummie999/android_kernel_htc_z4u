/* linux/arch/arm/mach-msm/board-z4-panel.c
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
#include <linux/i2c/cpld.h>
#include <mach/msm_iomap.h>
#include <mach/panel_id.h>
#include <mach/board.h>
#include <mach/debug_display.h>

#include "../devices.h"

#ifdef CONFIG_MACH_DUMMY
#include "../board-z4dug.h"
#endif
#ifdef CONFIG_MACH_DUMMY
#include "../board-z4dcg.h"
#endif
#ifdef CONFIG_MACH_Z4U
#include "../board-z4u.h"
#endif


#include "../../../../drivers/video/msm/msm_fb.h"
#include "../../../../drivers/video/msm/mipi_dsi.h"

static int mipi_power_save_on = 1;
static int bl_level_prevset = 0;
static int mipi_status = 1;
static int resume_blk = 0;

static struct dsi_buf z4_panel_tx_buf;
static struct dsi_buf z4_panel_rx_buf;
static struct dsi_cmd_desc *power_on_cmd = NULL;
static struct dsi_cmd_desc *display_off_cmd = NULL;
static int power_on_cmd_count = 0;
static int display_off_cmd_count = 0;
static char ptype[60] = "Panel Type = ";

#define DEFAULT_BRIGHTNESS_DCG 166
static unsigned int last_brightness = DEFAULT_BRIGHTNESS_DCG;

static unsigned char led_pwm1[] = {0x51, 0xFF}; 
static struct dsi_cmd_desc backlight_cmds[] = {
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0,
                sizeof(led_pwm1), led_pwm1},
};
static char sleep_out[] = {0x11, 0x00}; 
static char display_on[] = {0x29, 0x00}; 
static char display_off[] = {0x28, 0x00}; 
static char sleep_in[] = {0x10, 0x00}; 

static char MAUCCTR0[] = {
        0xF0, 0x55, 0xAA, 0x52,
        0x08, 0x00};
static char DOPCTR[] = {0xB1, 0x68, 0x00, 0x01};
static char VIVIDCTR[] = {
        0xB4, 0x10, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00};
static char cmd_d6[] = {
	0xD6, 0x00, 0x05, 0x10,
	0x17, 0x22, 0x26, 0x29,
	0x29, 0x26, 0x23, 0x17,
	0x12, 0x06, 0x02, 0x01,
	0x00};
static char cmd_d7[] = {
	0xD7, 0x30, 0x30, 0x30,
	0x28, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00};
static char cmd_d8[] = {
	0xD8, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x28,
	0x30, 0x00 };
static char SDHDTCTR[] = {0xB6, 0x07};
static char GSEQCTR[] = {0xB7, 0x33, 0x03};
static char SDEQCTR[] = {
        0xB8, 0x00, 0x00, 0x02,
        0x00};
static char SDVPCTR[] = {0xBA, 0x01};
static char SGOPCTR[] = {0xBB, 0x44, 0x40};
static char DPTMCTR10_1[] = {0xC1, 0x01};
static char DPTMCTR10_2[] = {
        0xC2, 0x00, 0x00, 0x55,
        0x55, 0x55, 0x00, 0x55,
        0x55};
static char PDOTCTR[] = {0xC7, 0x00};
static char DPTMCTR10_3[] = {
        0xCA, 0x05, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x01, 0x00};
static char PWMFRCTR[] = {0xE0, 0x00, 0x01};
static char FCBRTCB[] = {0xE1, 0x00, 0xFF};
static char cmd_e3[] = {
	0xE3, 0xFF, 0xF7, 0xEF,
	0xE7, 0xDF, 0xD7, 0xCF,
	0xC7, 0xBF, 0xB7};
static char cmd_5e[] = {0x5E, 0x06};
static char cmd_55[] = {0x55, 0x02};
static char cmd_ff_80[] = {
	0xFF, 0xAA, 0x55, 0xA5,
	0x80};
static char cmd_f5[] = {
	0xF5, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x00, 0xD9,
	0x17};
static char cmd_ff_00[] = {
	0xFF, 0xAA, 0x55, 0xA5,
	0x00};
static char MAUCCTR1[] = {
        0xF0, 0x55, 0xAA, 0x52,
        0x08, 0x01};
static char SETAVDD[] = {0xB0, 0x0A};
static char SETAVEE[] = {0xB1, 0x0A};
static char SETVCL[] = {0xB2, 0x00};
static char SETVGH[] = {0xB3, 0x08};
static char SETVRGH[] = {0xB4, 0x28};
static char SETVGL_REG[] = {0xB5, 0x05};
static char BT1CTR[] = {0xB6, 0x35};
static char BT2CTR[] = {0xB7, 0x35};
static char BT3CTR[] = {0xB8, 0x25};
static char BT4CTR[] = {0xB9, 0x37};
static char BT5CTR[] = {0xBA, 0x15};
static char STBCTR[] = {0xCC, 0x64};
static char write_ctrl_display[] = {0x53, 0x24};
static char peripheral_on[] = {0x00, 0x00}; 
static char peripheral_off[] = {0x00, 0x00}; 

static struct dsi_cmd_desc jdi_nt_on_cmds[] = {
        {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(MAUCCTR0), MAUCCTR0},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(DOPCTR), DOPCTR},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(VIVIDCTR), VIVIDCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cmd_d6), cmd_d6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cmd_d7), cmd_d7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cmd_d8), cmd_d8},
        {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SDHDTCTR), SDHDTCTR},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(GSEQCTR), GSEQCTR},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(SDEQCTR), SDEQCTR},
        {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SDVPCTR), SDVPCTR},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(SGOPCTR), SGOPCTR},
        {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(DPTMCTR10_1), DPTMCTR10_1},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(DPTMCTR10_2), DPTMCTR10_2},
        {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(PDOTCTR), PDOTCTR},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(DPTMCTR10_3), DPTMCTR10_3},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(PWMFRCTR), PWMFRCTR},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(FCBRTCB), FCBRTCB},

	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cmd_e3), cmd_e3},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(cmd_5e), cmd_5e},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(cmd_55), cmd_55},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cmd_ff_80), cmd_ff_80},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cmd_f5), cmd_f5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cmd_ff_00), cmd_ff_00},

        {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(MAUCCTR1), MAUCCTR1},
        {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETAVDD), SETAVDD},
        {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETAVEE), SETAVEE},
        {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETVCL), SETVCL},
        {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETVGH), SETVGH},
        {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETVRGH), SETVRGH},
        {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETVGL_REG), SETVGL_REG},
        {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT1CTR), BT1CTR},
        {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT2CTR), BT2CTR},
        {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT3CTR), BT3CTR},
        {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT4CTR), BT4CTR},
        {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT5CTR), BT5CTR},
        {DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(STBCTR), STBCTR},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(write_ctrl_display), write_ctrl_display},
	{DTYPE_PERIPHERAL_ON, 1, 0, 1, 120, sizeof(peripheral_on), peripheral_on},
};

static struct dsi_cmd_desc jdi_nt_off_cmds[] = {
	{DTYPE_PERIPHERAL_OFF, 1, 0, 1, 70, sizeof(peripheral_off), peripheral_off},
};

static char auo_f0_1[] = {
    0xF0, 0x55, 0xAA, 0x52,
    0x08, 0x01}; 
static char auo_b0_1[] = {0xB0, 0x05}; 
static char auo_b1_1[] = {0xB1, 0x05}; 
static char auo_b2[] = {0xB2, 0x00}; 
static char auo_b3[] = {0xB3, 0x09}; 
static char auo_b6_1[] = {0xB6, 0x14}; 
static char auo_b7_1[] = {0xB7, 0x15}; 
static char auo_b8_1[] = {0xB8, 0x24}; 
static char auo_b9[] = {0xB9, 0x36}; 
static char auo_ba[] = {0xBA, 0x24}; 
static char auo_bf[] = {0xBF, 0x01}; 
static char auo_c3[] = {0xC3, 0x11}; 
static char auo_c2[] = {0xC2, 0x00}; 
static char auo_c0[] = {0xC0, 0x00, 0x00}; 
static char auo_bc_1[] = {0xBC, 0x00, 0x88, 0x00}; 
static char auo_bd[] = {0xBD, 0x00, 0x88, 0x00}; 
static char auo_f0_2[] = {
    0xF0, 0x55, 0xAA, 0x52,
    0x08, 0x00}; 
static char auo_b6_2[] = {0xB6, 0x03}; 
static char auo_b7_2[] = {0xB7, 0x70, 0x70}; 
static char GOA_timing[] = {
	0xC8, 0x01, 0x00, 0x46,
	0x64, 0x46, 0x64, 0x46,
	0x64, 0x46, 0x64, 0x64,
	0x64, 0x64, 0x64, 0x64,
	0x64, 0x64, 0x64
};
static char auo_b8_2[] = {
    0xB8, 0x00, 0x02, 0x02,
    0x02}; 
static char auo_bc_2[] = {0xBC, 0x00}; 
static char auo_b0_2[] = {
    0xB0, 0x00, 0x0A, 0x0E,
    0x09, 0x04}; 
static char auo_b1_2[] = {
    0xB1, 0x60, 0x00, 0x01}; 
static char auo_ff_2[] = {
    0xFF, 0xAA, 0x55, 0xA5,
    0x00}; 
static char auo_b0_c2[] = {0xB0, 0x0F}; 
static char auo_b1_c2[] = {0xB1, 0x0F}; 
static char auo_b3_c2[] = {0xB3, 0x07}; 

static struct dsi_cmd_desc auo_nt_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_f0_1), auo_f0_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b0_1), auo_b0_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b1_1), auo_b1_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b2), auo_b2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b3), auo_b3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b6_1), auo_b6_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b7_1), auo_b7_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b8_1), auo_b8_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b9), auo_b9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_ba), auo_ba},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_bf), auo_bf},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_c3), auo_c3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_c2), auo_c2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_c0), auo_c0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_bc_1), auo_bc_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_bd), auo_bd},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_f0_2), auo_f0_2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b6_2), auo_b6_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_b7_2), auo_b7_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(GOA_timing), GOA_timing},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_b8_2), auo_b8_2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_bc_2), auo_bc_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_b0_2), auo_b0_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_b1_2), auo_b1_2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(PWMFRCTR), PWMFRCTR},
	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd_e3), cmd_e3},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmd_5e), cmd_5e},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmd_55), cmd_55},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd_ff_80), cmd_ff_80},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd_f5), cmd_f5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd_ff_00), cmd_ff_00},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(write_ctrl_display), write_ctrl_display},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(VIVIDCTR), VIVIDCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd_d6), cmd_d6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd_d7), cmd_d7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd_d8), cmd_d8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_ff_2), auo_ff_2},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out), sleep_out},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc auo_nt_on_cmds_c2[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_f0_1), auo_f0_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b0_c2), auo_b0_c2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b1_c2), auo_b1_c2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b2), auo_b2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b3_c2), auo_b3_c2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b6_1), auo_b6_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b7_1), auo_b7_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b8_1), auo_b8_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b9), auo_b9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_ba), auo_ba},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_bf), auo_bf},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_c3), auo_c3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_c2), auo_c2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_c0), auo_c0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_bc_1), auo_bc_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_bd), auo_bd},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_f0_2), auo_f0_2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_b6_2), auo_b6_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_b7_2), auo_b7_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(GOA_timing), GOA_timing},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_b8_2), auo_b8_2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_bc_2), auo_bc_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_b0_2), auo_b0_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_b1_2), auo_b1_2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(PWMFRCTR), PWMFRCTR},
	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd_e3), cmd_e3},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmd_5e), cmd_5e},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmd_55), cmd_55},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd_ff_80), cmd_ff_80},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd_f5), cmd_f5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd_ff_00), cmd_ff_00},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(write_ctrl_display), write_ctrl_display},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(VIVIDCTR), VIVIDCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd_d6), cmd_d6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd_d7), cmd_d7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd_d8), cmd_d8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_ff_2), auo_ff_2},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out), sleep_out},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc auo_nt_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 20, sizeof(display_off), display_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(sleep_in), sleep_in},
	{DTYPE_PERIPHERAL_OFF, 1, 0, 1, 40, sizeof(peripheral_off), peripheral_off},
};

#define LCM_ID0  (34)
#define LCM_ID1  (35)
#define LCM_TE   (97)

static bool dsi_power_on;
static void z4_panel_power(int on)
{
	int rc;

	PR_DISP_INFO("%s: power %s.\n", __func__, on ? "on" : "off");

	if (!dsi_power_on) {
		rc = gpio_request(LCM_ID0, "LCM ID0");
		if (rc) {
			pr_err("request LCM ID0 failed, rc=%d\n", rc);
			return;
		}

		rc = gpio_request(LCM_ID1, "LCM ID1");
		if (rc) {
			pr_err("request LCM ID1 failed, rc=%d\n", rc);
			return;
		}

		rc = gpio_request(LCM_TE, "LCM TE");
		if (rc) {
			pr_err("request LCM TE failed, rc=%d\n", rc);
			return;
		}

		dsi_power_on = true;
	}

	if (on) {
		gpio_tlmm_config(GPIO_CFG(LCM_ID0, 0,
			GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(LCM_ID1, 0,
			GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(LCM_TE, 1,
			GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

		if (panel_type == PANEL_ID_LUPUS_AUO_NT ||
			panel_type == PANEL_ID_LUPUS_AUO_NT_C2) {
			cpld_gpio_write(CPLD_EXT_GPIO_LCMIO_1V8_EN, 1);
			hr_msleep(1);
			cpld_gpio_write(CPLD_EXT_GPIO_LCM_3V_EN, 1);
		} else if (panel_type == PANEL_ID_LUPUS_JDI_NT) {
			cpld_gpio_write(CPLD_EXT_GPIO_LCMIO_1V8_EN, 1);
			hr_msleep(10);
			cpld_gpio_write(CPLD_EXT_GPIO_LCM_3V_EN, 1);
		}
	} else {
		if (panel_type == PANEL_ID_LUPUS_AUO_NT ||
			panel_type == PANEL_ID_LUPUS_AUO_NT_C2) {
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 0);
			hr_msleep(1);
			cpld_gpio_write(CPLD_EXT_GPIO_LCM_3V_EN, 0);
			hr_msleep(5);
			cpld_gpio_write(CPLD_EXT_GPIO_LCMIO_1V8_EN, 0);
		} else if (panel_type == PANEL_ID_LUPUS_JDI_NT) {
			hr_msleep(10);
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 0);
			hr_msleep(10);
			cpld_gpio_write(CPLD_EXT_GPIO_LCM_3V_EN, 0);
			hr_msleep(1);
			cpld_gpio_write(CPLD_EXT_GPIO_LCMIO_1V8_EN, 0);
		}

		gpio_tlmm_config(GPIO_CFG(LCM_ID0, 0,
			GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(LCM_ID1, 0,
			GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(LCM_TE, 1,
			GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}
}

static int mipi_panel_power(int on)
{
	int flag_on = !!on;

	if (mipi_power_save_on == flag_on)
		return 0;

	mipi_power_save_on = flag_on;

	z4_panel_power(on);

	return 0;
}

enum {
	DSI_SINGLE_LANE = 1,
	DSI_TWO_LANES,
};

static int msm_fb_get_lane_config(void)
{
	int rc = DSI_TWO_LANES;

	PR_DISP_INFO("DSI Two Lanes\n");

	return rc;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio = 97,
	.dsi_power_save = mipi_panel_power,
	.get_lane_config = msm_fb_get_lane_config,
};

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 142
#define BRI_SETTING_MAX                 255

static unsigned char z4_shrink_pwm(int val)
{
	unsigned int pwm_min, pwm_default, pwm_max;
	unsigned char shrink_br = BRI_SETTING_MAX;

	pwm_min = 14;
	pwm_default = 103;
	pwm_max = 255;

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

	

	return shrink_br;
}

static int z4_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct msm_fb_panel_data *pdata = NULL;
	struct mipi_panel_info *mipi;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	pdata = (struct msm_fb_panel_data *)mfd->pdev->dev.platform_data;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;

	if (mfd->first_init_lcd != 0) {
		printk("Display On - 1st time\n");
		mfd->first_init_lcd = 0;
		last_brightness = 0;
	} else {
		printk("Display On \n");

		if(panel_type == PANEL_ID_LUPUS_AUO_NT ||
			panel_type == PANEL_ID_LUPUS_AUO_NT_C2) {
			hr_msleep(50);
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 1);
			hr_msleep(20);
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 0);
			hr_msleep(30);
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 1);
			hr_msleep(120);
		} else if (panel_type == PANEL_ID_LUPUS_JDI_NT) {
			hr_msleep(50);
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 1);
			hr_msleep(10);
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 0);
			hr_msleep(10);
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 1);
			hr_msleep(120);
		}

		if (panel_type != PANEL_ID_NONE) {
			PR_DISP_INFO("%s\n", ptype);
			htc_mdp_sem_down(current, &mfd->dma->mutex);
			mipi_dsi_cmds_tx(&z4_panel_tx_buf, power_on_cmd,
				power_on_cmd_count);
			htc_mdp_sem_up(&mfd->dma->mutex);
		} else {
			printk(KERN_ERR "panel_type=0x%x not support at power on\n", panel_type);
			return -EINVAL;
		}
	}

	PR_DISP_INFO("Init done!\n");
	return 0;
}

static int z4_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	PR_DISP_INFO("%s\n", __func__);

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	resume_blk = 1;

	return 0;
}

static char dim_on[] = {0x53, 0x2C};
static char dim_off[] = {0x53, 0x24};
static struct dsi_cmd_desc dim_on_cmd[] = {
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(dim_on), dim_on},
};
static struct dsi_cmd_desc dim_off_cmd[] = {
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(dim_off), dim_off},
};

static void z4_dim_on(struct msm_fb_data_type *mfd)
{
        mipi_dsi_cmds_tx(&z4_panel_tx_buf, dim_on_cmd, 1);
        PR_DISP_INFO("%s\n", __func__);
}
static void z4_dim_off(struct msm_fb_data_type *mfd)
{
        mipi_dsi_cmds_tx(&z4_panel_tx_buf, dim_off_cmd, 1);
        PR_DISP_INFO("%s\n", __func__);
}

#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
static char cabc_movie[] = {
        0xE3, 0xFF, 0xF1, 0xE3,
        0xD4, 0xC6, 0xB8, 0xAA,
        0x9B, 0x8D, 0x7F
};
static struct dsi_cmd_desc cabc_still_cmds[] = {
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd_e3), cmd_e3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd_55), cmd_55},
};
static struct dsi_cmd_desc cabc_movie_cmds[] = {
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cabc_movie), cabc_movie},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd_55), cmd_55},
};
static void z4_set_cabc_lvl(struct msm_fb_data_type *mfd, int mode)
{
	struct dsi_cmd_desc* cabc_cmds = NULL;

	if(mode == 2)
		cabc_cmds = cabc_still_cmds;
	else if(mode == 3)
		cabc_cmds = cabc_movie_cmds;
	else
		return;

	mipi_dsi_cmds_tx(&z4_panel_tx_buf, cabc_cmds, 2);
}
#endif

static void z4_set_backlight(struct msm_fb_data_type *mfd)
{
	PR_DISP_INFO("%s\n", __func__);
	

	if (mipi_status == 0 || bl_level_prevset == mfd->bl_level) {
		PR_DISP_DEBUG("Skip the backlight setting > mipi_status : %d, bl_level_prevset : %d, bl_level : %d\n",
			mipi_status, bl_level_prevset, mfd->bl_level);
		return;
	}

	led_pwm1[1] = z4_shrink_pwm(mfd->bl_level);

	if (mfd->bl_level == 0) {
		led_pwm1[1] = 0;
	}

	htc_mdp_sem_down(current, &mfd->dma->mutex);
	if(led_pwm1[1] == 0) {
		z4_dim_off(mfd);
	}
	mipi_dsi_cmds_tx(&z4_panel_tx_buf, backlight_cmds, 1);
	htc_mdp_sem_up(&mfd->dma->mutex);
	bl_level_prevset = mfd->bl_level;

	
	if (mfd->bl_level >= BRI_SETTING_MIN)
		last_brightness = mfd->bl_level;

	PR_DISP_INFO("mipi_dsi_set_backlight > set brightness to %d(%d)\n", led_pwm1[1], mfd->bl_level);

	return;
}

static void z4_bkl_switch(struct msm_fb_data_type *mfd, bool on)
{
	PR_DISP_INFO("%s > %d\n", __func__, on);
	if (on) {
		mipi_status = 1;
		if (mfd->bl_level == 0)
			mfd->bl_level = last_brightness;
	} else {
		if (bl_level_prevset != 0) {
			if (bl_level_prevset >= BRI_SETTING_MIN)
				last_brightness = bl_level_prevset;
			mfd->bl_level = 0;
			z4_set_backlight(mfd);
		}
		mipi_status = 0;
	}
}

static void z4_display_on(struct msm_fb_data_type *mfd)
{
#if 0
	cmdreq.cmds = display_on_cmds;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);
#endif
	PR_DISP_INFO("%s\n", __func__);
}

static void z4_display_off(struct msm_fb_data_type *mfd)
{
#if 0
	cmdreq.cmds = display_off_cmds;
	cmdreq.cmds_cnt = display_off_cmds_count;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);
#endif
	mipi_dsi_cmds_tx(&z4_panel_tx_buf, display_off_cmd, display_off_cmd_count);
	PR_DISP_INFO("%s\n", __func__);
}

static int __devinit z4_lcd_probe(struct platform_device *pdev)
{
	msm_fb_add_device(pdev);

	PR_DISP_INFO("%s\n", __func__);
	return 0;
}

static void z4_lcd_shutdown(struct platform_device *pdev)
{
	mipi_panel_power(0);
}

static struct platform_driver this_driver = {
	.probe = z4_lcd_probe,
	.shutdown = z4_lcd_shutdown,
	.driver = {
		.name   = "mipi_z4",
	},
};

static struct msm_fb_panel_data z4_panel_data = {
	.on		= z4_lcd_on,
	.off		= z4_lcd_off,
	.set_backlight  = z4_set_backlight,
	.display_on     = z4_display_on,
	.display_off    = z4_display_off,
	.bklswitch      = z4_bkl_switch,
	.dimming_on     = z4_dim_on,
#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
	.set_cabc       = z4_set_cabc_lvl,
#endif
};

static int ch_used[3];

int mipi_z4_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_z4", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	z4_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &z4_panel_data,
		sizeof(z4_panel_data));
	if (ret) {
		PR_DISP_ERR("%s: platform_device_add_data failed!\n",
			__func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		PR_DISP_ERR("%s: platform_device_register failed!\n",
			__func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static struct msm_panel_info pinfo;

static struct mipi_dsi_phy_ctrl jdi_nt_phy_ctrl = {
{0x03, 0x01, 0x01, 0x00},
{0xAE, 0x30, 0x19, 0x00, 0x93, 0x96, 0x1D, 0x30,
0x13, 0x03, 0x04},
{0x7f, 0x00, 0x00, 0x00},
{0xbb, 0x02, 0x06, 0x00},
{0x00, 0x92, 0x31, 0xd2, 0x00, 0x40, 0x37, 0x62,
0x01, 0x0f, 0x07,
0x05, 0x14, 0x03, 0x0, 0x0, 0x0, 0x20, 0x0, 0x02, 0x0},
};

static int __init mipi_jdi_nt_init(void)
{
	int ret;

	pinfo.xres = 480;
	pinfo.yres = 800;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	if(panel_type == PANEL_ID_LUPUS_JDI_NT) {
		pinfo.lcdc.h_back_porch = 84;
		pinfo.lcdc.h_front_porch = 86;
		pinfo.lcdc.h_pulse_width = 4;
		pinfo.lcdc.v_back_porch = 30;
		pinfo.lcdc.v_front_porch = 30;
		pinfo.lcdc.v_pulse_width = 4;
	} else {
		pinfo.lcdc.h_back_porch = 100;
		pinfo.lcdc.h_front_porch = 100;
		pinfo.lcdc.h_pulse_width = 4;
		pinfo.lcdc.v_back_porch = 12;
		pinfo.lcdc.v_front_porch = 12;
		pinfo.lcdc.v_pulse_width = 4;
	}
	pinfo.lcdc.border_clr = 0;	
	pinfo.lcdc.underflow_clr = 0x00;	
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.camera_backlight = 201;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = FALSE;
	pinfo.mipi.hfp_power_stop = TRUE;
	pinfo.mipi.hbp_power_stop = TRUE;
	pinfo.mipi.hsa_power_stop = TRUE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_BURST_MODE;
	pinfo.mipi.dst_format =  DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.t_clk_post = 0x20;
	pinfo.mipi.t_clk_pre = 0x2F;
	pinfo.mipi.stream = 0; 
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &jdi_nt_phy_ctrl;
	pinfo.mipi.tx_eot_append = 0x01;

	ret = mipi_z4_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_QHD_PT);

	if (ret)
		PR_DISP_ERR("%s: failed to register device!\n", __func__);

	if(panel_type == PANEL_ID_LUPUS_AUO_NT_C2) {
		power_on_cmd = auo_nt_on_cmds_c2;
                power_on_cmd_count = ARRAY_SIZE(auo_nt_on_cmds_c2);
                display_off_cmd = auo_nt_off_cmds;
                display_off_cmd_count = ARRAY_SIZE(auo_nt_off_cmds);
                strcat(ptype, "PANEL_ID_LUPUS_AUO_NT_C2");
                PR_DISP_INFO("%s: assign initial setting for AUO, %s\n", __func__, ptype);
	} else if(panel_type == PANEL_ID_LUPUS_AUO_NT) {
		power_on_cmd = auo_nt_on_cmds;
		power_on_cmd_count = ARRAY_SIZE(auo_nt_on_cmds);
		display_off_cmd = auo_nt_off_cmds;
		display_off_cmd_count = ARRAY_SIZE(auo_nt_off_cmds);
		strcat(ptype, "PANEL_ID_LUPUS_AUO_NT");
		PR_DISP_INFO("%s: assign initial setting for AUO, %s\n", __func__, ptype);
	} else {
		power_on_cmd = jdi_nt_on_cmds;
		power_on_cmd_count = ARRAY_SIZE(jdi_nt_on_cmds);
		display_off_cmd = jdi_nt_off_cmds;
		display_off_cmd_count = ARRAY_SIZE(jdi_nt_off_cmds);
		strcat(ptype, "PANEL_ID_LUPUS_JDI_NT");
		PR_DISP_INFO("%s: assign initial setting for JDI, %s\n", __func__, ptype);
	}

	return ret;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.width = 56,
	.height = 93,
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
	.cont_splash_enabled = 0x00,
	.gpio = 97,
	.mdp_rev = MDP_REV_303,
};

int __init z4_init_panel(void)
{
	platform_device_register(&msm_fb_device);

	if (panel_type != PANEL_ID_NONE) {
		msm_fb_register_device("mdp", &mdp_pdata);
		msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
	}

	return 0;
}

static int __init z4_panel_panel_init(void)
{
	mipi_dsi_buf_alloc(&z4_panel_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&z4_panel_rx_buf, DSI_BUF_SIZE);

	PR_DISP_INFO("%s: enter 0x%x\n", __func__, panel_type);

	if(panel_type == PANEL_ID_LUPUS_AUO_NT_C2) {
		mipi_jdi_nt_init();
		PR_DISP_INFO("match PANEL_ID_LUPUS_AUO_NT_C2 panel_type\n");
	} else if(panel_type == PANEL_ID_LUPUS_JDI_NT) {
		mipi_jdi_nt_init();
		PR_DISP_INFO("match PANEL_ID_LUPUS_JDI_NT panel_type\n");
	} else if(panel_type == PANEL_ID_LUPUS_AUO_NT) {
		mipi_jdi_nt_init();
		PR_DISP_INFO("match PANEL_ID_LUPUS_AUO_NT panel_type\n");
	} else
		PR_DISP_INFO("Mis-match panel_type\n");

	return platform_driver_register(&this_driver);
}

device_initcall_sync(z4_panel_panel_init);
