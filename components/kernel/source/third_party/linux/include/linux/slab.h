/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Written by Mark Hemment, 1996 (markhe@nextd.demon.co.uk).
 *
 * (C) SGI 2006, Christoph Lameter
 * 	Cleaned up and restructured to ease the addition of alternative
 * 	implementations of SLAB allocators.
 * (C) Linux Foundation 2008-2013
 *      Unified interface for all slab allocators
 */

#ifndef _LINUX_SLAB_H
#define	_LINUX_SLAB_H

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <linux/gfp.h>

extern void *zalloc(size_t size);

#define krealloc(a, b, c) realloc(a, b)
#define kfree(a) free((void *)(a))

#define kmalloc(a, b) malloc(a)
#define kcalloc(a, b, c) calloc(a, b)
#define kzalloc(a, b) zalloc(a)
#define kmalloc_array(n, size, flags) calloc(n, size)

#define devm_kfree(a, b) kfree(b)
#define devm_kzalloc(a, b, c) kzalloc(b, c)
#define devm_kmalloc(a, b, c) kmalloc(b, c)
#define devm_kcalloc(a, b, c, d) kcalloc(b, c, d)

#define ARCH_DMA_MINALIGN 32
#define ARCH_KMALLOC_MINALIGN ARCH_DMA_MINALIGN

#define SLAB_DEBUG_FREE		0x00000100UL	/* DEBUG: Perform (expensive) checks on free */
#define SLAB_RED_ZONE		0x00000400UL	/* DEBUG: Red zone objs in a cache */
#define SLAB_POISON		0x00000800UL	/* DEBUG: Poison objects */
#define SLAB_HWCACHE_ALIGN	0x00002000UL	/* Align objs on cache lines */
#define SLAB_CACHE_DMA		0x00004000UL	/* Use GFP_DMA memory */
#define SLAB_STORE_USER		0x00010000UL	/* DEBUG: Store the last owner for bug hunting */
#define SLAB_PANIC		0x00040000UL	/* Panic if kmem_cache_create() fails */

struct kmem_cache {
	unsigned int size;	/* The aligned/padded/added on size  */
	unsigned int align;	/* Alignment as calculated */
	void (*ctor)(void *);	/* Called on object slot creation */
};

static inline struct kmem_cache *
kmem_cache_create(const char *name, size_t size, size_t align,
		  unsigned long flags, void (*ctor)(void *))
{
	struct kmem_cache *c;
	c = malloc(sizeof(struct kmem_cache));
	if (c) {
		c->size = size;
		c->ctor = ctor;
		c->align = align;
	}

	return c;
}

#define kmem_cache_free(c, b) free(b)

#define kmem_cache_zalloc(c, flags)                                            \
	({                                                                     \
		void *___b;                                                    \
		___b = memalign(c->align, c->size);                            \
		if (___b) {                                                    \
			memset(___b, 0, c->size);                              \
			if (c->ctor)                                           \
				c->ctor(___b);                                 \
		}                                                              \
		___b;                                                          \
	})

#endif	/* _LINUX_SLAB_H */
