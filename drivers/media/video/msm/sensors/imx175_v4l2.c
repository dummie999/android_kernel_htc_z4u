#include "msm_sensor.h"
#include "msm.h"

#ifdef CONFIG_RAWCHIP
#include "rawchip/rawchip.h"
#endif

#define SENSOR_NAME "imx175"
#define PLATFORM_DRIVER_NAME "msm_camera_imx175"
#define imx175_obj imx175_##obj

#define IMX175_REG_READ_MODE 0x0101
#define IMX175_READ_NORMAL_MODE 0x0000	
#define IMX175_READ_MIRROR 0x0001			
#define IMX175_READ_FLIP 0x0002			
#define IMX175_READ_MIRROR_FLIP 0x0003	


#define REG_DIGITAL_GAIN_GREEN_R 0x020E
#define REG_DIGITAL_GAIN_RED 0x0210
#define REG_DIGITAL_GAIN_BLUE 0x0212
#define REG_DIGITAL_GAIN_GREEN_B 0x0214

#if 0
#define DEFAULT_VCM_MAX 73
#define DEFAULT_VCM_MED 35
#define DEFAULT_VCM_MIN 8
#endif

#define CONFIG_MIPI_798MBPS

DEFINE_MUTEX(imx175_mut);
DEFINE_MUTEX(imx175_sensor_init_mut);
static struct msm_sensor_ctrl_t imx175_s_ctrl;

static struct msm_camera_i2c_reg_conf imx175_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf imx175_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf imx175_groupon_settings[] = {
	{0x104, 0x01},
};

static struct msm_camera_i2c_reg_conf imx175_groupoff_settings[] = {
	{0x104, 0x00},
};

static struct msm_camera_i2c_reg_conf imx175_mipi_settings[] = {

	    {0x3020, 0x10},
	    {0x302D, 0x03},
	    {0x302F, 0x80},
	    {0x3032, 0xA3},
	    {0x3033, 0x20},
	    {0x3034, 0x24},
	    {0x3041, 0x15},
	    {0x3042, 0x87},
	    {0x3050, 0x35},
	    {0x3056, 0x57},
	    {0x305D, 0x41},
	    {0x3097, 0x69},
	    {0x3109, 0x41},
	    {0x3148, 0x3F},
	    {0x330F, 0x07},

	    {0x4100, 0x06},
	    {0x4104, 0x32},
	    {0x4105, 0x32},
	    {0x4108, 0x01},
	    {0x4109, 0x7C},
	    {0x410A, 0x00},
	    {0x410B, 0x00},
};

static struct msm_camera_i2c_reg_conf imx175_pll_settings[] = {

	
	{0x030C, 0x00}, 
	{0x030D, 0xC7}, 
	{0x0301, 0x0A}, 
	{0x0303, 0x01}, 
	{0x0305, 0x06}, 
	{0x0309, 0x0A}, 
	{0x030B, 0x01}, 
	{0x3368, 0x18}, 
	{0x3369, 0x00},
	{0x3344, 0x00},
	{0x3345, 0x00},
	{0x3370, 0x7F},
	{0x3371, 0x37},
      {0x3372, 0x5F},
      {0x3373, 0x37},
      {0x3374, 0x37},
      {0x3375, 0x3F},
      {0x3376, 0xBF},
      {0x3377, 0x3F},

};

static struct msm_camera_i2c_reg_conf imx175_prev_settings[] = {

    {0x41C0, 0x01},
    {0x0100, 0x00},
    {0x030E, 0x01},
    {0x0202, 0x04},
    {0x0203, 0xDA},
    {0x0301, 0x0A},
    {0x0303, 0x01},
    {0x0305, 0x06},
    {0x0309, 0x0A},
    {0x030B, 0x01},

    {0x030C, 0x00},
    {0x030D, 0xC7},
    {0x0340, 0x04}, 	
    {0x0341, 0xF4},
    {0x0342, 0x0D},   
    {0x0343, 0x70},
    {0x0344, 0x00}, 
    {0x0345, 0x00},
    {0x0346, 0x00}, 
    {0x0347, 0x00},
    {0x0348, 0x0C},   
    {0x0349, 0xCF},
    {0x034A, 0x09},  
    {0x034B, 0x9F},
    {0x034C, 0x06},    
    {0x034D, 0x68},
    {0x034E, 0x04},    
    {0x034F, 0xD0},
    {0x0390, 0x01},	

    {0x3020, 0x10},
    {0x302D, 0x03},
    {0x302F, 0x80},
    {0x3032, 0xA3},
    {0x3033, 0x20},
    {0x3034, 0x24},
    {0x3041, 0x15},
    {0x3042, 0x87},
    {0x3050, 0x35},
    {0x3056, 0x57},
    {0x305D, 0x41},
    {0x3097, 0x69},
    {0x3109, 0x41},
    {0x3148, 0x3F},
    {0x330F, 0x07},

    {0x3344, 0x67},
    {0x3345, 0x1F},
    {0x3364, 0x02},

    {0x3368, 0x18},
    {0x3369, 0x00},
    {0x3370, 0x7F},
    {0x3371, 0x37},
    {0x3372, 0x5F},
    {0x3373, 0x37},
    {0x3374, 0x37},
    {0x3375, 0x3F},
    {0x3376, 0xBF},
    {0x3377, 0x3F},
    {0x33C8, 0x00},
    {0x33D4, 0x06},     
    {0x33D5, 0x68},
    {0x33D6, 0x04},     
    {0x33D7, 0xD0},

    {0x4100, 0x06},
    {0x4104, 0x32},
    {0x4105, 0x32},
    {0x4108, 0x01},
    {0x4109, 0x7C},
    {0x410A, 0x00},
    {0x410B, 0x00},
};

static struct msm_camera_i2c_reg_conf imx175_video_settings[] = {
		
	    {0x41C0, 0x01},
	    {0x0100, 0x00},
	    {0x030E, 0x01},
	    {0x0202, 0x03},
	    {0x0203, 0xE2},
	    {0x0301, 0x0A},
	    {0x0303, 0x01},
	    {0x0305, 0x06},
	    {0x0309, 0x0A},
	    {0x030B, 0x01},

