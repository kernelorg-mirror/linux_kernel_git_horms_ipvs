/*
 * Kodoh.c
 *
 * Copyright (C) 2010 Renesas Solutions Corp.
 * Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>
 *
 * Based on ak4642.c by Kuninori Morimoto
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include "kodoh.h"

#define KODOH_VERSION "0.0.1"

#define KHwRwAicREF_PWR_00h		0x00
#define KHwRwAicDAC_PWR_01h		0x01
#define KHwRwAicMIX1_DACL_02h		0x02
#define KHwRwAicMIX2_DACR_03h		0x03
#define KHwRwAicMUX3_SEL_04h		0x04
#define KHwRwAicMUX4_SEL_05h		0x05
#define KHwRwAicMUX5_SEL_06h		0x06
#define KHwRwAicReserve_07h		0x07
#define KHwRwAicPATH_CNT_08h		0x08
#define KHwRwAicO_MODE_09h		0x09
#define KHwRwAicPOP1_0Ah		0x0A
#define KHwRwAicDRV_PWR_0Bh		0x0B
#define KHwRwAicPOP2_0Ch		0x0C
#define KHwRwAicDRV_MT_0Dh		0x0D
#define KHwRwAicHPOUT_CNT_0Eh		0x0E
#define KHwRwAicHPPD_CNT_0Fh		0x0F
#define KHwRwAicReserve_10h		0x10
#define KHwRwAicReserve_11h		0x11
#define KHwRwAicOVR_1_12h		0x12
#define KHwRwAicOVR_2_13h		0x13
#define KHwRwAicOVR_3_14h		0x14
#define KHwRwAicOVR_4_15h		0x15
#define KHwRwAicOVR_5_16h		0x16
#define KHwRwAicReserve_17h		0x17
#define KHwRwAicIMICL_CNT_18h		0x18
#define KHwRwAicIMICR_CNT_19h		0x19
#define KHwRwAicU_PATH_1Ah		0x1A
#define KHwRwAicMICAMPL_GAIN_1Bh	0x1B
#define KHwRwAicMICAMPR_GAIN_1Ch	0x1C
#define KHwRwAicSW_MIC_1Dh		0x1D
#define KHwRwAicMUX1_SEL_1Eh		0x1E
#define KHwRwAicReserve_1Fh		0x1F
#define KHwRwAicDA_AGC_CNT_20h		0x20
#define KHwRwAicDA_AGC_VTH_21h		0x21
#define KHwRwAicDA_AGC_TIME_22h		0x22
#define KHwRwAicDA_AGC_HOLD_23h		0x23
#define KHwRwAicZERO_TIME_24h		0x24
#define KHwRwAicReserve_25h		0x25
#define KHwRwAicReserve_26h		0x26
#define KHwRwAicReserve_27h		0x27
#define KHwRwAicReserve_28h		0x28
#define KHwRwAicReserve_29h		0x29
#define KHwRwAicAUDIO_SET_2Ah		0x2A
#define KHwRwAicPULL_DOWN_2Bh		0x2B
#define KHwRwAicAUDIO_MUTE_2Ch		0x2C
#define KHwRwAicAUDIO_FS_2Dh		0x2D
#define KHwRwAicIVR_1_2Eh		0x2E
#define KHwRwAicReserve_2Fh		0x2F
#define KHwRwAicPCM_CONF_30h		0x30
#define KHwRwAicTXVOL1_31h		0x31
#define KHwRwAicRXVOL_32h		0x32
#define KHwRwAicST_CONT_33h		0x33
#define KHwRwAicDTVOL_34h		0x34
#define KHwRwAicTXVOL2_35h		0x35
#define KHwRwAicDT_F_SET_H_36h		0x36
#define KHwRwAicDT_F_SET_H_37h		0x37
#define KHwRwAicDT_F_SET_L_38h		0x38
#define KHwRwAicDT_F_SET_L_39h		0x39
#define KHwRwAicPATH_CONT_3Ah		0x3A
#define KHwRwAicPWR_CONT_3Bh		0x3B
#define KHwRwAicPCM_FORM_3Ch		0x3C
#define KHwRwAicPCM_LPBK_3Dh		0x3D
#define KHwRwAicRX_PATH_3Eh		0x3E
#define KHwRwAicPLL_SET_CP_3Fh		0x3F
#define KHwRwAicPLL_SET_M_N_40h		0x40
#define KHwRwAicPLL_CONT_41h		0x41
#define KHwRwAicPLL_PWR_42h		0x42
#define KHwRwAicReserve_43h		0x43
#define KHwRwAicReserve_44h		0x44
#define KHwRwAicIVR_2_45h		0x45
#define KHwRwAicIVR_3_46h		0x46
#define KHwRwAicReserve_47h		0x47
#define KHwRwAicReserve_48h		0x48
#define KHwRwAicReserve_49h		0x49
#define KHwRwAicReserve_4Ah		0x4A
#define KHwRwAicReserve_4Bh		0x4B
#define KHwRwAicReserve_4Ch		0x4C
#define KHwRwAicReserve_4Dh		0x4D
#define KHwRwAicReserve_4Eh		0x4E
#define KHwRwAicBT_LPBK_4Fh		0x4F
#define KHwRwAicPCM_CNT_B_50h		0x50
#define KHwRwAicRXVOLB_51h		0x51
#define KHwRwAicTXVOLB_52h		0x52
#define KHwRwAicLPVOL_53h		0x53
#define KHwRwAicPCM_B_PD_54h		0x54
#define KHwRwAicPCM_B_CONF_55h		0x55
#define KHwRwAicAD_AGC_CNT_56h		0x56
#define KHwRwAicAD_AGC_VTH_57h		0x57
#define KHwRwAicAD_AGC_TIME_58h		0x58
#define KHwRwAicAD_AGC_HOLD_59h		0x59
#define KHwRwAicReserve_5Ah		0x5A
#define KHwRwAicReserve_5Bh		0x5B
#define KHwRwAicReserve_5Ch		0x5C
#define KHwRwAicReserve_5Dh		0x5D
#define KHwRwAicReserve_5Eh		0x5E
#define KHwRwAicReserve_5Fh		0x5F
#define KHwRwAicReserve_60h		0x60
#define KHwRwAicTEST_CNT_61h		0x61
#define KHwRwAicReserve_62h		0x62
#define KHwRwAicReserve_63h		0x63
#define KHwRwAicReserve_64h		0x64
#define KHwRwAicReserve_65h		0x65
#define KHwRwAicReserve_66h		0x66
#define KHwRwAicReserve_67h		0x67
#define KHwRwAicReserve_68h		0x68
#define KHwRwAicReserve_69h		0x69
#define KHwRwAicReserve_6Ah		0x6A
#define KHwRwAicReserve_6Bh		0x6B
#define KHwRwAicReserve_6Ch		0x6C
#define KHwRwAicPATH_RST_6Dh		0x6D
#define KHwRwAicCODEC_RST_6Eh		0x6E
#define KHwRwAicRST_CNT_6Fh		0x6F
#define KHwRwAicDV_CODE_70h		0x70

#define KHwRwAicPOP1C1_ECh		0xEC
#define KHwRwAicPOP1C2_EDh		0xED

#define KNoAccess			0xFF
#define KNone				0x00

/* codec private data */
struct kodoh_priv {
	struct snd_soc_codec codec;
};

