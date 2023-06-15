#include <stdio.h>
#include <unistd.h>

#undef putchar
int putchar(int c)
{
	char byte = (char)c;

	fflush(stdout);
	return write(STDOUT_FILENO, &byte, 1);
}