	    {0x030C, 0x00},
	    {0x030D, 0xC7},
	    {0x0340, 0x06},  
	    {0x0341, 0x07},
	    {0x0342, 0x0D},      
	    {0x0343, 0x70},
	    {0x0344, 0x00},  
	    {0x0345, 0x00},
	    {0x0346, 0x00}, 
	    {0x0347, 0xF8},
	    {0x0348, 0x0C},  
	    {0x0349, 0xCF},
	    {0x034A, 0x08},  
	    {0x034B, 0xA7},
	    {0x034C, 0x06},    
	    {0x034D, 0x68},
	    {0x034E, 0x03},   
	    {0x034F, 0xD8},
	    {0x0390, 0x01},

	    {0x3020, 0x10},
	    {0x302D, 0x03},
	    {0x302F, 0x80},
	    {0x3032, 0xA3},
	    {0x3033, 0x20},
	    {0x3034, 0x24},
	    {0x3041, 0x15},
	    {0x3042, 0x87},
	    {0x3050, 0x35},
	    {0x3056, 0x57},
	    {0x305D, 0x41},
	    {0x3097, 0x69},
	    {0x3109, 0x41},
	    {0x3148, 0x3F},
	    {0x330F, 0x07},

	    {0x3344, 0x67},
	    {0x3345, 0x1F},
	    {0x3364, 0x02},

	    {0x3368, 0x18},
	    {0x3369, 0x00},
	    {0x3370, 0x7F},
	    {0x3371, 0x37},
	    {0x3372, 0x5F},
	    {0x3373, 0x37},
	    {0x3374, 0x37},
	    {0x3375, 0x3F},
	    {0x3376, 0xBF},
	    {0x3377, 0x3F},
	    {0x33C8, 0x00},
	    {0x33D4, 0x06},     
	    {0x33D5, 0x68},
	    {0x33D6, 0x03},     
	    {0x33D7, 0xD8},

	    {0x4100, 0x06},
	    {0x4104, 0x32},
	    {0x4105, 0x32},
	    {0x4108, 0x01},
	    {0x4109, 0x7C},
	    {0x410A, 0x00},
	    {0x410B, 0x00},

};

static struct msm_camera_i2c_reg_conf imx175_fast_video_settings[] = {

		
	    {0x41C0, 0x01},
	    {0x0100, 0x00},
	    {0x030E, 0x01},
	    {0x0202, 0x03},
	    {0x0203, 0xC2},
	    {0x0301, 0x0A},
	    {0x0303, 0x01},
	    {0x0305, 0x06},
	    {0x0309, 0x0A},
	    {0x030B, 0x01},

	    {0x030C, 0x00},
	    {0x030D, 0xC7},
	    {0x0340, 0x03},  
	    {0x0341, 0xC6},
	    {0x0342, 0x0D},      
	    {0x0343, 0x70},
	    {0x0344, 0x00},  
	    {0x0345, 0x00},
	    {0x0346, 0x01}, 
	    {0x0347, 0x38},
	    {0x0348, 0x0C},  
	    {0x0349, 0xCF},
	    {0x034A, 0x08},  
	    {0x034B, 0x67},
	    {0x034C, 0x06},    
	    {0x034D, 0x68},
	    {0x034E, 0x03},   
	    {0x034F, 0x98},
	    {0x0390, 0x01},

	    {0x3020, 0x10},
	    {0x302D, 0x03},
	    {0x302F, 0x80},
	    {0x3032, 0xA3},
	    {0x3033, 0x20},
	    {0x3034, 0x24},
	    {0x3041, 0x15},
	    {0x3042, 0x87},
	    {0x3050, 0x35},
	    {0x3056, 0x57},
	    {0x305D, 0x41},
	    {0x3097, 0x69},
	    {0x3109, 0x41},
	    {0x3148, 0x3F},
	    {0x330F, 0x07},

	    {0x3344, 0x67},
	    {0x3345, 0x1F},
	    {0x3364, 0x02},

	    {0x3368, 0x18},
	    {0x3369, 0x00},
	    {0x3370, 0x7F},
	    {0x3371, 0x37},
	    {0x3372, 0x5F},
	    {0x3373, 0x37},
	    {0x3374, 0x37},
	    {0x3375, 0x3F},
	    {0x3376, 0xBF},
	    {0x3377, 0x3F},
	    {0x33C8, 0x00},
	    {0x33D4, 0x06},     
	    {0x33D5, 0x68},
	    {0x33D6, 0x03},     
	    {0x33D7, 0x98},

	    {0x4100, 0x06},
	    {0x4104, 0x32},
	    {0x4105, 0x32},
	    {0x4108, 0x01},
	    {0x4109, 0x7C},
	    {0x410A, 0x00},
	    {0x410B, 0x00},
};

static struct msm_camera_i2c_reg_conf imx175_snap_settings[] = {

          {0x41C0, 0x01},
	    {0x0100, 0x00},
	    {0x030E, 0x01},
	    {0x0202, 0x09},
	    {0x0203, 0xAA},
	    {0x0301, 0x0A},
	    {0x0303, 0x01},
	    {0x0305, 0x06},
	    {0x0309, 0x0A},
	    {0x030B, 0x01},

	    {0x030C, 0x00},
	    {0x030D, 0xC7},
	    {0x0340, 0x09},    
	    {0x0341, 0xC4},
	    {0x0342, 0x0D},      
	    {0x0343, 0x70},
	    {0x0344, 0x00},  
	    {0x0345, 0x00},
	    {0x0346, 0x00},  
	    {0x0347, 0x00},
	    {0x0348, 0x0C},  
	    {0x0349, 0xCF},
	    {0x034A, 0x09},  
	    {0x034B, 0x9F},
	    {0x034C, 0x0C},    
	    {0x034D, 0xD0},
	    {0x034E, 0x09},    
	    {0x034F, 0xA0},
	    {0x0390, 0x00},

