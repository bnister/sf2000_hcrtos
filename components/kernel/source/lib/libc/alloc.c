#include <stdio.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/lib/console.h>
#include <kernel/lib/backtrace.h>
#include <kernel/io.h>
#include <kernel/ld.h>

extern void *__real_memset(void *addr, int c, size_t len);
extern void *__real_memmove(void *dest, const void *src, size_t len);
extern void *__real_memcpy(void *dest, const void *src, size_t len);
extern char *__real_strcat(char *s, const char *append);
extern char *__real_strcpy(char *dest, const char *src);
extern char *__real_strncat(char *dest, const char *src, size_t n);
extern char *__real_strncpy(char *dest, const char *src, size_t n);

#define NR_POOL_MMZ		(CONFIG_MEM_NR_MMZ)
#define NR_POOL			(NR_POOL_MMZ + 1)
#define ID_SYSMEM		(NR_POOL - 1)
#define NODE_MAGIC		(0xA5A5)

#define LEAK_DETECTING		(1 << 0)

struct pool_head;
struct pool_head *gMM[NR_POOL] = { NULL };
#ifdef CONFIG_MEM_DEBUG_MONITOR
static bool gLeakDetect = 0;
#endif

#define KB			(1024UL)
#define MB			(1024UL * 1024UL)
#define GB			(1024UL * 1024UL * 1024UL)
#ifndef MIN
#define MIN(a, b)		(((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)		(((a) < (b)) ? (b) : (a))
#endif

#define DEFAULT_BLOCKLOG	(12)

#define MEM_ALIGN_SIZE		sizeof(uint32_t)
#define BLOCKIFY(pool, size)	(((size) + pool->blksz - 1) / pool->blksz)
#define BASE_2			(2)

/* Address to block number and vice versa */
#define BLOCK(pool, A)		((size_t)((char *)(A) - (char *)(pool->base)) / pool->blksz + 1)
#define ADDRESS(pool, B)	((void *)((char *)(pool->base) + ((B) - 1) * pool->blksz))

/*
 * Number of contiguous free blocks allowed to build up at the end of
 * memory before they will be returned to the system
 */
#define FINAL_FREE_BLOCKS	(8)

/* Data structure giving per-block information */
typedef union {
	/* Heap information for a busy block */
	struct {
		/*
		 * Zero for a free block
		 * One for a busy block
		 */
		int status;

		/*
		 * Zero for a large (multiblock) object, or positive giving the
		 * logarithm to the base two of the fragment size
		 */
		int type;
		union {
			struct {
				size_t nfree; /* Free frags in a fragmented block */
				size_t first; /* First free fragment of the block */
			} frag;

			/*
			 * For a large object, in its first block, this has the number
			 * of blocks in the object.  In the other blocks, this has a
			 * negative number which says how far back the first block is
			 */
			size_t size;
		} info;
	} busy;

	/*
	 * Heap information for a free block
	 * (that may be the first of a free cluster)
	 */
	struct {
		int status;
		size_t size; /* Size (in blocks) of a free cluster */
		size_t next; /* Index of next free cluster */
		size_t prev; /* Index of previous free cluster */
	} free;
} heap_info;

/* Doubly linked lists of free fragments */
struct list {
	struct list *next;
	struct list *prev;
};

struct pool_head {
	void		*base;
	void		*top;
	void		*ptr;
	size_t		size;

	heap_info	*info;
	void		*info_end;
	size_t		info_size;
	size_t		info_index;
	size_t		info_limit;

	size_t		chunks_used;
	size_t		bytes_used;
	size_t		chunks_free;
	size_t		bytes_free;

	size_t		blklog;
	size_t		blksz;
	struct list	frag[0];
};

#ifdef CONFIG_MEM_DEBUG_MONITOR
struct node_info {
	unsigned short	magic;
	unsigned short	status;
	size_t		size;
#ifdef CONFIG_MEM_DEBUG_RECORD_TASKID
	unsigned int	taskid;
#endif
	unsigned int	lr[CONFIG_MEM_DEBUG_LR_CNT];
};
static int check_nodes(void *ptr, struct node_info *n, void *usrdata);
#endif

static void __free(struct pool_head *pool, void * ptr);

/*
 * Get nbytes more RAM.
 * We just increment a pointer in what's
 * left of memory in the pool
 */
static void *__sbrk(struct pool_head *pool, size_t nbytes)
{
	void *base = NULL;

	if (pool->ptr + nbytes < pool->top) {
		base = pool->ptr;
		pool->ptr += nbytes;
		return base;
	} else {
		return NULL;
	}
}


/* aligned allocation */
static void *__align(struct pool_head *pool, size_t size)
{
	void *result = NULL;
	size_t adj = 0;

	if (UINT_MAX == size) {
		return NULL;
	}

	result = __sbrk(pool, size);
	if (!result)
		return NULL;

	adj = (size_t)(result - NULL) % pool->blksz;
	if (adj) {
		adj = pool->blksz - adj;
		__sbrk(pool, adj);
		result += adj;
	}

	return result;
}

/* Get neatly aligned memory, initializing or
growing the heap info table as necessary. */
static void *morecore(struct pool_head *pool, size_t size)
{
	void *result = NULL;
	heap_info *newinfo = NULL;
	heap_info *oldinfo = NULL;
	size_t newsize = 0;
	size_t block;
	size_t blocks;

	result = __align(pool, size);
	if (NULL == result) {
		return NULL;
	}

	/* Check if we need to grow the info table.  */
	block = BLOCK(pool, result + size);
	if (block > pool->info_size) {
		newsize = pool->info_size;
		while (block > newsize) {
			newsize *= 2;
		}

		newinfo = (heap_info *)__align(pool, newsize * sizeof(heap_info));
		if (NULL == newinfo) {
			__sbrk(pool, -size);
			return NULL;
		}

		__real_memcpy(newinfo, pool->info, pool->info_size * sizeof(heap_info));
		__real_memset(&newinfo[pool->info_size], 0,
		       (newsize - pool->info_size) * sizeof(heap_info));
		oldinfo = pool->info;
		newinfo[BLOCK(pool, oldinfo)].busy.type = 0;
		newinfo[BLOCK(pool, oldinfo)].busy.info.size =
			BLOCKIFY(pool, pool->info_size * sizeof(heap_info));
		pool->info = newinfo;

		/* Account for the _info block itself in the statistics.  */
		pool->bytes_used += newsize * sizeof(heap_info);
		++pool->chunks_used;
		__free(pool, oldinfo);
		pool->info_size = newsize;
		pool->info_end = (void *)pool->info + pool->info_size * sizeof(heap_info);

		block = BLOCK(pool, (void *)newinfo);
		blocks = BLOCKIFY(pool, newsize * sizeof(heap_info));
		newinfo[block].busy.status = 1;
		newinfo[block].busy.type = 0;
		newinfo[block].busy.info.size = blocks;
		while (--blocks > 0) {
			newinfo[block + blocks].busy.status = 1;
			newinfo[block + blocks].busy.type = 0;
			newinfo[block + blocks].busy.info.size = -blocks;
		}
	}

	pool->info_limit = block;

	return result;
}

