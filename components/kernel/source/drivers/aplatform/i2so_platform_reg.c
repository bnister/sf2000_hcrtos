#include <kernel/drivers/snd.h>
#include <kernel/io.h>
#include <kernel/drivers/platform_register.h>
#include <kernel/drivers/avinit.h>
#include <hcuapi/gpio.h>
#include <hcuapi/sysdata.h>

static struct snd_soc_dai_link_component i2so_platform_dais[] = {
	{
		.name = "i2so-dai",
	},
	{
		.name = "cjc8990-for-i2so-dai",
	},
	{
		.name = "cs4344-dac-dai",
	},
	{
		.name = "8988-dac-dai",
	},
	{
		.name = "i2so-codec",
	},
	{
		.name = "pwm-dac-dai",
	},
	{
		.name = "cjc8988-for-i2so-dai",
	}
};

static struct snd_soc_platform i2so_platform = {
	.name = "i2so-platform",
	.devname = "i2so",
	.direction = SND_STREAM_PLAYBACK,
	.driver = &i2so_platform_driver,
	.priv = &i2so_platform_dev,
	.dailinks = i2so_platform_dais,
	.num_dailinks = ARRAY_SIZE(i2so_platform_dais),
	.dais = LIST_HEAD_INIT(i2so_platform.dais),
};

int i2so_platform_init(void)
{
	int np;
	u32 vol, fade_step, mute_polar;
	struct sysdata sysdata;

	if (fdt_node_probe_by_path("/hcrtos/i2s") < 0)
		return 0;

	np = fdt_node_probe_by_path("/hcrtos/i2so");
	if (np < 0) {
		return 0;
	}

	mute_polar = 0;
	i2so_platform_dev.pinmux_data = fdt_get_property_pinmux(np, "data");
	i2so_platform_dev.pinmux_mute = fdt_get_property_pinmux(np, "mute");
	fdt_get_property_u_32_index(np, "mute-polar", 0, &mute_polar);
	i2so_platform_dev.mute_polar = mute_polar;
	if (i2so_platform_dev.pinmux_mute) {
		gpio_configure(i2so_platform_dev.pinmux_mute->settings[0].pin,
			       GPIO_DIR_OUTPUT);
		gpio_set_output(i2so_platform_dev.pinmux_mute->settings[0].pin,
				!i2so_platform_dev.mute_polar);
	}

	if (sys_get_sysdata(&sysdata) != 0) {
		fdt_get_property_u_32_index(np, "volume", 0, &vol);
	} else {
		vol = sysdata.volume;
	}

	i2so_platform_dev.volume = (vol <= 100) ? vol : 100;

	if (fdt_get_property_u_32_index(np, "fade-step", 0, &fade_step)) {
		i2so_platform_dev.fade_en = 0;
		i2so_platform_dev.fade_step = 0xff;
	} else {
		i2so_platform_dev.fade_en = 1;
		i2so_platform_dev.fade_step = fade_step;
	}

	return snd_soc_register_platform(&i2so_platform);
}
