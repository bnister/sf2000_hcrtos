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
#include <cpu_func.h>
#include <hcuapi/sysdata.h>
#include <kernel/drivers/hc_clk_gate.h>
#include <sys/ioctl.h>
#if !defined(CONFIG_DISABLE_MOUNTPOINT)
#  include <sys/mount.h>
#endif
#include <hcuapi/gpio.h>

#include <errno.h>
#include <nuttx/fs/fs.h>
#include <kernel/completion.h>
#include <kernel/notify.h>
#include <hcuapi/sys-blocking-notify.h>

#include "hcfota.h"

#include "b2b_usb.h"
unsigned int HCFlag = 0;
unsigned int pFlashMax  = 0;
unsigned int pFlashAddr = 0;
unsigned int ulUsbBase  = 0xB8844000;

static int hcfota_report(hcfota_report_event_e event, unsigned long param,
			 unsigned long usrdata)
{
	switch (event) {
	case HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS: {
		printf("\n");
		unsigned char progress_sign[100 + 1];
		int per = param;
		int i;

		for (i = 0; i < 100; i++) {
			if (i < per) {
				progress_sign[i] = '=';
			} else if (per == i) {
				progress_sign[i] = '>';
			} else {
				progress_sign[i] = ' ';
			}
		}

		progress_sign[sizeof(progress_sign) - 1] = '\0';

		printf("\033[1A");
		fflush(stdout);
		printf("\033[K");
		fflush(stdout);
		printf("Upgrading: [%s] %3d%%", progress_sign, per);
		fflush(stdout);
	}

	default:
		break;
	}
}

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

#if defined(BR2_PACKAGE_PREBUILTS_SDMMCDRIVER)
static struct completion mmc_ready;
static int mmc_mount_notify(struct notifier_block *self, unsigned long action,
			    void *dev)
{
	switch (action) {
	case SDMMC_NOTIFY_CONNECT:
		if (strncmp((void *)dev, "mmc", 3) == 0) {
			complete(&mmc_ready);
		}
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block mmc_mount = {
	.notifier_call = mmc_mount_notify,
};
#endif
 
get_data_from_burner_t get_data_from_burner;

static void app_main(void *pvParameters)
{
	int ret;
	char devpath[64];
	uint32_t i = 0;
	uint32_t buf_read_size = 0;
	uint32_t finsh_len = 0, offset = 0;
	void * hcfota_buf = NULL;
	uint32_t hcfota_len = 0;
	unsigned char pCtrlWRStatus[4]={0xAA,0x00,0x97,0x19};

	char *init_mmc[] = {
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

#if defined(CONFIG_B2BUPGRADER_USB)
	if ((*((unsigned int *)0xA0000FF8) == 0xEF12EF12) &&
	    (*((unsigned int *)0xA0000FFC) == 0xDEADDEAD)) {
		HCFlag = 0x01;
		if (*((unsigned int *)0xB8844110) == 0x00)
			ulUsbBase = 0xB8850000;
		USBIntStatusEndpoint(ulUsbBase);
		USBEndpointDataPut(ulUsbBase, INDEX_TO_USB_EP(0x1),
				   pCtrlWRStatus, 4);
		USBEndpointDataSend(ulUsbBase, INDEX_TO_USB_EP(0x1),
				    USB_TRANS_IN);
		pFlashAddr = *((unsigned int *)0xA0000FF0);
		pFlashMax = *((unsigned int *)0xA0000FF4);
	}
#endif

#if defined(CONFIG_B2BUPGRADER_UART)
	if ((*((unsigned int *)0xA0000FF8) == 0xABCDABCD) &&
	    (*((unsigned int *)0xA0000FFC) == 0xDEADDEAD)) {
		HCFlag = 0x02;
		*((unsigned char *)0xB8800096) &= 0xFB;
		pFlashAddr = *((unsigned int *)0xA0000FF0);
		pFlashMax = *((unsigned int *)0xA0000FF4);
		*((unsigned char *)0xB8818300) = 0xAA;
		vTaskDelay(10);
	}
#endif

#if defined(BR2_PACKAGE_PREBUILTS_SDMMCDRIVER)
	init_completion(&mmc_ready);
	sys_register_notify(&mmc_mount);
	assert(module_init2("all", 7, init_mmc) == 0);
	if (wait_for_completion_timeout(&mmc_ready, 100000) == 0) {
		/* timeout */
		//reset();
	}
#else
	assert(module_init2("all", 9, excludes) == 0);
#endif

	/* Set default time zone is GMT+8 */
	setenv("TZ", CONFIG_APP_TIMEZONE, 1);
	tzset();

	HBRomSendStatus(1);

	get_data_from_burner = get_data_form_brom;

	hcfota_memory_b2b((unsigned char*)pFlashAddr, pFlashMax, hcfota_report, (unsigned long)*get_data_from_burner);

	/*DonE*/
//	HBRomSendStatus(0xBB);

	while(1);

	printf("=====>finsh burning!<======\n");
//	reset();


}