#ifdef CONFIG_MEM_DEBUG_MONITOR
static void check_free(struct pool_head *pool, void *ptr)
{
	int type = 0;
	size_t block = 0;
	size_t blocks = 0;
	heap_info *info = pool->info;

	block = BLOCK(pool, ptr);
	type = info[block].busy.type;

	if (type == 0) {
		struct node_info *n;
		blocks = info[block].busy.info.size;
		n = (struct node_info *)(ptr + blocks * pool->blksz - sizeof(struct node_info));
		assert(check_nodes(ptr, n, NULL) == 0);
		return;
	}

	if (type != 0) {
		struct node_info *n;
		n = (struct node_info *)(ptr + (1 << type) - sizeof(struct node_info));
		assert(check_nodes(ptr, n, NULL) == 0);
		return;
	}

	return;
}
#endif

void free(void * ptr)
{
	if ((NULL == ptr) || (ptr >= gMM[ID_SYSMEM]->top)) {
		return;
	}

#ifdef CONFIG_MEM_DEBUG_MONITOR
	check_free(gMM[ID_SYSMEM], ptr);
#endif

	taskENTER_CRITICAL();
	__free(gMM[ID_SYSMEM], ptr);
	taskEXIT_CRITICAL();

	return;
}

/* Allocate memory from the heap.  */
void *__malloc(struct pool_head *pool, size_t size)
{
	void *result = NULL;
	size_t block = 0;
	size_t blocks = 0;
	size_t lastblocks = 0;
	size_t start = 0;
	register size_t i = 0;
	struct list *next = NULL;
	heap_info *info = pool->info;

	/*
	 * ANSI C allows `malloc (0)' to either return NULL, or to return a
	 * valid address you can realloc and free (though not dereference).
	 * It turns out that some extant code (sunrpc, at least Ultrix's version)
	 * expects `malloc (0)' to return non-NULL and breaks otherwise. Be compatible
	 */
	if (UINT_MAX == size) {
		return result;
	}

	/* round up */
	size = MAX(size, sizeof(struct list));

	/* Determine the allocation policy based on the request size */

	/* 2 need fragment of a block */
	if (size <= (pool->blksz >> 1)) {
		/*
		 * Small allocation to receive a fragment of a block.
		 * Determine the logarithm to base two of the fragment size
		 */
		register size_t log = 1;

		--size;
		while ((size /= BASE_2) != 0) {
			++log;
		}

		/* Look in the fragment lists for a free fragment of the desired size */
		next = pool->frag[log].next;
		if (next != NULL) {
			/*
			 * There are free fragments of this size.
			 * Pop a fragment out of the fragment list and return it.
			 * Update the block's nfree and first counters
			 */
			result = (void *)next;
			next->prev->next = next->next;
			if (next->next != NULL) {
				next->next->prev = next->prev;
			}
			block = BLOCK(pool, result);
			if (--info[block].busy.info.frag.nfree != 0) {
				info[block].busy.info.frag.first =
					((size_t)((char *)next->next - (char *)NULL) % pool->blksz) >> log;
			} else {
				info[block].busy.info.frag.first = -1;
			}

			/* Update the statistics.  */
			++pool->chunks_used;
			pool->bytes_used += 1 << log;
			--pool->chunks_free;
			pool->bytes_free -= 1 << log;
		} else {
			/*
			 * No free fragments of the desired size, so get a new block
			 * and break it into fragments, returning the first
			 */

			/* 3 only once recursive self-call */
			result = (void *)__malloc(pool, pool->blksz);
			if (NULL == result) {
				return NULL;
			}

			/* Link all fragments but the first into the free list */
			for (i = 1; i < (size_t)(pool->blksz >> log); ++i) {
				next = (struct list *)((char *)result + (i << log));
				next->next = pool->frag[log].next;
				next->prev = &pool->frag[log];
				next->prev->next = next;
				if (next->next != NULL) {
					next->next->prev = next;
				}
			}

			/* Initialize the nfree and first counters for this block */
			block = BLOCK(pool, result);
			info[block].busy.type = log;
			info[block].busy.info.frag.nfree = i - 1;
			info[block].busy.info.frag.first = i - 1;

			pool->chunks_free += (pool->blksz >> log) - 1;
			pool->bytes_free += pool->blksz - (1 << log);
			pool->bytes_used -= pool->blksz - (1 << log);
		}
	} else {
		/* 2 need one or more blocks */

		/*
		 * Large allocation to receive one or more blocks.
		 * Search the free list in a circle starting at the last place visited.
		 * If we loop completely around without finding a large enough
		 * space we will have to get more memory from the system
		 */
		blocks = BLOCKIFY(pool, size);
		start = pool->info_index;
		block = pool->info_index;
		while (info[block].free.size < blocks) {
			block = info[block].free.next;
			if (block == start) {
				/*
				 * Need to get more from the system.  Check to see if
				 * the new core will be contiguous with the final free
				 * block; if so we don't need to get as much
				 */
				block = info[0].free.prev;
				lastblocks = info[block].free.size;
				if ((pool->info_limit != 0) &&
				    (block + lastblocks == pool->info_limit) &&
				    (__sbrk(pool, 0) == ADDRESS(pool, block + lastblocks)) &&
				    (morecore(pool, (blocks - lastblocks) * pool->blksz) != NULL)) {
					/*
					 * Which block we are extending (the `final free
					 * block' referred to above) might have changed, if
					 * it got combined with a freed info table
					 */
					info = pool->info;
					block = info[0].free.prev;
					info[block].free.size += (blocks - lastblocks);
					pool->bytes_free += (blocks - lastblocks) * pool->blksz;
					continue;
				}

				result = morecore(pool, blocks * pool->blksz);
				if (NULL == result) {
					return NULL;
				}

				info = pool->info;
				block = BLOCK(pool, result);
				info[block].busy.status = 1;
				info[block].busy.type = 0;
				info[block].busy.info.size = blocks;
				++pool->chunks_used;
				pool->bytes_used += blocks * pool->blksz;

				while (--blocks > 0) {
					info[block + blocks].busy.status = 1;
					info[block + blocks].busy.type = 0;
					info[block + blocks].busy.info.size = -blocks;
				}

				return result;
			}
		}

		/*
		 * At this point we have found a suitable free list entry.
		 * Figure out how to remove what we need from the list
		 */
		result = ADDRESS(pool, block);
		if (info[block].free.size > blocks) {
			/*
			 * The block we found has a bit left over,
			 * so relink the tail end back into the free list
			 */
			info[block + blocks].free.size = info[block].free.size - blocks;
			info[block + blocks].free.next = info[block].free.next;
			info[block + blocks].free.prev = info[block].free.prev;

			pool->info_index = block + blocks;
			info[info[block].free.next].free.prev = pool->info_index;
			info[info[block].free.prev].free.next = info[info[block].free.next].free.prev;
		} else {
			/*
			 * The block exactly matches our requirements,
			 * so just remove it from the list
			 */
			info[info[block].free.next].free.prev = info[block].free.prev;
			pool->info_index = info[block].free.next;
			info[info[block].free.prev].free.next = pool->info_index;
			--pool->chunks_free;
		}

		info[block].busy.status = 1;
		info[block].busy.type = 0;
		info[block].busy.info.size = blocks;
		++pool->chunks_used;
		pool->bytes_used += blocks * pool->blksz;
		pool->bytes_free -= blocks * pool->blksz;

		/*
		 * Mark all the blocks of the object just allocated except for the
		 * first with a negative number so you can find the first block by
		 * adding that adjustment
		 */
		while (--blocks > 0) {
			info[block + blocks].busy.status = 1;
			info[block + blocks].busy.type = 0;
			info[block + blocks].busy.info.size = -blocks;
		}
	}

	return result;
}

