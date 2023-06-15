#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <kernel/lib/console.h>
#include <kernel/module.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <kernel/lib/console.h>

#include <sys/mount.h>
#include <hcuapi/ramdisk.h>
#include <kernel/drivers/hcusb.h>
#include <nuttx/mtd/mtd.h>
#include <nuttx/drivers/ramdisk.h>
#include <nuttx/fs/fs.h>
#include <fsutils/mkfatfs.h>
#include <uapi/hcuapi/mmz.h>
#include <dirent.h>
#include <errno.h>
#include <kernel/lib/libfdt/libfdt.h>
#include <kernel/io.h>

#include <kernel/elog.h>
#define ferr  log_e
#define fwarn log_w
#define finfo log_i

#include "usbd_upgrade.h"
#include "hcfota.h"


#include <hcuapi/fb.h>
#include <kernel/fb.h>
#include <sys/mman.h>
#include <hcuapi/pinpad.h>
#include <hcuapi/gpio.h>
#include <dt-bindings/gpio/gpio.h>


#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_SCREEN
void fill_progress_bar(uint32_t len, uint32_t color);
int init_progress_bar(void);
#endif

#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_LED
int led_init(void);
#endif

int hcfota_report(hcfota_report_event_e event, unsigned long param, unsigned long usrdata);

