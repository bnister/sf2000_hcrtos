#ifndef __LINUX_BITMAP_H
#define __LINUX_BITMAP_H

#include <linux/bits.h>

#define BITMAP_FIRST_WORD_MASK(start) (~0UL << ((start) & (BITS_PER_LONG - 1)))
#define BITMAP_LAST_WORD_MASK(nbits) (~0UL >> (-(nbits) & (BITS_PER_LONG - 1)))

#define small_const_nbits(nbits) \
	(__builtin_constant_p(nbits) && (nbits) <= BITS_PER_LONG)

extern int __bitmap_full(const unsigned long *bitmap, int bits);
extern void bitmap_clear(unsigned long *map, int start, int nr);

static inline int bitmap_full(const unsigned long *src, int nbits)
{
	if (small_const_nbits(nbits))
		return ! (~(*src) & BITMAP_LAST_WORD_MASK(nbits));
	else
		return __bitmap_full(src, nbits);
}


#include <linux/bitops.h>
#include <string.h>


static inline void bitmap_zero(unsigned long *dst, int nbits)
{
	if (small_const_nbits(nbits)) {
		*dst = 0UL;
	} else {
		int len = BITS_TO_LONGS(nbits) * sizeof(unsigned long);

		memset(dst, 0, len);
	}
}

#endif /* __LINUX_BITMAP_H */
