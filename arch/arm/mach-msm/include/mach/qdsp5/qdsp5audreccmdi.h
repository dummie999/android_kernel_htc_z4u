#ifndef QDSP5AUDRECCMDI_H
#define QDSP5AUDRECCMDI_H

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
 *
 *    A U D I O   R E C O R D  I N T E R N A L  C O M M A N D S
 *
 * GENERAL DESCRIPTION
 *   This file contains defintions of format blocks of commands
 *   that are accepted by AUDREC Task
 *
 * REFERENCES
 *   None
 *
 * EXTERNALIZED FUNCTIONS
 *  None
 *
 * Copyright (c) 1992-2009, 2011 Code Aurora Forum. All rights reserved.
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
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/





#define AUDREC_CMD_CFG		0x0000
#define	AUDREC_CMD_CFG_LEN	sizeof(audrec_cmd_cfg)

#define	AUDREC_CMD_TYPE_0_INDEX_WAV	0x0000
#define	AUDREC_CMD_TYPE_0_INDEX_AAC	0x0001
#define	AUDREC_CMD_TYPE_0_INDEX_AMRNB	0x000A
#define	AUDREC_CMD_TYPE_0_INDEX_EVRC	0x000B
#define	AUDREC_CMD_TYPE_0_INDEX_QCELP	0x000C

#define AUDREC_CMD_TYPE_0_ENA		0x4000
#define AUDREC_CMD_TYPE_0_DIS		0x0000

#define AUDREC_CMD_TYPE_0_NOUPDATE	0x0000
#define AUDREC_CMD_TYPE_0_UPDATE	0x8000

#define	AUDREC_CMD_TYPE_1_INDEX_SBC	0x0002

#define AUDREC_CMD_TYPE_1_ENA		0x4000
#define AUDREC_CMD_TYPE_1_DIS		0x0000

#define AUDREC_CMD_TYPE_1_NOUPDATE	0x0000
#define AUDREC_CMD_TYPE_1_UPDATE	0x8000

typedef struct {
	unsigned short 	cmd_id;
	unsigned short	type_0;
	unsigned short	type_1;
} __attribute__((packed)) audrec_cmd_cfg;



#define	AUDREC_CMD_AREC0PARAM_CFG	0x0001
#define	AUDREC_CMD_AREC0PARAM_CFG_LEN	\
	sizeof(audrec_cmd_arec0param_cfg)

#define	AUDREC_CMD_SAMP_RATE_INDX_8000		0x000B
#define	AUDREC_CMD_SAMP_RATE_INDX_11025		0x000A
#define	AUDREC_CMD_SAMP_RATE_INDX_12000		0x0009
#define	AUDREC_CMD_SAMP_RATE_INDX_16000		0x0008
#define	AUDREC_CMD_SAMP_RATE_INDX_22050		0x0007
#define	AUDREC_CMD_SAMP_RATE_INDX_24000		0x0006
#define	AUDREC_CMD_SAMP_RATE_INDX_32000		0x0005
#define	AUDREC_CMD_SAMP_RATE_INDX_44100		0x0004
#define	AUDREC_CMD_SAMP_RATE_INDX_48000		0x0003

#define AUDREC_CMD_STEREO_MODE_MONO		0x0000
#define AUDREC_CMD_STEREO_MODE_STEREO		0x0001

typedef struct {
	unsigned short 	cmd_id;
	unsigned short	ptr_to_extpkt_buffer_msw;
	unsigned short	ptr_to_extpkt_buffer_lsw;
	unsigned short	buf_len;
	unsigned short	samp_rate_index;
	unsigned short	stereo_mode;
	unsigned short 	rec_quality;
} __attribute__((packed)) audrec_cmd_arec0param_cfg;


#define AUDREC_CMD_AREC1PARAM_CFG	0x0002
#define AUDREC_CMD_AREC1PARAM_CFG_LEN	\
	sizeof(audrec_cmd_arec1param_cfg)

#define AUDREC_CMD_PARAM_BUF_BLOCKS_4	0x0000
#define AUDREC_CMD_PARAM_BUF_BLOCKS_8	0x0001
#define AUDREC_CMD_PARAM_BUF_BLOCKS_12	0x0002
#define AUDREC_CMD_PARAM_BUF_BLOCKS_16	0x0003

#define AUDREC_CMD_PARAM_BUF_SUB_BANDS_8	0x0010
#define AUDREC_CMD_PARAM_BUF_MODE_MONO		0x0000
#define AUDREC_CMD_PARAM_BUF_MODE_DUAL		0x0040
#define AUDREC_CMD_PARAM_BUF_MODE_STEREO	0x0050
#define AUDREC_CMD_PARAM_BUF_MODE_JSTEREO	0x0060
#define AUDREC_CMD_PARAM_BUF_LOUDNESS		0x0000
#define AUDREC_CMD_PARAM_BUF_SNR		0x0100
#define AUDREC_CMD_PARAM_BUF_BASIC_VER		0x0000

