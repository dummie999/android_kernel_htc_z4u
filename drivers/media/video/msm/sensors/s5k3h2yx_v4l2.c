#include "msm_sensor.h"
#include "msm.h"

#ifdef CONFIG_RAWCHIP
#include "rawchip/rawchip.h"
#endif

#define SENSOR_NAME "s5k3h2yx"
#define PLATFORM_DRIVER_NAME "msm_camera_s5k3h2yx"
#define s5k3h2yx_obj s5k3h2yx_##obj

#define S5K3H2YX_REG_READ_MODE 0x0101
#define S5K3H2YX_READ_NORMAL_MODE 0x0000	
#define S5K3H2YX_READ_MIRROR 0x0001			
#define S5K3H2YX_READ_FLIP 0x0002			
#define S5K3H2YX_READ_MIRROR_FLIP 0x0003	

#if 0
#define DEFAULT_VCM_MAX 73
#define DEFAULT_VCM_MED 35
#define DEFAULT_VCM_MIN 8
#endif

#define CONFIG_MIPI_798MBPS

#ifdef CONFIG_DEBUG_FRAME_COUNT
struct msm_camera_i2c_client *s5k3h2yx_msm_camera_i2c_client_checkstatus = NULL;
#endif

DEFINE_MUTEX(s5k3h2yx_mut);
DEFINE_MUTEX(s5k3h2yx_sensor_init_mut);
static struct msm_sensor_ctrl_t s5k3h2yx_s_ctrl;

static struct msm_camera_i2c_reg_conf s5k3h2yx_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_groupon_settings[] = {
	{0x104, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_groupoff_settings[] = {
	{0x104, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_mipi_settings[] = {
	 
	{0x3065, 0x35},
	{0x310E, 0x00},
	{0x3098, 0xAB},
	{0x30C7, 0x0A},
	{0x309A, 0x01},
	{0x310D, 0xC6},
	{0x30C3, 0x40},
	{0x30BB, 0x02},
	{0x30BC, 0x38},
	{0x30BD, 0x40},
	{0x3110, 0x70},
	{0x3111, 0x80},
	{0x3112, 0x7B},
	{0x3113, 0xC0},
	{0x30C7, 0x1A},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_pll_settings[] = {
#ifdef CONFIG_MIPI_798MBPS
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0x85},
	{0x0303, 0x01},
	{0x0301, 0x05},
	{0x030B, 0x01},
	{0x0309, 0x05},
	{0x30CC, 0xA0},
	{0x31A1, 0x56},
#else
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0x66},
	{0x0303, 0x01},
	{0x0301, 0x05},
	{0x030B, 0x01},
	{0x0309, 0x05},
	{0x30CC, 0xA0},
	{0x31A1, 0x56},
#endif
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_prev_settings[] = {
#ifdef CONFIG_MIPI_798MBPS
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0x85},
	{0x0303, 0x01},
	{0x0301, 0x05},
	{0x030B, 0x01},
	{0x0309, 0x05},
	{0x30CC, 0xA0},
	{0x31A1, 0x56},
#else
	
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0x66},
	{0x0303, 0x01},
	{0x0301, 0x05},
	{0x030B, 0x01},
	{0x0309, 0x05},
	{0x30CC, 0xA0},
	{0x31A1, 0x56},
#endif

	
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x00},
	{0x0348, 0x0C},
	{0x0349, 0xCD},
	{0x034A, 0x09},
	{0x034B, 0x9F},
	{0x0381, 0x01},
	{0x0383, 0x03},
	{0x0385, 0x01},
	{0x0387, 0x03},
	{0x0401, 0x00},
	{0x0405, 0x10},
	{0x0700, 0x05},
	{0x0701, 0x30},
	{0x034C, 0x06},
	{0x034D, 0x68},
	{0x034E, 0x04},
	{0x034F, 0xD0},

	
	{0x0200, 0x02},
	{0x0201, 0x50},
	{0x0202, 0x04},
	{0x0203, 0xDB},
	{0x0204, 0x00},
	{0x0205, 0x20},
	{0x0342, 0x0D},
	{0x0343, 0x8E},
	{0x0340, 0x04},
	{0x0341, 0xE0},

	
	{0x300E, 0x2D},
	{0x31A3, 0x40},
	{0x301A, 0x77},
	{0x3053, 0xCB},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_video_settings[] = {

#ifdef CONFIG_MIPI_798MBPS
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0x85},
	{0x0303, 0x01},
	{0x0301, 0x05},
	{0x030B, 0x01},
	{0x0309, 0x05},
	{0x30CC, 0xA0},
	{0x31A1, 0x56},
#else
	
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0x66},
	{0x0303, 0x01},
	{0x0301, 0x05},
	{0x030B, 0x01},
	{0x0309, 0x05},
	{0x30CC, 0xA0},
	{0x31A1, 0x56},
#endif

	
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x00},
	{0x0348, 0x0C},
	{0x0349, 0xCD},
	{0x034A, 0x09},
	{0x034B, 0x9F},
	{0x0381, 0x01},
	{0x0383, 0x03},
	{0x0385, 0x01},
	{0x0387, 0x03},
	{0x0401, 0x00},
	{0x0405, 0x10},
	{0x0700, 0x05},
	{0x0701, 0x30},
	{0x034C, 0x06},	
	{0x034D, 0x68},
	{0x034E, 0x04},	
	{0x034F, 0xD0},

	
	{0x0200, 0x02},
	{0x0201, 0x50},
	{0x0202, 0x05},
	{0x0203, 0xED},
	{0x0204, 0x00},
	{0x0205, 0x20},
	{0x0342, 0x0D},	
	{0x0343, 0x8E},
	{0x0340, 0x05}, 
	{0x0341, 0xFD},

	
	{0x300E, 0x2D},
	{0x31A3, 0x40},
	{0x301A, 0x77},
	{0x3053, 0xCB},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_fast_video_settings[] = {
{0x0305, 0x04},	
{0x0306, 0x00},	
{0x0307, 0x85},	
{0x0303, 0x01},	
{0x0301, 0x05},	
{0x030B, 0x01},	
{0x0309, 0x05},	
{0x30CC, 0xD0},	
{0x31A1, 0x59},	

{0x0344, 0x00},	
{0x0345, 0x00},
{0x0346, 0x01},	
{0x0347, 0x38},
{0x0348, 0x0C},	
{0x0349, 0xCD},
{0x034A, 0x08},	
{0x034B, 0x67},

{0x0381, 0x01},	
{0x0383, 0x03},	
{0x0385, 0x01},	
{0x0387, 0x03},		

{0x0401, 0x00},	
{0x0405, 0x10},
{0x0700, 0x05},	
{0x0701, 0x30},

{0x034C, 0x06},	
{0x034D, 0x68},
{0x034E, 0x03},	
{0x034F, 0x98},

{0x0200, 0x02},	
{0x0201, 0x50},
{0x0202, 0x03},	
{0x0203, 0x60},
{0x0204, 0x00},	
{0x0205, 0x20},
{0x0342, 0x0D},	
{0x0343, 0x8E},
#ifdef CONFIG_RAWCHIP
  {0x0340, 0x03},	
  {0x0341, 0xBC},
#else
  {0x0340, 0x03},	
  {0x0341, 0x98},
#endif

{0x300E, 0x2D},
{0x31A3, 0x40},
{0x301A, 0xA7},
{0x3053, 0xCB}, 
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_snap_settings[] = {
#ifdef CONFIG_MIPI_798MBPS
	
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0x85},
	{0x0303, 0x01},
	{0x0301, 0x05},
	{0x030B, 0x01},
	{0x0309, 0x05},
	{0x30CC, 0xA0},
	{0x31A1, 0x56},
#else
	
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0x66},
	{0x0303, 0x01},
	{0x0301, 0x05},
	{0x030B, 0x01},
	{0x0309, 0x05},
	{0x30CC, 0xA0},
	{0x31A1, 0x56},
