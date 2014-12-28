#include "msm_sensor.h"

#ifdef CONFIG_RAWCHIP
#include "rawchip/rawchip.h"
#endif

#define SENSOR_NAME "ov5693"
#define PLATFORM_DRIVER_NAME "msm_camera_ov5693"
#define ov5693_obj ov5693_##obj


#define OV5693_FLIP          0x42             
#define OV5693_FLIP_NORMAL   (~OV5693_FLIP)   
#define OV5693_MIRROR        0x06             
#define OV5693_MIRROR_NORMAL (~OV5693_MIRROR) 

#define OV5693_REG_FLIP 0x3820
#define OV5693_REG_MIRROR 0x3821

DEFINE_MUTEX(ov5693_mut);
static struct msm_sensor_ctrl_t ov5693_s_ctrl;

static struct msm_camera_i2c_reg_conf ov5693_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf ov5693_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf ov5693_groupon_settings[] = {
	{0x3208, 0x00},

};

static struct msm_camera_i2c_reg_conf ov5693_groupoff_settings[] = {
	{0x3208, 0x10},
	{0x3208, 0xA0},
};

static struct msm_camera_i2c_reg_conf ov5693_snap_settings[] = {
	
	{0x3708, 0xe2}, 

	
	{0x3800, 0x00}, 
	{0x3801, 0x00}, 
	{0x3802, 0x00}, 
	{0x3803, 0x00}, 

	{0x3804, 0x0a}, 
	{0x3805, 0x3f}, 
	{0x3806, 0x07}, 
	{0x3807, 0xa3}, 

	{0x3808, 0x0a}, 
	{0x3809, 0x20}, 

	{0x380a, 0x07}, 
	{0x380b, 0xa0}, 

	{0x380c, 0x0a}, 
	{0x380d, 0x84}, 
	{0x380e, 0x07}, 
	{0x380f, 0xbc}, 

	{0x3810, 0x00}, 
	{0x3811, 0x02}, 
	{0x3812, 0x00}, 
	{0x3813, 0x02}, 

	{0x3814, 0x11}, 
	{0x3815, 0x11}, 
#if 1 
	{0x3820, 0x00}, 
	{0x3821, 0x18}, 
#endif
	{0x3823, 0x00}, 
	{0x3824, 0x00}, 
	{0x3825, 0x00}, 
	{0x3826, 0x00}, 
	{0x3827, 0x00}, 
	{0x382a, 0x04}, 

	
	{0x4004, 0x08}, 

	{0x4511, 0x05}, 
	{0x4512, 0x01}, 
};


static struct msm_camera_i2c_reg_conf ov5693_prev_settings[] = {
	
	{0x3708, 0xe2}, 

	
	{0x3800, 0x00}, 
	{0x3801, 0x00}, 
	{0x3802, 0x00}, 
	{0x3803, 0x00}, 

	{0x3804, 0x0a}, 
	{0x3805, 0x3f}, 
	{0x3806, 0x07}, 
	{0x3807, 0xa3}, 

	{0x3808, 0x0a}, 
	{0x3809, 0x20}, 

	{0x380a, 0x07}, 
	{0x380b, 0xa0}, 

	{0x380c, 0x0a}, 
	{0x380d, 0x84}, 
	{0x380e, 0x07}, 
	{0x380f, 0xbc}, 

	{0x3810, 0x00}, 
	{0x3811, 0x02}, 
	{0x3812, 0x00}, 
	{0x3813, 0x02}, 

	{0x3814, 0x11}, 
	{0x3815, 0x11}, 
#if 1 
	{0x3820, 0x00}, 
	{0x3821, 0x18}, 
#endif
	{0x3823, 0x00}, 
	{0x3824, 0x00}, 
	{0x3825, 0x00}, 
	{0x3826, 0x00}, 
	{0x3827, 0x00}, 
	{0x382a, 0x04}, 

	
	{0x4004, 0x08}, 

	{0x4511, 0x05}, 
	{0x4512, 0x01}, 
};

static struct msm_camera_i2c_reg_conf ov5693_video_settings[] = {
	
	{0x3708, 0xe6}, 

	
	{0x3800, 0x00}, 
	{0x3801, 0x00}, 
	{0x3802, 0x00}, 
	{0x3803, 0x00}, 

