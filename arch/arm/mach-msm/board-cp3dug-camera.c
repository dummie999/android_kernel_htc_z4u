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
#include "board-cp3dug.h"
#include <mach/vreg.h>
#include <media/msm_camera.h>

#ifdef CONFIG_MSM_CAMERA_FLASH
#include <linux/htc_flashlight.h>
#endif

#ifdef CONFIG_I2C_CPLD
#include <linux/i2c/cpld.h>
#endif
#ifdef CONFIG_SPI_CPLD
#include <linux/spi/cpld.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#endif


static int config_gpio_table(uint32_t *table, int len);

#ifdef CONFIG_MSM_CAMERA_V4L2

#ifdef CONFIG_SPI_CPLD

static uint32_t cpld_on_gpio_table[] = {
#if 1 
	GPIO_CFG(CP3DUG_GPIO_ADDR_0, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_ADDR_1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_ADDR_2, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_DATA_0, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_DATA_1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_DATA_2, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_DATA_3, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_DATA_4, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_DATA_5, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_DATA_6, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_DATA_7, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_OE, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_WE, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
#endif
};

static uint32_t cpld_off_gpio_table[] = {
#if 1 
	GPIO_CFG(CP3DUG_GPIO_ADDR_0, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_ADDR_1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_ADDR_2, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_DATA_0, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_DATA_1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_DATA_2, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_DATA_3, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_DATA_4, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_DATA_5, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_DATA_6, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_DATA_7, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_OE, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_WE, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
#endif
};

static void cpld_power(int on)
{
	pr_info("[CAM]%s: %d\n", __func__, on);

	if (on) {
		config_gpio_table(cpld_on_gpio_table, ARRAY_SIZE(cpld_on_gpio_table));

	} else {
		config_gpio_table(cpld_off_gpio_table, ARRAY_SIZE(cpld_off_gpio_table));

	#if 1 
		gpio_set_value(CP3DUG_GPIO_ADDR_0, 0);
		gpio_set_value(CP3DUG_GPIO_ADDR_1, 0);
		gpio_set_value(CP3DUG_GPIO_ADDR_2, 0);
		gpio_set_value(CP3DUG_GPIO_DATA_0, 0);
		gpio_set_value(CP3DUG_GPIO_DATA_1, 0);
		gpio_set_value(CP3DUG_GPIO_DATA_2, 0);
		gpio_set_value(CP3DUG_GPIO_DATA_3, 0);
		gpio_set_value(CP3DUG_GPIO_DATA_4, 0);
		gpio_set_value(CP3DUG_GPIO_DATA_5, 0);
		gpio_set_value(CP3DUG_GPIO_DATA_6, 0);
		gpio_set_value(CP3DUG_GPIO_DATA_7, 0);
		gpio_set_value(CP3DUG_GPIO_OE, 0);
		gpio_set_value(CP3DUG_GPIO_WE, 0);
	#endif
	}
}

static struct cpld_platform_data cpld_pdata = {
	.power_func		= cpld_power,
};

static struct platform_device cpld_platform_device = {
	.name   		= "cpld",
	.dev.platform_data	= &cpld_pdata,
};

static struct spi_board_info spi_devices[] = {
	{
		.modalias	= "gpio-cpld",
		.max_speed_hz	= 100000,
		.bus_num	= 0,
		.chip_select	= 0,
		.mode		= SPI_MODE_0,
		.controller_data = (void *)SPI_GPIO_NO_CHIPSELECT,
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

int cp3dug_rawchip_vreg_on(void)
{
	int rc;
	pr_info("[CAM]%s: rawchip vreg on\n", __func__);

	
	pr_info("[CAM]%s: CPLD_EXT_GPIO_CAM_VCM2V85_EN\n", __func__);
	rc = cpld_gpio_write(CPLD_EXT_GPIO_CAM_VCM2V85_EN, 1);
	if(rc < 0){
		pr_err("[CAM]%s: VCM 2V85 fail\n", __func__);
		goto err;
	}
	udelay(100);

	
	pr_info("[CAM]%s: CPLD_EXT_GPIO_RAW_1V8_EN\n", __func__);
	rc = cpld_gpio_write(CPLD_EXT_GPIO_RAW_1V8_EN, 1);
	if(rc < 0){
		pr_err("[CAM]%s: V_RAW_1V8 fail\n", __func__);
		goto err;
	}
	udelay(50);

	
	pr_info("[CAM]%s: CPLD_EXT_GPIO_RAW_1V2_EN\n", __func__);
	rc = cpld_gpio_write(CPLD_EXT_GPIO_RAW_1V2_EN, 1);
	if(rc < 0){
		pr_err("[CAM]%s: V_RAW_1V2 fail\n", __func__);
		goto err;
	}
	udelay(100);

	config_rawchip_on_gpios();

err:
	return rc;
}

static int cp3dug_rawchip_vreg_off(void)
{
	int rc;
	pr_info("[CAM]%s: rawchip vreg off\n", __func__);

	
	pr_info("[CAM]%s: CPLD_EXT_GPIO_RAW_1V2_EN\n", __func__);
	rc = cpld_gpio_write(CPLD_EXT_GPIO_RAW_1V2_EN, 0);
	if(rc < 0){
		pr_err("[CAM]%s: V_RAW_1V2 fail\n", __func__);
		goto err;
	}
	udelay(50);

	
	pr_info("[CAM]%s: CPLD_EXT_GPIO_RAW_1V8_EN\n", __func__);
	rc = cpld_gpio_write(CPLD_EXT_GPIO_RAW_1V8_EN, 0);
	if(rc < 0){
		pr_err("[CAM]%s: V_RAW_1V8 fail\n", __func__);
		goto err;
	}
	udelay(100);

	
	pr_info("[CAM]%s: CPLD_EXT_GPIO_CAM_VCM2V85_EN\n", __func__);
	rc = cpld_gpio_write(CPLD_EXT_GPIO_CAM_VCM2V85_EN, 0);
	if(rc < 0){
		pr_err("[CAM]%s: VCM 2V85 fail\n", __func__);
		goto err;
	}
	udelay(50);

	config_rawchip_off_gpios();

err:
	return rc;
}

static uint32_t rawchip_on_gpio_table[] = {
	GPIO_CFG(CP3DUG_GPIO_CAM_MCLK,   1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
	
	
	
};

static uint32_t rawchip_off_gpio_table[] = {
	GPIO_CFG(CP3DUG_GPIO_CAM_MCLK,   0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
	
	
	
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
	.rawchip_reset	= CPLD_EXT_GPIO_RAW_RSTN,
	.rawchip_intr0	= CP3DUG_GPIO_RAW_INTR0,
	.rawchip_intr1	= CP3DUG_GPIO_RAW_INTR1,
	.rawchip_spi_freq = 10, 
	.rawchip_mclk_freq = 24, 
	.camera_rawchip_power_on = cp3dug_rawchip_vreg_on,
	.camera_rawchip_power_off = cp3dug_rawchip_vreg_off,
	
	
	
};

static struct platform_device msm_rawchip_device = {
	.name	= "rawchip",
	.dev	= {
		.platform_data = &msm_rawchip_board_info,
	},
};
#endif



struct msm_camera_device_platform_data cp3dug_msm_camera_csi_device_data[] = {
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

#if defined(CONFIG_S5K3H2YX)
static void cp3dug_camera_vreg_config_s5k3h2yx(int vreg_en);
static int config_camera_on_gpios_rear_s5k3h2yx(void);
static int config_camera_off_gpios_rear_s5k3h2yx(void);

static uint32_t camera_off_gpio_table_s5k3h2yx[] = {
	GPIO_CFG(CP3DUG_GPIO_CAM_I2C_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_CAM_I2C_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
#ifndef CONFIG_RAWCHIP
	GPIO_CFG(CP3DUG_GPIO_CAM_MCLK,    0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
#endif
};

static uint32_t camera_on_gpio_table_s5k3h2yx[] = {
	GPIO_CFG(CP3DUG_GPIO_CAM_I2C_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_CAM_I2C_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
#ifndef CONFIG_RAWCHIP
	GPIO_CFG(CP3DUG_GPIO_CAM_MCLK,    1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),
#endif
};

static struct msm_camera_gpio_conf gpio_conf_s5k3h2yx = {
#if 0
	.camera_off_table = camera_off_gpio_table,
	.camera_off_table_size = ARRAY_SIZE(camera_off_gpio_table),
	.camera_on_table = camera_on_gpio_table,
	.camera_on_table_size = ARRAY_SIZE(camera_on_gpio_table),
	.cam_gpio_req_tbl = s5k3h2yx_cam_req_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(s5k3h2yx_cam_req_gpio),
	.cam_gpio_set_tbl = s5k3h2yx_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(s5k3h2yx_cam_gpio_set_tbl),
#endif
	.gpio_no_mux = 1,
};

#ifdef CONFIG_S5K3H2YX_ACT
static struct i2c_board_info s5k3h2yx_actuator_i2c_info = {
		I2C_BOARD_INFO("s5k3h2yx_act", 0x18 >>1),
};

static struct msm_actuator_info s5k3h2yx_actuator_info = {
	.board_info     = &s5k3h2yx_actuator_i2c_info,
	.bus_id         = MSM_GSBI0_QUP_I2C_BUS_ID,
	.vcm_pwd        = CPLD_EXT_GPIO_CAM1_VCM_PD,
	.vcm_enable     = 1,
};
#endif

#ifdef CONFIG_MSM_CAMERA_FLASH
static struct camera_led_est msm_camera_sensor_s5k3h2yx_led_table[] = {
		{
		.enable = 1, 
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 220,
		.min_step = 59,
		.max_step = 92
	},
		{
		.enable = 1, 
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 320,
		.min_step = 50,
		.max_step = 58
	},
		{
		.enable = 1, 
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 400,
		.lumen_value = 420,
		.min_step = 47,
		.max_step = 49
	},
		{
		.enable = 1, 
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 600,
		.lumen_value = 620,
		.min_step = 41,
		.max_step = 46
	},
		{
		.enable = 1, 
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 750,
		.min_step = 24,
		.max_step = 40    
	},
		{
		.enable = 0, 
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,
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
		.enable = 0, 
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,
		.min_step = 271,
		.max_step = 317    
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL5,
		.current_ma = 500,
		.lumen_value = 500,
		.min_step = 25,
		.max_step = 26
	},
		{
		.enable = 0, 
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 750,
		.min_step = 271,
		.max_step = 325
	},

	{
		.enable = 0,
		.led_state = FL_MODE_TORCH_LEVEL_2,
		.current_ma = 200,
		.lumen_value = 75,
		.min_step = 0,
		.max_step = 40
	},
};

static struct camera_led_info msm_camera_sensor_s5k3h2yx_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 750,  
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_s5k3h2yx_led_table),
};

static struct camera_flash_info msm_camera_sensor_s5k3h2yx_flash_info = {
	.led_info = &msm_camera_sensor_s5k3h2yx_led_info,
	.led_est_table = msm_camera_sensor_s5k3h2yx_led_table,
};

extern int flashlight_control(int mode);

static struct msm_camera_sensor_flash_src msm_flash_src_s5k3h2yx = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
	.camera_flash = flashlight_control,
};

#if 0
static struct camera_flash_cfg msm_camera_sensor_flash_cfg = {
	.num_flash_levels = FLASHLIGHT_NUM,
	.low_temp_limit = 5,
	.low_cap_limit = 15,
};
#endif

static struct camera_flash_cfg msm_camera_sensor_s5k3h2yx_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 14,
	.flash_info             = &msm_camera_sensor_s5k3h2yx_flash_info,
};
#endif

static struct msm_camera_sensor_flash_data flash_s5k3h2yx = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src              = &msm_flash_src_s5k3h2yx,
#endif
};

static struct msm_camera_sensor_platform_info s5k3h2yx_sensor_7627a_info = {
	.mount_angle = 90,
#if 0
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
#endif
	.gpio_conf = &gpio_conf_s5k3h2yx,
	.mirror_flip = CAMERA_SENSOR_MIRROR_FLIP,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3h2yx_data = {
	.sensor_name = "s5k3h2yx",
	.camera_power_on = config_camera_on_gpios_rear_s5k3h2yx,
	.camera_power_off = config_camera_off_gpios_rear_s5k3h2yx,
	.sensor_reset_enable = 0,
	.sensor_reset = 0,
	.sensor_pwd = CPLD_EXT_GPIO_CAM_PWDN,
	.pmic_gpio_enable    = 0,
	.pdata = &cp3dug_msm_camera_csi_device_data[0],
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
	.flash_cfg = &msm_camera_sensor_s5k3h2yx_flash_cfg, 
#endif
};

static void cp3dug_camera_vreg_config_s5k3h2yx(int vreg_en)
{
	int rc;

	pr_info("[CAM]%s: vreg_en %d\n", __func__, vreg_en);

	if (vreg_en) {
		 
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAM_A2V85_EN\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAM_A2V85_EN, 1);
		if(rc < 0){
			pr_err("[CAM]%s: CAM A2V85 fail\n", __func__);
			return;
		}
		udelay(50);
		 
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAMIO_1V8_EN\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAMIO_1V8_EN, 1);
		if(rc < 0){
			pr_err("[CAM]%s: CAMIO 1V8 fail\n", __func__);
			return;
		}
		udelay(50);
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAM_SEL\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAM_SEL, 0); 
		if(rc < 0){
			pr_err("[CAM]%s: MIPI Switch fail\n", __func__);
			return;
		}
		udelay(50);
	}else{
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAMIO_1V8_EN\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAMIO_1V8_EN, 0);
		if(rc < 0){
			pr_err("[CAM]%s: CAMIO 1V8 fail\n", __func__);
			return;
		}
		udelay(50);
		 
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAM_A2V85_EN\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAM_A2V85_EN, 0);
		if(rc < 0){
			pr_err("[CAM]%s: CAM A2V85 fail\n", __func__);
			return;
		}
		udelay(50);
		 
	}
}

static int config_camera_on_gpios_rear_s5k3h2yx(void)
{
	int rc = 0;

	cp3dug_camera_vreg_config_s5k3h2yx(1);

	pr_info("[CAM]%s: config gpio on table\n", __func__);
	rc = config_gpio_table(camera_on_gpio_table_s5k3h2yx,
			ARRAY_SIZE(camera_on_gpio_table_s5k3h2yx));
	if (rc < 0) {
		pr_err("[CAM]%s: CAMSENSOR gpio table request failed\n", __func__);
		return rc;
	}

	return rc;
}

static int config_camera_off_gpios_rear_s5k3h2yx(void)
{
	int rc = 0;

	cp3dug_camera_vreg_config_s5k3h2yx(0);

	rc = config_gpio_table(camera_off_gpio_table_s5k3h2yx,
			ARRAY_SIZE(camera_off_gpio_table_s5k3h2yx));
	if (rc < 0) {
		pr_err("[CAM]%s: CAMSENSOR gpio table request failed\n", __func__);
		return rc;
	}
	return rc;
}
#endif

#ifdef CONFIG_OV5693
static void cp3dug_camera_vreg_config_ov5693(int vreg_en);
static int config_camera_on_gpios_rear_ov5693(void);
static int config_camera_off_gpios_rear_ov5693(void);

static uint32_t camera_off_gpio_table_ov5693[] = {
	GPIO_CFG(CP3DUG_GPIO_CAM_I2C_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_CAM_I2C_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
#ifndef CONFIG_RAWCHIP
	GPIO_CFG(CP3DUG_GPIO_CAM_MCLK,    0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
#endif
};

static uint32_t camera_on_gpio_table_ov5693[] = {
	GPIO_CFG(CP3DUG_GPIO_CAM_I2C_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_CAM_I2C_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
#ifndef CONFIG_RAWCHIP
	GPIO_CFG(CP3DUG_GPIO_CAM_MCLK,    1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),
#endif
};

static struct msm_camera_gpio_conf gpio_conf_ov5693 = {
#if 0
	.camera_off_table = camera_off_gpio_table,
	.camera_off_table_size = ARRAY_SIZE(camera_off_gpio_table),
	.camera_on_table = camera_on_gpio_table,
	.camera_on_table_size = ARRAY_SIZE(camera_on_gpio_table),
	.cam_gpio_req_tbl = ov5693_cam_req_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(ov5693_cam_req_gpio),
	.cam_gpio_set_tbl = ov5693_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(ov5693_cam_gpio_set_tbl),
#endif
	.gpio_no_mux = 1,
};

#ifdef CONFIG_OV5693_ACT
static struct i2c_board_info ov5693_actuator_i2c_info = {
		I2C_BOARD_INFO("ov5693_act", 0x1c >>1),
};

static struct msm_actuator_info ov5693_actuator_info = {
	.board_info     = &ov5693_actuator_i2c_info,
	.bus_id         = MSM_GSBI0_QUP_I2C_BUS_ID,
	.vcm_pwd        = CPLD_EXT_GPIO_CAM1_VCM_PD,
	.vcm_enable     = 1,
};
#endif

#ifdef CONFIG_MSM_CAMERA_FLASH
static struct camera_led_est msm_camera_sensor_ov5693_led_table[] = {
		{
		.enable = 1, 
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 220,
		.min_step = 59,
		.max_step = 92
	},
		{
		.enable = 1, 
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 320,
		.min_step = 50,
		.max_step = 58
	},
		{
		.enable = 1, 
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 400,
		.lumen_value = 420,
		.min_step = 47,
		.max_step = 49
	},
		{
		.enable = 1, 
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 600,
		.lumen_value = 620,
		.min_step = 41,
		.max_step = 46
	},
		{
		.enable = 1, 
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 750,
		.min_step = 24,
		.max_step = 40    
	},
		{
		.enable = 0, 
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,
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
		.enable = 0, 
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,
		.min_step = 271,
		.max_step = 317    
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL5,
		.current_ma = 500,
		.lumen_value = 500,
		.min_step = 25,
		.max_step = 26
	},
		{
		.enable = 0, 
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 750,
		.min_step = 271,
		.max_step = 325
	},

	{
		.enable = 0,
		.led_state = FL_MODE_TORCH_LEVEL_2,
		.current_ma = 200,
		.lumen_value = 75,
		.min_step = 0,
		.max_step = 40
	},
};

static struct camera_led_info msm_camera_sensor_ov5693_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 750,  
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_ov5693_led_table),
};

static struct camera_flash_info msm_camera_sensor_ov5693_flash_info = {
	.led_info = &msm_camera_sensor_ov5693_led_info,
	.led_est_table = msm_camera_sensor_ov5693_led_table,
};

extern int flashlight_control(int mode);

static struct msm_camera_sensor_flash_src msm_flash_src_ov5693 = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
	.camera_flash = flashlight_control,
};

#if 0
static struct camera_flash_cfg msm_camera_sensor_flash_cfg = {
	.num_flash_levels = FLASHLIGHT_NUM,
	.low_temp_limit = 5,
	.low_cap_limit = 15,
};
#endif

static struct camera_flash_cfg msm_camera_sensor_ov5693_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 14,
	.flash_info             = &msm_camera_sensor_ov5693_flash_info,
};
#endif