#endif

	
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x00},
	{0x0348, 0x0C},
	{0x0349, 0xCF},
	{0x034A, 0x09},
	{0x034B, 0x9F},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0401, 0x00},
	{0x0405, 0x10},
	{0x0700, 0x05},
	{0x0701, 0x30},
	{0x034C, 0x0C},
	{0x034D, 0xD0},
	{0x034E, 0x09},	
	{0x034F, 0xA0},

	
	{0x0200, 0x02},
	{0x0201, 0x50},
	{0x0202, 0x04},
	{0x0203, 0xE7},
	{0x0204, 0x00},
	{0x0205, 0x20},
	{0x0342, 0x0D},
	{0x0343, 0x8E},
	{0x0340, 0x09},		
	{0x0341, 0xC4},

	
	{0x300E, 0x29},
	{0x31A3, 0x00},
	{0x301A, 0x77},
	{0x3053, 0xCF},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_4_3_settings[] = {
};
static struct msm_camera_i2c_reg_conf s5k3h2yx_snap_wide_settings[] = {

	{0x0305, 0x04},	
	{0x0306, 0x00},	
	{0x0307, 0x85},	
	{0x0303, 0x01},	
	{0x0301, 0x05},	
	{0x030B, 0x01},	
	{0x0309, 0x05},	
	{0x30CC, 0xA0},	
	{0x31A1, 0x56},	

	{0x0344, 0x00},	
	{0x0345, 0x00},
	{0x0346, 0x01},	
	{0x0347, 0x30},
	{0x0348, 0x0C},	
	{0x0349, 0xCF},
	{0x034A, 0x08},	
	{0x034B, 0x6F},