	    {0x3020, 0x10},
	    {0x302D, 0x03},
	    {0x302F, 0x80},
	    {0x3032, 0xA3},
	    {0x3033, 0x20},
	    {0x3034, 0x24},
	    {0x3041, 0x15},
	    {0x3042, 0x87},
	    {0x3050, 0x35},
	    {0x3056, 0x57},
	    {0x305D, 0x41},
	    {0x3097, 0x69},
	    {0x3109, 0x41},
	    {0x3148, 0x3F},
	    {0x330F, 0x07},

	    {0x3344, 0x67},
	    {0x3345, 0x1F},
	    {0x3364, 0x02},

	    {0x3368, 0x18},
	    {0x3369, 0x00},
	    {0x3370, 0x7F},
	    {0x3371, 0x37},
	    {0x3372, 0x5F},
	    {0x3373, 0x37},
	    {0x3374, 0x37},
	    {0x3375, 0x3F},
	    {0x3376, 0xBF},
	    {0x3377, 0x3F},
	    {0x33C8, 0x00},
	    {0x33D4, 0x0C},     
	    {0x33D5, 0xD0},
	    {0x33D6, 0x09},     
	    {0x33D7, 0xA0},

	    {0x4100, 0x06},
	    {0x4104, 0x32},
	    {0x4105, 0x32},
	    {0x4108, 0x01},
	    {0x4109, 0x7C},
	    {0x410A, 0x00},
	    {0x410B, 0x00},

};

static struct msm_camera_i2c_reg_conf imx175_4_3_settings[] = {
};
static struct msm_camera_i2c_reg_conf imx175_snap_wide_settings[] = {

	    {0x41C0, 0x01},
	    {0x0100, 0x00},
	    {0x030E, 0x01},
	    {0x0202, 0x07},
	    {0x0203, 0x40},
	    {0x0301, 0x0A},
	    {0x0303, 0x01},
	    {0x0305, 0x06},
	    {0x0309, 0x0A},
	    {0x030B, 0x01},

	    {0x030C, 0x00},
	    {0x030D, 0xC7},
	    {0x0340, 0x07},    
	    {0x0341, 0x5A},
	    {0x0342, 0x0D}, 
	    {0x0343, 0x70},
	    {0x0344, 0x00}, 
	    {0x0345, 0x00},
	    {0x0346, 0x01}, 
	    {0x0347, 0x36},
	    {0x0348, 0x0C}, 
	    {0x0349, 0xCF},
	    {0x034A, 0x08}, 
	    {0x034B, 0x6B},
	    {0x034C, 0x0C},
	    {0x034D, 0xD0},
	    {0x034E, 0x07},
	    {0x034F, 0x36},
	    {0x0390, 0x00},

	    {0x3020, 0x10},
	    {0x302D, 0x03},
	    {0x302F, 0x80},
	    {0x3032, 0xA3},
	    {0x3033, 0x20},
	    {0x3034, 0x24},
	    {0x3041, 0x15},
	    {0x3042, 0x87},
	    {0x3050, 0x35},
	    {0x3056, 0x57},
	    {0x305D, 0x41},
	    {0x3097, 0x69},
	    {0x3109, 0x41},
	    {0x3148, 0x3F},
	    {0x330F, 0x07},

	    {0x3344, 0x67},
	    {0x3345, 0x1F},
	    {0x3364, 0x02},

	    {0x3368, 0x18},
	    {0x3369, 0x00},
	    {0x3370, 0x7F},
	    {0x3371, 0x37},
	    {0x3372, 0x5F},
	    {0x3373, 0x37},
	    {0x3374, 0x37},
	    {0x3375, 0x3F},
	    {0x3376, 0xBF},
	    {0x3377, 0x3F},
	    {0x33C8, 0x00},
	    {0x33D4, 0x0C},     
	    {0x33D5, 0xD0},
	    {0x33D6, 0x07},         
	    {0x33D7, 0x36},

	    {0x4100, 0x06},
	    {0x4104, 0x32},
	    {0x4105, 0x32},
	    {0x4108, 0x01},
	    {0x4109, 0x7C},
	    {0x410A, 0x00},
	    {0x410B, 0x00},

};
static struct msm_camera_i2c_reg_conf imx175_video_hfr_5_3_settings[] = {
};

static struct msm_camera_i2c_reg_conf imx175_5_3_settings[] = {

	    {0x41C0, 0x01},
	    {0x0100, 0x00},
	    {0x030E, 0x01},
	    {0x0202, 0x07},
	    {0x0203, 0xBA},
	    {0x0301, 0x0A},
	    {0x0303, 0x01},
	    {0x0305, 0x06},
	    {0x0309, 0x0A},
	    {0x030B, 0x01},

	    {0x030C, 0x00},
	    {0x030D, 0xC7},
	    {0x0340, 0x07},    	
	    {0x0341, 0xD0},
	    {0x0342, 0x0D}, 
	    {0x0343, 0x70},
	    {0x0344, 0x00}, 
	    {0x0345, 0x00},
	    {0x0346, 0x00}, 
	    {0x0347, 0xF8},
	    {0x0348, 0x0C}, 
	    {0x0349, 0xCF},
	    {0x034A, 0x08}, 
	    {0x034B, 0xA7},
	    {0x034C, 0x0C},	 
	    {0x034D, 0xD0},
	    {0x034E, 0x07},	 
	    {0x034F, 0xB0},
	    {0x0390, 0x00},

	    {0x3020, 0x10},
	    {0x302D, 0x03},
	    {0x302F, 0x80},
	    {0x3032, 0xA3},
	    {0x3033, 0x20},
	    {0x3034, 0x24},
	    {0x3041, 0x15},
	    {0x3042, 0x87},
	    {0x3050, 0x35},
	    {0x3056, 0x57},
	    {0x305D, 0x41},
	    {0x3097, 0x69},
	    {0x3109, 0x41},
	    {0x3148, 0x3F},
	    {0x330F, 0x07},

	    {0x3344, 0x67},
	    {0x3345, 0x1F},
	    {0x3364, 0x02},

