/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __ARCH_ARM_MACH_MSM_BOARD_7627A__
#define __ARCH_ARM_MACH_MSM_BOARD_7627A__

#include "pm.h"
#include <linux/i2c/cpld.h>

void __init cp3dug_init_mmc(void);

void __init msm_msm7627a_allocate_memory_regions(void);
void __init msm_fb_add_devices(void);

#define MSM_RAM_CONSOLE_BASE    0x03A00000 
#define MSM_RAM_CONSOLE_SIZE    MSM_HTC_RAM_CONSOLE_SIZE

#define MSM_FB_BASE             0x3FA00000
#define MSM_FB_SIZE             0x00600000
#define MSM_PMEM_MDP_SIZE       0x2F00000
#define MSM_PMEM_ADSP_SIZE      0x5000000
#define MSM_PMEM_ADSP2_SIZE     0x2C0000
#define PMEM_KERNEL_EBI1_SIZE	0x3A000
#define MSM_PMEM_AUDIO_SIZE		0x1F4000

#define CPLD_GPIO_BASE						(NR_MSM_GPIOS+0x100)
#define CPLD_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio + CPLD_GPIO_BASE)
#define CPLD_GPIO_SYS_TO_PM(sys_gpio)	(sys_gpio - CPLD_GPIO_BASE)

enum {
	GPIO_EXPANDER_IRQ_BASE  = NR_MSM_IRQS + NR_GPIO_IRQS,
	GPIO_EXPANDER_GPIO_BASE = NR_MSM_GPIOS,
	
	GPIO_CORE_EXPANDER_BASE = GPIO_EXPANDER_GPIO_BASE,
	GPIO_BT_SYS_REST_EN     = GPIO_CORE_EXPANDER_BASE,
	GPIO_WLAN_EXT_POR_N,
	GPIO_DISPLAY_PWR_EN,
	GPIO_BACKLIGHT_EN,
	GPIO_PRESSURE_XCLR,
	GPIO_VREG_S3_EXP,
	GPIO_UBM2M_PWRDWN,
	GPIO_ETM_MODE_CS_N,
	GPIO_HOST_VBUS_EN,
	GPIO_SPI_MOSI,
	GPIO_SPI_MISO,
	GPIO_SPI_CLK,
	GPIO_SPI_CS0_N,
	GPIO_CORE_EXPANDER_IO13,
	GPIO_CORE_EXPANDER_IO14,
	GPIO_CORE_EXPANDER_IO15,
	
	GPIO_CAM_EXPANDER_BASE  = GPIO_CORE_EXPANDER_BASE + 16,
	GPIO_CAM_GP_STROBE_READY	= GPIO_CAM_EXPANDER_BASE,
	GPIO_CAM_GP_AFBUSY,
	GPIO_CAM_GP_CAM_PWDN,
	GPIO_CAM_GP_CAM1MP_XCLR,
	GPIO_CAM_GP_CAMIF_RESET_N,
	GPIO_CAM_GP_STROBE_CE,
	GPIO_CAM_GP_LED_EN1,
	GPIO_CAM_GP_LED_EN2,
};

enum {
	QRD_GPIO_HOST_VBUS_EN       = 107,
	QRD_GPIO_BT_SYS_REST_EN     = 114,
	QRD_GPIO_WAKE_ON_WIRELESS,
	QRD_GPIO_BACKLIGHT_EN,
	QRD_GPIO_NC,
	QRD_GPIO_CAM_3MP_PWDN,      
	QRD_GPIO_WLAN_EN,
	QRD_GPIO_CAM_5MP_SHDN_EN,
	QRD_GPIO_CAM_5MP_RESET,
	QRD_GPIO_TP,
	QRD_GPIO_CAM_GP_CAMIF_RESET,
};

#define ADSP_RPC_PROG           0x3000000a

#define FPGA_MSM_CNTRL_REG2 0x90008010
#define BAHAMA_SLAVE_ID_FM_REG 0x02
#define BAHAMA_SLAVE_ID_FM_ADDR  0x2A
#define BAHAMA_SLAVE_ID_QMEMBIST_ADDR   0x7B
#define FM_GPIO 42
#define BT_PCM_BCLK_MODE  0x88
#define BT_PCM_DIN_MODE   0x89
#define BT_PCM_DOUT_MODE  0x8A
#define BT_PCM_SYNC_MODE  0x8B
#define FM_I2S_SD_MODE    0x8E
#define FM_I2S_WS_MODE    0x8F
#define FM_I2S_SCK_MODE   0x90
#define I2C_PIN_CTL       0x15
#define I2C_NORMAL	  0x40