	{0x0381, 0x01},	
	{0x0383, 0x01},	
	{0x0385, 0x01},	
	{0x0387, 0x01},	

	{0x0401, 0x00},	
	{0x0405, 0x10},
	{0x0700, 0x05},	
	{0x0701, 0x30},

	{0x034C, 0x0C},	
	{0x034D, 0xD0},
	{0x034E, 0x07},	
	{0x034F, 0x40},

	{0x0200, 0x02},	
	{0x0201, 0x50},
	{0x0202, 0x04},	
	{0x0203, 0xE7},
	{0x0204, 0x00},	
	{0x0205, 0x20},
	{0x0342, 0x0D},	
	{0x0343, 0x8E},
#ifdef CONFIG_RAWCHIP
	{0x0340, 0x07},	
	{0x0341, 0x64},
#else
	{0x0340, 0x07},	
	{0x0341, 0x40},
#endif

	{0x300E, 0x29},
	{0x31A3, 0x00},
	{0x301A, 0x77},
	{0x3053, 0xCF},	
};
static struct msm_camera_i2c_reg_conf s5k3h2yx_video_hfr_5_3_settings[] = {
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_5_3_settings[] = {

#ifdef CONFIG_MIPI_798MBPS
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0x85},
	{0x0303, 0x01},
	{0x0301, 0x05},
	{0x030B, 0x01},
	{0x0309, 0x05},
	{0x30CC, 0xA0},
	{0x31A1, 0x56},
#else
	
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0x66},
	{0x0303, 0x01},
	{0x0301, 0x05},
	{0x030B, 0x01},
	{0x0309, 0x05},
	{0x30CC, 0xA0},
	{0x31A1, 0x56},
#endif

	
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0xFC},
	{0x0348, 0x0C},
	{0x0349, 0xCF},
	{0x034A, 0x08},
	{0x034B, 0xAB},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0401, 0x00},
	{0x0405, 0x10},
	{0x0700, 0x05},
	{0x0701, 0x30},
	{0x034C, 0x0C},	
	{0x034D, 0xD0},
	{0x034E, 0x07},	
	{0x034F, 0xB0},

	
	{0x0200, 0x02},
	{0x0201, 0x50},
	{0x0202, 0x04},
	{0x0203, 0xE7},
	{0x0204, 0x00},
	{0x0205, 0x20},
	{0x0342, 0x0D},	
	{0x0343, 0x8E},
	{0x0340, 0x07},
	{0x0341, 0xD0},

	
	{0x300E, 0x29},
	{0x31A3, 0x00},
	{0x301A, 0x77},
	{0x3053, 0xCF},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_video_24fps_16_9_settings[] = {
	
	{0x0305, 0x04},	
	{0x0306, 0x00},	
	{0x0307, 0x85},	
	{0x0303, 0x01},	
	{0x0301, 0x05},	
	{0x030B, 0x01},	
	{0x0309, 0x05},	
	{0x30CC, 0xD0},	
	{0x31A1, 0x59},	

	{0x0344, 0x00},	
	{0x0345, 0x00},
	{0x0346, 0x01},	
	{0x0347, 0x38},
	{0x0348, 0x0C},	
	{0x0349, 0xCD},
	{0x034A, 0x08},	
	{0x034B, 0x67},

	{0x0381, 0x01},	
	{0x0383, 0x03},	
	{0x0385, 0x01},	
	{0x0387, 0x03},		

	{0x0401, 0x00},	
	{0x0405, 0x10},
	{0x0700, 0x05},	
	{0x0701, 0x30},

	{0x034C, 0x06},	
	{0x034D, 0x68},
	{0x034E, 0x03},	
	{0x034F, 0x98},

	{0x0200, 0x02},	
	{0x0201, 0x50},
	{0x0202, 0x03},	
	{0x0203, 0x60},
	{0x0204, 0x00},	
	{0x0205, 0x20},
	{0x0342, 0x0D},	
	{0x0343, 0x8E},
#ifdef CONFIG_RAWCHIP
	  {0x0340, 0x07},	
	  {0x0341, 0x7E},
#else
	  {0x0340, 0x03},	
	  {0x0341, 0x98},
#endif

	{0x300E, 0x2D},
	{0x31A3, 0x40},
	{0x301A, 0xA7},
	{0x3053, 0xCB}, 
};
static struct msm_camera_i2c_reg_conf s5k3h2yx_video_60fps_settings[] = {
};
static struct msm_camera_i2c_reg_conf s5k3h2yx_video_24fps_5_3_settings[] = {

#ifdef CONFIG_MIPI_798MBPS
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0x85},
	{0x0303, 0x01},
	{0x0301, 0x05},
	{0x030B, 0x01},
	{0x0309, 0x05},
	{0x30CC, 0xD0},
	{0x31A1, 0x59},
