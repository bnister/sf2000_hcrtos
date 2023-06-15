#include <strings.h>

int ffs(int word)
{
	int r = 1;

	if (!word)
		return 0;

	if (!(word & 0xffffu)) {
		word >>= 16;
		r += 16;
	}

	if (!(word & 0xffu)) {
		word >>= 8;
		r += 8;
	}

	if (!(word & 0xfu)) {
		word >>= 4;
		r += 4;
	}

	if (!(word & 0x3u)) {
		word >>= 2;
		r += 2;
	}

	if (!(word & 0x1u)) {
		word >>= 1;
		r += 1;
	}

	return r;
}
