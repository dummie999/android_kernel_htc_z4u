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
void __init magnids_init_mmc(void);

void __init msm_msm7627a_allocate_memory_regions(void);
void __init msm_fb_add_devices(void);

#define MSM_RAM_CONSOLE_BASE    0x03100000 /* MSM_HTC_RAM_CONSOLE_PHYS must be the same */
#define MSM_RAM_CONSOLE_SIZE    MSM_HTC_RAM_CONSOLE_SIZE

#define MSM_FB_BASE             0x2FB00000
#define MSM_FB_SIZE             0x00500000
#define MSM_PMEM_MDP_SIZE       0x2300000
#define MSM_PMEM_ADSP_SIZE      0x4C00000 /* 76 */
#define MSM_PMEM_ADSP2_SIZE     0x2C0000
#define PMEM_KERNEL_EBI1_SIZE	0x3A000
#define MSM_PMEM_AUDIO_SIZE		0x1F4000

#define XA	0

enum {
	GPIO_EXPANDER_IRQ_BASE  = NR_MSM_IRQS + NR_GPIO_IRQS,
	GPIO_EXPANDER_GPIO_BASE = NR_MSM_GPIOS,
	/* SURF expander */
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
	/* Camera expander */
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
	QRD_GPIO_CAM_3MP_PWDN,      /* CAM_VGA */
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
#define FM_GPIO 83
#define BT_PCM_BCLK_MODE  0x88
#define BT_PCM_DIN_MODE   0x89
#define BT_PCM_DOUT_MODE  0x8A
#define BT_PCM_SYNC_MODE  0x8B
#define FM_I2S_SD_MODE    0x8E
#define FM_I2S_WS_MODE    0x8F
#define FM_I2S_SCK_MODE   0x90
#define I2C_PIN_CTL       0x15
#define I2C_NORMAL	  0x40

#define MAGNIDS_GPIO_PS_HOLD         (25)

/* Compass  */
#define MAGNIDS_GPIO_GSENSORS_INT         (81)
#define MAGNIDS_LAYOUTS			{ \
		{ {  0, -1, 0}, { -1,  0,  0}, {0, 0, -1} }, \
		{ {  0, -1, 0}, {  1,  0,  0}, {0, 0, -1} }, \
		{ { -1,  0, 0}, {  0,  1,  0}, {0, 0, -1} }, \
		{ { -1,  0, 0}, {  0,  0, -1}, {0, 1,  0} }  \
					}
/* NFC */
#define MAGNIDS_GPIO_NFC_IRQ (94)
#define MAGNIDS_GPIO_NFC_VEN (23)
#define MAGNIDS_GPIO_NFC_DL_MODE (96)

#define MAGNIDS_GPIO_USB_ID			(42)
#define UART1DM_RX_GPIO				(45)

/* HEADSET DRIVER BEGIN */
#define MAGNIDS_AUD_UART_OEz			(83)
#define MAGNIDS_AUD_HP_INz			(27)
#define MAGNIDS_AUD_REMO_PRESz		(41)
#define MAGNIDS_AUD_UART_SEL			(119)
#define MAGNIDS_IOEXT_AUD_UART_SEL		(7)
#define MAGNIDS_AUD_UART_RX			(122)
#define MAGNIDS_AUD_UART_TX			(123)

/* Audio External Micbias */
#define MAGNIDS_IOEXP_AUDIO_2V85_EN       (11)

/*flashlight */
#define MAGNIDS_GPIO_FLASH_ENABLE_XA   (98)
#define MAGNIDS_GPIO_FLASH_ENABLE_XB   (78)
#define MAGNIDS_GPIO_FLASH_SWITCH    (16)

