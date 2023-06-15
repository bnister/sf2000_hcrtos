#include <machine/regdef.h>
#include <mips/hal.h>

/* define the Hazards barrier instruction after MMFC0/MMTC0 instruction for MIPS32R1 */

#define MMTC0(rt,rd,sel)                \
	.word   0x40800000 | (rt<<16) | (rd<<11)|sel

#define MMFC0(rt,rd,sel)                \
        .word   0x40000000 | (rt<<16) | (rd<<11)|sel

	.extern xISRStackTop
	.extern vTaskStartScheduler
	.extern main

	.global hardware_init_hook .text
	.global software_init_hook .text

	.align      4
	.section    .boot
	.set        noreorder
	.set        at

#--------------------------------------------------------------------
#
#   Function_Name: _start
#   Description: Initialize hardware, register, stack and so on
#   Arguments: None
#   Return Value: None
#
#--------------------------------------------------------------------
#	.text
	.align  4
	.globl  _start
	.ent    _start

_start:
	nop
	MMFC0(8, 15, 1)
	andi    t0, t0, 0x3
	beqz    t0, 2f
	nop

	li      t0, 0xb880006b
	lb      t5, 0(t0)
	li      t1, 0x2
	bne     t5, t1, 0f
	nop
	jr      ra
	nop
0:
	move    t4, ra

	la      t0, _start
	lui     t1, 0xa000
	or      t0, t0, t1
	srl     t1, t0, 16
	li      t3, 0x3c080000
	or      t1, t1, t3
	li      t2, 0xb8800280
	sw      t1, 0(t2)

	andi    t1, t0, 0xffff
	li      t3, 0x35080000
	or      t1, t1, t3
	li      t2, 0xb8800284
	sw      t1, 0(t2)

	li      t0, 0xb8800288
	li      t1, 0x01000008
	sw      t1, 0(t0)

	li      t0, 0xb880028c
	li      t1, 0x00000000
	sw      t1, 0(t0)

	li      t0, 0xb8800220
	lw      t1, 0(t0)
	ori     t1, t1, 0x00000002
	sw      t1, 0(t0)

	li      a0, 10000
	jal     __usleep
	nop

	li      t0, 0xb880006b
	li      t1, 0x2
	lb      t2, 0(t0)
	beq     t2, t1, 1f

	li      t0, 0xb8800220
	lw      t1, 0(t0)
	li      t2, 0xfffffffd
	and     t1, t1, t2
	sw      t1, 0(t0)

	li      a0, 10000
	jal     __usleep
	nop

	ori     t1, t1, 0x00000002
	sw      t1, 0(t0)
1:
	li	t0, 0x1
	bne     t5, t0, 1f
	nop

	move    ra, t4
	jr      ra
	nop
1:
	nop
	.set	push
	.set	arch=r4000
	wait
	.set	pop
	nop
	b	1b
	nop
	.word   0x1000ffff
	nop
2:
# Step 1: Init CPU for boot up.
# CP0 CONFIG  bit map :
# bit[27:25]=0x3 : Kuseg and Useg Cacheable,
# noncoherent, write-back, write allocate
# bit[21]=1 : MIPS16 enable
# bit[17:16]=0x3  :  Instruction/Data next line buffer enable
# bit[15]=0x1 :  little endian
# bit[2:0]=0x3 : Kseg0   Cacheable, noncoherent, write-back, write allocate
	li      t0, 0xb880006b
	li      t1, 0x2
	sb      t1, 0(t0)

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
	jal     _invalidate_cache       # init. inst. caches
	nop
	li      a0, 1
	jal     _invalidate_cache       # init. data caches
	nop

# Step 6: Setup bss section.
	la      a0, _sbss               # First bss address
	la      a1, _ebss               # Last address
Clean_Loop:
	sw      zero, 0(a0)             # Clean first word
	sw      zero, 4(a0)             # Clean next word
	sw      zero, 8(a0)             # Clean next word
	sw      zero, 12(a0)            # Clean next word
	addu    a0, 16                  # Increment source address
	bltu    a0, a1, Clean_Loop      # Not at end? Continue with clean
	nop                             # Delay slot

# Step 7: Init the OS stack and other OS buffers.
	la      sp, xISRStackTop
	lw      sp, (sp)

# Step 8: Init the gp.
	la      gp, _gp

# Step 9: Init general exception entry and interrupt.
	# Controls the location of exception vectors is Bootstrap
	mfc0    v0, C0_SR
	HB
	li      v1, (0x1 << 22)
	or      v1, v0
	mtc0    v1, C0_SR
	HB
	la      t0, _osGeneralExcept
	srl     t0, 12
	sll     t0, 12
	MMTC0(8, 15, 1)                 # Store  exception handler into EBASE[31:12]
	HB
	nop

	mtc0    zero, C0_CAUSE
	HB

	li      t0, 0x10000000
	mtc0    t0, C0_SR
	HB
	HB
	nop

