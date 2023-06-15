#include <sys/param.h>
#include <netinet/in.h>

extern uint32_t ________swap32(uint32_t n);

uint32_t ntohl(uint32_t n)
{
	return ________swap32(n);
}