#ifdef CONFIG_MEM_DEBUG_MONITOR
static void fill_magic(void *start, void *end)
{
	while (start < end) {
		*(char *)start = (char)0xa5;
		start++;
	}
}

static int check_magic(void *start, void *end)
{
	while (start < end) {
		if (*(char *)start != (char)0xa5) {
			printf("(check magic %p: 0xa5 -> 0x%02x)\n", start, *(unsigned char *)start);
			return -1;
		}
		start++;
	}

	return 0;
}
#endif

void *malloc(size_t size)
{
	void *ptr;
	struct pool_head *pool = gMM[ID_SYSMEM];

#ifdef CONFIG_MEM_DEBUG_MONITOR
	size += sizeof(struct node_info);
#endif
	taskENTER_CRITICAL();
	ptr = __malloc(pool, size);
	taskEXIT_CRITICAL();

	if (!ptr)
		return NULL;

#ifdef CONFIG_MEM_DEBUG_MONITOR
	struct node_info *n;
	TaskStatus_t tstatus;

	if (size <= (pool->blksz >> 1)) {
		size_t s = size;
		register size_t log = 1;

		--s;
		while ((s /= BASE_2) != 0) {
			++log;
		}
		n = (struct node_info *)(ptr + (1 << log) - sizeof(struct node_info));
	} else {
		n = (struct node_info *)(ptr + ALIGN(size, pool->blksz) - sizeof(struct node_info));
	}

	__real_memset(n, 0, sizeof(struct node_info));

	if (xTaskGetCurrentTaskHandle())
		vTaskGetInfo(NULL, &tstatus, pdFALSE, eRunning);
	else
		tstatus.xTaskNumber = 0;
	n->size = size - sizeof(struct node_info);
	fill_magic(ptr + n->size, (void *)n);
	n->status = 0;
	n->magic = NODE_MAGIC;
#ifdef CONFIG_MEM_DEBUG_RECORD_TASKID
	n->taskid = tstatus.xTaskNumber;
#endif
	stacktrace_get_ra(n->lr, CONFIG_MEM_DEBUG_LR_CNT, CONFIG_MEM_DEBUG_LR_OMIT_CNT);
	if (gLeakDetect) {
		n->status |= LEAK_DETECTING;
	}
#endif
	return ptr;
}

static void __free(struct pool_head *pool, void *ptr)
{
	int type = 0;
	size_t block = 0;
	size_t blocks = 0;
	register size_t i = 0;
	struct list *prev = NULL;
	struct list *next = NULL;
	heap_info *info = pool->info;

	block = BLOCK(pool, ptr);
	type = info[block].busy.type;

	if (ptr < pool->base || ptr > pool->ptr) {
		printf("Error trye to free memory out of pool!\n");
		assert(0);
		return;
	}

	if (ptr >= (void *)pool->info && ptr < pool->info_end) {
		printf("Error trye to free memory inside pool info!\n");
		assert(0);
		return;
	}

	if (info[block].busy.status != 1) {
		printf("Error try to free freed memory!\n");
		assert(0);
		return;
	}

	if (type == 0) {
		if (ptr != ADDRESS(pool, block)) {
			printf("Error free block memory, the pointer not aligned!\n");
			assert(0);
			return;
		}

		if ((int)info[block].busy.info.size < 0) {
			printf("Error free block memory, the pointer is not the start of a cluster of blocks!\n");
			assert(0);
			return;
		}
	}

	if (type != 0) {
		if (!IS_ALIGNED((size_t)ptr, (1 << type))) {
			printf("Error free fragment memory, the pointer not aligned!\n");
			assert(0);
			return;
		}

		next = (struct list *)(ADDRESS(pool, block) + (info[block].busy.info.frag.first << type));
		for (i = 0; i < info[block].busy.info.frag.nfree; i++) {
			if (ptr == (void *)next) {
				printf("Error free fragment memory, try to free freed memory!\n");
				assert(0);
				return;
			}
			if (next == NULL || (unsigned int)next < 0x80000000) {
				printf("The freed fragment is modified!\n");
				printf("Block idx: %ld\n", block);
				printf("Block address: 0x%08x\n", (unsigned int)ADDRESS(pool, block));
				printf("First free fragment address: 0x%08x\n",
				       (unsigned int)(ADDRESS(pool, block) +
						      (info[block].busy.info.frag .first << type)));
				printf("Number of free fragments: %ld\n", info[block].busy.info.frag.nfree);
				assert(0);
			}
			next = next->next;
		}
	}

	switch (type) {
	case 0:
		/* Get as many statistics as early as we can */
		--pool->chunks_used;
		pool->bytes_used -= info[block].busy.info.size * pool->blksz;
		pool->bytes_free += info[block].busy.info.size * pool->blksz;

		/*
		 * Mark all the blocks of the object as status free
		 */
		info[block].busy.status = 0;
		blocks = info[block].busy.info.size;
		while (--blocks > 0) {
			info[block + blocks].busy.status = 0;
		}

		/*
		 * Find the free cluster previous to this one in the free list.
		 * Start searching at the last block referenced; this may benefit
		 * programs with locality of allocation
		 */
		i = pool->info_index;
		if (i > block) {
			while (i > block) {
				i = info[i].free.prev;
			}
		} else {
			do {
				i = info[i].free.next;
			} while ((i > 0) && (i < block));

			i = info[i].free.prev;
		}

		/* Determine how to link this block into the free list */
		if (block == i + info[i].free.size) {
			/* Coalesce this block with its predecessor */
			info[i].free.size += info[block].busy.info.size;
			info[block].free.size = 0;
			block = i;
		} else {
			/* Really link this block back into the free list */
			info[block].free.size = info[block].busy.info.size;
			info[block].free.next = info[i].free.next;
			info[block].free.prev = i;
			info[i].free.next = block;
			info[info[block].free.next].free.prev = block;
			++pool->chunks_free;
		}

		/*
		 * Now that the block is linked in, see if we can coalesce it
		 * with its successor (by deleting its successor from the list
		 * and adding in its size)
		 */
		if (block + info[block].free.size == info[block].free.next) {
			info[block].free.size += info[info[block].free.next].free.size;
			info[block].free.next = info[info[block].free.next].free.next;
			info[info[block].free.next].free.prev = block;
			--pool->chunks_free;
		}

		/* Now see if we can return stuff to the system */
		blocks = info[block].free.size;
		if ((blocks >= FINAL_FREE_BLOCKS) &&
		    (block + blocks == pool->info_limit) &&
		    (__sbrk(pool, 0) == ADDRESS(pool, block + blocks))) {
			register size_t bytes = blocks * pool->blksz;

			pool->info_limit -= blocks;
			__sbrk(pool, -bytes);
			info[info[block].free.prev].free.next = info[block].free.next;
			info[info[block].free.next].free.prev = info[block].free.prev;
			block = info[block].free.prev;
			--pool->chunks_free;
			pool->bytes_free -= bytes;
		}

		/* Set the next search to begin at this block */
		pool->info_index = block;

		break;

	default:
		/* Do some of the statistics */
		--pool->chunks_used;
		pool->bytes_used -= 1 << type;
		++pool->chunks_free;
		pool->bytes_free += 1 << type;

		/* Get the address of the first free fragment in this block */
		prev = (struct list *)(ADDRESS(pool, block) + (info[block].busy.info.frag.first << type));

		if (info[block].busy.info.frag.nfree == (size_t)((pool->blksz >> type) - 1)) {
			/*
			 * If all fragments of this block are free, remove them
			 * from the fragment list and free the whole block
			 */
			next = prev;
			for (i = 1; i < (size_t)(pool->blksz >> type); ++i) {
				next = next->next;
			}

			prev->prev->next = next;
			if (next != NULL) {
				next->prev = prev->prev;
			}
			info[block].busy.type = 0;
			info[block].busy.info.size = 1;

			/* Keep the statistics accurate */
			++pool->chunks_used;
			pool->bytes_used += pool->blksz;
			pool->chunks_free -= pool->blksz >> type;
			pool->bytes_free -= pool->blksz;

			__free(pool, ADDRESS(pool, block));

		} else if (info[block].busy.info.frag.nfree != 0) {
			/*
			 * If some fragments of this block are free, link this
			 * fragment into the fragment list after the first free
			 * fragment of this block
			 */
			next = (struct list *)ptr;
			next->next = prev->next;
			next->prev = prev;
			prev->next = next;
			if (next->next != NULL) {
				next->next->prev = next;
			}

			++info[block].busy.info.frag.nfree;

		} else {
			/*
			 * No fragments of this block are free, so link this
			 * fragment into the fragment list and announce that
			 * it is the first free fragment of this block
			 */
			prev = (struct list *)ptr;
			info[block].busy.info.frag.nfree = 1;
			info[block].busy.info.frag.first = ((size_t)((char *)ptr - (char *)NULL) % pool->blksz) >> type;

			prev->next = pool->frag[type].next;
			prev->prev = &pool->frag[type];
			prev->prev->next = prev;
			if (prev->next != NULL) {
				prev->next->prev = prev;
			}
		}
		break;
	}
}

