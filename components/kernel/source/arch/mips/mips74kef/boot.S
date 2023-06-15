#include <machine/regdef.h>
#include <mips/hal.h>

/*
 * define the Hazards barrier instruction after MMFC0/MMTC0 instruction for CPU_MIPS74KF
 */
#define MMFC0(rt,rd,sel)		\
	.word	0x40000000|(rt<<16)|(rd<<11)|sel
#define MMTC0(rt,rd,sel)		\
	.word	0x40800000|(rt<<16)|(rd<<11)|sel

	.extern xISRStackTop
	.global hardware_init_hook .text
	.global software_init_hook .text
	.extern main

	.align      4
	.section    .boot
	.set        noreorder
	.set        at

#	.text
	.align  4
	.globl  _start
	.ent    _start

_start:
	nop

# Init CPU for boot up.

	/*
	 * MIPS 74Kf CPU Initialization
	 */

	.set noat

	/* Initial GPRs */
	addiu $1,  $0, 0x0
	addiu $2,  $1, 0x0
	addiu $3,  $2, 0x0
	addiu $4,  $3, 0x0
	addiu $5,  $4, 0x0
	addiu $6,  $5, 0x0
	addiu $7,  $6, 0x0
	addiu $8,  $7, 0x0
	addiu $9,  $8, 0x0
	addiu $10, $9, 0x0
	addiu $11, $10, 0x0
	addiu $12,  $11, 0x0
	addiu $13,  $12, 0x0
	addiu $14,  $13, 0x0
	addiu $15,  $14, 0x0
	addiu $16,  $15, 0x0
	addiu $17,  $16, 0x0
	addiu $18,  $17, 0x0
	addiu $19,  $18, 0x0
	addiu $20,  $19, 0x0
	addiu $21,  $20, 0x0
	addiu $22,  $21, 0x0
	addiu $23,  $22, 0x0
	addiu $24,  $23, 0x0
	addiu $25,  $24, 0x0
	addiu $26,  $25, 0x0
	addiu $27,  $26, 0x0
	addiu $28,  $27, 0x0
	addiu $29,  $28, 0x0
	addiu $30,  $29, 0x0
	addiu $31,  $30, 0x0
	.set at

	mfc0 t0, C0_CONFIG
	HB                              # Hazard Barrier
	li t1, ~0x7
	and t0, t1
	ori t0, t0, 3
	mtc0 t0, C0_CONFIG              # cacheable, write-back
	HB                              # Hazard Barrier

	MMFC0(8,16,7)
	ori     t0, 0x100               # set config7 ES[8] bit
	MMTC0(8,16,7)

	MMFC0(8,16,2)                   # 0x40088002, mfc0 t0, Config2
	HB
	li t1, 0x00001000               # Set Config2 bit 12 as one
	or t0, t1
	HB
	MMTC0(8,16,2)                   # 0x40888002, mtc0 t0, Config2
	HB

	/*
	 * Watchpoint debug is disabled by default.
	 * However, because MIPS 24KE/MIPS74KF processor supports up to 4 watchpoints,
	 * clearance of all 4 pairs of WatchLo/WatchHi registers is required
	 * if one wants to disable watch exception for assurance in initialization sequence.
	 */
	MMTC0(0,18,0)                   # Disable watch exception for Instruction 0
	HB                              # Hazard Barrier
	MMTC0(0,18,1)                   # Disable watch exception for Instruction 1
	HB                              # Hazard Barrier
	MMTC0(0,18,2)                   # Disable watch exception for Data 0
	HB                              # Hazard Barrier
	MMTC0(0,18,3)                   # Disable watch exception for Data 1
	HB                              # Hazard Barrier

	li t0, 0x7
	MMTC0(8,19,0)                   # Clear watch status bits
	HB                              # Hazard Barrier
	MMTC0(8,19,1)                   # Clear watch status bits
	HB                              # Hazard Barrier
	MMTC0(8,19,2)                   # Clear watch status bits
	HB                              # Hazard Barrier
	MMTC0(8,19,3)                   # Clear watch status bits
	HB                              # Hazard Barrier

	mtc0 zero, C0_CAUSE             # Clear cause register
	HB                              # Hazard Barrier
	mtc0 zero, C0_COUNT             # Clear count register
	HB                              # Hazard Barrier

/* INIT_I_CACHE */
	li      a0, 0
	jal     _invalidate_cache
	nop

/* INIT_D_CACHE */
	li      a0, 1
	jal     _invalidate_cache
	nop

# Setup bss section.
	la      a0, _sbss               # First bss address
	la      a1, _ebss               # Last address
Clean_Loop:
	sw      zero, 0(a0)             # Clear first word
	sw      zero, 4(a0)             # Clear next word
	sw      zero, 8(a0)             # Clear next word
	sw      zero, 12(a0)            # Clear next word
	addu    a0, 16                  # Increment source address
	bltu    a0, a1, Clean_Loop      # Not at end? Continue with clear
	nop                             # Delay slot

# Init the OS stack and other OS buffers.
	la      sp, xISRStackTop
	lw      sp, (sp)

# Init the gp.
	la      gp, _gp

# Init general exception entry and interrupt.
	mfc0    v0, C0_SR
	HB
	li      v1, 0x400000
	or      v1, v0
	mtc0    v1, C0_SR
	HB
	la      t0, _osGeneralExcept
	srl     t0, 12
	sll     t0, 12
	MMTC0(8, 15, 1)        # Stor exception handler into EBASE[31:12]
	HB
	nop

	mtc0    zero, C0_CAUSE
	HB

	li      t0, 0x11000000
	mtc0    t0, C0_SR
	HB
	HB
	nop

# Jump to main()
	la      v0, hardware_init_hook
	beq     v0, zero, 1f
	nop
	jalr    v0
	nop
1:
	la      v0, software_init_hook
	beq     v0, zero, 1f
	nop
	jalr    v0
	nop
1:
	la      v0, main
	jalr    v0
	nop
	.end    _start

	.set mips2
	.text

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
	HB

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
	HB
	mtc0    $0, $29
	HB

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
	HB
	mtc0    $0, $29
	HB
#    mtc0    $0, $28, 2
	MMTC0(0,28,2)
	HB
#    mtc0    $0, $29, 2
	MMTC0(0,29,2)
	HB

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


	.text
	.align  12
	.byte   0
	.align  8
	.byte   0
	.align  7
	.globl  _osGeneralExcept
	.ent    _osGeneralExcept

_osGeneralExcept:
	la      k0, _osGeneralExceptHdl
	j       k0
	nop
	.end    _osGeneralExcept