#define CP3DUG_GPIO_PS_HOLD         		(25)
#define CP3DUG_POWER_KEY                	(37)

#define CP3DUG_GPIO_GSENSORS_INT         (83)
#define CP3DUG_LAYOUTS			{ \
		{ {  0, -1, 0}, { -1,  0, 0}, {0, 0, -1} }, \
		{ {  0, -1, 0}, { -1,  0, 0}, {0, 0,  1} }, \
		{ { -1,  0, 0}, {  0,  1, 0}, {0, 0, -1} }, \
		{ {  0, -1, 0}, {  0,  0, 1}, {1, 0,  0} }  \
					}

#define CP3DUG_LAYOUTS_EVT                  { \
                { {  0, -1, 0}, { -1,  0, 0}, {0, 0, -1} }, \
                { {  0, -1, 0}, { -1,  0, 0}, {0, 0,  1} }, \
                { { -1,  0, 0}, {  0,  1, 0}, {0, 0, -1} }, \
                { {  0,  1, 0}, {  0,  0,-1}, {1, 0,  0} }  \
                                        }


#define CP3DUG_GPIO_LCM_ID0        (34)
#define CP3DUG_AUD_UART_OEz			(30)
#define CP3DUG_AUD_REMO_PRESz		(40)
#define CP3DUG_AUD_UART_RX		(122)
#define CP3DUG_AUD_UART_TX		(123)
#define CP3DUG_AUD_UART_SEL			(69)



#define CP3DUG_GPIO_CAM_I2C_SDA    (61)
#define CP3DUG_GPIO_CAM_I2C_SCL    (60)
#define CP3DUG_GPIO_CAM_MCLK       (15)
#define CP3DUG_GPIO_CAM_ID         (58)

#define	CP3DUG_GPIO_RAW_INTR0      (23)
#define	CP3DUG_GPIO_RAW_INTR1      (26)

#define CPLDGPIO(x) (x)
#define CP3_VOL_UP						CPLD_EXT_GPIO_KEY_UP_INPUT_LEVEL
#define CP3_VOL_DN						CPLD_EXT_GPIO_KEY_DW_INPUT_LEVEL

#define	CP3DUG_GPIO_CPLD_TMS       (13)
#define	CP3DUG_GPIO_CPLD_TCK       (11)
#define	CP3DUG_GPIO_CPLD_RST       (49)
#define	CP3DUG_GPIO_CPLD_CLK       (4)
#define	CP3DUG_GPIO_CPLD_INT       (39)
#define	CP3DUG_GPIO_CPLD_I2CEN     (116)

#define	CP3DUG_GPIO_ADDR_0         (130)
#define	CP3DUG_GPIO_ADDR_1         (128)
#define	CP3DUG_GPIO_ADDR_2         (115)
#define	CP3DUG_GPIO_DATA_0         (120)
#define	CP3DUG_GPIO_DATA_1         (117)
#define	CP3DUG_GPIO_DATA_2         (111)
#define	CP3DUG_GPIO_DATA_3         (121)
#define	CP3DUG_GPIO_DATA_4         (119)
#define	CP3DUG_GPIO_DATA_5         (129)
#define	CP3DUG_GPIO_DATA_6         (124)
#define	CP3DUG_GPIO_DATA_7         (125)
#define	CP3DUG_GPIO_OE             (126)
#define	CP3DUG_GPIO_WE             (127)

#define CP3DUG_GPIO_MBAT_IN	(57)

#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_3K
#define MSM_TP_ATTz					(18)
#define MSM_V_TP_3V3_EN				(0)
#define MSM_TP_RSTz					(0)
#endif


struct bahama_config_register {
	u8 reg;
	u8 value;
	u8 mask;
};

struct bt_vreg_info {
	const char *name;
	unsigned int pmapp_id;
	unsigned int min_level;
	unsigned int max_level;
	unsigned int is_pin_controlled;
	struct regulator *reg;
};

#if defined(CONFIG_BT) && defined(CONFIG_MARIMBA_CORE)
void __init msm7627a_bt_power_init(void);
#endif
int __init cp3_wifi_init(void);

extern struct platform_device msm_device_adspdec;

void __init cp3dug_camera_init(void);

void __init msm7627a_add_io_devices(void);
void __init qrd7627a_add_io_devices(void);
int __init cp3dug_init_panel(void);
extern int __init cp3dcg_init_panel(void);
int msm8625Q_init_keypad(void);

extern int panel_type;
#endif
