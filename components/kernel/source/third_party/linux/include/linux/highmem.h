#ifndef _LINUX_HIGHMEM_H
#define _LINUX_HIGHMEM_H

#include <cpu_func.h>
#include <asm-generic/page.h>

static inline void flush_kernel_dcache_page(struct page *page)
{
	cache_flush(page_address(page), PAGE_SIZE);
}

static inline void kunmap(struct page *page)
{
	BUG_ON(in_interrupt());
	return;
}

#define kunmap_atomic(addr)                                     \
do {                                                            \
	__kunmap_atomic(addr);                                  \
} while (0)

#endif /* _LINUX_HIGHMEM_H */
