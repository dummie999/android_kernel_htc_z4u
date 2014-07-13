/* linux/spi/spi_aic3254_reg.h -  aic3254 Codec driver reg value table
 *
 * Copyright (C) 2008-2012 HTC Corporation.
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
 * This program is modified according to linux/spi/spi_aic3254_reg.h
 * Some labels are change:
 * 	spi_aic3254_reg.h		this_program
 *
 *	codec_dev		-->		aic3254_client
 *	codec_drv->idlelock	-->		aic3254_idlelock
 *	codec_drv->wakelock	-->		aic3254_wakelock
 *	pr_aud_info		-->		AIC3254_INF
 *	pr_aud_err		-->		AIC3254_ERR
 *	_CODEC_SPI_CMD		-->		aic3254_cmd
 *	CODEC_SPI_CMD		-->		struct aic3254_cmd
 *	AIC3254_PARAM		-->		aic3254_param
 *
 * Owner: Octagram Sun (octagram_sun@htc.com) HTCC_SSD3 Audio_Team
 * Version: alpha 1
 */

#ifndef __AIC3254_REG_H__
#define __AIC3254_REG_H__
static struct aic3254_cmd CODEC_POWER_OFF[] = {
	
	{'w', 0x00, 0x01},
	{'w', 0x09, 0x00},
	{'w', 0x3B, 0x00},
	{'w', 0x3C, 0x00},
	{'w', 0x7B, 0x01},

	{'w', 0x00, 0x00},
	{'w', 0x57, 0x00},
	{'w', 0x56, 0x00},
	{'w', 0x5F, 0x00},
	{'w', 0x5E, 0x00},

	{'w', 0x51, 0x00},
	{'w', 0x40, 0x0C},
	{'r', 0x24, 0x88},
	{'w', 0x3F, 0x16},
	{'r', 0x25, 0x00},

	{'r', 0x26, 0x11},
	{'w', 0x00, 0x01},
	{'w', 0x0C, 0x00},
	{'w', 0x0D, 0x00},
	{'w', 0x0E, 0x00},

	{'w', 0x0F, 0x00},
	{'w', 0x33, 0x00},
	{'w', 0x3A, 0x00},
	{'w', 0x00, 0x00},
	{'w', 0x1D, 0x00},

	{'w', 0x1A, 0x01},
	{'w', 0x43, 0x00},
	{'d', 0x00, 0xC8},
	{'w', 0x00, 0x01},
	{'w', 0x3A, 0x00},

	{'w', 0x01, 0x00},
	{'w', 0x02, 0xA8},
};

static struct aic3254_cmd CODEC_SET_VOLUME_L[] = {
	{'w', 0x00, 0x00},
	{'w', 0x41, 0x00}
};

static struct aic3254_cmd CODEC_SET_VOLUME_R[] = {
	{'w', 0x00, 0x00},
	{'w', 0x42, 0x00}
};

#endif 
