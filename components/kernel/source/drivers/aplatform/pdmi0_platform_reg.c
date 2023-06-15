#include <kernel/drivers/snd.h>
#include <kernel/io.h>
#include <kernel/drivers/platform_register.h>
#include <kernel/drivers/avinit.h>

static struct snd_soc_dai_link_component platform_dais[] = {
	{
		.name = "pdmi0-dai",
	},
};

static struct snd_soc_platform pdmi_platform = {
	.name = "pdmi0-platform",
	.devname = "pdmi0",
	.direction = SND_STREAM_CAPTURE,
	.driver = &pdmi0_platform_driver,
	.priv = &pdmi0_platform_dev,
	.dailinks = platform_dais,
	.num_dailinks = ARRAY_SIZE(platform_dais),
	.dais = LIST_HEAD_INIT(pdmi_platform.dais),
};

int pdmi0_platform_init(void)
{
	int np;

	np = fdt_node_probe_by_path("/hcrtos/pdmi0");
	if (np < 0) {
		return 0;
	}
	pdmi0_platform_dev.pinmux_data = fdt_get_property_pinmux(np, "data");

	return snd_soc_register_platform(&pdmi_platform);
}