typedef struct {
	unsigned short 	cmd_id;
	unsigned short	ptr_to_extpkt_buffer_msw;
	unsigned short	ptr_to_extpkt_buffer_lsw;
	unsigned short	buf_len;
	unsigned short	param_buf;
	unsigned short	bit_rate_0;
	unsigned short	bit_rate_1;
} __attribute__((packed)) audrec_cmd_arec1param_cfg;


#define AUDREC_CMD_ENC_CFG	0x0003
#define AUDREC_CMD_ENC_CFG_LEN	\
	sizeof(struct audrec_cmd_enc_cfg)


#define AUDREC_CMD_ENC_ENA		0x8000
#define AUDREC_CMD_ENC_DIS		0x0000

#define AUDREC_CMD_ENC_TYPE_MASK	0x001F

struct audrec_cmd_enc_cfg {
	unsigned short 	cmd_id;
	unsigned short	audrec_enc_type;
	unsigned short	audrec_obj_idx;
} __attribute__((packed));


#define AUDREC_CMD_ARECMEM_CFG	0x0004
#define AUDREC_CMD_ARECMEM_CFG_LEN	\
	sizeof(struct audrec_cmd_arecmem_cfg)


struct audrec_cmd_arecmem_cfg {
	unsigned short 	cmd_id;
	unsigned short	audrec_obj_idx;
	unsigned short	audrec_up_pkt_intm_cnt;
	unsigned short	audrec_extpkt_buffer_msw;
	unsigned short	audrec_extpkt_buffer_lsw;
	unsigned short	audrec_extpkt_buffer_num;
} __attribute__((packed));


#define AUDREC_CMD_ARECPARAM_CFG	0x0005
#define AUDREC_CMD_ARECPARAM_COMMON_CFG_LEN	\
	sizeof(struct audrec_cmd_arecparam_common_cfg)


struct audrec_cmd_arecparam_common_cfg {
	unsigned short 	cmd_id;
	unsigned short	audrec_obj_idx;
} __attribute__((packed));

#define AUDREC_CMD_ARECPARAM_WAV_CFG_LEN	\
	sizeof(struct audrec_cmd_arecparam_wav_cfg)


struct audrec_cmd_arecparam_wav_cfg {
	struct audrec_cmd_arecparam_common_cfg common;
	unsigned short 	samp_rate_idx;
	unsigned short  stereo_mode;
} __attribute__((packed));

#define AUDREC_CMD_ARECPARAM_AAC_CFG_LEN	\
	sizeof(struct audrec_cmd_arecparam_aac_cfg)


struct audrec_cmd_arecparam_aac_cfg {
	struct audrec_cmd_arecparam_common_cfg common;
	unsigned short 	samp_rate_idx;
	unsigned short  stereo_mode;
	unsigned short  rec_quality;
} __attribute__((packed));

#define AUDREC_CMD_ARECPARAM_SBC_CFG_LEN	\
	sizeof(struct audrec_cmd_arecparam_sbc_cfg)


struct audrec_cmd_arecparam_sbc_cfg {
	struct audrec_cmd_arecparam_common_cfg common;
	unsigned short 	param_buf;
	unsigned short  bit_rate_0;
	unsigned short  bit_rate_1;
} __attribute__((packed));

#define AUDREC_CMD_ARECPARAM_AMRNB_CFG_LEN	\
	sizeof(struct audrec_cmd_arecparam_amrnb_cfg)


struct audrec_cmd_arecparam_amrnb_cfg {
	struct audrec_cmd_arecparam_common_cfg common;
	unsigned short 	samp_rate_idx;
	unsigned short 	voicememoencweight1;
	unsigned short 	voicememoencweight2;
	unsigned short 	voicememoencweight3;
	unsigned short 	voicememoencweight4;
	unsigned short 	update_mode;
	unsigned short 	dtx_mode;
	unsigned short 	test_mode;
	unsigned short 	used_mode;
} __attribute__((packed));

#define AUDREC_CMD_ARECPARAM_EVRC_CFG_LEN	\
	sizeof(struct audrec_cmd_arecparam_evrc_cfg)


