/* arch/arm/mach-msm/qdsp5/audio_fm.c
 *
 * pcm audio output device
 *
 * Copyright (C) 2008 Google, Inc.
 * Copyright (C) 2008 HTC Corporation
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/msm_audio.h>
#include <mach/debug_mm.h>

#include "audmgr.h"
#include <mach/qdsp5/qdsp5audppmsg.h>
#include <mach/qdsp5/qdsp5audpp.h>
#include <linux/sched.h>
#include <mach/msm_adsp.h>
#include "adsp.h"
struct audio {
	struct mutex lock;
	int opened;
	int enabled;
	int running;
	struct audmgr audmgr;
	uint16_t volume;
	enum msm_aud_decoder_state dec_state; 
	wait_queue_head_t wait;
	uint8_t out_needed; 
	int dec_id;
	int wflush; 
	wait_queue_head_t write_wait;
	int teos; 
	
	uint32_t out_sample_rate;
	uint32_t out_channel_mode;
	uint32_t out_bits; 
	struct msm_adsp_module *audplay;
	const char *module_name;
	unsigned queue_id;
};

static struct audio fm_audio;

#define  AUDPP_DEC_STATUS_SLEEP	0
#define  AUDPP_DEC_STATUS_INIT  1
#define  AUDPP_DEC_STATUS_CFG   2
#define  AUDPP_DEC_STATUS_PLAY  3

#define AUDDEC_DEC_PCM 0

static void audio_dsp_event(void *private, unsigned id, uint16_t *msg);
static void audpp_cmd_cfg_adec_params(struct audio *audio);
static int auddec_dsp_config(struct audio *audio, int enable);


static int auddec_dsp_config(struct audio *audio, int enable)
{
	u16 cfg_dec_cmd[AUDPP_CMD_CFG_DEC_TYPE_LEN / sizeof(unsigned short)];

	memset(cfg_dec_cmd, 0, sizeof(cfg_dec_cmd));

	cfg_dec_cmd[0] = AUDPP_CMD_CFG_DEC_TYPE;
	if (enable)
		cfg_dec_cmd[1 + audio->dec_id] = AUDPP_CMD_UPDATDE_CFG_DEC |
				AUDPP_CMD_ENA_DEC_V | AUDDEC_DEC_PCM;
	else
		cfg_dec_cmd[1 + audio->dec_id] = AUDPP_CMD_UPDATDE_CFG_DEC |
				AUDPP_CMD_DIS_DEC_V;

	return audpp_send_queue1(&cfg_dec_cmd, sizeof(cfg_dec_cmd));
}

static void audpp_cmd_cfg_adec_params(struct audio *audio)
{
	audpp_cmd_cfg_adec_params_wav cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.common.cmd_id = AUDPP_CMD_CFG_ADEC_PARAMS;
	cmd.common.length = AUDPP_CMD_CFG_ADEC_PARAMS_WAV_LEN;
	cmd.common.dec_id = audio->dec_id;
	cmd.common.input_sampling_frequency = audio->out_sample_rate;
	cmd.stereo_cfg = audio->out_channel_mode;
	cmd.pcm_width = audio->out_bits;
	cmd.sign = 0;
	audpp_send_queue2(&cmd, sizeof(cmd));
}
static void audio_dsp_event(void *private, unsigned id, uint16_t *msg)
{
	struct audio *audio = private;

	switch (id) {
	case AUDPP_MSG_STATUS_MSG:{
			unsigned status = msg[1];

			switch (status) {
			case AUDPP_DEC_STATUS_SLEEP: {
				uint16_t reason = msg[2];
				MM_DBG("decoder status: sleep reason = \
						0x%04x\n", reason);
				if ((reason == AUDPP_MSG_REASON_MEM)
						|| (reason ==
						AUDPP_MSG_REASON_NODECODER)) {
					audio->dec_state =
						MSM_AUD_DECODER_STATE_FAILURE;
					wake_up(&audio->wait);
				} else if (reason == AUDPP_MSG_REASON_NONE) {
					
					audio->dec_state =
						MSM_AUD_DECODER_STATE_CLOSE;
					wake_up(&audio->wait);
				}
				break;
			}
			case AUDPP_DEC_STATUS_INIT:
				MM_DBG("decoder status: init\n");
				audpp_cmd_cfg_adec_params(audio);
				break;

			case AUDPP_DEC_STATUS_CFG:
				MM_DBG("decoder status: cfg \n");
				break;
			case AUDPP_DEC_STATUS_PLAY:
				MM_DBG("decoder status: play \n");
				audio->dec_state =
					MSM_AUD_DECODER_STATE_SUCCESS;
				wake_up(&audio->wait);
				break;
			default:
				MM_ERR("unknown decoder status \n");
				break;
			}
			break;
		}
	case AUDPP_MSG_CFG_MSG:
		if (msg[0] == AUDPP_MSG_ENA_ENA) {
			MM_DBG("CFG_MSG ENABLE\n");
			auddec_dsp_config(audio, 1);
			audio->out_needed = 0;
			audio->running = 1;
			audpp_set_volume_and_pan(audio->dec_id, audio->volume,
					0);
		} else if (msg[0] == AUDPP_MSG_ENA_DIS) {
			MM_DBG("CFG_MSG DISABLE\n");
			audio->running = 0;
		} else {
			MM_ERR("CFG_MSG %d?\n",	msg[0]);
		}
		break;
	case AUDPP_MSG_FLUSH_ACK:
		MM_DBG("FLUSH_ACK\n");
		audio->wflush = 0;
		wake_up(&audio->write_wait);
		break;

	case AUDPP_MSG_PCMDMAMISSED:
		MM_DBG("PCMDMAMISSED\n");
		audio->teos = 1;
		wake_up(&audio->write_wait);
		break;

	default:
		MM_ERR("UNKNOWN (%d)\n", id);
	}

}

static int audio_enable(struct audio *audio)
{
	struct audmgr_config cfg;
	int rc = 0;

	MM_DBG("\n"); 

	if (audio->enabled)
		return 0;

	cfg.tx_rate = RPC_AUD_DEF_SAMPLE_RATE_48000;
	cfg.rx_rate = RPC_AUD_DEF_SAMPLE_RATE_48000;
	cfg.def_method = RPC_AUD_DEF_METHOD_HOST_PCM;
	cfg.codec = RPC_AUD_DEF_CODEC_PCM;
	cfg.snd_method = RPC_SND_METHOD_MIDI; 
	rc = audmgr_enable(&audio->audmgr, &cfg);
	if (rc < 0)
		return rc;
	if (audpp_enable(audio->dec_id, audio_dsp_event, audio)) {
		MM_ERR("audpp_enable() failed\n");
		msm_adsp_disable(audio->audplay);
		audmgr_disable(&audio->audmgr);
		return -ENODEV;
	}

	audio->enabled = 1;
	return rc;
}

static int audio_disable(struct audio *audio)
{
	MM_DBG("\n"); 
	if (audio->enabled) {
		audio->enabled = 0;
		audmgr_disable(&audio->audmgr);
		audpp_disable(audio->dec_id, audio); 
	}
	return 0;
}

static long audio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct audio *audio = file->private_data;
	int rc = -EINVAL;

	MM_DBG("cmd %d", cmd);

	mutex_lock(&audio->lock);
	switch (cmd) {
	case AUDIO_START:
		MM_DBG("AUDIO_START\n");
		rc = audio_enable(audio);
		break;
	case AUDIO_STOP:
		MM_DBG("AUDIO_STOP\n");
		rc = audio_disable(audio);
		audio->running = 0;
		audio->enabled = 0;
		break;

	default:
		rc = -EINVAL;
	}
	mutex_unlock(&audio->lock);
	return rc;
}

static int audio_release(struct inode *inode, struct file *file)
{
	struct audio *audio = file->private_data;

	MM_DBG("audio instance 0x%08x freeing\n", (int)audio);
	mutex_lock(&audio->lock);
	audio_disable(audio);
	audio->running = 0;
	audio->enabled = 0;
	audio->opened = 0;
	
	if (audio->dec_id != -1)
		audpp_adec_free(audio->dec_id);
	
	mutex_unlock(&audio->lock);
	return 0;
}

static int audio_open(struct inode *inode, struct file *file)
{
	struct audio *audio = &fm_audio;
	int rc = 0, dec_attrb, dec_id = 0; 

	MM_DBG("\n"); 
	mutex_lock(&audio->lock);

	if (audio->opened) {
		MM_ERR("busy\n");
		rc = -EBUSY;
		goto done;
	}

	
	dec_attrb = AUDDEC_DEC_PCM;
	dec_attrb |= MSM_AUD_MODE_NONTUNNEL;  

	dec_id = audpp_adec_alloc(dec_attrb, &audio->module_name,
			&audio->queue_id);
	if (dec_id < 0) {
		MM_ERR("No free decoder available, use -1\n");
		audio->dec_id = -1;
	} else {
		audio->dec_id = dec_id & MSM_AUD_DECODER_MASK;
	}
	

	rc = audmgr_open(&audio->audmgr);

	if (rc) {
		MM_ERR("%s: failed to register listnet\n", __func__);
		goto done;
	}

	
	init_waitqueue_head(&audio->write_wait);
	init_waitqueue_head(&audio->wait);
	
	file->private_data = audio;
	audio->opened = 1;

done:
	mutex_unlock(&audio->lock);
	return rc;
}

static const struct file_operations audio_fm_fops = {
	.owner		= THIS_MODULE,
	.open		= audio_open,
	.release	= audio_release,
	.unlocked_ioctl	= audio_ioctl,
};

struct miscdevice audio_fm_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "msm_fm",
	.fops	= &audio_fm_fops,
};

static int __init audio_init(void)
{
	struct audio *audio = &fm_audio;

	mutex_init(&audio->lock);
	return misc_register(&audio_fm_misc);
}

device_initcall(audio_init);

MODULE_DESCRIPTION("MSM FM driver");
MODULE_LICENSE("GPL v2");
