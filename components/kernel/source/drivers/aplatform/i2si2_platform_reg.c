#include <kernel/drivers/snd.h>
#include <kernel/io.h>
#include <kernel/drivers/platform_register.h>
#include <kernel/drivers/avinit.h>
#include <kernel/module.h>

static struct snd_soc_dai_link_component i2si2_platform_dais[] = {
	{
		.name = "i2si2-dai",
	},
	{
		.name = "wm8960-for-i2si2-dai",
	},
};

static struct snd_soc_platform i2si2_platform = {
	.name = "i2si2-platform",
	.devname = "i2si2",
	.direction = SND_STREAM_CAPTURE,
	.driver = &i2si2_platform_driver,
	.priv = &i2si2_platform_dev,
	.dailinks = i2si2_platform_dais,
	.num_dailinks = ARRAY_SIZE(i2si2_platform_dais),
	.dais = LIST_HEAD_INIT(i2si2_platform.dais),
};

int i2si2_platform_init(void)
{
	int np;

	np = fdt_node_probe_by_path("/hcrtos/i2si2");
	if (np < 0) {
		return 0;
	}
	if (fdt_node_probe_by_path("/hcrtos/i2s") < 0)
		return 0;

	i2si2_platform_dev.pinmux_data = fdt_get_property_pinmux(np, "data");

	return snd_soc_register_platform(&i2si2_platform);
}

//module_driver(i2si2_platform, i2si2_platform_init, NULL, 1)
