/* linux/arch/arm/mach-msm/board-protou-panel.c
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

#include "../devices.h"
#include "../board-protou.h"
#include "../../../../drivers/video/msm/msm_fb.h"
#include "../../../../drivers/video/msm/mipi_dsi.h"

static int mipi_power_save_on = 1;
static int bl_level_prevset = 0;

static int resume_blk = 0;
static int mipi_status = 1;

static struct dsi_buf protou_panel_tx_buf;
static struct dsi_buf protou_panel_rx_buf;
static struct dsi_cmd_desc *power_on_cmd = NULL;
static struct dsi_cmd_desc *display_on_cmds = NULL;
static struct dsi_cmd_desc *display_off_cmds = NULL;
static int power_on_cmd_size = 0;
static int display_on_cmds_count = 0;
static int display_off_cmds_count = 0;
static char ptype[60] = "Panel Type = ";

#define DEFAULT_BRIGHTNESS 143
static unsigned int last_brightness = DEFAULT_BRIGHTNESS;

void mdp_color_enhancement(const struct mdp_reg *reg_seq, int size);

static struct mdp_reg protou_color_v105[] = {
	{0x93400, 0x0211, 0x0},
	{0x93404, 0xFFF2, 0x0},
	{0x93408, 0xFFFF, 0x0},
	{0x9340C, 0xFFF8, 0x0},
	{0x93410, 0x0209, 0x0},
	{0x93414, 0xFFFC, 0x0},
	{0x93418, 0xFFF8, 0x0},
	{0x9341C, 0xFFF4, 0x0},
	{0x93420, 0x0217, 0x0},
	{0x93600, 0x0000, 0x0},
	{0x93604, 0x00FF, 0x0},
	{0x93608, 0x0000, 0x0},
	{0x9360C, 0x00FF, 0x0},
	{0x93610, 0x0000, 0x0},
	{0x93614, 0x00FF, 0x0},
	{0x93680, 0x0000, 0x0},
	{0x93684, 0x00FF, 0x0},
	{0x93688, 0x0000, 0x0},
	{0x9368C, 0x00FF, 0x0},
	{0x93690, 0x0000, 0x0},
	{0x93694, 0x00FF, 0x0},
};

static int protou_mdp_color_enhance(void)
{
	mdp_color_enhancement(protou_color_v105, ARRAY_SIZE(protou_color_v105));

	return 0;
}

static struct mdp_reg protou_gamma[] = {
	{0x93800, 0x000000, 0x0},
	{0x93804, 0x010000, 0x0},
	{0x93808, 0x020101, 0x0},
	{0x9380C, 0x030202, 0x0},
	{0x93810, 0x040303, 0x0},
	{0x93814, 0x050404, 0x0},
	{0x93818, 0x060505, 0x0},
	{0x9381C, 0x070606, 0x0},
	{0x93820, 0x080707, 0x0},
	{0x93824, 0x090808, 0x0},
	{0x93828, 0x0A0909, 0x0},
	{0x9382C, 0x0B0A0A, 0x0},
	{0x93830, 0x0C0B0B, 0x0},
	{0x93834, 0x0D0C0C, 0x0},
	{0x93838, 0x0E0D0D, 0x0},
	{0x9383C, 0x0F0E0E, 0x0},
	{0x93840, 0x100E0E, 0x0},
	{0x93844, 0x110F0F, 0x0},
	{0x93848, 0x121010, 0x0},
	{0x9384C, 0x131111, 0x0},
	{0x93850, 0x141212, 0x0},
	{0x93854, 0x151313, 0x0},
	{0x93858, 0x161414, 0x0},
	{0x9385C, 0x171515, 0x0},
	{0x93860, 0x181616, 0x0},
	{0x93864, 0x191717, 0x0},
	{0x93868, 0x1A1818, 0x0},
	{0x9386C, 0x1B1919, 0x0},
	{0x93870, 0x1C1A1A, 0x0},
	{0x93874, 0x1D1B1B, 0x0},
	{0x93878, 0x1E1C1C, 0x0},
	{0x9387C, 0x1F1D1D, 0x0},
	{0x93880, 0x201D1D, 0x0},
	{0x93884, 0x211E1E, 0x0},
	{0x93888, 0x221F1F, 0x0},
	{0x9388C, 0x232020, 0x0},
	{0x93890, 0x242121, 0x0},
	{0x93894, 0x252222, 0x0},
	{0x93898, 0x262323, 0x0},
	{0x9389C, 0x272424, 0x0},
	{0x938A0, 0x282525, 0x0},
	{0x938A4, 0x292626, 0x0},
	{0x938A8, 0x2A2727, 0x0},
	{0x938AC, 0x2B2828, 0x0},
	{0x938B0, 0x2C2929, 0x0},
	{0x938B4, 0x2D2A2A, 0x0},
	{0x938B8, 0x2E2B2B, 0x0},
	{0x938BC, 0x2F2C2C, 0x0},
	{0x938C0, 0x302C2C, 0x0},
	{0x938C4, 0x312D2D, 0x0},
	{0x938C8, 0x322E2E, 0x0},
	{0x938CC, 0x332F2F, 0x0},
	{0x938D0, 0x343030, 0x0},
	{0x938D4, 0x353131, 0x0},
	{0x938D8, 0x363232, 0x0},
	{0x938DC, 0x373333, 0x0},
	{0x938E0, 0x383434, 0x0},
	{0x938E4, 0x393535, 0x0},
	{0x938E8, 0x3A3636, 0x0},
	{0x938EC, 0x3B3737, 0x0},
	{0x938F0, 0x3C3838, 0x0},
	{0x938F4, 0x3D3939, 0x0},
	{0x938F8, 0x3E3A3A, 0x0},
	{0x938FC, 0x3F3B3B, 0x0},
	{0x93900, 0x403B3B, 0x0},
	{0x93904, 0x413C3C, 0x0},
	{0x93908, 0x423D3D, 0x0},
	{0x9390C, 0x433E3E, 0x0},
	{0x93910, 0x443F3F, 0x0},
	{0x93914, 0x454040, 0x0},
	{0x93918, 0x464141, 0x0},
	{0x9391C, 0x474242, 0x0},
	{0x93920, 0x484343, 0x0},
	{0x93924, 0x494444, 0x0},
	{0x93928, 0x4A4545, 0x0},
	{0x9392C, 0x4B4646, 0x0},
	{0x93930, 0x4C4747, 0x0},
	{0x93934, 0x4D4848, 0x0},
	{0x93938, 0x4E4949, 0x0},
	{0x9393C, 0x4F4A4A, 0x0},
	{0x93940, 0x504A4A, 0x0},
	{0x93944, 0x514B4B, 0x0},
	{0x93948, 0x524C4C, 0x0},
	{0x9394C, 0x534D4D, 0x0},
	{0x93950, 0x544E4E, 0x0},
	{0x93954, 0x554F4F, 0x0},
	{0x93958, 0x565050, 0x0},
	{0x9395C, 0x575151, 0x0},
	{0x93960, 0x585252, 0x0},
	{0x93964, 0x595353, 0x0},
	{0x93968, 0x5A5454, 0x0},
	{0x9396C, 0x5B5555, 0x0},
	{0x93970, 0x5C5656, 0x0},
	{0x93974, 0x5D5757, 0x0},
	{0x93978, 0x5E5858, 0x0},
	{0x9397C, 0x5F5959, 0x0},
	{0x93980, 0x605959, 0x0},
	{0x93984, 0x615A5A, 0x0},
	{0x93988, 0x625B5B, 0x0},
	{0x9398C, 0x635C5C, 0x0},
	{0x93990, 0x645D5D, 0x0},
	{0x93994, 0x655E5E, 0x0},
	{0x93998, 0x665F5F, 0x0},
	{0x9399C, 0x676060, 0x0},
	{0x939A0, 0x686161, 0x0},
	{0x939A4, 0x696262, 0x0},
	{0x939A8, 0x6A6363, 0x0},
	{0x939AC, 0x6B6464, 0x0},
	{0x939B0, 0x6C6565, 0x0},
	{0x939B4, 0x6D6666, 0x0},
	{0x939B8, 0x6E6767, 0x0},
	{0x939BC, 0x6F6868, 0x0},
	{0x939C0, 0x706868, 0x0},
	{0x939C4, 0x716969, 0x0},
	{0x939C8, 0x726A6A, 0x0},
	{0x939CC, 0x736B6B, 0x0},
	{0x939D0, 0x746C6C, 0x0},
	{0x939D4, 0x756D6D, 0x0},
	{0x939D8, 0x766E6E, 0x0},
	{0x939DC, 0x776F6F, 0x0},
	{0x939E0, 0x787070, 0x0},
	{0x939E4, 0x797171, 0x0},
	{0x939E8, 0x7A7272, 0x0},
	{0x939EC, 0x7B7373, 0x0},
	{0x939F0, 0x7C7474, 0x0},
	{0x939F4, 0x7D7575, 0x0},
	{0x939F8, 0x7E7676, 0x0},
	{0x939FC, 0x7F7777, 0x0},
	{0x93A00, 0x807777, 0x0},
	{0x93A04, 0x817878, 0x0},
	{0x93A08, 0x827979, 0x0},
	{0x93A0C, 0x837A7A, 0x0},
	{0x93A10, 0x847B7B, 0x0},
	{0x93A14, 0x857C7C, 0x0},
	{0x93A18, 0x867D7D, 0x0},
	{0x93A1C, 0x877E7E, 0x0},
	{0x93A20, 0x887F7F, 0x0},
	{0x93A24, 0x898080, 0x0},
	{0x93A28, 0x8A8181, 0x0},
	{0x93A2C, 0x8B8282, 0x0},
	{0x93A30, 0x8C8383, 0x0},
	{0x93A34, 0x8D8484, 0x0},
	{0x93A38, 0x8E8585, 0x0},
	{0x93A3C, 0x8F8686, 0x0},
	{0x93A40, 0x908686, 0x0},
	{0x93A44, 0x918787, 0x0},
	{0x93A48, 0x928888, 0x0},
	{0x93A4C, 0x938989, 0x0},
	{0x93A50, 0x938A8A, 0x0},
	{0x93A54, 0x958B8B, 0x0},
	{0x93A58, 0x968C8C, 0x0},
	{0x93A5C, 0x978D8D, 0x0},
	{0x93A60, 0x988E8E, 0x0},
	{0x93A64, 0x998F8F, 0x0},
	{0x93A68, 0x9A9090, 0x0},
	{0x93A6C, 0x9B9191, 0x0},
	{0x93A70, 0x9C9292, 0x0},
	{0x93A74, 0x9D9393, 0x0},
	{0x93A78, 0x9E9494, 0x0},
	{0x93A7C, 0x9F9595, 0x0},
	{0x93A80, 0xA09595, 0x0},
	{0x93A84, 0xA19696, 0x0},
	{0x93A88, 0xA29797, 0x0},
	{0x93A8C, 0xA39898, 0x0},
	{0x93A90, 0xA49999, 0x0},
	{0x93A94, 0xA59A9A, 0x0},
	{0x93A98, 0xA69B9B, 0x0},
	{0x93A9C, 0xA79C9C, 0x0},
	{0x93AA0, 0xA89D9D, 0x0},
	{0x93AA4, 0xA99E9E, 0x0},
	{0x93AA8, 0xAA9F9F, 0x0},
	{0x93AAC, 0xABA0A0, 0x0},
	{0x93AB0, 0xACA1A1, 0x0},
	{0x93AB4, 0xADA2A2, 0x0},
	{0x93AB8, 0xAEA3A3, 0x0},
	{0x93ABC, 0xAFA4A4, 0x0},
	{0x93AC0, 0xB0A4A4, 0x0},
	{0x93AC4, 0xB1A5A5, 0x0},
	{0x93AC8, 0xB2A6A6, 0x0},
	{0x93ACC, 0xB3A7A7, 0x0},
	{0x93AD0, 0xB4A8A8, 0x0},
	{0x93AD4, 0xB5A9A9, 0x0},
	{0x93AD8, 0xB6AAAA, 0x0},
	{0x93ADC, 0xB7ABAB, 0x0},
	{0x93AE0, 0xB8ACAC, 0x0},
	{0x93AE4, 0xB9ADAD, 0x0},
	{0x93AE8, 0xBAAEAE, 0x0},
	{0x93AEC, 0xBBAFAF, 0x0},
	{0x93AF0, 0xBCB0B0, 0x0},
	{0x93AF4, 0xBDB1B1, 0x0},
	{0x93AF8, 0xBEB2B2, 0x0},
	{0x93AFC, 0xBFB3B3, 0x0},
	{0x93B00, 0xC0B3B3, 0x0},
	{0x93B04, 0xC1B4B4, 0x0},
	{0x93B08, 0xC2B5B5, 0x0},
	{0x93B0C, 0xC3B6B6, 0x0},
	{0x93B10, 0xC4B7B7, 0x0},
	{0x93B14, 0xC5B8B8, 0x0},
	{0x93B18, 0xC6B9B9, 0x0},
	{0x93B1C, 0xC7BABA, 0x0},
	{0x93B20, 0xC8BBBB, 0x0},
	{0x93B24, 0xC9BCBC, 0x0},
	{0x93B28, 0xCABDBD, 0x0},
	{0x93B2C, 0xCBBEBE, 0x0},
	{0x93B30, 0xCCBFBF, 0x0},
	{0x93B34, 0xCDC0C0, 0x0},
	{0x93B38, 0xCEC1C1, 0x0},
	{0x93B3C, 0xCFC2C2, 0x0},
	{0x93B40, 0xD0C2C2, 0x0},
	{0x93B44, 0xD1C3C3, 0x0},
	{0x93B48, 0xD2C4C4, 0x0},
	{0x93B4C, 0xD3C5C5, 0x0},
	{0x93B50, 0xD4C6C6, 0x0},
	{0x93B54, 0xD5C7C7, 0x0},
	{0x93B58, 0xD6C8C8, 0x0},
	{0x93B5C, 0xD7C9C9, 0x0},
	{0x93B60, 0xD8CACA, 0x0},
	{0x93B64, 0xD9CBCB, 0x0},
	{0x93B68, 0xDACCCC, 0x0},
	{0x93B6C, 0xDBCDCD, 0x0},
	{0x93B70, 0xDCCECE, 0x0},
	{0x93B74, 0xDDCFCF, 0x0},
	{0x93B78, 0xDED0D0, 0x0},
	{0x93B7C, 0xDFD1D1, 0x0},
	{0x93B80, 0xE0D1D1, 0x0},
	{0x93B84, 0xE1D2D2, 0x0},
	{0x93B88, 0xE2D3D3, 0x0},
	{0x93B8C, 0xE3D4D4, 0x0},
	{0x93B90, 0xE4D5D5, 0x0},
	{0x93B94, 0xE5D6D6, 0x0},
	{0x93B98, 0xE6D7D7, 0x0},
	{0x93B9C, 0xE7D8D8, 0x0},
	{0x93BA0, 0xE8D9D9, 0x0},
	{0x93BA4, 0xE9DADA, 0x0},
	{0x93BA8, 0xEADBDB, 0x0},
	{0x93BAC, 0xEBDCDC, 0x0},
	{0x93BB0, 0xECDDDD, 0x0},
	{0x93BB4, 0xEDDEDE, 0x0},
	{0x93BB8, 0xEEDFDF, 0x0},
	{0x93BBC, 0xEFE0E0, 0x0},
	{0x93BC0, 0xF0E0E0, 0x0},
	{0x93BC4, 0xF1E1E1, 0x0},
	{0x93BC8, 0xF2E2E2, 0x0},
	{0x93BCC, 0xF3E3E3, 0x0},
	{0x93BD0, 0xF4E4E4, 0x0},
	{0x93BD4, 0xF5E5E5, 0x0},
	{0x93BD8, 0xF6E6E6, 0x0},
	{0x93BDC, 0xF7E7E7, 0x0},
	{0x93BE0, 0xF8E8E8, 0x0},
	{0x93BE4, 0xF9E9E9, 0x0},
	{0x93BE8, 0xFAEAEA, 0x0},
	{0x93BEC, 0xFBEBEB, 0x0},
	{0x93BF0, 0xFCECEC, 0x0},
	{0x93BF4, 0xFDEDED, 0x0},
	{0x93BF8, 0xFEEEEE, 0x0},
	{0x93BFC, 0xFFEFEF, 0x0},
	{0x90070, 0x1F, 0x0}
};

static int protou_mdp_gamma(void)
{
	mdp_color_enhancement(protou_gamma, ARRAY_SIZE(protou_gamma));

	return 0;
}

static unsigned char led_pwm1[] = {0x51, 0xFF}; 
static unsigned char bkl_enable_cmds[] = {0x53, 0x24};
static unsigned char bkl_disable_cmds[] = {0x53, 0x00};
static char enable_cabc[] = {0x55, 0x01}; 
static char write_cabc_minimum_brightness[] = {0x5E, 0xB3}; 

static char sleep_out[] = {0x11, 0x00}; 
static char display_on[] = {0x29, 0x00}; 

static char display_off[2] = {0x28, 0x00}; 
static char sleep_in[2] = {0x10, 0x00}; 

static char display_inv[] = {0x20, 0x00}; 

static char set_address_mode[] = {0x36, 0x00}; 
static char interface_pixel_format[] = {0x3A, 0x70}; 

static char panel_characteristics_setting[] = {
	0xB2, 0x20, 0xC8}; 
static char panel_drive_setting[] = {0xB3, 0x00}; 
static char display_mode_control[] = {0xB4, 0x04}; 
static char display_control1[] = {0xB5, 0x20, 0x10, 0x10}; 
static char display_control2[] = {
	0xB6, 0x03, 0x0F, 0x02, 0x40, 0x10, 0xE8}; 
static char internal_oscillator_setting[] = {
	0xC0, 0x01, 0x1A}; 
static char power_control2[] = {0xC2, 0x00}; 
static char power_control3[] = {
	0xC3, 0x07, 0x05, 0x05, 0x05, 0x07}; 
static char power_control4[] = {
	0xC4, 0x12, 0x24, 0x12, 0x12, 0x05, 0x4c}; 
static char mtp_vocm[] = {0xF9, 0x40}; 
static char power_control6[] = {
	0xC6, 0x41, 0x63, 0x03}; 

static char positive_gamma_red[] = {
	0xD0, 0x03, 0x10, 0x73, 0x07, 0x00, 0x01, 0x50,
	0x13, 0x02}; 

static char negative_gamma_red[] = {
	0xD1, 0x03, 0x10, 0x73, 0x07, 0x00, 0x02, 0x50,
	0x13, 0x02}; 

static char positive_gamma_green[] = {
	0xD2, 0x03, 0x10, 0x73, 0x07, 0x00, 0x01, 0x50,
	0x13, 0x02}; 

static char negative_gamma_green[] = {
	0xD3, 0x03, 0x10, 0x73, 0x07, 0x00, 0x02, 0x50,
	0x13, 0x02}; 

static char positive_gamma_blue[] = {
	0xD4, 0x03, 0x10, 0x73, 0x07, 0x00, 0x01, 0x50,
	0x13, 0x02}; 

static char negative_gamma_blue[] = {
	0xD5, 0x03, 0x10, 0x73, 0x07, 0x00, 0x02, 0x50,
	0x13, 0x02}; 

static char disable_high_speed_timeout[] = {0x03, 0x00}; 
static char backlight_control[] = {
	0xC8, 0x11, 0x03}; 

static char pro_001[] = {0xff, 0x80, 0x09, 0x01, 0x01};
static char pro_002[] = {0x00, 0x80};
static char pro_003[] = {0xff, 0x80, 0x09};
static char pro_004[] = {0x00, 0x97};
static char pro_005[] = {0xce, 0x26};
static char pro_006[] = {0x00, 0x9a};
static char pro_007[] = {0xce, 0x27};
static char pro_008[] = {0x00, 0xc7};
static char pro_009[] = {0xcf, 0x88};
static char pro_010[] = {0x00, 0xb1};
static char pro_011[] = {0xc6, 0x0a};
static char pro_012[] = {0x00, 0x00};
static char pro_gamma_positive[] = {
	0xE1, 0x00, 0x0a, 0x11,
	0x0f, 0x09, 0x11, 0x0d,
	0x0b, 0x02, 0x06, 0x06,
	0x03, 0x10, 0x14, 0x10,
	0x06};
static char pro_gamma_negative[] = {
	0xE2, 0x00, 0x0a, 0x11,
	0x0f, 0x09, 0x11, 0x0d,
	0x0b, 0x02, 0x06, 0x06,
	0x03, 0x10, 0x14, 0x10,
	0x06};
static char pro_gamma_red[] = {
	0xEC, 0x30, 0x53, 0x53,
	0x55, 0x34, 0x33, 0x43,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x34, 0x44,
	0x34, 0x44, 0x43, 0x54,
	0x44, 0x33, 0x43, 0x33,
	0x44, 0x54, 0x55, 0x34,
	0x44, 0x44};
static char pro_gamma_green[] = {
	0xED, 0x40, 0x44, 0x54,
	0x55, 0x44, 0x33, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x43, 0x44, 0x44, 0x44,
	0x44, 0x34, 0x44, 0x44,
	0x34, 0x55, 0x44, 0x44,
	0x44, 0x44};
static char pro_gamma_blue[] = {
	0xEE, 0x30, 0x53, 0x55,
	0x54, 0x44, 0x33, 0x43,
	0x44, 0x54, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x43, 0x43, 0x44, 0x45,
	0x43, 0x44, 0x44, 0x44,
	0x43, 0x55, 0x44, 0x44,
	0x34, 0x44};

static struct dsi_cmd_desc lg_video_on_cmds[] = {
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0,
		sizeof(disable_high_speed_timeout), disable_high_speed_timeout},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_inv), display_inv},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(set_address_mode), set_address_mode},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(interface_pixel_format), interface_pixel_format},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(panel_characteristics_setting), panel_characteristics_setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(panel_drive_setting), panel_drive_setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(display_mode_control), display_mode_control},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(display_control1), display_control1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(display_control2), display_control2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(internal_oscillator_setting), internal_oscillator_setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(power_control2), power_control2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(power_control3), power_control3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(power_control4), power_control4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(mtp_vocm), mtp_vocm},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(power_control6), power_control6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(positive_gamma_red), positive_gamma_red},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(negative_gamma_red), negative_gamma_red},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(positive_gamma_green), positive_gamma_green},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(negative_gamma_green), negative_gamma_green},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(positive_gamma_blue), positive_gamma_blue},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(negative_gamma_blue), negative_gamma_blue},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(write_cabc_minimum_brightness), write_cabc_minimum_brightness},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(enable_cabc), enable_cabc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(backlight_control), backlight_control},
	{DTYPE_DCS_WRITE, 1, 0, 0, 100,
		sizeof(sleep_out), sleep_out},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc lg_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc lg_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 100,
		sizeof(sleep_in), sleep_in},
};

static struct dsi_cmd_desc sharp_orise_video_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out), sleep_out},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(pro_001), pro_001},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_002), pro_002},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(pro_003), pro_003},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_004), pro_004},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_005), pro_005},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_006), pro_006},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_007), pro_007},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_008), pro_008},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_009), pro_009},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_010), pro_010},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_011), pro_011},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pro_012), pro_012},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(pro_gamma_positive), pro_gamma_positive},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(pro_gamma_negative), pro_gamma_negative},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(pro_gamma_red), pro_gamma_red},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(pro_gamma_green), pro_gamma_green},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(pro_gamma_blue), pro_gamma_blue},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(bkl_enable_cmds), bkl_enable_cmds},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(enable_cabc), enable_cabc},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sharp_C1_orise_video_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out), sleep_out},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(bkl_enable_cmds), bkl_enable_cmds},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(enable_cabc), enable_cabc},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc orise_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_in), sleep_in}
};


static struct dsi_cmd_desc backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(led_pwm1), led_pwm1},
};

static struct dsi_cmd_desc lg_bkl_enable_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(backlight_control), backlight_control},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(bkl_enable_cmds), bkl_enable_cmds},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(enable_cabc), enable_cabc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(write_cabc_minimum_brightness), write_cabc_minimum_brightness},
};

static struct dsi_cmd_desc lg_bkl_disable_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(bkl_disable_cmds), bkl_disable_cmds},
};

#if 0
static struct dsi_cmd_desc lg_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};
#endif

#define PROTOU_GPIO_LCD_ID0             (34)
#define PROTOU_GPIO_LCD_ID1             (35)
#define PROTOU_GPIO_MDDI_TE             (97)
#define PROTOU_GPIO_LCD_RST_N           (118)
#define PROTOU_GPIO_LCM_1v8_EN             (5)
#define PROTOU_GPIO_LCM_3v_EN             (6)

static void protou_panel_power(int on)
{

	PR_DISP_INFO("%s: power %s.\n", __func__, on ? "on" : "off");

	if (on) {
		if (panel_type == PANEL_ID_PROTOU_SHARP || panel_type == PANEL_ID_PROTOU_SHARP_C1) {
			gpio_set_value(PROTOU_GPIO_LCD_RST_N, 0);

			gpio_set_value(PROTOU_GPIO_LCM_3v_EN, 1);
			usleep(100);
			gpio_set_value(PROTOU_GPIO_LCM_1v8_EN, 1);

			msleep(15);
			gpio_set_value(PROTOU_GPIO_LCD_RST_N, 1);
			msleep(1);
			gpio_set_value(PROTOU_GPIO_LCD_RST_N, 0);
			msleep(1);
			gpio_set_value(PROTOU_GPIO_LCD_RST_N, 1);
			msleep(25);
		} else {
			gpio_set_value(PROTOU_GPIO_LCD_RST_N, 0);

			gpio_set_value(PROTOU_GPIO_LCM_3v_EN, 1);
			usleep(100);
			gpio_set_value(PROTOU_GPIO_LCM_1v8_EN, 1);

			msleep(2);
			gpio_set_value(PROTOU_GPIO_LCD_RST_N, 1);
			msleep(15);
		}
	} else {
		msleep(65);
		gpio_set_value(PROTOU_GPIO_LCD_RST_N, 0);
		msleep(10);
		gpio_set_value(PROTOU_GPIO_LCM_1v8_EN, 0);
		usleep(100);
		gpio_set_value(PROTOU_GPIO_LCM_3v_EN, 0);
	}
}

static int mipi_panel_power(int on)
{
	int flag_on = !!on;

	if (mipi_power_save_on == flag_on)
		return 0;

	mipi_power_save_on = flag_on;

	protou_panel_power(on);

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
	.dlane_swap = 0x01,
};

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 142
#define BRI_SETTING_MAX                 255

static unsigned char protou_shrink_pwm(int val)
{
	unsigned int pwm_min, pwm_default, pwm_max;
	unsigned char shrink_br = BRI_SETTING_MAX;

	if (panel_type == PANEL_ID_PROTOU_LG) {
		pwm_min = 9;
		pwm_default = 136;
		pwm_max = 242;
	} else if (panel_type == PANEL_ID_PROTOU_SHARP || panel_type == PANEL_ID_PROTOU_SHARP_C1) {
		pwm_min = 11;
		pwm_default = 136;
		pwm_max = 255;
	} else {
		pwm_min = 10;
		pwm_default = 133;
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

	

	return shrink_br;
}

static int protou_lcd_on(struct platform_device *pdev)
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

		if (panel_type == PANEL_ID_PROTOU_SHARP || panel_type == PANEL_ID_PROTOU_SHARP_C1) {
			mipi_dsi_cmds_tx(&protou_panel_tx_buf, power_on_cmd,
					power_on_cmd_size);
		}

		mfd->first_init_lcd = 0;
		last_brightness = 0;
	} else {
		printk("Display On \n");
		if (panel_type != PANEL_ID_NONE) {
			PR_DISP_INFO("%s\n", ptype);

			htc_mdp_sem_down(current, &mfd->dma->mutex);
			mipi_dsi_cmds_tx(&protou_panel_tx_buf, power_on_cmd,
				power_on_cmd_size);
			htc_mdp_sem_up(&mfd->dma->mutex);
		} else {
			printk(KERN_ERR "panel_type=0x%x not support at power on\n", panel_type);
			return -EINVAL;
		}
	}

	PR_DISP_INFO("Init done!\n");
	return 0;
}

static int protou_lcd_off(struct platform_device *pdev)
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

int protou_lcd_off2(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	PR_DISP_INFO("%s\n", __func__);

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(&protou_panel_tx_buf, display_off_cmds,
		display_off_cmds_count);
	resume_blk = 1;

	return 0;
}

int protou_orise_lcd_pre_off(struct platform_device *pdev) {
	struct msm_fb_data_type *mfd;

	PR_DISP_INFO("%s\n", __func__);


	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(&protou_panel_tx_buf, orise_display_off_cmds,
			ARRAY_SIZE(orise_display_off_cmds));

	return 0;
}

static void protou_set_backlight(struct msm_fb_data_type *mfd)
{
	PR_DISP_INFO("%s\n", __func__);

	

	if (mipi_status == 0 || bl_level_prevset == mfd->bl_level) {
		PR_DISP_DEBUG("Skip the backlight setting > mipi_status : %d, bl_level_prevset : %d, bl_level : %d\n",
			mipi_status, bl_level_prevset, mfd->bl_level);
		return;
	}

	led_pwm1[1] = protou_shrink_pwm(mfd->bl_level);

	if (mfd->bl_level == 0) {
		led_pwm1[1] = 0;
	}

	htc_mdp_sem_down(current, &mfd->dma->mutex);
	if (panel_type == PANEL_ID_PROTOU_LG) {
		
		MIPI_OUTP(MIPI_DSI_BASE + 0xA8, 0x10000000);
		mipi_dsi_cmd_bta_sw_trigger();

		
		if (led_pwm1[1] == 0) {
				mipi_dsi_cmds_tx(&protou_panel_tx_buf, lg_bkl_disable_cmds,1 );
		} else if (bl_level_prevset == 0) {
				mipi_dsi_cmds_tx(&protou_panel_tx_buf, lg_bkl_enable_cmds, 4);
		}
	}

	mipi_dsi_cmds_tx(&protou_panel_tx_buf, backlight_cmds, 1);
	bl_level_prevset = mfd->bl_level;
	htc_mdp_sem_up(&mfd->dma->mutex);

	
	if (mfd->bl_level >= BRI_SETTING_MIN)
		last_brightness = mfd->bl_level;

	PR_DISP_INFO("mipi_dsi_set_backlight > set brightness to %d(%d)\n", led_pwm1[1], mfd->bl_level);

	return;
}

static void protou_bkl_switch(struct msm_fb_data_type *mfd, bool on)
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
			protou_set_backlight(mfd);
		}
		mipi_status = 0;
	}
}


static void protou_display_on(struct msm_fb_data_type *mfd)
{
#if 0
	htc_mdp_sem_down(current, &mfd->dma->mutex);
	mipi_dsi_cmds_tx(&protou_panel_tx_buf, display_on_cmds, display_on_cmds_count);
	htc_mdp_sem_up(&mfd->dma->mutex);
#endif
	PR_DISP_INFO("%s\n", __func__);
}

static void protou_display_off(struct msm_fb_data_type *mfd)
{
#if 0
	cmdreq.cmds = display_off_cmds;
	cmdreq.cmds_cnt = display_off_cmds_count;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);
#endif
	mipi_dsi_cmds_tx(&protou_panel_tx_buf, display_off_cmds, display_off_cmds_count);
	PR_DISP_INFO("%s\n", __func__);
}

static int __devinit protou_lcd_probe(struct platform_device *pdev)
{
	msm_fb_add_device(pdev);

	PR_DISP_INFO("%s\n", __func__);
	return 0;
}

static void protou_lcd_shutdown(struct platform_device *pdev)
{
	mipi_panel_power(0);
}

static struct platform_driver this_driver = {
	.probe = protou_lcd_probe,
	.shutdown = protou_lcd_shutdown,
	.driver = {
		.name   = "mipi_protou",
	},
};

static struct msm_fb_panel_data protou_panel_data = {
	.on		= protou_lcd_on,
	.off		= protou_lcd_off,
	.set_backlight  = protou_set_backlight,
	.display_on  = protou_display_on,
	.display_off  = protou_display_off,
	.bklswitch      = protou_bkl_switch,
};

static int ch_used[3];

int mipi_protou_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_protou", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	protou_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &protou_panel_data,
		sizeof(protou_panel_data));
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

static struct mipi_dsi_phy_ctrl mipi_dsi_protou_phy_ctrl = {
	
	
	{0x03, 0x01, 0x01, 0x00},
	
	{0xa4, 0x89, 0x14, 0x00, 0x16, 0x8e, 0x18, 0x8b,
	0x16, 0x03, 0x04},
	
	{0x7f, 0x00, 0x00, 0x00},
	
	{0xff, 0x02, 0x06, 0x00},
	
	{0x00, 0x66, 0x31, 0xd2, 0x00, 0x40, 0x37, 0x62,
	0x01, 0x0f, 0x07,
	0x05, 0x14, 0x03, 0x0, 0x0, 0x0, 0x20, 0x0, 0x02, 0x0},
};

static int mipi_video_lg_wvga_pt_init(void)
{
	int ret;

	PR_DISP_INFO("panel: mipi_video_lg_wvga\n");

	pinfo.xres = 480;
	pinfo.yres = 800;
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
	pinfo.camera_backlight = 238;

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
	pinfo.mipi.dsi_phy_db = &mipi_dsi_protou_phy_ctrl;
	
	pinfo.mipi.tx_eot_append = 0x01;

	ret = mipi_protou_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_WVGA_PT);
	if (ret)
		pr_err("%s: failed to register device!\n", __func__);

	strcat(ptype, "PANEL_ID_PROTOU_LG");
	PR_DISP_INFO("%s: assign initial setting for LG, %s\n", __func__, ptype);

	power_on_cmd = lg_video_on_cmds;
	power_on_cmd_size = ARRAY_SIZE(lg_video_on_cmds);
	display_on_cmds = lg_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(lg_display_on_cmds);
	display_off_cmds = lg_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(lg_display_off_cmds);

	return ret;
}

static int mipi_video_orise_wvga_pt_init(void)
{
	int ret;

	PR_DISP_INFO("panel: mipi_video_orise_wvga\n");

	pinfo.xres = 480;
	pinfo.yres = 800;
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
	pinfo.camera_backlight = 238;

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
	pinfo.mipi.frame_rate = 59;
	pinfo.mipi.dsi_phy_db = &mipi_dsi_protou_phy_ctrl;
	
	pinfo.mipi.tx_eot_append = 0x01;


	ret = mipi_protou_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_WVGA_PT);
	if (ret)
		pr_err("%s: failed to register device!\n", __func__);

	if (panel_type == PANEL_ID_PROTOU_SHARP) {
		strcat(ptype, "PANEL_ID_PROTOU_SHARP");
		PR_DISP_INFO("%s: assign initial setting for SHARP, %s\n", __func__, ptype);
		power_on_cmd = sharp_orise_video_on_cmds;
		power_on_cmd_size = ARRAY_SIZE(sharp_orise_video_on_cmds);
		display_on_cmds = lg_display_on_cmds;
		display_on_cmds_count = ARRAY_SIZE(lg_display_on_cmds);
		display_off_cmds = orise_display_off_cmds;
		display_off_cmds_count = ARRAY_SIZE(orise_display_off_cmds);
	} else if (panel_type == PANEL_ID_PROTOU_SHARP_C1) {
		strcat(ptype, "PANEL_ID_PROTOU_SHARP_C1");
		PR_DISP_INFO("%s: assign initial setting for SHARP_C1, %s\n", __func__, ptype);
		power_on_cmd = sharp_C1_orise_video_on_cmds;
		power_on_cmd_size = ARRAY_SIZE(sharp_C1_orise_video_on_cmds);
		display_on_cmds = lg_display_on_cmds;
		display_on_cmds_count = ARRAY_SIZE(lg_display_on_cmds);
		display_off_cmds = orise_display_off_cmds;
		display_off_cmds_count = ARRAY_SIZE(orise_display_off_cmds);
	}

	return ret;
}

#define MSM_FB_BASE             0x2FB00000
#define MSM_FB_SIZE             0x00500000

static struct msm_fb_platform_data msm_fb_pdata = {
	.width = 52,
	.height = 86,
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
	.mdp_color_enhance = protou_mdp_color_enhance,
	.mdp_gamma = protou_mdp_gamma,
};

int __init protou_init_panel(void)
{
	platform_device_register(&msm_fb_device);

	if (panel_type != PANEL_ID_NONE) {
		msm_fb_register_device("mdp", &mdp_pdata);
		msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
	}

	return 0;
}

static int __init protou_panel_panel_init(void)
{
	mipi_dsi_buf_alloc(&protou_panel_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&protou_panel_rx_buf, DSI_BUF_SIZE);

	PR_DISP_INFO("%s: enter 0x%x\n", __func__, panel_type);

	if (panel_type == PANEL_ID_PROTOU_LG) {
		mipi_video_lg_wvga_pt_init();
		PR_DISP_INFO("match PANEL_ID_PROTOU_LG panel_type\n");
	} else if (panel_type == PANEL_ID_PROTOU_SHARP || panel_type == PANEL_ID_PROTOU_SHARP_C1) {
		mipi_power_save_on = 0;
		mipi_video_orise_wvga_pt_init();
		PR_DISP_INFO("match PANEL_ID_PROTOU_SHARP panel_type\n");
	} else
		PR_DISP_INFO("Mis-match panel_type\n");

	return platform_driver_register(&this_driver);
}

device_initcall_sync(protou_panel_panel_init);
