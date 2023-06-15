#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/ld.h>
#include <kernel/lib/console.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mips/hal.h>
#include <mips/m32c0.h>
#include "mips_opcode.h"
#include "regnum.h"

uint32_t *pRaException = NULL;
uint32_t *pSpException = NULL;

#define	MIPS_JR_RA	0x03e00008	/* instruction code for jr ra */
#define	MIPS_JR_K0	0x03400008	/* instruction code for jr k0 */
#define	MIPS_ERET	0x42000018	/* instruction code for eret */

typedef unsigned int mips_reg_t;
typedef unsigned int vaddr_t;
typedef signed long intptr_t;

struct pt_regs {
	/* Pad bytes for argument save space on the stack. */
	unsigned long pad0[8];

	/* Saved main processor registers. */
	unsigned long regs[32];

	/* Saved special registers. */
	unsigned long cp0_status;
	unsigned long hi;
	unsigned long lo;

	unsigned long cp0_badvaddr;
	unsigned long cp0_cause;
	unsigned long cp0_epc;
	unsigned long __last[0];
} __aligned(8);

#define PTR_LA		la
#define LONG_S		sw
#define LONG_L		lw
#define LONGSIZE	4

#define __stringify_1(x...) #x
#define __stringify(x...) __stringify_1(x)

#define STR_PTR_LA    __stringify(PTR_LA)
#define STR_LONG_S    __stringify(LONG_S)
#define STR_LONG_L    __stringify(LONG_L)
#define STR_LONGSIZE  __stringify(LONGSIZE)

#define STORE_ONE_REG(r) \
    STR_LONG_S   " $" __stringify(r)",("STR_LONGSIZE"*"__stringify(r)")(%1)\n\t"

static inline __attribute__((__always_inline__)) void prepare_frametrace(struct pt_regs *regs)
{

	asm volatile(
		".set push\n\t"
		".set noat\n\t"
		/* Store $1 so we can use it */
		STR_LONG_S " $1,"STR_LONGSIZE"(%1)\n\t"
		/* Store the PC */
		"1: " STR_PTR_LA " $1, 1b\n\t"
		STR_LONG_S " $1,%0\n\t"
		STORE_ONE_REG(2)
		STORE_ONE_REG(3)
		STORE_ONE_REG(4)
		STORE_ONE_REG(5)
		STORE_ONE_REG(6)
		STORE_ONE_REG(7)
		STORE_ONE_REG(8)
		STORE_ONE_REG(9)
		STORE_ONE_REG(10)
		STORE_ONE_REG(11)
		STORE_ONE_REG(12)
		STORE_ONE_REG(13)
		STORE_ONE_REG(14)
		STORE_ONE_REG(15)
		STORE_ONE_REG(16)
		STORE_ONE_REG(17)
		STORE_ONE_REG(18)
		STORE_ONE_REG(19)
		STORE_ONE_REG(20)
		STORE_ONE_REG(21)
		STORE_ONE_REG(22)
		STORE_ONE_REG(23)
		STORE_ONE_REG(24)
		STORE_ONE_REG(25)
		STORE_ONE_REG(26)
		STORE_ONE_REG(27)
		STORE_ONE_REG(28)
		STORE_ONE_REG(29)
		STORE_ONE_REG(30)
		STORE_ONE_REG(31)
		/* Restore $1 */
		STR_LONG_L " $1,"STR_LONGSIZE"(%1)\n\t"
		".set pop\n\t"
		: "=m" (regs->cp0_epc)
		: "r" (regs->regs)
		: "memory");
}

static inline bool __isexcption(void)
{
	uint32_t status_cp0 = (uint32_t)mips32_getsr();
	if (status_cp0 & (SR_EXL | SR_ERL))
		return true;
	else
		return false;
}

static inline bool __isinterrupt(void)
{
	uint32_t casue_cp0 = (uint32_t)mips32_getcr();
	if (!(casue_cp0 & CR_XMASK))
		return true;
	else
		return false;
}

static bool
kdbpeek(vaddr_t addr, unsigned *valp,
    int (*printfn)(const char*, ...))
{
	if (addr & 3) {
		(*printfn)("kdbpeek: unaligned address 0x%08x\n", addr);
		/* We might have been called from DDB, so do not go there. */
		return false;
	} else if (addr == 0) {
		(*printfn)("kdbpeek: NULL\n");
		return false;
	} else {
		*valp = *(unsigned *)addr;
		return true;
	}
}

