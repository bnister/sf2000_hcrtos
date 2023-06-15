#define LOG_TAG "main"

#include <generated/br2_autoconf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <fcntl.h>
#include <kernel/elog.h>
#include <sys/poll.h>
#include <kernel/module.h>
#include <kernel/io.h>
#include <kernel/lib/console.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/lib/libfdt/libfdt.h>
#include <mtdload.h>
#include <bootm.h>
#include <showlogo.h>
#include <upgrade.h>
#include <cpu_func.h>
#include <hcuapi/sysdata.h>
#include <kernel/drivers/hc_clk_gate.h>
#include <sys/ioctl.h>
#include <standby.h>
#include <backlight.h>
#include <boot_lcd.h>
#include <pq_start.h>
#if !defined(CONFIG_DISABLE_MOUNTPOINT)
#  include <sys/mount.h>
#endif
#include <hcuapi/gpio.h>

#include "usbd_upgrade.h"
#include <errno.h>
#include <nuttx/fs/fs.h>
#include <kernel/completion.h>
#include <kernel/notify.h>
#include <hcuapi/sys-blocking-notify.h>
#include <hcuapi/watchdog.h>

const char *fdt_get_sysmem_path(void)
{
	return "/hcrtos/memory-mapping/bootmem";
}

const char *fdt_get_stdio_path(void)
{
	return "/hcrtos/boot-stdio";
}

const char *fdt_get_fb0_path(void)
{
	return "/hcrtos/boot-fb0";
}

static void app_main(void *pvParameters);
int main(void)
{
	xTaskCreate(app_main, (const char *)"app_main", configTASK_STACK_DEPTH,
		    NULL, portPRI_TASK_NORMAL, NULL);

	vTaskStartScheduler();

	abort();
	return 0;
}

static int get_mtdblock_devpath(char *devpath, int len, const char *partname)
{
	static int np = -1;
	static u32 part_num = 0;
	u32 i = 1;
	const char *label;
	char property[32];

#ifdef CONFIG_BOOT_STARTUP_FROM_EMMC_SD
	if (!strcmp("dtb", partname))
		snprintf(devpath, len, "/dev/mmcblk0p1");
	else if (!strcmp("avp", partname))
		snprintf(devpath, len, "/dev/mmcblk0p2");
	else if (!strcmp("linux", partname))
		snprintf(devpath, len, "/dev/mmcblk0p3");
	return 0;
#endif

	if (np < 0) {
		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash/partitions");
	}

	if (np < 0)
		return -1;

	if (part_num == 0)
		fdt_get_property_u_32_index(np, "part-num", 0, &part_num);

	for (i = 1; i <= part_num; i++) {
		snprintf(property, sizeof(property), "part%d-label", i);
		if (!fdt_get_property_string_index(np, property, 0, &label) &&
		    !strcmp(label, partname)) {
			memset(devpath, 0, len);
			snprintf(devpath, len, "/dev/mtdblock%d", i);
			return i;
		}
	}

	return -1;
}

int sysdata_update_dt(void *dtb)
{
	struct sysdata sysdata = { 0 };
	int np;

	if (sys_get_sysdata(&sysdata)) {
		return -1;
	}

	np = fdt_path_offset(dtb, "/hcrtos/de-engine");
	if (np >= 0) {
		u32 tvtype = sysdata.tvtype;
		if ((int)tvtype >= 0)
			fdt_setprop_u32(dtb, np, "tvtype", tvtype);
	}

	np = fdt_path_offset(dtb, "/hcrtos/i2so");
	if (np >= 0) {
		u32 volume = sysdata.volume;
		if (volume <= 100)
			fdt_setprop_u32(dtb, np, "volume", volume);
	}

	return 0;
}

static void set_gpio_def_st(void)
{
	int np, num_pins, i, pin, pin_val;
	bool val;

	np = fdt_node_probe_by_path("/hcrtos/gpio-out-def");
	if (np < 0)
		return;

	num_pins = 0;
	if (fdt_get_property_data_by_name(np, "gpio-group", &num_pins) == NULL)
		num_pins = 0;

	num_pins >>= 3;

	if (num_pins == 0)
		return;

	for (i = 0; i < num_pins; i++) {
		fdt_get_property_u_32_index(np, "gpio-group", i * 2, &pin);
		fdt_get_property_u_32_index(np, "gpio-group", i * 2 + 1, &pin_val);

		val = !pin_val;
		gpio_configure(pin, GPIO_DIR_OUTPUT);
		gpio_set_output(pin, val);
	}

	return;
}

