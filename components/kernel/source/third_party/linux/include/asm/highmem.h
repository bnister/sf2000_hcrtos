#ifndef _ASM_HIGHMEM_H
#define _ASM_HIGHMEM_H

#define kmap(page) ((void *)(page))
#define kunmap(page) do{}while(0)

#define kmap_atomic(page) ((void *)(page))
#define kunmap_atomic(kvaddr) do {} while (0)
#define flush_kernel_dcache_page(page) do {} while (0)

#endif /* _ASM_HIGHMEM_H */