static mips_reg_t
kdbrpeek(vaddr_t addr, size_t n,
    int (*printfn)(const char*, ...))
{
	mips_reg_t rc = 0;

	if (addr & (n - 1)) {
		(*printfn)("kdbrpeek: unaligned address 0x%08x\n", addr);
		/* We might have been called from DDB, so do not go there. */
		//stacktrace();
		rc = -1;
	} else if (addr == 0) {
		(*printfn)("kdbrpeek: NULL\n");
		rc = 0xdeadfeed;
	} else {
		if (sizeof(mips_reg_t) == 8 && n == 8)
			rc = *(int64_t *)addr;
 		else
			rc = *(int32_t *)addr;
	}
	return rc;
}

static bool
is_sp_move_ins(unsigned instr)
{
	InstFmt i;

	i.word = instr;
	switch (i.JType.op) {
	case OP_ADDI:
	case OP_ADDIU:
#if !defined(__mips_o32)
	case OP_DADDI:
	case OP_DADDIU:
#endif
			/* look for stack pointer adjustment */
		if (i.IType.rs != _R_SP || i.IType.rt != _R_SP)
			return 0;
		if ((short)i.IType.imm > 0)
			return 0;
		return 1;
	}

	return 0;
}

static bool
is_ra_save_ins(unsigned instr)
{
	InstFmt i;

	i.word = instr;
	switch (i.JType.op) {
	case OP_SW:
	case OP_SD:
		if (i.IType.rs == _R_SP && i.IType.rt == _R_RA)
			return 1;
	}

	return 0;
}

/*
 * Do a stack backtrace.
 * (*printfn)()  prints the output to either the system log,
 * the console, or both.
 */
