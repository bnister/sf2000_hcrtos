#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <kernel/ld.h>
#include <kernel/io.h>
#include <kernel/vfs.h>
#include <kernel/module.h>
#include <nuttx/wqueue.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <hcuapi/watchdog.h>
#include "wdt_reg_struct.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/lib/console.h>

#define LOG_TAG "wdt"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR
#include <kernel/elog.h>

/*
 * WDT clocksF27M_CLK(Crystal clock,27 MHz)
 * Timer clockF27M_CLK(Crystal clock,27 MHz)
 */
#define WDT_DEFAULT_FREQ (27000000)
#define WDT_MAX_CNT (0xFFFFFFFF)

#define usec2tick(usec) ((usec)*27 / 32)
#define tick2usec(tick) ((tick)*32 / 27)

struct wdt_device {
	wdt_reg_t *reg;
	int irq;
	int refs;
	int mode;
	struct work_s work;
	unsigned int timebase;
	unsigned int timeout;
};

static void wdt_interrupt(uint32_t data)
{
	struct wdt_device *dev = (struct wdt_device *)data;

	work_notifier_signal2(WDIOC_NOTIFY_TIMEOUT, NULL, 0);

	dev->reg->count = dev->timebase;
}

static void wdt_stop(struct wdt_device *dev)
{
	dev->reg->count = 0;
	dev->reg->conf.val = 0;
}

static int wdt_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	struct inode *inode = filep->f_inode;
	struct wdt_device *dev = inode->i_private;
	int ret = 0;

	if (!dev) {
		return -EIO;
	}

	switch (cmd) {
	case WDIOC_START:
		dev->timebase = WDT_MAX_CNT - usec2tick(dev->timeout);
		dev->reg->count = dev->timebase;
		dev->reg->conf.en = 1;
		dev->reg->conf.wdten = (dev->mode == WDT_MODE_WATCHDOG);
		dev->reg->conf.ien = (dev->mode == WDT_MODE_TIMER);
		break;
	case WDIOC_STOP:
		dev->reg->conf.val = 0;
		break;
	case WDIOC_SETMODE: {
		int mode = arg & (WDT_MODE_WATCHDOG | WDT_MODE_TIMER);
		if (mode) {
			dev->mode = mode;
			dev->reg->conf.wdten = (dev->mode == WDT_MODE_WATCHDOG);
			dev->reg->conf.ien = (dev->mode == WDT_MODE_TIMER);
		} else {
			return -EINVAL;
		}
		break;
	}
	case WDIOC_KEEPALIVE:
		dev->reg->count = dev->timebase;
		break;
	case WDIOC_SETTIMEOUT:
		dev->timeout = (unsigned int)arg;
		dev->timebase = WDT_MAX_CNT - usec2tick((unsigned int)arg);
		dev->reg->count = dev->timebase;
		break;
	case WDIOC_GETTIMEOUT:
		*(unsigned int *)arg = dev->timeout;
		break;
	case WDIOC_GETTIMELEFT:
		*(unsigned int *)arg = tick2usec(WDT_MAX_CNT - dev->reg->count);
		break;
	default:
		return -EIO;
	}

	return ret;
}

static int wdt_open(struct file *filep)
{
	struct inode *inode = filep->f_inode;
	struct wdt_device *dev = inode->i_private;

	dev->refs++;

	return 0;
}

static int wdt_close(struct file *filep)
{
	struct inode *inode = filep->f_inode;
	struct wdt_device *dev = inode->i_private;

	if (dev->refs > 0)
		dev->refs--;

	if (dev->refs == 0 && dev->mode == WDT_MODE_TIMER) {
		dev->reg->conf.ien = 0;
	}

	return 0;
}

static const struct file_operations wdt_fops = {
	.open = wdt_open, /* open */
	.close = wdt_close, /* close */
	.read = dummy_read, /* read */
	.write = dummy_write, /* write */
	.seek = NULL, /* seek */
	.ioctl = wdt_ioctl, /* ioctl */
	.poll = NULL /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	,
	.unlink = NULL /* unlink */
#endif
};

#if defined(CONFIG_WDT_AUTO_FEED)

static void wdt_auto_feed(uint32_t param)
{
	struct wdt_device *dev = (struct wdt_device *)param;
	dev->reg->count = dev->timebase;
}

static void wdt_start_auto_feed(void *parameter)
{
	struct wdt_device *dev = (struct wdt_device *)parameter;

	dev->timebase = WDT_MAX_CNT - usec2tick(dev->timeout);
	dev->reg->count = dev->timebase;
	dev->reg->conf.en = 1;
	dev->reg->conf.wdten = (dev->mode == WDT_MODE_WATCHDOG);
	dev->reg->conf.ien = (dev->mode == WDT_MODE_TIMER);

	vApplicationIdleHookRegister(wdt_auto_feed, (uint32_t)parameter);
}
#endif

