/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <asm/mach-types.h>
#include <mach/msm_iomap.h>
#include <mach/board.h>
#include <mach/irqs-7xxx.h>
#include "devices-msm7x2xa.h"
#include "board-magnids.h"
#include <mach/vreg.h>
#include <media/msm_camera.h>
#include <mach/TCA6418_ioextender.h>

#ifdef CONFIG_MSM_CAMERA_FLASH
#include <linux/htc_flashlight.h>
#endif

#ifdef CONFIG_SPI_CPLD
#include <linux/spi/cpld.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#endif

#ifdef CONFIG_MSM_CAMERA_V4L2
static int config_gpio_table(uint32_t *table, int len);
static void magnids_camera_vreg_config(int vreg_en);
static int config_camera_on_gpios_rear(void);
static int config_camera_off_gpios_rear(void);

#ifdef CONFIG_SPI_CPLD
extern int cpld_clk_set(int);

static struct vreg *vreg_wlan1p2c150; //L19
static void cpld_power(int on)
{
	int rc = 0;

	/* sensor VCM power on with CPLD power */
	pr_info("[CAM]%s: %d\n", __func__, on);
	if (vreg_wlan1p2c150 == NULL) { /* VCM 2V85 */
		vreg_wlan1p2c150 = vreg_get(NULL, "wlan4");
		if (IS_ERR(vreg_wlan1p2c150)) {
			pr_err("[CAM]%s: vreg_get(%s) VCM 2V85 failed (%ld)\n",
				__func__, "wlan1p2c150", PTR_ERR(vreg_wlan1p2c150));
			return;
		}

		rc = vreg_set_level(vreg_wlan1p2c150, 2850);
		if (rc) {
			pr_err("[CAM]%s: VCM 2V85 set_level failed (%d)\n", __func__, rc);
		}
	}
	
    if (on) {
		/* sensor VCM power on with CPLD power */
		rc = vreg_enable(vreg_wlan1p2c150);// VCM 2V85
		printk("[CAM]%s: vreg_enable(vreg_wlan1p2c150) VCM 2V85\n", __func__);

		if (rc) {
			pr_err("[CAM]%s: VCM 2V85 enable failed (%d)\n", __func__, rc);
		}
		mdelay(1);

		pr_info("[CAM]%s: MAGNIDS_GPIO_CPLD_1V8_EN as High\n", __func__);
		gpio_set_value(MAGNIDS_GPIO_CPLD_1V8_EN, 1);
		mdelay(1);
		pr_info("[CAM]%s: MAGNIDS_GPIO_CPLD_RST as High\n", __func__);
		gpio_set_value(MAGNIDS_GPIO_CPLD_RST, 1);
		mdelay(1);
		pr_info("[CAM]%s: MAGNIDS_GPIO_CPLD_SPI_CS as Low\n", __func__);
		gpio_set_value(MAGNIDS_GPIO_CPLD_SPI_CS, 0);
		
	} else {
		pr_info("[CAM]%s: MAGNIDS_GPIO_CPLD_SPI_CS as Low\n", __func__);
		gpio_set_value(MAGNIDS_GPIO_CPLD_SPI_CS, 0);
		pr_info("[CAM]%s: MAGNIDS_GPIO_CPLD_RST as Low\n", __func__);
		gpio_set_value(MAGNIDS_GPIO_CPLD_RST, 0);
		mdelay(1);
		pr_info("[CAM]%s: MAGNIDS_GPIO_CPLD_1V8_EN as Low\n", __func__);
		gpio_set_value(MAGNIDS_GPIO_CPLD_1V8_EN, 0);
		mdelay(1);

		/* sensor VCM power on with CPLD power */
		rc = vreg_disable(vreg_wlan1p2c150);
		printk("[CAM]%s: vreg_disable(vreg_wlan1p2c150) VCM 2V85\n", __func__);
		if (rc) {
			pr_err("[CAM]%s: VCM 2V85 disable failed (%d)\n", __func__, rc);
		}
	}
}

