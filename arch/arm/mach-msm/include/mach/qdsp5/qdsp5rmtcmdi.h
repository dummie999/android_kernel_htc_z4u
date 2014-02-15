/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
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

#ifndef QDSP5RMTCMDI_H
#define QDSP5RMTCMDI_H



#define RM_CMD_AUD_CODEC_CFG	0x0

#define RM_AUD_CLIENT_ID	0x0
#define RMT_ENABLE		0x1
#define RMT_DISABLE		0x0

struct aud_codec_config_cmd {
	unsigned short			cmd_id;
	unsigned char			task_id;
	unsigned char			client_id;
	unsigned short			enable;
	unsigned short			dec_type;
} __attribute__((packed));

#endif 