static struct snd_soc_codec *kodoh_codec;

/*
 * read kodoh register cache
 */
static inline unsigned int kodoh_read(struct snd_soc_codec *codec,
	unsigned int reg)
{
	return -1;
}

/*
 * write to the KODOH register space
 */
static int kodoh_write(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int value)
{
	u8 data[2];

	/* data is
	 *   D15..D8 KODOH register offset
	 *   D7...D0 register data
	 */
	data[0] = reg & 0xff;
	data[1] = value & 0xff;

	if (codec->hw_write(codec->control_data, data, 2) == 2)
		return 0;
	else
		return -EIO;
}

static int kodoh_dai_startup(struct snd_pcm_substream *substream,
			      struct snd_soc_dai *dai)
{
	int is_play = substream->stream == SNDRV_PCM_STREAM_PLAYBACK;
	struct snd_soc_codec *codec = dai->codec;

	if (!is_play)
		return -EIO;

	if (is_play) {
		int i;
		u8 addr[] = {
			KHwRwAicREF_PWR_00h,	KNoAccess,		KHwRwAicREF_PWR_00h,	KHwRwAicMIX1_DACL_02h,	KHwRwAicMIX2_DACR_03h,
			KHwRwAicMUX3_SEL_04h,	KHwRwAicMUX5_SEL_06h,	KHwRwAicPULL_DOWN_2Bh,	KHwRwAicIVR_2_45h,	KHwRwAicIVR_3_46h,
			KHwRwAicAUDIO_SET_2Ah,	KHwRwAicAUDIO_FS_2Dh,	KHwRwAicPLL_PWR_42h,	KHwRwAicPLL_PWR_42h,	KNoAccess,
			KHwRwAicAUDIO_MUTE_2Ch,	KHwRwAicDAC_PWR_01h,	KHwRwAicAUDIO_SET_2Ah,	KHwRwAicPOP2_0Ch,	KHwRwAicPOP1_0Ah,
			KHwRwAicPOP1C1_ECh,	KHwRwAicPOP1C2_EDh,	KHwRwAicOVR_4_15h,	KHwRwAicPATH_CNT_08h,	KHwRwAicDRV_PWR_0Bh,
			KNoAccess,		KHwRwAicDRV_MT_0Dh,	KNoAccess
		};

		u8 data[] = {
			0x27,	KNone,	0x23,	0x01,	0x02,
			0x01,	0x02,	0x28,	0x05,	0x05,
			0x11,	0x02,	0x08,	0x0A,	KNone,
			0x02,	0x03,	0x10,	0x03,	0x0F,
			0x53,	0x38,	0xe0,	0x30,	0x30,
			KNone,	0x30,	KNone
		};

		for (i = 0; i < ARRAY_SIZE(addr); i++) {
			if(KNoAccess == addr[i])
				continue;

			kodoh_write(codec, addr[i], data[i]);
		}
	}

