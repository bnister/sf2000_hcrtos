#include <machine/regdef.h>
#include <mips/hal.h>

	.set    noreorder
	.text

	.set    noreorder
	.globl  arch_local_irq_disable
	.ent    arch_local_irq_disable
# disable all maskable interrupt
arch_local_irq_disable:
	mfc0    t0, C0_STATUS   # Get CP0 Status Register
	EHB
	li      t1, 6
	and     t1, t0          # Check if ECL or EXL is set
	bne     t1, zero, no_need_disable

	li      t1, ~SR_IE
	and     t0, t1
	mtc0    t0, C0_STATUS   # Clear IEC bit in C0_SR
	EHB

	# Clear IEC bit in C0_SR again! to avoid a interrupt is raised
	mtc0    t0, C0_STATUS
	EHB

no_need_disable:
	jr      ra
	nop

	.end    arch_local_irq_disable

	.globl  arch_local_irq_enable
	.ent    arch_local_irq_enable
arch_local_irq_enable:
	mfc0    t0, C0_STATUS           # Get CP0 Status Register
	EHB
	li      t1, 6
	and     t1, t0                  # Check if ECL or EXL is set
	bne     t1, zero, no_need_enable

	or      t0, SR_IE               # Set IEC bit in C0_SR
	mtc0    t0, C0_STATUS
	EHB

no_need_enable:
	jr      ra
	nop

	.end     arch_local_irq_enable

	.globl  arch_local_irq_save
	.ent    arch_local_irq_save
# disable all maskable interrupt
arch_local_irq_save:
	mfc0    t0, C0_STATUS   # Get CP0 Status Register
	EHB
	move    v0, t0
	li      t1, 6
	and     t1, t0          # Check if ECL or EXL is set
	bne     t1, zero, 1f

	li      t1, ~SR_IE
	and     t0, t1
	mtc0    t0, C0_STATUS   # Clear IEC bit in C0_SR
	EHB

	# Clear IEC bit in C0_SR again! to avoid a interrupt is raised
	mtc0    t0, C0_STATUS
	EHB

1:
	jr      ra
	nop

	.end    arch_local_irq_save

	.globl  arch_local_irq_restore
	.ent    arch_local_irq_restore
arch_local_irq_restore:
	move    t0, a0
	li      t1, 6
	and     t1, t0                  # Check if ECL or EXL is set
	bne     t1, zero, 1f

	li      t1, 1
	and     t1, t0
	beq     t1, zero, 1f

	mfc0    t0, C0_STATUS   # Get CP0 Status Register
	EHB

	or      t0, SR_IE               # Set IEC bit in C0_SR
	mtc0    t0, C0_STATUS
	EHB

1:
	jr      ra
	nop

	.end     arch_local_irq_restore
