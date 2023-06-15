#include <stdlib.h>
#include <string.h>
#include <kernel/soc/soc_common.h>
#include <kernel/drivers/snd.h>
#include <kernel/io.h>
#include <kernel/lib/fdt_api.h>

struct dai_device {
	struct snd_capability cap;
};

/*
 * Some devices do not support rates lower than 44100, remove those
 * low rates from the capability.
 */
#define SND_PCM_RATE_32K48K                                                    \
	(/*SND_PCM_RATE_8000 | SND_PCM_RATE_12000 | SND_PCM_RATE_16000 |         \
	 SND_PCM_RATE_24000 | SND_PCM_RATE_32000 | */SND_PCM_RATE_48000 |        \
	 SND_PCM_RATE_64000 | SND_PCM_RATE_96000 | SND_PCM_RATE_128000 |       \
	 SND_PCM_RATE_192000)

#define SND_PCM_RATE_44_1K                                                     \
	(/*SND_PCM_RATE_5512 | SND_PCM_RATE_11025 | SND_PCM_RATE_22050 | \
	*/SND_PCM_RATE_44100 | SND_PCM_RATE_88200 | SND_PCM_RATE_176400)

static struct dai_device dai_dev = {
	.cap =
		{
			.rates = (SND_PCM_RATE_32K48K | SND_PCM_RATE_44_1K),
			.formats =
				(SND_PCM_FMTBIT_S8 | SND_PCM_FMTBIT_U8 |
				 SND_PCM_FMTBIT_S16_LE | SND_PCM_FMTBIT_U16_LE |
				 SND_PCM_FMTBIT_S24_LE | SND_PCM_FMTBIT_U24_LE),
		},
};

static int cs4344_dai_hw_params(struct snd_soc_dai *dai, unsigned int rate,
			      snd_pcm_format_t fmt, unsigned int channels,
			      uint8_t align)
{
	return 0;
}

static int cs4344_dai_trigger(struct snd_soc_dai *dai, unsigned int cmd)
{
	return 0;
}

static int cs4344_dai_ioctl(struct snd_soc_dai *dai, unsigned int cmd, void *arg)
{
	return 0;
}

static int cs4344_dai_get_capability(struct snd_soc_dai *dai,
				   struct snd_capability *cap)
{
	struct dai_device *dev = dai->priv;

	memcpy(cap, &dev->cap, sizeof(*cap));

	return 0;
}

static int cs4344_dai_hw_free(struct snd_soc_dai *dai)
{
  	return 0;
}

static struct snd_soc_dai_driver cs4344_dai_driver = {
	.hw_params = cs4344_dai_hw_params,
	.trigger = cs4344_dai_trigger,
	.ioctl = cs4344_dai_ioctl,
	.get_capability = cs4344_dai_get_capability,
	.hw_free = cs4344_dai_hw_free,
};

static struct snd_soc_dai cs4344_dai = {
	.name = "cs4344-dac-dai",
	.driver = &cs4344_dai_driver,
	.priv = &dai_dev,
};

int cs4344_dai_init(void)
{
	int np;

	np = fdt_node_probe_by_path("/hcrtos/cs4344");
	if (np < 0) {
		return 0;
	}

	return snd_soc_register_dai(&cs4344_dai);
}