	return 0;
}

static void kodoh_dai_shutdown(struct snd_pcm_substream *substream,
			       struct snd_soc_dai *dai)
{
	int is_play = substream->stream == SNDRV_PCM_STREAM_PLAYBACK;
	struct snd_soc_codec *codec = dai->codec;

	if (is_play) {
		int i;
		u8 addr[] = {
			KHwRwAicDRV_MT_0Dh,	KNoAccess,
			KHwRwAicDRV_PWR_0Bh,	KNoAccess,
			KHwRwAicRST_CNT_6Fh,	KNoAccess
		};

		u8 data[] = {
			0x00,	KNone,
			0x00,	KNone,
			0x01,	KNone,
		};

		for (i = 0; i < ARRAY_SIZE(addr); i++) {
			if(KNoAccess == addr[i])
				continue;

			kodoh_write(codec, addr[i], data[i]);
		}
	}

}

static int kodoh_dai_hw_params(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params,
			       struct snd_soc_dai *dai)
{
	u8 val = 0x02;
	struct snd_soc_codec *codec = dai->codec;

	switch (params_rate(params)) {
	case 8000:	val = 0x02;	break;
	case 11025:	val = 0x12;	break;
	case 12000:	val = 0x22;	break;
	case 16000:	val = 0x31;	break;
	case 22050:	val = 0x41;	break;
	case 24000:	val = 0x51;	break;
	case 32000:	val = 0x60;	break;
	case 44100:	val = 0x70;	break;
	case 48000:	val = 0x80;	break;
	default:	val = 0x02;	break;
	}

	kodoh_write(codec, KHwRwAicAUDIO_FS_2Dh, val);
	return 0;
}

static struct snd_soc_dai_ops kodoh_dai_ops = {
	.startup	= kodoh_dai_startup,
	.shutdown	= kodoh_dai_shutdown,
	.hw_params	= kodoh_dai_hw_params,
};

