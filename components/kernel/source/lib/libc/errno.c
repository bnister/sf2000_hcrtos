#include <errno.h>

int *__wrap___errno(void)
{
	return &_REENT->_errno;
}
