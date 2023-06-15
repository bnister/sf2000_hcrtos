#ifndef _LINUX_PAGEMAP_H
#define _LINUX_PAGEMAP_H

#include <asm-generic/page.h>

/*
 * The page cache can be done in larger chunks than
 * one page, because it allows for more efficient
 * throughput (it can then be mapped into user
 * space in smaller chunks for same flexibility).
 *
 * Or rather, it _will_ be done in larger chunks.
 */
#define PAGE_CACHE_SHIFT	PAGE_SHIFT
#define PAGE_CACHE_SIZE		PAGE_SIZE
#define PAGE_CACHE_MASK		PAGE_MASK
#define PAGE_CACHE_ALIGN(addr)  (((addr) + PAGE_CACHE_SIZE - 1) & PAGE_CACHE_MASK)

#endif