struct snd_soc_dai kodoh_dai = {
	.name = "KODOH",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE },
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE },
	.ops = &kodoh_dai_ops,
	.symmetric_rates = 1,
};
EXPORT_SYMBOL_GPL(kodoh_dai);

/*
 * initialise the KODOH driver
 * register the mixer and dsp interfaces with the kernel
 */
static int kodoh_init(struct kodoh_priv *kodoh)
{
	struct snd_soc_codec *codec = &kodoh->codec;
	int ret = 0;

	if (kodoh_codec) {
		dev_err(codec->dev, "Another kodoh is registered\n");
		return -EINVAL;
	}

	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	snd_soc_codec_set_drvdata(codec, kodoh);
	codec->name		= "KODOH";
	codec->owner		= THIS_MODULE;
	codec->read		= kodoh_read;
	codec->write		= kodoh_write;
	codec->dai		= &kodoh_dai;
	codec->num_dai		= 1;
	codec->hw_write		= (hw_write_t)i2c_master_send;

	kodoh_dai.dev = codec->dev;
	kodoh_codec = codec;

	ret = snd_soc_register_codec(codec);
	if (ret) {
		dev_err(codec->dev, "Failed to register codec: %d\n", ret);
		return ret;
	}

	ret = snd_soc_register_dai(&kodoh_dai);
	if (ret) {
		dev_err(codec->dev, "Failed to register DAI: %d\n", ret);
		snd_soc_unregister_codec(codec);
		return ret;
	}

	return ret;
}

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
static int kodoh_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct kodoh_priv *kodoh;
	struct snd_soc_codec *codec;
	int ret;

	kodoh = kzalloc(sizeof(struct kodoh_priv), GFP_KERNEL);
	if (!kodoh)
		return -ENOMEM;

	codec = &kodoh->codec;
	codec->dev = &i2c->dev;

	i2c_set_clientdata(i2c, kodoh);
	codec->control_data = i2c;

	ret = kodoh_init(kodoh);
	if (ret < 0)
		printk(KERN_ERR "failed to initialise KODOH\n");

	return ret;
}

static int kodoh_i2c_remove(struct i2c_client *client)
{
	struct kodoh_priv *kodoh = i2c_get_clientdata(client);

	snd_soc_unregister_dai(&kodoh_dai);
	snd_soc_unregister_codec(&kodoh->codec);
	kfree(kodoh);
	kodoh_codec = NULL;

	return 0;
}

static const struct i2c_device_id kodoh_i2c_id[] = {
	{ "kodoh", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, kodoh_i2c_id);

static struct i2c_driver kodoh_i2c_driver = {
	.driver = {
		.name = "KODOH I2C Codec",
		.owner = THIS_MODULE,
	},
	.probe		= kodoh_i2c_probe,
	.remove		= kodoh_i2c_remove,
	.id_table	= kodoh_i2c_id,
};

#endif

static int kodoh_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	int ret;

	if (!kodoh_codec) {
		dev_err(&pdev->dev, "Codec device not registered\n");
		return -ENODEV;
	}

	socdev->card->codec = kodoh_codec;

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "kodoh: failed to create pcms\n");
		goto pcm_err;
	}

	dev_info(&pdev->dev, "KODOH Audio Codec %s", KODOH_VERSION);
	return ret;

pcm_err:
	return ret;

}

/* power down chip */
static int kodoh_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);

	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_kodoh = {
	.probe =	kodoh_probe,
	.remove =	kodoh_remove,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_kodoh);

static int __init kodoh_modinit(void)
{
	int ret = 0;
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	ret = i2c_add_driver(&kodoh_i2c_driver);
#endif
	return ret;

}
module_init(kodoh_modinit);

static void __exit kodoh_exit(void)
{
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	i2c_del_driver(&kodoh_i2c_driver);
#endif

}
module_exit(kodoh_exit);

MODULE_DESCRIPTION("Soc KODOH driver");
MODULE_AUTHOR("Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>");
MODULE_LICENSE("GPL");
