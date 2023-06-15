#include <kernel/drivers/snd.h>
#include <kernel/io.h>
#include <kernel/drivers/platform_register.h>
#include <kernel/drivers/avinit.h>

static struct snd_soc_dai_link_component pcmi_platform_dais[] = {
	{
		.name = "i2so-for-pcmi-dai",
	},
	{
		.name = "pcmoi-dai",
	},
	{
		.name = "pcmi-dai",
	},
	{
		.name = "wm8960i-dai",
	},
};

static struct snd_soc_platform pcmi_platform = {
	.name = "pcmi-platform",
	.devname = "pcmi",
	.direction = SND_STREAM_CAPTURE,
	.driver = &i2si_platform_driver,
	.priv = &i2si_platform_dev,
	.dailinks = pcmi_platform_dais,
	.num_dailinks = ARRAY_SIZE(pcmi_platform_dais),
	.dais = LIST_HEAD_INIT(pcmi_platform.dais),
};

int pcmi_platform_init(void)
{
	int np;

	np = fdt_node_probe_by_path("/hcrtos/pcmi");
	if (np < 0) {
		return 0;
	}

	i2si_platform_dev.pinmux_data = fdt_get_property_pinmux(np, "data");
	return snd_soc_register_platform(&pcmi_platform);
}
