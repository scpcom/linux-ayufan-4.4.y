/*
 * armada370-db.c
 *
 * (c) 2010 Arnaud Patard <apatard@mandriva.com>
 * (c) 2010 Arnaud Patard <arnaud.patard@rtp-net.org>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <sound/soc.h>
#include <plat/audio.h>
#include <asm/mach-types.h>
#include "../codecs/cs42l51.h"

static int a370_client_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;
	unsigned int freq, fmt;

	fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS;
	ret = snd_soc_dai_set_fmt(cpu_dai, fmt);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_fmt(codec_dai, fmt);
	if (ret < 0)
		return ret;

	switch (params_rate(params)) {
	default:
	case 44100:
		freq = 11289600;
		break;
	case 48000:
		freq = 12288000;
		break;
	case 96000:
		freq = 24576000;
		break;
	}

	return snd_soc_dai_set_sysclk(codec_dai, 0, freq, SND_SOC_CLOCK_IN);

}

static struct snd_soc_ops a370_client_ops = {
	.hw_params = a370_client_hw_params,
};

static struct snd_soc_dai_link a370_client_dai[] = {
{
	.name = "CS42L51",
	.stream_name = "CS42L51 HiFi",
	.cpu_dai_name = "kirkwood-i2s",
	.platform_name = "kirkwood-pcm-audio",
	.codec_dai_name = "cs42l51-hifi",
	.codec_name = "cs42l51-codec.0-004a",
	.ops = &a370_client_ops,
},
};

static struct snd_soc_card a370_client = {
	.name = "Armada-370",
	.dai_link = a370_client_dai,
	.num_links = ARRAY_SIZE(a370_client_dai),
};

static struct platform_device *a370_client_snd_device;

static int __init a370_client_init(void)
{
	int ret;

	if (!machine_is_armada_370())
		return 0;

	a370_client_snd_device = platform_device_alloc("soc-audio", -1);
	if (!a370_client_snd_device)
		return -ENOMEM;

	platform_set_drvdata(a370_client_snd_device,
			&a370_client);

	ret = platform_device_add(a370_client_snd_device);
	if (ret) {
		printk(KERN_ERR "%s: platform_device_add failed\n", __func__);
		platform_device_put(a370_client_snd_device);
	}

	return ret;
}

static void __exit a370_client_exit(void)
{
	platform_device_unregister(a370_client_snd_device);
}

module_init(a370_client_init);
module_exit(a370_client_exit);

/* Module information */
MODULE_AUTHOR("Eran Ben-Avi <benavi@marvell.com>");
MODULE_DESCRIPTION("ALSA SoC for Armada-370");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:soc-audio");