/*
 * Resize the given region to the new size, returning a pointer
 * to the (possibly moved) region.  This is optimized for speed;
 * some benchmarks seem to indicate that greater compactness is
 * achieved by unconditionally allocating and copying to a
 * new region.  This module has incestuous knowledge of the
 * internals of both free and malloc
 */
static void * __realloc(struct pool_head *pool, void * ptr, size_t size)
{
	void * result = NULL;
	int type = 0;
	size_t block = 0;
	size_t blocks = 0;
	size_t oldlimit = 0;
	void * previous = NULL;
	heap_info *info = pool->info;
	
	block = BLOCK(pool, ptr);
	type = pool->info[block].busy.type;

	switch (type) {
	case 0:
		/* Maybe reallocate a large block to a small fragment */
		if (size <= (pool->blksz >> 1)) {
			result = (void *)__malloc(pool, size);
			if (result != NULL) {
				__real_memcpy(result, ptr, size);
				__free(pool, ptr);
				return result;
			}
		}

		/*
		 * The new size is a large allocation as well;
		 * see if we can hold it in place
		 */
		blocks = BLOCKIFY(pool, size);
		if (blocks < info[block].busy.info.size) {
			/*
			 * The new size is smaller; return
			 *  excess memory to the free list
			 */
			info[block + blocks].busy.type = 0;
			info[block + blocks].busy.info.size = info[block].busy.info.size - blocks;
			info[block].busy.info.size = blocks;
			/*
			 * We have just created a new chunk by splitting a chunk in two.
			 *  Now we will free this chunk; increment the statistics counter
			 *  so it doesn't become wrong when __free decrements it
			 */
			++pool->chunks_used;
			__free(pool, ADDRESS(pool, block + blocks));
			result = ptr;
		} else if (blocks == info[block].busy.info.size) {
			/* No size change necessary */
			result = ptr;
		} else {
			/*
			 * Won't fit, so allocate a new region that will.
			 * Free the old region first in case there is sufficient
			 * adjacent free space to grow without moving
			 */
			blocks = info[block].busy.info.size;

			/* Prevent free from actually returning memory to the system */
			oldlimit = pool->info_limit;
			pool->info_limit = 0;
			__free(pool, ptr);
			pool->info_limit = oldlimit;
			result = (void *)__malloc(pool, size);
			if (NULL == result) {
				/*
				 * Now we're really in trouble.  We have to unfree
				 * the thing we just freed.  Unfortunately it might
				 * have been coalesced with its neighbors
				 */
				if (pool->info_index == block) {
					__malloc(pool, blocks * pool->blksz);
				} else {
					previous = __malloc(pool, (block - pool->info_index) * pool->blksz);
					__malloc(pool, blocks * pool->blksz);
					__free(pool, previous);
				}

				return NULL;
			}

			if (ptr != result) {
				__real_memmove(result, ptr, blocks * pool->blksz);
			}
		}
		break;

	default:
		/*
		 * Old size is a fragment; type is logarithm
		 * to base two of the fragment size
		 */
		if ((size > (size_t)(1 << (type - 1))) &&
		    (size <= (size_t)(1 << type))) {
			/* The new size is the same kind of fragment */
			result = ptr;
		} else {
			/*
			 * The new size is different; allocate a new space,
			 * and copy the lesser of the new size and the old
			 */
			result = (void *)__malloc(pool, size);
			if (NULL == result) {
				return NULL;
			}

			__real_memcpy(result, ptr, MIN(size, (size_t)1 << type));
			__free(pool, ptr);
			return result;
		}
		break;
	}

	return result;
}

void *realloc(void *ptr, size_t size)
{
	struct pool_head *pool = gMM[ID_SYSMEM];

	if (0 == size) {
		free(ptr);
		return malloc(0);
	} else if (NULL == ptr) {
		return malloc(size);
	}

#ifdef CONFIG_MEM_DEBUG_MONITOR
	size += sizeof(struct node_info);
#endif

	taskENTER_CRITICAL();
	ptr = __realloc(pool, ptr, size);
	taskEXIT_CRITICAL();

	if (!ptr)
		return NULL;

#ifdef CONFIG_MEM_DEBUG_MONITOR
	struct node_info *n;
	TaskStatus_t tstatus;
	if (size <= (pool->blksz >> 1)) {
		size_t s = size;
		register size_t log = 1;

		--s;
		while ((s /= BASE_2) != 0) {
			++log;
		}
		n = (struct node_info *)(ptr + (1 << log) - sizeof(struct node_info));
	} else {
		n = (struct node_info *)(ptr + ALIGN(size, pool->blksz) - sizeof(struct node_info));
	}

	__real_memset(n, 0, sizeof(struct node_info));

	if (xTaskGetCurrentTaskHandle())
		vTaskGetInfo(NULL, &tstatus, pdFALSE, eRunning);
	else
		tstatus.xTaskNumber = 0;
	n->size = size - sizeof(struct node_info);
	fill_magic(ptr + n->size, (void *)n);
	n->status = 0;
	n->magic = NODE_MAGIC;
#ifdef CONFIG_MEM_DEBUG_RECORD_TASKID
	n->taskid = tstatus.xTaskNumber;
#endif
	stacktrace_get_ra(n->lr, CONFIG_MEM_DEBUG_LR_CNT, CONFIG_MEM_DEBUG_LR_OMIT_CNT);
	if (gLeakDetect) {
		n->status |= LEAK_DETECTING;
	}
#endif

	return ptr;
}