/* Camera */
#define MAGNIDS_GPIO_CAM_I2C_SDA      (61)
#define MAGNIDS_GPIO_CAM_I2C_SCL      (60)
#define MAGNIDS_GPIO_CAM_MCLK         (15)
#define	MAGNIDS_GPIO_CAM_1V8_EN       (81)
/* Rawchip */
#define	MAGNIDS_GPIO_RAW_RST          (111)
#define	MAGNIDS_GPIO_RAW_INTR0        (85)
#define	MAGNIDS_GPIO_RAW_INTR1        (49)
#define	MAGNIDS_GPIO_RAW_1V2_EN       (9)
#define	MAGNIDS_GPIO_RAW_1V8_EN       MAGNIDS_GPIO_CAM_1V8_EN
/* CPLD */
#define	MAGNIDS_GPIO_CPLD_1V8_EN      MAGNIDS_GPIO_CAM_1V8_EN
#define	MAGNIDS_GPIO_CPLD_RST         (98)
#define	MAGNIDS_GPIO_CPLD_SPI_CS      (121)
#define	MAGNIDS_GPIO_CPLD_CLK         (109)

/* Camera extend IO on TCA6418E */
#define MAGNIDS_EXT_GPIO_CAM_PD       (12)
#define MAGNIDS_EXT_GPIO_VCM_PD       (13)
#define MAGNIDS_EXT_GPIO_CAM_D1V2_EN  (14)
#define MAGNIDS_EXT_GPIO_CAMIO_1V8_EN (15)

/* Camera vreg ldo on PMIC */
#define MAGNIDS_VREG_CAM_A2V85_LDO    "ldo17"
#define MAGNIDS_VREG_VCM_2V85_LDO     "ldo19"
/* USB */
#define MAGNIDS_GPIO_USB_ID_PIN		(94)
/* DISP */
#define	MAGNIDS_LCD_R7		(5)
#define	MAGNIDS_LCD_R6		(6)
#define	MAGNIDS_LCD_R5		(7)
#define	MAGNIDS_LCD_R4		(8)
#define	MAGNIDS_LCD_R3		(9)
#define	MAGNIDS_LCD_R2		(10)
#define	MAGNIDS_LCD_R1		(11)
#define	MAGNIDS_LCD_R0		(12)
#define	MAGNIDS_LCD_G6		(13)
#define	MAGNIDS_LCD_G7		(14)
#define	MAGNIDS_LCD_G5		(130)
#define	MAGNIDS_LCD_G4		(119)
#define	MAGNIDS_LCD_G3		(120)
#define	MAGNIDS_LCD_G2		(121)
#define	MAGNIDS_LCD_G1		(111)
#define	MAGNIDS_LCD_G0		(112)
#define	MAGNIDS_LCD_B7		(113)
#define	MAGNIDS_LCD_B6		(114)
#define	MAGNIDS_LCD_B5		(115)
#define	MAGNIDS_LCD_B4		(116)
#define	MAGNIDS_LCD_B3		(117)
#define	MAGNIDS_LCD_B2		(118)
#define	MAGNIDS_LCD_B1		(125)
#define	MAGNIDS_LCD_B0_ID1	(126)

#define	MAGNIDS_LCD_PCLK		(4)
#define	MAGNIDS_LCD_VSYNC	(127)
#define	MAGNIDS_LCD_HSYNC	(128)
#define	MAGNIDS_LCD_DE		(129)

#define	MAGNIDS_LCM_3V_EN	(23)
#define	MAGNIDS_LCD_RST_ID0	(97)

#define	MAGNIDS_LCD_SPI_CS1	(31)
#define	MAGNIDS_LCD_SPI_DIN	(32)
#define	MAGNIDS_LCD_SPI_DOUT	(33)
#define	MAGNIDS_LCD_SPI_CLK	(34)


/* Bluetooth */
#define MAGNIDS_GPIO_BT_UART1_RTS        (43)
#define MAGNIDS_GPIO_BT_UART1_CTS        (44)
#define MAGNIDS_GPIO_BT_UART1_RX         (45)
#define MAGNIDS_GPIO_BT_UART1_TX         (46)
#define MAGNIDS_GPIO_BT_HOST_WAKE        (42)
#define MAGNIDS_GPIO_BT_WAKE             (107)
#define MAGNIDS_IOEXP_BT_RESET_N         (4)
#define MAGNIDS_IOEXP_BT_SD_N            (9)


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

void __init magnids_camera_init(void);

void __init msm7627a_add_io_devices(void);
void __init qrd7627a_add_io_devices(void);
int __init magnids_init_panel(void);
void __init magnids_audio_init(void);

extern int panel_type;
#endif
