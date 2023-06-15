#include <lwip/netdb.h>

struct hostent *
gethostbyname(const char *name)
{
	return lwip_gethostbyname(name);
}

struct hostent *gethostbyaddr(const void *a, socklen_t l, int af)
{
	asm volatile("nop;.word 0x1000ffff;nop;");
	return NULL;
}

int getaddrinfo (const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
	return lwip_getaddrinfo(node,service,hints,res);
}

void freeaddrinfo (struct addrinfo *res)
{
	return lwip_freeaddrinfo(res);
}