#else
	
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0x66},
	{0x0303, 0x01},
	{0x0301, 0x05},
	{0x030B, 0x01},
	{0x0309, 0x05},
	{0x30CC, 0xA0},
	{0x31A1, 0x56},
#endif

	
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0xFC},
	{0x0348, 0x0C},
	{0x0349, 0xCD},
	{0x034A, 0x08},
	{0x034B, 0xAB},
	{0x0381, 0x01},
	{0x0383, 0x03},
	{0x0385, 0x01},
	{0x0387, 0x03},
	{0x0401, 0x00},
	{0x0405, 0x10},
	{0x0700, 0x05},
	{0x0701, 0x30},
	{0x034C, 0x06},	
	{0x034D, 0x68},
	{0x034E, 0x03},	
	{0x034F, 0xD8},

	
	{0x0200, 0x02},
	{0x0201, 0x50},
	{0x0202, 0x03},
	{0x0203, 0x60},
	{0x0204, 0x00},
	{0x0205, 0x20},
	{0x0342, 0x0D},	
	{0x0343, 0x8E},
	{0x0340, 0x07},
	{0x0341, 0x7C},

	
	{0x300E, 0x2D},
	{0x31A3, 0x40},
	{0x301A, 0xA7},
	{0x3053, 0xCB},
};
static struct msm_camera_i2c_reg_conf s5k3h2yx_video_16_9_settings[] = {
	
	{0x0305, 0x04},	
	{0x0306, 0x00},	
	{0x0307, 0x85},	
	{0x0303, 0x01},	
	{0x0301, 0x05},	
	{0x030B, 0x01},	
	{0x0309, 0x05},	
	{0x30CC, 0xD0},	
	{0x31A1, 0x59},	

	{0x0344, 0x00},	
	{0x0345, 0x00},
	{0x0346, 0x01},	
	{0x0347, 0x38},
	{0x0348, 0x0C},	
	{0x0349, 0xCD},
	{0x034A, 0x08},	
	{0x034B, 0x67},

	{0x0381, 0x01},	
	{0x0383, 0x03},	
	{0x0385, 0x01},	
	{0x0387, 0x03},		

	{0x0401, 0x00},	
	{0x0405, 0x10},
	{0x0700, 0x05},	
	{0x0701, 0x30},

	{0x034C, 0x06},	
	{0x034D, 0x68},
	{0x034E, 0x03},	
	{0x034F, 0x98},

	{0x0200, 0x02},	
	{0x0201, 0x50},
	{0x0202, 0x03},	
	{0x0203, 0x60},
	{0x0204, 0x00},	
	{0x0205, 0x20},
	{0x0342, 0x0D},	
	{0x0343, 0x8E},
#ifdef CONFIG_RAWCHIP
	  {0x0340, 0x05},	
	  {0x0341, 0xFE},
#else
	  {0x0340, 0x03},	
	  {0x0341, 0x98},
#endif

	{0x300E, 0x2D},
	{0x31A3, 0x40},
	{0x301A, 0xA7},
	{0x3053, 0xCB}, 
};
static struct msm_camera_i2c_reg_conf s5k3h2yx_video_5_3_settings[] = {

#ifdef CONFIG_MIPI_798MBPS
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0x85},
	{0x0303, 0x01},
	{0x0301, 0x05},
	{0x030B, 0x01},
	{0x0309, 0x05},
	{0x30CC, 0xD0},
	{0x31A1, 0x59},
#else
	
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0x66},
	{0x0303, 0x01},
	{0x0301, 0x05},
	{0x030B, 0x01},
	{0x0309, 0x05},
	{0x30CC, 0xA0},
	{0x31A1, 0x56},
