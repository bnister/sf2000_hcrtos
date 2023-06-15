#include <errno.h>

int kill(int pid, int sig)
{
	pid = pid;
	sig = sig;
	errno = EINVAL;
	return -1;
}