	{0x3804, 0x0a}, 
	{0x3805, 0x3f}, 
	{0x3806, 0x07}, 
	{0x3807, 0xa3}, 

	{0x3808, 0x05}, 
	{0x3809, 0x10}, 

	{0x380a, 0x03}, 
	{0x380b, 0xcc}, 

	{0x380c, 0x0a}, 
	{0x380d, 0x84}, 
	{0x380e, 0x07}, 
	{0x380f, 0xbc}, 

	{0x3810, 0x00}, 
	{0x3811, 0x02}, 
	{0x3812, 0x00}, 
	{0x3813, 0x02}, 

	{0x3814, 0x31}, 
	{0x3815, 0x31}, 
#if 1 
	{0x3820, 0x01}, 
	{0x3821, 0x19}, 
#endif
	{0x3823, 0x00}, 
	{0x3824, 0x00}, 
	{0x3825, 0x00}, 
	{0x3826, 0x00}, 
	{0x3827, 0x00}, 
	{0x382a, 0x04}, 

	
	{0x4004, 0x08}, 

	{0x4511, 0x05}, 
	{0x4512, 0x01}, 
};

static struct msm_camera_i2c_reg_conf ov5693_fast_video_settings[] = {
	
	{0x3708, 0xe2}, 

	
	{0x3800, 0x00}, 
	{0x3801, 0x10}, 
	{0x3802, 0x00}, 
	{0x3803, 0xf6}, 

	{0x3804, 0x0a}, 
	{0x3805, 0x2f}, 
	{0x3806, 0x06}, 
	{0x3807, 0xab}, 

	{0x3808, 0x05}, 
	{0x3809, 0x00}, 

	{0x380a, 0x02}, 
	{0x380b, 0xd0}, 

	{0x380c, 0x06}, 
	{0x380d, 0xd8}, 
	{0x380e, 0x02}, 
	{0x380f, 0xf8}, 

	{0x3810, 0x00}, 
	{0x3811, 0x04}, 
	{0x3812, 0x00}, 
	{0x3813, 0x02}, 

	{0x3814, 0x31}, 
	{0x3815, 0x31}, 
#if 1 
	{0x3820, 0x00}, 
	{0x3821, 0x18}, 
#endif
	{0x3823, 0x00}, 
	{0x3824, 0x00}, 
	{0x3825, 0x00}, 
	{0x3826, 0x00}, 
	{0x3827, 0x00}, 
	{0x382a, 0x04}, 

	
	{0x4004, 0x08}, 

	{0x4511, 0x05}, 
	{0x4512, 0x01}, 
};

static struct msm_camera_i2c_reg_conf ov5693_recommend_settings[] = {
	


	
	{0x0103, 0x01}, 
	{0x3001, 0x0a}, 
	{0x3002, 0x80}, 
	{0x3006, 0x00}, 
	{0x3011, 0x21}, 
	{0x3012, 0x09}, 
	{0x3013, 0x10}, 
	{0x3014, 0x00}, 
	{0x3015, 0x08}, 
	{0x3016, 0xf0}, 
	{0x3017, 0xf0},  
	{0x3018, 0xf0}, 
	{0x301b, 0xb4}, 
	{0x301d, 0x02}, 
	{0x3021, 0x00}, 
	{0x3022, 0x01}, 
	{0x3028, 0x44}, 


	
	{0x3090, 0x02}, 
	{0x3091, 0x0e}, 
	{0x3092, 0x00}, 
	{0x3093, 0x00}, 
	{0x3098, 0x03}, 
	{0x3099, 0x1e}, 
	{0x309a, 0x02}, 
	{0x309b, 0x01}, 
	{0x309c, 0x00}, 
	{0x30a0, 0xd2}, 
	{0x30a2, 0x01}, 
	{0x30b2, 0x00},  
	{0x30b3, 0x64},  
	{0x30b4, 0x03},  
	{0x30b5, 0x04},  
	{0x30b6, 0x01},  
	{0x3104, 0x21}, 
	{0x3106, 0x00}, 

	
	{0x3400, 0x04}, 
	{0x3401, 0x00}, 
	{0x3402, 0x04}, 
	{0x3403, 0x00}, 
	{0x3404, 0x04}, 
	{0x3405, 0x00}, 
	{0x3406, 0x01}, 

	
	{0x3500, 0x00}, 
	{0x3501, 0x7b}, 
	{0x3502, 0x80}, 
#if 1
	{0x3503, 0x07}, 
#else
	{0x3503, 0x20},   
#endif

