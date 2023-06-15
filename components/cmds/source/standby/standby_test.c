#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <hcuapi/standby.h>
#include <kernel/lib/console.h>

static void print_usage(const char *prog)
{
	printf("Usage: %s [-igaspPcLH]\n", prog);
	puts("  -i --ir       enable wakeup by ir, default [enabled]\n"
	     "  -g --gpio     enable wakeup by gpio, default [disabled]\n"
	     "  -a --saradc   enable wakeup by saradc, default [disabled]\n"
	     "  -s --scancode ir scancode for wakeup, default [28]\n"
	     "  -p --pinpad   gpio pinpad for wakeup, default [96]\n"
	     "  -P --polarity gpio pin polarity for wakeup, default [0]\n"
	     "  -c --channel  saradc channel for wakeup, default [0]\n"
	     "  -L --min      min saradc sample value, default [1300]\n"
	     "  -H --max      max saradc sample value, default [1500]\n");
}

static int standby_test(int argc, char **argv)
{
	int fd;
	struct standby_ir_setting ir = { 0 };
	struct standby_gpio_setting gpio = { 0 };
	struct standby_saradc_setting adc = { 0 };
	struct standby_pwroff_ddr_setting ddr = { 0 };
	bool ir_enabled = 0;
	bool gpio_enabled = 0;
	bool adc_enabled = 0;
	bool ddr_enabled = 0;

	opterr = 0;
	optind = 0;

	fd = open("/dev/standby", O_RDWR);
	if (fd < 0) {
		return -1;
	}

	while (1) {
		static const struct option lopts[] = {
			{ "ir",      	1, 0, 'i' },
			{ "gpio",    	1, 0, 'g' },
			{ "saradc",  	1, 0, 'a' },
			{ "scancode",	1, 0, 's' },
			{ "pinpad",  	1, 0, 'p' },
			{ "polarity",	1, 0, 'P' },
			{ "channel",	1, 0, 'c' },
			{ "min",	1, 0, 'L' },
			{ "max",	1, 0, 'H' },
			{ "ddr",	1, 0, 'D' },
			{ "ddrpin",	1, 0, 'd' },
			{ "ddrpolarity",1, 0, 'y' }, 
			{ NULL,		0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "igas:p:P:c:L:H:D:d:y", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'i':
			ir_enabled = true;
			break;
		case 'g':
			gpio_enabled = true;
			break;
		case 'a':
			adc_enabled = true;
			break;
		case 's':
			ir.num_of_scancode = 1;
			ir.scancode[0] = atoi(optarg);
			break;
		case 'p':
			gpio.pin = atoi(optarg);
			break;
		case 'P':
			gpio.polarity = atoi(optarg);
			break;
		case 'c':
			adc.channel = atoi(optarg);
			break;
		case 'L':
			adc.min = atoi(optarg);
			break;
		case 'H':
			adc.max = atoi(optarg);
			break;
		case 'D':
			ddr_enabled = true;
			break;
		case 'd':
			ddr.pin = atoi(optarg);
			break;
		case 'y':
			ddr.polarity = atoi(optarg);
			break;
		default:
			print_usage(argv[0]);
			return -1;
		}
	}

	if (ddr_enabled)
		ioctl(fd, STANDBY_SET_PWROFF_DDR, (unsigned long)&ddr);
	if (ir_enabled)
		ioctl(fd, STANDBY_SET_WAKEUP_BY_IR, (unsigned long)&ir);
	if (adc_enabled)
		ioctl(fd, STANDBY_SET_WAKEUP_BY_SARADC, (unsigned long)&adc);
	if (gpio_enabled)
		ioctl(fd, STANDBY_SET_WAKEUP_BY_GPIO, (unsigned long)&gpio);

	ioctl(fd, STANDBY_ENTER, 0);

	return 0;
}

CONSOLE_CMD(standby, NULL, standby_test, CONSOLE_CMD_MODE_SELF, "standby test operations")
