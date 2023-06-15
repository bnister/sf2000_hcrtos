#include <generated/br2_autoconf.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <kernel/io.h>
#include <kernel/ld.h>
#include <hcuapi/pinmux.h>
#include <kernel/lib/fdt_api.h>

int pinmux_configure(pinpad_e padctl, pinmux_pinset_t muset)
{
#ifdef CONFIG_SOC_HC16XX
	if (padctl >= PINPAD_T00 && padctl <= PINPAD_T05 && muset == 0) {
		muset |= (0x7 << 3); /* enhance driver capability */
		REG32_SET_BIT(0xb8800184, BIT24);
	}
#endif
	if (padctl < 32)
		REG8_WRITE((uint32_t)&PINMUXL + padctl, muset);
	else if (padctl < 64)  
		REG8_WRITE((uint32_t)&PINMUXB + padctl - 32, muset);
	else if (padctl < 96)
		REG8_WRITE((uint32_t)&PINMUXR + padctl - 64, muset);
	else if (padctl < 128)
		REG8_WRITE((uint32_t)&PINMUXT + padctl - 96, muset);
	else
		return -1;

	return 0;
}

int pinmux_select_setting(struct pinmux_setting *setting)
{
	int i;

	if (!setting)
		return 0;

	for (i = 0; i < setting->num_pins; i++) {
		pinmux_configure(setting->settings[i].pin, setting->settings[i].func);
	}

	return 0;
}

struct pinmux_setting *fdt_get_property_pinmux(int np, const char *name)
{
	u32 i, pin, func, num_pins;
	struct pinmux_setting *setting;
	char node[64];

	if (np < 0)
		return NULL;

	if (!name)
		strncpy(node, "pinmux", sizeof(node));
	else
		snprintf(node, sizeof(node), "pinmux-%s", name);

	num_pins = 0;
	if (fdt_get_property_data_by_name(np, node, &num_pins) == NULL)
		num_pins = 0;
	num_pins >>= 3;

	if (num_pins == 0)
		return NULL;

	setting = malloc(sizeof(struct pinmux_setting) +
			 num_pins * sizeof(struct pinmux_setting_mux));
	if (!setting)
		return NULL;

	setting->num_pins = num_pins;
	for (i = 0; i < num_pins; i++) {
		fdt_get_property_u_32_index(np, node, i * 2, &pin);
		fdt_get_property_u_32_index(np, node, i * 2 + 1, &func);

		setting->settings[i].pin = pin;
		setting->settings[i].func = func;
	}

	return setting;
}