static void
stacktrace_subr(mips_reg_t a0, mips_reg_t a1, mips_reg_t a2, mips_reg_t a3,
    vaddr_t pc, vaddr_t sp, vaddr_t fp, vaddr_t ra, vaddr_t *save_ra, int ra_cnt, int omit_cnt,
    int (*printfn)(const char*, ...))
{
	vaddr_t va, subr;
	unsigned instr, mask;
	InstFmt i;
	int more, stksize;
	unsigned int frames =  0;
	int foundframesize = 0;
	mips_reg_t regs[32] = {
		[_R_ZERO] = 0,
		[_R_A0] = a0, [_R_A1] = a1, [_R_A2] = a2, [_R_A3] = a3,
		[_R_RA] = ra,
	};
	int index = 0, cnt = 0;
	bool spmove = 0, rasave = 0;;

/* Jump here when done with a frame, to start a new one */
loop:
	spmove = 0;
	rasave = 0;
	stksize = 0;
	subr = 0;
	mask = 1;
	if (frames++ > 100) {
		(*printfn)("\nstackframe count exceeded\n");
		/* return breaks stackframe-size heuristics with gcc -O2 */
		goto finish;	/*XXX*/
	}

	/* check for bad SP: could foul up next frame */
	if ((sp & (sizeof(sp)-1)) || (intptr_t)sp >= 0) {
		(*printfn)("SP 0x%08x: not in kernel\n", sp);
		ra = 0;
		subr = 0;
		goto done;
	}

	/* Check for bad PC */
	if (pc & 3 || (intptr_t)pc >= 0 || (intptr_t)pc >= (intptr_t)&__text_end) {
		(*printfn)("PC 0x%08x: not in kernel space\n", pc);
		ra = 0;
		goto done;
	}

	/*
	 * Find the beginning of the current subroutine by
	 * scanning backwards from the current PC for the end
	 * of the previous subroutine.
	 *
	 * XXX This won't work well because nowadays gcc is so
	 *     aggressive as to reorder instruction blocks for
	 *     branch-predict. (i.e. 'jr ra' wouldn't indicate
	 *     the end of subroutine)
	 */
	va = pc;
	do {
		va -= sizeof(int);
		if (va <= (vaddr_t)&__text_start)
			goto finish;
		if (!kdbpeek(va, &instr, printfn))
			return;
		if (instr == MIPS_ERET)
			goto mips3_eret;
		spmove |= is_sp_move_ins(instr);
		rasave |= is_ra_save_ins(instr);
	} while (!spmove || !rasave);

mips3_eret:
	/* skip over nulls which might separate .o files */
	instr = 0;
	while (instr == 0) {
		if (!kdbpeek(va, &instr, printfn))
			return;
		if (instr == 0)
			va += sizeof(int);
	}
	subr = va;

	/* scan forwards to find stack size and any saved registers */
	stksize = 0;
	more = 3;
	mask &= 0x40ff0001;	/* if s0-s8 are valid, leave then as valid */
	foundframesize = 0;
	for (va = subr; more; va += sizeof(int),
			      more = (more == 3) ? 3 : more - 1) {
		/* stop if hit our current position */
		if (va >= pc)
			break;
		if (!kdbpeek(va, &instr, printfn))
			return;
		i.word = instr;
		switch (i.JType.op) {
		case OP_SPECIAL:
			switch (i.RType.func) {
			case OP_JR:
			case OP_JALR:
				more = 2; /* stop after next instruction */
				break;

			case OP_ADD:
			case OP_ADDU:
			case OP_DADD:
			case OP_DADDU:
				if (!(mask & (1 << i.RType.rd))
				    || !(mask & (1 << i.RType.rt)))
					break;
				if (i.RType.rd != _R_ZERO)
					break;
				mask |= (1 << i.RType.rs);
				regs[i.RType.rs] = regs[i.RType.rt];
				if (i.RType.func >= OP_DADD)
					break;
				regs[i.RType.rs] = (int32_t)regs[i.RType.rs];
				break;

			case OP_SYSCALL:
			case OP_BREAK:
				more = 1; /* stop now */
				break;
			}
			break;

		case OP_REGIMM:
		case OP_J:
		case OP_JAL:
		case OP_BEQ:
		case OP_BNE:
		case OP_BLEZ:
		case OP_BGTZ:
			more = 2; /* stop after next instruction */
			break;

		case OP_COP0:
		case OP_COP1:
		case OP_COP2:
		case OP_COP3:
			switch (i.RType.rs) {
			case OP_BCx:
			case OP_BCy:
				more = 2; /* stop after next instruction */
			};
			break;

		case OP_SW:
		case OP_SD:
		{
			size_t size = (i.JType.op == OP_SW) ? 4 : 8;

			/* look for saved registers on the stack */
			if (i.IType.rs != _R_SP)
				break;
			switch (i.IType.rt) {
			case _R_A0: /* a0 */
			case _R_A1: /* a1 */
			case _R_A2: /* a2 */
			case _R_A3: /* a3 */
			case _R_S0: /* s0 */
			case _R_S1: /* s1 */
			case _R_S2: /* s2 */
			case _R_S3: /* s3 */
			case _R_S4: /* s4 */
			case _R_S5: /* s5 */
			case _R_S6: /* s6 */
			case _R_S7: /* s7 */
			case _R_S8: /* s8 */
			case _R_RA: /* ra */
				regs[i.IType.rt] =
				    kdbrpeek(sp + (int16_t)i.IType.imm, size, printfn);
				mask |= (1 << i.IType.rt);
				break;
			}
			break;
		}

		case OP_ADDI:
		case OP_ADDIU:
#if !defined(__mips_o32)
		case OP_DADDI:
		case OP_DADDIU:
#endif
			/* look for stack pointer adjustment */
			if (i.IType.rs != _R_SP || i.IType.rt != _R_SP)
				break;
			/* don't count pops for mcount */
			if (!foundframesize) {
				stksize = - ((short)i.IType.imm);
				foundframesize = 1;
			}
			break;
		}
	}
done:
	if (mask & (1 << _R_RA))
		ra = regs[_R_RA];

	if ((ra <= (vaddr_t)&__text_start) || (ra >= (vaddr_t)&__text_end))
		goto finish;

	if (index >= omit_cnt) {
		if (cnt < ra_cnt)
			save_ra[cnt++] = ra;
		else if (ra_cnt > 0)
			goto finish;
	}
	index++;

	(*printfn)("<0x%08x: +0x%x>\t(0x%08x, 0x%08x, 0x%08x, 0x%08x) ra 0x%08x sz %d\n",
		   sp, pc - subr, regs[_R_A0], regs[_R_A1], regs[_R_A2],
		   regs[_R_A3], ra, stksize);

	if (ra) {
		if (pc == ra && stksize == 0)
			(*printfn)("stacktrace: loop!\n");
		else {
			pc = ra;
			sp += stksize;
			ra = 0;
			goto loop;
		}
	}
finish:

	return;
}