	    {0x3368, 0x18},
	    {0x3369, 0x00},
	    {0x3370, 0x7F},
	    {0x3371, 0x37},
	    {0x3372, 0x5F},
	    {0x3373, 0x37},
	    {0x3374, 0x37},
	    {0x3375, 0x3F},
	    {0x3376, 0xBF},
	    {0x3377, 0x3F},
	    {0x33C8, 0x00},
	    {0x33D4, 0x0C},     
	    {0x33D5, 0xD0},
	    {0x33D6, 0x07},     
	    {0x33D7, 0xB0},

	    {0x4100, 0x06},
	    {0x4104, 0x32},
	    {0x4105, 0x32},
	    {0x4108, 0x01},
	    {0x4109, 0x7C},
	    {0x410A, 0x00},
	    {0x410B, 0x00},
};

static struct msm_camera_i2c_reg_conf imx175_video_60fps_settings[] = {
};

static struct msm_camera_i2c_reg_conf imx175_video_24fps_5_3_settings[] = {
		
	    {0x41C0, 0x01},
	    {0x0100, 0x00},
	    {0x030E, 0x01},
	    {0x0202, 0x03},
	    {0x0203, 0xE2},
	    {0x0301, 0x0A},
	    {0x0303, 0x01},
	    {0x0305, 0x06},
	    {0x0309, 0x0A},
	    {0x030B, 0x01},

	    {0x030C, 0x00},
	    {0x030D, 0xC7},
	    {0x0340, 0x07},  
	    {0x0341, 0x8E},
	    {0x0342, 0x0D},      
	    {0x0343, 0x70},
	    {0x0344, 0x00},  
	    {0x0345, 0x00},
	    {0x0346, 0x00}, 
	    {0x0347, 0xF8},
	    {0x0348, 0x0C},  
	    {0x0349, 0xCF},
	    {0x034A, 0x08},  
	    {0x034B, 0xA7},
	    {0x034C, 0x06},    
	    {0x034D, 0x68},
	    {0x034E, 0x03},   
	    {0x034F, 0xD8},
	    {0x0390, 0x01},

	    {0x3020, 0x10},
	    {0x302D, 0x03},
	    {0x302F, 0x80},
	    {0x3032, 0xA3},
	    {0x3033, 0x20},
	    {0x3034, 0x24},
	    {0x3041, 0x15},
	    {0x3042, 0x87},
	    {0x3050, 0x35},
	    {0x3056, 0x57},
	    {0x305D, 0x41},
	    {0x3097, 0x69},
	    {0x3109, 0x41},
	    {0x3148, 0x3F},
	    {0x330F, 0x07},

	    {0x3344, 0x67},
	    {0x3345, 0x1F},
	    {0x3364, 0x02},

	    {0x3368, 0x18},
	    {0x3369, 0x00},
	    {0x3370, 0x7F},
	    {0x3371, 0x37},
	    {0x3372, 0x5F},
	    {0x3373, 0x37},
	    {0x3374, 0x37},
	    {0x3375, 0x3F},
	    {0x3376, 0xBF},
	    {0x3377, 0x3F},
	    {0x33C8, 0x00},
	    {0x33D4, 0x06},     
	    {0x33D5, 0x68},
	    {0x33D6, 0x03},     
	    {0x33D7, 0xD8},

	    {0x4100, 0x06},
	    {0x4104, 0x32},
	    {0x4105, 0x32},
	    {0x4108, 0x01},
	    {0x4109, 0x7C},
	    {0x410A, 0x00},
	    {0x410B, 0x00},
};

static struct msm_camera_i2c_reg_conf imx175_video_16_9_settings[] = {

		{0x41C0, 0x01},
		{0x0100, 0x00},
		{0x030E, 0x01},
		{0x0202, 0x06},
		{0x0203, 0x03},
		{0x0301, 0x0A},
		{0x0303, 0x01},
		{0x0305, 0x06},
		{0x0309, 0x0A},
		{0x030B, 0x01},
		{0x030C, 0x00},
		{0x030D, 0xC7},
		{0x0340, 0x06},
		{0x0341, 0x07},
		{0x0342, 0x0D},
		{0x0343, 0x70},
		{0x0344, 0x00},
		{0x0345, 0x00},
		{0x0346, 0x01},
		{0x0347, 0x38},
		{0x0348, 0x0C},
		{0x0349, 0xCF},
		{0x034A, 0x08},
		{0x034B, 0x67},
		{0x034C, 0x06},
		{0x034D, 0x68},
		{0x034E, 0x03},
		{0x034F, 0x98},
		{0x0390, 0x01},
		{0x3020, 0x10},
		{0x302D, 0x03},
		{0x302F, 0x80},
		{0x3032, 0xA3},
		{0x3033, 0x20},
		{0x3034, 0x24},
		{0x3041, 0x15},
		{0x3042, 0x87},
		{0x3050, 0x35},
		{0x3056, 0x57},
		{0x305D, 0x41},
		{0x3097, 0x69},
		{0x3109, 0x41},
		{0x3148, 0x3F},
		{0x330F, 0x07},
		{0x3344, 0x67},
		{0x3345, 0x1F},
		{0x3364, 0x02},
		{0x3368, 0x18},
		{0x3369, 0x00},
		{0x3370, 0x7F},
		{0x3371, 0x37},
		{0x3372, 0x5F},
		{0x3373, 0x37},
		{0x3374, 0x37},
		{0x3375, 0x3F},
		{0x3376, 0xBF},
		{0x3377, 0x3F},
		{0x33C8, 0x00},
		{0x33D4, 0x06},	 
		{0x33D5, 0x68},
		{0x33D6, 0x03},	 
		{0x33D7, 0x98},
		{0x4100, 0x06},
		{0x4104, 0x32},
		{0x4105, 0x32},
		{0x4108, 0x01},
		{0x4109, 0x7C},
		{0x410A, 0x00},
		{0x410B, 0x00},

};

static struct msm_camera_i2c_reg_conf imx175_video_5_3_settings[] = {
	
	    {0x41C0, 0x01},
	    {0x0100, 0x00},
	    {0x030E, 0x01},
	    {0x0202, 0x03},
	    {0x0203, 0xE2},
	    {0x0301, 0x0A},
	    {0x0303, 0x01},
	    {0x0305, 0x06},
	    {0x0309, 0x0A},
	    {0x030B, 0x01},

