#include <errno.h>

int fork(void)
{
	errno = EAGAIN;
	return -1;
}
