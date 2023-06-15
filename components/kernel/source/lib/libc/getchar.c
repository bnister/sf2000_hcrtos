#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <freertos/FreeRTOS.h>
#include <poll.h>

#undef getchar
int getchar(void)
{
	char byte = 0;
	struct pollfd pfd[1];

	memset(pfd, 0, sizeof(pfd));

	pfd[0].fd = 0;
	pfd[0].events = POLLIN | POLLRDNORM;

	if (poll(pfd, 1, portMAX_DELAY)) {
		if (pfd[0].revents & (POLLRDNORM | POLLIN)) {
			read(STDIN_FILENO, &byte, 1);
		}
	}

	return (int)byte;
}
