#define _GNU_SOURCE
#include <unistd.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>

unsigned if_nametoindex(const char *name)
{
	struct ifreq ifr;
	int fd, r;
	
	if ((fd = socket(AF_INET, SOCK_DGRAM|SOCK_CLOEXEC, 0)) < 0) return 0;
	strncpy(ifr.ifr_name, name, sizeof ifr.ifr_name);
	r = ioctl(fd, SIOCGIFINDEX, &ifr);
	close(fd);
	return r < 0 ? 0 : ifr.ifr_ifindex;
}
