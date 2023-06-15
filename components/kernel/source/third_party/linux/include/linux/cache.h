#ifndef __LINUX_CACHE_H
#define __LINUX_CACHE_H

#ifndef ____cacheline_aligned
#define ____cacheline_aligned __attribute__((__aligned__(32)))
#endif

#endif /* __LINUX_CACHE_H */
