#include <kernel/drivers/snd.h>
#include <kernel/io.h>
#include <kernel/drivers/platform_register.h>
#include <kernel/drivers/avinit.h>
#include <kernel/module.h>

static struct snd_soc_dai_link_component pcmi1_platform_dais[] = {
	{
		.name = "i2so-for-pcmi1-dai",
	},
	{
		.name = "pcmi1-dai",
	},
	{
		.name = "wm8960-for-pcmi1-dai",
	},
};

static struct snd_soc_platform pcmi1_platform = {
	.name = "pcmi1-platform",
	.devname = "pcmi1",
	.direction = SND_STREAM_CAPTURE,
	.driver = &pcmi1_platform_driver,
	.priv = &pcmi1_platform_dev,
	.dailinks = pcmi1_platform_dais,
	.num_dailinks = ARRAY_SIZE(pcmi1_platform_dais),
	.dais = LIST_HEAD_INIT(pcmi1_platform.dais),
};

int pcmi1_platform_init(void)
{
	int np;
	const char *status;

	np = fdt_node_probe_by_path("/hcrtos/pcmi1");
	if (np < 0) {
		return 0;
	}

	if (!fdt_get_property_string_index(np, "status", 0, &status) &&
	!strcmp(status, "disabled")) {
		return 0;
	}
	pcmi1_platform_dev.pinmux_data = fdt_get_property_pinmux(np, "data");

	return snd_soc_register_platform(&pcmi1_platform);
}
