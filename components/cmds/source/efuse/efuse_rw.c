#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <kernel/lib/console.h>
#include <linux/bitops.h>
#include <linux/bits.h>

#include <hcuapi/efuse.h>
#include <hcuapi/chipid.h>

static void print_help(void)
{
	printf("efuse program:\n");
	printf("\t'o' - bits offset (base 10)\n");
	printf("\t'm' - bits mask, default bit ones when missing (base 16)\n");
	printf("\t'w' - number of bits from offset to program (base 10)\n");
	printf("\t'v' - bits value to program (base 16)\n");
	printf("Note :\n");
	printf("\tThe program will read out the efuse bits and only program the bits with current 0 but request 1\n");
}

static void dump_efuse(struct hc_efuse_bit_map *bitmap)
{
	printf("\n================================================\r\n");
	printf(">>chip_vendor<<\r\n");
	printf("\tunique_id0 : 0x%lx\r\n", bitmap->chip_vendor.unique_id0);
	printf("\tunique_id1 : 0x%lx\r\n", bitmap->chip_vendor.unique_id1);
	printf("\thichip_reserve0 : 0x%x\r\n",
	       bitmap->chip_vendor.hichip_reserve0);
	printf("\thichip_reserve1 : 0x%x\r\n",
	       bitmap->chip_vendor.hichip_reserve1);
	printf("\thichip_reserve2 : 0x%x\r\n",
	       bitmap->chip_vendor.hichip_reserve2);
	printf(">>customer<<\r\n");
	printf("\tcontent0 : 0x%lx\r\n", bitmap->customer.content0);
	printf("\tcontent1 : 0x%lx\r\n", bitmap->customer.content1);
	printf("\tcontent2 : 0x%lx\r\n", bitmap->customer.content2);
	printf("\tcontent3 : 0x%lx\r\n", bitmap->customer.content3);
	printf("\tcontent4 : 0x%x\r\n", bitmap->customer.content4);
	printf(">>write protect bit<<\r\n");
	printf("\twp_zone0 : %s\r\n", bitmap->wp.wp_zone0 ? "true" : "false");
	printf("\twp_zone1 : %s\r\n", bitmap->wp.wp_zone1 ? "true" : "false");
	printf("\twp_zone2 : %s\r\n", bitmap->wp.wp_zone2 ? "true" : "false");
	printf("\twp_zone3 : %s\r\n", bitmap->wp.wp_zone3 ? "true" : "false");
	printf("\twp_zone4 : %s\r\n", bitmap->wp.wp_zone4 ? "true" : "false");
	printf("\twp_zone5 : %s\r\n", bitmap->wp.wp_zone5 ? "true" : "false");
	printf("\twp_zone6 : %s\r\n", bitmap->wp.wp_zone6 ? "true" : "false");
	printf("\twp_zone7 : %s\r\n", bitmap->wp.wp_zone7 ? "true" : "false");
	printf("================================================\r\n\n");
}

static int efuse_dump(int argc, char **argv)
{
	int fd;
	struct hc_efuse_bit_map bitmap;

	/* open */
	fd = open("/dev/efuse", O_RDWR);
	if (fd < 0) {
		printf("[error] cannot open /dev/efuse, ret:%d\r\n", fd);
		return -1;
	}

	/* read */
	memset(&bitmap, 0, sizeof(struct hc_efuse_bit_map));
	if (0 > read(fd, &bitmap, sizeof(struct hc_efuse_bit_map))) {
		printf("[error] cannot read /dev/efuse\r\n");
		return -1;
	}

	dump_efuse(&bitmap);
	close(fd);

	return 0;
}

static int efuse_program(int argc, char **argv)
{
	struct hc_efuse_bit_map bitmap;
	struct hc_efuse_bit_map bitmap_program;
	volatile unsigned long offset, mask, width, value;
	unsigned long bit;
	long long tmp;
	char ch;
	int fd;

	opterr = 0;
	optind = 0;

	offset = 0;
	mask = 0;
	width = 0;
	value = 0;

	while ((ch = getopt(argc, argv, "ho:m:v:w:")) != EOF) {
		switch (ch) {
		case 'h':
			print_help();
			return 0;
		case 'o':
			tmp = strtoll(optarg, NULL, 10);
			offset = (unsigned long)tmp;
			break;
		case 'm':
			tmp = strtoll(optarg, NULL, 16);
			mask = (unsigned long)tmp;
			break;
		case 'w':
			tmp = strtoll(optarg, NULL, 10);
			width = (unsigned long)tmp;
			break;
		case 'v':
			tmp = strtoll(optarg, NULL, 16);
			value = (unsigned long)tmp;
			break;
		default:
			printf("Invalid parameter %c\r\n", ch);
			print_help();
			return -1;
		}
	}

	if (value == 0 || width == 0 || (offset + width) > 256) {
		print_help();
		return -1;
	}

	fd = open("/dev/efuse", O_RDWR);
	if (fd < 0) {
		printf("[error] cannot open /dev/efuse, ret:%d\r\n", fd);
		return -1;
	}

	memset(&bitmap, 0, sizeof(struct hc_efuse_bit_map));
	if (0 > read(fd, &bitmap, sizeof(struct hc_efuse_bit_map))) {
		printf("[error] cannot read /dev/efuse\r\n");
		close(fd);
		return -1;
	}

	printf("efuse bits before program:\n");
	dump_efuse(&bitmap);

	memset(&bitmap_program, 0, sizeof(struct hc_efuse_bit_map));
	for (bit = offset; bit < (offset + width); bit++) {
		if (test_bit(bit, (volatile unsigned long *)&bitmap))
			continue;
		if (mask != 0 && !test_bit(bit - offset, &mask))
			continue;
		if (!test_bit(bit - offset, &value))
			continue;
		set_bit(bit, (volatile unsigned long *)&bitmap_program);
	}

	ioctl(fd, EFUSE_PROGRAM, (uint32_t)&bitmap_program);

	memset(&bitmap, 0, sizeof(struct hc_efuse_bit_map));
	if (0 > read(fd, &bitmap, sizeof(struct hc_efuse_bit_map))) {
		printf("[error] cannot read /dev/efuse\r\n");
		close(fd);
		return -1;
	}

	printf("efuse bits after program:\n");
	dump_efuse(&bitmap);

	close(fd);

	return 0;
}