#endif

	
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0xFC},
	{0x0348, 0x0C},
	{0x0349, 0xCD},
	{0x034A, 0x08},
	{0x034B, 0xAB},
	{0x0381, 0x01},
	{0x0383, 0x03},
	{0x0385, 0x01},
	{0x0387, 0x03},
	{0x0401, 0x00},
	{0x0405, 0x10},
	{0x0700, 0x05},
	{0x0701, 0x30},
	{0x034C, 0x06},	
	{0x034D, 0x68},
	{0x034E, 0x03},	
	{0x034F, 0xD8},

	
	{0x0200, 0x02},
	{0x0201, 0x50},
	{0x0202, 0x03},
	{0x0203, 0x60},
	{0x0204, 0x00},
	{0x0205, 0x20},
	{0x0342, 0x0D},	
	{0x0343, 0x8E},
	{0x0340, 0x05},
	{0x0341, 0xFD},

	
	{0x300E, 0x2D},
	{0x31A3, 0x40},
	{0x301A, 0xA7},
	{0x3053, 0xCB},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_recommend_settings[] = {
	{0x3000, 0x08},
	{0x3001, 0x05},
	{0x3002, 0x0D},
	{0x3003, 0x21},
	{0x3004, 0x62},
	{0x3005, 0x0B},
	{0x3006, 0x6D},
	{0x3007, 0x02},
	{0x3008, 0x62},
	{0x3009, 0x62},
	{0x300A, 0x41},
	{0x300B, 0x10},
	{0x300C, 0x21},
	{0x300D, 0x04},
	{0x307E, 0x03},
	{0x307F, 0xA5},
	{0x3080, 0x04},
	{0x3081, 0x29},
	{0x3082, 0x03},
	{0x3083, 0x21},
	{0x3011, 0x5F},
	{0x3156, 0xE2},
	{0x3027, 0xBE},
	{0x300f, 0x02},
	{0x3010, 0x10},
	{0x3017, 0x74},
	{0x3018, 0x00},
	{0x3020, 0x02},
	{0x3021, 0x00},
	{0x3023, 0x80},
	{0x3024, 0x08},
	{0x3025, 0x08},
	{0x301C, 0xD4},
	{0x315D, 0x00},
	
	{0x3054, 0x00},
	{0x3055, 0x35},
	{0x3062, 0x04},
	{0x3063, 0x38},
	{0x31A4, 0x04},
	{0x3016, 0x54},
	{0x3157, 0x02},
	{0x3158, 0x00},
	{0x315B, 0x02},
	{0x315C, 0x00},
	{0x301B, 0x05},
	{0x3028, 0x41},
	{0x302A, 0x10},
	{0x3060, 0x00},
	{0x302D, 0x19},
	{0x302B, 0x05},
	{0x3072, 0x13},
	{0x3073, 0x21},
	{0x3074, 0x82},
	{0x3075, 0x20},
	{0x3076, 0xA2},
	{0x3077, 0x02},
	{0x3078, 0x91},
	{0x3079, 0x91},
	{0x307A, 0x61},
	{0x307B, 0x28},
	{0x307C, 0x31},
	
	{0x304E, 0x40},
	{0x304F, 0x01},
	{0x3050, 0x00},
	{0x3088, 0x01},
	{0x3089, 0x00},
	{0x3210, 0x01},
	{0x3211, 0x00},
	{0x308E, 0x01},
	{0x308F, 0x8F},
	{0x3064, 0x03},
	{0x31A7, 0x0F},
};

static struct v4l2_subdev_info s5k3h2yx_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	
};

