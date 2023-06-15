#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <hcuapi/dsc.h>


static uint8_t key[16] = {0x11,0x22,0x23,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xe,0xf};
static uint8_t iv[16] = {0,11,22,33,44,55,66,77,88,99,10,11,12,13,14,15};
static uint8_t input[1024*1024] = {0};
static uint8_t output[32] = {0};

static void dump_bin(char *prefix, uint8_t *data, int size)
{
	if(!data)
		return;
	printf("%s", prefix);
	for (int i = 0; i < size; i++)
		printf("%02x ", data[i]);
	printf("\n");
}

int main(int argc, char *argv[])
{
	struct dsc_sha_params algo_params = {
		.type = DSC_SHA_TYPE_256,
		.input = {
			.buffer = input,
			.size = 1024*1024,
		},
		.output = {
			.buffer = output,
			.size = 32,
		},
	};

	FILE *fin = NULL;
	FILE *fout = NULL;
	size_t ret;

	if(argc != 3){
		printf("Usage: %s <input file> <output file>\n", argv[0]);
		exit(-1);
	}

	fin = fopen(argv[1], "rb");
	fout = fopen(argv[2], "wb");

	int fd = open("/dev/dsc", O_RDWR);
	if(fd<0){
		printf("Open /dev/dsc error, %s\n", strerror(errno));
		exit(-1);
	}

	ret = fread(input, 1, 1024*1024, fin);
	if(ret < 0){
		printf("read file error\n");
	}else if(ret == 0){
		printf("read eos\n");
	}
	ioctl(fd, DSC_DO_SHA, &algo_params);
	fwrite(output, 1, 32, fout);

	close(fd);
	fflush(fout);
	fclose(fin);
	fclose(fout);
	return 0;
}
