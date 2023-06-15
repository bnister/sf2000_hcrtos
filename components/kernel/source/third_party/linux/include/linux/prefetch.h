#ifndef _LINUX_PREFETCH_H
#define _LINUX_PREFETCH_H

#define prefetch(x) __builtin_prefetch(x)

#endif
