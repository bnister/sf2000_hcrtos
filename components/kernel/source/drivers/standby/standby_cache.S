#include <machine/regdef.h>
#include <mips/hal.h>

	.extern		standby_stack_top
	.extern		standby_save_sp

	.globl		standby_prepare_enter
	.section	.standby.text
	.ent		standby_prepare_enter
standby_prepare_enter:
# switch context to un-cached space
	la		t0, run_in_uncache
	lui		t1, 0xA000
	ori		t1, 0x0000

	or		t0, t1
	j		t0
	nop

# load text into icache
run_in_uncache:
	la		t0, __STANDBY_TEXT_START
	la		t1, __STANDBY_TEXT_END
	lui		t3, 0x8FFF
	ori		t3, 0xFFFF

	and		t0, t3
	and		t1, t3
	nop

	.set		mips3
1:
	cache		Fill_I, 0x0(t0)
	cache		Fetch_Lock_I, 0x0(t0)
	sync
	.set		mips1
	addu		t0, a0
	nop
	bltu		t0, t1, 1b
	nop

# switch context to cached space
	la		t0, run_in_cache
	lui		t1, 0x8FFF
	ori		t1, 0xFFFF
	
	and		t0, t1
	j		t0
	nop

# load data into dcache
run_in_cache:
	la		t0, __STANDBY_DATA_START
	la		t1, __STANDBY_BSS_END
	li		t3, 1
	nop

1:
	lw		t2, 0(t0)
	andi		t2, 0x3
	addu		t3, t2
	addiu		t0, 4
	nop
	bltu		t0, t1, 1b
	nop

	la              t0, standby_save_sp
	sw              sp, (t0)
	la		sp, standby_stack_top
	lw		sp, (sp)

	jr		ra
	nop
	.end		standby_prepare_enter

	.globl		standby_prepare_exit
	.section	.standby.text
	.ent		standby_prepare_exit
standby_prepare_exit:
	la              t0, standby_save_sp
	lw              sp, (t0)
	jr		ra
	nop
	.end		standby_prepare_exit
