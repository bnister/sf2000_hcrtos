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
static uint8_t input[4096] = {0};
static uint8_t output[4096] = {0};

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
	struct dsc_algo_params algo_params = {
		.algo_type = DSC_ALGO_AES,
		.crypto_mode = DSC_DECRYPT,
		.chaining_mode = DSC_MODE_CTR, //output: 0a 94 3f d0 a0 99 33 fc 75 c7 6c d5 cb 14 76 0b
		.residue_mode  = DSC_RESIDUE_CLEAR,
		.key_mode = KEY_FROM_SRAM,
		.otp_key_mode = 0,
		.key = {key, 16},
		.iv = {iv, 16},
		.input = NULL,
		.output = NULL,
	};
	struct dsc_crypt out = {
		.input = input,
		.output = output,
		.size = 4096
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

	if(ioctl(fd, DSC_CONFIG, &algo_params) < 0){
		printf("ioctl error, %s\n", strerror(errno));
		exit(-1);
	}

	dump_bin("key:", key, 16);
	dump_bin("iv:", iv, 16);

	while(1){
		ret = fread(input, 1, 4096, fin);
		if(ret < 0){
			printf("read file error\n");
			break;
		}else if(ret == 0){
			printf("read eos\n");
			break;
		}
		ioctl(fd, DSC_DO_ENCRYPT, &out);
		fwrite(out.output, 1, 4096, fout);
	}
	

	close(fd);
	fflush(fout);
	fclose(fin);
	fclose(fout);
	return 0;
}
