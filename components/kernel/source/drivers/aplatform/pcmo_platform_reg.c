#include <kernel/drivers/snd.h>
#include <kernel/io.h>
#include <kernel/drivers/platform_register.h>
#include <kernel/drivers/avinit.h>

static struct snd_soc_dai_link_component pcmo_platform_dais[] = {
	{
		.name = "pcmo-dai",
	},
	{
		.name = "wm8960-dai",
	},
	{
		.name = "i2so-for-pcmo-dai",
	},
};

static struct snd_soc_platform pcmo_platform = {
	.name = "pcmo-platform",
	.devname = "pcmo",
	.direction = SND_STREAM_PLAYBACK,
	.driver = &i2so_platform_driver,
	.priv = &i2so_platform_dev,
	.dailinks = pcmo_platform_dais,
	.num_dailinks = ARRAY_SIZE(pcmo_platform_dais),
	.dais = LIST_HEAD_INIT(pcmo_platform.dais),
};

int pcmo_platform_init(void)
{
	int np;

	np = fdt_node_probe_by_path("/hcrtos/pcmo");
	if (np < 0) {
		return 0;
	}
	i2so_platform_dev.pinmux_data = fdt_get_property_pinmux(np, "data");

	return snd_soc_register_platform(&pcmo_platform);
}