static struct msm_camera_i2c_conf_array s5k3h2yx_init_conf[] = {
	{&s5k3h2yx_mipi_settings[0],
	ARRAY_SIZE(s5k3h2yx_mipi_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_recommend_settings[0],
	ARRAY_SIZE(s5k3h2yx_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_pll_settings[0],
	ARRAY_SIZE(s5k3h2yx_pll_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_conf_array s5k3h2yx_confs[] = {
	{&s5k3h2yx_snap_settings[0],
	ARRAY_SIZE(s5k3h2yx_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_prev_settings[0],
	ARRAY_SIZE(s5k3h2yx_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_video_settings[0],
	ARRAY_SIZE(s5k3h2yx_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_fast_video_settings[0],
	ARRAY_SIZE(s5k3h2yx_fast_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_snap_wide_settings[0],
	ARRAY_SIZE(s5k3h2yx_snap_wide_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_4_3_settings[0],
	ARRAY_SIZE(s5k3h2yx_4_3_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_video_hfr_5_3_settings[0],
	ARRAY_SIZE(s5k3h2yx_video_hfr_5_3_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_5_3_settings[0],
	ARRAY_SIZE(s5k3h2yx_5_3_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_video_24fps_16_9_settings[0],
	ARRAY_SIZE(s5k3h2yx_video_24fps_16_9_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_video_60fps_settings[0],
	ARRAY_SIZE(s5k3h2yx_video_60fps_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_video_24fps_5_3_settings[0],
	ARRAY_SIZE(s5k3h2yx_video_24fps_5_3_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_video_16_9_settings[0],
	ARRAY_SIZE(s5k3h2yx_video_16_9_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_video_5_3_settings[0],
	ARRAY_SIZE(s5k3h2yx_video_5_3_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};
static struct msm_sensor_output_info_t s5k3h2yx_dimensions[] = {
	
	{
		.x_output = 0xCD0,	
		.y_output = 0x9A0,	
		.line_length_pclk = 0xD8E,
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
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x600,
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
		.y_output = 0x4D0,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x5FD,
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
		.y_output = 0x398,	
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x3BC,
#else
		.frame_length_lines = 0x398,
#endif
		.vt_pixel_clk = 159600000,
		.op_pixel_clk = 159600000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0x138,
		.x_addr_end = 0xCCD,
		.y_addr_end = 0x867,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
	{
		.x_output = 0xCD0,	
		.y_output = 0x740,	
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x764,
#else
		.frame_length_lines = 0x740,
#endif
		.vt_pixel_clk = 159600000,
		.op_pixel_clk = 159600000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0x0130,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x86F,
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
		.binning_rawchip = 0x22,
	},
	{
		.x_output = 0xCD0,	
		.y_output = 0x7B0,	
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x7D0,
#else
		.frame_length_lines = 0x7C0,
#endif
		.vt_pixel_clk = 159600000,
		.op_pixel_clk = 159600000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0x0FC,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x8AB,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	
	{
		.x_output = 0x668,
		.y_output = 0x398,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x77E,	
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
		.y_addr_start = 0xFC,
		.x_addr_end = 0xCCF,	
		.y_addr_end = 0x8AB,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
	
	{
		.x_output = 0x668,	
		.y_output = 0x398,	
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x5FE,
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
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x5FD,
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
		.y_addr_start = 0xFC,
		.x_addr_end = 0xCCF,	
		.y_addr_end = 0x8AB,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
};

#if 0
static struct msm_camera_csid_vc_cfg s5k3h2yx_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params s5k3h2yx_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 2,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = s5k3h2yx_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 2,
		.settle_cnt = 0x1B,
	},
};
#else
static struct msm_camera_csi_params s5k3h2yx_csi_params = {
	.data_format = CSI_10BIT,
	.lane_cnt    = 2,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 0x2a,
	.dt = CSI_RAW10,
};
#endif

static struct msm_camera_csi_params *s5k3h2yx_csi_params_array[] = {
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
};

static struct msm_sensor_output_reg_addr_t s5k3h2yx_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static struct msm_sensor_id_info_t s5k3h2yx_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x382B,
};

static struct msm_sensor_exp_gain_info_t s5k3h2yx_exp_gain_info = {
	.coarse_int_time_addr = 0x202,
	.global_gain_addr = 0x204,
	.vert_offset = 16,
	.min_vert = 4,  
	.sensor_max_linecount = 65519,  
};

static int s5k3h2yx_read_fuseid(struct sensor_cfg_data *cdata,
	struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t  rc;
	int page = 0;
	unsigned short info_value = 0, info_index = 0;
	unsigned short  OTP[10] = {0};

	struct msm_camera_i2c_client *s5k3h2yx_msm_camera_i2c_client = s_ctrl->sensor_i2c_client;

	pr_info("%s: sensor OTP information:\n", __func__);

	
	rc = msm_camera_i2c_write_b(s5k3h2yx_msm_camera_i2c_client, 0x3A1C, 0x00);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x3A1C fail\n", __func__);

	
	rc = msm_camera_i2c_write_b(s5k3h2yx_msm_camera_i2c_client, 0x0A00, 0x04);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x0A00 (Start) fail\n", __func__);

	mdelay(4);

	
	for (info_index = 0; info_index < 10; info_index++) {
		for (page = 20; page >= 16; page--) {
			rc = msm_camera_i2c_write_b(s5k3h2yx_msm_camera_i2c_client, 0x0A02, page);
			if (rc < 0)
				pr_err("%s: i2c_write_b 0x0A02 (select page %d) fail\n", __func__, page);

			
			rc = msm_camera_i2c_write_b(s5k3h2yx_msm_camera_i2c_client, 0x0A00, 0x01);
			if (rc < 0)
				pr_err("%s: i2c_write_b 0x0A00: Set read mode fail\n", __func__);

			
			rc = msm_camera_i2c_read_b(s5k3h2yx_msm_camera_i2c_client, (0x0A04 + info_index), &info_value);
			if (rc < 0)
				pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A04 + info_index));

			 
			if (((info_value&0x0F) != 0) || page == 0)
				break;
		}
		OTP[info_index] = (short)(info_value&0x0F);
		info_value = 0;
	}

	if (OTP[0] != 0 && OTP[1] != 0) {
		pr_info("Get Fuseid from Page20 to Page16\n");
		goto get_done;
	}

	
	memset(OTP, 0, sizeof(OTP));
	for (info_index = 0; info_index < 10; info_index++) {
		for (page = 4; page >= 0; page--) {
			rc = msm_camera_i2c_write_b(s5k3h2yx_msm_camera_i2c_client, 0x0A02, page);
			if (rc < 0)
				pr_err("%s: i2c_write_b 0x0A02 (select page %d) fail\n", __func__, page);

			
			rc = msm_camera_i2c_write_b(s5k3h2yx_msm_camera_i2c_client, 0x0A00, 0x01);
			if (rc < 0)
				pr_err("%s: i2c_write_b 0x0A00: Set read mode fail\n", __func__);

			
			rc = msm_camera_i2c_read_b(s5k3h2yx_msm_camera_i2c_client, (0x0A04 + info_index), &info_value);
			if (rc < 0)
				pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A04 + info_index));

			 
			if (((info_value & 0x0F) != 0) || page == 0)
				break;
		}
		OTP[info_index] = (short)(info_value&0x0F);
		info_value = 0;
	}

get_done:
	
	rc = msm_camera_i2c_write_b(s5k3h2yx_msm_camera_i2c_client, 0x0A00, 0x00);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x0A00 (Stop) fail\n", __func__);

	pr_info("[CAM]%s: VenderID=%x,LensID=%x,SensorID=%x%x\n", __func__,
		OTP[0], OTP[1], OTP[2], OTP[3]);
	pr_info("[CAM]%s: ModuleFuseID= %x%x%x%x%x%x\n", __func__,
		OTP[4], OTP[5], OTP[6], OTP[7], OTP[8], OTP[9]);

    cdata->cfg.fuse.fuse_id_word1 = 0;
    cdata->cfg.fuse.fuse_id_word2 = 0;
	cdata->cfg.fuse.fuse_id_word3 = (OTP[0]);
	cdata->cfg.fuse.fuse_id_word4 =
		(OTP[4]<<20) |
		(OTP[5]<<16) |
		(OTP[6]<<12) |
		(OTP[7]<<8) |
		(OTP[8]<<4) |
		(OTP[9]);

	pr_info("[CAM]s5k3h2yx: fuse->fuse_id_word1:%d\n",
		cdata->cfg.fuse.fuse_id_word1);
	pr_info("[CAM]s5k3h2yx: fuse->fuse_id_word2:%d\n",
		cdata->cfg.fuse.fuse_id_word2);
	pr_info("[CAM]s5k3h2yx: fuse->fuse_id_word3:0x%08x\n",
		cdata->cfg.fuse.fuse_id_word3);
	pr_info("[CAM]s5k3h2yx: fuse->fuse_id_word4:0x%08x\n",
		cdata->cfg.fuse.fuse_id_word4);
	return 0;
}

static int s5k3h2yx_mirror_flip_setting(void)
{
	int rc = 0;
	uint16_t value = 0;

	pr_info("[CAM] %s\n", __func__);

	
	if (s5k3h2yx_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR_FLIP)
		value = S5K3H2YX_READ_MIRROR_FLIP;
	else if (s5k3h2yx_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR)
		value = S5K3H2YX_READ_MIRROR;
	else if (s5k3h2yx_s_ctrl.mirror_flip == CAMERA_SENSOR_FLIP)
		value = S5K3H2YX_READ_FLIP;
	else
		value = S5K3H2YX_READ_NORMAL_MODE;

	msm_camera_i2c_write(s5k3h2yx_s_ctrl.sensor_i2c_client,
		S5K3H2YX_REG_READ_MODE, value, MSM_CAMERA_I2C_BYTE_DATA);

	return rc;
}

static int s5k3h2yx_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	pr_info("[CAM] %s\n", __func__);
	
	if (data->sensor_platform_info)
		s5k3h2yx_s_ctrl.mirror_flip = data->sensor_platform_info->mirror_flip;
	

	return rc;
}

static const char *s5k3h2yxVendor = "samsung";
static const char *s5k3h2yxNAME = "s5k3h2yx";
static const char *s5k3h2yxSize = "8M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", s5k3h2yxVendor, s5k3h2yxNAME, s5k3h2yxSize);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);

static struct kobject *android_s5k3h2yx;

static int s5k3h2yx_sysfs_init(void)
{
	int ret ;
	pr_info("s5k3h2yx:kobject creat and add\n");
	android_s5k3h2yx = kobject_create_and_add("android_camera", NULL);
	if (android_s5k3h2yx == NULL) {
		pr_info("s5k3h2yx_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("s5k3h2yx:sysfs_create_file\n");
	ret = sysfs_create_file(android_s5k3h2yx, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("s5k3h2yx_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_s5k3h2yx);
	}

	return 0 ;
}

int32_t s5k3h2yx_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	rc = 0;
	pr_info("[CAM] %s recovery version:3.0\n", __func__);
	rc = msm_sensor_i2c_probe(client, id);
	if(rc >= 0)
		s5k3h2yx_sysfs_init();
	pr_info("[CAM] %s: rc(%d)\n", __func__, rc);
	return rc;
}

static const struct i2c_device_id s5k3h2yx_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&s5k3h2yx_s_ctrl},
	{ }
};

static struct i2c_driver s5k3h2yx_i2c_driver = {
	.id_table = s5k3h2yx_i2c_id,
	.probe  = s5k3h2yx_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client s5k3h2yx_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};


int32_t s5k3h2yx_power_up(struct msm_sensor_ctrl_t *s_ctrl)
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

	s5k3h2yx_sensor_open_init(sdata);
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

int32_t s5k3h2yx_power_down(struct msm_sensor_ctrl_t *s_ctrl)
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
static int s5k3h2yx_probe(struct platform_device *pdev)
{
	int	rc = 0;

	pr_info("%s\n", __func__);

	rc = msm_sensor_register(pdev, s5k3h2yx_sensor_v4l2_probe);
	if(rc >= 0)
		s5k3h2yx_sysfs_init();
	return rc;
}

struct platform_driver s5k3h2yx_driver = {
	.probe = s5k3h2yx_probe,
	.driver = {
		.name = PLATFORM_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init msm_sensor_init_module(void)
{
	pr_info("%s\n", __func__);
	return platform_driver_register(&s5k3h2yx_driver);
}
#else
static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&s5k3h2yx_i2c_driver);
}
#endif

static struct v4l2_subdev_core_ops s5k3h2yx_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops s5k3h2yx_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops s5k3h2yx_subdev_ops = {
	.core = &s5k3h2yx_subdev_core_ops,
	.video  = &s5k3h2yx_subdev_video_ops,
};

int32_t s5k3h2yx_write_exp_gain1_ex(struct msm_sensor_ctrl_t *s_ctrl,
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
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}

int32_t s5k3h2yx_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res) {

	int rc = 0;
	pr_info("[CAM] %s\n", __func__);

#if ((defined CONFIG_I2C_CPLD) && ((defined CONFIG_MACH_CP3DCG) || (defined CONFIG_MACH_CP3DUG) || (defined CONFIG_MACH_CP3DTG) || (defined CONFIG_MACH_CP3U)))
	rc = msm_sensor_setting_parallel1(s_ctrl, update_type, res);
#else
	rc = msm_sensor_setting1(s_ctrl, update_type, res);
#endif

	if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		s5k3h2yx_mirror_flip_setting();
	}

	return rc;
}


static struct msm_sensor_fn_t s5k3h2yx_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_write_exp_gain_ex = s5k3h2yx_write_exp_gain1_ex,
	.sensor_write_snapshot_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_write_snapshot_exp_gain_ex = s5k3h2yx_write_exp_gain1_ex,
#if 0
	.sensor_setting = msm_sensor_setting,
#else
	.sensor_csi_setting = s5k3h2yx_sensor_setting,
#endif
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_power_up = s5k3h2yx_power_up,
	.sensor_power_down = s5k3h2yx_power_down,
	.sensor_config = msm_sensor_config,
	.sensor_get_csi_params = msm_sensor_get_csi_params,
	.sensor_i2c_read_fuseid = s5k3h2yx_read_fuseid,
};

static struct msm_sensor_reg_t s5k3h2yx_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = s5k3h2yx_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(s5k3h2yx_start_settings),
	.stop_stream_conf = s5k3h2yx_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(s5k3h2yx_stop_settings),
	.group_hold_on_conf = s5k3h2yx_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(s5k3h2yx_groupon_settings),
	.group_hold_off_conf = s5k3h2yx_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(s5k3h2yx_groupoff_settings),
	.init_settings = &s5k3h2yx_init_conf[0],
	.init_size = ARRAY_SIZE(s5k3h2yx_init_conf),
	.mode_settings = &s5k3h2yx_confs[0],
	.output_settings = &s5k3h2yx_dimensions[0],
	.num_conf = ARRAY_SIZE(s5k3h2yx_confs),
};

static struct msm_sensor_ctrl_t s5k3h2yx_s_ctrl = {
	.msm_sensor_reg = &s5k3h2yx_regs,
	.sensor_i2c_client = &s5k3h2yx_sensor_i2c_client,
	.sensor_i2c_addr = 0x20,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_output_reg_addr = &s5k3h2yx_reg_addr,
	.sensor_id_info = &s5k3h2yx_id_info,
	.sensor_exp_gain_info = &s5k3h2yx_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
#if 0
	.csi_params = &s5k3h2yx_csi_params_array[0],
#else
	.csic_params = &s5k3h2yx_csi_params_array[0],
#endif
	.msm_sensor_mutex = &s5k3h2yx_mut,
	.sensor_i2c_driver = &s5k3h2yx_i2c_driver,
	.sensor_v4l2_subdev_info = s5k3h2yx_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k3h2yx_subdev_info),
	.sensor_v4l2_subdev_ops = &s5k3h2yx_subdev_ops,
	.func_tbl = &s5k3h2yx_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
	.sensor_first_mutex = &s5k3h2yx_sensor_init_mut,
};

#ifdef CONFIG_DEBUG_FRAME_COUNT
void s5k3h2yx_check_frame_count(void)
{

	int32_t  rc;
	unsigned short info_value = 0;

	if (s5k3h2yx_msm_camera_i2c_client_checkstatus == NULL)
	   return;

	rc = msm_camera_i2c_read_b(s5k3h2yx_msm_camera_i2c_client_checkstatus, 0x0005, &info_value);
	if (rc < 0)
		pr_err("%s: i2c_read_b 0x0005 fail\n", __func__);
	else
		pr_info("%s sensor frame count = %d", __func__, info_value);

}

EXPORT_SYMBOL(s5k3h2yx_check_frame_count);
#endif


module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Samsung 8 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