	    {0x030C, 0x00},
	    {0x030D, 0xC7},
	    {0x0340, 0x06},  
	    {0x0341, 0x07},
	    {0x0342, 0x0D},      
	    {0x0343, 0x70},
	    {0x0344, 0x00},  
	    {0x0345, 0x00},
	    {0x0346, 0x00}, 
	    {0x0347, 0xF8},
	    {0x0348, 0x0C},  
	    {0x0349, 0xCF},
	    {0x034A, 0x08},  
	    {0x034B, 0xA7},
	    {0x034C, 0x06},    
	    {0x034D, 0x68},
	    {0x034E, 0x03},   
	    {0x034F, 0xD8},
	    {0x0390, 0x01},

	    {0x3020, 0x10},
	    {0x302D, 0x03},
	    {0x302F, 0x80},
	    {0x3032, 0xA3},
	    {0x3033, 0x20},
	    {0x3034, 0x24},
	    {0x3041, 0x15},
	    {0x3042, 0x87},
	    {0x3050, 0x35},
	    {0x3056, 0x57},
	    {0x305D, 0x41},
	    {0x3097, 0x69},
	    {0x3109, 0x41},
	    {0x3148, 0x3F},
	    {0x330F, 0x07},

	    {0x3344, 0x67},
	    {0x3345, 0x1F},
	    {0x3364, 0x02},

	    {0x3368, 0x18},
	    {0x3369, 0x00},
	    {0x3370, 0x7F},
	    {0x3371, 0x37},
	    {0x3372, 0x5F},
	    {0x3373, 0x37},
	    {0x3374, 0x37},
	    {0x3375, 0x3F},
	    {0x3376, 0xBF},
	    {0x3377, 0x3F},
	    {0x33C8, 0x00},
	    {0x33D4, 0x06},     
	    {0x33D5, 0x68},
	    {0x33D6, 0x03},     
	    {0x33D7, 0xD8},

	    {0x4100, 0x06},
	    {0x4104, 0x32},
	    {0x4105, 0x32},
	    {0x4108, 0x01},
	    {0x4109, 0x7C},
	    {0x410A, 0x00},
	    {0x410B, 0x00},

};


static struct msm_camera_i2c_reg_conf imx175_video_24fps_16_9_settings[] = {

		
	    {0x41C0, 0x01},
	    {0x0100, 0x00},
	    {0x030E, 0x01},
	    {0x0202, 0x06},
	    {0x0203, 0x03},
	    {0x0301, 0x0A},
	    {0x0303, 0x01},
	    {0x0305, 0x06},
	    {0x0309, 0x0A},
	    {0x030B, 0x01},

	    {0x030C, 0x00},
	    {0x030D, 0xC7},
	    {0x0340, 0x03},  
	    {0x0341, 0xC6},
	    {0x0342, 0x0D},      
	    {0x0343, 0x70},
	    {0x0344, 0x00},  
	    {0x0345, 0x00},
	    {0x0346, 0x01}, 
	    {0x0347, 0x38},
	    {0x0348, 0x0C},  
	    {0x0349, 0xCF},
	    {0x034A, 0x08},  
	    {0x034B, 0x67},
	    {0x034C, 0x06},    
	    {0x034D, 0x68},
	    {0x034E, 0x03},   
	    {0x034F, 0x98},
	    {0x0390, 0x01},

	    {0x3020, 0x10},
	    {0x302D, 0x03},
	    {0x302F, 0x80},
	    {0x3032, 0xA3},
	    {0x3033, 0x20},
	    {0x3034, 0x24},
	    {0x3041, 0x15},
	    {0x3042, 0x87},
	    {0x3050, 0x35},
	    {0x3056, 0x57},
	    {0x305D, 0x41},
	    {0x3097, 0x69},
	    {0x3109, 0x41},
	    {0x3148, 0x3F},
	    {0x330F, 0x07},

	    {0x3344, 0x67},
	    {0x3345, 0x1F},
	    {0x3364, 0x02},

	    {0x3368, 0x18},
	    {0x3369, 0x00},
	    {0x3370, 0x7F},
	    {0x3371, 0x37},
	    {0x3372, 0x5F},
	    {0x3373, 0x37},
	    {0x3374, 0x37},
	    {0x3375, 0x3F},
	    {0x3376, 0xBF},
	    {0x3377, 0x3F},
	    {0x33C8, 0x00},
	    {0x33D4, 0x06},     
	    {0x33D5, 0x68},
	    {0x33D6, 0x03},     
	    {0x33D7, 0x98},

	    {0x4100, 0x06},
	    {0x4104, 0x32},
	    {0x4105, 0x32},
	    {0x4108, 0x01},
	    {0x4109, 0x7C},
	    {0x410A, 0x00},
	    {0x410B, 0x00},
};

static struct msm_camera_i2c_reg_conf imx175_recommend_settings[] = {
};

static struct v4l2_subdev_info imx175_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	
};

