#ifndef _PINMUX_H_
#define _PINMUX_H_

#ifdef __HCRTOS__

#ifdef __cplusplus
extern "C" {
#endif

#include <generated/br2_autoconf.h>
#include <hcuapi/pinpad.h>

#ifdef CONFIG_SOC_HC16XX
#include "pinmux/hc16xx_pinmux.h"
#elif defined(CONFIG_SOC_HC15XX)
#include "pinmux/hc15xx_pinmux.h"
#else
#error "No pin multiplexing"
#endif

typedef unsigned char pinmux_pinset_t;

struct pinmux_setting_mux {
	pinmux_pinset_t func;
	pinpad_e pin;
};

struct pinmux_setting {
	int num_pins;
	struct pinmux_setting_mux settings[0];
};

extern int pinmux_configure(pinpad_e padctl, pinmux_pinset_t muxset);
extern int pinmux_select_setting(struct pinmux_setting *setting);
extern struct pinmux_setting *fdt_get_property_pinmux(int np, const char *name);

#ifdef __cplusplus
}
#endif
#endif
#endif /* _PINMUX_H_ */
