#include <sys/param.h>
#include <netinet/in.h>

extern uint16_t ________swap16(uint16_t n);
uint16_t ntohs(uint16_t n)
{
	return ________swap16(n);
}
