#include <errno.h>

int wait(int *status)
{
	status = status;
	errno = ECHILD;
	return -1;
}
