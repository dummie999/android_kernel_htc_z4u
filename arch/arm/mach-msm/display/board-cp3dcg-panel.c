/* linux/arch/arm/mach-msm/board-cp3dcg-panel.c
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

#ifdef CONFIG_MACH_CP3DUG
#include "../board-cp3dug.h"
#elif defined CONFIG_MACH_CP3DCG
#include "../board-cp3dcg.h"
#elif defined CONFIG_MACH_CP3DTG
#include "../board-cp3dtg.h"
#elif defined CONFIG_MACH_CP3U
#include "../board-cp3u.h"
#elif defined CONFIG_MACH_Z4DUG
#include "../board-z4dug.h"
#else
#warning ("Warning : no cp3 series config and use cp3 dsiplay file")
#endif

#include "../../../../drivers/video/msm/msm_fb.h"
#include "../../../../drivers/video/msm/mipi_dsi.h"

static int mipi_power_save_on = 1;
static int bl_level_prevset = 0;
static int mipi_status = 1;
static int resume_blk = 0;

static struct dsi_buf cp3dcg_panel_tx_buf;
static struct dsi_buf cp3dcg_panel_rx_buf;
static struct dsi_cmd_desc *power_on_cmd = NULL;

static struct dsi_cmd_desc *display_off_cmd = NULL;
static int power_on_cmd_count = 0;

static int display_off_cmd_count = 0;
static char ptype[60] = "Panel Type = ";

#define DEFAULT_BRIGHTNESS_DCG 166
static unsigned int last_brightness = DEFAULT_BRIGHTNESS_DCG;

static unsigned char led_pwm1[] = {0x51, 0xFF}; 

static char sleep_out[] = {0x11, 0x00}; 
static char display_on[] = {0x29, 0x00}; 

static char display_off[2] = {0x28, 0x00}; 
static char sleep_in[2] = {0x10, 0x00}; 
static struct dsi_cmd_desc backlight_cmds[] = {
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0,
                sizeof(led_pwm1), led_pwm1},
};
static char sony_orise_001[] ={0x00, 0x00}; 
static char sony_orise_002[] = {
        0xFF, 0x96, 0x01, 0x01}; 
static char sony_orise_003[] ={0x00, 0x80}; 
static char sony_orise_004[] = {
        0xFF, 0x96, 0x01};
static char sony_inv_01[] = {0x00, 0xB3};
static char sony_inv_02[] = {0xC0, 0x50};
static char sony_timing1_01[] = {0x00, 0x80};
static char sony_timing1_02[] = {0xF3, 0x04};
static char sony_timing2_01[] = {0x00, 0xC0};
static char sony_timing2_02[] = {0xC2, 0xB0};
static char sony_pwrctl2_01[] = {0x00, 0xA0};
static char sony_pwrctl2_02[] = {
	0xC5, 0x04, 0x3A, 0x56,
	0x44, 0x44, 0x44, 0x44};
static char sony_pwrctl3_01[] = {0x00, 0xB0};
static char sony_pwrctl3_02[] = {
	0xC5, 0x04, 0x3A, 0x56,
	0x44, 0x44, 0x44, 0x44};

static char sony_gamma28_00[] ={0x00, 0x00}; 
static char sony_gamma28_01[] = {
	0xe1, 0x07, 0x10, 0x16,
	0x0F, 0x08, 0x0F, 0x0D,
	0x0C, 0x02, 0x06, 0x0F,
	0x0B, 0x11, 0x0D, 0x07,
	0x00
}; 

static char sony_gamma28_02[] ={0x00, 0x00}; 
static char sony_gamma28_03[] = {
	0xe2, 0x07, 0x10, 0x16,
	0x0F, 0x08, 0x0F, 0x0D,
	0x0C, 0x02, 0x06, 0x0F,
	0x0B, 0x11, 0x0D, 0x07,
	0x00
}; 

static char sony_gamma28_04[] ={0x00, 0x00}; 
static unsigned char sony_gamma28_05[] = {
	0xe3, 0x19, 0x1D, 0x20,
	0x0C, 0x04, 0x0B, 0x0B,
	0x0A, 0x03, 0x07, 0x12,
	0x0B, 0x11, 0x0D, 0x07,
	0x00
}; 

static char sony_gamma28_06[] ={0x00, 0x00}; 
static char sony_gamma28_07[] = {
	0xe4, 0x19, 0x1D, 0x20,
	0x0C, 0x04, 0x0B, 0x0B,
	0x0A, 0x03, 0x07, 0x12,
	0x0B, 0x11, 0x0D, 0x07,
	0x00
}; 

static char sony_gamma28_08[] ={0x00, 0x00}; 
static char sony_gamma28_09[] = {
	0xe5, 0x07, 0x0F, 0x15,
	0x0D, 0x06, 0x0E, 0x0D,
	0x0C, 0x02, 0x06, 0x0F,
	0x09, 0x0D, 0x0D, 0x06,
	0x00
}; 

static char sony_gamma28_10[] ={0x00, 0x00}; 
static char sony_gamma28_11[] = {
	0xe6, 0x07, 0x0F, 0x15,
	0x0D, 0x06, 0x0E, 0x0D,
	0x0C, 0x02, 0x06, 0x0F,
	0x09, 0x0D, 0x0D, 0x06,
	0x00
}; 

static char pwm_freq_sel_cmds1[] = {0x00, 0xB4}; 
static char pwm_freq_sel_cmds2[] = {0xC6, 0x00}; 

static char pwm_dbf_cmds1[] = {0x00, 0xB1}; 
static char pwm_dbf_cmds2[] = {0xC6, 0x04}; 

static char sony_ce_table1[] = {
	0xD4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xfe, 0xfe, 0xfe, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40};

static char sony_ce_table2[] = {
	0xD5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xfe, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xfe, 0xfd, 0xfd, 0xfd, 0x4f, 0x4f, 0x4e,
	0x4e, 0x4e, 0x4e, 0x4e, 0x4e, 0x4e, 0x4e, 0x4d,
	0x4d, 0x4d, 0x4d, 0x4d, 0x4d, 0x4d, 0x4e, 0x4e,
	0x4e, 0x4f, 0x4f, 0x4f, 0x50, 0x50, 0x50, 0x51,
	0x51, 0x51, 0x52, 0x52, 0x52, 0x53, 0x53, 0x53,
	0x54, 0x54, 0x54, 0x54, 0x55, 0x55, 0x55, 0x56,
	0x56, 0x56, 0x56, 0x56, 0x56, 0x56, 0x56, 0x55,
	0x55, 0x55, 0x55, 0x54, 0x54, 0x54, 0x54, 0x54,
	0x53, 0x53, 0x54, 0x54, 0x54, 0x55, 0x55, 0x55,
	0x55, 0x56, 0x56, 0x56, 0x57, 0x57, 0x57, 0x58,
	0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x59, 0x59,
	0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x5a, 0x5a,
	0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a,
	0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x5a, 0x59,
	0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x58,
	0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58,
	0x58, 0x59, 0x59, 0x59, 0x59, 0x59, 0x5a, 0x5a,
	0x5a, 0x5a, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b,
	0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b,
	0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b,
	0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b,
	0x5b, 0x5b, 0x5b, 0x5a, 0x59, 0x58, 0x58, 0x57,
	0x56, 0x55, 0x54, 0x54, 0x53, 0x52, 0x51, 0x50,
	0x50};

static char sony_ce_01[] = {0x00, 0x00};
static char sony_ce_02[] = {0xD6, 0x08};
static char dsi_orise_pwm2[] = {0x53, 0x24};
static char dsi_orise_pwm3[] = {0x55, 0x00};

static struct dsi_cmd_desc sony_orise_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise_001), sony_orise_001},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise_002), sony_orise_002},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise_003), sony_orise_003},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise_004), sony_orise_004},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_inv_01), sony_inv_01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_inv_02), sony_inv_02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_timing1_01), sony_timing1_01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_timing1_02), sony_timing1_02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_timing2_01), sony_timing2_01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_timing2_02), sony_timing2_02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_pwrctl2_01), sony_pwrctl2_01},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_pwrctl2_02), sony_pwrctl2_02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_pwrctl3_01), sony_pwrctl3_01},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_pwrctl3_02), sony_pwrctl3_02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise_001), sony_orise_001},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma28_00), sony_gamma28_00},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma28_01), sony_gamma28_01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma28_02), sony_gamma28_02},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma28_03), sony_gamma28_03},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma28_04), sony_gamma28_04},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma28_05), sony_gamma28_05},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma28_06), sony_gamma28_06},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma28_07), sony_gamma28_07},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma28_08), sony_gamma28_08},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma28_09), sony_gamma28_09},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma28_10), sony_gamma28_10},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma28_11), sony_gamma28_11},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise_001), sony_orise_001},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pwm_freq_sel_cmds1), pwm_freq_sel_cmds1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pwm_freq_sel_cmds2), pwm_freq_sel_cmds2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pwm_dbf_cmds1), pwm_dbf_cmds1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pwm_dbf_cmds2), pwm_dbf_cmds2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_ce_01), sony_ce_01},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_ce_table1), sony_ce_table1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_ce_01), sony_ce_01},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_ce_table2), sony_ce_table2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_ce_01), sony_ce_01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_ce_02), sony_ce_02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(dsi_orise_pwm2), dsi_orise_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(dsi_orise_pwm3), dsi_orise_pwm3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(led_pwm1), led_pwm1},
	{DTYPE_DCS_WRITE,  1, 0, 0, 120, sizeof(sleep_out), sleep_out},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sony_orise_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(sleep_in), sleep_in}
};

static char cmd_set_page1[] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x08, 0x01
};
static char pwr_ctrl_AVDD[] = {0xB6, 0x34};
static char pwr_ctrl_AVEE[] = {0xB7, 0x34};
static char pwr_ctrl_VCL[] = {0xB8, 0x13};
static char pwr_ctrl_VGH[] = {0xB9, 0x22};
static char pwr_ctrl_VGLX[] = {0xBA, 0x23};
static char set_VGMP_VGSP_vol[] = {0xBC, 0x00, 0x88, 0x00};
static char set_VGMN_VGSN_vol[] = {0xBD, 0x00, 0x84, 0x00};
static char GPO_ctrl[] = {0xC0, 0x04, 0x00};
static char gamma_curve_ctrl[] = {0xCF, 0x04};
static char gamma_corr_red1[] = {
	0xD1, 0x00, 0x00, 0x00,
	0x48, 0x00, 0x71, 0x00,
	0x95, 0x00, 0xA4, 0x00,
	0xC1, 0x00, 0xD4, 0x00,
	0xFA
};
static char gamma_corr_red2[] = {
	0xD2, 0x01, 0x22, 0x01,
	0x5F, 0x01, 0x89, 0x01,
	0xCC, 0x02, 0x03, 0x02,
	0x05, 0x02, 0x38, 0x02,
	0x71
};
static char gamma_corr_red3[] = {
	0xD3, 0x02, 0x90, 0x02,
	0xC9, 0x02, 0xF4, 0x03,
	0x1A, 0x03, 0x35, 0x03,
	0x52, 0x03, 0x62, 0x03,
	0x76
};
static char gamma_corr_red4[] = {
	0xD4, 0x03, 0x8F, 0x03,
	0xC0
};
static char normal_display_mode_on[] = {0x13, 0x00};
static char disp_on[] = {0x29, 0x00};
static char cmd_set_page0[] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x08, 0x00
};
static char disp_opt_ctrl[] = {
	0xB1, 0x68, 0x00, 0x01
};
static char disp_scan_line_ctrl[] = {0xB4, 0x78};
static char eq_ctrl[] = {
	0xB8, 0x01, 0x02, 0x02,
	0x02
};
static char inv_drv_ctrl[] = {0xBC, 0x00, 0x00, 0x00};
static char display_timing_control[] = {
	0xC9, 0x63, 0x06, 0x0D,
	0x1A, 0x17, 0x00
};
static char write_ctrl_display[] = {0x53, 0x24};
static char te_on[] = {0x35, 0x00};
static char pwm_freq_ctrl[] = {0xE0, 0x01, 0x03};
static char pwr_blk_enable[] = {
	0xFF, 0xAA, 0x55, 0x25,
	0x01};
static char pwr_blk_disable[] = {
	0xFF, 0xAA, 0x55, 0x25,
	0x00};
static char set_para_idx[] = {0x6F, 0x0A};
static char pwr_blk_sel[] = {0xFA, 0x03};
static char bkl_off[] = {0x53, 0x00};
static char set_cabc_level[] = {0x55, 0x92};

static char vivid_color_setting[] = {
	0xD6, 0x00, 0x05, 0x10,
	0x17, 0x22, 0x26, 0x29,
	0x29, 0x26, 0x23, 0x17,
	0x12, 0x06, 0x02, 0x01,
	0x00};
static char cabc_still[] = {
	0xE3, 0xFF, 0xFB, 0xF3,
	0xEC, 0xE2, 0xCA, 0xC3,
	0xBC, 0xB5, 0xB3
};
static char idx_13[] = {0x6F, 0x13};
static char idx_14[] = {0x6F, 0x14};
static char idx_15[] = {0x6F, 0x15};
static char val_80[] = {0xF5, 0x80};
static char val_FF[] = {0xF5, 0xFF};

static char ctrl_ie_sre[] = {
	0xD4, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00};
static char skin_tone_setting1[] = {
	0xD7, 0x30, 0x30, 0x30,
	0x28, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00};
static char skin_tone_setting2[] = {
	0xD8, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x28,
	0x30, 0x00};
static char test[] = {0xCF, 0x00};
static char rp1[] = {
	0xD1, 0x00, 0x47, 0x00,
	0x60, 0x00, 0x81, 0x00,
	0x9B, 0x00, 0xAC, 0x00,
	0xCE, 0x00, 0xEB, 0x01,
	0x13
};
static char rp2[] = {
	0xD2,0x01,0x34,0x01,
	0x6B,0x01,0x97,0x01,
	0xD9,0x02,0x0F,0x02,
	0x10,0x02,0x46,0x02,
	0x82
};
static char rp3[] = {
	0xD3, 0x02, 0xA8, 0x02,
	0xDF, 0x03, 0x03, 0x03,
	0x2D, 0x03, 0x48, 0x03,
	0x61, 0x03, 0x62, 0x03,
	0x63
};
static char rp4[] = {
	0xD4, 0x03, 0x78, 0x03,
	0x7B
};
static char gp1[] = {
	0xD5, 0x00, 0x4C, 0x00,
	0x60, 0x00, 0x7F, 0x00,
	0x97, 0x00, 0xAB, 0x00,
	0xCC, 0x00, 0xE7, 0x01,
	0x13
};
static char gp2[] = {
	0xD6, 0x01, 0x34, 0x01,
	0x69, 0x01, 0x94, 0x01,
	0xD7, 0x02, 0x0D, 0x02,
	0x0F, 0x02, 0x45, 0x02,
	0x81
};
static char gp3[] = {
	0xD7, 0x02, 0xA8, 0x02,
	0xDC, 0x02, 0xFF, 0x03,
	0x30, 0x03, 0x4F, 0x03,
	0x78, 0x03, 0x9D, 0x03,
	0xE6
};
static char gp4[] = {
	0xD8, 0x03, 0xFE, 0x03,
	0xFE
};
static char bp1[] = {
	0xD9, 0x00, 0x52, 0x00,
	0x60, 0x00, 0x78, 0x00,
	0x8D, 0x00, 0x9F, 0x00,
	0xBE, 0x00, 0xDA, 0x01,
	0x07
};
static char bp2[] = {
	0xDD, 0x01, 0x29, 0x01,
	0x5F, 0x01, 0x8C, 0x01,
	0xD1, 0x02, 0x08, 0x02,
	0x0A, 0x02, 0x42, 0x02,
	0x7D
};
static char bp3[] = {
	0xDE, 0x02, 0xA4, 0x02,
	0xDA, 0x02, 0xFF, 0x03,
	0x38, 0x03, 0x66, 0x03,
	0xFB, 0x03, 0xFC, 0x03,
	0xFD
};
static char bp4[] = {
	0xDF, 0x03, 0xFE, 0x03,
	0xFE
};
static char rn1[] = {
	0xE0, 0x00, 0x47, 0x00,
	0x60, 0x00, 0x81, 0x00,
	0x9B, 0x00, 0xAC, 0x00,
	0xCE, 0x00, 0xEB, 0x01,
	0x13
};
static char rn2[] = {
	0xE1, 0x01, 0x34, 0x01,
	0x6B, 0x01, 0x97, 0x01,
	0xD9, 0x02, 0x0F, 0x02,
	0x10, 0x02, 0x46, 0x02,
	0x82
};
static char rn3[] = {
	0xE2, 0x02, 0xA8, 0x02,
	0xDF, 0x03, 0x03, 0x03,
	0x2D, 0x03, 0x48, 0x03,
	0x61, 0x03, 0x62, 0x03,
	0x63
};
static char rn4[] = {
	0xE3, 0x03, 0x78, 0x03,
	0x7B
};
static char gn1[] = {
	0xE4, 0x00, 0x4C, 0x00,
	0x60, 0x00, 0x7F, 0x00,
	0x97, 0x00, 0xAB, 0x00,
	0xCC, 0x00, 0xE7, 0x01,
	0x13
};
static char gn2[] = {
	0xE5, 0x01, 0x34, 0x01,
	0x69, 0x01, 0x94, 0x01,
	0xD7, 0x02, 0x0D, 0x02,
	0x0F, 0x02, 0x45, 0x02,
	0x81
};
static char gn3[] = {
	0xE6, 0x02, 0xA8, 0x02,
	0xDC, 0x02, 0xFF, 0x03,
	0x30, 0x03, 0x4F, 0x03,
	0x78, 0x03, 0x9D, 0x03,
	0xE6
};
static char gn4[] = {
	0xE7, 0x03, 0xFE, 0x03,
	0xFE
};
static char bn1[] = {
	0xE8, 0x00, 0x52, 0x00,
	0x60, 0x00, 0x78, 0x00,
	0x8D, 0x00, 0x9F, 0x00,
	0xBE, 0x00, 0xDA, 0x01,
	0x07
};
static char bn2[] = {
	0xE9, 0x01, 0x29, 0x01,
	0x5F, 0x01, 0x8C, 0x01,
	0xD1, 0x02, 0x08, 0x02,
	0x0A, 0x02, 0x42, 0x02,
	0x7D
};
static char bn3[] = {
	0xEA, 0x02, 0xA4, 0x02,
	0xDA, 0x02, 0xFF, 0x03,
	0x38, 0x03, 0x66, 0x03,
	0xFB, 0x03, 0xFC, 0x03,
	0xFD
};
static char bn4[] = {
	0xEB, 0x03, 0xFE, 0x03,
	0xFE
};

static struct dsi_cmd_desc lg_novatek_video_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(cmd_set_page1), cmd_set_page1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_ctrl_AVDD), pwr_ctrl_AVDD},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_ctrl_AVEE), pwr_ctrl_AVEE},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_ctrl_VCL), pwr_ctrl_VCL},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_ctrl_VGH), pwr_ctrl_VGH},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_ctrl_VGLX), pwr_ctrl_VGLX},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(set_VGMP_VGSP_vol), set_VGMP_VGSP_vol},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(set_VGMN_VGSN_vol), set_VGMN_VGSN_vol},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(GPO_ctrl), GPO_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(gamma_curve_ctrl), gamma_curve_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(gamma_corr_red1), gamma_corr_red1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(gamma_corr_red2), gamma_corr_red2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(gamma_corr_red3), gamma_corr_red3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(gamma_corr_red4), gamma_corr_red4},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out), sleep_out},

	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cmd_set_page1), cmd_set_page1},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(test), test},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(rp1), rp1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(rp2), rp2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(rp3), rp3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(rp4), rp4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gp1), gp1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gp2), gp2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gp3), gp3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gp4), gp4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(bp1), bp1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(bp2), bp2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(bp3), bp3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(bp4), bp4},

	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(rn1), rn1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(rn2), rn2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(rn3), rn3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(rn4), rn4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gn1), gn1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gn2), gn2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gn3), gn3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gn4), gn4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(bn1), bn1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(bn2), bn2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(bn3), bn3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(bn4), bn4},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(cmd_set_page0), cmd_set_page0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(disp_opt_ctrl), disp_opt_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(disp_scan_line_ctrl), disp_scan_line_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(eq_ctrl), eq_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(inv_drv_ctrl), inv_drv_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(display_timing_control), display_timing_control},
	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(te_on), te_on},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(pwm_freq_ctrl), pwm_freq_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(cabc_still), cabc_still},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(write_ctrl_display), write_ctrl_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(set_cabc_level), set_cabc_level},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(ctrl_ie_sre), ctrl_ie_sre},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(vivid_color_setting), vivid_color_setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(skin_tone_setting1), skin_tone_setting1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(skin_tone_setting2), skin_tone_setting2},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(pwr_blk_enable), pwr_blk_enable},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(set_para_idx), set_para_idx},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_blk_sel), pwr_blk_sel},

	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_13), idx_13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_80), val_80},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_14), idx_14},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_FF), val_FF},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_15), idx_15},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_FF), val_FF},

	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(normal_display_mode_on), normal_display_mode_on},
	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(disp_on), disp_on},
};

static struct dsi_cmd_desc lg_novatek_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(bkl_off), bkl_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 45, sizeof(display_off), display_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 130, sizeof(sleep_in), sleep_in}
};

static char RGBCTR[] = {
	0xB0, 0x00, 0x16, 0x14,
	0x34, 0x34};
static char DPRSLCTR[] = {0xB2, 0x54, 0x01, 0x80};
static char SDHDTCTR[] = {0xB6, 0x0A};
static char GSEQCTR[] = {0xB7, 0x00, 0x22};
static char SDEQCTR[] = {
	0xB8, 0x00, 0x00, 0x07,
	0x00};
static char SDVPCTR[] = {0xBA, 0x02};
static char SGOPCTR[] = {0xBB, 0x44, 0x40};
static char DPFRCTR1[] = {
	0xBD, 0x01, 0xD1, 0x16,
	0x14};
static char DPTMCTR10_2[] = {0xC1, 0x03};
static char DPTMCTR10[] = {
	0xCA, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00};
static char PWMFRCTR[] = {0xE0, 0x01, 0x03};
static char SETAVDD[] = {0xB0, 0x07};
static char SETAVEE[] = {0xB1, 0x07};
static char SETVCL[] = {0xB2, 0x00};
static char SETVGH[] = {0xB3, 0x10};
static char SETVGLX[] = {0xB4, 0x0A};
static char BT1CTR[] = {0xB6, 0x34};
static char BT2CTR[] = {0xB7, 0x35};
static char BT3CTR[] = {0xB8, 0x16};
static char BT4CTR[] = {0xB9, 0x33};
static char BT5CTR[] = {0xBA, 0x15};
static char SETVGL_REG[] = {0xC4, 0x05};
static char GSVCTR[] = {0xCA, 0x21};
static char MAUCCTR[] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x00, 0x00};
static char SETPARIDX[] = {0x6F, 0x01};
static char skew_delay_en[] = {0xF8, 0x24};
static char skew_delay[] = {0xFC, 0x41};

static struct dsi_cmd_desc jdi_novatek_video_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cmd_set_page0), cmd_set_page0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(RGBCTR), RGBCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(DPRSLCTR), DPRSLCTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SDHDTCTR), SDHDTCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(GSEQCTR), GSEQCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(SDEQCTR), SDEQCTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SDVPCTR), SDVPCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(SGOPCTR), SGOPCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(DPFRCTR1), DPFRCTR1},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(DPTMCTR10_2), DPTMCTR10_2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(DPTMCTR10), DPTMCTR10},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(PWMFRCTR), PWMFRCTR},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(cabc_still), cabc_still},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(write_ctrl_display), write_ctrl_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(set_cabc_level), set_cabc_level},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(ctrl_ie_sre), ctrl_ie_sre},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(vivid_color_setting), vivid_color_setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(skin_tone_setting1), skin_tone_setting1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(skin_tone_setting2), skin_tone_setting2},

	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(pwr_blk_enable), pwr_blk_enable},
	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_13), idx_13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_80), val_80},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_14), idx_14},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_FF), val_FF},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_15), idx_15},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_FF), val_FF},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETPARIDX), SETPARIDX},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(skew_delay_en), skew_delay_en},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETPARIDX), SETPARIDX},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(skew_delay), skew_delay},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(pwr_blk_disable), pwr_blk_disable},

	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cmd_set_page1), cmd_set_page1},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETAVDD), SETAVDD},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETAVEE), SETAVEE},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETVCL), SETVCL},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETVGH), SETVGH},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETVGLX), SETVGLX},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT1CTR), BT1CTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT2CTR), BT2CTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT3CTR), BT3CTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT4CTR), BT4CTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT5CTR), BT5CTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETVGL_REG), SETVGL_REG},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(GSVCTR), GSVCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(MAUCCTR), MAUCCTR},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out), sleep_out},
	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(disp_on), disp_on},
};

static struct dsi_cmd_desc jdi_novatek_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 40, sizeof(display_off), display_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(sleep_in), sleep_in}
};

#define LCM_ID0  (34)
#define LCM_ID1  (35)
#define LCM_TE   (97)

static bool dsi_power_on;
static void uranus_panel_power(int on)
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
		if (panel_type != PANEL_ID_URANUS_SONY_ORISE) {
			gpio_tlmm_config(GPIO_CFG(LCM_ID0, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(LCM_ID1, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(LCM_TE, 1,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		}

		if (panel_type == PANEL_ID_URANUS_SONY_ORISE) {
			cpld_gpio_write(CPLD_EXT_GPIO_LCM_3V_EN, 1);
			hr_msleep(10);
			cpld_gpio_write(CPLD_EXT_GPIO_LCMIO_1V8_EN, 1);
			hr_msleep(5);
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 1);
			hr_msleep(10);
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 0);
			hr_msleep(1);
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 1);
			hr_msleep(30);
		} else if (panel_type == PANEL_ID_URANUS_LG_NOVATEK) {
			hr_msleep(1);
			cpld_gpio_write(CPLD_EXT_GPIO_LCMIO_1V8_EN, 1);
			hr_msleep(1);
			cpld_gpio_write(CPLD_EXT_GPIO_LCM_3V_EN, 1);
			hr_msleep(30);
		} else if (panel_type == PANEL_ID_URANUS_JDI_NOVATEK) {
			cpld_gpio_write(CPLD_EXT_GPIO_LCMIO_1V8_EN, 1);
			hr_msleep(2);
			cpld_gpio_write(CPLD_EXT_GPIO_LCM_3V_EN, 1);
			hr_msleep(2);
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 1);
			hr_msleep(1);
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 0);
			hr_msleep(1);
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 1);
			hr_msleep(20);
		}
	} else {
		if (panel_type == PANEL_ID_URANUS_SONY_ORISE) {
			hr_msleep(65);
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 0);
			hr_msleep(2);
			cpld_gpio_write(CPLD_EXT_GPIO_LCMIO_1V8_EN, 0);
			usleep(100);
			cpld_gpio_write(CPLD_EXT_GPIO_LCM_3V_EN, 0);
		} else if (panel_type == PANEL_ID_URANUS_LG_NOVATEK) {
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 0);
			hr_msleep(130);
			cpld_gpio_write(CPLD_EXT_GPIO_LCMIO_1V8_EN, 0);
			usleep(100);
			cpld_gpio_write(CPLD_EXT_GPIO_LCM_3V_EN, 0);
		} else if (panel_type == PANEL_ID_URANUS_JDI_NOVATEK) {
			hr_msleep(10);
			cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 0);
			hr_msleep(5);
			cpld_gpio_write(CPLD_EXT_GPIO_LCM_3V_EN, 0);
			usleep(2);
			cpld_gpio_write(CPLD_EXT_GPIO_LCMIO_1V8_EN, 0);
		}

		if (panel_type != PANEL_ID_URANUS_SONY_ORISE) {
			gpio_tlmm_config(GPIO_CFG(LCM_ID0, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(LCM_ID1, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(LCM_TE, 1,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		}
	}
}

static int mipi_panel_power(int on)
{
	int flag_on = !!on;

	if (mipi_power_save_on == flag_on)
		return 0;

	mipi_power_save_on = flag_on;

	uranus_panel_power(on);

	return 0;
}

static int mipi_panel_power_with_pre_off(int on)
{
	int flag_on = !!on;

	if (mipi_power_save_on == flag_on)
		return 0;

	if (panel_type == PANEL_ID_URANUS_SONY_ORISE)
	{
		mipi_power_save_on = flag_on;
		uranus_panel_power(on);
	}

	return 0;
}

#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
static char cabc_movie[] = {
	0xE3, 0xFF, 0xF6, 0xF0,
	0xEA, 0xD8, 0xC5, 0xB4,
	0x9D, 0x8E, 0x76
};
static struct dsi_cmd_desc cabc_still_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd_set_page0), cmd_set_page0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cabc_still), cabc_still}
};
static struct dsi_cmd_desc cabc_movie_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmd_set_page0), cmd_set_page0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cabc_movie), cabc_movie}
};
static void uranus_set_cabc_lvl(struct msm_fb_data_type *mfd, int mode)
{
	struct dsi_cmd_desc* cabc_cmds = NULL;

	if(mode == 2)
		cabc_cmds = cabc_still_cmds;
	else if(mode == 3)
		cabc_cmds = cabc_movie_cmds;
	else
		return;

	mipi_dsi_cmds_tx(&cp3dcg_panel_tx_buf, cabc_cmds, 2);
}
#endif

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

int uranus_mipi_orise_lcd_pre_off(struct platform_device *pdev) {
	struct msm_fb_data_type *mfd;
	PR_DISP_INFO("%s\n", __func__);
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (panel_type == PANEL_ID_URANUS_SONY_ORISE)
		mipi_dsi_cmds_tx(&cp3dcg_panel_tx_buf, sony_orise_display_off_cmds,
			ARRAY_SIZE(sony_orise_display_off_cmds));
	return 0;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio = 97,
	.dsi_power_save = mipi_panel_power,
	.dsi_power_save_with_pre_off = mipi_panel_power_with_pre_off,
	.get_lane_config = msm_fb_get_lane_config,
	.lcd_pre_off = uranus_mipi_orise_lcd_pre_off,
};

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 142
#define BRI_SETTING_MAX                 255

static unsigned char uranus_shrink_pwm(int val)
{
	unsigned int pwm_min, pwm_default, pwm_max;
	unsigned char shrink_br = BRI_SETTING_MAX;

	pwm_min = 11;
	pwm_default = 88;
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

static int uranus_lcd_on(struct platform_device *pdev)
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

		if (panel_type == PANEL_ID_URANUS_SONY_ORISE) {
			htc_mdp_sem_down(current, &mfd->dma->mutex);
			mipi_dsi_cmds_tx(&cp3dcg_panel_tx_buf, power_on_cmd,power_on_cmd_count);
			htc_mdp_sem_up(&mfd->dma->mutex);
		}
		mfd->first_init_lcd = 0;
		last_brightness = 0;
	} else {
		printk("Display On \n");
		if (panel_type != PANEL_ID_NONE) {
			PR_DISP_INFO("%s\n", ptype);

			if (panel_type == PANEL_ID_URANUS_LG_NOVATEK) {
				hr_msleep(15);
				cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 1);
				hr_msleep(5);
				cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 0);
				hr_msleep(5);
				cpld_gpio_write(CPLD_EXT_GPIO_LCD_RST, 1);
				hr_msleep(30);
			}
			htc_mdp_sem_down(current, &mfd->dma->mutex);
			mipi_dsi_cmds_tx(&cp3dcg_panel_tx_buf, power_on_cmd,
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

static int uranus_lcd_off(struct platform_device *pdev)
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

static void uranus_dim_on(struct msm_fb_data_type *mfd)
{
	mipi_dsi_cmds_tx(&cp3dcg_panel_tx_buf, dim_on_cmd, 1);
        PR_DISP_INFO("%s\n", __func__);
}
static void uranus_dim_off(struct msm_fb_data_type *mfd)
{
	mipi_dsi_cmds_tx(&cp3dcg_panel_tx_buf, dim_off_cmd, 1);
	PR_DISP_INFO("%s\n", __func__);
}

static void uranus_set_backlight(struct msm_fb_data_type *mfd)
{
	PR_DISP_INFO("%s\n", __func__);
	

	if (mipi_status == 0 || bl_level_prevset == mfd->bl_level) {
		PR_DISP_DEBUG("Skip the backlight setting > mipi_status : %d, bl_level_prevset : %d, bl_level : %d\n",
			mipi_status, bl_level_prevset, mfd->bl_level);
		return;
	}

	led_pwm1[1] = uranus_shrink_pwm(mfd->bl_level);



	if (mfd->bl_level == 0) {
		led_pwm1[1] = 0;
	}

	htc_mdp_sem_down(current, &mfd->dma->mutex);

	if(led_pwm1[1] == 0)
		uranus_dim_off(mfd);

	mipi_dsi_cmds_tx(&cp3dcg_panel_tx_buf, backlight_cmds, 1);
	htc_mdp_sem_up(&mfd->dma->mutex);
	bl_level_prevset = mfd->bl_level;

	
	if (mfd->bl_level >= BRI_SETTING_MIN)
		last_brightness = mfd->bl_level;

	PR_DISP_INFO("mipi_dsi_set_backlight > set brightness to %d(%d)\n", led_pwm1[1], mfd->bl_level);

	return;
}

static void uranus_bkl_switch(struct msm_fb_data_type *mfd, bool on)
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
			uranus_set_backlight(mfd);
		}
		mipi_status = 0;
	}
}

static void uranus_display_on(struct msm_fb_data_type *mfd)
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

static void uranus_display_off(struct msm_fb_data_type *mfd)
{

#if 0
	cmdreq.cmds = display_off_cmds;
	cmdreq.cmds_cnt = display_off_cmds_count;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);
#endif
	mipi_dsi_cmds_tx(&cp3dcg_panel_tx_buf, display_off_cmd, display_off_cmd_count);
	PR_DISP_INFO("%s\n", __func__);
}

static int __devinit uranus_lcd_probe(struct platform_device *pdev)
{
	msm_fb_add_device(pdev);

	PR_DISP_INFO("%s\n", __func__);
	return 0;
}

static void uranus_lcd_shutdown(struct platform_device *pdev)
{
	mipi_panel_power(0);
}

static struct platform_driver this_driver = {
	.probe = uranus_lcd_probe,
	.shutdown = uranus_lcd_shutdown,
	.driver = {
		.name   = "mipi_cp3dcg",
	},
};

static struct msm_fb_panel_data cp3dcg_panel_data = {
	.on		= uranus_lcd_on,
	.off		= uranus_lcd_off,
	.set_backlight  = uranus_set_backlight,
	.display_on  = uranus_display_on,
	.display_off  = uranus_display_off,
	.bklswitch      = uranus_bkl_switch,
	.dimming_on = uranus_dim_on,
#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
	.set_cabc = uranus_set_cabc_lvl,
#endif
};

static int ch_used[3];

int mipi_uranus_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_cp3dcg", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	cp3dcg_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &cp3dcg_panel_data,
		sizeof(cp3dcg_panel_data));
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

static struct mipi_dsi_phy_ctrl mipi_dsi_cp3dcg_phy_ctrl = {
	
	
	{0x03, 0x01, 0x01, 0x00},
	
	{0xAE, 0x8B, 0x19, 0x00, 0x93, 0x96, 0x1D, 0x8D,
	0x13, 0x03, 0x04},
	
	{0x7f, 0x00, 0x00, 0x00},
	
	{0xbb, 0x02, 0x06, 0x00},
	
	{0x00, 0x92, 0x31, 0xd2, 0x00, 0x40, 0x37, 0x62,
	0x01, 0x0f, 0x07,
	0x05, 0x14, 0x03, 0x0, 0x0, 0x0, 0x20, 0x0, 0x02, 0x0},
};
static struct mipi_dsi_phy_ctrl mipi_dsi_lg_novatek_phy_ctrl = {
	
	
	{0x03, 0x01, 0x01, 0x00},
	
	{0xb3, 0x30, 0x1c, 0x00, 0x95, 0x9f, 0x1f,
	0x30, 0x15, 0x03, 0x04},
	
	{0x7f, 0x00, 0x00, 0x00},
	
	{0xff, 0x02, 0x06, 0x00}, 
	
	{0x00, 0xbb, 0x31, 0xd2, 0x00, 0x40, 0x37, 0x62,
	0x01, 0x0f, 0x07,
	0x05, 0x14, 0x03, 0x0, 0x0, 0x0, 0x20, 0x0, 0x02, 0x0},
};

static int __init mipi_cmd_sony_orise_init(void)
{
	int ret;

	pinfo.xres = 540;
	pinfo.yres = 960;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.lcdc.h_back_porch = 76;
	pinfo.lcdc.h_front_porch = 16;
	pinfo.lcdc.h_pulse_width = 8;
	pinfo.lcdc.v_back_porch = 64;
	pinfo.lcdc.v_front_porch = 38;
	pinfo.lcdc.v_pulse_width = 8;
	pinfo.lcdc.border_clr = 0;	
	pinfo.lcdc.underflow_clr = 0x00;	
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.camera_backlight = 211;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = FALSE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = FALSE;
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
	pinfo.mipi.frame_rate = 58;
	pinfo.mipi.dsi_phy_db = &mipi_dsi_cp3dcg_phy_ctrl;
	pinfo.mipi.tx_eot_append = 0x01;

	ret = mipi_uranus_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_QHD_PT);

	if (ret)
		PR_DISP_ERR("%s: failed to register device!\n", __func__);

	strcat(ptype, "PANEL_ID_URANUS_SONY_ORISE");
	PR_DISP_INFO("%s: assign initial setting for SONY, %s\n", __func__, ptype);

	power_on_cmd = sony_orise_cmd_on_cmds;
	power_on_cmd_count = ARRAY_SIZE(sony_orise_cmd_on_cmds);
	display_off_cmd = sony_orise_display_off_cmds;
	display_off_cmd_count = ARRAY_SIZE(sony_orise_display_off_cmds);

	return ret;
}

static int __init mipi_cmd_lg_novatek_init(void)
{
	int ret;

	pinfo.xres = 540;
	pinfo.yres = 960;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.lcdc.h_back_porch = 60;
	pinfo.lcdc.h_front_porch = 20;
	pinfo.lcdc.h_pulse_width = 2;
	pinfo.lcdc.v_back_porch = 20;
	pinfo.lcdc.v_front_porch = 20;
	pinfo.lcdc.v_pulse_width = 2;
	pinfo.lcdc.border_clr = 0;	
	pinfo.lcdc.underflow_clr = 0x00;	
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.camera_backlight = 182;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = FALSE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = FALSE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_BURST_MODE;
	pinfo.mipi.dst_format =  DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.t_clk_post = 0x20;
	pinfo.mipi.t_clk_pre = 0x30;
	pinfo.mipi.stream = 0; 
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 58;
	pinfo.mipi.dsi_phy_db = &mipi_dsi_lg_novatek_phy_ctrl;
	pinfo.mipi.tx_eot_append = 0x01;

	ret = mipi_uranus_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_QHD_PT);

	if (ret)
		PR_DISP_ERR("%s: failed to register device!\n", __func__);

	strcat(ptype, "PANEL_ID_URANUS_LG_NOVATEK");
	PR_DISP_INFO("%s: assign initial setting for LG, %s\n", __func__, ptype);

	power_on_cmd = lg_novatek_video_on_cmds;
	power_on_cmd_count = ARRAY_SIZE(lg_novatek_video_on_cmds);
	display_off_cmd = lg_novatek_display_off_cmds;
	display_off_cmd_count = ARRAY_SIZE(lg_novatek_display_off_cmds);

	return ret;
}

static int __init mipi_cmd_jdi_novatek_init(void)
{
	int ret;

	pinfo.xres = 540;
	pinfo.yres = 960;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.lcdc.h_back_porch = 60;
	pinfo.lcdc.h_front_porch = 20;
	pinfo.lcdc.h_pulse_width = 2;
	pinfo.lcdc.v_back_porch = 20;
	pinfo.lcdc.v_front_porch = 20;
	pinfo.lcdc.v_pulse_width = 2;
	pinfo.lcdc.border_clr = 0;	
	pinfo.lcdc.underflow_clr = 0xff;	
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.camera_backlight = 182;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = FALSE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = FALSE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_BURST_MODE;
	pinfo.mipi.dst_format =  DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.t_clk_post = 0x20;
	pinfo.mipi.t_clk_pre = 0x30;
	pinfo.mipi.stream = 0; 
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &mipi_dsi_lg_novatek_phy_ctrl;
	pinfo.mipi.tx_eot_append = 0x01;

	ret = mipi_uranus_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_QHD_PT);

	if (ret)
		PR_DISP_ERR("%s: failed to register device!\n", __func__);

	strcat(ptype, "PANEL_ID_URANUS_JDI_NOVATEK");
	PR_DISP_INFO("%s: assign initial setting for JDI, %s\n", __func__, ptype);

	power_on_cmd = jdi_novatek_video_on_cmds;
	power_on_cmd_count = ARRAY_SIZE(jdi_novatek_video_on_cmds);
	display_off_cmd = jdi_novatek_display_off_cmds;
	display_off_cmd_count = ARRAY_SIZE(jdi_novatek_display_off_cmds);

	return ret;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.width = 55,
	.height = 98,
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

int __init cp3dcg_init_panel(void)
{
	platform_device_register(&msm_fb_device);

	if (panel_type != PANEL_ID_NONE) {
		msm_fb_register_device("mdp", &mdp_pdata);
		msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
	}

	return 0;
}

static int __init uranus_panel_panel_init(void)
{
	mipi_dsi_buf_alloc(&cp3dcg_panel_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&cp3dcg_panel_rx_buf, DSI_BUF_SIZE);

	PR_DISP_INFO("%s: enter 0x%x\n", __func__, panel_type);

	if(panel_type == PANEL_ID_URANUS_SONY_ORISE) {
		mipi_power_save_on = 0;
		mipi_cmd_sony_orise_init();
		PR_DISP_INFO("match PANEL_ID_URANUS_SONY_ORISE panel_type\n");
	} else if(panel_type == PANEL_ID_URANUS_LG_NOVATEK) {
                mipi_cmd_lg_novatek_init();
                PR_DISP_INFO("match PANEL_ID_URANUS_LG_NOVATEK panel_type\n");
	} else if(panel_type == PANEL_ID_URANUS_JDI_NOVATEK) {
                mipi_cmd_jdi_novatek_init();
                PR_DISP_INFO("match PANEL_ID_URANUS_JDI_NOVATEK panel_type\n");
	} else
		PR_DISP_INFO("Mis-match panel_type\n");

	return platform_driver_register(&this_driver);
}

device_initcall_sync(uranus_panel_panel_init);
