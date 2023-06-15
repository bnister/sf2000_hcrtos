#ifndef _LINUX_BITOPS_H
#define _LINUX_BITOPS_H

#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/bits.h>

#define BIT_WORD(nr)		((nr) / BITS_PER_LONG)
#define BITOP_WORD(nr)		((nr) / BITS_PER_LONG)
#define __DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define BITS_PER_BYTE		8
#define BITS_TO_LONGS(nr)	__DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

#define SZLONG_LOG 5
#define SZLONG_MASK 31UL
#define __LL		"ll	"
#define __SC		"sc	"
#define __INS		"ins	"
#define __EXT		"ext	"


static inline void set_bit(unsigned long nr, volatile unsigned long *addr)
{
	unsigned long *m = ((unsigned long *) addr) + (nr >> SZLONG_LOG);
	int bit = nr & SZLONG_MASK;
	unsigned long temp;

	do {
		__asm__ __volatile__(
		"	.set	mips3				\n"
		"	" __LL "%0, %1		# set_bit	\n"
		"	or	%0, %2				\n"
		"	" __SC	"%0, %1				\n"
		"	.set	mips0				\n"
		: "=&r" (temp), "+m" (*m)
		: "ir" (1UL << bit));
	} while (unlikely(!temp));
}

static inline void clear_bit(unsigned long nr, volatile unsigned long *addr)
{
	unsigned long *m = ((unsigned long *) addr) + (nr >> SZLONG_LOG);
	int bit = nr & SZLONG_MASK;
	unsigned long temp;

	do {
		__asm__ __volatile__(
		"	.set	mips3				\n"
		"	" __LL "%0, %1		# clear_bit	\n"
		"	and	%0, %2				\n"
		"	" __SC "%0, %1				\n"
		"	.set	mips0				\n"
		: "=&r" (temp), "+m" (*m)
		: "ir" (~(1UL << bit)));
	} while (unlikely(!temp));
}

#define __set_bit(nr, vaddr)	set_bit(nr, vaddr)
#define __clear_bit(nr, vaddr)	clear_bit(nr, vaddr)

static inline int test_bit(int nr, const volatile unsigned long *addr)
{
	return 1UL & (addr[BIT_WORD(nr)] >> (nr & (BITS_PER_LONG-1)));
}

static inline unsigned long __fls(unsigned long word)
{
	int num;
	
	__asm__(
	"	.set	push				\n"
	"	.set	mips32				\n"
	"	clz	%0, %1					\n"
	"	.set	mips2				\n"
	"	.set	pop					\n"
	: "=r" (num)
	: "r" (word));

	return 31 - num;
}

static inline unsigned long __ffs(unsigned long word)
{
	return __fls(word & -word);
}

#define ffz(x)  __ffs(~(x))

unsigned long find_next_zero_bit(const unsigned long *addr, unsigned long size, unsigned long offset);
unsigned long find_first_zero_bit(const unsigned long *addr, unsigned long size);
unsigned long find_first_bit(const unsigned long *addr, unsigned long size);
unsigned long find_next_bit(const unsigned long *addr, unsigned long size, unsigned long offset);

#define for_each_set_bit(bit, addr, size) \
	for ((bit) = find_first_bit((addr), (size));		\
	     (bit) < (size);					\
	     (bit) = find_next_bit((addr), (size), (bit) + 1))

/*
 * test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static inline int test_and_clear_bit(unsigned long nr, volatile unsigned long *addr)
{
	unsigned short bit = nr & SZLONG_MASK;
	unsigned long res;
	unsigned long *m = ((unsigned long *) addr) + (nr >> SZLONG_LOG);
	unsigned long temp;

	smp_mb__before_llsc();

	__asm__ __volatile__(
	"	.set	push					\n"
	"	.set	noreorder				\n"
	"	.set	mips3					\n"
	"1:	" __LL	"%0, %1		# test_and_clear_bit	\n"
	"	or	%2, %0, %3				\n"
	"	xor	%2, %3					\n"
	"	" __SC 	"%2, %1					\n"
	"	beqz	%2, 2f					\n"
	"	 and	%2, %0, %3				\n"
	"	.subsection 2					\n"
	"2:	b	1b					\n"
	"	 nop						\n"
	"	.previous					\n"
	"	.set	pop					\n"
	: "=&r" (temp), "=m" (*m), "=&r" (res)
	: "r" (1UL << bit), "m" (*m)
	: "memory");

	smp_llsc_mb();

	return res != 0;
}

static inline int test_and_set_bit(unsigned long nr, volatile unsigned long *addr)
{
	unsigned short bit = nr & SZLONG_MASK;
	unsigned long res;
	unsigned long *m = ((unsigned long *) addr) + (nr >> SZLONG_LOG);
	unsigned long temp;

	smp_mb__before_llsc();

	__asm__ __volatile__(
	"    .set    push                            \n"
	"    .set    noreorder                        \n"
	"    .set    mips3                            \n"
	"1:    " __LL "%0, %1        # test_and_set_bit    \n"
	"    or    %2, %0, %3                            \n"
	"    " __SC    "%2, %1                            \n"
	"    beqz    %2, 2f                            \n"
	"     and    %2, %0, %3                        \n"
	"    .subsection 2                            \n"
	"2:    b    1b                                    \n"
	"     nop                                    \n"
	"    .previous                                \n"
	"    .set    pop                                \n"
	: "=&r" (temp), "=m" (*m), "=&r" (res)
	: "r" (1UL << bit), "m" (*m)
	: "memory");

	smp_llsc_mb();

	return res != 0;
}

static inline unsigned int generic_hweight32(unsigned int w)
{
        unsigned int res = (w & 0x55555555) + ((w >> 1) & 0x55555555);
        res = (res & 0x33333333) + ((res >> 2) & 0x33333333);
        res = (res & 0x0F0F0F0F) + ((res >> 4) & 0x0F0F0F0F);
        res = (res & 0x00FF00FF) + ((res >> 8) & 0x00FF00FF);
        return (res & 0x0000FFFF) + ((res >> 16) & 0x0000FFFF);
}

static inline unsigned long generic_hweight64(unsigned long long w)
{
        return generic_hweight32((unsigned int)(w >> 32)) +
               generic_hweight32((unsigned int)w);
}

static inline unsigned long hweight_long(unsigned long w)
{
        return sizeof(w) == 4 ? generic_hweight32(w) : generic_hweight64(w);
}


static inline void __change_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);

	*p ^= mask;
}

#endif