static struct msm_camera_sensor_flash_data flash_ov5693 = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src              = &msm_flash_src_ov5693,
#endif
};

static struct msm_camera_sensor_platform_info ov5693_sensor_7627a_info = {
	.mount_angle = 90,
#if 0
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
#endif
	.gpio_conf = &gpio_conf_ov5693,
	.mirror_flip = CAMERA_SENSOR_FLIP, 
};

static struct msm_camera_sensor_info msm_camera_sensor_ov5693_data = {
	.sensor_name = "ov5693",
	.camera_power_on = config_camera_on_gpios_rear_ov5693,
	.camera_power_off = config_camera_off_gpios_rear_ov5693,
	.sensor_reset_enable = 0,
	.sensor_reset = 0,
	.sensor_pwd = CPLD_EXT_GPIO_CAM_PWDN,
	.pmic_gpio_enable    = 0,
	.pdata = &cp3dug_msm_camera_csi_device_data[0],
	.flash_data             = &flash_ov5693,
	.sensor_platform_info = &ov5693_sensor_7627a_info,
	.csi_if = 1,
#ifdef CONFIG_RAWCHIP
	.use_rawchip = 1,
#else
	.use_rawchip = 0,
#endif
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
#ifdef CONFIG_OV5693_ACT
	.actuator_info = &ov5693_actuator_info,
#endif
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_cfg = &msm_camera_sensor_ov5693_flash_cfg, 
#endif
};

