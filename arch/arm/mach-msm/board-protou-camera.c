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
#include "board-protou.h"
#include <mach/vreg.h>
#include <media/msm_camera.h>

#ifdef CONFIG_MSM_CAMERA_FLASH
#include <linux/htc_flashlight.h>
#endif

#ifdef CONFIG_SPI_CPLD
#include <linux/spi/cpld.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#endif


static int config_gpio_table(uint32_t *table, int len);

#ifdef CONFIG_MSM_CAMERA_V4L2

#if defined(CONFIG_S5K4E5YX) || defined(CONFIG_OV5693)
static void protou_camera_vreg_config(int vreg_en);
static int config_camera_on_gpios_rear(void);
static int config_camera_off_gpios_rear(void);
#endif

#ifdef CONFIG_SPI_CPLD
extern int cpld_clk_set(int);

static struct vreg *vreg_wlan1p2c150; 
static void cpld_power(int on)
{
	int rc = 0;

	
	pr_info("[CAM]%s: %d\n", __func__, on);
	if (vreg_wlan1p2c150 == NULL) { 
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
		
		rc = vreg_enable(vreg_wlan1p2c150);
		printk("[CAM]%s: vreg_enable(vreg_wlan1p2c150) VCM 2V85\n", __func__);

		if (rc) {
			pr_err("[CAM]%s: VCM 2V85 enable failed (%d)\n", __func__, rc);
		}
		mdelay(1);

		pr_info("[CAM]%s: PROTOU_GPIO_CPLD_1V8_EN as High\n", __func__);
		gpio_set_value(PROTOU_GPIO_CPLD_1V8_EN, 1);
		mdelay(1);
		pr_info("[CAM]%s: PROTOU_GPIO_CPLD_RST as High\n", __func__);
		gpio_set_value(PROTOU_GPIO_CPLD_RST, 1);
		mdelay(1);
		pr_info("[CAM]%s: PROTOU_GPIO_CPLD_SPI_CS as Low\n", __func__);
		gpio_set_value(PROTOU_GPIO_CPLD_SPI_CS, 0);
		
	} else {
		pr_info("[CAM]%s: PROTOU_GPIO_CPLD_SPI_CS as Low\n", __func__);
		gpio_set_value(PROTOU_GPIO_CPLD_SPI_CS, 0);
		pr_info("[CAM]%s: PROTOU_GPIO_CPLD_RST as Low\n", __func__);
		gpio_set_value(PROTOU_GPIO_CPLD_RST, 0);
		mdelay(1);
		pr_info("[CAM]%s: PROTOU_GPIO_CPLD_1V8_EN as Low\n", __func__);
		gpio_set_value(PROTOU_GPIO_CPLD_1V8_EN, 0);
		mdelay(1);

		
		rc = vreg_disable(vreg_wlan1p2c150);
		printk("[CAM]%s: vreg_disable(vreg_wlan1p2c150) VCM 2V85\n", __func__);
		if (rc) {
			pr_err("[CAM]%s: VCM 2V85 disable failed (%d)\n", __func__, rc);
		}
	}
}