void * calloc(size_t nelem, size_t elsize)
{
	unsigned char *ptr = NULL;

	ptr = malloc(nelem * elsize);
	if (ptr) {
		__real_memset(ptr, 0, nelem * elsize);
	}

	return ptr;
}

void *zalloc(size_t size)
{
	void *t = malloc(size);

	if (t != NULL)
		__real_memset(t, 0, size);

	return t;
}

void *memalign(size_t alignment, size_t size)
{
	void *p;
	size_t mask = alignment - 1;

	if (alignment <= 8)
		return malloc(size);
	
	if ((alignment & mask) || alignment > 4096) {
		printf("Not support non-2-based alignment!\n");
		assert(0);
		return NULL;
	}

	size = MAX(8, size);
	p = malloc(ALIGN(size, alignment));

	assert((((uint32_t)p) & mask) == 0);

	return p;
}

size_t mmz_total(int id)
{
	if (id < 0 || id >= NR_POOL_MMZ || gMM[id] == NULL)
		return 0;

	return gMM[id]->size;
}

void *mmz_malloc(int id, size_t size)
{
	struct pool_head *pool;
	void *ptr;

	if (id < 0 || id >= NR_POOL_MMZ || gMM[id] == NULL)
		return NULL;

	pool = gMM[id];

#ifdef CONFIG_MEM_DEBUG_MONITOR
	size += sizeof(struct node_info);
#endif

	taskENTER_CRITICAL();
	ptr = __malloc(gMM[id], size);
	taskEXIT_CRITICAL();

	if (!ptr)
		return NULL;

#ifdef CONFIG_MEM_DEBUG_MONITOR
	struct node_info *n;
	TaskStatus_t tstatus;
	if (size <= (pool->blksz >> 1)) {
		size_t s = size;
		register size_t log = 1;

		--s;
		while ((s /= BASE_2) != 0) {
			++log;
		}
		n = (struct node_info *)(ptr + (1 << log) - sizeof(struct node_info));
	} else {
		n = (struct node_info *)(ptr + ALIGN(size, pool->blksz) - sizeof(struct node_info));
	}

	__real_memset(n, 0, sizeof(struct node_info));

	if (xTaskGetCurrentTaskHandle())
		vTaskGetInfo(NULL, &tstatus, pdFALSE, eRunning);
	else
		tstatus.xTaskNumber = 0;
	n->size = size - sizeof(struct node_info);
	fill_magic(ptr + n->size, (void *)n);
	n->status = 0;
	n->magic = NODE_MAGIC;
#ifdef CONFIG_MEM_DEBUG_RECORD_TASKID
	n->taskid = tstatus.xTaskNumber;
#endif
	stacktrace_get_ra(n->lr, CONFIG_MEM_DEBUG_LR_CNT, CONFIG_MEM_DEBUG_LR_OMIT_CNT);
	if (gLeakDetect) {
		n->status |= LEAK_DETECTING;
	}
#endif

	return ptr;
}

void *mmz_zalloc(int id, size_t size)
{
	void *t;

	if (id < 0 || id >= NR_POOL_MMZ || gMM[id] == NULL)
		return NULL;

	t = mmz_malloc(id, size);

	if (t != NULL)
		__real_memset(t, 0, size);

	return t;
}

void mmz_free(int id, void *ptr)
{
	if (id < 0 || id >= NR_POOL_MMZ || gMM[id] == NULL)
		return;

	if ((NULL == ptr) || (ptr >= gMM[id]->top)) {
		return;
	}

#ifdef CONFIG_MEM_DEBUG_MONITOR
	check_free(gMM[id], ptr);
#endif

	taskENTER_CRITICAL();
	__free(gMM[id], ptr);
	taskEXIT_CRITICAL();

	return;
}

void *mmz_calloc(int id, size_t nmemb, size_t size)
{
	void *ptr = NULL;

	if (id < 0 || id >= NR_POOL_MMZ || gMM[id] == NULL)
		return NULL;

	ptr = mmz_malloc(id, nmemb * size);
	if (ptr) {
		__real_memset(ptr, 0, nmemb * size);
	}

	return ptr;
}

void *mmz_realloc(int id, void *ptr, size_t size)
{
	if (id < 0 || id >= NR_POOL_MMZ || gMM[id] == NULL)
		return NULL;

	if (0 == size) {
		mmz_free(id, ptr);
		return mmz_malloc(id, 0);
	} else if (NULL == ptr) {
		return mmz_malloc(id, size);
	}

#ifdef CONFIG_MEM_DEBUG_MONITOR
	size += sizeof(struct node_info);
#endif

	taskENTER_CRITICAL();
	ptr = __realloc(gMM[id], ptr, size);
	taskEXIT_CRITICAL();

	if (!ptr)
		return NULL;

#ifdef CONFIG_MEM_DEBUG_MONITOR
	struct pool_head *pool = gMM[id];
	struct node_info *n;
	TaskStatus_t tstatus;
	if (size <= (pool->blksz >> 1)) {
		size_t s = size;
		register size_t log = 1;

		--s;
		while ((s /= BASE_2) != 0) {
			++log;
		}
		n = (struct node_info *)(ptr + (1 << log) - sizeof(struct node_info));
	} else {
		n = (struct node_info *)(ptr + ALIGN(size, pool->blksz) - sizeof(struct node_info));
	}

	__real_memset(n, 0, sizeof(struct node_info));

	if (xTaskGetCurrentTaskHandle())
		vTaskGetInfo(NULL, &tstatus, pdFALSE, eRunning);
	else
		tstatus.xTaskNumber = 0;
	n->size = size - sizeof(struct node_info);
	fill_magic(ptr + n->size, (void *)n);
	n->status = 0;
	n->magic = NODE_MAGIC;
#ifdef CONFIG_MEM_DEBUG_RECORD_TASKID
	n->taskid = tstatus.xTaskNumber;
#endif
	stacktrace_get_ra(n->lr, CONFIG_MEM_DEBUG_LR_CNT, CONFIG_MEM_DEBUG_LR_OMIT_CNT);
	if (gLeakDetect) {
		n->status |= LEAK_DETECTING;
	}
#endif

	return ptr;
}

