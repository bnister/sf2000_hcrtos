#include <kernel/module.h>
#include <stdio.h>

#include <kernel/drivers/hc_clk_gate.h>

static int hc_saradc_clk_enable(void)
{
	int ret = 0;

	hc_clk_enable(SAR_ADC_CLK);

	return ret;
}

module_driver(hc_saradc_clk_init, hc_saradc_clk_enable, NULL, 0)