static void cpld_init_clk_pin(int on) 
{
	int rc = 0;
	static uint32_t SDMC4_CLK_CPLD_ON[] = {
		GPIO_CFG(PROTOU_GPIO_CPLD_CLK, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	};

	static uint32_t SDMC4_CLK_CPLD_OFF[] = {
		GPIO_CFG(PROTOU_GPIO_CPLD_CLK, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
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
	.cpld_power_pwd	= PROTOU_GPIO_CPLD_1V8_EN,
	.power_func		= cpld_power,
	.init_cpld_clk = cpld_init_clk_pin,
	.clock_set  = cpld_clk_set,
};

static struct resource cpld_resources[] = {
	[0] = {		
		.start = 0xa0d00000,
		.end   = 0xa0e00000,
		.flags = IORESOURCE_MEM,
	},
	[1] = {		
		.start = 0x88000000,
		.end   = 0x8c000000,
		.flags = IORESOURCE_MEM,
	},
	[2] = {		
		.start = 0xa9000000,
		.end   = 0xa9400000,
		.flags = IORESOURCE_MEM,
	},
	[3] = {		
		.start = 0xa8600000,
		.end   = 0xa8700000,
		.flags = IORESOURCE_MEM,
	},
	[4] = {		
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

static struct spi_board_info spi_devices[] = {
	{
		.modalias	= "gpio-cpld",
		.max_speed_hz	= 100000,
		.bus_num	= 0,
		.chip_select	= 0,
		.mode		= SPI_MODE_0,
		.controller_data = (void *)PROTOU_GPIO_CPLD_SPI_CS,	
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

int protou_rawchip_vreg_on(void)
{
	int rc;
	pr_info("[CAM]%s: rawchip vreg on\n", __func__);

	
	rc = gpio_request(PROTOU_GPIO_RAW_1V2_EN, "raw");
	if (!rc) {
		gpio_direction_output(PROTOU_GPIO_RAW_1V2_EN, 1);
		gpio_free(PROTOU_GPIO_RAW_1V2_EN);
	} else{
		pr_err("[CAM]%s: PROTOU_GPIO_RAW_1V2_EN enable failed!\n", __func__);
	}

	config_rawchip_on_gpios();
    return rc;
}

static int protou_rawchip_vreg_off(void)
{
	int rc = 1;
	pr_info("[CAM]%s: rawchip vreg off\n", __func__);

	
	rc = gpio_request(PROTOU_GPIO_RAW_1V2_EN, "raw");
	if (!rc) {
		gpio_direction_output(PROTOU_GPIO_RAW_1V2_EN, 0);
		gpio_free(PROTOU_GPIO_RAW_1V2_EN);
	} else{
		pr_err("[CAM]PROTOU_GPIO_RAW_1V2_EN disable failed!\n");
	}

	config_rawchip_off_gpios();

	return rc;
}

static uint32_t rawchip_on_gpio_table[] = {
	GPIO_CFG(PROTOU_GPIO_CAM_MCLK,   1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
	
	
	
};

static uint32_t rawchip_off_gpio_table[] = {
	GPIO_CFG(PROTOU_GPIO_CAM_MCLK,   0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA),
	
	
	
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
	.rawchip_reset	= PROTOU_GPIO_RAW_RST,
	.rawchip_intr0	= PROTOU_GPIO_RAW_INTR0,
	.rawchip_intr1	= PROTOU_GPIO_RAW_INTR1,
	.rawchip_spi_freq = 25, 
	.rawchip_mclk_freq = 24, 
	.camera_rawchip_power_on = protou_rawchip_vreg_on,
	.camera_rawchip_power_off = protou_rawchip_vreg_off,
	
	
	
};

static struct platform_device msm_rawchip_device = {
	.name	= "rawchip",
	.dev	= {
		.platform_data = &msm_rawchip_board_info,
	},
};
#endif



struct msm_camera_device_platform_data protou_msm_camera_csi_device_data[] = {
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

#if defined(CONFIG_S5K4E5YX) || defined(CONFIG_OV5693)
static uint32_t camera_off_gpio_table[] = {
	GPIO_CFG(PROTOU_GPIO_CAM_I2C_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(PROTOU_GPIO_CAM_I2C_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	
	
	
};

static uint32_t camera_on_gpio_table[] = {
	GPIO_CFG(PROTOU_GPIO_CAM_I2C_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(PROTOU_GPIO_CAM_I2C_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	
	
	
};
#endif

#ifdef CONFIG_S5K4E5YX
static struct msm_camera_gpio_conf gpio_conf_s5k4e5yx = {
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

#ifdef CONFIG_S5K4E5YX_ACT
static struct i2c_board_info s5k4e5yx_actuator_i2c_info = {
		I2C_BOARD_INFO("s5k4e5yx_act", 0x18 >>1),
};

static struct msm_actuator_info s5k4e5yx_actuator_info = {
	.board_info     = &s5k4e5yx_actuator_i2c_info,
	.bus_id         = MSM_GSBI0_QUP_I2C_BUS_ID,
	.vcm_pwd        = PROTOU_GPIO_CAM_PWDN,
	.vcm_enable     = 1,
};
#endif

#ifdef CONFIG_MSM_CAMERA_FLASH
static struct camera_led_est msm_camera_sensor_s5k4e5yx_led_table[] = {
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

static struct camera_led_info msm_camera_sensor_s5k4e5yx_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 750,  
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_s5k4e5yx_led_table),
};

static struct camera_flash_info msm_camera_sensor_s5k4e5yx_flash_info = {
	.led_info = &msm_camera_sensor_s5k4e5yx_led_info,
	.led_est_table = msm_camera_sensor_s5k4e5yx_led_table,
};

extern int flashlight_control(int mode);

static struct msm_camera_sensor_flash_src msm_flash_src_s5k4e5yx = {
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

static struct camera_flash_cfg msm_camera_sensor_s5k4e5yx_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 15,
	.flash_info             = &msm_camera_sensor_s5k4e5yx_flash_info,
};
#endif

static struct msm_camera_sensor_flash_data flash_s5k4e5yx = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src              = &msm_flash_src_s5k4e5yx,
#endif
};

static struct msm_camera_sensor_platform_info s5k4e5yx_sensor_7627a_info = {
	.mount_angle = 90,
#if 0
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
#endif
	.gpio_conf = &gpio_conf_s5k4e5yx,
	.mirror_flip = CAMERA_SENSOR_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k4e5yx_data = {
	.sensor_name = "s5k4e5yx",
	.camera_power_on = config_camera_on_gpios_rear,
	.camera_power_off = config_camera_off_gpios_rear,
	.sensor_reset_enable = 1,
	.sensor_reset = PROTOU_GPIO_CAM_RST,
	.pmic_gpio_enable    = 0,
	.pdata = &protou_msm_camera_csi_device_data[0],
	.flash_data             = &flash_s5k4e5yx,
	.sensor_platform_info = &s5k4e5yx_sensor_7627a_info,
	.csi_if = 1,
	.use_rawchip = 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
#ifdef CONFIG_S5K4E5YX_ACT
	.actuator_info = &s5k4e5yx_actuator_info,
#endif
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_cfg = &msm_camera_sensor_s5k4e5yx_flash_cfg, 
#endif
};
#endif


#ifdef CONFIG_OV5693
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
		I2C_BOARD_INFO("ov5693_act", 0x0C),
};

static struct msm_actuator_info ov5693_actuator_info = {
	.board_info     = &ov5693_actuator_i2c_info,
	.bus_id         = MSM_GSBI0_QUP_I2C_BUS_ID,
	.vcm_pwd        = PROTOU_GPIO_CAM_PWDN,
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
	.low_cap_limit		= 15,
	.flash_info             = &msm_camera_sensor_ov5693_flash_info,
};
#endif

static struct msm_camera_sensor_flash_data flash_ov5693 = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src              = &msm_flash_src_ov5693,
#endif
};

static struct msm_camera_sensor_platform_info ov5693_sensor_info = {
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
	.camera_power_on = config_camera_on_gpios_rear,
	.camera_power_off = config_camera_off_gpios_rear,
	.sensor_reset_enable = 1,
	.sensor_reset = PROTOU_GPIO_CAM_RST,
	.pmic_gpio_enable    = 0,
	.pdata = &protou_msm_camera_csi_device_data[0],
	.flash_data             = &flash_ov5693,
	.sensor_platform_info = &ov5693_sensor_info,
	.csi_if = 1,
	.use_rawchip = 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
#ifdef CONFIG_OV5693_ACT
	.actuator_info = &ov5693_actuator_info,
#endif
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_cfg = &msm_camera_sensor_ov5693_flash_cfg, 
#endif
};
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

#if defined(CONFIG_S5K4E5YX) || defined(CONFIG_OV5693)
static struct vreg *vreg_bt;
static void protou_camera_vreg_config(int vreg_en)
{

	int rc;

	if (vreg_bt == NULL) { 
		vreg_bt = vreg_get(NULL, "bt");
		if (IS_ERR(vreg_bt)) {
			pr_err("[CAM]%s: vreg_get(%s) failed (%ld)\n",
				__func__, "bt", PTR_ERR(vreg_bt));
			return;
		}
		printk("[CAM]%s: A2.85v\n", __func__);
		rc = vreg_set_level(vreg_bt, 2850);
		if (rc) {
			pr_err("[CAM]%s: A2.85v  set level failed (%d)\n",
				__func__, rc);
		}
	}

	if (vreg_en) {
	#if 0 
		
		printk("[CAM]%s: set D1V8 enable\n", __func__);
		gpio_set_value(PROTOU_GPIO_CAM_1V8_EN, 1);
		udelay(5);
	#endif

		printk("[CAM]%s: vreg_enable(vreg_bt) A2V85\n", __func__);
		rc = vreg_enable(vreg_bt);
		if (rc) {
			pr_err("[CAM]%s: A2V85 enable failed (%d)\n", __func__, rc);
		}
		udelay(100);

		
		printk("[CAM]%s: set IO 1V8 enable\n", __func__);
		gpio_set_value(PROTOU_GPIO_CAMIO_1V8_EN, 1);

	} else {
		
		printk("[CAM]%s: vreg_disable(vreg_bt) A2V85\n", __func__);
		rc = vreg_disable(vreg_bt);
		if (rc) {
			pr_err("[CAM]%s: A2V85 disable failed (%d)\n",
				__func__, rc);
		}
		udelay(100);

		
		printk("[CAM]%s: set IO 1V8 disable\n", __func__);
		gpio_set_value(PROTOU_GPIO_CAMIO_1V8_EN, 0);
		mdelay(1);

	#if 0 
		
		printk("[CAM]%s: set D1V8 disable\n", __func__);
		gpio_set_value(PROTOU_GPIO_CAM_1V8_EN, 0);
		mdelay(1);
	#endif
	}
}

static int config_camera_on_gpios_rear(void)
{
	int rc = 0;

	protou_camera_vreg_config(1);

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

	protou_camera_vreg_config(0);

	rc = config_gpio_table(camera_off_gpio_table,
			ARRAY_SIZE(camera_off_gpio_table));
	if (rc < 0) {
		pr_err("[CAM]%s: CAMSENSOR gpio table request failed\n", __func__);
		return rc;
	}
	return rc;
}
#endif

static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};

static void __init protou_init_cam(void)
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

static struct i2c_board_info i2c_camera_devices_main_source[] = {
#ifdef CONFIG_S5K4E5YX
	{
		I2C_BOARD_INFO("s5k4e5yx", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_s5k4e5yx_data,
	},
#endif
};

static struct i2c_board_info i2c_camera_devices_2nd_source[] = {
#ifdef CONFIG_OV5693
	{
		I2C_BOARD_INFO("ov5693", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_ov5693_data,
	},
#endif
};


void __init protou_camera_init(void)
{
	int rc = 0;
	int voltage = 1; 

#ifdef CONFIG_SPI_CPLD
	spi_init();
#endif

	protou_init_cam();

#ifdef CONFIG_I2C
	
	rc = gpio_request(PROTOU_GPIO_CAM_ID, "MSM_CAM_ID");
	pr_info("[CAM] cam id gpio_request, %d\n", PROTOU_GPIO_CAM_ID);
	if (rc < 0) {
		pr_err("[CAM] GPIO(%d) request failed\n", PROTOU_GPIO_CAM_ID);
	} else {
		gpio_tlmm_config(
			GPIO_CFG(PROTOU_GPIO_CAM_ID, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
		msleep(1);
		
		voltage = gpio_get_value(PROTOU_GPIO_CAM_ID);
		pr_info("[CAM] CAM ID voltage: %d\n", voltage);
		gpio_tlmm_config(
			GPIO_CFG(PROTOU_GPIO_CAM_ID, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
		gpio_free(PROTOU_GPIO_CAM_ID);
	}

	pr_info("[CAM]%s: i2c_register_board_info\n", __func__);
	if (voltage == 1) { 
		pr_info("[CAM] use s5k4e5yx\n");
		i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
				i2c_camera_devices_main_source, ARRAY_SIZE(i2c_camera_devices_main_source));
	} else { 
		pr_info("[CAM] use ov5693\n");
		i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
				i2c_camera_devices_2nd_source, ARRAY_SIZE(i2c_camera_devices_2nd_source));
	}
#endif
}
#endif