struct audrec_cmd_arecparam_evrc_cfg {
	struct audrec_cmd_arecparam_common_cfg common;
	unsigned short 	samp_rate_idx;
	unsigned short 	voicememoencweight1;
	unsigned short 	voicememoencweight2;
	unsigned short 	voicememoencweight3;
	unsigned short 	voicememoencweight4;
	unsigned short 	update_mode;
	unsigned short 	enc_min_rate;
	unsigned short 	enc_max_rate;
	unsigned short 	rate_modulation_cmd;
} __attribute__((packed));

#define AUDREC_CMD_ARECPARAM_QCELP_CFG_LEN	\
	sizeof(struct audrec_cmd_arecparam_qcelp_cfg)


struct audrec_cmd_arecparam_qcelp_cfg {
	struct audrec_cmd_arecparam_common_cfg common;
	unsigned short 	samp_rate_idx;
	unsigned short 	voicememoencweight1;
	unsigned short 	voicememoencweight2;
	unsigned short 	voicememoencweight3;
	unsigned short 	voicememoencweight4;
	unsigned short 	update_mode;
	unsigned short 	enc_min_rate;
	unsigned short 	enc_max_rate;
	unsigned short 	rate_modulation_cmd;
	unsigned short 	reduced_rate_level;
} __attribute__((packed));

#define AUDREC_CMD_ARECPARAM_FGVNB_CFG_LEN	\
	sizeof(struct audrec_cmd_arecparam_fgvnb_cfg)


struct audrec_cmd_arecparam_fgvnb_cfg {
	struct audrec_cmd_arecparam_common_cfg common;
	unsigned short 	samp_rate_idx;
	unsigned short 	voicememoencweight1;
	unsigned short 	voicememoencweight2;
	unsigned short 	voicememoencweight3;
	unsigned short 	voicememoencweight4;
	unsigned short 	update_mode;
	unsigned short 	fgv_min_rate;
	unsigned short 	fgv_max_rate;
	unsigned short 	reduced_rate_level;
} __attribute__((packed));


#define AUDREC_CMD_ROUTING_MODE		0x0006
#define	AUDREC_CMD_ROUTING_MODE_LEN	\
	sizeof(struct audpreproc_audrec_cmd_routing_mode)

#define AUDIO_ROUTING_MODE_FTRT		0x0001
#define AUDIO_ROUTING_MODE_RT		0x0002

struct audrec_cmd_routing_mode {
	unsigned short cmd_id;
	unsigned short routing_mode;
} __packed;


#define AUDREC_CMD_PCM_CFG_ARM_TO_ENC 0x0007
#define AUDREC_CMD_PCM_CFG_ARM_TO_ENC_LEN	\
	sizeof(struct audrec_cmd_pcm_cfg_arm_to_enc)

struct audrec_cmd_pcm_cfg_arm_to_enc {
	unsigned short cmd_id;
	unsigned short config_update_flag;
	unsigned short enable_flag;
	unsigned short sampling_freq;
	unsigned short channels;
	unsigned short frequency_of_intimation;
	unsigned short max_number_of_buffers;
} __packed;

#define AUDREC_PCM_CONFIG_UPDATE_FLAG_ENABLE -1
#define AUDREC_PCM_CONFIG_UPDATE_FLAG_DISABLE 0

#define AUDREC_ENABLE_FLAG_VALUE -1
#define AUDREC_DISABLE_FLAG_VALUE 0


#define AUDREC_CMD_PCM_BUFFER_PTR_REFRESH_ARM_TO_ENC 0x0008
#define AUDREC_CMD_PCM_BUFFER_PTR_REFRESH_ARM_TO_ENC_LEN \
	sizeof(struct audrec_cmd_pcm_buffer_ptr_refresh_arm_enc)

struct audrec_cmd_pcm_buffer_ptr_refresh_arm_enc {
	unsigned short cmd_id;
	unsigned short num_buffers;
	unsigned short buffer_write_cnt_msw;
	unsigned short buffer_write_cnt_lsw;
	unsigned short buf_address_length[8];
} __packed;


#define AUDREC_CMD_FLUSH 0x009
#define AUDREC_CMD_FLUSH_LEN	\
	sizeof(struct audrec_cmd_flush)

struct audrec_cmd_flush {
	unsigned short cmd_id;
} __packed;



#define AUDREC_CMD_PACKET_EXT_PTR		0x0000
#define AUDREC_CMD_PACKET_EXT_PTR_LEN	\
	sizeof(audrec_cmd_packet_ext_ptr)

#define AUDREC_CMD_TYPE_0	0x0000
#define AUDREC_CMD_TYPE_1	0x0001

typedef struct {
	unsigned short  cmd_id;
	unsigned short	type; 
	unsigned short 	curr_rec_count_msw;
	unsigned short 	curr_rec_count_lsw;
} __attribute__((packed)) audrec_cmd_packet_ext_ptr;

#endif
