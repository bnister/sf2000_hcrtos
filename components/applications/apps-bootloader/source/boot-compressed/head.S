/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995 Waldorf Electronics
 * Written by Ralf Baechle and Andreas Busse
 * Copyright (C) 1995 - 1999 Ralf Baechle
 * Copyright (C) 1996 Paul M. Antoine
 * Modified for DECStation and hence R3000 support by Paul M. Antoine
 * Further modifications by David S. Miller and Harald Koerfgen
 * Copyright (C) 1999 Silicon Graphics, Inc.
 */

#include <machine/regdef.h>
#include <mips/hal.h>
#include "asm.h"
#include "regdef.h"

#define Hit_Writeback_D                  0x19        /* 6       1 */
#define MMTC0(rt,rd,sel)		\
	.word	0x40800000|(rt<<16)|(rd<<11)|sel

	.set noreorder
	.cprestore
	LEAF(_start)
_start:
	/* Save boot rom start args */
	move	s0, a0
	move	s1, a1
	move	s2, a2
	move	s3, a3

	nop
	mfc0    t0, C0_CONFIG
	HB
	li      t1, 0xF1FF7FF8          # Set to little endian
	li      t2, 0x06200003          # En-blocking. Enable KSEG0 cache, MIPS16
	and     t1, t0
	or      t1, t2
	li      t2, 0x00030000
	or      t1, t2
	mtc0    t1, C0_CONFIG
	HB

	mfc0    t0, $22                 # for CPU can't support lwl
	HB
	or      t0, 1
	mtc0    t0, $22
	HB

	li      a0, 0
	jal     _invalidate_cache
	nop

	li      a0, 1
	jal     _invalidate_cache
	nop

	/* Clear BSS */
	PTR_LA	a0, _edata
	PTR_LA	a2, _end
1:	sw	zero, 0(a0)
	bne	a2, a0, 1b
	addiu	a0, a0, 4

	PTR_LA	a0, (.heap)	     /* heap address */
	PTR_LA	sp, (.stack + 8192)  /* stack address */

	PTR_LA	ra, 2f
	PTR_LA	k0, decompress_kernel
	jr	k0
	nop
2:
	move	a0, s0
	move	a1, s1
	move	a2, s2
	move	a3, s3
	PTR_LI	k0, KERNEL_ENTRY
	jr	k0
	 nop
3:
	b	3b
	 nop
	END(_start)

/*
 * _invalidate_cache: invalidates the given cache
 *
 *      syntax: int _invalidate_cache(int cache)
 *              cache = 0: ICACHE
 *                      1: DCACHE
 *              returns: v0: cache size in byte
 *                       v1: cache line size in byte
 *
 *        note: This function forces itself into a kseg1 (uncached)
 *              address. Caller must be in supervisor mode to call.
 *
 *              By reading s0, this function calculates
 *              the corresponding cache size and cache line size,
 *              and executes cache invalidating.
 *
 *              To accommodate the format of this function for R3K,
 *              the cache size is returned, and saved.  The cache
 *              set size is therefore derived by assuming a two-way
 *              associate set.
 */

	.align  2
	.globl  _invalidate_cache
	.ent    _invalidate_cache
_invalidate_cache:

#--- transition to kseg1 from undetermined kernel segment
	la      t1, 1f
	or      t1, 0xa0000000
	jr      t1
	nop
1:

# Determine how big the I$ is
	.word 0x400a8001                # mfc0 $10, $16, 1
	nop
	nop
	nop
	nop
	nop

	bnez    a0,10f                  # if not i-cache, invalidate d-cache
	nop
# Isolate I$ Line Size
	sll     $11, $10, 10
	srl     $11, 29

