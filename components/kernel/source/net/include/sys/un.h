#ifndef	_SYS_UN_H
#define	_SYS_UN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/features.h>
#include <sys/types.h>

struct sockaddr_un {
	sa_family_t sun_family;
	char sun_path[108];
};

#if defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
size_t strlen(const char *);
#define SUN_LEN(s) (2+strlen((s)->sun_path))
#endif

#ifdef __cplusplus
}
#endif

#endif
