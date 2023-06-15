#include <generated/br2_autoconf.h>
#include <kernel/io.h>

#include <hcuapi/gpio.h>
#include "hc_gpio.h"

static void * ctrlreg[4] = {
	(void *)&GPIOLCTRL,
	(void *)&GPIOBCTRL,
	(void *)&GPIORCTRL,
	(void *)&GPIOTCTRL,
};


static void gpio_config_setin(pinpad_e padctl)
{
	uint32_t bit = BIT(padctl % 32);
	void *reg = ctrlreg[padctl / 32] + DIR_REG;

	REG32_CLR_BIT(reg, bit);
	
	return ;
}

static void gpio_config_setout(pinpad_e padctl)
{
	uint32_t bit = BIT(padctl % 32);
	void *reg = ctrlreg[padctl / 32] + DIR_REG;

	REG32_SET_BIT(reg, bit);
	
	return ;
}

static void gpio_set_output_high(pinpad_e padctl)
{
	uint32_t bit = BIT(padctl % 32);
	void *reg = ctrlreg[padctl / 32] + OUTPUT_VAL_REG;
	
	REG32_SET_BIT(reg, bit);
	
	return ;
}

static void gpio_set_output_low(pinpad_e padctl)
{
	uint32_t bit = BIT(padctl % 32);
	void *reg = ctrlreg[padctl / 32] + OUTPUT_VAL_REG;
	
	REG32_CLR_BIT(reg, bit);
	
	return ;
}

/******************************************************************************
 * Public Function
 *****************************************************************************/
void gpio_set_output(pinpad_e padctl, bool val)
{
	if (padctl >= PINPAD_MAX)
		return;

	if (val)
		gpio_set_output_high(padctl);
	else
		gpio_set_output_low(padctl);
	return ;
}

int gpio_get_input(pinpad_e padctl)
{
	uint32_t bit = padctl % 32;
	void *reg = ctrlreg[padctl / 32] + INPUT_ST_REG;
	
	if (padctl >= PINPAD_MAX)
		return -1;

	return ((REG32_READ(reg) >> bit) & 0x1);	
}

int gpio_configure(pinpad_e padctl, gpio_pinset_t pinset)
{
	if (padctl >= PINPAD_MAX)
		return -1;

	pinmux_configure(padctl, 0);
	switch (pinset & GPIO_DIR_MUSK)
	{
	case GPIO_DIR_INPUT:	
		gpio_config_setin(padctl);
		gpio_config_irq(padctl, pinset);
		break;
	case GPIO_DIR_OUTPUT:
		gpio_config_setout(padctl);
		break;
	default:
		break;
	}
	return 0;
}
