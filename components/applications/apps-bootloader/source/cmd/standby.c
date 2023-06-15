#define LOG_TAG "boot_standby"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <kernel/hwspinlock.h>
#include <kernel/completion.h>
#include <kernel/lib/console.h>

#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/auddec.h>
#include <hcuapi/viddec.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/dis.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/lib/libfdt/libfdt.h>
#include <hcuapi/hdmi_tx.h>
#include <hcuapi/persistentmem.h>
#include <hcuapi/sysdata.h>
#include <hcuapi/standby.h>
#include <standby.h>
#include <kernel/elog.h>
#include <hcuapi/lvds.h>

int boot_enter_standby(int argc, char *argv[])
{
	int i;
	int fd_standby;
	int lvds_fd;
	standby_bootup_mode_e temp = 0;

	log_d("enter standby! 1\n");

	fd_standby = open("/dev/standby", O_RDWR);
	if (fd_standby < 0) {
		log_e("Open /dev/standby failed!\n");
		return -1;
	}

	log_d("enter standby! 2\n");

	ioctl(fd_standby, STANDBY_GET_BOOTUP_MODE, &temp);
	if (temp == STANDBY_BOOTUP_COLD_BOOT) {
		printf("enter standby!\n");
		usleep(1000 * 500);
#if defined(CONFIG_WDT_AUTO_FEED)
		close_watchdog();
#endif
		ioctl(fd_standby, STANDBY_ENTER, 0);
	}

	close(fd_standby);
	return 0;
}

CONSOLE_CMD(standby, NULL, boot_enter_standby, CONSOLE_CMD_MODE_SELF,
	    "boot enter standby")
