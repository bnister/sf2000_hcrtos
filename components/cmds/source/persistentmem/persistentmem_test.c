#define LOG_TAG "persistentmem"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <kernel/lib/console.h>
#include <kernel/elog.h>
#include <kernel/module.h>

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define PERSISTENTMEM_SIZE 2048

int persistentmem_test(int argc, char *argv[])
{
	int i, cnt, ret;
	uint8_t val;
	off_t offset;
	size_t nbytes;
	uint8_t tmp_buf[PERSISTENTMEM_SIZE];
	uint8_t buf[PERSISTENTMEM_SIZE];
	int fd;

	fd = open("/dev/persistentmem", O_RDWR);
	if (fd < 0) {
		log_e("Open /dev/persistentmem failed (%d)\n", fd);
		return -1;
	}

	cnt = 1000;

	memset(tmp_buf, 0, sizeof(tmp_buf));

	lseek(fd, 0, SEEK_SET);
	write(fd, tmp_buf, sizeof(tmp_buf));

	for (i = 0; i < cnt; i++) {
		val = rand() & 0xff;
		offset = rand() & (PERSISTENTMEM_SIZE - 1);

		while (!(nbytes = rand() & (PERSISTENTMEM_SIZE - 1)))
			nbytes = rand() & (PERSISTENTMEM_SIZE - 1);
		nbytes = min(nbytes, (PERSISTENTMEM_SIZE - 1) - offset);

		memset(buf, val, nbytes);

		lseek(fd, offset, SEEK_SET);
		write(fd, buf, nbytes);
		memcpy(tmp_buf + offset, buf, nbytes);
	}

	close(fd);

	module_exit("persistentmem");
	module_init("persistentmem");

	fd = open("/dev/persistentmem", O_RDWR);
	if (fd < 0) {
		log_e("Open /dev/persistentmem failed after module init (%d)\n", fd);
		return -1;
	}

	memset(buf, 0, sizeof(buf));

	lseek(fd, 0, SEEK_SET);
	read(fd, buf, sizeof(buf));

	if (memcmp(buf, tmp_buf, sizeof(buf))) {
		log_e("persistent memory test failed!\n");
		elog_hexdump("Expected:", 16, tmp_buf, sizeof(tmp_buf));
		elog_hexdump("Read:", 16, buf, sizeof(buf));
		return -1;
	} else {
		log_e("persistent memory test success!\n");
	}

	return 0;
}

CONSOLE_CMD(persistentmem, NULL, persistentmem_test, CONSOLE_CMD_MODE_SELF, "persistentmem test commands")
