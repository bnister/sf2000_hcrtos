#include <machine/regdef.h>
#include <mips/hal.h>

	.text
	.set    noreorder

 #--------------------------------------------------------------------
 #
 #   Function_Name: cache_flush_all
 #
 #--------------------------------------------------------------------
	.globl  cache_flush_all
	.ent    cache_flush_all

cache_flush_all:
	lui     t0, 0x8000
	addiu   t1, t0, 0x8000			# Start addess + dcache_set_size
	move    t2, t1
FLUSH_ALL_D:
	.set    mips3
	cache   Index_Writeback_Inv_D, 0x0(t0)
	.set    mips1
	addiu   t0, 16
	blt     t0, t2, FLUSH_ALL_D
	addiu   t3, 16          # delay slot

	nop
	.set    mips3
	sync     #synchronization barrier
	.set    mips1
	nop
	jr      ra
	nop
	.end    cache_flush_all

 #--------------------------------------------------------------------
 #
 #   Function_Name: cache_flush
 #
 #--------------------------------------------------------------------
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
	addiu   t0, 32          # next cache line

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

 #--------------------------------------------------------------------
 #
 #   Function_Name: cache_invalidate
 #
 #--------------------------------------------------------------------
	.globl  cache_invalidate
	.ent    cache_invalidate

cache_invalidate:
	blez    a1, INVALIDATE_END
	li      t2, 0xffffffe0  # Get start address
	and     t0, a0, t2
	add     t1, a0, a1      # Get end address
	addu    t1, -1
	and     t1, t2

INVALIDATE_D:
	.set    push
	.set    noreorder
	.set    mips3
	cache   Hit_Invalidate_D, 0(t0)  # CacheOP instr:
	#   hi_invalidate
	.set    pop
	bne     t0, t1, INVALIDATE_D     # if invalidated data len. < len
	addiu   t0, 32                   # next cache line

INVALIDATE_END:
	jr      ra
	nop

	.end    cache_invalidate

	.globl  cache_flush_invalidate
	.ent    cache_flush_invalidate

cache_flush_invalidate:
	addiu   sp, sp, -24
	sw      ra, 20(sp)
	#jal    arch_local_irq_disable
	nop

	blez    a1, FLUSH_INVALIDATE_END
	li      t2, 0xffffffe0  # Get start address
	and     t0, a0, t2
	add     t1, a0, a1      # Get end address
	addu    t1, -1
	and     t1, t2
FLUSH_INVALIDATE_D:
	.set    push
	.set    noreorder
	.set    mips3
	cache   Hit_Writeback_Inv_D, 0(t0)  # CacheOP instr:
	#   hi_writeback
	.set    pop
	bne     t0, t1, FLUSH_INVALIDATE_D # if flushed data len. < len
	addiu   t0, 32          # next cache line

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

FLUSH_INVALIDATE_END:
	lw      ra, 20(sp)
	jr      ra
	addiu   sp, sp, 24
	nop

	.end    cache_flush_invalidate

	.set    reorder