	{0x3504, 0x00}, 
	{0x3505, 0x00}, 
	{0x3506, 0x00}, 
	{0x3507, 0x02}, 
	{0x3508, 0x00}, 
	{0x3509, 0x10}, 
	{0x350a, 0x00}, 
	{0x350b, 0x10}, 

	
	{0x3601, 0x0a}, 
	{0x3602, 0x18}, 
	{0x3612, 0x80}, 
	{0x3620, 0x54}, 
	{0x3621, 0xc7}, 
	{0x3622, 0x0f}, 
	{0x3625, 0x10}, 
	{0x3630, 0x55}, 
	{0x3631, 0xf4}, 
	{0x3632, 0x00}, 
	{0x3633, 0x34}, 
	{0x3634, 0x02}, 
	{0x364d, 0x0d}, 
	{0x364f, 0xdd}, 
	{0x3660, 0x04}, 
	{0x3662, 0x10}, 
	{0x3663, 0xf1}, 
	{0x3665, 0x00}, 
	{0x3666, 0x20}, 
	{0x3667, 0x00}, 
	{0x366a, 0x80}, 
	{0x3680, 0xe0}, 
	{0x3681, 0x00}, 

	
	{0x3620, 0x44},
	{0x3621, 0xb5},
	{0x3622, 0x0c},
	{0x3600, 0xbc},

	
	{0x3700, 0x42}, 
	{0x3701, 0x14}, 
	{0x3702, 0xa0}, 
	{0x3703, 0xd8}, 
	{0x3704, 0x78}, 
	{0x3705, 0x02}, 
	{0x3708, 0xe2}, 
	{0x3709, 0xc3}, 
	{0x370a, 0x00}, 
	{0x370b, 0x20}, 
	{0x370c, 0x0c}, 
	{0x370d, 0x11}, 
	{0x370e, 0x00}, 
	{0x370f, 0x40}, 
	{0x3710, 0x00}, 
	{0x371a, 0x1c}, 
	{0x371b, 0x05}, 
	{0x371c, 0x01}, 
	{0x371e, 0xa1}, 
	{0x371f, 0x0c}, 
	{0x3721, 0x00}, 
	{0x3724, 0x10}, 
	{0x3726, 0x00}, 
	{0x372a, 0x01}, 
	{0x3730, 0x10}, 
	{0x3738, 0x22}, 
	{0x3739, 0xe5}, 
	{0x373a, 0x50}, 
	{0x373b, 0x02}, 
	{0x373c, 0x41}, 
	{0x373f, 0x02}, 
	{0x3740, 0x42}, 
	{0x3741, 0x02}, 
	{0x3742, 0x18}, 
	{0x3743, 0x01}, 
	{0x3744, 0x02}, 
	{0x3747, 0x10}, 
	{0x374c, 0x04}, 
	{0x3751, 0xf0}, 
	{0x3752, 0x00}, 
	{0x3753, 0x00}, 
	{0x3754, 0xc0}, 
	{0x3755, 0x00}, 
	{0x3756, 0x1a}, 
	{0x3758, 0x00}, 
	{0x3759, 0x0f}, 
	{0x376b, 0x44}, 
	{0x375c, 0x04}, 
	{0x3776, 0x00}, 
	{0x377f, 0x08}, 

	
	{0x3780, 0x22}, 
	{0x3781, 0x0c}, 
	{0x3784, 0x2c}, 
	{0x3785, 0x1e}, 
	{0x378f, 0xf5}, 
	{0x3791, 0xb0}, 
	{0x3795, 0x00}, 
	{0x3796, 0x64}, 
	{0x3797, 0x11}, 
	{0x3798, 0x30}, 
	{0x3799, 0x41}, 
	{0x379a, 0x07}, 
	{0x379b, 0xb0}, 
	{0x379c, 0x0c}, 

	
	{0x37c5, 0x00}, 
	{0x37c6, 0x00}, 
	{0x37c7, 0x00}, 

