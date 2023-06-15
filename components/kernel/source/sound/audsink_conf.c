#include <generated/br2_autoconf.h>
#include <stdlib.h>
#include <hcuapi/audsink.h>
#include <kernel/module.h>
#include <linux/kconfig.h>

int get_config_audsink_output_bitdepth(void)
{
#if IS_ENABLED(CONFIG_AUDSINK_16BIT_OUT)
	return 16;
#elif  IS_ENABLED(CONFIG_AUDSINK_24BIT_OUT)
	return 24;
#else
	return 16;
#endif
}

snd_pcm_format_t get_config_audsink_output_fmt(void)
{
#if IS_ENABLED(CONFIG_AUDSINK_16BIT_OUT)
	return SND_PCM_FORMAT_S16_LE;
#else
	return SND_PCM_FORMAT_S24_LE;
#endif
}
