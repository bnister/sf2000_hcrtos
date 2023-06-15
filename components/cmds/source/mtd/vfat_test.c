#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/lib/console.h>

static int vfat_test(int argc, char **argv)
{
	return 0;
}

static int vfat_write_test(int argc, char **argv)
{
	FILE *fp;

	fp = fopen(argv[1], "w");
	if (!fp) {
		perror("Error fopen");
		return -1;
	}

	fprintf(fp, "this is test, hello world\n");
	fclose(fp);

	return 0;
}

static int vfat_read_test(int argc, char **argv)
{
	FILE *fp;
	char str[1024] = { 0 };

	fp = fopen(argv[1], "r");

	if (!fp) {
		perror("Error fopen");
		return -1;
	}

	fgets(str, 1024, fp);

	printf("%s\n", str);

	fclose(fp);

	return 0;
}

CONSOLE_CMD(vfat, NULL, vfat_test, CONSOLE_CMD_MODE_SELF, "vfat test")
CONSOLE_CMD(write, "vfat", vfat_write_test, CONSOLE_CMD_MODE_SELF, "vfat write test")
CONSOLE_CMD(read, "vfat", vfat_read_test, CONSOLE_CMD_MODE_SELF, "vfat read test")