	{0x37c9, 0x00}, 
	{0x37ca, 0x00}, 
	{0x37cb, 0x00}, 

	{0x37cc, 0x00}, 
	{0x37cd, 0x00}, 

	{0x37ce, 0x01}, 
	{0x37cf, 0x00}, 

	

	{0x37de, 0x00}, 
	{0x37df, 0x00}, 

	
	{0x3a04, 0x06}, 
	{0x3a05, 0x14}, 
	{0x3a06, 0x00}, 
	{0x3a07, 0xfe}, 
	{0x3b00, 0x00}, 
	{0x3b02, 0x00}, 
	{0x3b03, 0x00}, 
	{0x3b04, 0x00}, 
	{0x3b05, 0x00}, 


	
	{0x3d00, 0x00}, 
	{0x3d01, 0x00}, 
	{0x3d02, 0x00}, 
	{0x3d03, 0x00}, 
	{0x3d04, 0x00}, 
	{0x3d05, 0x00}, 
	{0x3d06, 0x00}, 
	{0x3d07, 0x00}, 
	{0x3d08, 0x00}, 
	{0x3d09, 0x00}, 
	{0x3d0a, 0x00}, 
	{0x3d0b, 0x00}, 
	{0x3d0c, 0x00}, 
	{0x3d0d, 0x00}, 
	{0x3d0e, 0x00}, 
	{0x3d0f, 0x00}, 
	{0x3d80, 0x00}, 
	{0x3d81, 0x00}, 
	{0x3d84, 0x00}, 
	{0x3e07, 0x20}, 


	
	{0x4000, 0x08}, 
	{0x4001, 0x04}, 
	{0x4002, 0x45}, 
	{0x4004, 0x08}, 
	{0x4005, 0x18}, 
	{0x4006, 0x20}, 
	{0x4008, 0x24}, 
	{0x4009, 0x10}, 
	{0x400c, 0x00},
	{0x400d, 0x00},
	{0x404e, 0x37}, 
	{0x404f, 0x8f}, 
	{0x4058, 0x00},

	
	{0x4101, 0xb2}, 
	{0x4303, 0x00}, 
	{0x4304, 0x08}, 
	{0x4307, 0x30}, 
	{0x4311, 0x04},
	{0x4315, 0x01}, 

	{0x4511, 0x05}, 
	{0x4512, 0x01}, 


	
	{0x4806, 0x00}, 
	{0x4816, 0x52},
	{0x481f, 0x30},
	{0x4826, 0x2c}, 

	{0x4831, 0x64}, 
	{0x4837, 0x0a}, 

	{0x4d00, 0x04}, 
	{0x4d01, 0x71}, 
	{0x4d02, 0xfd}, 
	{0x4d03, 0xf5}, 
	{0x4d04, 0x0c}, 
	{0x4d05, 0xcc}, 

	
	{0x5000, 0x06}, 
	{0x5001, 0x01}, 
	{0x5002, 0x00}, 
	{0x5003, 0x20}, 
	{0x5046, 0x0a},
	{0x5013, 0x00}, 
	{0x5046, 0x0a},
	{0x5780, 0x1c}, 
	{0x5786, 0x20}, 
	{0x5787, 0x10}, 
	{0x5788, 0x18}, 
	{0x578a, 0x04}, 
	{0x578b, 0x02}, 
	{0x578c, 0x02}, 
	{0x578e, 0x06}, 
	{0x578f, 0x02}, 
	{0x5790, 0x02}, 
	{0x5791, 0xff}, 

	
	{0x5842, 0x01}, 
	{0x5843, 0x2b}, 
	{0x5844, 0x01}, 
	{0x5845, 0x92}, 
	{0x5846, 0x01}, 
	{0x5847, 0x8f}, 
	{0x5848, 0x01}, 
	{0x5849, 0x0c}, 

#if 1
	{0x5e00, 0x00}, 
#else
	{0x5e00, 0x80},   
#endif

	{0x5e10, 0x0c}, 
	
	


#if 0
	{0x0102, 0x01},
	{0x0100, 0x00}, 
	{0x309A, 0x05},
	{0x309B, 0x01},
	{0x30B6, 0x02},
	{0x0102, 0x01},
	{0x0100, 0x00}, 
#endif

	

};

