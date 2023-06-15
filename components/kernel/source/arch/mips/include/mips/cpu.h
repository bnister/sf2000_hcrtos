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

#ifndef _CPU_H_
#define _CPU_H_

#include <mips/m32c0.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__ASSEMBLER__)
/*
 * Generic MIPS cache handling
 *
 *   primary: virtual index, physical tag, write back;
 *   secondary: physical index, physical tag, write back;
 *   pass correct virtual address to primary cache routines.
 */

/*
 * Coprocessor 0 register manipulation
 * Warning: all non-atomic in face of interrupts.
 */
#if defined(_mips_mfc0)

/* exchange (swap) VAL and cp0 register REG */
#define _mips_mxc0(reg, val) \
__extension__ ({ \
    register reg32_t __o; \
    __o = _mips_mfc0 (reg); \
    _mips_mtc0 (reg, (val)); \
    __o; \
})

/* bit clear non-zero bits from CLR in cp0 register REG */
#define _mips_bcc0(reg, clr) \
__extension__ ({ \
    register reg32_t __o; \
    __o = _mips_mfc0 (reg); \
    _mips_mtc0 (reg, __o & ~(clr)); \
    __o; \
})

/* bit set non-zero bits from SET in cp0 register REG */
#define _mips_bsc0(reg, set) \
__extension__ ({ \
    register reg32_t __o; \
    __o = _mips_mfc0 (reg); \
    _mips_mtc0 (reg, __o | (set)); \
    __o; \
})

/* bit clear nz bits in from CLR and set nz bits from SET in REG */
#define _mips_bcsc0(reg, clr, set) \
__extension__ ({ \
    register reg32_t __o; \
    __o = _mips_mfc0 (reg); \
    _mips_mtc0 (reg, (__o & ~(clr)) | (set)); \
    __o; \
})

/*
 * Standard MIPS CP0 register access functions
 */

/* CP0 Status register (NOTE: not atomic operations) */
#define mips_getsr()		_mips_mfc0(C0_SR)
#define mips_setsr(v)		_mips_mtc0(C0_SR,v)
#define mips_xchsr(v)		_mips_mxc0(C0_SR,v)
#define mips_bicsr(clr)		_mips_bcc0(C0_SR,clr)
#define mips_bissr(set)		_mips_bsc0(C0_SR,set)
#define mips_bcssr(c,s)		_mips_bcsc0(C0_SR,c,s)

/* CP0 Cause register (NOTE: not atomic operations) */
#define mips_getcr()		_mips_mfc0(C0_CR)
#define mips_setcr(v)		_mips_mtc0(C0_CR,v)
#define mips_xchcr(v)		_mips_mxc0(C0_CR,v)
#define mips_biccr(clr)		_mips_bcc0(C0_CR,clr)
#define mips_biscr(set)		_mips_bsc0(C0_CR,set)
#define mips_bcscr(c,s)		_mips_bcsc0(C0_CR,c,s)

/* CP0 PrID register */
#define mips_getprid()		_mips_mfc0(C0_PRID)

#ifdef C0_COUNT
/* CP0 Count register */
#define mips_getcount()		_mips_mfc0(C0_COUNT)
#define mips_setcount(v)	_mips_mtc0(C0_COUNT,v)
#define mips_xchcount(v)	_mips_mxc0(C0_COUNT,v)
#endif

#ifdef C0_COMPARE
/* CP0 Compare register*/
#define mips_getcompare()	_mips_mfc0(C0_COMPARE)
#define mips_setcompare(v)	_mips_mtc0(C0_COMPARE,v)
#define mips_xchcompare(v)	_mips_mxc0(C0_COMPARE,v)
#endif

#ifdef C0_CONFIG
/* CP0 Config register */
#define mips_getconfig()	_mips_mfc0(C0_CONFIG)
#define mips_setconfig(v)	_mips_mtc0(C0_CONFIG,v)
#define mips_xchconfig(v)	_mips_mxc0(C0_CONFIG,v)
#define mips_bicconfig(c)	_mips_bcc0(C0_CONFIG,c)
#define mips_bisconfig(s)	_mips_bsc0(C0_CONFIG,s)
#define mips_bcsconfig(c,s)	_mips_bcsc0(C0_CONFIG,c,s)
#endif

#ifdef C0_ECC
/* CP0 ECC register */
#define mips_getecc()		_mips_mfc0(C0_ECC)
#define mips_setecc(x)		_mips_mtc0(C0_ECC, x)
#define mips_xchecc(x)		_mips_mxc0(C0_ECC, x)
#endif

#ifdef C0_TAGLO
/* CP0 TagLo register */
#define mips_gettaglo()		_mips_mfc0(C0_TAGLO)
#define mips_settaglo(x)	_mips_mtc0(C0_TAGLO, x)
#define mips_xchtaglo(x)	_mips_mxc0(C0_TAGLO, x)
#endif

#ifdef C0_TAGHI
/* CP0 TagHi register */
#define mips_gettaghi()		_mips_mfc0(C0_TAGHI)
#define mips_settaghi(x)	_mips_mtc0(C0_TAGHI, x)
#define mips_xchtaghi(x)	_mips_mxc0(C0_TAGHI, x)
#endif

#ifdef C0_WATCHLO
/* CP0 WatchLo register */
#define mips_getwatchlo()	_mips_mfc0(C0_WATCHLO)
#define mips_setwatchlo(x)	_mips_mtc0(C0_WATCHLO, x)
#define mips_xchwatchlo(x)	_mips_mxc0(C0_WATCHLO, x)
#endif

#ifdef C0_WATCHHI
/* CP0 WatchHi register */
#define mips_getwatchhi()	_mips_mfc0(C0_WATCHHI)
#define mips_setwatchhi(x)	_mips_mtc0(C0_WATCHHI, x)
#define mips_xchwatchhi(x)	_mips_mxc0(C0_WATCHHI, x)
#endif

#endif /*_mips_mfc0*/

/*
 * Default versions of get/put for any MIPS CPU.
 */
#ifndef mips_get_byte
#define mips_get_byte(addr, errp)	(*(volatile unsigned char *)(addr))
#define mips_get_half(addr, errp)	(*(volatile unsigned short *)(addr))
#define mips_get_word(addr, errp)	(*(volatile unsigned int *)(addr))
#define mips_get_dword(addr, errp)	(*(volatile unsigned long long *)(addr))

#define mips_put_byte(addr, v)	(*(volatile unsigned char *)(addr)=(v))
#define mips_put_half(addr, v)	(*(volatile unsigned short *)(addr)=(v))
#define mips_put_word(addr, v)	(*(volatile unsigned int *)(addr)=(v))
#define mips_put_dword(addr, v)	(*(volatile unsigned long long *)(addr)=(v))
#endif /* mips_get_byte */

/* unoptimisable 2 instruction loop */

#define mips_cycle(count)				\
    do {						\
      unsigned int __count = (count);			\
      __asm__ volatile ("%(nop; nop; 1: bnez %0,1b; subu %0,1%)"	\
	: "+d" (__count)); 				\
    } while (0)

/* default implementation of _mips_intdisable is a function */

#endif /* !ASSEMBLER */

#ifdef __cplusplus
}
#endif
#endif /*_CPU_H_*/
