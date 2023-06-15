#include <errno.h>

int execve(char *name, char **argv, char **env)
{
	name = name;
	argv = argv;
	env = env;
	errno = ENOMEM;
	return -1;
}