# Step 11: Jump to main()
	la      v0, hardware_init_hook
	beq     v0,zero,1f
	nop
	jalr    v0
	nop
1:
	la      v0, software_init_hook
	beq     v0,zero,1f
	nop
	jalr    v0
	nop

1:
	la      v0, main
	jalr    v0
	nop
	.end    _start

	.align  4
	.ent    __usleep
__usleep:
	mfc0    v1,$9
	HB
	li      v0,297
	mul     a0,a0, v0
	nor     a1,zero,v1
1:
	mfc0    v0,$9
	HB
	sltu    a2,v1,v0
	beqz    a2,3f
	nop
	subu    v0,v0,v1
2:
	sltu    v0,a0,v0
	beqz    v0,1b
	move    v0,zero
	jr      ra
	nop
3:
	b       2b
	addu    v0,a1,v0
	.end

	.set mips2

/*-------------------------------------------------------------------
**
** _invalidate_cache: invalidates the given cache
**
**      syntax: int _invalidate_cache(int cache)
**              cache = 0: ICACHE
**                      1: DCACHE
**              returns: v0: cache size in byte
**                       v1: cache line size in byte
**
**        note: This function forces itself into a kseg1 (uncached)
**              address. Caller must be in supervisor mode to call.
**
**              By reading s0, this function calculates
**              the corresponding cache size and cache line size,
**              and executes cache invalidating.
**
**              To accommodate the format of this function for R3K,
**              the cache size is returned, and saved.  The cache
**              set size is therefore derived by assuming a two-way
**              associate set.
**
**-------------------------------------------------------------------
*/
	.align  2
	.ent    _invalidate_cache
_invalidate_cache:
#--- transition to kseg1 from undetermined kernel segment
	la      t1, _inval_start_
	or      t1, 0x20000000          # modify to support 0x8FFFFFFF -> 0xAFFFFFFF & 0x9FFFFFFF -> 0xBFFFFFFF
	jr      t1
	nop
_inval_start_:
# Determine how big the I$ is
	.word   0x400a8001              # mfc0 $10, $16, 1
	bnez    a0,_inval_d_            # if not i-cache, invalidate d-cache
	nop
# Isolate I$ Line Size
	sll     $11, $10, 10
	srl     $11, 29

# Skip ahead if No I$
	beq     $11, $0, _inva_end_
	nop

	li      $14, 2
	sllv    $11, $14, $11           # Now have true I$ line size in bytes

	sll     $12, $10, 7
	srl     $12, 29
	li      $14, 64
	sllv    $12, $14, $12           # I$ Sets per way

	sll     $13, $10, 13
	srl     $13, 29                 # I$ Assoc (-1)
	add     $13, 1
	mul     $12, $12, $13           # Total number of sets

	lui     $14, 0x8000             # Get a KSeg0 address for cacheops

# Clear TagLo/TagHi registers
	mtc0    $0, $28
	ehb
	mtc0    $0, $29
	ehb

	move    $15, $12

# Index Store Tag Cache Op
# Will invalidate the tag entry, clear the lock bit, and clear the LRF bit
1:
	.set mips3
	cache   0x8, 0($14)
	.set mips1
	addiu   $15, -1                 # Decrement set counter

	add     $14, $11                # Get next line address
	bne     $15, $0, 1b
	nop

# Now go through and invalidate the D$
# Now that the I$ has been flushed, the rest of the code can be
# moved to kseg0 and run from the cache to go faster
_inval_d_:
	addi    a0,-1
	bnez    a0, _inva_end_

# Isolate D$ Line Size
	sll     $11, $10, 19
	srl     $11, 29

# Skip ahead if No D$
	beq     $11, $0, _inva_end_
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
	ehb
	mtc0    $0, $29
	ehb
	MMTC0(0,28,2)
	MMTC0(0,29,2)
	ehb

	move    $15, $12

# Index Store Tag Cache Op
# Will invalidate the tag entry, clear the lock bit, and clear the LRF bit
1:	
	.set mips3
	cache   0x9, 0($14)
	.set mips1
	addiu   $15, -1                 # Decrement set counter

	add     $14, $11                # Get next line address
	bne     $15, $0, 1b
	nop
_inva_end_:
	jr       ra
	nop
	.end _invalidate_cache

#--------------------------------------------------------------------
#
#   Function_Name: _osGeneralExcept
#   Description: Setup jump instruction, need copy them to INT_BASE
#   Arguments: None
#   Return Value: None
#
#--------------------------------------------------------------------
#	.text
	.section        .int_head
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
