#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>

static void print_usage(const char *prog)
{
	printf("Usage: %s [-iosftenpb]\n", prog);
	puts("  -i --input    input file\n"
	     "  -o --output   output file\n"
	     "  -s --size     bootloader size\n"
	     "  -f --from     bootloader address in sflash\n"
	     "  -t --to       bootloader address in dram\n"
	     "  -e --entry    bootloader entry address\n"
	     "  -n --nand     spi nand boot\n"
	     "  -p --pagesize spi nand pagesize, default 0x800\n"
	     "  -b --erasesize spi nand erasesize, default 0x20000\n");
}

int main(int argc, char *argv[])
{
	uint32_t magic;
	uint32_t chip;
	uint32_t size = 0, from = 0, to = 0, entry = 0;
	uint32_t is_nand = 0, nand_pagesize = 0x800, nand_erasesize = (0x800 * 64);
	char *input = NULL;
	char *output = NULL;
	char *buf = NULL;
	char *ptr = NULL;
	struct stat sb;
	FILE *fpin, *fpout;
	size_t ret = -1;

	opterr = 0;
	optind = 0;

	while (1) {
		static const struct option lopts[] = {
			{ "input",  1, 0, 'i' },
			{ "output", 1, 0, 'o' },
			{ "size",   1, 0, 's' },
			{ "from",   1, 0, 'f' },
			{ "to",     1, 0, 't' },
			{ "entry",  1, 0, 'e' },
			{ "nand",  0, 0, 'n' },
			{ "pagesize",  1, 0, 'p' },
			{ "erasesize",  1, 0, 'b' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "i:o:s:f:t:e:np:b:", lopts, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'i':
			input = optarg;
			break;
		case 'o':
			output = optarg;
			break;
		case 's':
			size = strtoul(optarg, NULL, 0);
			size = ((size + 31) / 32) * 32;
			break;
		case 'f':
			from = strtoul(optarg, NULL, 0);
			break;
		case 't':
			to = strtoul(optarg, NULL, 0);
			to |= 0xa0000000;
			break;
		case 'e':
			entry = strtoul(optarg, NULL, 0);
			entry |= 0xa0000000;
			break;
		case 'n':
			is_nand = 1;
			break;
		case 'p':
			nand_pagesize = strtoul(optarg, NULL, 0);
			break;
		case 'b':
			nand_erasesize = strtoul(optarg, NULL, 0);
			break;
		default:
			print_usage(argv[0]);
			return -1;
		}
	}

	if (!input) {
		printf("No input file!\n");
		print_usage(argv[0]);
		return -1;
	}

	if (stat(input, &sb) == -1) {
		printf("stat %s fail", input);
		return -1;
	}

	if (!output) {
		fpin = fpout = fopen(input, "wb+");
	} else {
		fpin = fopen(input, "rb");
		fpout = fopen(output, "wb");
	}

	if (is_nand) {
		buf = malloc((unsigned int)sb.st_size + nand_pagesize);
		memset(buf, 0, (unsigned int)sb.st_size + nand_pagesize);
		*(uint32_t *)(buf + 0x0) = 0xaeeaaeea;
		*(uint32_t *)(buf + 0x4) = 0xaeeaaeea;
		ptr = buf + nand_pagesize;
	} else {
		buf = malloc((unsigned int)sb.st_size);
		memset(buf, 0, (unsigned int)sb.st_size);
		ptr = buf;
	}

	fseek(fpin, 0, SEEK_SET);
	if ((ret = fread(ptr, 1, (unsigned int)sb.st_size, fpin)) < 0) {
		printf("read input %s failed\n", input);
		free(buf);
		return -1;
	}

	printf("fixup size %d\n", size);
	printf("fixup bootloader flash address 0x%08x\n", from);
	printf("fixup bootloader dram address  0x%08x\n", to);
	printf("fixup bootloader entry address 0x%08x\n", entry);

	magic = *(uint32_t *)(ptr + 0x10);
	if (magic == 0x5a5aa5a5) {
		if (is_nand) {
			*(uint32_t *)(ptr + 0x10 + 0x4) = 0xeaaeeaae;
			*(uint32_t *)(ptr + 0x10 + 0x8) = nand_pagesize;
			*(uint32_t *)(ptr + 0x10 + 0xc) = nand_erasesize;
		}
		*(uint32_t *)(ptr + 0x20 + 0x0) = size;
		*(uint32_t *)(ptr + 0x20 + 0x4) = to;
		*(uint32_t *)(ptr + 0x20 + 0x8) = from;
		*(uint32_t *)(ptr + 0x20 + 0xc) = entry;
	} else {
		chip = *(uint32_t *)(ptr + 0x14);
		if (chip == 0x1512) {
			*(uint8_t *)(ptr + 0x814) = (size & 0x00ff0000) >> 16;
			*(uint8_t *)(ptr + 0x819) = (size & 0x0000ff00) >> 8;
			*(uint8_t *)(ptr + 0x818) = (size & 0x000000ff);

			*(uint8_t *)(ptr + 0x810) = (entry & 0x00ff0000) >> 16;
			*(uint8_t *)(ptr + 0x811) = (entry & 0xff000000) >> 24;
			*(uint8_t *)(ptr + 0x84c) = (entry & 0x00ff0000) >> 16;
			*(uint8_t *)(ptr + 0x84d) = (entry & 0xff000000) >> 24;
		} else if (chip == 0x1600) {
			*(uint8_t *)(ptr + 0x814) = (size & 0x00ff0000) >> 16;
			*(uint8_t *)(ptr + 0x819) = (size & 0x0000ff00) >> 8;
			*(uint8_t *)(ptr + 0x818) = (size & 0x000000ff);

			*(uint8_t *)(ptr + 0x810) = (entry & 0x00ff0000) >> 16;
			*(uint8_t *)(ptr + 0x811) = (entry & 0xff000000) >> 24;
			*(uint8_t *)(ptr + 0x83c) = (entry & 0x00ff0000) >> 16;
			*(uint8_t *)(ptr + 0x83d) = (entry & 0xff000000) >> 24;
		}
	}

	fseek(fpout, 0, SEEK_SET);
	fwrite(buf, 1, (unsigned int)sb.st_size, fpout);

	if (!output) {
		fclose(fpout);
	} else {
		fclose(fpin);
		fclose(fpout);
	}

	free(buf);
	return 0;
}

