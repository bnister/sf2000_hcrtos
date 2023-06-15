#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>
#include <nuttx/mtd/mtd.h>
#include <freertos/FreeRTOS.h>
#include <kernel/module.h>
#include <kernel/lib/console.h>
#include <image.h>

int mtdloadraw(int argc, char **argv)
{
	off_t seekpos;
	ssize_t nbytes;
	FILE *fp = NULL;
	int ret;
	const char *charname = argv[2];
	void *addr = NULL;
	ssize_t length;
	struct stat sb;

	if (argc < 4) {
		printf("ERROR arguments. Usage: <addr> </dev/mtdblockX> <size>\n");
		return -1;
	}

	addr = (void *)strtoul(argv[1], NULL, 0);
	if (!addr) {
		printf("ERROR: <addr>\n");
		return -1;
	}

	length = (ssize_t)strtoul(argv[3], NULL, 0);
	if (!length) {
		printf("ERROR: <size>\n");
		return -1;
	}

	if (stat(charname, &sb) == -1) {
		printf("%s stat failed\n", charname);
		return -ENOENT;
	}

	length = MIN(length, (ssize_t)sb.st_size);

	fp = fopen(charname, "rb");
	if (!fp) {
		printf("ERROR: open %s\n", charname);
		return -1;
	}

	seekpos = fseek(fp, 0, SEEK_SET);
	if (seekpos != 0) {
		printf("ERROR: lseek to offset %ld failed: %d\n",
		       (unsigned long)0, errno);
		ret = -1;
		goto err_release_fp;
	}

	nbytes = fread(addr, 1, length, fp);
	if (nbytes < 0) {
		printf("ERROR: read from %s failed: %d\n", charname, errno);
		ret = -1;
		goto err_release_fp;
	} else if (nbytes == 0) {
		printf("ERROR: Unexpected end-of file in %s\n", charname);
		ret = -1;
		goto err_release_fp;
	} else if (nbytes != length) {
		printf("ERROR: Unexpected read size from %s: %ld\n", charname,
		       (unsigned long)nbytes);
		ret = -1;
		goto err_release_fp;
	}

	fclose(fp);
	return 0;

err_release_fp:
	fclose(fp);
	return ret;
}

int mtdloaduImage(int argc, char **argv)
{
	const char *charname = argv[1];
	image_header_t hdr;
	off_t seekpos;
	ssize_t nbytes;
	ssize_t length = 0;
	FILE *fp = NULL;
	int ret;

	if (argc < 2) {
		printf("ERROR arguments. Usage: </dev/mtdblockX>\n");
		return -1;
	}

	fp = fopen(charname, "rb");
	if (!fp) {
		printf("ERROR: open %s\n", charname);
		return -1;
	}

	seekpos = fseek(fp, 0, SEEK_SET);
	if (seekpos != 0) {
		printf("ERROR: lseek to offset %ld failed: %d\n",
		       (unsigned long)0, errno);
		ret = -1;
		goto err_release_fp;
	}

	length = image_get_header_size();
	nbytes = fread(&hdr, 1, length, fp);
	if (nbytes != length) {
		printf("ERROR: read from %s failed: %d\n", charname, errno);
		ret = -1;
		goto err_release_fp;
	}

	length = image_get_image_size(&hdr);
	if ((void *)image_load_addr == NULL)
		image_load_addr = (unsigned long)malloc(length);
	else
		image_load_addr = (unsigned long)realloc((void *)image_load_addr, length);
	if (!image_load_addr) {
		printf("ERROR: malloc for image with size %ld\n", length);
		ret = -1;
		goto err_release_fp;
	}
	printf("default image load address = 0x%08lx\n", image_load_addr);

	seekpos = fseek(fp, 0, SEEK_SET);
	if (seekpos != 0) {
		printf("ERROR: lseek to offset %ld failed: %d\n",
		       (unsigned long)0, errno);
		ret = -1;
		goto err_release_fp;
	}

	nbytes = fread((void *)image_load_addr, 1, length, fp);
	if (nbytes != length) {
		printf("ERROR: read from %s failed: %d\n", charname, errno);
		ret = -1;
		goto err_release_fp;
	}

	fclose(fp);
	return 0;

err_release_fp:
	fclose(fp);
	return ret;
}

CONSOLE_CMD(mtdloadraw, NULL, mtdloadraw, CONSOLE_CMD_MODE_SELF, "<addr> </dev/mtdblockX> <size> load raw data from /dev/mtdblockX")
CONSOLE_CMD(mtdloaduImage, NULL, mtdloaduImage, CONSOLE_CMD_MODE_SELF, "</dev/mtdblockX> load uImage from /dev/mtdblockX")
