/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_PAGE_H
#define __ASM_GENERIC_PAGE_H

#include <kernel/io.h>

#define PAGE_SHIFT		12
#define PAGE_SIZE		(1 << PAGE_SHIFT)
#define PAGE_MASK		(~((1 << PAGE_SHIFT) - 1))

#define offset_in_page(p)	((unsigned long)(p) & ~PAGE_MASK)
#define page_to_phys(page)	(dma_addr_t)PHY_ADDR(page)
#define phys_to_page(p)		(void *)MIPS_CACHED_ADDR(p)

#define virt_to_page(kaddr)	(void *)PTR_ALIGN_DOWN(kaddr, PAGE_SIZE)
#define page_address(x)		(void *)PTR_ALIGN_DOWN(x, PAGE_SIZE)

#define page_to_pfn(page)	((unsigned long)(page) / PAGE_SIZE)
#define pfn_to_page(pfn)	(void *)((pfn) * PAGE_SIZE)
#define nth_page(page,n)	pfn_to_page(page_to_pfn((page)) + (n))

#endif /* __ASM_GENERIC_PAGE_H */
