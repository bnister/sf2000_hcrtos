#include <kernel/drivers/snd.h>
#include <kernel/io.h>
#include <kernel/drivers/platform_register.h>
#include <kernel/drivers/avinit.h>

static struct snd_soc_dai_link_component platform_dais[] = {
	{
		.name = "tdmi-dai",
	},
};

static struct snd_soc_platform tdmi_platform = {
	.name = "tdmi-platform",
	.devname = "tdmi",
	.direction = SND_STREAM_CAPTURE,
	.driver = &tdmi_platform_driver,
	.priv = &tdmi_platform_dev,
	.dailinks = platform_dais,
	.num_dailinks = ARRAY_SIZE(platform_dais),
	.dais = LIST_HEAD_INIT(tdmi_platform.dais),
};

int tdmi_platform_init(void)
{
	int np;

	np = fdt_node_probe_by_path("/hcrtos/tdmi");
	if (np < 0) {
		return 0;
	}
	tdmi_platform_dev.pinmux_data = fdt_get_property_pinmux(np, "data");

	return snd_soc_register_platform(&tdmi_platform);
}