static void cpld_init_clk_pin(int on) //pull high SDMC4_CLK_CPLD CLK 
{
	int rc = 0;
	static uint32_t SDMC4_CLK_CPLD_ON[] = {
		GPIO_CFG(MAGNIDS_GPIO_CPLD_CLK, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	};

	static uint32_t SDMC4_CLK_CPLD_OFF[] = {
		GPIO_CFG(MAGNIDS_GPIO_CPLD_CLK, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	};

	pr_info("[CAM]%s: gpio_tlmm_config: on = %d\n", __func__, on);

	if (on){
		rc = gpio_tlmm_config(SDMC4_CLK_CPLD_ON[0], GPIO_CFG_ENABLE);
		if (rc)
			pr_err("[CAM]%s: gpio_tlmm_config=%d\n", __func__, rc);
	}else{
		rc = gpio_tlmm_config(SDMC4_CLK_CPLD_OFF[0], GPIO_CFG_ENABLE);
		if (rc)
			pr_err("[CAM]%s: gpio_tlmm_config=%d\n", __func__, rc);
	}
}

static struct cpld_platform_data cpld_pdata = {
	.cpld_power_pwd	= MAGNIDS_GPIO_CPLD_1V8_EN,
	.power_func		= cpld_power,
	.init_cpld_clk = cpld_init_clk_pin,
	.clock_set  = cpld_clk_set,
};

static struct resource cpld_resources[] = {
	[0] = {		/* EBI2 / LCD config */
		.start = 0xa0d00000,
		.end   = 0xa0e00000,
		.flags = IORESOURCE_MEM,
	},
	[1] = {		/* EBI2 CS0 memory */
		.start = 0x88000000,
		.end   = 0x8c000000,
		.flags = IORESOURCE_MEM,
	},
	[2] = {		/* TLMM GPIO */
		.start = 0xa9000000,
		.end   = 0xa9400000,
		.flags = IORESOURCE_MEM,
	},
	[3] = {		/* CLOCK */
		.start = 0xa8600000,
		.end   = 0xa8700000,
		.flags = IORESOURCE_MEM,
	},
	[4] = {		/* SDC4 */
		.start = 0xa0700000,
		.end   = 0xa0700008,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device cpld_platform_device = {
	.name   		= "cpld",
	.id     		= -1,
	.num_resources  	= ARRAY_SIZE(cpld_resources),
	.resource       	= cpld_resources,
	.dev.platform_data	= &cpld_pdata,
};

/* SPI devices */
static struct spi_board_info spi_devices[] = {
	{
		.modalias	= "gpio-cpld",
		.max_speed_hz	= 100000,
		.bus_num	= 0,
		.chip_select	= 0,
		.mode		= SPI_MODE_0,
		.controller_data = (void *)MAGNIDS_GPIO_CPLD_SPI_CS,	
	},
};

static void __init spi_init(void)
{
	printk("[CAM]init_spi\n");
	spi_register_board_info(spi_devices, ARRAY_SIZE(spi_devices));
}
#endif


#ifdef CONFIG_RAWCHIP
static int config_rawchip_on_gpios(void);
static void config_rawchip_off_gpios(void);

int magnids_rawchip_vreg_on(void)
{
	int rc;
	pr_info("[CAM]%s: rawchip vreg on\n", __func__);

	/* V_RAW_1V2 */
	rc = gpio_request(MAGNIDS_GPIO_RAW_1V2_EN, "raw");
	if (!rc) {
		gpio_direction_output(MAGNIDS_GPIO_RAW_1V2_EN, 1);
		gpio_free(MAGNIDS_GPIO_RAW_1V2_EN);
	} else{
		pr_err("[CAM]%s: MAGNIDS_GPIO_RAW_1V2_EN enable failed!\n", __func__);
	}

	config_rawchip_on_gpios();
    return rc;
}

static int magnids_rawchip_vreg_off(void)
{
	int rc = 1;
	pr_info("[CAM]%s: rawchip vreg off\n", __func__);

	/* V_RAW_1V2 */
	rc = gpio_request(MAGNIDS_GPIO_RAW_1V2_EN, "raw");
	if (!rc) {
		gpio_direction_output(MAGNIDS_GPIO_RAW_1V2_EN, 0);
		gpio_free(MAGNIDS_GPIO_RAW_1V2_EN);
	} else{
		pr_err("[CAM]MAGNIDS_GPIO_RAW_1V2_EN disable failed!\n");
	}

	config_rawchip_off_gpios();

	return rc;
}

static uint32_t rawchip_on_gpio_table[] = {
	GPIO_CFG(MAGNIDS_GPIO_CAM_MCLK,   1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
	//GPIO_CFG(MAGNIDS_GPIO_RAW_RST,    0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* RAW CHIP Reset */
	//GPIO_CFG(MAGNIDS_GPIO_RAW_1V2_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* RAW CHIP  */
	//GPIO_CFG(MAGNIDS_GPIO_RAW_1V8_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* RAW CHIP  */
};

static uint32_t rawchip_off_gpio_table[] = {
	GPIO_CFG(MAGNIDS_GPIO_CAM_MCLK,   0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),
	//GPIO_CFG(MAGNIDS_GPIO_RAW_RST,    0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* RAW CHIP Reset */
	//GPIO_CFG(MAGNIDS_GPIO_RAW_1V2_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* RAW CHIP  */
	//GPIO_CFG(MAGNIDS_GPIO_CAM_1V8_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* RAW CHIP  */
};

static int config_rawchip_on_gpios(void)
{
	pr_info("[CAM]config_rawchip_on_gpios\n");
	config_gpio_table(rawchip_on_gpio_table,
		ARRAY_SIZE(rawchip_on_gpio_table));
	return 0;
}

static void config_rawchip_off_gpios(void)
{
	pr_info("[CAM]config_rawchip_off_gpios\n");
	config_gpio_table(rawchip_off_gpio_table,
		ARRAY_SIZE(rawchip_off_gpio_table));
}

static struct msm_camera_rawchip_info msm_rawchip_board_info = {
	.rawchip_reset	= MAGNIDS_GPIO_RAW_RST,
	.rawchip_intr0	= MAGNIDS_GPIO_RAW_INTR0,
	.rawchip_intr1	= MAGNIDS_GPIO_RAW_INTR1,
	.rawchip_spi_freq = 25, /* MHz, should be the same to spi max_speed_hz */
	.rawchip_mclk_freq = 24, /* MHz, should be the same as cam csi0 mclk_clk_rate */
	.camera_rawchip_power_on = magnids_rawchip_vreg_on,
	.camera_rawchip_power_off = magnids_rawchip_vreg_off
};

static struct platform_device msm_rawchip_device = {
	.name	= "rawchip",
	.dev	= {
		.platform_data = &msm_rawchip_board_info,
	},
};
#endif

#ifdef CONFIG_MSM_CAMERA_FLASH
extern int flashlight_control(int mode);

static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
	.camera_flash = flashlight_control,
};

/* HTC_BEGIN Andrew_Cheng linear led 20111205 MB */
#if 0
static struct camera_flash_cfg msm_camera_sensor_flash_cfg = {
	.num_flash_levels = FLASHLIGHT_NUM,
	.low_temp_limit = 5,
	.low_cap_limit = 15,
};
#endif
/* HTC_END Andrew_Cheng linear led 20111205 ME */
#endif

struct msm_camera_device_platform_data magnids_msm_camera_csi_device_data[] = {
#if 1
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 266000000,
		.csid_core = 1,
		.is_csic = 1,
	},
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 266000000,
		.csid_core = 0,
		.is_csic = 1,
	},
#else
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 192000000,
		.csid_core = 1,
		.is_csic = 1,
	},
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 192000000,
		.csid_core = 0,
		.is_csic = 1,
	},
#endif
};