static int dummy_printf(const char*ptr, ...)
{
	return 0;
}

int stacktrace_get_ra(unsigned int *save_ra, int ra_cnt, int omit_cnt)
{
	struct pt_regs regs;
	mips_reg_t a0, a1, a2, a3;
	vaddr_t pc, sp, fp, ra;

	memset(&regs, 0, sizeof(regs));

	prepare_frametrace(&regs);

	fp = 0;
	a0 = regs.regs[4];
	a1 = regs.regs[5];
	a2 = regs.regs[6];
	a3 = regs.regs[7];
	sp = regs.regs[29];
	ra = regs.regs[31];
	pc = regs.cp0_epc;

	stacktrace_subr(a0, a1, a2, a3, pc, sp, fp, ra, save_ra, ra_cnt, omit_cnt, dummy_printf);

	return 0;
}

typedef struct tskTaskControlBlock_internal {
	unsigned int *pxTopOfStack;
} tskTCB_internal;

void show_stack(TaskHandle_t task)
{
	struct pt_regs regs;
	mips_reg_t a0, a1, a2, a3;
	vaddr_t pc, sp, fp, ra;

	/*
	 * Remove any garbage that may be in regs (specially func
	 * addresses) to avoid show_raw_backtrace() to report them
	 */
	memset(&regs, 0, sizeof(regs));

	if (!__isexcption()) {
		if (task && task != xTaskGetCurrentTaskHandle()) {
			tskTCB_internal *pxTCB = (tskTCB_internal *)task;
			unsigned int *pxStack = pxTCB->pxTopOfStack;

			regs.regs[29] = (unsigned long)((void *)pxStack[28] + CTX_SIZE);
			regs.regs[31] = pxStack[30];
			regs.cp0_epc = pxStack[31];
		} else {
			prepare_frametrace(&regs);
		}
	} else {
		prepare_frametrace(&regs);
		if (__isinterrupt()) {
			printf("\r\n======= Stack Dump (before entering Interrupt mode) ======\r\n");
		} else {
			printf("\r\n======= Stack Dump (before entering Exception mode) ======\r\n");
			printf("Exception code = %lu\r\n", (mips32_getcr() & CR_XMASK) >> 2);
		}
		for (int i = 0; i < 32; i++) {
			printf("reg[%d]: 0x%x\n", i, (unsigned int)regs.regs[i]);
		}

		regs.regs[29] = (uint32_t)pSpException;
		regs.regs[31] = (uint32_t)pRaException;

		regs.cp0_epc = mips32_get_c0(C0_EPC);
		printf("reg[29]: 0x%x\n", (unsigned int)regs.regs[29]);
		printf("reg[31]: 0x%x\n", (unsigned int)regs.regs[31]);


	}

	fp = 0;
	a0 = regs.regs[4];
	a1 = regs.regs[5];
	a2 = regs.regs[6];
	a3 = regs.regs[7];
	sp = regs.regs[29];
	ra = regs.regs[31];
	pc = regs.cp0_epc;

	stacktrace_subr(a0, a1, a2, a3, pc, sp, fp, ra, NULL, 0, 0, printf);
}

static int dumpstack(int argc, char **argv)
{
	TaskHandle_t task = NULL;

	if (argc == 2 && !strcasecmp("all", argv[1])) {
		TaskStatus_t *pxTaskStatusArray;
		UBaseType_t x, uxArraySize = uxTaskGetNumberOfTasks();

		pxTaskStatusArray = malloc(uxArraySize * sizeof(TaskStatus_t));
		if (!pxTaskStatusArray) {
			show_stack(task);
			return 0;
		}

		uxArraySize = uxTaskGetSystemState(pxTaskStatusArray,
						   uxArraySize, NULL);
		for (x = 0; x < uxArraySize; x++) {
			printf("Task stack(%p)\n", pxTaskStatusArray[x].xHandle);
			show_stack(pxTaskStatusArray[x].xHandle);
		}

		free(pxTaskStatusArray);
		return 0;
	}

	if (argc == 2) {
		long long val;
		val = strtoll(argv[1], NULL, 16);
		task = (TaskHandle_t)(unsigned long)val;
	}

	show_stack(task);

	return 0;
}

CONSOLE_CMD(dumpstack, NULL, dumpstack, CONSOLE_CMD_MODE_SELF, "memory status entry")
