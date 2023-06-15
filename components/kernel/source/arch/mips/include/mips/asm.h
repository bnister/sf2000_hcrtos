/*
 * Copyright 2014-2017, Imagination Technologies Limited and/or its
 *                      affiliated group companies.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _MIPS_ASM_H_
#define _MIPS_ASM_H_

/*
 * asm.h: various macros to help assembly language writers
 */

/* ABI specific stack frame layout and manipulation. */
/* Standard O32 */
#define SZREG		4	/* saved register size */
#define	REG_S		sw	/* store saved register */
#define	REG_L		lw	/* load saved register */
#define SZARG		4	/* argument register size */
#define	NARGSAVE	4	/* arg register space on caller stack */
#define ALSZ		7	/* stack alignment - 1 */
#define ALMASK		(~7)	/* stack alignment mask */
#define LOG2_STACK_ALGN	3	/* log2(8) */
#define SZPTR		4	/* pointer size */
#define LOG2_SZPTR	2	/* log2(4) */
#define PTR_S		sw	/* store pointer */
#define PTR_L		lw	/* load pointer */
#define PTR_SUBU	subu	/* decrement pointer */
#define PTR_ADDU	addu	/* increment pointer */
#define PTR_MFC0	mfc0	/* access CP0 pointer width register */
#define PTR_MTC0	mtc0	/* access CP0 pointer width register */
#define LA		la	/* load an address */
#define PTR		.word	/* pointer type pseudo */

#endif /*_MIPS_ASM_H_*/