void *mmz_memalign(int id, size_t alignment, size_t size)
{
	void *p;
	size_t mask = alignment - 1;

	if (id < 0 || id >= NR_POOL_MMZ || gMM[id] == NULL)
		return NULL;

	if (alignment <= 8)
		return mmz_malloc(id, size);
	
	assert(alignment <= 4096 && !(alignment & mask));

	size = MAX(8, size);
	p = mmz_malloc(id, ALIGN(size, alignment));

	assert((((uint32_t)p) & mask) == 0);

	return p;
}

#ifdef CONFIG_MEM_DEBUG_STORE
static void check_addr_region_by_pool(struct pool_head *pool, void *addr,
				      size_t len)
{
	void *start;
	void *end;
	void *addr_end;
	int type = 0;
	size_t block = 0;
	register size_t i = 0;
	struct list *next = NULL;
	heap_info *info = pool->info;

	if (addr < pool->base || addr >= pool->top)
		return;

	addr_end = addr + len - 1;
	if ((addr >= (void *)pool->info && addr < pool->info_end) ||
	    (addr_end >= (void *)pool->info && addr_end < pool->info_end)) {
		printf("Error try to store pool info region!\n");
		assert(0);
		return;
	}

	block = BLOCK(pool, addr);
	if (info[block].busy.status != 1) {
		printf("Error try to store freed memory(%p)!\n", addr);
		assert(0);
		return;
	}

	type = info[block].busy.type;
	if (type == 0) {
		if (info[block].busy.info.size == 0) {
			printf("Error try to store a block with size 0!\n");
			assert(0);
			return;
		}

		if ((int)info[block].busy.info.size < 0) {
			block += (int)info[block].busy.info.size;
		}

		end = ADDRESS(pool, block + info[block].busy.info.size);
#ifdef CONFIG_MEM_DEBUG_MONITOR
		struct node_info *n;
		n = (struct node_info *)(end - sizeof(struct node_info));
		if (n->magic == NODE_MAGIC)
			end = ADDRESS(pool, block) + n->size;
#endif
		if (addr_end >= end) {
			printf("Error try to store a block but overflow!\n");
			assert(0);
			return;
		}
	} else {
		start = PTR_ALIGN_DOWN(addr, (1 << type));
		end = start + (1 << type);
#ifdef CONFIG_MEM_DEBUG_MONITOR
		struct node_info *n;
		n = (struct node_info *)(end - sizeof(struct node_info));
		if (n->magic == NODE_MAGIC)
			end = start + n->size;
#endif
		if (addr_end >= end) {
			printf("Error try to store a fragment but overflow\n");
			assert(0);
			return;
		}

		next = (struct list *)(ADDRESS(pool, block) + (info[block].busy.info.frag.first << type));
		for (i = 0; i < info[block].busy.info.frag.nfree; i++) {
			if (start == (void *)next) {
				printf("Error try to store freed fragment (0x%08x)!\n", (unsigned int)start);
				assert(0);
				return;
			}
			if (next == NULL || (unsigned int)next < 0x80000000) {
				printf("The freed fragment is modified!\n");
				printf("Block idx: %ld\n", block);
				printf("Block address: 0x%08x\n", (unsigned int)ADDRESS(pool, block));
				printf("First free fragment address: 0x%08x\n",
				       (unsigned int)(ADDRESS(pool, block) +
						      (info[block].busy.info.frag .first << type)));
				printf("Number of free fragments: %ld\n", info[block].busy.info.frag.nfree);
				assert(0);
			}

			next = next->next;
		}
	}
}

static void check_addr_region(void *addr, size_t len)
{
	int i;

	taskENTER_CRITICAL();

	for (i = 0; i < NR_POOL; i++) {
		if (!gMM[i])
			continue;

		check_addr_region_by_pool(gMM[i], addr, len);
	}

	taskEXIT_CRITICAL();
}
#else
static void check_addr_region(void *addr, size_t len)
{
}
#endif
void *__wrap_memset(void *addr, int c, size_t len)
{
	check_addr_region(addr, len);
	return __real_memset(addr, c, len);
}

void *__wrap_memmove(void *dest, const void *src, size_t len)
{
	check_addr_region(dest, len);
	return __real_memmove(dest, src, len);
}

void *__wrap_memcpy(void *dest, const void *src, size_t len)
{
	check_addr_region(dest, len);
	return __real_memcpy(dest, src, len);
}

char *__wrap_strcat(char *s, const char *append)
{
#ifdef CONFIG_MEM_DEBUG_STORE
	if ((s == NULL) || (append == NULL)) {
		return NULL;
	}

	char *end = s;
	size_t len = strlen(append);
	for (; *end != '\0'; ++end) {
	}

	check_addr_region((void *)end, len + 1);
#endif
	return __real_strcat(s, append);
}

char *__wrap_strcpy(char *dest, const char *src)
{
#ifdef CONFIG_MEM_DEBUG_STORE
	if ((dest == NULL) || (src == NULL)) {
		return NULL;
	}

	size_t len = strlen(src);

	check_addr_region((void *)dest, len + 1);
#endif
	return __real_strcpy(dest, src);
}

char *__wrap_strncat(char *dest, const char *src, size_t n)
{
#ifdef CONFIG_MEM_DEBUG_STORE
	if ((dest == NULL) || (src == NULL)) {
		return NULL;
	}

	char *end = dest;
	size_t len = strlen(src);
	size_t size = len > n ? n : len;
	for (; *end != '\0'; ++end) {
	}

	check_addr_region((void *)end, size + 1);
#endif
	return __real_strncat(dest, src, n);
}

char *__wrap_strncpy(char *dest, const char *src, size_t n)
{
#ifdef CONFIG_MEM_DEBUG_STORE
	if ((dest == NULL) || (src == NULL)) {
		return NULL;
	}

	size_t len = strlen(src);
	size_t size = len > n ? n : len;

	check_addr_region((void *)dest, size);
#endif
	return __real_strncpy(dest, src, n);
}

static struct pool_head *pool_init(void *start, size_t size, size_t blklog)
{
	struct pool_head *pool;
	size_t pool_head_sz;
	size_t blksz = (1 << blklog);
	size_t hsize = size >> blklog;
	size_t block = 0;
	size_t blocks = 0;

	pool_head_sz = sizeof(struct pool_head) + blklog * sizeof(struct list);
	size -= (unsigned long)start % MEM_ALIGN_SIZE;
	start = PTR_ALIGN(start, MEM_ALIGN_SIZE);
	size = ALIGN_DOWN(size, blksz);

	pool = (struct pool_head *)start;
	__real_memset(pool, 0, pool_head_sz);
	pool->blklog = blklog;
	pool->blksz = blksz;
	pool->top = start + size;
	pool->ptr = (void *)pool + pool_head_sz;
	pool->info_size = hsize;
	hsize *= sizeof(heap_info);
	pool->info = (heap_info *)__align(pool, hsize);
	pool->info_end = (void *)pool->info + pool->info_size * sizeof(heap_info);
	pool->base = pool->info;
	pool->size = pool->top - pool->base;
	pool->bytes_used = hsize;
	pool->chunks_used = 1;
	__real_memset(pool->info, 0, hsize);