static struct v4l2_subdev_info ov5693_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	
};

static struct msm_camera_i2c_conf_array ov5693_init_conf[] = {
	{&ov5693_recommend_settings[0],
	ARRAY_SIZE(ov5693_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array ov5693_confs[] = {
	{&ov5693_snap_settings[0],
	ARRAY_SIZE(ov5693_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5693_prev_settings[0],
	ARRAY_SIZE(ov5693_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5693_video_settings[0],
	ARRAY_SIZE(ov5693_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5693_fast_video_settings[0],
	ARRAY_SIZE(ov5693_fast_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t ov5693_dimensions[] = {
	{
		
		.x_output = 0xA20,	
		.y_output = 0x7A0,	
		.line_length_pclk = 0xA84,		
		.frame_length_lines = 0x7C8,	
		.vt_pixel_clk = 160000000,
		.op_pixel_clk = 160000000,
		.binning_factor = 1,
		
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xA1F,
		.y_addr_end = 0x79F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{
		
		.x_output = 0xA20,	
		.y_output = 0x7A0,	
		.line_length_pclk = 0xA84,		
		.frame_length_lines = 0x7C8,	
		.vt_pixel_clk = 160000000,
		.op_pixel_clk = 160000000,
		.binning_factor = 1,
		
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xA1F,
		.y_addr_end = 0x79F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{
		
		.x_output = 0x510,	
		.y_output = 0x3cc,	
		.line_length_pclk = 0xA84, 
		.frame_length_lines = 0x7C8,  
		.vt_pixel_clk = 160000000,
		.op_pixel_clk = 160000000,
		.binning_factor = 2,
		
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xa1f,
		.y_addr_end = 0x797,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
	{
		
		.x_output = 0x500,	
		.y_output = 0x2D0,	
		.line_length_pclk = 0x6D8,		
		.frame_length_lines = 0x308,	
		.vt_pixel_clk = 160000000,
		.op_pixel_clk = 160000000,
		.binning_factor = 1,
		
		.x_addr_start = 0x0,
		.y_addr_start = 0x0,
		.x_addr_end = 0x4FF,
		.y_addr_end = 0x2EF,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
};

#if 0
static struct msm_camera_csid_vc_cfg ov5693_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params ov5693_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 2,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = ov5693_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 2,
		.settle_cnt = 20,
	},
};
#else
static struct msm_camera_csi_params ov5693_csi_params = {
	.data_format = CSI_10BIT,
	.lane_cnt    = 2,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 0x20,
	.dt = CSI_RAW10,
};
#endif

static struct msm_camera_csi_params *ov5693_csi_params_array[] = {
	&ov5693_csi_params,
	&ov5693_csi_params,
	&ov5693_csi_params,
	&ov5693_csi_params
};

static struct msm_sensor_output_reg_addr_t ov5693_reg_addr = {
	.x_output = 0x3808,
	.y_output = 0x380a,
	.line_length_pclk = 0x380c,
	.frame_length_lines = 0x380e,
};

static struct msm_sensor_id_info_t ov5693_id_info = {
	.sensor_id_reg_addr = 0x300A,

	.sensor_id = 0x5690,
};

static struct msm_sensor_exp_gain_info_t ov5693_exp_gain_info = {
	.coarse_int_time_addr = 0x3500,
	.global_gain_addr = 0x350A,
	.vert_offset = 6,
	.min_vert = 4,  
	.sensor_max_linecount = 32761,  
};


#define MAX_FUSE_ID_INFO 17
unsigned short fuse_id[MAX_FUSE_ID_INFO] = {0};
unsigned short OTP[10] = {0};
static int fuseid_read_once = 0;

static int ov5693_read_fuseid_once(void)
{
	struct msm_camera_i2c_client *ov5693_msm_camera_i2c_client = ov5693_s_ctrl.sensor_i2c_client;
	int i;
	unsigned short bank_addr;
	int count = 0, dirty = 0;

	
	msm_camera_i2c_write_b(ov5693_msm_camera_i2c_client, 0x0100, 0x01);

	for (i = 0; i < 5; i++) {
		bank_addr = 0xC0 + (0x14 - i); 
		pr_info("[CAM]ov5693_read_fuseid_once  bank_addr=0x%x\n", bank_addr);

		msm_camera_i2c_write_b(ov5693_msm_camera_i2c_client, 0x3d84, bank_addr);
		msm_camera_i2c_write_b(ov5693_msm_camera_i2c_client, 0x3d81, 0x01);
		msleep(3);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d00, &fuse_id[0]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d01, &fuse_id[1]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d02, &fuse_id[2]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d03, &fuse_id[3]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d04, &fuse_id[4]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d05, &fuse_id[5]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d06, &fuse_id[6]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d07, &fuse_id[7]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d08, &fuse_id[8]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d09, &fuse_id[9]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d0A, &fuse_id[10]);
		msm_camera_i2c_read_b(ov5693_msm_camera_i2c_client, 0x3d0B, &fuse_id[11]);

		for (count = 0; count < MAX_FUSE_ID_INFO; count++) {
			if (fuse_id[count] != 0) {
				dirty = 1;
				break;
			}
		}

		if (dirty) {
			OTP[0] = fuse_id[0]; 
			OTP[1] = fuse_id[1]; 
			OTP[2] = fuse_id[4]; 
			OTP[3] = fuse_id[5]; 
			OTP[4] = fuse_id[6]; 
			OTP[5] = fuse_id[7]; 
			OTP[6] = fuse_id[8]; 
			OTP[7] = fuse_id[9]; 
			fuseid_read_once = 1;
			break;
		}
	}

	msm_camera_i2c_write_b(ov5693_msm_camera_i2c_client, 0x0100, 0x00);

	return 0;
}

static int ov5693_read_fuseid(struct sensor_cfg_data *cdata,
	struct msm_sensor_ctrl_t *s_ctrl)
{
	pr_info("[CAM]ov5693_read_fuseid\n");

	if (fuseid_read_once == 0)
		ov5693_read_fuseid_once();

	if (fuseid_read_once) {
		cdata->cfg.fuse.fuse_id_word1 = (uint32_t) (OTP[0]<<8) | (OTP[1]);
		cdata->cfg.fuse.fuse_id_word2 = (uint32_t) (OTP[2]<<8) | (OTP[3]);
		cdata->cfg.fuse.fuse_id_word3 = (uint32_t) (OTP[4]<<8) | (OTP[5]);
		cdata->cfg.fuse.fuse_id_word4 = (uint32_t) (OTP[6]<<8) | (OTP[7]);
	}

	pr_info("[CAM]ov5693: fuse->fuse_id_word1:0x%04x\n",
		cdata->cfg.fuse.fuse_id_word1);
	pr_info("[CAM]ov5693: fuse->fuse_id_word2:0x%04x\n",
		cdata->cfg.fuse.fuse_id_word2);
	pr_info("[CAM]ov5693: fuse->fuse_id_word3:0x%04x\n",
		cdata->cfg.fuse.fuse_id_word3);
	pr_info("[CAM]ov5693: fuse->fuse_id_word4:0x%04x\n",
		cdata->cfg.fuse.fuse_id_word4);
	return 0;
}


static int ov5693_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	pr_info("[CAM] %s\n", __func__);

	if (fuseid_read_once == 0)
		ov5693_read_fuseid_once();

	return rc;
}

static const char *ov5693Vendor = "Omnivision";
static const char *ov5693NAME = "ov5693";
static const char *ov5693Size = "5M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", ov5693Vendor, ov5693NAME, ov5693Size);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);

static struct kobject *android_ov5693;

static int ov5693_sysfs_init(void)
{
	int ret ;
	pr_info("ov5693:kobject creat and add\n");
	android_ov5693 = kobject_create_and_add("android_camera", NULL);
	if (android_ov5693 == NULL) {
		pr_info("ov5693_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("ov5693:sysfs_create_file\n");
	ret = sysfs_create_file(android_ov5693, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("ov5693_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_ov5693);
	}

	return 0 ;
}

int32_t ov5693_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	rc = 0;
	pr_info("[CAM] %s\n", __func__);
	rc = msm_sensor_i2c_probe(client, id);
	if(rc >= 0)
		ov5693_sysfs_init();
	pr_info("[CAM] %s: rc(%d)\n", __func__, rc);
	return rc;
}

static const struct i2c_device_id ov5693_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ov5693_s_ctrl},
	{ }
};

static struct i2c_driver ov5693_i2c_driver = {
	.id_table = ov5693_i2c_id,
	.probe  = ov5693_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client ov5693_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};


int32_t ov5693_power_up(struct msm_sensor_ctrl_t *s_ctrl)
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

	ov5693_sensor_open_init(sdata);
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

int32_t ov5693_power_down(struct msm_sensor_ctrl_t *s_ctrl)
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
static int ov5693_probe(struct platform_device *pdev)
{
	int	rc = 0;

	pr_info("%s\n", __func__);

	rc = msm_sensor_register(pdev, ov5693_sensor_v4l2_probe);
	if(rc >= 0)
		ov5693_sysfs_init();
	return rc;
}

struct platform_driver ov5693_driver = {
	.probe = ov5693_probe,
	.driver = {
		.name = PLATFORM_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init msm_sensor_init_module(void)
{
	pr_info("%s\n", __func__);
	return platform_driver_register(&ov5693_driver);
}
#else
static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&ov5693_i2c_driver);
}
#endif

static struct v4l2_subdev_core_ops ov5693_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ov5693_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ov5693_subdev_ops = {
	.core = &ov5693_subdev_core_ops,
	.video  = &ov5693_subdev_video_ops,
};


int32_t ov5693_write_exp_gain1_ex(struct msm_sensor_ctrl_t *s_ctrl,
		int mode, uint16_t gain, uint16_t dig_gain, uint32_t line) 
{
	uint32_t fl_lines;
	uint8_t offset;
	uint8_t line_p0,line_p1,line_p2;
	uint8_t fl_lines_h,fl_lines_l;
	uint8_t gain_h,gain_l;


	uint32_t fps_divider = Q10;


	CDBG("[CAM] ov5693_write_exp_gain1_ex line=0x%x gain=0x%x", line, gain);
	CDBG("[CAM] ov5693_write_exp_gain1_ex line=%d gain=%d", line, gain);

	if(line > s_ctrl->sensor_exp_gain_info->sensor_max_linecount)
		line = s_ctrl->sensor_exp_gain_info->sensor_max_linecount;

	fl_lines = s_ctrl->curr_frame_length_lines;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line * Q10 > (fl_lines - offset) * fps_divider)
		fl_lines = line + offset;
	else
		fl_lines = (fl_lines * fps_divider) / Q10;

	fl_lines_h = (uint8_t)(fl_lines/256);
	fl_lines_l = (uint8_t)(fl_lines%256);
	gain_h = (uint8_t)(gain/256);
	gain_l = (uint8_t)(gain%256);

	line_p0 = (uint8_t)(line/(256*16));
	line_p1 = (uint8_t)((line%(256*16))/16);
	line_p2 = (uint8_t)((line%16)*16);

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines, fl_lines_h,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines+1, fl_lines_l,
		MSM_CAMERA_I2C_BYTE_DATA);


	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, line_p0,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr+1, line_p1,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr+2, line_p2,
		MSM_CAMERA_I2C_BYTE_DATA);


	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain_h,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr+1, gain_l,
		MSM_CAMERA_I2C_BYTE_DATA);
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}

int32_t ov5693_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res) {

	int rc = 0;
	int rc1 = 0, rc2 = 0;
	unsigned short f_value = 0;	
	unsigned short m_value = 0;	
	struct msm_camera_sensor_info *sdata = NULL;
	pr_info("[CAM] %s\n", __func__);

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_err("[CAM] %s: s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	rc = msm_sensor_setting1(s_ctrl, update_type, res);

	if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		ov5693_s_ctrl.mirror_flip = sdata->sensor_platform_info->mirror_flip;

		
		rc1 = msm_camera_i2c_read_b(ov5693_s_ctrl.sensor_i2c_client, OV5693_REG_FLIP, &f_value);
		if (rc1 < 0)
			pr_info("[CAM]%s: msm_camera_i2c_read_b 0x%x fail\n", __func__, OV5693_REG_FLIP);
		rc2 = msm_camera_i2c_read_b(ov5693_s_ctrl.sensor_i2c_client, OV5693_REG_MIRROR, &m_value);
		if (rc2 < 0)
			pr_info("[CAM]%s: msm_camera_i2c_read_b 0x%x fail\n", __func__, OV5693_REG_MIRROR);

		pr_info("[CAM] f_value = 0x%x m_value = 0x%x", f_value, m_value);

		
		if (ov5693_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR_FLIP) {
			pr_info("[CAM] CAMERA_SENSOR_MIRROR_FLIP");
			f_value |= OV5693_FLIP;
			m_value |= OV5693_MIRROR;
		} else if (ov5693_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR) {
			pr_info("[CAM] CAMERA_SENSOR_MIRROR");
			f_value &= OV5693_FLIP_NORMAL;
			m_value |= OV5693_MIRROR;
		} else if (ov5693_s_ctrl.mirror_flip == CAMERA_SENSOR_FLIP) {
			pr_info("[CAM] CAMERA_SENSOR_FLIP");
			f_value |= OV5693_FLIP;
			m_value &= OV5693_MIRROR_NORMAL;
		} else {
			pr_info("[CAM] CAMERA_SENSOR_NONE");
			f_value &= OV5693_FLIP_NORMAL;
			m_value &= OV5693_MIRROR_NORMAL;
		}

		pr_info("[CAM] New f_value = 0x%x m_value = 0x%x", f_value, m_value);

		rc1 = msm_camera_i2c_write_b(ov5693_s_ctrl.sensor_i2c_client, OV5693_REG_FLIP, f_value);
		if (rc1 < 0)
			pr_info("[CAM]%s: msm_camera_i2c_read_b 0x%x fail\n", __func__, OV5693_REG_FLIP);

		rc2 = msm_camera_i2c_write_b(ov5693_s_ctrl.sensor_i2c_client, OV5693_REG_MIRROR, m_value);
		if (rc2 < 0)
			pr_info("[CAM]%s: msm_camera_i2c_read_b 0x%x fail\n", __func__, OV5693_REG_MIRROR);

		msleep(30); 
	}

	return rc;
}


static struct msm_sensor_fn_t ov5693_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_write_exp_gain_ex = ov5693_write_exp_gain1_ex,
	.sensor_write_snapshot_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_write_snapshot_exp_gain_ex = ov5693_write_exp_gain1_ex,
#if 0
	.sensor_setting = msm_sensor_setting,
#else
	.sensor_csi_setting = ov5693_sensor_setting, 
#endif
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_power_up = ov5693_power_up,
	.sensor_power_down = ov5693_power_down,
	.sensor_config = msm_sensor_config,
	.sensor_get_csi_params = msm_sensor_get_csi_params,
	.sensor_i2c_read_fuseid = ov5693_read_fuseid,	
};

static struct msm_sensor_reg_t ov5693_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = ov5693_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ov5693_start_settings),
	.stop_stream_conf = ov5693_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ov5693_stop_settings),
	.group_hold_on_conf = ov5693_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(ov5693_groupon_settings),
	.group_hold_off_conf = ov5693_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(ov5693_groupoff_settings),
	.init_settings = &ov5693_init_conf[0],
	.init_size = ARRAY_SIZE(ov5693_init_conf),
	.mode_settings = &ov5693_confs[0],
	.output_settings = &ov5693_dimensions[0],
	.num_conf = ARRAY_SIZE(ov5693_confs),
};

static struct msm_sensor_ctrl_t ov5693_s_ctrl = {
	.msm_sensor_reg = &ov5693_regs,
	.sensor_i2c_client = &ov5693_sensor_i2c_client,
	.sensor_i2c_addr = 0x20,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_output_reg_addr = &ov5693_reg_addr,
	.sensor_id_info = &ov5693_id_info,
	.sensor_exp_gain_info = &ov5693_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
#if 0
	.csi_params = &ov5693_csi_params_array[0],
#else
	.csic_params = &ov5693_csi_params_array[0],
#endif
	.msm_sensor_mutex = &ov5693_mut,
	.sensor_i2c_driver = &ov5693_i2c_driver,
	.sensor_v4l2_subdev_info = ov5693_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ov5693_subdev_info),
	.sensor_v4l2_subdev_ops = &ov5693_subdev_ops,
	.func_tbl = &ov5693_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Omnivision 5 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");

