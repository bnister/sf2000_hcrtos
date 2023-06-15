#ifndef _ADAPT_SYS_TYPES_H
#define _ADAPT_SYS_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif
#include_next <sys/types.h>

typedef off_t off64_t;
typedef unsigned short sa_family_t;
typedef unsigned socklen_t;
struct iovec { void *iov_base; size_t iov_len; };

#ifdef __cplusplus
}
#endif

#endif