	block = BLOCK(pool, (void *)pool->info);
	blocks = BLOCKIFY(pool, pool->info_size * sizeof(heap_info));
	pool->info[block].busy.status = 1;
	pool->info[block].busy.type = 0;
	pool->info[block].busy.info.size = blocks;
	while (--blocks > 0) {
		pool->info[block + blocks].busy.status = 1;
		pool->info[block + blocks].busy.type = 0;
		pool->info[block + blocks].busy.info.size = -blocks;
	}

	return pool;
}

const char * __attribute__((weak)) fdt_get_sysmem_path(void)
{
	return "/hcrtos/memory-mapping/sysmem";
}

static uint32_t get_sysmem_end(void)
{
	u32 start = 0, size = 0;
	int np = fdt_get_node_offset_by_path(fdt_get_sysmem_path());

	fdt_get_property_u_32_index(np, "reg", 0, &start);
	fdt_get_property_u_32_index(np, "reg", 1, &size);

	return MIPS_CACHED_ADDR(start + size);
}

static size_t get_sysmem_blklog(void)
{
	u32 blklog = DEFAULT_BLOCKLOG;
	int np = fdt_get_node_offset_by_path(fdt_get_sysmem_path());

	fdt_get_property_u_32_index(np, "blocklog", 0, &blklog);

	return (size_t)blklog;
}

int OsKHeapInit(void)
{
	void *start = (void *)&_ebss;
	size_t size = (void *)get_sysmem_end() - start;
	size_t blklog = get_sysmem_blklog();

	gMM[ID_SYSMEM] = pool_init(start, size, blklog);

	return 0;
}

int OsKMmzInit(void)
{
	char name[128];
	int i, np;
	u32 start = 0, size = 0, id = 0, blklog;

	for (i = 0; i < 10; i++) {
		blklog = DEFAULT_BLOCKLOG;
		start = 0;
		size = 0;
		id = 0xffffffff;
		__real_memset(name, 0, sizeof(name));
		sprintf(name, "/hcrtos/memory-mapping/mmz%d", i);
		np = fdt_get_node_offset_by_path(name);
		if (np < 0)
			continue;
		fdt_get_property_u_32_index(np, "reg", 0, &start);
		fdt_get_property_u_32_index(np, "reg", 1, &size);
		fdt_get_property_u_32_index(np, "id", 0, &id);
		fdt_get_property_u_32_index(np, "blocklog", 0, &blklog);
		if (size > 0 && id < (u32)NR_POOL_MMZ) {
			gMM[i] = pool_init(MIPS_CACHED_ADDR((void *)start), size, blklog);
		}
	}

	return 0;
}

int mmz_create(void *start, size_t size)
{
	int i;

	for (i = 0; i < 10; i++) {
		if (gMM[i] != NULL)
			continue;

		gMM[i] = pool_init(MIPS_CACHED_ADDR((void *)start), size,
				   DEFAULT_BLOCKLOG);

		return i;
	}

	return -1;
}

int mmz_delete(int id)
{
	if (id < 0 || id >= NR_POOL_MMZ || gMM[id] == NULL)
		return -1;

	gMM[id] = NULL;

	return 0;
}

static void print_sizeinfo(const char *tag, size_t start, size_t end)
{
	size_t size = end - start;

	printf("%s%10ld (%8.02f KB, %6.02f MB)\n", tag, size,
	       (float)size / KB, (float)size / MB);
}

static void show_pool(struct pool_head *pool)
{
	printf        (" Range           %p  -  %p\n", pool->base, pool->top);
	print_sizeinfo(" Total available ", 0, pool->size);
	print_sizeinfo(" WaterLine       ", (size_t)pool->base, (size_t)pool->ptr);
	print_sizeinfo(" Unexploited     ", (size_t)pool->ptr, (size_t)pool->top);
	print_sizeinfo(" Used            ", 0, (size_t)pool->bytes_used);
	print_sizeinfo(" Free            ", 0, pool->bytes_free + (size_t)(pool->top - pool->ptr));
}

static int mem_free(int argc, char *argv[])
{
	int i;
	size_t size;

	size = (size_t)&_ebss - (size_t)&__RAM_BASE;
	printf        ("Code size info:\n");
	printf        (" Range           %p  -  %p\n", (void *)&__RAM_BASE, (void *)&_ebss);
	print_sizeinfo(" Code size       ", 0, size);
	print_sizeinfo("   text size     ", (size_t)&__text_start, (size_t)&__text_end);
	print_sizeinfo("   rodata size   ", (size_t)&__rodata_start, (size_t)&__rodata_end);
	print_sizeinfo("   data size     ", (size_t)&__data_start, (size_t)&__data_end);
	print_sizeinfo("   sdata size    ", (size_t)&__sdata_start, (size_t)&__sdata_end);
	print_sizeinfo("   init size     ", (size_t)&_init_start, (size_t)&_init_end);
	print_sizeinfo("   sbss size     ", (size_t)&__sbss_start, (size_t)&__sbss_end);
	print_sizeinfo("   bss size      ", (size_t)&__bss_start, (size_t)&__bss_end);

	printf("System Heap info:\n");
	show_pool(gMM[ID_SYSMEM]);

	for (i = 0; i < NR_POOL_MMZ; i++) {
		if (gMM[i] == NULL)
			continue;

		printf("MMZ(%d) Heap info:\n", i);
		show_pool(gMM[i]);
	}

	return 0;
}

#ifdef CONFIG_MEM_DEBUG_MONITOR
typedef int (*node_func_t)(void *ptr, struct node_info *n, void *usrdata);

static void scan_pool(struct pool_head *pool, node_func_t func, void *usrdata)
{
	size_t block;
	size_t blocks;
	void *ptr;
	heap_info *info = pool->info;
	struct node_info *n;

	if (func == NULL)
		return;

	for (block = 1; block < pool->info_limit;) {
		if (info[block].busy.status == 0) {
			block += info[block].free.size;
			continue;
		}

		if (block == BLOCK(pool, pool->info)) {
			block += info[block].busy.info.size;
			continue;
		}

		if (info[block].busy.type == 0) {
			blocks = info[block].busy.info.size;
			ptr = ADDRESS(pool, block);
			n = (struct node_info *)(ptr + blocks * pool->blksz -
						 sizeof(struct node_info));
			func(ptr, n, usrdata);

			block += blocks;
		} else {
			uint32_t bitmap[16] = { 0 };
			int type = info[block].busy.type;
			size_t nfragments = (size_t)pool->blksz >> type;
			struct list *next = NULL;
			size_t i, j;
			void *frag;

			ptr = ADDRESS(pool, block);
			next = (struct list *)(ptr + (info[block].busy.info.frag.first << type));
			for (i = 0; i < info[block].busy.info.frag.nfree; i++) {
				j = (size_t)((void *)next - ptr) >> type;
				bitmap[j / 32] |= (1 << (j % 32));
				next = next->next;
			}

			for (i = 0; i < nfragments; i++) {
				if (bitmap[i / 32] & (1 << (i %32)))
					continue;

				frag = ptr + (i << type);
				n = (struct node_info *)(frag + (1 << type) - sizeof(struct node_info));
				func(frag, n, usrdata);
			}

			block++;
		}
	}
}

