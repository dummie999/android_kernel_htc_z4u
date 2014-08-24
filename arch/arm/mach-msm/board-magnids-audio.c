/* linux/arch/arm/mach-msm/board-primods-audio.c
*
* Copyright (C) 2010-2011 HTC Corporation.
*
* This software is licensed under the terms of the GNU General Public
*
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/


#include <mach/gpio.h>
#include <mach/pmic.h>
#include <mach/dal.h>
#include "board-magnids.h"
#include <mach/htc_acoustic.h>
#include <mach/board_htc.h>
#include <mach/TCA6418_ioextender.h>

void magnids_mic_bias_enable(int en, int shift)
{
    pr_aud_info("%s: %d\n", __func__, en);

    if (en) {
        ioext_gpio_set_value(MAGNIDS_IOEXP_AUDIO_2V85_EN,1);
    } else {
        ioext_gpio_set_value(MAGNIDS_IOEXP_AUDIO_2V85_EN,0);
    }
}

static struct acoustic_ops acoustic = {
    .enable_mic_bias = magnids_mic_bias_enable,
};

void __init magnids_audio_init(void)
{
    gpio_request(MAGNIDS_IOEXP_AUDIO_2V85_EN, "aud_2v85_en");
    acoustic_register_ops(&acoustic);
    ioext_gpio_set_value(MAGNIDS_IOEXP_AUDIO_2V85_EN,0);
}
