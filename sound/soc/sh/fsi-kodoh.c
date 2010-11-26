/*
 * FSI-kodoh sound support
 *
 * Copyright (C) 2010 Renesas Solutions Corp.
 * Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <sound/sh_fsi.h>
#include <../sound/soc/codecs/kodoh.h>

static struct snd_soc_dai_link fsi_dai_link = {
	.name		= "KODOH",
	.stream_name	= "KODOH",
	.cpu_dai	= &fsi_soc_dai[0], /* FSI A */
	.codec_dai	= &kodoh_dai,
	.ops		= NULL,
};

static struct snd_soc_card fsi_soc_card  = {
	.name		= "FSI",
	.platform	= &fsi_soc_platform,
	.dai_link	= &fsi_dai_link,
	.num_links	= 1,
};

static struct snd_soc_device fsi_snd_devdata = {
	.card		= &fsi_soc_card,
	.codec_dev	= &soc_codec_dev_kodoh,
};

static struct platform_device *fsi_snd_device;

static int __init fsi_kodoh_init(void)
{
	int ret = -ENOMEM;

	fsi_snd_device = platform_device_alloc("soc-audio", -1);
	if (!fsi_snd_device)
		goto out;

	platform_set_drvdata(fsi_snd_device,
			     &fsi_snd_devdata);
	fsi_snd_devdata.dev = &fsi_snd_device->dev;
	ret = platform_device_add(fsi_snd_device);

	if (ret)
		platform_device_put(fsi_snd_device);

out:
	return ret;
}

static void __exit fsi_kodoh_exit(void)
{
	platform_device_unregister(fsi_snd_device);
}

module_init(fsi_kodoh_init);
module_exit(fsi_kodoh_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Generic SH4 FSI-KODOH sound card");
MODULE_AUTHOR("Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>");
