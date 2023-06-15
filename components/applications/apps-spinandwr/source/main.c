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
#include <hcuapi/gpio.h>
#include <errno.h>
#include <nuttx/fs/fs.h>
#include <kernel/completion.h>
#include <kernel/notify.h>
#include <nuttx/mtd/mtd.h>
#include <sys/ioctl.h>
#include <linux/minmax.h>
#include <errno.h>
#define RAM_CODE_LENGTH         0x800001F0
#define ROM_CODE_START          0x800001F4
#define RAM_CODE_START          0x800001F8
#define RAM_CODE_TYPE		0x800001FC
#define BURN_NOR		0x00000000
#define PRINTF                  soc_printf

extern int soc_printf(const char *format, ...);

static int burn_status = 0;
static int burn_ret = -1;

static int nor_flash_wr(void);
static int nand_flash_wr(void);
static void app_main(void *pvParameters);

int main(void)
{
	unsigned long type = *((unsigned long *)RAM_CODE_TYPE);

	xTaskCreate(app_main, (const char *)"app_main", configTASK_STACK_DEPTH,
			NULL, portPRI_TASK_NORMAL, NULL);

	vTaskStartScheduler();

	abort();
	return 0;
}

static void app_main(void *pvParameters)
{
	int ret;
	unsigned long type = *((unsigned long *)RAM_CODE_TYPE);

	assert(module_init("all") == 0);

	if (type == BURN_NOR) {
		PRINTF("Code Type: Spi Nor\n");
		nor_flash_wr();
	} else {
		PRINTF("Code Type: Spi Nand\n");
		nand_flash_wr();
	}

	/* Set default time zone is GMT+8 */
	setenv("TZ", CONFIG_APP_TIMEZONE, 1);
	tzset();

	console_init();
	/* Console loop */
	console_start();

	/* Program should not run to here. */
	for (;;);

	/* Delete current thread. */
	vTaskDelete(NULL);
}

static int nor_flash_wr(void)
{
	int err;
	int fd = -1;
	int st = 0;

	/* Info */
	unsigned char *p, *temp_buf;
	unsigned long flash_start;
	unsigned long binary_len, i;
	unsigned long len;
	struct mtd_geometry_s geo = { 0 };
	struct mtd_eraseinfo_s erase;

	p = (unsigned char *)(*((unsigned long *)RAM_CODE_START));
	binary_len = *((unsigned long *)RAM_CODE_LENGTH);
	flash_start = *((unsigned long *)ROM_CODE_START);
	size_t segment = 0x10000, ret;
	burn_status = 1;

	fd = open("/dev/mtdblock0", O_RDWR);
	if (fd < 0) {
		PRINTF("open mtdblock0 fail\n");
		return -ENOENT;
	}

	err = ioctl(fd, MTDIOC_GEOMETRY, &geo);
	if (err < 0) {
		close(fd);
		PRINTF("MTDIOC_GEOMETERY fail\n");
		return -ENOENT;
	}

	segment = max(segment, geo.erasesize);

	temp_buf = (unsigned char *)malloc(segment);

	PRINTF("Flash Writer: source=%p, target(Offset)=0x%08x, length=0x%08x\n",p, flash_start, binary_len);
	for (i = flash_start; i < binary_len; i += segment) {
		lseek(fd, i, SEEK_SET);
		read(fd, temp_buf, segment);

		if (memcmp(temp_buf, p, segment) != 0x00) {
			lseek(fd, i, SEEK_SET);
			ret = write(fd, p, segment);
			if (ret != segment) {
				PRINTF("%08x FAIL\n", i);
				printf("%08lx FAIL\n", i);
				st = -1;
			} else {
				PRINTF("%08x OK!\n", i);
				printf("%08lx OK!\n", i);
			}
		} else {

			printf("%08lx OK!\n", i);
			PRINTF("%08x OK!\n", i);
		}
		p += segment;
	}

	if(!st)
		PRINTF("NOR FLASH Burnning Done!\n");
	else
		PRINTF("NOR FLASH Burnning Fail!\n");

	close(fd);
	return 0;
}

static int nand_flash_wr(void)
{
	int fd = -1;
	ssize_t ret = 0;

	unsigned char *pbuf;
	unsigned int sfaddr;
	unsigned int length;
	unsigned int copied = 0;
	unsigned int i;
	struct mtd_geometry_s geo = { 0 };
	struct mtd_eraseinfo_s erase;

	pbuf = (unsigned char *)(*((unsigned long *)RAM_CODE_START));
	length = *((unsigned long *)RAM_CODE_LENGTH);
	sfaddr = *((unsigned long *)ROM_CODE_START);

	fd = open("/dev/mtdblock0", O_RDWR);
	if (fd < 0) {
		PRINTF("open mtdblock0 fail\n");
		return -ENOENT;
	}

	if (ioctl(fd, MTDIOC_GEOMETRY, &geo) < 0) {
		close(fd);
		PRINTF("MTDIOC_GEOMETERY fail\n");
		return -ENOENT;
	}

	if (sfaddr % geo.erasesize != 0) {
		close(fd);
		PRINTF("flash start address is not aligend with flash erasesize fail\n");
		return -EFAULT;
	}

	erase.length = geo.erasesize;

	PRINTF("start... \n");

	erase.start = sfaddr;

	while (copied < length) {
		if (ioctl(fd, MTDIOC_MEMERASE, &erase) == 0) {
			lseek(fd, erase.start, SEEK_SET);
			write(fd, pbuf + copied, erase.length);
			copied += erase.length;
			PRINTF("programe 0x%08x, size 0x%x...\n", erase.start, erase.length);
		} else {
			ioctl(fd, MTDIOC_MAKEBAD, erase.start);
			PRINTF("badblock found 0x%08x...\n", erase.start);
		}
		erase.start += erase.length;
	}

	PRINTF("program done\n");

	close(fd);
	while (1);
	return 0;

}
