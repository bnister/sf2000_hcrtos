#include <stdio.h>
#include <stdint.h>

int main(void) {
	uint32_t tab[256], c;
	int i, j;
	FILE *pf = fopen("bisrv.asd", "rb+");
	static uint8_t buf[8192];
	size_t sz;

	for (i = 0; i < 256; i++) {
		c = (unsigned long)i << 24;
		for (j = 0; j < 8; j++)
			c = c & (1 << 31) ? c << 1 ^ 0x4c11db7 : c << 1;
		tab[i] = c;
	}
	c = 0xffffffff;
	fseek(pf, 512, SEEK_SET);
	while ((sz = fread(buf, 1, sizeof buf, pf)) > 0) {
		for (i = 0; i < sz; i++) c = c << 8 ^ tab[c >> 24 ^ buf[i]];
	}
	buf[0] = c & 255;
	buf[1] = c >> 8 & 255;
	buf[2] = c >> 16 & 255;
	buf[3] = c >> 24;
	fseek(pf, 0x18c, SEEK_SET);
	fwrite(buf, 1, 4, pf);
	fclose(pf);
	return 0;
}