int close_watchdog(void)
{
	int ret = -1;
	int wdt_fd = -1;

	wdt_fd = open("/dev/watchdog", O_RDWR);
	if (wdt_fd < 0) {
		printf("can't open %s\n", "/dev/watchdog");
		return -1;
	}

	ret = ioctl(wdt_fd, WDIOC_STOP, 0);
	if (!ret) {
		return -1;
	}

	close(wdt_fd);
}


#if defined(CONFIG_BOOT_STARTUP_FROM_EMMC_SD)
static struct completion mmc_ready;

static void load_mmcblk_init(void)
{
	char *excludes[] = {
		/* usb support */
		"usb",
		"musb_driver",
		"hc16xx_driver",
		"hcdisk_driver",
		"usb_storage_driver",
		/* usb gadget support */
		"mass_storage",
		/* usb host support */
		"usb_core",
	};

	assert(module_init2("all", 7, excludes) == 0);
}

static int mmc_connect_notify(struct notifier_block *self, unsigned long action,
		       void *dev)
{
	switch (action) {
	case SDMMC_NOTIFY_CONNECT:
		complete(&mmc_ready);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block mmc_connect = {
       .notifier_call = mmc_connect_notify,
};
#endif

static void app_main(void *pvParameters)
{
	int ret;
	char devpath[64];
	char *excludes[] = {
		/* mmc/sd support */
		"hc15_mmc_device",
		"hcmmc_device",
		/* usb support */
		"usb",
		"musb_driver",
		"hc16xx_driver",
		"hcdisk_driver",
		"usb_storage_driver",
		/* usb gadget support */
		"mass_storage",
		/* usb host support */
		"usb_core",
		};

	hc_clk_disable_all();

	set_gpio_def_st();

	assert(module_init2("all", 9, excludes) == 0);

	/* Set default time zone is GMT+8 */
	setenv("TZ", CONFIG_APP_TIMEZONE, 1);
	tzset();

#if defined(CONFIG_BOOT_USBD_UPGRADE) &&                                \
	defined(CONFIG_BOOT_USBD_UPGRADE_REQUEST_TIME) &&                      \
	!defined(CONFIG_BOOT_UPGRADE_SUPPORT_USBDEVICE)
#if defined(CONFIG_BOOT_USBD_UPGRADE_ENTER_BY_CHAR)
	if (CONFIG_BOOT_USBD_UPGRADE_REQUEST_TIME >= 0 &&
	    wait_any_key_pressed("PRESS ANY KEY ON KEYBOARD TO ENTER USBD UPGRADE MODE")) {
		usbd_upgrade();

		console_init();
		/* Console loop */
		console_start();

		/* Program should not run to here. */
		for (;;);
	}
#elif defined(CONFIG_BOOT_USBD_UPGRADE_ENTER_BY_KEY)
	char *init_usb[] = {
		/* mmc/sd support */
		"hc15_mmc_device",
		"hcmmc_device",
	};

	printf("try to enter usb device upgrade\n");
	if (upgrade_detect_key()) {
		assert(module_init2("all", 2, init_usb) == 0);

		create_usbd_upgarde_task();

		console_init();
		/* Console loop */
		console_start();

		/* Program should not run to here. */
		for (;;);
	}
	printf("try to enter usb device upgrade fail\n");
#endif

#endif

#if defined(CONFIG_BOOT_STARTUP_FROM_EMMC_SD)
	/*mmc init in upgrade detect*/
	init_completion(&mmc_ready);
	sys_register_notify(&mmc_connect);
	load_mmcblk_init();
	if (wait_for_completion_timeout(&mmc_ready, 50000) == 0) {
		/* timeout */
		reset();
	}
#endif

	upgrade_detect();

#if defined(CONFIG_BOOT_STANDBY)
	boot_enter_standby(1, ((char *[]){ "standby" }));
#endif

#if defined(CONFIG_BOOT_PQ_START)
	open_pq_start(1, ((char *[]){ "pq_start" }));
#endif

#if defined(CONFIG_BOOT_LCD)
	open_boot_lcd_init(1,((char *[]){"boot_lcd"}));
#endif

#if defined(CONFIG_BOOT_SHOWLOGO) && !defined(CONFIG_DISABLE_MOUNTPOINT)
	ret = get_mtdblock_devpath(devpath, sizeof(devpath), "eromfs");
	if (ret >= 0)
		ret = mount(devpath, "/etc", "romfs", MS_RDONLY, NULL);

	if (ret >= 0) {
		showlogo(2, ((char *[]){ "showlogo", "/etc/logo.hc" }));
		wait_show_logo_finish_feed();
	}
#endif

#if defined(CONFIG_BOOT_BACKLIGHT)
	open_lcd_backlight(1,((char *[]){"backlight"}));
#endif

#if defined(CONFIG_BOOT_HCRTOS)
	ret = get_mtdblock_devpath(devpath, sizeof(devpath), "firmware");
	if (ret >= 0) {
		mtdloaduImage(2, ((char *[]){ "mtdloaduImage", devpath }));
		if (bootm(NULL, 0, 1, ((char *[]){ "bootm" })))
			upgrade_force();
	}
#elif defined(CONFIG_BOOT_HCLINUX_DUALCORE)
	void *dtb = malloc(0x10000);
	char loadaddr[16] = { 0 };
	char dtbaddr[16] = { 0 };

	sprintf(dtbaddr, "0x%08x", (unsigned int)dtb);
	REG8_WRITE(0xb880006b, 0x1);
	REG32_WRITE(0xb8800004, (uint32_t)dtb);

	ret = get_mtdblock_devpath(devpath, sizeof(devpath), "dtb");
	if (ret >= 0) {
		mtdloadraw(4, ((char *[]){ "mtdloadraw", dtbaddr, devpath, "0x10000" }));
		sysdata_update_dt(dtb);
		cache_flush(dtb, fdt_totalsize(dtb));
	}

	ret = get_mtdblock_devpath(devpath, sizeof(devpath), "avp");
	if (ret >= 0) {
		mtdloaduImage(2, ((char *[]){ "mtdloaduImage", devpath }));
		if (bootm(NULL, 0, 1, ((char *[]){ "bootm" })))
			upgrade_force();

		if (REG8_READ(0xb880006b) != 0x2) {
			/* scpu boot fail */
			reset();
		}
	}

	ret = get_mtdblock_devpath(devpath, sizeof(devpath), "linux");
	if (ret >= 0) {
		mtdloaduImage(2, ((char *[]){ "mtdloaduImage", devpath }));
		sprintf(loadaddr, "0x%08lx", image_load_addr);
		if (bootm(NULL, 0, 4, ((char *[]){ "bootm", loadaddr, "-", dtbaddr })))
			upgrade_force();
	}
#elif defined(CONFIG_BOOT_HCLINUX_SINGLECORE)
	void *dtb = malloc(0x10000);
	char loadaddr[16] = { 0 };
	char dtbaddr[16] = { 0 };

	sprintf(dtbaddr, "0x%08x", (unsigned int)dtb);
	ret = get_mtdblock_devpath(devpath, sizeof(devpath), "dtb");
	if (ret >= 0) {
		mtdloadraw(4, ((char *[]){ "mtdloadraw", dtbaddr, devpath, "0x10000" }));
		sysdata_update_dt(dtb);
		cache_flush(dtb, fdt_totalsize(dtb));
	}

	ret = get_mtdblock_devpath(devpath, sizeof(devpath), "linux");
	if (ret >= 0) {
		mtdloaduImage(2, ((char *[]){ "mtdloaduImage", devpath }));
		sprintf(loadaddr, "0x%08lx", image_load_addr);
		if (bootm(NULL, 0, 4, ((char *[]){ "bootm", loadaddr, "-", dtbaddr })))
			upgrade_force();
	}
#endif

	console_init();
	/* Console loop */
	console_start();

	/* Program should not run to here. */
	for (;;);

	/* Delete current thread. */
	vTaskDelete(NULL);
}