int console_get_chip_id(int argc, char **argv)
{
	int fd, ret;
	enum HC_CHIPID chip;

	fd = open("/dev/efuse", O_RDWR);
	if (fd < 0) {
		printf("[error] cannot open /dev/efuse, ret:%d\r\n", fd);
		return -1;
	}

	ret = ioctl(fd, EFUSE_GET_CHIPID, (uint32_t)&chip);
	if(ret < 0){
		printf("[error] ioctrl CMD EFUSE_GET_CHIP_ID, ret:%d\r\n", fd);
		return -1;
	}

	switch(chip){
		case HICHIP_A3000: printf("chip: A3000\n"); break;
		case HICHIP_A3100: printf("chip: A3100\n"); break;
		case HICHIP_A3200: printf("chip: A3200\n"); break;
		case HICHIP_A3300: printf("chip: A3300\n"); break;
		case HICHIP_A5000: printf("chip: A5000\n"); break;
		case HICHIP_A5100: printf("chip: A5100\n"); break;
		case HICHIP_A5200: printf("chip: A5200\n"); break;

		case HICHIP_B3100: printf("chip: B3100\n"); break;
		case HICHIP_B3200: printf("chip: B3200\n"); break;
		case HICHIP_B3120: printf("chip: B3120\n"); break;

		case HICHIP_C3000: printf("chip: C3000\n"); break;
		case HICHIP_C3100: printf("chip: C3100\n"); break;
		case HICHIP_C5000: printf("chip: C5000\n"); break;
		case HICHIP_C5200: printf("chip: C5200\n"); break;
		case HICHIP_C3200: printf("chip: C3200\n"); break;

		case HICHIP_D3000: printf("chip: D3000\n"); break;
		case HICHIP_D3100: printf("chip: D3100\n"); break;
		case HICHIP_D3200: printf("chip: D3200\n"); break;
		case HICHIP_D5000: printf("chip: D5000\n"); break;
		case HICHIP_D5200: printf("chip: D5200\n"); break;
		case HICHIP_D3101: printf("chip: D3101\n"); break;
		case HICHIP_D3201: printf("chip: D3201\n"); break;

		case HICHIP_E3000: printf("chip: E3000\n"); break;
		case HICHIP_E3100: printf("chip: E3100\n"); break;
		default : printf("chip: NOT KNOWN CHIP ID\n"); break;
	}
	close(fd);
	return 0;
}

#include <kernel/io.h>
#include <hcuapi/chipid.h>

int efuse_burn_chip_id(int argc, char **argv)
{
	int fd, ret;
	enum HC_CHIPID chip;
	struct hc_efuse_bit_map bitmap_program;

	fd = open("/dev/efuse", O_RDWR);
	if (fd < 0) {
		printf("[error] cannot open /dev/efuse, ret:%d\r\n", fd);
		return -1;
	}

	ret = ioctl(fd, EFUSE_GET_CHIPID, (uint32_t)&chip);
	if(ret < 0){
		printf("[error] ioctrl CMD EFUSE_GET_CHIP_ID, ret:%d\r\n", fd);
		return -1;
	}

	// memset(&bitmap_program, 0, sizeof(struct hc_efuse_bit_map));

	// ioctl(fd, EFUSE_PROGRAM, (uint32_t)&bitmap_program);

	close(fd);

	printf("efuse burn finish!!!\n");
	return 0;
}



CONSOLE_CMD(efuse, NULL, NULL, CONSOLE_CMD_MODE_SELF, "hichip efuse bits")
CONSOLE_CMD(dump, "efuse", efuse_dump, CONSOLE_CMD_MODE_SELF, "dump efuse bits")
CONSOLE_CMD(program, "efuse", efuse_program, CONSOLE_CMD_MODE_SELF,
	    "program efuse bits")
CONSOLE_CMD(chip, "efuse", console_get_chip_id, CONSOLE_CMD_MODE_SELF, "Print Chip ID")
CONSOLE_CMD(burn, "efuse", efuse_burn_chip_id, CONSOLE_CMD_MODE_SELF,
	    "program efuse bits for specified chip ID")