static void cp3dug_camera_vreg_config_ov5693(int vreg_en)
{
	int rc;

	pr_info("[CAM]%s: vreg_en %d\n", __func__, vreg_en);

	if (vreg_en) {
		 
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAM_A2V85_EN\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAM_A2V85_EN, 1);
		if(rc < 0){
			pr_err("[CAM]%s: CAM A2V85 fail\n", __func__);
			return;
		}
		udelay(50);
		 
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAMIO_1V8_EN\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAMIO_1V8_EN, 1);
		if(rc < 0){
			pr_err("[CAM]%s: CAMIO 1V8 fail\n", __func__);
			return;
		}
		udelay(50);
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAM_SEL\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAM_SEL, 0); 
		if(rc < 0){
			pr_err("[CAM]%s: MIPI Switch fail\n", __func__);
			return;
		}
		udelay(50);
	}else{
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAMIO_1V8_EN\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAMIO_1V8_EN, 0);
		if(rc < 0){
			pr_err("[CAM]%s: CAMIO 1V8 fail\n", __func__);
			return;
		}
		udelay(50);
		 
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAM_A2V85_EN\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAM_A2V85_EN, 0);
		if(rc < 0){
			pr_err("[CAM]%s: CAM A2V85 fail\n", __func__);
			return;
		}
		udelay(50);
		 
	}
}

