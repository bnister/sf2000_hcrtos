#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int getentropy(void *buffer, size_t len)
{
	char *pos = buffer;
	long r = 0;
	int ret = 0;

	if (len > 256) {
		errno = EIO;
		return -1;
	}

	while (len) {
		r = random();
		ret = len > sizeof(r) ? sizeof(r) : len;
		memcpy(pos, &r, ret);
		pos += ret;
		len -= ret;
	}

	return 0;
}
