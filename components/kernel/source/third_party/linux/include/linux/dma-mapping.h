/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_DMA_MAPPING_H
#define _LINUX_DMA_MAPPING_H

#include <cpu_func.h>
#include <freertos/FreeRTOS.h>
#include <kernel/io.h>
#include <linux/types.h>
#include <linux/dma-direction.h>
#include <linux/bug.h>
#include <asm-generic/page.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/dmapool.h>
#include <linux/device.h>
#include <linux/err.h>

struct device;

#define dma_mapping_error(x, y)	0

#define ARCH_DMA_MINALIGN 32

dma_addr_t dma_map_single(struct device *dev, void *vaddr, size_t len,
				 enum dma_data_direction dir);

void dma_unmap_single(struct device *dev, dma_addr_t addr, size_t len,
				    enum dma_data_direction dir);

#define DMA_BIT_MASK(n)	(((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))

int dma_map_sg(struct device *dev, struct scatterlist *sglist,
	int nents, enum dma_data_direction direction);

void dma_unmap_sg(struct device *dev, struct scatterlist *sglist,
		  int nhwentries, enum dma_data_direction direction);

dma_addr_t dma_map_page(struct device *dev, struct page *page, size_t offset,
			size_t size, enum dma_data_direction dir);

void dma_unmap_page(struct device *dev, dma_addr_t addr, size_t size,
		    enum dma_data_direction dir);

void *dma_alloc_coherent(struct device *dev, size_t size,
			 dma_addr_t *dma_handle, gfp_t flag);

void dma_free_coherent(struct device *dev, size_t size,
			 void *vaddr, dma_addr_t dma_handle);

static inline int plat_dma_supported(struct device *dev, u64 mask)
{
	/*
	 * we fall back to GFP_DMA when the mask isn't all 1s,
	 * so we can't guarantee allocations that must be
	 * within a tighter range than GFP_DMA..
	 */
	if (mask < DMA_BIT_MASK(24))
		return 0;

	return 1;
}

static inline int dma_supported(struct device *dev, u64 mask)
{
	return plat_dma_supported(dev, mask);
}

static inline int dma_set_mask(struct device *dev, u64 mask)
{
	 if (!dev->dma_mask || !dma_supported(dev, mask))
		 return -EIO;
	 *dev->dma_mask = mask;
	 return 0;
}

static inline int dma_set_coherent_mask(struct device *dev, u64 mask)
{
	if (!dma_supported(dev, mask))
		return -EIO;
	dev->coherent_dma_mask = mask;
	return 0;
}

static inline void
dma_sync_sg_for_cpu(struct device *dev, struct scatterlist *sglist,
		    int nelems, enum dma_data_direction direction)
{
	int i;
	struct scatterlist *sg;
	for_each_sg(sglist, sg, nelems, i) {
			__dma_sync(sg_page(sg), sg->offset, sg->length,
				   direction);
	}
}

#ifndef dma_max_pfn
static inline unsigned long dma_max_pfn(struct device *dev)
{
	return *dev->dma_mask >> PAGE_SHIFT;
}
#endif

void *dmam_alloc_coherent(struct device *dev, size_t size,
			 dma_addr_t *dma_handle, gfp_t flag);

#endif
