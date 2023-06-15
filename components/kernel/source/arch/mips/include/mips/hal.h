/*
 * Copyright 2014-2018, Imagination Technologies Limited and/or its
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

#ifndef _HAL_H
#define _HAL_H

#include <mips/asm.h>
#include <mips/m32c0.h>

#define CTX_REG(REGNO)	((SZREG)*((REGNO)-1))

#define CTX_AT		(CTX_REG(1))
#define CTX_V0		(CTX_REG(2))
#define CTX_V1		(CTX_REG(3))
#define CTX_A0		(CTX_REG(4))
#define CTX_A1		(CTX_REG(5))
#define CTX_A2		(CTX_REG(6))
#define CTX_A3		(CTX_REG(7))
#define CTX_T0		(CTX_REG(8))
#define CTX_T1		(CTX_REG(9))
#define CTX_T2		(CTX_REG(10))
#define CTX_T3		(CTX_REG(11))
#define CTX_T4		(CTX_REG(12))
#define CTX_T5		(CTX_REG(13))
#define CTX_T6		(CTX_REG(14))
#define CTX_T7		(CTX_REG(15))
#define CTX_S0		(CTX_REG(16))
#define CTX_S1		(CTX_REG(17))
#define CTX_S2		(CTX_REG(18))
#define CTX_S3		(CTX_REG(19))
#define CTX_S4		(CTX_REG(20))
#define CTX_S5		(CTX_REG(21))
#define CTX_S6		(CTX_REG(22))
#define CTX_S7		(CTX_REG(23))
#define CTX_T8		(CTX_REG(24))
#define CTX_T9		(CTX_REG(25))
#define CTX_K0		(CTX_REG(26))
#define CTX_K1		(CTX_REG(27))
#define CTX_GP		(CTX_REG(28))
#define CTX_SP		(CTX_REG(29))
#define CTX_FP		(CTX_REG(30))
#define CTX_RA		(CTX_REG(31))
#define CTX_EPC		(CTX_REG(32))
#define CTX_BADVADDR	(CTX_REG(33))
#define CTX_HI0		(CTX_REG(34))
#define CTX_LO0		(CTX_REG(35))
#define CTX_HILO_SIZE	(2*SZREG)
#define CTX_LINK	(CTX_REG(34)+CTX_HILO_SIZE)
#define CTX_STATUS	((CTX_REG(34))+CTX_HILO_SIZE+SZPTR)
#define CTX_CAUSE	((CTX_REG(34))+CTX_HILO_SIZE+SZPTR+4)
#define CTX_BADINSTR	((CTX_REG(34))+CTX_HILO_SIZE+SZPTR+8)
#define CTX_BADPINSTR	((CTX_REG(34))+CTX_HILO_SIZE+SZPTR+12)
#define CTX_SIZE	((CTX_REG(34))+CTX_HILO_SIZE+SZPTR+16)

#endif // _HAL_H
