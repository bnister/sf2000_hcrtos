#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <kernel/module.h>
#include <kernel/lib/console.h>

typedef struct {
	uint32_t date;
	uint16_t clock;
	uint16_t size;
	uint16_t id;
	uint8_t type;
	uint8_t reserve0[5];
} ddr_info_t;

static int get_ddr_info(uint32_t flag, char *buf, ddr_info_t *ddr_info)
{
	int i;
	uint8_t b0 = (flag >> 0) & 0xff;
	uint8_t b1 = (flag >> 8) & 0xff;
	uint8_t b2 = (flag >> 16) & 0xff;
	uint8_t b3 = (flag >> 24) & 0xff;
	uint8_t a0,a1,a2,a3;
	uint32_t ddrinfo;

	for (i = 0x800; i < (8*1024 - 4); i +=4) {
		a0 = buf[i] & 0xff;	
		a1 = buf[i+1] & 0xff;	
		a2 = buf[i+2] & 0xff;	
		a3 = buf[i+3] & 0xff;	
		if ((a0 == b0) && (a1 == b1) && (a2 == b2) && (a3 == b3)) {
			ddr_info->date = *(uint32_t *)&buf[i+4];
			ddrinfo = *(uint32_t *)&buf[i+8];			
			ddr_info->clock = ddrinfo & 0xffff;
			ddr_info->size = (ddrinfo >> 16) & 0xfff;
			ddr_info->type = (ddrinfo >> 28) & 0xf;
			printf("\nDDR Info:\n");
			printf("\tdate:%ld\n", ddr_info->date);
			printf("\ttype:DDR%d\n", ddr_info->type);
			printf("\tsize:%dM\n", ddr_info->size);
			printf("\tclock:%dMHz\n\n", ddr_info->clock);

			return 0;
		}
	}
	return -1;
}

static int meminfo(int argc, char **argv)
{
	int i;
	int fd = -1;
	uint8_t tmp_buf[16] = {0}; 
	ddr_info_t ddr_info;
	uint8_t *p = (uint8_t *)&ddr_info;
	uint32_t info = 0x0df0adba;
	uint32_t magic;
	char *buf = NULL;


	fd = open("/dev/mtdblock0", O_RDONLY);
	if (fd < 0)
		printf("open mtdblock0 fail\n");

	fd = open("/dev/mtdblock0", O_RDONLY);
	if (fd < 0)
		printf("open mtdblock0 fail\n");

	buf = (char *)malloc(8*1024);

	lseek(fd, 0, SEEK_SET);
	read(fd, buf, 8*1024);

	magic = *(uint32_t *)(buf + 0x10);


	if (magic == 0x5a5aa5a5) {
		get_ddr_info(info, buf, &ddr_info);
		goto cl;
	}


	if (*((uint8_t *)0xb8800003) != 0x16)
		goto cl;

	lseek(fd, 0xff0, SEEK_SET);
	read(fd, tmp_buf, 16);
	
	for (i = 0; i < 16; i++) {
		p[i] = tmp_buf[15 - i];
	}

	if (ddr_info.type == 0x3a)
		ddr_info.type = 0x03;
	else
		ddr_info.type = 0x02;

	printf("\nDDR Info:\n");
	printf("\tid:%x\n", ddr_info.id);
	printf("\ttype:DDR%x\n", ddr_info.type);
	printf("\tsize:%xM\n", ddr_info.size);
	printf("\tclock:%xMHz\n\n", ddr_info.clock);

cl:
	free(buf);
	close(fd);
	return 0;
}

static int show_meminfo(void)
{
	

	return meminfo(0, NULL);
}

__initcall(show_meminfo);

CONSOLE_CMD(meminfo, NULL, meminfo, CONSOLE_CMD_MODE_SELF, "show memory info")