static uint32_t camera_off_gpio_table[] = {
	GPIO_CFG(MAGNIDS_GPIO_CAM_I2C_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(MAGNIDS_GPIO_CAM_I2C_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
#ifndef CONFIG_RAWCHIP
	GPIO_CFG(MAGNIDS_GPIO_CAM_MCLK,    0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),
#endif
};

static uint32_t camera_on_gpio_table[] = {
	GPIO_CFG(MAGNIDS_GPIO_CAM_I2C_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(MAGNIDS_GPIO_CAM_I2C_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
#ifndef CONFIG_RAWCHIP
	GPIO_CFG(MAGNIDS_GPIO_CAM_MCLK,    1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),
#endif
};


#ifdef CONFIG_S5K3H2YX
static struct msm_camera_gpio_conf gpio_conf_s5k3h2yx = {
#if 0
	.camera_off_table = camera_off_gpio_table,
	.camera_off_table_size = ARRAY_SIZE(camera_off_gpio_table),
	.camera_on_table = camera_on_gpio_table,
	.camera_on_table_size = ARRAY_SIZE(camera_on_gpio_table),
	.cam_gpio_req_tbl = s5k4e5yx_cam_req_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(s5k4e5yx_cam_req_gpio),
	.cam_gpio_set_tbl = s5k4e5yx_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(s5k4e5yx_cam_gpio_set_tbl),
#endif
	.gpio_no_mux = 1,
};

#ifdef CONFIG_S5K3H2YX_ACT
static struct i2c_board_info s5k3h2yx_actuator_i2c_info = {
		I2C_BOARD_INFO("s5k3h2yx_act", 0x11),
};

static struct msm_actuator_info s5k3h2yx_actuator_info = {
	.board_info     = &s5k3h2yx_actuator_i2c_info,
	.bus_id         = MSM_GSBI0_QUP_I2C_BUS_ID,
	.vcm_pwd        = MAGNIDS_EXT_GPIO_VCM_PD,
	.vcm_enable     = 1,
};
#endif

/* HTC_BEGIN Andrew_Cheng linear led 20111205 MB */
//200 mA FL_MODE_FLASH_LEVEL1
//300 mA FL_MODE_FLASH_LEVEL2
//400 mA FL_MODE_FLASH_LEVEL3
//600 mA FL_MODE_FLASH_LEVEL4
static struct camera_led_est msm_camera_sensor_s5k3h2yx_led_table[] = {
	{
		.enable = 1, //.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL1,
		.current_ma = 200,
		.lumen_value = 200,//245,//240,   //mk0118
		.min_step = 0,//23,  //mk0210
		.max_step = 25
	},
		{
		.enable = 1, //.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 300,
		.lumen_value = 300,
		.min_step = 26,
		.max_step = 28
	},
		{
		.enable = 1, //.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 400,
		.lumen_value = 400,
		.min_step = 29,
		.max_step = 31
	},
		{
		.enable = 1, //.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 600,
		.lumen_value = 600,
		.min_step = 31,
		.max_step = 32
	},
		{
		.enable = 1, //.enable = 1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 750,//725,   //mk0217  //mk0221
		.min_step = 33,
		.max_step = 40    //mk0210
	},
		{
		.enable = 0, //.enable = 2,
		.led_state = FL_MODE_FLASH_LEVEL1,
		.current_ma = 200,
		.lumen_value = 250,//245,
		.min_step = 0,
		.max_step = 270
	},
		{
		.enable = 0,
		.led_state = FL_MODE_OFF,
		.current_ma = 0,
		.lumen_value = 0,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 0,
		.led_state = FL_MODE_TORCH,
		.current_ma = 150,
		.lumen_value = 150,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 0, //.enable = 2,     //mk0210
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,//725,   //mk0217   //mk0221
		.min_step = 271,
		.max_step = 317    //mk0210
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 600,
		.lumen_value = 600,
		.min_step = 25,
		.max_step = 26
	},
		{
		.enable = 0, // 3,  //mk0210
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 750,//740,//725,
		.min_step = 271,
		.max_step = 325
	},

	{
		.enable = 0,
		.led_state = FL_MODE_TORCH_LEVEL_1,
		.current_ma = 200,
		.lumen_value = 75,
		.min_step = 0,
		.max_step = 40
	},
};

static struct camera_led_info msm_camera_sensor_s5k3h2yx_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 750,  //mk0210
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_s5k3h2yx_led_table),
};

static struct camera_flash_info msm_camera_sensor_s5k3h2yx_flash_info = {
	.led_info = &msm_camera_sensor_s5k3h2yx_led_info,
	.led_est_table = msm_camera_sensor_s5k3h2yx_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_s5k3h2yx_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 15,
	.flash_info             = &msm_camera_sensor_s5k3h2yx_flash_info,
};
/* HTC_END Andrew_Cheng linear led 20111205 ME */

static struct msm_camera_sensor_flash_data flash_s5k3h2yx = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src              = &msm_flash_src
#endif
};

static struct msm_camera_sensor_platform_info s5k3h2yx_sensor_7627a_info = {
	.mount_angle = 90,
#if 0
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
#endif
	.gpio_conf = &gpio_conf_s5k3h2yx,
	.mirror_flip = CAMERA_SENSOR_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3h2yx_data = {
	.sensor_name = "s5k3h2yx",
	.camera_power_on = config_camera_on_gpios_rear,
	.camera_power_off = config_camera_off_gpios_rear,
	.sensor_reset_enable = 1,
	.sensor_reset = MAGNIDS_EXT_GPIO_CAM_PD,
	.pmic_gpio_enable    = 0,
	.pdata = &magnids_msm_camera_csi_device_data[0],
	.flash_data             = &flash_s5k3h2yx,
	.sensor_platform_info = &s5k3h2yx_sensor_7627a_info,
	.csi_if = 1,
#ifdef CONFIG_RAWCHIP
	.use_rawchip = 1,
#else
	.use_rawchip = 0,
#endif
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
#ifdef CONFIG_S5K3H2YX_ACT
	.actuator_info = &s5k3h2yx_actuator_info,
#endif
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_cfg = &msm_camera_sensor_s5k3h2yx_flash_cfg, /* HTC Andrew_Cheng linear led 20111205 */
#endif
};
#endif


static void magnids_camera_vreg_config(int vreg_en)
{
	static struct vreg *vreg_vcm_2v85=NULL;
	static struct vreg *vreg_cam_a2v85=NULL;
	int rc;

	pr_info("[CAM]%s: %d\n", __func__, vreg_en);

	if (vreg_vcm_2v85 == NULL) { /* VCM 2V85 */
		vreg_vcm_2v85 = vreg_get(NULL, MAGNIDS_VREG_VCM_2V85_LDO);
		if (IS_ERR(vreg_vcm_2v85)) {
			pr_err("[CAM]%s: vreg_get(%s) VCM 2V85 failed (%ld)\n",
				__func__, "vreg_vcm_2v85", PTR_ERR(vreg_vcm_2v85));
			return;
		}

		rc = vreg_set_level(vreg_vcm_2v85, 2850);
		if (rc) {
			pr_err("[CAM]%s: VCM 2V85 set_level failed (%d)\n", __func__, rc);
		}
	}

	if (vreg_cam_a2v85 == NULL) { /* CAM A2V85 */
		vreg_cam_a2v85 = vreg_get(NULL, MAGNIDS_VREG_CAM_A2V85_LDO);
		if (IS_ERR(vreg_cam_a2v85)) {
			pr_err("[CAM]%s: vreg_get(%s) CAM A2V85 failed (%ld)\n",
				__func__, "vreg_cam_a2v85", PTR_ERR(vreg_cam_a2v85));
			return;
		}
		printk("[CAM]%s: CAM A2V85\n", __func__);
		rc = vreg_set_level(vreg_cam_a2v85, 2850);
		if (rc) {
			pr_err("[CAM]%s: A2.85v  set level failed (%d)\n",
				__func__, rc);
		}
	}

	if (vreg_en) {
		/* VCM 2V85 */
		printk("[CAM]%s: vreg_enable(vreg_vcm_2v85) VCM 2V85\n", __func__);
		rc = vreg_enable(vreg_vcm_2v85);
		if (rc) {
			pr_err("[CAM]%s: VCM 2V85 enable failed (%d)\n", __func__, rc);
		}
		//udelay(50);

		/* CAM A2V85 */
		printk("[CAM]%s: vreg_enable(vreg_cam_a2v85) CAM A2V85\n", __func__);
		rc = vreg_enable(vreg_cam_a2v85);
		if (rc) {
			pr_err("[CAM]%s: CAM A2V85 enable failed (%d)\n", __func__, rc);
		}
		//udelay(50);

		/* CAM_D1V2 */
		printk("[CAM]%s: set D1V2 enable\n", __func__);
		rc = ioext_gpio_set_value(MAGNIDS_EXT_GPIO_CAM_D1V2_EN, 1);
		if (rc) {
			pr_err("[CAM]%s: set IO 1V8 enable failed (%d)\n", __func__, rc);
		}
		//udelay(50);

		/* CAMIO 1V8 */
		printk("[CAM]%s: set IO 1V8 enable\n", __func__);
		rc = ioext_gpio_set_value(MAGNIDS_EXT_GPIO_CAMIO_1V8_EN, 1);
		if (rc) {
			pr_err("[CAM]%s: set IO 1V8 enable failed (%d)\n", __func__, rc);
		}
		udelay(50);

	} else {
		/* CAMIO 1V8 */
		printk("[CAM]%s: set IO 1V8 disable\n", __func__);
		rc = ioext_gpio_set_value(MAGNIDS_EXT_GPIO_CAMIO_1V8_EN, 0);
		if (rc) {
			pr_err("[CAM]%s: set IO 1V8 disable failed (%d)\n", __func__, rc);
		}
		udelay(50);

		/* CAM_D1V2 */
		printk("[CAM]%s: set D1V2 disable\n", __func__);
		rc = ioext_gpio_set_value(MAGNIDS_EXT_GPIO_CAM_D1V2_EN, 0);
		if (rc) {
			pr_err("[CAM]%s: set D1V2 disable failed (%d)\n", __func__, rc);
		}
		udelay(50);

		/* CAM A2V85 */
		printk("[CAM]%s: vreg_disable(vreg_cam_a2v85) CAM A2V85\n", __func__);
		rc = vreg_disable(vreg_cam_a2v85);
		if (rc) {
			pr_err("[CAM]%s: CAM A2V85 disable failed (%d)\n", __func__, rc);
		}
		udelay(50);

		/* VCM 2V85 */
		printk("[CAM]%s: vreg_disable(vreg_vcm_2v85) VCM 2V85\n", __func__);
		rc = vreg_disable(vreg_vcm_2v85);
		if (rc) {
			pr_err("[CAM]%s: VCM 2V85 disable failed (%d)\n", __func__, rc);
		}
		udelay(50);
	}
}

static int config_gpio_table(uint32_t *table, int len)
{
	int rc = 0, i = 0;

	for (i = 0; i < len; i++) {
		rc = gpio_tlmm_config(table[i], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("[CAM]%s not able to get gpio\n", __func__);
			for (i--; i >= 0; i--)
				gpio_tlmm_config(table[i],
					GPIO_CFG_ENABLE);
		break;
		}
	}
	return rc;
}

static int config_camera_on_gpios_rear(void)
{
	int rc = 0;

	magnids_camera_vreg_config(1);

	pr_info("[CAM]%s: config gpio on table\n", __func__);
	rc = config_gpio_table(camera_on_gpio_table,
			ARRAY_SIZE(camera_on_gpio_table));
	if (rc < 0) {
		pr_err("[CAM]%s: CAMSENSOR gpio table request failed\n", __func__);
		return rc;
	}

	return rc;
}

static int config_camera_off_gpios_rear(void)
{
	int rc = 0;

	magnids_camera_vreg_config(0);

	rc = config_gpio_table(camera_off_gpio_table,
			ARRAY_SIZE(camera_off_gpio_table));
	if (rc < 0) {
		pr_err("[CAM]%s: CAMSENSOR gpio table request failed\n", __func__);
		return rc;
	}
	return rc;
}

static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};

static void __init magnids_init_cam(void)
{
    platform_device_register(&msm_camera_server);
	platform_device_register(&msm8625_device_csic0);
	platform_device_register(&msm8625_device_csic1);
	platform_device_register(&msm7x27a_device_clkctl);
	platform_device_register(&msm7x27a_device_vfe);
#ifdef CONFIG_RAWCHIP
	platform_device_register(&msm_rawchip_device);
#endif
#ifdef CONFIG_SPI_CPLD
	platform_device_register(&cpld_platform_device);
#endif
}

static struct i2c_board_info i2c_camera_devices[] = {
#ifdef CONFIG_S5K3H2YX
	{
		I2C_BOARD_INFO("s5k3h2yx", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_s5k3h2yx_data,
	},
#endif
#ifdef CONFIG_S5K4E5YX
	{
		I2C_BOARD_INFO("s5k4e5yx", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_s5k4e5yx_data,
	},
#endif
};

void __init magnids_camera_init(void)
{
#ifdef CONFIG_SPI_CPLD
	spi_init();
#endif

	magnids_init_cam();

#ifdef CONFIG_I2C
	pr_info("[CAM]%s: i2c_register_board_info\n", __func__);
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
			i2c_camera_devices,
			ARRAY_SIZE(i2c_camera_devices));
#endif
}
#endif