# Skip ahead if No I$
	beq     $11, $0, 10f
	nop

	li      $14, 2
	sllv    $11, $14, $11           # Now have true I$ line size in bytes

	sll     $12, $10, 7
	srl     $12, 29
	li      $14, 64
	sllv    $12, $14, $12           # I$ Sets per way

	sll     $13, $10, 13
	srl     $13, 29                  # I$ Assoc (-1)
	add     $13, 1
	mul     $12, $12, $13            # Total number of sets

	lui     $14, 0x8000              # Get a KSeg0 address for cacheops

# Clear TagLo/TagHi registers
	mtc0    $0, $28
	nop
	nop
	nop
	nop
	nop
	mtc0    $0, $29
	nop
	nop
	nop
	nop
	nop

	move    $15, $12

# Index Store Tag Cache Op
# Will invalidate the tag entry, clear the lock bit, and clear the LRF bit
1:
	.set mips3
	cache   0x8, 0($14)
	.set mips1
	addiu   $15, -1                 # Decrement set counter

	bne    $15, $0, 1b
	add    $14, $11                 # Get next line address

# Now go through and invalidate the D$
# Now that the I$ has been flushed, the rest of the code can be
# moved to kseg0 and run from the cache to go faster
10:
	addi    a0,-1
	bnez    a0,10f

# Isolate D$ Line Size
	sll     $11, $10, 19
	srl     $11, 29

# Skip ahead if No D$
	beq     $11, $0, 10f
	nop

	li      $14, 2
	sllv    $11, $14, $11           # Now have true D$ line size in bytes

	sll     $12, $10, 16
	srl     $12, 29
	li      $14, 64
	sllv    $12, $14, $12           # D$ Sets per way

	sll     $13, $10, 22
	srl     $13, 29                 # D$ Assoc (-1)
	add     $13, 1

	mul     $12, $12, $13           # Get total number of sets

	lui     $14, 0x8000             # Get a KSeg0 address for cacheops

# Clear TagLo/TagHi registers
	mtc0    $0, $28
	nop
	nop
	nop
	nop
	nop
	mtc0    $0, $29
	nop
	nop
	nop
	nop
	nop
#    mtc0    $0, $28, 2
	MMTC0(0,28,2)
	nop
	nop
	nop
	nop
	nop
#    mtc0    $0, $29, 2
	MMTC0(0,29,2)
	nop
	nop
	nop
	nop
	nop

	move    $15, $12

# Index Store Tag Cache Op
# Will invalidate the tag entry, clear the lock bit, and clear the LRF bit
1:
	.set mips3
	cache   0x9, 0($14)
	.set mips1
	addiu   $15, -1                 # Decrement set counter

	bne     $15, $0, 1b
	add     $14, $11                # Get next line address
10:
	jr      ra
	nop

	.end    _invalidate_cache

	.globl  cache_flush
	.ent    cache_flush

cache_flush:
	addiu   sp, sp, -24
	sw      ra, 20(sp)
	#jal    arch_local_irq_disable
	nop

	blez    a1, FLUSH_END
	li      t2, 0xffffffe0  # Get start address
	and     t0, a0, t2
	add     t1, a0, a1      # Get end address
	addu    t1, -1
	and     t1, t2
FLUSH_D:
	.set    push
	.set    noreorder
	.set    mips3
	cache   Hit_Writeback_D, 0(t0)  # CacheOP instr:
	#   hi_writeback
	.set    pop
	bne     t0, t1, FLUSH_D # if flushed data len. < len
	addu    t0, a2          # next cache line

	nop
	.set	mips3
	sync     #synchronization barrier
	.set	mips1		
	nop

	lui     t2, 0xa000
	or      t1, t1, t2
	lw      t2, (t1)
	nop
	.set    push
	.set    noreorder
	.set    mips2
	sync
	.set    pop
	.set    mips1
	nop
	nop

FLUSH_END:
	lw      ra, 20(sp)
	jr      ra
	addiu   sp, sp, 24
	nop

	.end    cache_flush

	.comm .heap,BOOT_HEAP_SIZE,4
	.comm .stack,4096*2,4