struct wdt_device *dev;

static int wdt_init(void)
{
	dev = malloc(sizeof(struct wdt_device));
	if (!dev) {
		return -ENOMEM;
	}

	memset(dev, 0, sizeof(struct wdt_device));

	dev->timeout = CONFIG_WDT_TIMEOUT * 1000;
	dev->reg = (wdt_reg_t *)&WDT0;
	dev->irq = (int)&WTD_INTR;

#if defined(CONFIG_WDT_MODE_WATCHDOG)
	dev->mode = WDT_MODE_WATCHDOG;
#elif defined(CONFIG_WDT_MODE_TIMER)
	dev->mode = WDT_MODE_TIMER;
#endif

	dev->reg->conf.val = 0;
	dev->reg->count = 0;
	dev->reg->conf.clkdiv = 0;

	xPortInterruptInstallISR(dev->irq, wdt_interrupt, (uint32_t)dev);

	register_driver("/dev/watchdog", &wdt_fops, 0666, dev);

#if defined(CONFIG_WDT_AUTO_FEED)
	work_queue(HPWORK, &dev->work, wdt_start_auto_feed, (void *)dev, 10);
#endif

	return 0;
}

module_arch(wdt, wdt_init, NULL, 0)

#if defined(CONFIG_WDT_MODE_WATCHDOG)
CONSOLE_CMD(wdt, NULL, NULL, CONSOLE_CMD_MODE_SELF, "watchdog cmds entry")

static int wdt_show_status_cmd(int argc, char *argv[])
{
	printf("\twdt status 	is %s !\n",dev->reg->conf.en? "running":"stop");
	printf("\twdt timeout 	is %d ms!\n",dev->timeout / 1000);
	printf("\twdt timeleft 	is %ld ms!\n",(tick2usec(WDT_MAX_CNT - dev->reg->count)) / 1000);

	return 0;
}
CONSOLE_CMD(status, "wdt", wdt_show_status_cmd, CONSOLE_CMD_MODE_SELF, "start watchdog")

static int wdt_stop_cmd(int argc, char *argv[])
{
	static int wdt_fd = -1;

	wdt_fd = open("/dev/watchdog", O_RDWR);

	ioctl(wdt_fd, WDIOC_STOP, 0);

	wdt_show_status_cmd(0, NULL);

	close(wdt_fd);

	return 0;
}
CONSOLE_CMD(stop, "wdt", wdt_stop_cmd, CONSOLE_CMD_MODE_SELF, "stop watchdog")

static int wdt_start_cmd(int argc, char *argv[])
{
	static int wdt_fd = -1;

	wdt_fd = open("/dev/watchdog", O_RDWR);

	ioctl(wdt_fd, WDIOC_START, 0);

	close(wdt_fd);

	wdt_show_status_cmd(0, NULL);

	return 0;
}
CONSOLE_CMD(start, "wdt", wdt_start_cmd, CONSOLE_CMD_MODE_SELF, "start watchdog")

static void print_help(void) {
	printf("-----------------------------------\n");
	printf("wdt settimeout cmds help\n");
	printf("\tfor example : settimeout -t 10000\n");
	printf("\t't'	10000 means 10s\n");
	printf("\tsettimeout range is 1ms-120s\n");
	printf("-----------------------------------\n");
}

static int wdt_settimeout_cmd(int argc, char *argv[])
{
	char ch;
	opterr = 0;
	optind = 0;

	static int wdt_fd = -1;
	uint32_t timeout = 0;

	if (argc < 2) {
		print_help();
		return -1;
	}

	while ((ch = getopt(argc, argv, "ht:")) != EOF) {
		switch (ch) {
		case 'h':
			print_help();
			return 0;
		case 't':
			timeout = strtoll(optarg, NULL, 10);
			break;
		default:
			printf("Invalid parameter %c\r\n", ch);
			print_help();
			return -1;
		}
	}

	if (timeout > 120000) {
		printf("timeout value over range\n");
		print_help();
		return -1;
	}

	wdt_fd = open("/dev/watchdog", O_RDWR);

	ioctl(wdt_fd, WDIOC_SETTIMEOUT,timeout * 1000);

	close(wdt_fd);

	wdt_show_status_cmd(0, NULL);

	return 0;
}
CONSOLE_CMD(settimeout, "wdt", wdt_settimeout_cmd, CONSOLE_CMD_MODE_SELF, "start watchdog")
#endif