static int config_camera_on_gpios_rear_ov5693(void)
{
	int rc = 0;

	cp3dug_camera_vreg_config_ov5693(1);

	pr_info("[CAM]%s: config gpio on table\n", __func__);
	rc = config_gpio_table(camera_on_gpio_table_ov5693,
			ARRAY_SIZE(camera_on_gpio_table_ov5693));
	if (rc < 0) {
		pr_err("[CAM]%s: CAMSENSOR gpio table request failed\n", __func__);
		return rc;
	}

	return rc;
}

static int config_camera_off_gpios_rear_ov5693(void)
{
	int rc = 0;

	cp3dug_camera_vreg_config_ov5693(0);

	rc = config_gpio_table(camera_off_gpio_table_ov5693,
			ARRAY_SIZE(camera_off_gpio_table_ov5693));
	if (rc < 0) {
		pr_err("[CAM]%s: CAMSENSOR gpio table request failed\n", __func__);
		return rc;
	}
	return rc;
}
#endif	

#if defined(CONFIG_S5K6A2YA)
static int config_camera_on_gpios_rear_s5k6a2ya(void);
static int config_camera_off_gpios_rear_s5k6a2ya(void);

static uint32_t camera_off_gpio_table_s5k6a2ya[] = {
	GPIO_CFG(CP3DUG_GPIO_CAM_I2C_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_CAM_I2C_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
#ifndef CONFIG_RAWCHIP
	GPIO_CFG(CP3DUG_GPIO_CAM_MCLK,    0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
#endif
};

static uint32_t camera_on_gpio_table_s5k6a2ya[] = {
	GPIO_CFG(CP3DUG_GPIO_CAM_I2C_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(CP3DUG_GPIO_CAM_I2C_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
#ifndef CONFIG_RAWCHIP
	GPIO_CFG(CP3DUG_GPIO_CAM_MCLK,    1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),
#endif
};

static struct msm_camera_gpio_conf gpio_conf_s5k6a2ya = {
	.gpio_no_mux = 1,
};


static struct msm_camera_sensor_flash_data flash_s5k6a2ya = {
	.flash_type             = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_platform_info s5k6a2ya_sensor_7627a_info = {
	.mount_angle = 270,
#if 0
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
#endif
	.gpio_conf = &gpio_conf_s5k6a2ya,
	.mirror_flip = CAMERA_SENSOR_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k6a2ya_data = {
	.sensor_name = "s5k6a2ya",
	.camera_power_on = config_camera_on_gpios_rear_s5k6a2ya,
	.camera_power_off = config_camera_off_gpios_rear_s5k6a2ya,
	.sensor_reset_enable = 1,
	.sensor_reset = CPLD_EXT_GPIO_FRONT_CAM_RST,
	.pmic_gpio_enable    = 0,
	.pdata = &cp3dug_msm_camera_csi_device_data[0],
	.flash_data             = &flash_s5k6a2ya,
	.sensor_platform_info = &s5k6a2ya_sensor_7627a_info,
	.csi_if = 1,
	.use_rawchip = 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
};

static void cp3dug_camera_vreg_config_s5k6a2ya(int vreg_en)
{
	int rc;
	pr_info("[CAM]%s: vreg_en %d\n", __func__, vreg_en);

	if (vreg_en) {
		 
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAM_A2V85_EN\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAM_A2V85_EN, 1);
		if(rc < 0){
			pr_err("[CAM]%s: CAM A2V85 fail\n", __func__);
			return;
		}
		udelay(50);
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAM2_D1V2_EN\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAM2_D1V2_EN, 1);
		if(rc < 0){
			pr_err("[CAM]%s: CAMIO 1V8 fail\n", __func__);
			return;
		}
		udelay(50);
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAMIO_1V8_EN\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAMIO_1V8_EN, 1);
		if(rc < 0){
			pr_err("[CAM]%s: CAMIO 1V8 fail\n", __func__);
			return;
		}
		udelay(50);
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAM_SEL\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAM_SEL, 1); 
		if(rc < 0){
			pr_err("[CAM]%s: MIPI Switch fail\n", __func__);
			return;
		}
		udelay(50);
	}else{
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAM_SEL\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAM_SEL, 0); 
		if(rc < 0){
			pr_err("[CAM]%s: MIPI Switch fail\n", __func__);
			return;
		}
		udelay(50);
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAMIO_1V8_EN\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAMIO_1V8_EN, 0);
		if(rc < 0){
			pr_err("[CAM]%s: CAMIO 1V8 fail\n", __func__);
			return;
		}
		udelay(50);
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAM2_D1V2_EN\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAM2_D1V2_EN, 0);
		if(rc < 0){
			pr_err("[CAM]%s: CAMIO 1V8 fail\n", __func__);
			return;
		}
		udelay(50);
		
		pr_info("[CAM]%s: CPLD_EXT_GPIO_CAM_A2V85_EN\n", __func__);
		rc = cpld_gpio_write(CPLD_EXT_GPIO_CAM_A2V85_EN, 0);
		if(rc < 0){
			pr_err("[CAM]%s: CAM A2V85 fail\n", __func__);
			return;
		}
		udelay(50);
		 
	}
}