static void print_ra(unsigned int *lr, int cnt)
{
	int i;

	for (i = 0; i < cnt; i++)
		printf("0x%08x,", lr[i]);
	
	printf("\n");
}

static int check_nodes(void *ptr, struct node_info *n, void *usrdata)
{
	if (n->magic != NODE_MAGIC || check_magic(ptr + n->size, (void *)n)) {
		printf("%p\tMemory integrity check fail\n", ptr);
		printf("%p\tMemory node_info\n", n);
		if (n->magic == NODE_MAGIC) {
			printf("%p\tBut node magic is correct, node %p\n", ptr, (void *)n);
#ifdef CONFIG_MEM_DEBUG_RECORD_TASKID
			printf("%p\t%d\tra: ", (void *)ptr, n->taskid);
#else
			printf("%p\tra: ", (void *)ptr);
#endif
			print_ra(n->lr, CONFIG_MEM_DEBUG_LR_CNT);
		}else{
			printf("0x%x\tMemory magic number\n", n->magic);
		}

		return -1;
	}

	return 0;
}

static int print_nodes(void *ptr, struct node_info *n, void *usrdata)
{
	if (n->magic != NODE_MAGIC || check_magic(ptr + n->size, (void *)n)) {
		printf("%p\tMemory integrity check fail\n", ptr);
		if (n->magic == NODE_MAGIC) {
			printf("%p\tBut node magic is correct, node %p\n", ptr, (void *)n);
#ifdef CONFIG_MEM_DEBUG_RECORD_TASKID
			printf("%p\t%d\tra: ", (void *)ptr, n->taskid);
#else
			printf("%p\tra: ", (void *)ptr);
#endif
			print_ra(n->lr, CONFIG_MEM_DEBUG_LR_CNT);
		}
		return -1;
	} else {
#ifdef CONFIG_MEM_DEBUG_RECORD_TASKID
		if (usrdata != NULL && n->taskid != *(unsigned int *)usrdata)
			return 0;
		printf("%p\t%d\tra: ", (void *)ptr, n->taskid);
#else
		printf("%p\tra: ", (void *)ptr);
#endif
		print_ra(n->lr, CONFIG_MEM_DEBUG_LR_CNT);
	}

	return 0;
}

static int reset_nodes(void *ptr, struct node_info *n, void *usrdata)
{
	if (n->magic != NODE_MAGIC || check_magic(ptr + n->size, (void *)n)) {
		printf("%p\tMemory integrity check fail\n", ptr);
		if (n->magic == NODE_MAGIC) {
			printf("%p\tBut node magic is correct, node %p\n", ptr, (void *)n);
#ifdef CONFIG_MEM_DEBUG_RECORD_TASKID
			printf("%p\t%d\tra: ", (void *)ptr, n->taskid);
#else
			printf("%p\tra: ", (void *)ptr);
#endif
			print_ra(n->lr, CONFIG_MEM_DEBUG_LR_CNT);
		}
		return -1;
	} else {
		n->status &= ~(LEAK_DETECTING);
	}

	return 0;
}

static int print_leaks(void *ptr, struct node_info *n, void *usrdata)
{
	if (n->magic != NODE_MAGIC || check_magic(ptr + n->size, (void *)n)) {
		printf("%p\tMemory integrity check fail\n", ptr);
		if (n->magic == NODE_MAGIC) {
			printf("%p\tBut node magic is correct, node %p\n", ptr, (void *)n);
#ifdef CONFIG_MEM_DEBUG_RECORD_TASKID
			printf("%p\t%d\tra: ", (void *)ptr, n->taskid);
#else
			printf("%p\tra: ", (void *)ptr);
#endif
			print_ra(n->lr, CONFIG_MEM_DEBUG_LR_CNT);
		}
		return -1;
	} else if (n->status & LEAK_DETECTING) {
#ifdef CONFIG_MEM_DEBUG_RECORD_TASKID
		printf("%p\t%d\tra: ", (void *)ptr, n->taskid);
#else
		printf("%p\tra: ", (void *)ptr);
#endif
		print_ra(n->lr, CONFIG_MEM_DEBUG_LR_CNT);
		return -1;
	}

	return 0;
}

static int scan_pools(node_func_t func, void *usrdata)
{
	int i;

	taskENTER_CRITICAL();

	printf("System memory list:\n");
	scan_pool(gMM[ID_SYSMEM], func, usrdata);
	printf("System memory list finished!\n");

	for (i = 0; i < NR_POOL_MMZ; i++) {
		if (gMM[i] == NULL)
			continue;

		printf("MMZ(%d) memory check:\n", i);
		scan_pool(gMM[i], func, usrdata);
		printf("MMZ(%d) memory check finished!\n", i);
	}
	taskEXIT_CRITICAL();
	return 0;
}

static int mem_check(int argc, char **argv)
{
	scan_pools(check_nodes, NULL);
	return 0;
}

static int mem_list(int argc, char **argv)
{
	void *usrdata = NULL;
	unsigned int taskid = 0;

#ifdef CONFIG_MEM_DEBUG_RECORD_TASKID
	if (argc == 2) {
		long long val;
		val = strtoll(argv[1], NULL, 0);
		taskid = (unsigned int)val;
		usrdata = (void *)&taskid;
	}
#endif
	scan_pools(print_nodes, usrdata);
	return 0;
}

static int mem_leak_start(int argc, char **argv)
{
	gLeakDetect = 1;
	return 0;
}

static int mem_leak_stop(int argc, char **argv)
{
	gLeakDetect = 0;
	return 0;
}

static int mem_leak_reset(int argc, char **argv)
{
	gLeakDetect = 0;
	scan_pools(reset_nodes, NULL);
	return 0;
}

static int mem_leak_show(int argc, char **argv)
{
	gLeakDetect = 0;
	scan_pools(print_leaks, NULL);
	return 0;
}
#endif

CONSOLE_CMD(mem, NULL, NULL, CONSOLE_CMD_MODE_SELF, "memory status entry")
CONSOLE_CMD(free, "mem", mem_free, CONSOLE_CMD_MODE_SELF, "show dynamic memory status")
#ifdef CONFIG_MEM_DEBUG_MONITOR
CONSOLE_CMD(check, "mem", mem_check, CONSOLE_CMD_MODE_SELF, "check memory integrity")
CONSOLE_CMD(list, "mem", mem_list, CONSOLE_CMD_MODE_SELF, "list memory nodes")
CONSOLE_CMD(leakdetect, "mem", NULL, CONSOLE_CMD_MODE_SELF, "memory leak detect entry")
CONSOLE_CMD(reset, "leakdetect", mem_leak_reset, CONSOLE_CMD_MODE_SELF, "reset memory leak initial status")
CONSOLE_CMD(start, "leakdetect", mem_leak_start, CONSOLE_CMD_MODE_SELF, "start memory leak detect")
CONSOLE_CMD(stop, "leakdetect", mem_leak_stop, CONSOLE_CMD_MODE_SELF, "stop memory leak detect")
CONSOLE_CMD(show, "leakdetect", mem_leak_show, CONSOLE_CMD_MODE_SELF, "show leaked memory")
#endif
