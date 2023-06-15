#ifndef _LINUX_MM_H
#define _LINUX_MM_H

#include <kernel/io.h>

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr) ALIGN(addr, PAGE_SIZE)

/* test whether an address (unsigned long or pointer) is aligned to PAGE_SIZE */
#define PAGE_ALIGNED(addr) IS_ALIGNED((unsigned long)addr, PAGE_SIZE)

#define is_vmalloc_addr(x) 0

#endif /* _LINUX_MM_H */