static int config_camera_on_gpios_rear_s5k6a2ya(void)
{
	int rc = 0;

	cp3dug_camera_vreg_config_s5k6a2ya(1);

	pr_info("[CAM]%s: config gpio on table\n", __func__);
	rc = config_gpio_table(camera_on_gpio_table_s5k6a2ya,
			ARRAY_SIZE(camera_on_gpio_table_s5k6a2ya));
	if (rc < 0) {
		pr_err("[CAM]%s: CAMSENSOR gpio table request failed\n", __func__);
		return rc;
	}

	return rc;
}

static int config_camera_off_gpios_rear_s5k6a2ya(void)
{
	int rc = 0;

	cp3dug_camera_vreg_config_s5k6a2ya(0);

	rc = config_gpio_table(camera_off_gpio_table_s5k6a2ya,
			ARRAY_SIZE(camera_off_gpio_table_s5k6a2ya));
	if (rc < 0) {
		pr_err("[CAM]%s: CAMSENSOR gpio table request failed\n", __func__);
		return rc;
	}
	return rc;
}
#endif


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

static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};

static void __init cp3dug_init_cam(void)
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

#ifdef CONFIG_OV5693
	{
		I2C_BOARD_INFO("ov5693", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_ov5693_data,
	},
#endif

#ifdef CONFIG_S5K3H2YX
	{
		I2C_BOARD_INFO("s5k3h2yx", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_s5k3h2yx_data,
	},
#endif
#ifdef CONFIG_S5K6A2YA
	{
		I2C_BOARD_INFO("s5k6a2ya", 0x6C >> 1),
		.platform_data = &msm_camera_sensor_s5k6a2ya_data,
	},
#endif
};

void __init cp3dug_camera_init(void)
{

	int rc = 0;

#ifdef CONFIG_SPI_CPLD
	spi_init();
#endif

	cp3dug_init_cam();

#ifdef CONFIG_I2C

#if 0
	rc = gpio_request(CP3DUG_GPIO_CAM_ID, "MSM_CAM_ID");
	pr_info("[CAM] cam id gpio_request, %d\n", CP3DUG_GPIO_CAM_ID);
	if (rc < 0) {
		pr_err("[CAM] GPIO(%d) request failed\n", CP3DUG_GPIO_CAM_ID);
	} else {
		gpio_tlmm_config(
			GPIO_CFG(CP3DUG_GPIO_CAM_ID, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
		msleep(1);
		
		voltage = gpio_get_value(CP3DUG_GPIO_CAM_ID);
		pr_info("[CAM] CAM ID voltage: %d\n", voltage);
		gpio_tlmm_config(
			GPIO_CFG(CP3DUG_GPIO_CAM_ID, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
		gpio_free(CP3DUG_GPIO_CAM_ID);
	}

	pr_info("[CAM]%s: i2c_register_board_info\n", __func__);
	if (voltage == 1) { 
		pr_info("[CAM] use s5k3h2yx\n");
		i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
				i2c_camera_devices_main_source, ARRAY_SIZE(i2c_camera_devices_main_source));
	} else { 
		pr_info("[CAM] use ov5693\n");
		i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
				i2c_camera_devices_2nd_source, ARRAY_SIZE(i2c_camera_devices_2nd_source));
	}
#else

	
	rc = gpio_request(CP3DUG_GPIO_CAM_ID, "MSM_CAM_ID");
	pr_info("[CAM] cam id gpio_request, %d\n", CP3DUG_GPIO_CAM_ID);
	if (rc < 0) {
		pr_err("[CAM] GPIO(%d) request failed\n", CP3DUG_GPIO_CAM_ID);
	} else {
		gpio_tlmm_config(
			GPIO_CFG(CP3DUG_GPIO_CAM_ID, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
	}
	

	pr_info("[CAM]%s: i2c_register_board_info\n", __func__);
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
			i2c_camera_devices,
			ARRAY_SIZE(i2c_camera_devices));
#endif
#endif
}
#endif

