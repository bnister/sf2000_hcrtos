#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <kernel/vfs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <kernel/lib/console.h>
#include <kernel/elog.h>
#include <kernel/module.h>
#include <hcuapi/pinmux.h>
#include <kernel/lib/fdt_api.h>
#include <nuttx/pwm/pwm.h>
#include <kernel/drivers/hc_clk_gate.h>

#define RGB_PING_MUN 28
#define SYSTEM_CLOCK_CONTROL_REG        0xB8800078

static int rgb_probe(const char *node)
{
	int np = 0;
	const char *path = NULL;
	struct pinmux_setting *active_state = NULL;
	u32 frequency = 0, duty = 0, polarity = 0 , tmpVal = 0;
	struct pwm_info_s info = { 0 };
	int fd = 0;
	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return 0;

	active_state = fdt_get_property_pinmux(np, "active");
	if (active_state) {
		pinmux_select_setting(active_state);
		free(active_state);
	}

	if(fdt_get_property_u_32_index(np, "rgb-clk-inv", 0, &tmpVal) == 0)
	{
		printf("rgb-clk-inv = %d\n",tmpVal);
		if(tmpVal == 0)
			REG32_CLR_BIT(SYSTEM_CLOCK_CONTROL_REG, 15);
		else
			REG32_SET_BIT(SYSTEM_CLOCK_CONTROL_REG, 15);
	}

	hc_clk_enable(RGB_CLK);

	if (fdt_get_property_string_index(np, "vcom-pwmdev", 0, &path))
		return 0;
	fdt_get_property_u_32_index(np, "vcom-frequency", 0, &frequency);
	fdt_get_property_u_32_index(np, "vcom-duty", 0, &duty);
	fdt_get_property_u_32_index(np, "polarity", 0, &polarity);
	info.period_ns = 1000000000/frequency;
	info.duty_ns = (info.period_ns/100)*duty;
	info.polarity = polarity;

	printf("vcom-pwmdev = %s vcom-frequency %ld vcom-duty %ld polarity %d",path,info.period_ns,info.duty_ns,info.polarity);
	fd = open(path, O_RDWR);
	if (fd < 0) {
		return 0;
	}

	ioctl(fd, PWMIOC_START, 0);
	ioctl(fd, PWMIOC_SETCHARACTERISTICS, &info);
	return 0;
}

static int rgb_init(void)
{
	return rgb_probe("/hcrtos/rgb");
}

/* rgb depends on pwm driver */
module_driver(rgb, rgb_init, NULL, 1)
