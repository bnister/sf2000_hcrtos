/* Copyright (C) 2023 Nikita Burnashev

Permission to use, copy, modify, and/or distribute this software
for any purpose with or without fee is hereby granted.

THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND! */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define IMAGE_START 0x1000
#define LCFG_HEADER_SIZE 0x200

int main(int argc, char *argv[])
{
	uint32_t tab[256], c;
	int i, j;
	FILE *pf;
	uint8_t *buf;
	size_t sz;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s <output/images/avp.bin> <output/images/bisrv.asd>\n", argv[0]);
		return EXIT_FAILURE;
	}

	pf = fopen(argv[1], "rb");
	if (pf == NULL) {
		fprintf(stderr, "Cannot open %s for reading\n", argv[1]);
		return EXIT_FAILURE;
	}
	fseek(pf, 0, SEEK_END);
	sz = ftell(pf);
	buf = malloc(IMAGE_START + sz);
	fseek(pf, 0, SEEK_SET);
	fread(&buf[IMAGE_START], 1, sz, pf);
	fclose(pf);

	memset(buf, 0, IMAGE_START); /* MIPS NOPs */
	sz = IMAGE_START + sz - LCFG_HEADER_SIZE;

	/* CRC-32/MPEG-2 variant https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-32-mpeg-2 */
	for (i = 0; i < 256; i++) {
		c = (unsigned long)i << 24;
		for (j = 0; j < 8; j++) {
			c = c & (1 << 31) ? c << 1 ^ 0x4c11db7 : c << 1;
		}
		tab[i] = c;
	}
	c = 0xffffffff;
	for (i = LCFG_HEADER_SIZE; i < LCFG_HEADER_SIZE + sz; i++) {
		c = c << 8 ^ tab[c >> 24 ^ buf[i]];
	}

	buf[0] = 'L';
	buf[1] = 'C';
	buf[2] = 'F';
	buf[3] = 'G';

	buf[0x184] = sz & 255;
	buf[0x185] = sz >> 8 & 255;
	buf[0x186] = sz >> 16 & 255;
	buf[0x187] = sz >> 24;

	buf[0x18c] = c & 255;
	buf[0x18d] = c >> 8 & 255;
	buf[0x18e] = c >> 16 & 255;
	buf[0x18f] = c >> 24;

	pf = fopen(argv[2], "wb");
	if (pf == NULL) {
		fprintf(stderr, "Cannot open %s for writing\n", argv[2]);
		return EXIT_FAILURE;
	}
	fwrite(buf, 1, LCFG_HEADER_SIZE + sz, pf);
	fclose(pf);

	return EXIT_SUCCESS;
}
