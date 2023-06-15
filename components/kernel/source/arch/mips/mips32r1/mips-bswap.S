#include <machine/regdef.h>
#include <mips/hal.h>

	.set    noreorder
	.text

	.set    noreorder
	.globl  ________swap32
	.ent    ________swap32
________swap32:
	.word 0x70801010
	jr      ra
	nop
	.end    ________swap32

	.globl  ________swap16
	.ent    ________swap16
________swap16:
	.word 0x70801010
	jr      ra
	srl     v0, 16
	.end    ________swap16