static struct msm_camera_i2c_conf_array imx175_init_conf[] = {
	{&imx175_mipi_settings[0],
	ARRAY_SIZE(imx175_mipi_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_recommend_settings[0],
	ARRAY_SIZE(imx175_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_pll_settings[0],
	ARRAY_SIZE(imx175_pll_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_conf_array imx175_confs[] = {
	{&imx175_snap_settings[0],
	ARRAY_SIZE(imx175_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_prev_settings[0],
	ARRAY_SIZE(imx175_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_video_settings[0],
	ARRAY_SIZE(imx175_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_fast_video_settings[0],
	ARRAY_SIZE(imx175_fast_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_snap_wide_settings[0],
	ARRAY_SIZE(imx175_snap_wide_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_4_3_settings[0],
	ARRAY_SIZE(imx175_4_3_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_video_hfr_5_3_settings[0],
	ARRAY_SIZE(imx175_video_hfr_5_3_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_5_3_settings[0],
	ARRAY_SIZE(imx175_5_3_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_video_24fps_16_9_settings[0],
	ARRAY_SIZE(imx175_video_24fps_16_9_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_video_60fps_settings[0],
	ARRAY_SIZE(imx175_video_60fps_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_video_24fps_5_3_settings[0],
	ARRAY_SIZE(imx175_video_24fps_5_3_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_video_16_9_settings[0],
	ARRAY_SIZE(imx175_video_16_9_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_video_5_3_settings[0],
	ARRAY_SIZE(imx175_video_5_3_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t imx175_dimensions[] = {
	
	{
		.x_output = 0xCD0,	
		.y_output = 0x9A0,	
		.line_length_pclk = 0xD70,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x9C4,	
#else
		.frame_length_lines = 0x9B0,
#endif

#ifdef CONFIG_MIPI_798MBPS
		.vt_pixel_clk = 159600000,
		.op_pixel_clk = 159600000,
#else
		.vt_pixel_clk = 122400000,
		.op_pixel_clk = 122400000,
#endif
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x99F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	
	{
		.x_output = 0x668,
		.y_output = 0x4D0,
		.line_length_pclk = 0xD70,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x4F4,
#else
		.frame_length_lines = 0x600,
#endif
#ifdef CONFIG_MIPI_798MBPS
		.vt_pixel_clk = 159600000,
		.op_pixel_clk = 159600000,
#else
		.vt_pixel_clk = 122400000,
		.op_pixel_clk = 122400000,
#endif
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x99F,
		.x_even_inc = 1,
		.x_odd_inc = 3,	
		.y_even_inc = 1,
		.y_odd_inc = 3,	
		.binning_rawchip = 0x22,
	},
	
	{
		.x_output = 0x668,
		.y_output = 0x3D8,
		.line_length_pclk = 0xD70,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x607,
#else
		.frame_length_lines = 0x600,
#endif
#ifdef CONFIG_MIPI_798MBPS
		.vt_pixel_clk = 159600000,
		.op_pixel_clk = 159600000,
#else
		.vt_pixel_clk = 122400000,
		.op_pixel_clk = 122400000,
#endif
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0xF8,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x8A7,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
	{
	
		.x_output = 0x668,
		.y_output = 0x398,
		.line_length_pclk = 0xD70,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x3C6,	
#else
		.frame_length_lines = 0x600,
#endif
#ifdef CONFIG_MIPI_798MBPS
		.vt_pixel_clk = 159600000,
		.op_pixel_clk = 159600000,
#else
		.vt_pixel_clk = 122400000,
		.op_pixel_clk = 122400000,
#endif
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0x138,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x867,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
	{
		.x_output = 0xCD0,	
		.y_output = 0x736,	
		.line_length_pclk = 0xD70,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x75A,
#else
		.frame_length_lines = 0x740,
#endif
		.vt_pixel_clk = 159600000,
		.op_pixel_clk = 159600000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0x0136,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x86B,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{
		.x_output = 0xCD0,
		.y_output = 0x9A0,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x9C4,
#else
		.frame_length_lines = 0x9C0,
#endif
		.vt_pixel_clk = 182400000,
		.op_pixel_clk = 182400000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x99F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{
		.x_output = 0x668,
		.y_output = 0x4D0,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x4F4,
#else
		.frame_length_lines = 0x4E0,
#endif
		.vt_pixel_clk = 182400000,
		.op_pixel_clk = 182400000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xCCD,
		.y_addr_end = 0x99F,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x11,
	},
	{
		.x_output = 0xCD0,	
		.y_output = 0x7B0,	
		.line_length_pclk = 0xD70,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x7D0,
#else
		.frame_length_lines = 0x7C0,
#endif
		.vt_pixel_clk = 159600000,
		.op_pixel_clk = 159600000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0x0F8,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x8A7,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	
	{
		.x_output = 0x668,
		.y_output = 0x398,
		.line_length_pclk = 0xD70,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x3C6,
#else
		.frame_length_lines = 0x600,
#endif
#ifdef CONFIG_MIPI_798MBPS
		.vt_pixel_clk = 159600000,
		.op_pixel_clk = 159600000,
#else
		.vt_pixel_clk = 122400000,
		.op_pixel_clk = 122400000,
#endif
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0x138,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x867,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
	
	{
		.x_output = 0x668,
		.y_output = 0x4D0,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x77C,
#else
		.frame_length_lines = 0x600,
#endif
#ifdef CONFIG_MIPI_798MBPS
		.vt_pixel_clk = 159600000,
		.op_pixel_clk = 159600000,
#else
		.vt_pixel_clk = 122400000,
		.op_pixel_clk = 122400000,
#endif
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xCCD,
		.y_addr_end = 0x99F,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
	
	{
		.x_output = 0x668,
		.y_output = 0x3D8,
		.line_length_pclk = 0xD70,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x78E,	
#else
		.frame_length_lines = 0x600,
#endif
#ifdef CONFIG_MIPI_798MBPS
		.vt_pixel_clk = 159600000,
		.op_pixel_clk = 159600000,
#else
		.vt_pixel_clk = 122400000,
		.op_pixel_clk = 122400000,
#endif
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0xF8,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x8A7,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
	
	{
		.x_output = 0x668,
		.y_output = 0x398,
		.line_length_pclk = 0xD70,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x607,
#else
		.frame_length_lines = 0x600,
#endif
#ifdef CONFIG_MIPI_798MBPS
		.vt_pixel_clk = 159600000,
		.op_pixel_clk = 159600000,
#else
		.vt_pixel_clk = 122400000,
		.op_pixel_clk = 122400000,
#endif
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0x138,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x867,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
	
	{
		.x_output = 0x668,
		.y_output = 0x3D8,
		.line_length_pclk = 0xD70,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x607,
#else
		.frame_length_lines = 0x600,
#endif
#ifdef CONFIG_MIPI_798MBPS
		.vt_pixel_clk = 159600000,
		.op_pixel_clk = 159600000,
#else
		.vt_pixel_clk = 122400000,
		.op_pixel_clk = 122400000,
#endif
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0xF8,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x8A7,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,	},
};

#if 0
static struct msm_camera_csid_vc_cfg imx175_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params imx175_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 2,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = imx175_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 2,
		.settle_cnt = 0x1B,
	},
};
#else
static struct msm_camera_csi_params imx175_csi_params = {
	.data_format = CSI_10BIT,
	.lane_cnt    = 2,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 0x1B,
	.dt = CSI_RAW10,
};
#endif

static struct msm_camera_csi_params *imx175_csi_params_array[] = {
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
};

static struct msm_sensor_output_reg_addr_t imx175_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static struct msm_sensor_id_info_t imx175_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x0175,
};

static struct msm_sensor_exp_gain_info_t imx175_exp_gain_info = {
	.coarse_int_time_addr = 0x202,
	.global_gain_addr = 0x204,
	.vert_offset = 4,
	.min_vert = 4,  
	.sensor_max_linecount = 65531,  
};

static int imx175_read_fuseid(struct sensor_cfg_data *cdata,
	struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	int i;
	uint16_t read_data = 0;
	uint8_t OTP[10] = {0};

	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3400, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0)
		pr_err("%s: msm_camera_i2c_write 0x3400 failed\n", __func__);

	
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3402, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0)
		pr_err("%s: msm_camera_i2c_write 0x3402 failed\n", __func__);

	for (i = 0; i < 10; i++) {
		rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, (0x3404 + i), &read_data, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0)
			pr_err("%s: msm_camera_i2c_read 0x%x failed\n", __func__, (0x3404 + i));

		OTP[i] = (uint8_t)(read_data&0x00FF);
		read_data = 0;
	}

	pr_info("%s: VenderID=%x,LensID=%x,SensorID=%02x%02x\n", __func__,
		OTP[0], OTP[1], OTP[2], OTP[3]);
	pr_info("%s: ModuleFuseID= %02x%02x%02x%02x%02x%02x\n", __func__,
		OTP[4], OTP[5], OTP[6], OTP[7], OTP[8], OTP[9]);

	cdata->cfg.fuse.fuse_id_word1 = 0;
	cdata->cfg.fuse.fuse_id_word2 = (OTP[0]);
	cdata->cfg.fuse.fuse_id_word3 =
		(OTP[4]<<24) |
		(OTP[5]<<16) |
		(OTP[6]<<8) |
		(OTP[7]);
	cdata->cfg.fuse.fuse_id_word4 =
		(OTP[8]<<8) |
		(OTP[9]);

	pr_info("imx175: fuse->fuse_id_word1:%d\n",
		cdata->cfg.fuse.fuse_id_word1);
	pr_info("imx175: fuse->fuse_id_word2:%d\n",
		cdata->cfg.fuse.fuse_id_word2);
	pr_info("imx175: fuse->fuse_id_word3:0x%08x\n",
		cdata->cfg.fuse.fuse_id_word3);
	pr_info("imx175: fuse->fuse_id_word4:0x%08x\n",
		cdata->cfg.fuse.fuse_id_word4);
	return 0;

}

static int imx175_mirror_flip_setting(void)
{
	int rc = 0;
	uint16_t value = 0;

	pr_info("[CAM] %s\n", __func__);

	
	if (imx175_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR_FLIP)
		value = IMX175_READ_MIRROR_FLIP;
	else if (imx175_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR)
		value = IMX175_READ_MIRROR;
	else if (imx175_s_ctrl.mirror_flip == CAMERA_SENSOR_FLIP)
		value = IMX175_READ_FLIP;
	else
		value = IMX175_READ_NORMAL_MODE;

	msm_camera_i2c_write(imx175_s_ctrl.sensor_i2c_client,
		IMX175_REG_READ_MODE, value, MSM_CAMERA_I2C_BYTE_DATA);

	return rc;
}
int32_t imx175_set_dig_gain(struct msm_sensor_ctrl_t *s_ctrl, uint16_t dig_gain)
{
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		REG_DIGITAL_GAIN_GREEN_R, dig_gain,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		REG_DIGITAL_GAIN_RED, dig_gain,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		REG_DIGITAL_GAIN_BLUE, dig_gain,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		REG_DIGITAL_GAIN_GREEN_B, dig_gain,
		MSM_CAMERA_I2C_WORD_DATA);

	return 0;
}


static int imx175_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	pr_info("[CAM] %s\n", __func__);
	
	if (data->sensor_platform_info)
		imx175_s_ctrl.mirror_flip = data->sensor_platform_info->mirror_flip;
	

	return rc;
}

static const char *imx175Vendor = "sony";
static const char *imx175NAME = "imx175";
static const char *imx175Size = "8M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", imx175Vendor, imx175NAME, imx175Size);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);

static struct kobject *android_imx175;

static int imx175_sysfs_init(void)
{
	int ret ;
	pr_info("imx175:kobject creat and add\n");
	android_imx175 = kobject_create_and_add("android_camera", NULL);
	if (android_imx175 == NULL) {
		pr_info("imx175_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("imx175:sysfs_create_file\n");
	ret = sysfs_create_file(android_imx175, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("imx175_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_imx175);
	}

	return 0 ;
}

int32_t imx175_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	rc = 0;
	pr_info("[CAM] %s\n", __func__);
	rc = msm_sensor_i2c_probe(client, id);
	if(rc >= 0)
		imx175_sysfs_init();
	pr_info("[CAM] %s: rc(%d)\n", __func__, rc);
	return rc;
}

static const struct i2c_device_id imx175_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&imx175_s_ctrl},
	{ }
};

static struct i2c_driver imx175_i2c_driver = {
	.id_table = imx175_i2c_id,
	.probe  = imx175_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client imx175_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};


int32_t imx175_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc;
	struct msm_camera_sensor_info *sdata = NULL;
	pr_info("[CAM] %s\n", __func__);

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_err("[CAM] %s: s_ctrl sensordata NULL\n", __func__);
		return -EINVAL;
	}

	if (sdata->camera_power_on == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}

	if (!sdata->use_rawchip) {
		rc = msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
		if (rc < 0) {
			pr_err("[CAM] %s: msm_camio_sensor_clk_on failed:%d\n",
			 __func__, rc);
			goto enable_mclk_failed;
		}
	}

	rc = sdata->camera_power_on();
	if (rc < 0) {
		pr_err("[CAM] %s failed to enable power\n", __func__);
		goto enable_power_on_failed;
	}

	rc = msm_sensor_set_power_up(s_ctrl);
	if (rc < 0) {
		pr_err("[CAM] %s msm_sensor_power_up failed\n", __func__);
		goto enable_sensor_power_up_failed;
	}

	imx175_sensor_open_init(sdata);
	pr_info("[CAM] %s end\n", __func__);

	return rc;

enable_sensor_power_up_failed:
	if (sdata->camera_power_off == NULL)
		pr_err("sensor platform_data didnt register\n");
	else
		sdata->camera_power_off();
enable_power_on_failed:
	if (!sdata->use_rawchip)
	msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
enable_mclk_failed:
	return rc;
}

int32_t imx175_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	struct msm_camera_sensor_info *sdata = NULL;
	pr_info("[CAM] %s\n", __func__);

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_err("[CAM] %s: s_ctrl sensordata NULL\n", __func__);
		return -EINVAL;
	}

	rc = msm_sensor_set_power_down(s_ctrl);
	if (rc < 0) {
		pr_err("[CAM] %s msm_sensor_set_power_down failed\n", __func__);
	}

	if (sdata->camera_power_off == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}

	rc = sdata->camera_power_off();
	if (rc < 0) {
		pr_err("[CAM] %s failed to disable power\n", __func__);
		return rc;
	}

	if (!sdata->use_rawchip) {
		msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
		if (rc < 0)
			pr_err("[CAM] %s: msm_camio_sensor_clk_off failed:%d\n",
				 __func__, rc);
	}

	return rc;
}

#if 0
static int imx175_probe(struct platform_device *pdev)
{
	int	rc = 0;

	pr_info("%s\n", __func__);

	rc = msm_sensor_register(pdev, imx175_sensor_v4l2_probe);
	if(rc >= 0)
		imx175_sysfs_init();
	return rc;
}

struct platform_driver imx175_driver = {
	.probe = imx175_probe,
	.driver = {
		.name = PLATFORM_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init msm_sensor_init_module(void)
{
	pr_info("%s\n", __func__);
	return platform_driver_register(&imx175_driver);
}
#else
static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&imx175_i2c_driver);
}
#endif

static struct v4l2_subdev_core_ops imx175_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops imx175_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops imx175_subdev_ops = {
	.core = &imx175_subdev_core_ops,
	.video  = &imx175_subdev_video_ops,
};

int32_t imx175_write_exp_gain1_ex(struct msm_sensor_ctrl_t *s_ctrl,
		int mode, uint16_t gain, uint16_t dig_gain, uint32_t line) 
{
	uint32_t fl_lines;
	uint8_t offset;

	uint32_t fps_divider = Q10;
	if (s_ctrl->mode == SENSOR_PREVIEW_MODE)
		fps_divider = s_ctrl->fps_divider;

	if(line > s_ctrl->sensor_exp_gain_info->sensor_max_linecount)
		line = s_ctrl->sensor_exp_gain_info->sensor_max_linecount;

	fl_lines = s_ctrl->curr_frame_length_lines;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line * Q10 > (fl_lines - offset) * fps_divider)
		fl_lines = line + offset;
	else
		fl_lines = (fl_lines * fps_divider) / Q10;

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines, fl_lines,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, line,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain,
		MSM_CAMERA_I2C_WORD_DATA);

	if (s_ctrl->func_tbl->sensor_set_dig_gain)
		s_ctrl->func_tbl->sensor_set_dig_gain(s_ctrl, dig_gain);

	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}

int32_t imx175_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res) {

	int rc = 0;
	pr_info("[CAM] %s\n", __func__);

#if ((defined CONFIG_I2C_CPLD) && ((defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY) || (defined CONFIG_MACH_DUMMY)))
	rc = msm_sensor_setting_parallel1(s_ctrl, update_type, res);
#else
	rc = msm_sensor_setting1(s_ctrl, update_type, res);
#endif

	if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		imx175_mirror_flip_setting();
	}

	return rc;
}


static struct msm_sensor_fn_t imx175_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_write_exp_gain_ex = imx175_write_exp_gain1_ex,
	
	.sensor_set_dig_gain = imx175_set_dig_gain,
	
	.sensor_write_snapshot_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_write_snapshot_exp_gain_ex = imx175_write_exp_gain1_ex,
#if 0
	.sensor_setting = msm_sensor_setting,
#else
	.sensor_csi_setting = imx175_sensor_setting,
#endif
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_power_up = imx175_power_up,
	.sensor_power_down = imx175_power_down,
	.sensor_config = msm_sensor_config,
	.sensor_get_csi_params = msm_sensor_get_csi_params,
	.sensor_i2c_read_fuseid = imx175_read_fuseid,
};

static struct msm_sensor_reg_t imx175_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = imx175_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(imx175_start_settings),
	.stop_stream_conf = imx175_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(imx175_stop_settings),
	.group_hold_on_conf = imx175_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(imx175_groupon_settings),
	.group_hold_off_conf = imx175_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(imx175_groupoff_settings),
	.init_settings = &imx175_init_conf[0],
	.init_size = ARRAY_SIZE(imx175_init_conf),
	.mode_settings = &imx175_confs[0],
	.output_settings = &imx175_dimensions[0],
	.num_conf = ARRAY_SIZE(imx175_confs),
};

static struct msm_sensor_ctrl_t imx175_s_ctrl = {
	.msm_sensor_reg = &imx175_regs,
	.sensor_i2c_client = &imx175_sensor_i2c_client,
	.sensor_i2c_addr = 0x20,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_output_reg_addr = &imx175_reg_addr,
	.sensor_id_info = &imx175_id_info,
	.sensor_exp_gain_info = &imx175_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
#if 0
	.csi_params = &imx175_csi_params_array[0],
#else
	.csic_params = &imx175_csi_params_array[0],
#endif
	.msm_sensor_mutex = &imx175_mut,
	.sensor_i2c_driver = &imx175_i2c_driver,
	.sensor_v4l2_subdev_info = imx175_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(imx175_subdev_info),
	.sensor_v4l2_subdev_ops = &imx175_subdev_ops,
	.func_tbl = &imx175_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
	.sensor_first_mutex = &imx175_sensor_init_mut,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Sony 8 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
