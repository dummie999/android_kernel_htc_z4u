/* include/linux/i2c/aic3254.h - aic3254 Codec driver header file
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
 * This program is modified according to linux/spi/spi_aic3254.h
 * Some labels are change:
 * 	spi_aic3254.h			this_program
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

#ifndef __AIC3254_H__
#define __AIC3254_H__

#define AIC3254_I2C_NAME		"aic3254"
#define AIC3254_I2C_ADDR		0x18

#define AIC3254_REG_NUM			0xFF
#define AIC3254_MAX_PAGES		255
#define AIC3254_MAX_REGS		128
#define AIC3254_MAX_RETRY		10

#define IO_CTL_ROW_MAX			64
#define IO_CTL_COL_MAX			1024
#define MINIDSP_ROW_MAX			32
#define MINIDSP_COL_MAX			16384

#define AIC3254_IOCTL_MAGIC     's'
#define AIC3254_SET_TX_PARAM     _IOW(AIC3254_IOCTL_MAGIC, 0x10, unsigned)
#define AIC3254_SET_RX_PARAM     _IOW(AIC3254_IOCTL_MAGIC, 0x11, unsigned)
#define AIC3254_CONFIG_TX        _IOW(AIC3254_IOCTL_MAGIC, 0x12, unsigned int)
#define AIC3254_CONFIG_RX        _IOW(AIC3254_IOCTL_MAGIC, 0x13, unsigned int)
#define AIC3254_SET_DSP_PARAM    _IOW(AIC3254_IOCTL_MAGIC, 0x20, unsigned)
#define AIC3254_CONFIG_MEDIA     _IOW(AIC3254_IOCTL_MAGIC, 0x21, unsigned int)
#define AIC3254_CONFIG_VOICE     _IOW(AIC3254_IOCTL_MAGIC, 0x22, unsigned int)
#define AIC3254_CONFIG_VOLUME_L  _IOW(AIC3254_IOCTL_MAGIC, 0x23, unsigned int)
#define AIC3254_CONFIG_VOLUME_R  _IOW(AIC3254_IOCTL_MAGIC, 0x24, unsigned int)
#define AIC3254_POWERDOWN        _IOW(AIC3254_IOCTL_MAGIC, 0x25, unsigned int)
#define AIC3254_LOOPBACK         _IOW(AIC3254_IOCTL_MAGIC, 0x26, unsigned int)
#define AIC3254_DUMP_PAGES       _IOW(AIC3254_IOCTL_MAGIC, 0x30, unsigned int)
#define AIC3254_READ_REG         _IOWR(AIC3254_IOCTL_MAGIC, 0x31, unsigned)
#define AIC3254_WRITE_REG        _IOW(AIC3254_IOCTL_MAGIC, 0x32, unsigned)
#define AIC3254_RESET            _IOW(AIC3254_IOCTL_MAGIC, 0x33, unsigned int)
#define AIC3254_WRITE_ARRAY      _IOW(AIC3254_IOCTL_MAGIC, 0x88, unsigned)

struct aic3254_cmd {
	uint8_t act;
	uint8_t reg;
	uint8_t data;
};

struct aic3254_param {
    unsigned int row_num;
    unsigned int col_num;
    void *cmd_data;
};

enum aic3254_uplink_mode {
	INITIAL = 0,
	CALL_UPLINK_IMIC_RECEIVER = 1,
	CALL_UPLINK_EMIC_HEADPHONE,
	CALL_UPLINK_IMIC_HEADPHONE,
	CALL_UPLINK_IMIC_SPEAKER,
	VOICERECORD_IMIC,
	VOICERECORD_EMIC,
	VIDEORECORD_IMIC,
	VIDEORECORD_EMIC,
	VOICERECOGNITION_IMIC,
	VOICERECOGNITION_EMIC,
	FM_IN_SPEAKER,
	FM_IN_HEADSET,
	VOIP_RECEIVER_IMIC,
	VOIP_SPEAKER_IMIC,
	VOIP_HEADSET_EMIC,
	VOIP_NO_MIC_HEADSET_IMIC,
	BT,
	UPLINK_OFF,
	UPLINK_WAKEUP,
	POWER_OFF,
};

#endif
