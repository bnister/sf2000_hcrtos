#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define HOST_NAME_MAX_LEN 48
static char _hostname[HOST_NAME_MAX_LEN] = {0};

int gethostname(char *name, size_t len)
{
	if ((name == NULL) || (strlen(_hostname) >= len)) {
		errno = -EINVAL;
		return  -1;
	}

	strcpy(name, _hostname);

	return 0;
}

int sethostname(const char *name, size_t len)
{
	if (name == NULL) {
		errno = -EINVAL;
		return  -1;
	}

	if (len >= HOST_NAME_MAX_LEN) {
		errno = ENAMETOOLONG;
		return -1;
	}

	/* No need to use mutex to protect it, rarely use API. */
	memset(_hostname, 0, HOST_NAME_MAX_LEN);
	memcpy(_hostname, name, len);

	return 0;
}
