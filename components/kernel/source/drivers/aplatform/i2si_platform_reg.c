#include <kernel/drivers/snd.h>
#include <kernel/io.h>
#include <kernel/drivers/platform_register.h>
#include <kernel/drivers/avinit.h>

static struct snd_soc_dai_link_component i2si_platform_dais[] = {
	{
		.name = "i2si-dai",
	},
	{
		.name = "cjc8990-for-i2si-dai",
	},
	{
		.name = "wm8960i-dai",
	},
	{
		.name = "cjc8988-for-i2si-dai",
	},
};

static struct snd_soc_platform i2si_platform = {
	.name = "i2si-platform",
	.devname = "i2si",
	.direction = SND_STREAM_CAPTURE,
	.driver = &i2si_platform_driver,
	.priv = &i2si_platform_dev,
	.dailinks = i2si_platform_dais,
	.num_dailinks = ARRAY_SIZE(i2si_platform_dais),
	.dais = LIST_HEAD_INIT(i2si_platform.dais),
};

int i2si_platform_init(void)
{
	int np;
	u32 vol;

	np = fdt_node_probe_by_path("/hcrtos/i2si");
	if (np < 0) {
		return 0;
	}
	if (fdt_node_probe_by_path("/hcrtos/i2s") < 0)
		return 0;

	i2si_platform_dev.pinmux_data = fdt_get_property_pinmux(np, "data");

	vol = 0xff;
	fdt_get_property_u_32_index(np, "volume", 0, &vol);
	i2si_platform_dev.volume = vol;

	return snd_soc_register_platform(&i2si_platform);
}
